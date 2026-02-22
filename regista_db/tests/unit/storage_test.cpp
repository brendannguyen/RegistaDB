#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <google/protobuf/util/time_util.h>
#include "StorageManager.h"

namespace fs = std::filesystem;

class StorageManagerTester : public StorageManager {
public:
    // Pull the constructor into scope
    using StorageManager::StorageManager;

    // Make the protected method public for our tests
    using StorageManager::GetRawIndexIterator;
    using StorageManager::GetRawDataIterator;
};

class StorageTest : public ::testing::Test {
protected:
    // This folder will be created specifically for the test
    std::string test_path = "./test_db_sandbox";
    StorageManagerTester* storage;

    // Runs BEFORE every test
    void SetUp() override {
        // Clean up any old crashed test data first
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }
        // Initialize your StorageManager pointing to the test path
        storage = new StorageManagerTester(test_path, false);
    }

    // Runs AFTER every test
    void TearDown() override {
        delete storage; // Closes RocksDB and releases file locks
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }
    }
};

// Test encoding and decoding keys
TEST_F(StorageTest, EncodingCompositeSymmetry) {
    uint64_t ts = 123456789;
    uint64_t id = 404;

    std::string key = storage->EncodeCompositeKey(ts, id);
    auto decoded = storage->DecodeCompositeKey(key.data());

    EXPECT_EQ(decoded.first, ts);
    EXPECT_EQ(decoded.second, id);
}

TEST_F(StorageTest, EncodingIndexSymmetry) {
    uint64_t id = 400;

    std::string key = storage->EncodeIndexKey(id);
    auto decoded = storage->DecodeIndexKey(key.data());

    EXPECT_EQ(decoded, id);
}

// Test reverse sorting of keys
TEST_F(StorageTest, NewestEntriesComeFirst) {
    // Store an "Old" entry
    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    google::protobuf::Timestamp now;
    now.set_seconds(micros / 1'000'000);
    now.set_nanos((micros % 1'000'000) * 1000);


    registadb::Entry old_obj;
    old_obj.set_id(1);
    old_obj.mutable_created_at()->CopyFrom(now);
    storage->StoreEntry(old_obj);

    // Wait a tiny bit and store a "New" entry
    std::this_thread::sleep_for(std::chrono::microseconds(2));
    micros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    now.set_seconds(micros / 1'000'000);
    now.set_nanos((micros % 1'000'000) * 1000);
    
    registadb::Entry new_obj;
    new_obj.set_id(2);
    new_obj.mutable_created_at()->CopyFrom(now);
    storage->StoreEntry(new_obj);

    // Create an iterator and check the first result
    auto it = storage->GetRawDataIterator(); 
    it->SeekToFirst();
    
    // The very first item must be the NEWEST one (ID 2)
    registadb::Entry first;
    first.ParseFromString(it->value().ToString());
    EXPECT_EQ(first.id(), 2);
    delete it;
}

// Test that we can store and retrieve binary data with null bytes without corruption
TEST_F(StorageTest, HandlesBinaryData) {
    std::string tricky_data = "Null\0Byte Test\0With\0Multiple\0Nulls";
    tricky_data += std::string("\0", 1); // Add a null byte at the end
    google::protobuf::Timestamp now =
    google::protobuf::util::TimeUtil::GetCurrentTime();

    registadb::Entry obj;
    obj.set_id(123);
    obj.mutable_created_at()->CopyFrom(now);
    obj.mutable_data()->set_bytes_value(tricky_data);

    storage->StoreEntry(obj);

    registadb::Entry retrieved;
    storage->GetEntryById(123, &retrieved);

    EXPECT_EQ(retrieved.data().bytes_value(), tricky_data);
    EXPECT_EQ(retrieved.data().bytes_value().size(), tricky_data.size());
}

// Test that composite key prevents collisions when rapidly ingesting entries with the same timestamp
TEST_F(StorageTest, RapidIngestionNoCollisions) {
    google::protobuf::Timestamp now =
    google::protobuf::util::TimeUtil::GetCurrentTime();
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        registadb::Entry obj;
        obj.set_id(i);
        obj.mutable_created_at()->CopyFrom(now);
        storage->StoreEntry(obj);
    }

    // Verify we didn't lose any records due to overwriting keys
    // using secondary key (id) to avoid collisions
    int found_count = 0;
    auto it = storage->GetRawDataIterator();
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        found_count++;
    }
    EXPECT_EQ(found_count, count);
    delete it;
}