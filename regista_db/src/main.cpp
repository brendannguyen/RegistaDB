#include <iostream>
#include <string>
#include "rocksdb/db.h"
#include "playbook.pb.h" // This is the generated header

int main() {
    // 1. Create a "Play" using the Protobuf class
    registadb::LogEntry entry;
    entry.set_id(101);
    entry.set_category("TRANSFER");
    entry.set_content("Player transferred from Java to C++");
    entry.set_timestamp(123456789);

    // 2. Serialize it (turn it into a string of bytes)
    std::string serialized_data;
    entry.SerializeToString(&serialized_data);

    // 3. Open RocksDB (Simplified for this test)
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "../data/registadb_store", &db);

    if (!status.ok()) return 1;

    // 4. Save the Protobuf "Play" into RocksDB
    db->Put(rocksdb::WriteOptions(), "last_play", serialized_data);
    std::cout << "RegistaDB: Successfully stored a Protobuf message!" << std::endl;

    // 5. Read it back
    std::string read_value;
    status = db->Get(rocksdb::ReadOptions(), "last_play", &read_value);

    if (status.ok()) {
        registadb::LogEntry read_entry;
        // Turn the bytes back into a C++ object
        if (read_entry.ParseFromString(read_value)) {
            std::cout << "--- Verified Data from Disk ---" << std::endl;
            std::cout << "ID: " << read_entry.id() << std::endl;
            std::cout << "Category: " << read_entry.category() << std::endl;
            std::cout << "Content: " << read_entry.content() << std::endl;
        }
    } else {
        std::cerr << "Failed to retrieve data!" << std::endl;
    }


    delete db;
    return 0;
}