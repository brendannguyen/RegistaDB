#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <atomic>
#include <string>
#include <rocksdb/db.h>
#include "playbook.pb.h"


/**
 * @brief Manages all interactions with RocksDB, including storing, retrieving, and deleting entries. Implements a composite key structure for efficient time-based retrieval and an index for ID-based lookups.
 * 
 */
class StorageManager {
public:
    StorageManager(const std::string& db_path, bool enable_stats);
    ~StorageManager();

    static constexpr const char* kIndexCF = "index_cf";
    static constexpr const char* kDataCF = "data_cf";

    // Write: Saves data and creates the ID index
    bool StoreEntry(const registadb::Entry& entry);

    // Read: Finds data by ID using the index
    bool GetEntryById(int64_t id, registadb::Entry* out_entry);

    // Delete: Finds data by ID using the index and deletes
    bool DeleteEntryById(int64_t id);

    std::string EncodeCompositeKey(uint64_t timestamp, uint64_t id);
    std::string EncodeIndexKey(uint64_t id);
    uint64_t DecodeIndexKey(const char* key);
    std::pair<uint64_t, uint64_t> DecodeCompositeKey(const char* key);
    uint64_t ToEpochMicros(const google::protobuf::Timestamp& ts);

    uint64_t GetNextId() {
        return ++global_id_counter_;
    }
    void SetStartingId(uint64_t id) {
        global_id_counter_.store(id);
    }

    std::shared_ptr<rocksdb::Statistics> GetStats() const {
        return rocks_stats;
    }
private:
    rocksdb::DB* db;
    rocksdb::Options options;
    std::shared_ptr<rocksdb::Statistics> rocks_stats;
    rocksdb::ColumnFamilyHandle* index_handle_ = nullptr;
    rocksdb::ColumnFamilyHandle* data_handle_ = nullptr;
    rocksdb::ColumnFamilyHandle* default_handle_ = nullptr;

    std::atomic<uint64_t> global_id_counter_{1};

protected:
    rocksdb::Iterator* GetRawIndexIterator() {
        return db->NewIterator(rocksdb::ReadOptions(), index_handle_);
    }
    rocksdb::Iterator* GetRawDataIterator() {
        return db->NewIterator(rocksdb::ReadOptions(), data_handle_);
    }
};

#endif