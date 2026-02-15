#include <iostream>
#include <fstream>
#include <string>
#include <zmq.hpp>
#include "rocksdb/db.h"
#include "playbook.pb.h" // The generated header

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PULL);
    socket.bind("tcp://*:5555"); // Wait for Java to connect

    std::cout << "RegistaDB Engine listening on port 5555..." << std::endl;

    while (true) {
        zmq::message_t request;
        auto res = socket.recv(request, zmq::recv_flags::none);
        
        registadb::LogEntry entry;
        entry.ParseFromArray(request.data(), request.size());

        std::cout << "Received via Network: " << entry.content() << std::endl;
        
        // Open RocksDB
        rocksdb::DB* db;
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::Status status = rocksdb::DB::Open(options, "../../data/registadb_store", &db);

        if (!status.ok()) {
            std::cerr << "RocksDB connection failed: " << status.ToString() << std::endl;
            return 1;
        }

        // Store the serialized data into RocksDB
        std::string serialized_data;
        entry.SerializeToString(&serialized_data);

        // We'll use the ID as the key
        std::string key = "entry_" + std::to_string(entry.id());
        status = db->Put(rocksdb::WriteOptions(), key, serialized_data);

        if (status.ok()) {
            std::cout << "Successfully committed Java message to RocksDB with key: " << key << std::endl;
        }

        delete db;
    }

}