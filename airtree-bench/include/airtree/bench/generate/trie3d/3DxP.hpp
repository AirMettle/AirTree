#ifndef AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP
#define AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace airtree::bench::generate::trie3D {

class AirTreeBench3DxP : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TLE_3D_3x10> airTree3DxP_root;
  std::vector<char> serializedTrie;

  void SetUp(const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        && BenchmarkData<T>::fpharrays.size() != 3) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];
    const auto &fpharray3 = BenchmarkData<T>::fpharrays[2];

    for (auto _ : state) {
      state.PauseTiming();
      specialCounts = std::make_unique<SpecialCounts>();
      curr_trie_size = 0;
      state.ResumeTiming();

      airTree3DxP_root = execCreateAndInsert_3D_3x10(
          fpharray1, fpharray2, fpharray3, curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(airTree3DxP_root);
    }

    state.counters["Speed"] = benchmark::Counter(
        state.iterations() * fpharray1.length * getFPHTypeSize(fpharray1.type),
        benchmark::Counter::kIsRate);
    state.counters["Size of trie in-mem (bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        && BenchmarkData<T>::fpharrays.size() != 3) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];
    const auto &fpharray3 = BenchmarkData<T>::fpharrays[2];

    auto root = execCreateAndInsert_3D_3x10(
        fpharray1, fpharray2, fpharray3, curr_trie_size, specialCounts, true);


    for (auto _ : state) {
      auto serializedTrie = execSerialize_3D_3x10(
          root.get(), curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(serializedTrie.data());
      benchmark::ClobberMemory();

      // Update counter inside loop so it registers properly
      state.counters["Size"] = serializedTrie.size();
    }
  }

  void TearDown(const ::benchmark::State &state) override {
    serializedTrie.clear();
  }
};

} // namespace airtree::bench::generate::trie3D

#endif // AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP
