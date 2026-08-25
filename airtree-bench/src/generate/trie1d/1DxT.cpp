// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/generate/trie1d/1DxT.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <cassert>

using namespace airtree::bench::generate::trie1D;
using namespace airtree::bench;

BENCHMARK_F(AirTreeBench1DxT, CreateAndInsert_double)(benchmark::State &state) {
  runCreateAndInsert<double>(state);
}

BENCHMARK_F(AirTreeBench1DxT, Serialize_double)(benchmark::State &state) {
  runSerialize<double>(state);
}

BENCHMARK_F(AirTreeBench1DxT, CreateAndInsert_float)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench1DxT, Serialize_float)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench1DxT, CreateAndInsert_int32)(benchmark::State &state) {
  runCreateAndInsert<int32_t>(state);
}

BENCHMARK_F(AirTreeBench1DxT, Serialize_int32)(benchmark::State &state) {
  runSerialize<int32_t>(state);
}

BENCHMARK_F(AirTreeBench1DxT, CreateAndInsert_int64)(benchmark::State &state) {
  runCreateAndInsert<int64_t>(state);
}

BENCHMARK_F(AirTreeBench1DxT, Serialize_int64)(benchmark::State &state) {
  runSerialize<int64_t>(state);
}
