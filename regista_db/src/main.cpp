#include <iostream>
#include <assert.h>
#include "rocksdb/db.h"

int main() {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;

    // Use a path relative to your project
    std::string db_path = "../data/registadb_store";
    
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    
    if (!status.ok()) {
        std::cerr << "RegistaDB failed to start: " << status.ToString() << std::endl;
        return 1;
    }

    std::cout << "RegistaDB Core is online. Storage initialized at: " << db_path << std::endl;

    // Cleanup
    delete db;
    return 0;
}