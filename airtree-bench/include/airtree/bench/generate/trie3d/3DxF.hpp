// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_GENERATE_TRIE3D_3DXF_HPP
#define AIRTREE_BENCH_GENERATE_TRIE3D_3DXF_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace airtree::bench::generate::trie3D {

class AirTreeBench3DxF : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TLE_3D_888> airTree3DxF_root;
  std::vector<char> serializedTrie;

  void SetUp([[maybe_unused]] const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 3) {
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

      airTree3DxF_root = execCreateAndInsert_3D_888(
          fpharray1, fpharray2, fpharray3, curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(airTree3DxF_root);
    }

    state.counters["Speed"] =
        benchmark::Counter(state.iterations() * fpharray1.length
                               * getFPHTypeSize(fpharray1.type) * 3,
                           benchmark::Counter::kIsRate);
    state.counters["Points_Per_Second"] = benchmark::Counter(
        state.iterations() * fpharray1.length, benchmark::Counter::kIsRate);
    state.counters["Size of trie in-mem (bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 3) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];
    const auto &fpharray3 = BenchmarkData<T>::fpharrays[2];

    airTree3DxF_root = execCreateAndInsert_3D_888(
        fpharray1, fpharray2, fpharray3, curr_trie_size, specialCounts, true);


    for (auto _ : state) {
      auto serialized = execSerialize_3D_888(
          airTree3DxF_root.get(), curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(serialized.data());
      benchmark::ClobberMemory();

      // Update counter inside loop so it registers properly
      state.counters["Size"] = serialized.size();
    }
  }

  void TearDown([[maybe_unused]] const ::benchmark::State &state) override {
    // serializedTrie.clear();
  }
};

} // namespace airtree::bench::generate::trie3D

#endif // AIRTREE_BENCH_GENERATE_TRIE3D_3DXF_HPP
