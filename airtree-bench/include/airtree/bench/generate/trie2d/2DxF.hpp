#ifndef AIRTREE_BENCH_GENERATE_TRIE2D_2DXF_HPP
#define AIRTREE_BENCH_GENERATE_TRIE2D_2DXF_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace airtree::bench::generate::trie2D {

class AirTreeBench2DxF : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TLETrieNode_2D> airTree2DxF_root;
  std::vector<char> serializedTrie;

  void SetUp(const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 2) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];

    for (auto _ : state) {
      state.PauseTiming();
      specialCounts = std::make_unique<SpecialCounts>();
      curr_trie_size = 0;
      state.ResumeTiming();

      airTree2DxF_root = execCreateAndInsert_2D(
          fpharray1, fpharray2, curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(airTree2DxF_root);
    }

    state.counters["Speed"] =
        benchmark::Counter(state.iterations() * fpharray1.length
                               * getFPHTypeSize(fpharray1.type) * 2,
                           benchmark::Counter::kIsRate);
    state.counters["Points_Per_Second"] = benchmark::Counter(
        state.iterations() * fpharray1.length, benchmark::Counter::kIsRate);
    state.counters["Size of trie in-mem (bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 2) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = airtree::bench::BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = airtree::bench::BenchmarkData<T>::fpharrays[1];

    airTree2DxF_root = execCreateAndInsert_2D(
        fpharray1, fpharray2, curr_trie_size, specialCounts, true);

    for (auto _ : state) {
      auto serializedTrie = execSerialize_2D(
          airTree2DxF_root.get(), curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(serializedTrie.data());
      benchmark::ClobberMemory();

      // Update counter inside loop so it registers properly
      state.counters["Size"] = serializedTrie.size();
    }
  }

  void TearDown(const ::benchmark::State &state) override {}
};

} // namespace airtree::bench::generate::trie2D

#endif // AIRTREE_BENCH_GENERATE_TRIE2D_2DXF_HPP
