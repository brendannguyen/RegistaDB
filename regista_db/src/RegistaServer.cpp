#include <atomic>

#include "RegistaServer.h"

extern std::atomic<bool> keep_running;

/**
 * @brief Construct a new Regista Server:: Regista Server object
 * 
 * @param storage StorageManager instance to use for data operations
 * @param ingest_p Ingest port number for receiving data
 * @param query_p Query port number for handling requests
 */
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

/**
 * @brief Runs the main server loop, handling both ingest and query requests.
 * 
 */
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

/**
 * @brief Prepares a RegistaObject for storage by setting timestamp and generating an ID if not present.
 * 
 * @param entry The RegistaObject to prepare.
 * @return registadb::StatusCode The status of the preparation operation.
 */
registadb::StatusCode RegistaServer::PrepareEntry(registadb::RegistaObject& entry) {
    // server-side timestamping
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    entry.set_timestamp(ms);
    
    // id generation
    if (entry.id() == 0) {
        entry.set_id(storage_.GetNextId());
    }

    // type validation
    switch (entry.type()) {
        // binary/text data
        case registadb::RegistaObject::STRING:
        case registadb::RegistaObject::JSON:
            if (entry.data_case() != registadb::RegistaObject::kBlob) {
                return registadb::StatusCode::ERR_TYPE_MISMATCH;
            }
            break;
        // sequential data
        case registadb::RegistaObject::LIST:
        case registadb::RegistaObject::VECTOR:
            if (entry.data_case() != registadb::RegistaObject::kList) {
                return registadb::StatusCode::ERR_TYPE_MISMATCH;
            }
            break;
        case registadb::RegistaObject::HASH:
            if (entry.data_case() != registadb::RegistaObject::kHash) {
                return registadb::StatusCode::ERR_TYPE_MISMATCH;
            }
            break;
        default:
            return registadb::StatusCode::ERR_UNKNOWN_TYPE;
    }

    return registadb::StatusCode::SUCCESS;
}

/**
 * @brief Handles incoming data on the ingest socket, prepares it, and stores it using StorageManager.
 * 
 */
void RegistaServer::HandleIngest() {
    zmq::message_t msg;
    if (ingest_socket_.recv(msg, zmq::recv_flags::none)) {
        registadb::RegistaObject entry;
        if (entry.ParseFromArray(msg.data(), msg.size())) {

            registadb::StatusCode status = PrepareEntry(entry);
            if (status == registadb::StatusCode::SUCCESS) {
                storage_.StoreEntry(entry);
            }
        }
    }
}

/**
 * @brief Handles incoming requests on the query socket, processes them, and sends appropriate responses back to the client.
 * 
 */
void RegistaServer::HandleQuery() {
    zmq::message_t msg;
    if (query_socket_.recv(msg, zmq::recv_flags::none)) {
        registadb::RegistaRequest envelope;
        envelope.ParseFromArray(msg.data(), msg.size());

        switch (envelope.payload_case()) {
            // ingest, send confirmation
            case registadb::RegistaRequest::kStoreRequest: {
                registadb::RegistaObject* store_req = envelope.mutable_store_request();

                registadb::StatusCode status = PrepareEntry(*store_req);
                if (status == registadb::StatusCode::SUCCESS) {
                    storage_.StoreEntry(*store_req);
                    query_socket_.send(zmq::buffer("OK"), zmq::send_flags::none);
                    break;
                } else {
                    query_socket_.send(zmq::buffer("ERR: " + std::to_string(status) + " " + registadb::StatusCode_Name(status)), zmq::send_flags::none);
                    break;
                }
            }

            // fetch entry
            case registadb::RegistaRequest::kFetchId: {
                registadb::RegistaObject result;
                if (storage_.GetEntryById(envelope.fetch_id(), &result)) {
                    std::string serialized;
                    result.SerializeToString(&serialized);
                    query_socket_.send(zmq::buffer(serialized), zmq::send_flags::none);
                } else {
                    query_socket_.send(zmq::buffer("NOT_FOUND"), zmq::send_flags::none);
                }
                break;
            }

            // delete entry
            case registadb::RegistaRequest::kDeleteId: {
                if (storage_.DeleteEntryById(envelope.delete_id())) {
                    query_socket_.send(zmq::buffer("OK"), zmq::send_flags::none);
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