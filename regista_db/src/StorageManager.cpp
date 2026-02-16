#include "StorageManager.h"
#include <rocksdb/write_batch.h>
#include <iostream>
#include <arpa/inet.h>



StorageManager::StorageManager(const std::string& db_path) {
    options.create_if_missing = true;
    options.create_missing_column_families = true;

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

StorageManager::~StorageManager() {
    db->DestroyColumnFamilyHandle(index_handle_);
    db->DestroyColumnFamilyHandle(data_handle_);
    db->DestroyColumnFamilyHandle(default_handle_);
    delete db;
}

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

std::string StorageManager::EncodeIndexKey(uint64_t id) {
    uint64_t reversed_id = UINT64_MAX - id; // subtract from max to appear at beginning

    // convert to big-endian
    uint64_t big_endian_id = htobe64(reversed_id);

    // return 8 raw bytes as string
    char buf[8];
    std::memcpy(buf, &big_endian_id, 8);
    return std::string(buf, 8);
}

std::pair<uint64_t, uint64_t> StorageManager::DecodeCompositeKey(const char* key) {
    uint64_t be_ts, be_id;
    std::memcpy(&be_ts, key, 8);
    std::memcpy(&be_id, key + 8, 8);

    return {
        UINT64_MAX - be64toh(be_ts),
        be64toh(be_id)
    };
}

uint64_t StorageManager::DecodeIndexKey(const char* key) {

    uint64_t value;
    std::memcpy(&value, key, 8);
    value = be64toh(value);

    return (UINT64_MAX - value);
}


bool StorageManager::StoreEntry(const registadb::RegistaObject& entry) {
    // prepare keys
    uint64_t entry_timestamp = static_cast<uint64_t>(entry.timestamp());
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

bool StorageManager::GetEntryById(int64_t id, registadb::RegistaObject* out_entry) {
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