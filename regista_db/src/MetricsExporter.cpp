#include "MetricsExporter.hpp"
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/gauge.h>
#include <rocksdb/statistics.h>
#include <thread>
#include <chrono>
#include <atomic>

static std::atomic<bool> keep_running{true};
static std::unique_ptr<std::thread> worker_thread;

/**
 * @brief Starts a metrics bridge from RocksDB statistics to Prometheus exposer.
 * 
 * @param rocks_stats  The RocksDB statistics object to bridge.
 */
void StartMetricsBridge(std::shared_ptr<rocksdb::Statistics> rocks_stats) {
    if (!rocks_stats) return; // Safety check if stats are disabled

    // HTTP Exposer (Port 8080 is standard for metrics)
    static prometheus::Exposer exposer{"0.0.0.0:8080"};
    static auto registry = std::make_shared<prometheus::Registry>();

    // Metrics
    static auto& rocksdb_family = prometheus::BuildGauge()
        .Name("rocksdb_internal_stats")
        .Help("RocksDB Ticker Statistics")
        .Register(*registry);

    auto& read_bytes_gauge = rocksdb_family.Add({{"ticker", "bytes_read"}});
    auto& write_bytes_gauge = rocksdb_family.Add({{"ticker", "bytes_written"}});
    auto& stall_gauge = rocksdb_family.Add({{"ticker", "stall_micros"}});

    auto& cache_hit_gauge = rocksdb_family.Add({{"ticker", "block_cache_hit"}});
    auto& cache_miss_gauge = rocksdb_family.Add({{"ticker", "block_cache_miss"}});

    auto& memtable_hit_gauge = rocksdb_family.Add({{"ticker", "memtable_hit"}});
    auto& compaction_keys_gauge = rocksdb_family.Add({{"ticker", "compaction_keys_dropped"}});

    // register the registry with the HTTP server
    exposer.RegisterCollectable(registry);

    // polling thread
    std::thread([rocks_stats, &read_bytes_gauge, &write_bytes_gauge, &stall_gauge, &cache_hit_gauge, &cache_miss_gauge, &memtable_hit_gauge, &compaction_keys_gauge]() {
        while (true) {
            if (rocks_stats) {
                read_bytes_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::BYTES_READ)));
                
                write_bytes_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::BYTES_WRITTEN)));
                
                stall_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::STALL_MICROS)));

                cache_hit_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::BLOCK_CACHE_HIT)));

                cache_miss_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::BLOCK_CACHE_MISS)));

                memtable_hit_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::MEMTABLE_HIT)));

                compaction_keys_gauge.Set(static_cast<double>(
                    rocks_stats->getTickerCount(rocksdb::Tickers::COMPACTION_KEY_DROP_OBSOLETE)));
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }).detach(); // run in background
}

/**
 * @brief Stops the metrics bridge gracefully.
 * 
 */
void StopMetricsBridge() {
    keep_running = false; 
    if (worker_thread && worker_thread->joinable()) {
        worker_thread->join();
    }
}