// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/cdf/CDF.hpp>
#include <airtree/query/percentile/Percentile.hpp>
#include <benchmark/benchmark.h>
#include <vector>

namespace {

using airtree::bench::query::QueryFixtureBase;

// Probes the CDF at the buffer's own median (computed untimed in SetUp).
class CDFFixtureBase : public QueryFixtureBase {
protected:
  double p50_ = 0.0;

  void SetUp(const ::benchmark::State &state) override {
    QueryFixtureBase::SetUp(state);
    if (hasBuffer()) {
      airtree::query::percentile::Percentile p(buffer_);
      p50_ = p.getPercentile(50.0);
    }
  }
};

#define DEFINE_CDF_SUITE(SchemaClass)                                          \
  class SchemaClass : public CDFFixtureBase {};                                \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, CDF_ctor)(benchmark::State & state) {        \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      std::vector<char> buf = buffer_;                                         \
      airtree::query::cdf::CDF q(buf);                                                        \
      benchmark::DoNotOptimize(q);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::CDF_ctor);                                    \
    applyResultSize(state, 0);                                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, CDF_ctor);                                 \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, CDF_at_p50)(benchmark::State & state) {      \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    std::vector<char> buf = buffer_;                                           \
    airtree::query::cdf::CDF q(buf);                                                          \
    for (auto _ : state) {                                                     \
      double v = q.getCDF(p50_, true);                                         \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::CDF_at_p50);                                  \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_value"] = p50_;                                      \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, CDF_at_p50);                               \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, CDF_cold_at_p50)(benchmark::State & state) { \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      std::vector<char> buf = buffer_;                                         \
      airtree::query::cdf::CDF q(buf);                                                        \
      double v = q.getCDF(p50_, true);                                         \
      benchmark::DoNotOptimize(v);                                             \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::CDF_cold_at_p50);                             \
    applyResultSize(state, sizeof(double));                                    \
    state.counters["Param_value"] = p50_;                                      \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, CDF_cold_at_p50);

DEFINE_CDF_SUITE(AirTreeQuery1DxT_CDF)
DEFINE_CDF_SUITE(AirTreeQuery1DxF_CDF)
DEFINE_CDF_SUITE(AirTreeQuery1DxP_CDF)

#undef DEFINE_CDF_SUITE

} // namespace
