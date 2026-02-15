#include "StorageManager.h"
#include <rocksdb/write_batch.h>
#include <iostream>

StorageManager::StorageManager(const std::string& db_path) {
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        std::cerr << "Failed to open RocksDB: " << status.ToString() << std::endl;
    }
}

StorageManager::~StorageManager() {
    delete db;
}

bool StorageManager::StoreEntry(const registadb::LogEntry& entry) {
    // prepare Keys
    std::string primary_key = "log_" + std::to_string(entry.timestamp());
    std::string index_key = "idx_id_" + std::to_string(entry.id());

    // serialize Data
    std::string serialized_data;
    entry.SerializeToString(&serialized_data);

    // atomic Write Batch
    rocksdb::WriteBatch batch;
    batch.Put(primary_key, serialized_data);
    batch.Put(index_key, primary_key); // Index points to the primary key

    rocksdb::Status s = db->Write(rocksdb::WriteOptions(), &batch);
    return s.ok();
}

bool StorageManager::GetEntryById(int32_t id, registadb::LogEntry* out_entry) {
    std::string index_key = "idx_id_" + std::to_string(id);
    std::string primary_key;

    // look up pointer in the index
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), index_key, &primary_key);
    if (!s.ok()) return false;

    // look up the actual data using pointer
    std::string serialized_data;
    s = db->Get(rocksdb::ReadOptions(), primary_key, &serialized_data);
    
    if (s.ok()) {
        return out_entry->ParseFromString(serialized_data);
    }
    return false;
}