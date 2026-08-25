// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_QUERY_QUERYFIXTUREBASE_HPP
#define AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_QUERY_QUERYFIXTUREBASE_HPP

#include <airtree/bench/BenchPaths.hpp>
#include <benchmark/benchmark.h>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace airtree::bench::query {

inline std::vector<char> loadAirtreeFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Failed to open .airtree file: " + path);
  }
  return std::vector<char>((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());
}

class QueryFixtureBase : public benchmark::Fixture {
protected:
  std::vector<char> buffer_;

  void SetUp([[maybe_unused]] const ::benchmark::State &state) override {
    buffer_ = BenchPaths::query_buffer;
  }

  bool hasBuffer() const { return !buffer_.empty(); }

  void applyCommonCounters(benchmark::State &state) const {
    state.counters["BufferSize"] = static_cast<double>(buffer_.size());
  }

  // Query identity for consolidated reporting 
  enum class QueryId : int {
    TopK_k1 = 0,
    TopK_k5 = 1,
    TopK_k15 = 2,
    MinMax_getMin = 3,
    MinMax_getMax = 4,
    MinMax_getMinValue = 5,
    MinMax_getMaxValue = 6,
    Percentile_p50 = 7,
    Percentile_p90 = 8,
    Grid_steps8 = 9,
    BoundingBox_mid50 = 10,
    // ctor = construction only, cold = construction + call per iteration
    Percentile_p99 = 11,
    Percentile_ctor = 12,
    Percentile_cold_p50 = 13,
    Percentile_cold_p99 = 14,
    MinMax_ctor = 15,
    TopK_ctor = 16,
    CDF_ctor = 17,
    CDF_at_p50 = 18,
    CDF_cold_at_p50 = 19,
    BinBoundary_generate = 20,
    Merge_pair = 21,
    Merge_fold = 22,
  };

  void applyQueryId(benchmark::State &state, QueryId id) const {
    state.counters["query_id"] = static_cast<double>(static_cast<int>(id));
  }

  // n_bytes = size of the timed call's return value in bytes 
  void applyResultSize(benchmark::State &state, std::size_t n_bytes) const {
    state.counters["result_size_B"] = static_cast<double>(n_bytes);
  }
  
};

} // namespace airtree::bench::query

#endif // AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_QUERY_QUERYFIXTUREBASE_HPP