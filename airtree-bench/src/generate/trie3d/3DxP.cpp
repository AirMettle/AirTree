#include <airtree/bench/generate/trie3d/3DxP.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <cassert>

using namespace airtree::bench::generate::trie3D;
using namespace airtree::bench;

BENCHMARK_F(AirTreeBench3DxP, CreateAndInsert_double)(benchmark::State &state) {
  runCreateAndInsert<double>(state);
}

BENCHMARK_F(AirTreeBench3DxP, Serialize_double)(benchmark::State &state) {
  runSerialize<double>(state);
}

BENCHMARK_F(AirTreeBench3DxP, CreateAndInsert_float)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench3DxP, Serialize_float)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench3DxP, CreateAndInsert_int32)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench3DxP, Serialize_int32)(benchmark::State &state) {
  runSerialize<float>(state);
}

BENCHMARK_F(AirTreeBench3DxP, CreateAndInsert_int64)(benchmark::State &state) {
  runCreateAndInsert<float>(state);
}

BENCHMARK_F(AirTreeBench3DxP, Serialize_int64)(benchmark::State &state) {
  runSerialize<float>(state);
}
