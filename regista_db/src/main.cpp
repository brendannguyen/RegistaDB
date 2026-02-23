#include <csignal>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <pthread.h>
#include <drogon/drogon.h>
#include "MetricsExporter.hpp"
#include "StorageManager.h"
#include "RegistaServer.h"

std::atomic<bool> keep_running(true);
RegistaServer* g_regista_server = nullptr;

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
    g_regista_server = &server;

    unsigned int num_cores = std::thread::hardware_concurrency();
    std::cout << "System detected with " << num_cores << " cores." << std::endl;
    int drogon_thread_count = (num_cores > 1) ? (num_cores - 1) : 1;
    drogon::app().setThreadNum(drogon_thread_count);

    std::thread zmq_thread([&server, num_cores]() {
        if (num_cores > 1) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(0, &cpuset);
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
            std::cout << "[Affinity] ZMQ Engine pinned to Core 0" << std::endl;
        }
        std::cout << "RegistaDB Engine Started..." << std::endl;
        std::cout << "Ingest: 5555 | Query: 5556" << std::endl;
        server.Run();
    });

    drogon::app().registerPreRoutingAdvice([num_cores](const drogon::HttpRequestPtr &, 
                                                        drogon::AdviceCallback &&cb, 
                                                        drogon::AdviceChainCallback &&cccb) {
        if (num_cores > 1) {
            thread_local bool is_pinned = false;
            if (!is_pinned) {
                static std::atomic<int> next_core(1);
                int target = next_core++ % num_cores;
                if (target == 0) target = 1; // double check to keep Core 0 free for ZMQ

                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(target, &cpuset);
                pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
                
                std::cout << "[Affinity] Drogon worker thread initialized on Core " << target << std::endl;
                is_pinned = true;
            }
        }
        cccb(); // continue to the actual controller
    });

    std::cout << "RESTful: 8081" << std::endl;
    drogon::app().addListener("0.0.0.0", 8081).run();

    std::cout << "Shutting down..." << std::endl;
    
    keep_running = false; 
    server.Stop();

    if (zmq_thread.joinable()) {
        zmq_thread.join();
    }

    std::cout << "Shutting down metrics bridge..." << std::endl;
    StopMetricsBridge();

    std::cout << "Engine stopped cleanly." << std::endl;
    return 0;
}