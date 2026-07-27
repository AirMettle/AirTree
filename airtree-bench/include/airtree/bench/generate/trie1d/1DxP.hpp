// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP
#define AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace airtree::bench::generate::trie1D {

inline constexpr double megaByteConversion = 1'000'000.0;

inline uint64_t calculateTotalInputBytes(const FPHArray &fpharray) {
  return static_cast<uint64_t>(fpharray.length) *
         static_cast<uint64_t>(getFPHTypeSize(fpharray.type));
}

template <typename T>
uint64_t countDistinctValues(const std::vector<T> &data) {
  return static_cast<uint64_t>(
      std::unordered_set<T>(data.begin(), data.end()).size());
}

inline uint64_t countPreciseBins1DxP(const TrieNode_20 *root) {
  if (!root) {
    return 0;
  }

  uint64_t precise_bins = 0;
  for (unsigned int i = 0; i < BINS_256; ++i) {
    if (!root->populated.test(i) || !root->nodes[i]) {
      continue;
    }
    for (unsigned int j = 0; j < BINS_64; ++j) {
      if (!root->nodes[i]->populated.test(j) || !root->nodes[i]->nodes[j]) {
        continue;
      }
      for (unsigned int k = 0; k < BINS_64; ++k) {
        if (root->nodes[i]->nodes[j]->counts[k] > 0) {
          ++precise_bins;
        }
      }
    }
  }
  return precise_bins;
}

class AirTreeBench1DxP : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TrieNode_20> airTree1DxP_root;
  std::vector<char> serializedTrie;

  void SetUp([[maybe_unused]] const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray = BenchmarkData<T>::fpharrays[0];
    const uint64_t dataset_size_bytes = calculateTotalInputBytes(fpharray);
    const uint64_t dataset_value_count = static_cast<uint64_t>(fpharray.length);
    const uint64_t distinct_values =
        countDistinctValues(BenchmarkData<T>::raw_data[0]);

    for (auto _ : state) {
      state.PauseTiming();
      specialCounts = std::make_unique<SpecialCounts>();
      curr_trie_size = 0;
      state.ResumeTiming();

      airTree1DxP_root = execCreateAndInsert_TrieNode20(
          *specialCounts, curr_trie_size, fpharray, true);

      benchmark::DoNotOptimize(airTree1DxP_root);
    }

    const uint64_t total_input_bytes =
        static_cast<uint64_t>(state.iterations()) * dataset_size_bytes;
    const uint64_t precise_bins = countPreciseBins1DxP(airTree1DxP_root.get());
    const double avg_bytes_per_bin =
        precise_bins > 0
            ? static_cast<double>(curr_trie_size) /
                  static_cast<double>(precise_bins)
            : 0.0;
    state.SetBytesProcessed(total_input_bytes);
    state.counters["Insertion Speed (MB/s)"] = benchmark::Counter(
        static_cast<double>(total_input_bytes) / megaByteConversion,
        benchmark::Counter::kIsRate);
    state.counters["Points_Per_Second"] = benchmark::Counter(
        state.iterations() * fpharray.length, benchmark::Counter::kIsRate);
    state.counters["Dataset Size MB"] = dataset_size_bytes / megaByteConversion;
    state.counters["Dataset Value Count"] = dataset_value_count;
    state.counters["Distinct Values"] = distinct_values;
    state.counters["Precise Bins"] = precise_bins;
    state.counters["Avg Bytes/Bin"] = avg_bytes_per_bin;
    state.counters["Trie Size (Bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray = airtree::bench::BenchmarkData<T>::fpharrays[0];

    airTree1DxP_root = execCreateAndInsert_TrieNode20(
        *specialCounts, curr_trie_size, fpharray, true);

    for (auto _ : state) {
      auto serializedTrieLocal = execSerialization_TrieNode20(
          airTree1DxP_root, *specialCounts, curr_trie_size, true);

      benchmark::DoNotOptimize(serializedTrieLocal.data());
      benchmark::ClobberMemory();

      // Update counter inside loop so it registers properly
      state.counters["Size"] = serializedTrieLocal.size();
    }
  }

};

} // namespace airtree::bench::generate::trie1D

#endif // AIRTREE_BENCH_GENERATE_TRIE1D_1DXP_HPP