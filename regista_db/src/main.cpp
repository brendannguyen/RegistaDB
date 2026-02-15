#include <iostream>
#include <fstream>
#include <string>
#include <zmq.hpp>
#include "rocksdb/db.h"
#include "playbook.pb.h" // The generated header

// Global flag to handle graceful shutdown
bool keep_running = true;
void signal_handler(int s) { keep_running = false; }

int main() {
    // Signal Handler (Ctrl+C)
    signal(SIGINT, signal_handler);

    // Initialise RocksDB
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "../../data/registadb_store", &db);
    if (!status.ok()) {
        std::cerr << "RocksDB failed: " << status.ToString() << std::endl;
        return 1;
    }

    // Setup ZeroMQ
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PULL);
    socket.bind("tcp://*:5555");

    std::cout << "RegistaDB Engine is LIVE. Press Ctrl+C to stop." << std::endl;

    // Service Loop
    try {
        while (keep_running) {
            zmq::message_t message;
            // timeout until message received
            auto res = socket.recv(message, zmq::recv_flags::none);

            if (message.size() > 0) {
                registadb::LogEntry entry;
                if (entry.ParseFromArray(message.data(), message.size())) {
                    // store in rocksdb
                    std::string serialized;
                    entry.SerializeToString(&serialized);
                    std::string key = "log_" + std::to_string(entry.timestamp());

                    db->Put(rocksdb::WriteOptions(), key, serialized);

                    std::cout << "[STORED] ID: " << entry.id() << " Category: " << entry.category() << std::endl;
                }
            } 
        }
    } catch (const zmq::error_t& e) {
        // ignore error if "Interrupted"
        if (e.num() != EINTR) { 
            std::cerr << "ZMQ Error: " << e.what() << std::endl;
        }
    }
    
    // Cleanup
    std::cout << "\nShutting down RegistaDB safely..." << std::endl;
    delete db;
    return 0;
}