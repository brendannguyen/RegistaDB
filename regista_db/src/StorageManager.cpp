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
}

StorageManager::~StorageManager() {
    db->DestroyColumnFamilyHandle(index_handle_);
    db->DestroyColumnFamilyHandle(data_handle_);
    db->DestroyColumnFamilyHandle(default_handle_);
    delete db;
}

std::string StorageManager::EncodeFixed64(uint64_t value) {
    value = UINT64_MAX - value; // subtract from max to appear at beginning

    // convert to big-endian
    uint64_t big_endian_val = htole64(value);

    // return 8 raw bytes as string
    return std::string(reinterpret_cast<char*>(&big_endian_val), sizeof(uint64_t));
}

uint64_t StorageManager::DecodeFixed64(const std::string& key) {
    uint64_t value;
    std::memcpy(&value, key.data(), sizeof(uint64_t));
    value = be64toh(value);

    return (UINT64_MAX - value);
}

bool StorageManager::StoreEntry(const registadb::LogEntry& entry) {
    // prepare keys
    uint64_t entry_timestamp = static_cast<uint64_t>(entry.timestamp());
    uint64_t entry_id = static_cast<uint64_t>(entry.id());
    std::string primary_key = EncodeFixed64(entry_timestamp);
    std::string index_key = EncodeFixed64(entry_id);

    // serialize data
    std::string serialized_data;
    entry.SerializeToString(&serialized_data);

    // atomic write batch
    rocksdb::WriteBatch batch;
    batch.Put(index_handle_, index_key, primary_key);
    batch.Put(data_handle_, primary_key, serialized_data);

    rocksdb::Status s = db->Write(rocksdb::WriteOptions(), &batch);
    return s.ok();
}

bool StorageManager::GetEntryById(int64_t id, registadb::LogEntry* out_entry) {
    uint64_t entry_id = static_cast<uint64_t>(id);
    std::string index_key = EncodeFixed64(entry_id);
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
    std::string index_key = EncodeFixed64(entry_id);
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