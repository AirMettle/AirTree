#ifndef AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP
#define AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace airtree::bench::generate::trie1D {

class AirTreeBench1DxP : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TrieNode_20> airTree1DxP_root;
  std::vector<char> serializedTrie;

  void SetUp(const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    assert(BenchmarkData<double>::fpharrays.size() == 1);

    const auto &fpharray = BenchmarkData<double>::fpharrays[0];

    for (auto _ : state) {
      state.PauseTiming();
      specialCounts = std::make_unique<SpecialCounts>();
      curr_trie_size = 0;
      state.ResumeTiming();

      airTree1DxP_root = execCreateAndInsert_TrieNode20(
          *specialCounts, curr_trie_size, fpharray, true);

      benchmark::DoNotOptimize(airTree1DxP_root);
    }

    state.counters["Speed"] = benchmark::Counter(
        state.iterations() * fpharray.length * getFPHTypeSize(fpharray.type),
        benchmark::Counter::kIsRate);
    state.counters["Size of trie in-mem (bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(::benchmark::State &state) {
    assert(BenchmarkData<double>::fpharrays.size() == 1);

    const auto &fpharray = BenchmarkData<double>::fpharrays[0];

    airTree1DxP_root = execCreateAndInsert_TrieNode20(
        *specialCounts, curr_trie_size, fpharray, true);

    for (auto _ : state) {
      serializedTrie = execSerialization_TrieNode20(
          airTree1DxP_root, *specialCounts, curr_trie_size, true);

      benchmark::DoNotOptimize(serializedTrie.data());
      benchmark::ClobberMemory();
    }

    state.counters["Size"] = serializedTrie.size();
  }

  void TearDown(const ::benchmark::State &state) override {
    serializedTrie.clear();
  }
};

} // namespace airtree::bench::generate::trie1D

#endif // AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP
