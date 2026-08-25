// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/BenchPaths.hpp>
#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/merge/AirTreeMerge.hpp>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <vector>

namespace {

using airtree::bench::BenchPaths;
using airtree::bench::query::QueryFixtureBase;

// Inputs come from BenchPaths::merge_buffers; Merge_fold is the pairwise fold over all N.
class AirTreeMerge : public QueryFixtureBase {
protected:
  const std::vector<std::vector<char>> *buffers_ = nullptr;

  void SetUp([[maybe_unused]] const ::benchmark::State &state) override {
    buffers_ = &BenchPaths::merge_buffers;
  }

  std::size_t inputBytes(std::size_t n) const {
    std::size_t total = 0;
    for (std::size_t i = 0; i < n && i < buffers_->size(); ++i) {
      total += (*buffers_)[i].size();
    }
    return total;
  }
};

BENCHMARK_DEFINE_F(AirTreeMerge, Merge_pair)(benchmark::State &state) {
  if (buffers_ == nullptr || buffers_->size() < 2) {
    state.SkipWithError("merge needs at least two buffers");
    return;
  }
  std::size_t last_result_bytes = 0;
  for (auto _ : state) {
    auto merged = airtree::merge::mergeAirTree((*buffers_)[0], (*buffers_)[1]);
    last_result_bytes = merged.size();
    benchmark::DoNotOptimize(merged.data());
    benchmark::ClobberMemory();
  }
  applyQueryId(state, QueryId::Merge_pair);
  applyResultSize(state, last_result_bytes);
  state.counters["N"] = 2;
  state.counters["InputBytes"] = static_cast<double>(inputBytes(2));
}
BENCHMARK_REGISTER_F(AirTreeMerge, Merge_pair);

BENCHMARK_DEFINE_F(AirTreeMerge, Merge_fold)(benchmark::State &state) {
  if (buffers_ == nullptr || buffers_->size() < 2) {
    state.SkipWithError("merge needs at least two buffers");
    return;
  }
  const std::size_t n = buffers_->size();
  std::size_t last_result_bytes = 0;
  for (auto _ : state) {
    std::vector<char> acc = (*buffers_)[0];
    for (std::size_t i = 1; i < n; ++i) {
      acc = airtree::merge::mergeAirTree(acc, (*buffers_)[i]);
    }
    last_result_bytes = acc.size();
    benchmark::DoNotOptimize(acc.data());
    benchmark::ClobberMemory();
  }
  applyQueryId(state, QueryId::Merge_fold);
  applyResultSize(state, last_result_bytes);
  state.counters["N"] = static_cast<double>(n);
  state.counters["InputBytes"] = static_cast<double>(inputBytes(n));
}
BENCHMARK_REGISTER_F(AirTreeMerge, Merge_fold);

} // namespace
