#include <csignal>
#include <atomic>

#include "StorageManager.h"
#include "RegistaServer.h"

std::atomic<bool> keep_running(true);

/**
 * @brief Handles system signals (e.g., SIGINT) to gracefully shut down the server.
 * 
 * @param signal The received signal number.
 */
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[Signal] Interrupt received. Shutting down gracefully..." << std::endl;
        keep_running = false;
    }
}

/**
 * @brief Main entry point for the RegistaDB server application.
 * 
 * @return int Exit status code.
 */
int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::string db_path = "../../data/registadb_store"; // default

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--path" && i + 1 < argc) {
            db_path = argv[i + 1];
        }
    }

    StorageManager storage(db_path);
    RegistaServer server(storage, 5555, 5556);

    std::cout << "RegistaDB Engine Started..." << std::endl;
    std::cout << "Ingest: 5555 | Query: 5556" << std::endl;

    server.Run();
    return 0;
}