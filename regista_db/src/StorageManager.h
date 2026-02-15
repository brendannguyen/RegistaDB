#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <rocksdb/db.h>
#include "playbook.pb.h"

class StorageManager {
public:
    StorageManager(const std::string& db_path);
    ~StorageManager();

    // Write: Saves data and creates the ID index
    bool StoreEntry(const registadb::LogEntry& entry);

    // Read: Finds data by ID using the index
    bool GetEntryById(int32_t id, registadb::LogEntry* out_entry);

private:
    rocksdb::DB* db;
    rocksdb::Options options;
};

#endif