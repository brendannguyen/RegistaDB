#include <csignal>
#include <atomic>
#include <cstdlib>
#include "MetricsExporter.hpp"
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

    std::string db_path = "../../data/registadb_store";
    bool enable_stats = false;

    // GET OPTIONS
    const char* env_path = std::getenv("REGISTADB_STORE_PATH");
    const char* env_stats = std::getenv("ENABLE_STATS");
    
    if (env_path) db_path = env_path;
    if (env_stats && (std::string(env_stats) == "true" || std::string(env_stats) == "1")) {
        enable_stats = true;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--path" && i + 1 < argc) {
            db_path = argv[++i];
        } else if (arg == "--stats") {
            enable_stats = true;
        } else if (arg == "--no-stats") {
            enable_stats = false;
        }
    }

    std::cout << "RocksDB Stats: " << (enable_stats ? "ENABLED" : "DISABLED") << std::endl;
    StorageManager storage(db_path, enable_stats);

    if (enable_stats) {
        StartMetricsBridge(storage.GetStats());
        std::cout << "Monitoring server active on port 8080" << std::endl;
    }

    RegistaServer server(storage, 5555, 5556);

    std::cout << "RegistaDB Engine Started..." << std::endl;
    std::cout << "Ingest: 5555 | Query: 5556" << std::endl;

    server.Run();

    std::cout << "Shutting down metrics bridge..." << std::endl;
    StopMetricsBridge();

    std::cout << "Engine stopped cleanly." << std::endl;
    return 0;
}