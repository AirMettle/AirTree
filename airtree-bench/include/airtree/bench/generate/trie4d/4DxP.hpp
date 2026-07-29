// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_GENERATE_TRIE4D_4DXP_HPP
#define AIRTREE_BENCH_GENERATE_TRIE4D_4DXP_HPP

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/io/AirTreeWriter.hpp>
#include <airtree/bench/BenchPaths.hpp>
#include <airtree/bench/BenchmarkData.hpp>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace airtree::bench::generate::trie4D {

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

inline uint64_t countPreciseBins4DxP(const TLE_4D_4x10 *root) {
  if (!root) {
    return 0;
  }

  uint64_t precise_bins = 0;
  for (unsigned int i = 0; i < BINS_4096; ++i) {
    if (!root->populated.test(i) || !root->nodes[i]) {
      continue;
    }
    for (unsigned int j = 0; j < BINS_1024; ++j) {
      if (!root->nodes[i]->populated.test(j) || !root->nodes[i]->nodes[j]) {
        continue;
      }
      for (unsigned int k = 0; k < BINS_1024; ++k) {
        if (!root->nodes[i]->nodes[j]->populated.test(k) ||
            !root->nodes[i]->nodes[j]->nodes[k]) {
          continue;
        }
        for (unsigned int l = 0; l < BINS_1024; ++l) {
          if (!root->nodes[i]->nodes[j]->nodes[k]->populated.test(l) ||
              !root->nodes[i]->nodes[j]->nodes[k]->nodes[l]) {
            continue;
          }
          for (unsigned int m = 0; m < BINS_1024; ++m) {
            if (root->nodes[i]->nodes[j]->nodes[k]->nodes[l]->counts[m] > 0) {
              ++precise_bins;
            }
          }
        }
      }
    }
  }
  return precise_bins;
}

class AirTreeBench4DxP : public benchmark::Fixture {
protected:
  std::unique_ptr<SpecialCounts> specialCounts;
  uint64_t curr_trie_size = 0;

  std::unique_ptr<TLE_4D_4x10> airTree4DxP_root;
  std::vector<char> serializedTrie;

  void SetUp([[maybe_unused]] const ::benchmark::State &state) override {
    specialCounts = std::make_unique<SpecialCounts>();
    curr_trie_size = 0;
  }

  template <typename T> void runCreateAndInsert(::benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 4) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];
    const auto &fpharray3 = BenchmarkData<T>::fpharrays[2];
    const auto &fpharray4 = BenchmarkData<T>::fpharrays[3];
    const uint64_t dataset_size_bytes =
        calculateTotalInputBytes(fpharray1) + calculateTotalInputBytes(fpharray2) +
        calculateTotalInputBytes(fpharray3) + calculateTotalInputBytes(fpharray4);
    const uint64_t dataset_value_count = static_cast<uint64_t>(fpharray1.length);
    const uint64_t distinct_values =
        countDistinctValues(BenchmarkData<T>::raw_data[0]) +
        countDistinctValues(BenchmarkData<T>::raw_data[1]) +
        countDistinctValues(BenchmarkData<T>::raw_data[2]) +
        countDistinctValues(BenchmarkData<T>::raw_data[3]);

    for (auto _ : state) {
      state.PauseTiming();
      specialCounts = std::make_unique<SpecialCounts>();
      curr_trie_size = 0;
      state.ResumeTiming();

      airTree4DxP_root = execCreateAndInsert_4D_4x10(
          fpharray1, fpharray2, fpharray3, fpharray4, curr_trie_size,
          specialCounts, true);

      benchmark::DoNotOptimize(airTree4DxP_root);
    }

    const uint64_t total_input_bytes =
        static_cast<uint64_t>(state.iterations()) * dataset_size_bytes;
    const uint64_t precise_bins = countPreciseBins4DxP(airTree4DxP_root.get());
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
        state.iterations() * fpharray1.length, benchmark::Counter::kIsRate);
    state.counters["Dataset Size MB"] = dataset_size_bytes / megaByteConversion;
    state.counters["Dataset Value Count"] = dataset_value_count;
    state.counters["Distinct Values"] = distinct_values;
    state.counters["Precise Bins"] = precise_bins;
    state.counters["Avg Bytes/Bin"] = avg_bytes_per_bin;
    state.counters["Trie Size (Bytes)"] = curr_trie_size;
  }

  template <typename T> void runSerialize(benchmark::State &state) {
    if (BenchmarkData<T>::fpharrays.empty()
        || BenchmarkData<T>::fpharrays.size() < 4) {
      state.SkipWithError("Data array is empty. Skipping benchmark.");
      return;
    }

    const auto &fpharray1 = BenchmarkData<T>::fpharrays[0];
    const auto &fpharray2 = BenchmarkData<T>::fpharrays[1];
    const auto &fpharray3 = BenchmarkData<T>::fpharrays[2];
    const auto &fpharray4 = BenchmarkData<T>::fpharrays[3];

    airTree4DxP_root =
        execCreateAndInsert_4D_4x10(fpharray1, fpharray2, fpharray3, fpharray4,
                                    curr_trie_size, specialCounts, true);


    for (auto _ : state) {
      auto serializedTrieLocal = execSerialize_4D_4x10(
          airTree4DxP_root.get(), curr_trie_size, specialCounts, true);

      benchmark::DoNotOptimize(serializedTrieLocal.data());
      benchmark::ClobberMemory();

      // Update counter inside loop so it registers properly
      state.counters["Size"] = serializedTrieLocal.size();
    }

    // Untimed materialize: write serialized histogram once if requested.
    if (!BenchPaths::write_airtree_path.empty()) {
      auto buffer = execSerialize_4D_4x10(
          airTree4DxP_root.get(), curr_trie_size, specialCounts, true);
      airtree::core::io::AirTreeWriter::Write(buffer,
                                              BenchPaths::write_airtree_path);
      BenchPaths::write_airtree_path.clear();
    }
  
  }

};

} // namespace airtree::bench::generate::trie4D

#endif // AIRTREE_BENCH_GENERATE_TRIE4D_4DXP_HPP