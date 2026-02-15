#include <atomic>

#include "RegistaServer.h"

extern std::atomic<bool> keep_running;

RegistaServer::RegistaServer(StorageManager& storage, int ingest_p, int query_p)
    : storage_(storage), 
      context_(1),
      ingest_socket_(context_, zmq::socket_type::pull),
      query_socket_(context_, zmq::socket_type::rep),
      running_(true) 
{
    ingest_socket_.bind("tcp://*:" + std::to_string(ingest_p));
    query_socket_.bind("tcp://*:" + std::to_string(query_p));
}

void RegistaServer::Run() {
    // setup polling items
    zmq::pollitem_t items[] = {
        { static_cast<void*>(ingest_socket_), 0, ZMQ_POLLIN, 0 },
        { static_cast<void*>(query_socket_),  0, ZMQ_POLLIN, 0 }
    };
    try {
        while (keep_running) {
            // poll for 100ms
            zmq::poll(&items[0], 2, std::chrono::milliseconds(100));

            // handle Ingest (PUSH/PULL)
            if (items[0].revents & ZMQ_POLLIN) {
                HandleIngest();
            }

            // handle Query (REQ/REP)
            if (items[1].revents & ZMQ_POLLIN) {
                HandleQuery();
            }
        }
    } catch (const zmq::error_t& e) {
        // ignore error if "Interrupted"
        if (e.num() != EINTR) { 
            std::cerr << "ZMQ Error: " << e.what() << std::endl;
        }
    }
    
    std::cout << "Server loop stopped. Cleaning up sockets..." << std::endl;
}

void RegistaServer::HandleIngest() {
    zmq::message_t msg;
    if (ingest_socket_.recv(msg, zmq::recv_flags::none)) {
        registadb::LogEntry entry;
        if (entry.ParseFromArray(msg.data(), msg.size())) {
            storage_.StoreEntry(entry);
        }
    }
}

void RegistaServer::HandleQuery() {
    zmq::message_t msg;
    if (query_socket_.recv(msg, zmq::recv_flags::none)) {
        registadb::RegistaRequest envelope;
        envelope.ParseFromArray(msg.data(), msg.size());

        switch (envelope.payload_case()) {
            // ingest, send confirmation
            case registadb::RegistaRequest::kStoreRequest:
                storage_.StoreEntry(envelope.store_request());
                query_socket_.send(zmq::buffer("OK"), zmq::send_flags::none);
                break;
            // fetch entry
            case registadb::RegistaRequest::kFetchId: {
                registadb::LogEntry result;
                if (storage_.GetEntryById(envelope.fetch_id(), &result)) {
                    std::string serialized;
                    result.SerializeToString(&serialized);
                    query_socket_.send(zmq::buffer(serialized), zmq::send_flags::none);
                } else {
                    query_socket_.send(zmq::buffer("NOT_FOUND"), zmq::send_flags::none);
                }
                break;
            }

            default:
                query_socket_.send(zmq::buffer("UNKNOWN_CMD"), zmq::send_flags::none);
                break;
        }
    }
}