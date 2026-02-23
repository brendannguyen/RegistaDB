#include <atomic>

#include "RegistaServer.h"
#include <google/protobuf/util/time_util.h>

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
        while (keep_running && running_) {
            // poll for 100ms
            int rc = zmq::poll(&items[0], 2, std::chrono::milliseconds(100));

            if (rc == 0) continue;

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
    ingest_socket_.close();
    query_socket_.close();
    std::cout << "Engine sockets closed cleanly." << std::endl;
}

void RegistaServer::Stop() {
    running_ = false;
}

/**
 * @brief Prepares an entry for storage by setting server-side timestamps and generating an ID if not provided.
 * 
 * @param entry The entry to prepare.
 * @return true if the entry was successfully prepared.
 * @return false if there was an error preparing the entry.
 */
bool RegistaServer::PrepareEntry(registadb::Entry& entry) {
    // server-side timestamping
    uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    google::protobuf::Timestamp now;
    now.set_seconds(micros / 1'000'000);
    now.set_nanos((micros % 1'000'000) * 1000);

    entry.mutable_created_at()->CopyFrom(now);
    entry.mutable_updated_at()->CopyFrom(now);

    
    // id generation
    if (entry.id() == 0) {
        entry.set_id(storage_.GetNextId());
    }

    return true;
}

/**
 * @brief Handles incoming data on the ingest socket, prepares it, and stores it using StorageManager.
 * 
 */
void RegistaServer::HandleIngest() {
    zmq::message_t msg;
    if (ingest_socket_.recv(msg, zmq::recv_flags::none)) {
        registadb::Entry entry;
        if (entry.ParseFromArray(msg.data(), msg.size())) {
            if (PrepareEntry(entry)) {
                storage_.StoreEntry(entry);
            }
        }
    }
}

/**
 * @brief Sends a Response protobuf back to the client over the query socket.
 * 
 * @param resp The Response protobuf to send.
 */
void RegistaServer::SendResponse(const registadb::Response& resp) {
    std::string bytes = resp.SerializeAsString();
    query_socket_.send(zmq::buffer(bytes), zmq::send_flags::none);
}


/**
 * @brief Handles incoming requests on the query socket, processes them, and sends appropriate responses back to the client.
 * 
 */
void RegistaServer::HandleQuery() {
    zmq::message_t msg;

    if (!query_socket_.recv(msg, zmq::recv_flags::none)) {
        return; // no message
    }

    registadb::Request req;
    if (!req.ParseFromArray(msg.data(), msg.size())) {
        registadb::Response resp;
        resp.set_status(registadb::STATUS_INVALID_ARGUMENT);
        resp.set_message("Failed to parse Request protobuf");
        SendResponse(resp);
        return;
    } else {
        registadb::Response resp = ExecuteRequest(req);
        SendResponse(resp);
        return;
    }
  
}

registadb::Response RegistaServer::ExecuteRequest(const registadb::Request& req) {
    registadb::Response resp;

    switch (req.op()) {

        case registadb::OP_CREATE: {
            if (!req.has_entry()) {
                resp.set_status(registadb::STATUS_INVALID_ARGUMENT);
                resp.set_message("Missing entry for CREATE");
                break;
            }

            registadb::Entry entry = req.entry();

            if (!PrepareEntry(entry)) {
                resp.set_status(registadb::STATUS_INTERNAL_ERROR);
                resp.set_message("Unable to prepare entry for CREATE");
                break;
            }

            bool ok = storage_.StoreEntry(entry);

            if (!ok) {
                resp.set_status(registadb::STATUS_INTERNAL_ERROR);
                resp.set_message("Failed to store entry");
            } else {
                resp.set_status(registadb::STATUS_OK);
                resp.mutable_entry()->CopyFrom(entry);
            }
            break;
        }

        case registadb::OP_READ: {
            uint64_t id = req.id();

            registadb::Entry entry;

            if (storage_.GetEntryById(id, &entry)) {
                resp.set_status(registadb::STATUS_OK);
                resp.mutable_entry()->CopyFrom(entry);
            } else {
                resp.set_status(registadb::STATUS_NOT_FOUND);
                resp.set_message("Entry not found");
            }
            break;
        }

        case registadb::OP_UPDATE: {
            if (!req.has_entry()) {
                resp.set_status(registadb::STATUS_INVALID_ARGUMENT);
                resp.set_message("Missing entry for UPDATE");
                break;
            }

            registadb::Entry entry = req.entry();

            if (entry.id() == 0) {
                resp.set_status(registadb::STATUS_INVALID_ARGUMENT);
                resp.set_message("UPDATE requires a valid id");
                break;
            }

            registadb::Entry old_entry;
            if (!storage_.GetEntryById(entry.id(), &old_entry)) {
                resp.set_status(registadb::STATUS_NOT_FOUND);
                resp.set_message("Entry not found");
                break;
            }

            // Update timestamp
            uint64_t micros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            google::protobuf::Timestamp now;
            now.set_seconds(micros / 1'000'000);
            now.set_nanos((micros % 1'000'000) * 1000);
            entry.mutable_updated_at()->CopyFrom(now);
            entry.mutable_created_at()->CopyFrom(old_entry.created_at());

            bool ok = storage_.StoreEntry(entry);

            if (!ok) {
                resp.set_status(registadb::STATUS_INTERNAL_ERROR);
                resp.set_message("Failed to update entry");
            } else {
                resp.set_status(registadb::STATUS_OK);
                resp.mutable_entry()->CopyFrom(entry);
            }

            break;
        }

        case registadb::OP_DELETE: {
            uint64_t id = req.id();

            if (storage_.DeleteEntryById(id)) {
                resp.set_status(registadb::STATUS_OK);
            } else {
                resp.set_status(registadb::STATUS_NOT_FOUND);
                resp.set_message("Entry not found");
            }
            break;
        }

        default: {
            resp.set_status(registadb::STATUS_INVALID_ARGUMENT);
            resp.set_message("Unknown operation");
            break;
        }
    }
    return resp;
}