// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/percentile/Percentile.hpp>
#include <benchmark/benchmark.h>

namespace {

using airtree::bench::query::QueryFixtureBase;

#define DEFINE_PERCENTILE_SUITE(SchemaClass)                                   \
  class SchemaClass : public QueryFixtureBase {};                              \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_p50)                              \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::percentile::Percentile q(buffer_);                         \
    for (auto _ : state) {                                                     \
      double v = q.getPercentile(50.0);                                        \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_p50);                              \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_p"] = 50;                                            \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_p50);                           \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_p90)                              \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::percentile::Percentile q(buffer_);                         \
    for (auto _ : state) {                                                     \
      double v = q.getPercentile(90.0);                                        \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_p90);                              \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_p"] = 90;                                            \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_p90);

DEFINE_PERCENTILE_SUITE(AirTreeQuery1DxT_Percentile)
DEFINE_PERCENTILE_SUITE(AirTreeQuery1DxF_Percentile)
DEFINE_PERCENTILE_SUITE(AirTreeQuery1DxP_Percentile)

#undef DEFINE_PERCENTILE_SUITE

// Service-shaped variants: p99 (warm), construction only, and cold (construct + query per iteration).
#define DEFINE_PERCENTILE_SERVICE_SUITE(SchemaClass)                           \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_p99)                              \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    airtree::query::percentile::Percentile q(buffer_);                         \
    for (auto _ : state) {                                                     \
      double v = q.getPercentile(99.0);                                        \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_p99);                              \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_p"] = 99;                                            \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_p99);                           \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_ctor)                             \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      airtree::query::percentile::Percentile q(buffer_);                       \
      benchmark::DoNotOptimize(q);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_ctor);                             \
    applyResultSize(state, 0);                                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_ctor);                          \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_cold_p50)                         \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      airtree::query::percentile::Percentile q(buffer_);                       \
      double v = q.getPercentile(50.0);                                        \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_cold_p50);                         \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_p"] = 50;                                            \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_cold_p50);                      \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Percentile_cold_p99)                         \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      airtree::query::percentile::Percentile q(buffer_);                       \
      double v = q.getPercentile(99.0);                                        \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Percentile_cold_p99);                         \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_p"] = 99;                                            \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Percentile_cold_p99);

DEFINE_PERCENTILE_SERVICE_SUITE(AirTreeQuery1DxT_Percentile)
DEFINE_PERCENTILE_SERVICE_SUITE(AirTreeQuery1DxF_Percentile)
DEFINE_PERCENTILE_SERVICE_SUITE(AirTreeQuery1DxP_Percentile)

#undef DEFINE_PERCENTILE_SERVICE_SUITE

} // namespace
