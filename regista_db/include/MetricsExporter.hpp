#ifndef METRICS_EXPORTER_HPP
#define METRICS_EXPORTER_HPP

#include <memory>
#include <rocksdb/statistics.h>

void StartMetricsBridge(std::shared_ptr<rocksdb::Statistics> rocks_stats);
void StopMetricsBridge();

#endif