#include <gtest/gtest.h>
#include <filesystem>
#include <atomic>
#include "RegistaServer.h"
#include "StorageManager.h"

namespace fs = std::filesystem;
std::atomic<bool> keep_running(true);
RegistaServer* g_regista_server = nullptr;

class ServerLogicTest : public ::testing::Test {
protected:
    // This folder will be created specifically for the test
    std::string test_path = "./test_db_sandbox";
    StorageManager* storage;

    // Runs BEFORE every test
    void SetUp() override {
        // Clean up any old crashed test data first
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }
        // Initialize your StorageManager pointing to the test path
        storage = new StorageManager(test_path, false);
    }

    // Runs AFTER every test
    void TearDown() override {
        delete storage; // Closes RocksDB and releases file locks
        if (fs::exists(test_path)) {
            fs::remove_all(test_path);
        }
    }
};

class RegistaServerTester : public RegistaServer {
public:
    using RegistaServer::RegistaServer; // Use the same constructor
    
    // This "lifts" the protected method into public for the test
    using RegistaServer::PrepareEntry; 
};

TEST_F(ServerLogicTest, PrepareEntry) {
    
    RegistaServerTester* server = new  RegistaServerTester(*storage, 0, 0);

    registadb::Entry obj;
    obj.set_id(500);
    
    // Ensure timestamp is 0 initially
    ASSERT_EQ(obj.created_at().seconds(), 0);
    
    // Call the protected method
    server->PrepareEntry(obj);
    
    // After preparation, timestamp should be set to a non-zero value (e.g., current time)
    ASSERT_NE(obj.created_at().seconds(), 0);

    delete server;
}