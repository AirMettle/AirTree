#include <airtree/bench/generate/trie2d/2DxP.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <cassert>

using namespace airtree::bench::generate::trie2D;
using namespace airtree::bench;

BENCHMARK_F(AirTreeBench2DxP, CreateAndInsert_double)(benchmark::State &state) {
  runCreateAndInsert<double>(state);
}

BENCHMARK_F(AirTreeBench2DxP, Serialize_double)(benchmark::State &state) {
  runSerialize<double>(state);
}

BENCHMARK_F(AirTreeBench2DxP, CreateAndInsert_float)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench2DxP, Serialize_float)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench2DxP, CreateAndInsert_int32)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench2DxP, Serialize_int32)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench2DxP, CreateAndInsert_int64)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench2DxP, Serialize_int64)(benchmark::State &state) {
  runSerialize<float>(state);
}
