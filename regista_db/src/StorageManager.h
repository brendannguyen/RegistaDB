#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <rocksdb/db.h>
#include "playbook.pb.h"

class StorageManager {
public:
    StorageManager(const std::string& db_path);
    ~StorageManager();

    static constexpr const char* kIndexCF = "index_cf";
    static constexpr const char* kDataCF = "data_cf";

    // Write: Saves data and creates the ID index
    bool StoreEntry(const registadb::LogEntry& entry);

    // Read: Finds data by ID using the index
    bool GetEntryById(int64_t id, registadb::LogEntry* out_entry);

    // Delete: Finds data by ID using the index and deletes
    bool DeleteEntryById(int64_t id);

    std::string EncodeFixed64(uint64_t value);
    uint64_t DecodeFixed64(const std::string& key);

private:
    rocksdb::DB* db;
    rocksdb::Options options;
    rocksdb::ColumnFamilyHandle* index_handle_ = nullptr;
    rocksdb::ColumnFamilyHandle* data_handle_ = nullptr;
    rocksdb::ColumnFamilyHandle* default_handle_ = nullptr;
};

#endif