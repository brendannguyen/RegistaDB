#ifndef REGISTA_SERVER_H
#define REGISTA_SERVER_H

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "StorageManager.h"
#include "playbook.pb.h"

class RegistaServer {
public:
    RegistaServer(StorageManager& storage, int ingest_port, int query_port);
    void Run();
    void Stop();

private:
    StorageManager& storage_;
    zmq::context_t context_;
    zmq::socket_t ingest_socket_;
    zmq::socket_t query_socket_;
    bool running_;

    void HandleIngest();
    void HandleQuery();

protected:
    registadb::StatusCode PrepareEntry(registadb::RegistaObject& entry);
};

#endif