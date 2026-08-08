// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/minmax/MinMax.hpp>
#include <benchmark/benchmark.h>

namespace {

using airtree::bench::query::QueryFixtureBase;

#define DEFINE_MINMAX_SUITE(SchemaClass)                                       \
  class SchemaClass : public QueryFixtureBase {};                              \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, MinMax_getMin)                               \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::minmax::MinMax q(buffer_);                                 \
    std::size_t last_result_bytes = 0;                                         \
    for (auto _ : state) {                                                     \
      auto r = q.getMin();                                                     \
      last_result_bytes =                                                      \
          r.size() * sizeof(typename decltype(r)::value_type);                 \
      benchmark::DoNotOptimize(r.data());                                      \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::MinMax_getMin);                               \
    applyResultSize(state, last_result_bytes);                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, MinMax_getMin);                            \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, MinMax_getMax)                               \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::minmax::MinMax q(buffer_);                                 \
    std::size_t last_result_bytes = 0;                                         \
    for (auto _ : state) {                                                     \
      auto r = q.getMax();                                                     \
      last_result_bytes =                                                      \
          r.size() * sizeof(typename decltype(r)::value_type);                 \
      benchmark::DoNotOptimize(r.data());                                      \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::MinMax_getMax);                               \
    applyResultSize(state, last_result_bytes);                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, MinMax_getMax);                            \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, MinMax_getMinValue)                          \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::minmax::MinMax q(buffer_);                                 \
    std::size_t last_result_bytes = 0;                                         \
    for (auto _ : state) {                                                     \
      auto r = q.getMinValue();                                                \
      last_result_bytes =                                                      \
          r.size() * sizeof(typename decltype(r)::value_type);                 \
      benchmark::DoNotOptimize(r.data());                                      \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::MinMax_getMinValue);                          \
    applyResultSize(state, last_result_bytes);                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, MinMax_getMinValue);                       \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, MinMax_getMaxValue)                          \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::minmax::MinMax q(buffer_);                                 \
    std::size_t last_result_bytes = 0;                                         \
    for (auto _ : state) {                                                     \
      auto r = q.getMaxValue();                                                \
      last_result_bytes =                                                      \
          r.size() * sizeof(typename decltype(r)::value_type);                 \
      benchmark::DoNotOptimize(r.data());                                      \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::MinMax_getMaxValue);                          \
    applyResultSize(state, last_result_bytes);                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, MinMax_getMaxValue);

DEFINE_MINMAX_SUITE(AirTreeQuery1DxT_MinMax)
DEFINE_MINMAX_SUITE(AirTreeQuery1DxF_MinMax)
DEFINE_MINMAX_SUITE(AirTreeQuery1DxP_MinMax)

#undef DEFINE_MINMAX_SUITE

} // namespace
