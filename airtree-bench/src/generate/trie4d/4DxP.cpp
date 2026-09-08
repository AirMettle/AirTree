// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/generate/trie4d/4DxP.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <cassert>

using namespace airtree::bench::generate::trie4D;
using namespace airtree::bench;

BENCHMARK_F(AirTreeBench4DxP, CreateAndInsert_double)(benchmark::State &state) {
  runCreateAndInsert<double>(state);
}

BENCHMARK_F(AirTreeBench4DxP, Serialize_double)(benchmark::State &state) {
  runSerialize<double>(state);
}

BENCHMARK_F(AirTreeBench4DxP, CreateAndInsert_float)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench4DxP, Serialize_float)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench4DxP, CreateAndInsert_int32)(benchmark::State &state) {
  runCreateAndInsert<int32_t>(state);
}

BENCHMARK_F(AirTreeBench4DxP, Serialize_int32)(benchmark::State &state) {
  runSerialize<int32_t>(state);
}

BENCHMARK_F(AirTreeBench4DxP, CreateAndInsert_int64)(benchmark::State &state) {
  runCreateAndInsert<int64_t>(state);
}

BENCHMARK_F(AirTreeBench4DxP, Serialize_int64)(benchmark::State &state) {
  runSerialize<int64_t>(state);
}
