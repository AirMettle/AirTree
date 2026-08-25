// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <type_traits>
#include <variant>

namespace {

using airtree::bench::query::QueryFixtureBase;
using airtree::query::bin_boundary::BinBoundary;

// BinBoundary is single-use (the object owns the buffer and walks it once), so
// construction + generateBinBoundaries() are timed together. This is the path
// that produces native bin bounds for export and for bound-carrying answers.
#define DEFINE_BINBOUNDARY_SUITE(SchemaClass)                                  \
  class SchemaClass : public QueryFixtureBase {};                              \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, BinBoundary_generate)                        \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    std::size_t last_result_bytes = 0;                                         \
    std::size_t last_bins = 0;                                                 \
    for (auto _ : state) {                                                     \
      BinBoundary bb(buffer_);                                                 \
      auto result = bb.generateBinBoundaries();                                \
      auto boundaries = result.getBoundaries();                                \
      last_bins = std::visit(                                                  \
          [](const auto &v) { return v.size(); }, *boundaries);                \
      last_result_bytes = std::visit(                                          \
          [](const auto &v) {                                                  \
            using V = std::decay_t<decltype(v)>;                               \
            return v.size() * sizeof(typename V::value_type);                  \
          },                                                                   \
          *boundaries);                                                        \
      benchmark::DoNotOptimize(last_bins);                                     \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::BinBoundary_generate);                        \
    applyResultSize(state, last_result_bytes);                                 \
    state.counters["Bins"] = static_cast<double>(last_bins);                   \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, BinBoundary_generate);

DEFINE_BINBOUNDARY_SUITE(AirTreeQuery1DxT_BinBoundary)
DEFINE_BINBOUNDARY_SUITE(AirTreeQuery1DxF_BinBoundary)
DEFINE_BINBOUNDARY_SUITE(AirTreeQuery1DxP_BinBoundary)
DEFINE_BINBOUNDARY_SUITE(AirTreeQuery2DxP_BinBoundary)
DEFINE_BINBOUNDARY_SUITE(AirTreeQuery3DxP_BinBoundary)
DEFINE_BINBOUNDARY_SUITE(AirTreeQuery4DxP_BinBoundary)

#undef DEFINE_BINBOUNDARY_SUITE

} // namespace
