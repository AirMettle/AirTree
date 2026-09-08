// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/core/io/AirTreeReader.hpp>
#include <benchmark/benchmark.h>

namespace {

using airtree::bench::query::QueryFixtureBase;

// Deserialization alone: buffer -> in-memory trie (and its teardown), no bin table, no query.
#define DEFINE_READER_SUITE(SchemaClass)                                       \
  class SchemaClass : public QueryFixtureBase {};                              \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Reader_read)                                 \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    for (auto _ : state) {                                                     \
      airtree::core::io::AirTreeReader reader;                                 \
      reader.read(buffer_);                                                    \
      benchmark::DoNotOptimize(reader);                                        \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Reader_read);                                 \
    applyResultSize(state, 0);                                                 \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Reader_read);

DEFINE_READER_SUITE(AirTreeQuery1DxT_Reader)
DEFINE_READER_SUITE(AirTreeQuery1DxF_Reader)
DEFINE_READER_SUITE(AirTreeQuery1DxP_Reader)
DEFINE_READER_SUITE(AirTreeQuery2DxP_Reader)
DEFINE_READER_SUITE(AirTreeQuery3DxP_Reader)
DEFINE_READER_SUITE(AirTreeQuery4DxP_Reader)

#undef DEFINE_READER_SUITE

} // namespace
