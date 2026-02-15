#include <csignal>
#include <atomic>

#include "StorageManager.h"
#include "RegistaServer.h"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[Signal] Interrupt received. Shutting down gracefully..." << std::endl;
        keep_running = false;
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    StorageManager storage("../../data/registadb_store");
    RegistaServer server(storage, 5555, 5556);

    std::cout << "RegistaDB Engine Started..." << std::endl;
    std::cout << "Ingest: 5555 | Query: 5556" << std::endl;

    server.Run();
    return 0;
}