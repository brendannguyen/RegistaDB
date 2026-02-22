#include "StorageManager.h"
#include <rocksdb/write_batch.h>
#include <iostream>
#include <arpa/inet.h>
#include "rocksdb/statistics.h"


/**
 * @brief Construct a new Storage Manager:: Storage Manager object
 * 
 * @param db_path The path to the RocksDB database directory
 */
StorageManager::StorageManager(const std::string& db_path, bool enable_stats) {
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    if (enable_stats) {
        this->rocks_stats = rocksdb::CreateDBStatistics();
        options.statistics = this->rocks_stats;
        options.statistics->set_stats_level(rocksdb::StatsLevel::kExceptDetailedTimers);
    }

    // column families
    std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
    column_families.push_back({rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()});
    column_families.push_back({kIndexCF, rocksdb::ColumnFamilyOptions()});
    column_families.push_back({kDataCF, rocksdb::ColumnFamilyOptions()});

    // vector to hold the handles RocksDB will give back
    std::vector<rocksdb::ColumnFamilyHandle*> handles;

    rocksdb::Status status = rocksdb::DB::Open(options, db_path, column_families, &handles, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
    }

    // assign handle pointers
    default_handle_ = handles[0];
    index_handle_   = handles[1];
    data_handle_    = handles[2];

    // set global counter to last id in db
    std::unique_ptr<rocksdb::Iterator> it(db->NewIterator(rocksdb::ReadOptions(), index_handle_));
    it->SeekToFirst();
    if (it->Valid()) {
        uint64_t decoded_id = DecodeIndexKey(it->key().data());
        SetStartingId(decoded_id);
    } else {
        SetStartingId(0);
    }

}

/**
 * @brief Destroy the Storage Manager:: Storage Manager object
 * 
 */
StorageManager::~StorageManager() {
    db->DestroyColumnFamilyHandle(index_handle_);
    db->DestroyColumnFamilyHandle(data_handle_);
    db->DestroyColumnFamilyHandle(default_handle_);
    delete db;
}

/**
 * @brief Encodes a composite key using timestamp and ID, with reversed timestamp for reverse sorting in RocksDB.
 * 
 * @param timestamp The timestamp to encode in the key.
 * @param id The ID to encode in the key.
 * @return std::string The encoded composite key.
 */
std::string StorageManager::EncodeCompositeKey(uint64_t timestamp, uint64_t id) {
    uint64_t reversed_timestamp = UINT64_MAX - timestamp; // subtract from max to appear at beginning

    // convert to big-endian
    uint64_t big_endian_ts = htobe64(reversed_timestamp);
    uint64_t big_endian_id = htobe64(id);

    // return 16 raw bytes as string
    char buf[16];
    std::memcpy(buf, &big_endian_ts, 8); // first 8 bytes: time
    std::memcpy(buf + 8, &big_endian_id, 8); // last 8 bytes: id
    return std::string(buf, 16);
}

/**
 * @brief Encodes an index key using the given ID, with reversed ID for reverse sorting in RocksDB.
 * 
 * @param id The ID to encode in the key.
 * @return std::string The encoded index key.
 */
std::string StorageManager::EncodeIndexKey(uint64_t id) {
    uint64_t reversed_id = UINT64_MAX - id; // subtract from max to appear at beginning

    // convert to big-endian
    uint64_t big_endian_id = htobe64(reversed_id);

    // return 8 raw bytes as string
    char buf[8];
    std::memcpy(buf, &big_endian_id, 8);
    return std::string(buf, 8);
}

/**
 * @brief Decodes a composite key back into its original timestamp and ID components.
 * 
 * @param key The composite key to decode, expected to be 16 bytes long.
 * @return std::pair<uint64_t, uint64_t> The decoded timestamp and ID as a pair.
 */
std::pair<uint64_t, uint64_t> StorageManager::DecodeCompositeKey(const char* key) {
    uint64_t be_ts, be_id;
    std::memcpy(&be_ts, key, 8);
    std::memcpy(&be_id, key + 8, 8);

    return {
        UINT64_MAX - be64toh(be_ts),
        be64toh(be_id)
    };
}

/**
 * @brief Decodes an index key back into its original ID component.
 * 
 * @param key The index key to decode, expected to be 8 bytes long.
 * @return uint64_t The decoded ID.
 */
uint64_t StorageManager::DecodeIndexKey(const char* key) {

    uint64_t value;
    std::memcpy(&value, key, 8);
    value = be64toh(value);

    return (UINT64_MAX - value);
}

/**
 * @brief Converts a google::protobuf::Timestamp to microseconds since epoch.
 * 
 * @param ts The timestamp to convert.
 * @return uint64_t The timestamp in microseconds since epoch.
 */
uint64_t StorageManager::ToEpochMicros(const google::protobuf::Timestamp& ts) {
    return ts.seconds() * 1000000ULL + ts.nanos() / 1000ULL;
}


/**
 * @brief Stores an entry in RocksDB by creating a composite key for the data column family and an index entry for the ID column family, using a WriteBatch for atomicity.
 * 
 * @param entry The entry to store.
 * @return true if the entry was successfully stored.
 * @return false if there was an error storing the entry.
 */
bool StorageManager::StoreEntry(const registadb::Entry& entry) {
    // prepare keys
    uint64_t entry_timestamp = ToEpochMicros(entry.created_at());
    uint64_t entry_id = static_cast<uint64_t>(entry.id());
    std::string primary_key = EncodeCompositeKey(entry_timestamp, entry_id);
    std::string index_key = EncodeIndexKey(entry_id);

    // serialize data
    std::string serialized_data;
    entry.SerializeToString(&serialized_data);

    // atomic write batch
    rocksdb::WriteBatch batch;
    batch.Put(index_handle_, index_key, primary_key);
    batch.Put(data_handle_, primary_key, serialized_data);
    // std::cout << "WRITING TO DISK -> ID: " << entry.id()
    //             << " | index_key: " << entry_id << " | timestamp: " << entry.timestamp()
    //           << " | Content: " << entry.blob().substr(0, 30) << "..." << std::endl;
    rocksdb::Status s = db->Write(rocksdb::WriteOptions(), &batch);
    return s.ok();
}

/**
 * @brief Retrieves an entry from RocksDB by its ID.
 * 
 * @param id The ID of the entry to retrieve.
 * @param out_entry The output entry to populate with the retrieved data.
 * @return true if the entry was successfully retrieved.
 * @return false if there was an error retrieving the entry.
 */
bool StorageManager::GetEntryById(int64_t id, registadb::Entry* out_entry) {
    uint64_t entry_id = static_cast<uint64_t>(id);
    std::string index_key = EncodeIndexKey(entry_id);
    std::string primary_key;

    // look up pointer in the index
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), index_handle_, index_key, &primary_key);
    if (!s.ok()) return false;

    // look up the actual data using pointer
    std::string serialized_data;
    s = db->Get(rocksdb::ReadOptions(), data_handle_, primary_key, &serialized_data);
    
    if (s.ok()) {
        return out_entry->ParseFromString(serialized_data);
    }
    return false;
}

/**
 * @brief Deletes (tombstones) an entry from RocksDB by looking up the ID in the index column family to find the primary key, then deleting both the index and data entries atomically using a WriteBatch.
 * 
 * @param id The ID of the entry to delete.
 * @return true: The entry was successfully deleted.
 * @return false: There was an error deleting the entry.
 */
bool StorageManager::DeleteEntryById(int64_t id) {
    uint64_t entry_id = static_cast<uint64_t>(id);
    std::string index_key = EncodeIndexKey(entry_id);
    std::string primary_key;

    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), index_handle_, index_key, &primary_key);

    if (s.ok()) {
        rocksdb::WriteBatch batch;
        batch.Delete(index_handle_, index_key);
        batch.Delete(data_handle_, primary_key);

        rocksdb::Status s = db->Write(rocksdb::WriteOptions(), &batch);
        return s.ok();
    }
    return false;
}