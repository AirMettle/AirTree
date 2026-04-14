#include <airtree/core/common/SpecialCounts.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <limits>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <vector>

using namespace airtree::query::bin_boundary;

template <size_t N>
[[nodiscard]] std::bitset<N> deserializeBitset(const std::vector<char> buffer,
                                               size_t &offset, int len) {
  std::vector<uint64_t> compact_array_values =
      deserializeCompactBooleanArray(buffer, offset, len);
  BooleanArray compact_array = BooleanArray(compact_array_values);
  std::bitset<N> populated;
  for (size_t i = 0; i < N; ++i) {
    populated[i] = compact_array.get(i);
  }
  return populated;
}

template <size_t N>
[[nodiscard]] std::vector<uint32_t>
deserializeCountsToOriginalLen(const std::vector<char> &buffer, size_t &offset,
                               std::bitset<N> &populated) {
  std::vector<uint32_t> counts =
      deserializeCounts(buffer, offset, populated.count());
  std::vector<uint32_t> expandedCounts(N, 0);
  size_t countIdx = 0;
  for (size_t i = 0; i < N; ++i) {
    if (populated.test(i)) {
      expandedCounts[i] = counts[countIdx];
      countIdx++;
    }
  }

  return expandedCounts;
}

// Lookup bin boundaries from the sorted reference table.
//
// Positive: [recon, next)  — includes recon, excludes next
// Negative: (prev, recon]  — excludes prev, includes recon
//
// Both are stored as (lower, upper) pairs. The caller determines the
// convention from the sign of the values.
static std::pair<double, double>
lookupBinBounds(double reconstructed,
                const std::vector<double> &refBoundaries) {
  auto idx = std::lower_bound(
      refBoundaries.begin(), refBoundaries.end(), reconstructed);
  if (idx == refBoundaries.end()) {
    return {*(idx - 1), std::numeric_limits<double>::infinity()};
  }
  if (reconstructed < 0) {
    // Negative: (prev, recon]
    double prev = (idx != refBoundaries.begin())
                      ? *(idx - 1)
                      : -std::numeric_limits<double>::infinity();
    return {prev, *idx};
  }
  if ((idx + 1) == refBoundaries.end()) {
    return {*idx, std::numeric_limits<double>::infinity()};
  }
  // Positive: [recon, next)
  return {*idx, *(idx + 1)};
}

// Returns a sorted vector 64-bit IEEE 754 floating point numbers
// that each represent a bin for N-bit internal representation
// Defaults to 12 bits if no argument is provided
std::vector<double> generateNBitBinBoundaries(uint32_t bitLength = 12) {
  std::vector<double> binBoundaries;
  auto numBins = (1u << bitLength);
  for (uint32_t i = 0; i < numBins; ++i) {
    double boundary = reConstruct<double>(i, bitLength);
    binBoundaries.push_back(boundary);
  }
  std::sort(binBoundaries.begin(), binBoundaries.end());
  return binBoundaries;
}

BinBoundary1DList BinBoundary::buildBinBoundaries1DxT() {
  BinBoundary1DList binBoundaries;

  auto ref13BitBinBoundaries = generateNBitBinBoundaries(13);

  // Deserialize l0 populated bitset
  auto l0_populated =
      deserializeBitset<BINS_256>(buffer_, offset_, BINS_256 / 64);
  // Deserialize l0 counts
  auto l0_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);

  for (size_t l0_idx = 0; l0_idx < BINS_256; ++l0_idx) {
    if (!l0_populated[l0_idx]) {
      continue; // Skip if this l0 node is not populated
    }
    // Deserialize l1 populated bitset
    auto l1_populated = deserializeBitset<BINS_32>(buffer_, offset_, 1);
    // Deserialize l1 counts
    auto l1_counts =
        deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
    for (size_t l1_idx = 0; l1_idx < BINS_32; ++l1_idx) {
      if (!l1_populated[l1_idx]) {
        continue; // Skip if this l1 node is not populated
      }

      // Start by saving the count
      auto count = l1_counts[l1_idx];
      // generate bin boundaries
      uint32_t internalRep = (l0_idx << 5) | l1_idx;
      auto reconstructed = reConstruct<double>(internalRep, 13);
      auto [binLeft, binRight] =
          lookupBinBounds(reconstructed, ref13BitBinBoundaries);
      binBoundaries.emplace_back(binLeft, binRight, count);
    }
  }
  return binBoundaries;
}

BinBoundary1DList BinBoundary::buildBinBoundaries1DxF() {
  BinBoundary1DList binBoundaries;

  auto ref16BitBinBoundaries = generateNBitBinBoundaries(16);

  // Deserialize l0 populated bitset
  auto l0_populated =
      deserializeBitset<BINS_256>(buffer_, offset_, BINS_256 / 64);
  // Deserialize l0 counts
  auto l0_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);

  for (size_t l0_idx = 0; l0_idx < BINS_256; ++l0_idx) {
    if (!l0_populated[l0_idx]) {
      continue; // Skip if this l0 node is not populated
    }
    // Deserialize l1 populated bitset
    auto l1_populated =
        deserializeBitset<BINS_256>(buffer_, offset_, BINS_256 / 64);
    // Deserialize l1 counts
    auto l1_counts =
        deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
    for (size_t l1_idx = 0; l1_idx < BINS_256; ++l1_idx) {
      if (!l1_populated[l1_idx]) {
        continue; // Skip if this l1 node is not populated
      }

      // Start by saving the count
      auto count = l1_counts[l1_idx];
      // generate bin boundaries
      uint32_t internalRep = (l0_idx << 8) | l1_idx;
      auto reconstructed = reConstruct<double>(internalRep, 16);
      auto [binLeft, binRight] =
          lookupBinBounds(reconstructed, ref16BitBinBoundaries);
      binBoundaries.emplace_back(binLeft, binRight, count);
    }
  }

  return binBoundaries;
}

BinBoundary1DList BinBoundary::buildBinBoundaries1DxP() {
  BinBoundary1DList binBoundaries;

  auto ref20BitBinBoundaries = generateNBitBinBoundaries(20);

  // Deserialize l0 populated bitset
  auto l0_populated =
      deserializeBitset<BINS_256>(buffer_, offset_, BINS_256 / 64);
  // Deserialize l0 counts
  auto l0_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);

  for (size_t l0_idx = 0; l0_idx < BINS_256; ++l0_idx) {
    if (!l0_populated[l0_idx]) {
      continue; // Skip if this l0 node is not populated
    }
    // Deserialize l1 populated bitset
    auto l1_populated =
        deserializeBitset<BINS_64>(buffer_, offset_, BINS_64 / 64);
    // Deserialize l1 counts
    auto l1_counts =
        deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
    for (size_t l1_idx = 0; l1_idx < BINS_64; ++l1_idx) {
      if (!l1_populated[l1_idx]) {
        continue; // Skip if this l1 node is not populated
      }

      // Deserialize l2 populated bitset
      auto l2_populated =
          deserializeBitset<BINS_64>(buffer_, offset_, BINS_64 / 64);
      // Deserialize l2 counts
      auto l2_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l2_populated);
      for (size_t l2_idx = 0; l2_idx < BINS_64; ++l2_idx) {
        if (!l2_populated[l2_idx]) {
          continue; // Skip if this l2 node is not populated
        }

        // Start by saving the count
        auto count = l2_counts[l2_idx];
        // generate bin boundaries
        uint32_t internalRep = (l0_idx << 12) | (l1_idx << 6) | l2_idx;
        auto reconstructed = reConstruct<double>(internalRep, 20);
        auto [binLeft, binRight] =
            lookupBinBounds(reconstructed, ref20BitBinBoundaries);
        binBoundaries.emplace_back(binLeft, binRight, count);
      }
    }
  }
  return binBoundaries;
}

BinBoundary2DList BinBoundary::buildBinBoundaries2DxP() {
  // This holds a vector of (xmin, xmax, ymin, ymax, count)
  BinBoundary2DList binBoundaries;

  auto ref12BitBinBoundaries = generateNBitBinBoundaries();

  // Deserialize TLE bitset
  auto tle_populated =
      deserializeBitset<BINS_64>(buffer_, offset_, BINS_64 / 64);
  // Deserialize TLE counts   forward
  auto tle_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, tle_populated);

  for (size_t tle_idx = 0; tle_idx < BINS_64; ++tle_idx) {
    if (!tle_populated[tle_idx]) {
      continue; // Skip if this TLE is not populated
    }

    auto [dimensionInfoVec, specialCount] =
        deconstructTLE(tle_idx, 2); // Deconstruct TLE for 2D

    // All dimensions are special
    // Build bin boundaries with special values and TLE count
    if (specialCount == 2) {
      std::vector<std::pair<double, double>> special_bins;
      for (auto &dim_info : dimensionInfoVec) {
        auto special_dims = getSpecialValue(dim_info.valueType);
        special_bins.emplace_back(special_dims.first, special_dims.second);
      }
      binBoundaries.emplace_back(special_bins[0].first, special_bins[0].second,
                                 special_bins[1].first, special_bins[1].second,
                                 tle_counts[tle_idx]);
      continue;
    } // end of 2 special dimensions handling


    // 1 dimension is special
    // We have 10 bits of internal representation
    if (specialCount == 1) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Start by saving the count
        auto count = l0_counts[l0_idx];
        // generate bin boundaries
        uint32_t internalRep = l0_idx;
        std::vector<double> bins;
        for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size(); ++dim_idx) {
          auto curr_dim_info = dimensionInfoVec[dim_idx];
          if (curr_dim_info.isSpecial) {
            auto special_dims = getSpecialValue(curr_dim_info.valueType);
            bins.emplace_back(special_dims.first);
            bins.emplace_back(special_dims.second);
            continue; // Skip to next dimension
          }
          uint32_t bits12 = (curr_dim_info.prefix << 10) | internalRep;
          double binBoundary = reConstruct<double>(bits12, 12);
          auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
          bins.emplace_back(bL);
          bins.emplace_back(bR);
        }
        if (bins.size() == 4) {
          // Ensure we have 4 bins for 2D
          // bins order: [x-min, x-max, y-min, y-max]
          // Constructor expects: (min_x, min_y, max_x, max_y, count)
          binBoundaries.emplace_back(bins[0], bins[2], bins[1], bins[3], count);
        } else {
          SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
        }
      }
      continue;
    } // end of 1 special dimension handling

    // No special dimensions
    // We have 20 bits of internal representation
    if (specialCount == 0) {
      // deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // Deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }

          // Start by saving the count
          auto count = l1_counts[l1_idx];
          // generate bin boundaries
          uint32_t internalRep = (l0_idx << 10) | l1_idx;
          auto chunkPair = reverse_combine_chunks_10b(internalRep);
          std::vector<uint32_t> dims = {chunkPair.x, chunkPair.y};
          std::vector<double> bins;
          for (size_t dim_idx = 0; dim_idx < dims.size(); ++dim_idx) {
            auto curr_dim = dims[dim_idx];
            auto curr_dim_info = dimensionInfoVec[dim_idx];

            uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
            double binBoundary = reConstruct<double>(bits12, 12);
            auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
            bins.emplace_back(bL);
            bins.emplace_back(bR);
          }
          if (bins.size() == 4) {
            // Ensure we have 4 bins for 2D
            // bins order: [x-min, x-max, y-min, y-max]
            // Constructor expects: (min_x, min_y, max_x, max_y, count)
            binBoundaries.emplace_back(
                bins[0], bins[2], bins[1], bins[3], count);
          } else {
            SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
          }
        }
      }
      continue;
    } // end of no special dimensions handling
  } // end of TLE iteration
  return binBoundaries;
}

BinBoundary3DList BinBoundary::buildBinBoundaries3DxP() {
  // This holds a vector of (xmin, xmax, ymin, ymax, zmin, zmax, count)
  BinBoundary3DList binBoundaries;

  auto ref12BitBinBoundaries = generateNBitBinBoundaries();

  // Deserialize TLE bitset
  auto tle_populated =
      deserializeBitset<BINS_512>(buffer_, offset_, BINS_512 / 64);
  // Deserialize TLE counts
  auto tle_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, tle_populated);

  // iterate through TLEs
  for (size_t tle_idx = 0; tle_idx < BINS_512; ++tle_idx) {
    if (!tle_populated[tle_idx]) {
      continue; // Skip if this TLE is not populated
    }

    // Deconstruct TLE for 3D
    auto [dimensionInfoVec, specialCount] = deconstructTLE(tle_idx, 3);

    // All dimensions are special
    // Build bin boundaries with special values and TLE count
    if (specialCount == 3) {
      std::vector<std::pair<double, double>> special_bins;
      for (auto &dim_info : dimensionInfoVec) {
        auto special_dims = getSpecialValue(dim_info.valueType);
        special_bins.emplace_back(special_dims.first, special_dims.second);
      }
      binBoundaries.emplace_back(special_bins[0].first, special_bins[0].second,
                                 special_bins[1].first, special_bins[1].second,
                                 special_bins[2].first, special_bins[2].second,
                                 tle_counts[tle_idx]);
      continue;
    } // end of 3 special dimensions handling

    // 2 dimensions are special
    // We have 10 bits of internal representation
    if (specialCount == 2) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; l0_idx++) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Start by saving the count
        auto count = l0_counts[l0_idx];
        // generate bin boundaries
        uint32_t internalRep = l0_idx;
        std::vector<double> bins;
        for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size(); dim_idx++) {
          auto curr_dim_info = dimensionInfoVec[dim_idx];
          if (curr_dim_info.isSpecial) {
            auto special_dims = getSpecialValue(curr_dim_info.valueType);
            bins.emplace_back(special_dims.first);
            bins.emplace_back(special_dims.second);
            continue; // Skip to next dimension
          }

          uint32_t bits12 = (curr_dim_info.prefix << 10) | internalRep;
          double binBoundary = reConstruct<double>(bits12, 12);
          auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
          bins.emplace_back(bL);
          bins.emplace_back(bR);
        }
        if (bins.size() == 6) {
          // Ensure we have 6 bins for 3D
          // bins order: [x-min, x-max, y-min, y-max, z-min, z-max]
          // Constructor expects: (min_x, min_y, min_z, max_x, max_y, max_z,
          // count)
          binBoundaries.emplace_back(
              bins[0], bins[2], bins[4], bins[1], bins[3], bins[5], count);
        } else {
          SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
        }
      }
      continue;
    } // end of 2 special dimensions handling

    // 1 dimension is special
    // We have 20 bits of internal representation
    if (specialCount == 1) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // Deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }

          // Start by saving the count
          auto count = l1_counts[l1_idx];
          // generate bin boundaries
          uint32_t internalRep = (l0_idx << 10) | l1_idx;
          auto chunkPair = reverse_combine_chunks_10b(internalRep);
          std::queue<uint32_t> dims;
          dims.push(chunkPair.x);
          dims.push(chunkPair.y);
          std::vector<double> bins;
          for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size();
               ++dim_idx) {
            auto curr_dim_info = dimensionInfoVec[dim_idx];
            if (curr_dim_info.isSpecial) {
              auto special_dims = getSpecialValue(curr_dim_info.valueType);
              bins.emplace_back(special_dims.first);
              bins.emplace_back(special_dims.second);
              continue; // Skip to next dimension
            }
            auto curr_dim = dims.front();
            dims.pop();
            uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
            double binBoundary = reConstruct<double>(bits12, 12);
            auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
            bins.emplace_back(bL);
            bins.emplace_back(bR);
          }
          if (bins.size() == 6) {
            // Ensure we have 6 bins for 3D
            // bins order: [x-min, x-max, y-min, y-max, z-min, z-max]
            // Constructor expects: (min_x, min_y, min_z, max_x, max_y, max_z,
            // count)
            binBoundaries.emplace_back(
                bins[0], bins[2], bins[4], bins[1], bins[3], bins[5], count);
          } else {
            SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
          }
        }
      }
      continue;
    }

    // No special dimensions
    // We have 30 bits of internal representation
    if (specialCount == 0) {
      // deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // Deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }
          // Deserialize l2 populated bitset
          auto l2_populated =
              deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
          // Deserialize l2 counts
          auto l2_counts =
              deserializeCountsToOriginalLen(buffer_, offset_, l2_populated);
          for (size_t l2_idx = 0; l2_idx < BINS_1024; ++l2_idx) {
            if (!l2_populated[l2_idx]) {
              continue; // Skip if this l2 node is not populated
            }
            // Start by saving the count
            auto count = l2_counts[l2_idx];
            // generate bin boundaries
            uint32_t internalRep = (l0_idx << 20) | (l1_idx << 10) | l2_idx;
            auto chunkTriple = reverse_combine_chunks_10b_3(internalRep);
            std::queue<uint32_t> dims;
            dims.push(chunkTriple.x);
            dims.push(chunkTriple.y);
            dims.push(chunkTriple.z);
            std::vector<double> bins;
            for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size();
                 ++dim_idx) {
              auto curr_dim = dims.front();
              dims.pop();
              auto curr_dim_info = dimensionInfoVec[dim_idx];
              uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
              double binBoundary = reConstruct<double>(bits12, 12);
              auto [bL, bR] =
                  lookupBinBounds(binBoundary, ref12BitBinBoundaries);
              bins.emplace_back(bL);
              bins.emplace_back(bR);
            }
            if (bins.size() == 6) {
              // Ensure we have 6 bins for 3D
              // bins order: [x-min, x-max, y-min, y-max, z-min, z-max]
              // Constructor expects: (min_x, min_y, min_z, max_x, max_y, max_z,
              // count)
              binBoundaries.emplace_back(
                  bins[0], bins[2], bins[4], bins[1], bins[3], bins[5], count);
            } else {
              SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
            }
          }
        }
      }
      continue;
    } // end of no special dimensions handling
  } // end of TLE iteration

  return binBoundaries;
}

BinBoundary4DList BinBoundary::buildBinBoundaries4DxP() {
  // This holds a vector of (xmin, xmax, ymin, ymax, zmin, zmax, wmin, wmax,
  // count)
  BinBoundary4DList binBoundaries;

  auto ref12BitBinBoundaries = generateNBitBinBoundaries();
  // Deserialize TLE bitset
  auto tle_populated =
      deserializeBitset<BINS_4096>(buffer_, offset_, BINS_4096 / 64);
  // Deserialize TLE counts
  auto tle_counts =
      deserializeCountsToOriginalLen(buffer_, offset_, tle_populated);

  // iterate through TLEs
  for (size_t tle_idx = 0; tle_idx < BINS_4096; ++tle_idx) {
    if (!tle_populated[tle_idx]) {
      continue; // Skip if this TLE is not populated
    }
    // Deconstruct TLE for 4D
    auto [dimensionInfoVec, specialCount] = deconstructTLE(tle_idx, 4);

    // All dimensions are special
    // Build bin boundaries with special values and TLE count
    if (specialCount == 4) {
      std::vector<std::pair<double, double>> special_bins;
      for (auto &dim_info : dimensionInfoVec) {
        auto special_dims = getSpecialValue(dim_info.valueType);
        special_bins.emplace_back(special_dims.first, special_dims.second);
      }
      binBoundaries.emplace_back(
          special_bins[0].first, special_bins[0].second, special_bins[1].first,
          special_bins[1].second, special_bins[2].first, special_bins[2].second,
          special_bins[3].first, special_bins[3].second, tle_counts[tle_idx]);
      continue;
    } // end of 4 special dimensions handling

    // 3 of 4 dimensions are special
    // We have 10 bits of internal representation
    if (specialCount == 3) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Start by saving the count
        auto count = l0_counts[l0_idx];
        // generate bin boundaries
        uint32_t internalRep = l0_idx;
        std::vector<double> bins;
        for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size(); ++dim_idx) {
          auto curr_dim_info = dimensionInfoVec[dim_idx];
          if (curr_dim_info.isSpecial) {
            auto special_dims = getSpecialValue(curr_dim_info.valueType);
            bins.emplace_back(special_dims.first);
            bins.emplace_back(special_dims.second);
            continue; // Skip to next dimension
          }
          uint32_t bits12 = (curr_dim_info.prefix << 10) | internalRep;
          double binBoundary = reConstruct<double>(bits12, 12);
          auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
          bins.emplace_back(bL);
          bins.emplace_back(bR);
        }
        if (bins.size() == 8) {
          // Ensure we have 8 bins for 4D
          // bins order: [x-min, x-max, y-min, y-max, z-min, z-max, w-min,
          // w-max] Constructor expects: (min_x, min_y, min_z, min_w, max_x,
          // max_y, max_z, max_w, count)
          binBoundaries.emplace_back(bins[0], bins[2], bins[4], bins[6],
                                     bins[1], bins[3], bins[5], bins[7], count);
        } else {
          SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
        }
      }
      continue;
    } // end of 3 special dimensions handling

    // 2 dimensions are special
    // We have 20 bits of internal representation
    if (specialCount == 2) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // Deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // Deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }
          // Start by saving the count
          auto count = l1_counts[l1_idx];
          // generate bin boundaries
          uint32_t internalRep = (l0_idx << 10) | l1_idx;
          auto chunkPair = reverse_combine_chunks_10b(internalRep);
          std::queue<uint32_t> dims;
          dims.push(chunkPair.x);
          dims.push(chunkPair.y);
          std::vector<double> bins;
          for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size();
               ++dim_idx) {
            auto curr_dim_info = dimensionInfoVec[dim_idx];
            if (curr_dim_info.isSpecial) {
              auto special_dims = getSpecialValue(curr_dim_info.valueType);
              bins.emplace_back(special_dims.first);
              bins.emplace_back(special_dims.second);
              continue; // Skip to next dimension
            }
            auto curr_dim = dims.front();
            dims.pop();
            uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
            double binBoundary = reConstruct<double>(bits12, 12);
            auto [bL, bR] = lookupBinBounds(binBoundary, ref12BitBinBoundaries);
            bins.emplace_back(bL);
            bins.emplace_back(bR);
          }
          if (bins.size() == 8) {
            // Ensure we have 8 bins for 4D
            // bins order: [x-min, x-max, y-min, y-max, z-min, z-max, w-min,
            // w-max] Constructor expects: (min_x, min_y, min_z, min_w, max_x,
            // max_y, max_z, max_w, count)
            binBoundaries.emplace_back(bins[0], bins[2], bins[4], bins[6],
                                       bins[1], bins[3], bins[5], bins[7],
                                       count);
          } else {
            SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
          }
        }
      }
      continue;
    } // end of 2 special dimensions handling

    // 1 dimension is special
    // We have 30 bits of internal representation
    if (specialCount == 1) {
      // Deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // Deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }
          // deserialize l2 populated bitset
          auto l2_populated =
              deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
          // deserialize l2 counts
          auto l2_counts =
              deserializeCountsToOriginalLen(buffer_, offset_, l2_populated);
          for (size_t l2_idx = 0; l2_idx < BINS_1024; ++l2_idx) {
            if (!l2_populated[l2_idx]) {
              continue; // Skip if this l2 node is not populated
            }
            // Start by saving the count
            auto count = l2_counts[l2_idx];
            // generate bin boundaries
            uint32_t internalRep = (l0_idx << 20) | (l1_idx << 10) | l2_idx;
            auto chunkTriple = reverse_combine_chunks_10b_3(internalRep);
            std::queue<uint32_t> dims;
            dims.push(chunkTriple.x);
            dims.push(chunkTriple.y);
            dims.push(chunkTriple.z);
            std::vector<double> bins;
            for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size();
                 ++dim_idx) {
              auto curr_dim_info = dimensionInfoVec[dim_idx];
              if (curr_dim_info.isSpecial) {
                auto special_dims = getSpecialValue(curr_dim_info.valueType);
                bins.emplace_back(special_dims.first);
                bins.emplace_back(special_dims.second);
                continue; // Skip to next dimension
              }
              auto curr_dim = dims.front();
              dims.pop();
              uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
              double binBoundary = reConstruct<double>(bits12, 12);
              auto [bL, bR] =
                  lookupBinBounds(binBoundary, ref12BitBinBoundaries);
              bins.emplace_back(bL);
              bins.emplace_back(bR);
            }
            if (bins.size() == 8) {
              // Ensure we have 8 bins for 4D
              // bins order: [x-min, x-max, y-min, y-max, z-min, z-max, w-min,
              // w-max] Constructor expects: (min_x, min_y, min_z, min_w, max_x,
              // max_y, max_z, max_w, count)
              binBoundaries.emplace_back(bins[0], bins[2], bins[4], bins[6],
                                         bins[1], bins[3], bins[5], bins[7],
                                         count);
            } else {
              SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
            }
          }
        }
      }
      continue;
    } // end of 1 special dimension handling

    // No special dimensions
    // We have 40 bits of internal representation
    if (specialCount == 0) {
      // deserialize l0 populated bitset
      auto l0_populated =
          deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
      // deserialize l0 counts
      auto l0_counts =
          deserializeCountsToOriginalLen(buffer_, offset_, l0_populated);
      for (size_t l0_idx = 0; l0_idx < BINS_1024; ++l0_idx) {
        if (!l0_populated[l0_idx]) {
          continue; // Skip if this l0 node is not populated
        }
        // deserialize l1 populated bitset
        auto l1_populated =
            deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
        // deserialize l1 counts
        auto l1_counts =
            deserializeCountsToOriginalLen(buffer_, offset_, l1_populated);
        for (size_t l1_idx = 0; l1_idx < BINS_1024; ++l1_idx) {
          if (!l1_populated[l1_idx]) {
            continue; // Skip if this l1 node is not populated
          }
          // deserialize l2 populated bitset
          auto l2_populated =
              deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
          // deserialize l2 counts
          auto l2_counts =
              deserializeCountsToOriginalLen(buffer_, offset_, l2_populated);
          for (size_t l2_idx = 0; l2_idx < BINS_1024; ++l2_idx) {
            if (!l2_populated[l2_idx]) {
              continue; // Skip if this l2 node is not populated
            }
            // deserialize l3 populated bitset
            auto l3_populated =
                deserializeBitset<BINS_1024>(buffer_, offset_, BINS_1024 / 64);
            // deserialize l3 counts
            auto l3_counts =
                deserializeCountsToOriginalLen(buffer_, offset_, l3_populated);
            for (size_t l3_idx = 0; l3_idx < BINS_1024; ++l3_idx) {
              if (!l3_populated[l3_idx]) {
                continue; // Skip if this l3 node is not populated
              }
              // Start by saving the count
              auto count = l3_counts[l3_idx];
              // generate bin boundaries
              uint64_t internalRep = (static_cast<uint64_t>(l0_idx) << 30)
                                     | (static_cast<uint64_t>(l1_idx) << 20)
                                     | (l2_idx << 10) | l3_idx;
              auto chunkQuad = reverse_combine_chunks_10b_4(internalRep);
              std::queue<uint32_t> dims;
              dims.push(chunkQuad.x);
              dims.push(chunkQuad.y);
              dims.push(chunkQuad.z);
              dims.push(chunkQuad.w);
              std::vector<double> bins;
              for (size_t dim_idx = 0; dim_idx < dimensionInfoVec.size();
                   ++dim_idx) {
                auto curr_dim_info = dimensionInfoVec[dim_idx];
                uint32_t curr_dim = dims.front();
                dims.pop();
                uint32_t bits12 = (curr_dim_info.prefix << 10) | curr_dim;
                double binBoundary = reConstruct<double>(bits12, 12);
                auto [bL, bR] =
                    lookupBinBounds(binBoundary, ref12BitBinBoundaries);
                bins.emplace_back(bL);
                bins.emplace_back(bR);
              }
              if (bins.size() == 8) {
                // Ensure we have 8 bins for 4D
                // bins order: [x-min, x-max, y-min, y-max, z-min, z-max, w-min,
                // w-max] Constructor expects: (min_x, min_y, min_z, min_w,
                // max_x, max_y, max_z, max_w, count)
                binBoundaries.emplace_back(bins[0], bins[2], bins[4], bins[6],
                                           bins[1], bins[3], bins[5], bins[7],
                                           count);
              } else {
                SPDLOG_ERROR("Invalid number of bins: {}", bins.size());
              }
            }
          }
        }
      }
      continue;
    } // end of 0 special dimension handling

  } // TLE iteration loop
  return binBoundaries;
}

BinBoundary::BinBoundary(std::vector<char> buffer) {
  // determine the config type from the buffer and check if it supported
  if (buffer.size() == 0) {
    SPDLOG_ERROR("Buffer is empty");
    return;
  }

  buffer_ = std::move(buffer);
  auto header = airtree::core::common::deserializeHeader(buffer_);
  header_ = std::make_unique<airtree::core::common::AirTreeHeader>(header);
  offset_ = header.header_length;

  auto params = airtree::core::common::configParams(header);
  std::cout << "Header Dims: " << static_cast<int>(params.dims) << std::endl;
  std::cout << "Header Config: 0x" << std::hex
            << static_cast<int>(static_cast<uint8_t>(header.config))
            << std::dec << std::endl;

  specialCounts_ = std::make_unique<SpecialCounts>();

  specialCounts_->posInfCount = header.pos_inf_count;
  specialCounts_->negInfCount = header.neg_inf_count;
  specialCounts_->posZeroCount = header.pos_zero_count;
  specialCounts_->negZeroCount = header.neg_zero_count;
  specialCounts_->nanCount = header.nan_count;
}

BinBoundaryResult BinBoundary::generateBinBoundaries() {
  using airtree::core::common::ConfigWire;
  switch (header_->config) {
  case ConfigWire::Config_1D_Tiny: {
    auto res = buildBinBoundaries1DxT();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  case ConfigWire::Config_1D_Fast: {
    auto res = buildBinBoundaries1DxF();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  case ConfigWire::Config_1D_Precise: {
    auto res = buildBinBoundaries1DxP();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  case ConfigWire::Config_2D_Precise: {
    auto res = buildBinBoundaries2DxP();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  case ConfigWire::Config_3D_Precise: {
    auto res = buildBinBoundaries3DxP();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  case ConfigWire::Config_4D_Precise: {
    auto res = buildBinBoundaries4DxP();
    return BinBoundaryResult(std::move(specialCounts_), res);
  }
  default:
    SPDLOG_ERROR("Unsupported configuration: 0x{:02x}",
                 static_cast<uint8_t>(header_->config));
    throw std::runtime_error("Unsupported BinBoundary configuration");
  }
}


BinBoundary1D::BinBoundary1D(double min, double max, uint32_t count)
    : min_(min), max_(max), count_(count) {}

double BinBoundary1D::getLowerBound() const {
  return min_;
}

double BinBoundary1D::getUpperBound() const {
  return max_;
}

uint32_t BinBoundary1D::getCount() const {
  return count_;
}

void BinBoundary1D::setLowerBound(double min) {
  min_ = min;
}

void BinBoundary1D::setUpperBound(double max) {
  max_ = max;
}

void BinBoundary1D::setCount(uint32_t count) {
  count_ = count;
}


BinBoundary2D::BinBoundary2D(double min_x, double min_y, double max_x,
                             double max_y, uint32_t count)
    : min_x_(min_x), min_y_(min_y), max_x_(max_x), max_y_(max_y),
      count_(count) {}

double BinBoundary2D::getLowerBoundX() const {
  return min_x_;
}

double BinBoundary2D::getUpperBoundX() const {
  return max_x_;
}

double BinBoundary2D::getLowerBoundY() const {
  return min_y_;
}

double BinBoundary2D::getUpperBoundY() const {
  return max_y_;
}

uint32_t BinBoundary2D::getCount() const {
  return count_;
}

void BinBoundary2D::setLowerBoundX(double min_x) {
  min_x_ = min_x;
}

void BinBoundary2D::setUpperBoundX(double max_x) {
  max_x_ = max_x;
}

void BinBoundary2D::setLowerBoundY(double min_y) {
  min_y_ = min_y;
}

void BinBoundary2D::setUpperBoundY(double max_y) {
  max_y_ = max_y;
}

void BinBoundary2D::setCount(uint32_t count) {
  count_ = count;
}

BinBoundary3D::BinBoundary3D(double min_x, double min_y, double min_z,
                             double max_x, double max_y, double max_z,
                             uint32_t count)
    : min_x_(min_x), min_y_(min_y), min_z_(min_z), max_x_(max_x), max_y_(max_y),
      max_z_(max_z), count_(count) {}

double BinBoundary3D::getLowerBoundX() const {
  return min_x_;
}

double BinBoundary3D::getUpperBoundX() const {
  return max_x_;
}

double BinBoundary3D::getLowerBoundY() const {
  return min_y_;
}

double BinBoundary3D::getUpperBoundY() const {
  return max_y_;
}

double BinBoundary3D::getLowerBoundZ() const {
  return min_z_;
}

double BinBoundary3D::getUpperBoundZ() const {
  return max_z_;
}

uint32_t BinBoundary3D::getCount() const {
  return count_;
}

void BinBoundary3D::setLowerBoundX(double min_x) {
  min_x_ = min_x;
}

void BinBoundary3D::setUpperBoundX(double max_x) {
  max_x_ = max_x;
}

void BinBoundary3D::setLowerBoundY(double min_y) {
  min_y_ = min_y;
}

void BinBoundary3D::setUpperBoundY(double max_y) {
  max_y_ = max_y;
}

void BinBoundary3D::setLowerBoundZ(double min_z) {
  min_z_ = min_z;
}

void BinBoundary3D::setUpperBoundZ(double max_z) {
  max_z_ = max_z;
}

void BinBoundary3D::setCount(uint32_t count) {
  count_ = count;
}

BinBoundary4D::BinBoundary4D(double min_x, double min_y, double min_z,
                             double min_w, double max_x, double max_y,
                             double max_z, double max_w, uint32_t count)
    : min_x_(min_x), min_y_(min_y), min_z_(min_z), min_w_(min_w), max_x_(max_x),
      max_y_(max_y), max_z_(max_z), max_w_(max_w), count_(count) {}

double BinBoundary4D::getLowerBoundX() const {
  return min_x_;
}

double BinBoundary4D::getUpperBoundX() const {
  return max_x_;
}

double BinBoundary4D::getLowerBoundY() const {
  return min_y_;
}

double BinBoundary4D::getUpperBoundY() const {
  return max_y_;
}

double BinBoundary4D::getLowerBoundZ() const {
  return min_z_;
}

double BinBoundary4D::getUpperBoundZ() const {
  return max_z_;
}

double BinBoundary4D::getLowerBoundW() const {
  return min_w_;
}

double BinBoundary4D::getUpperBoundW() const {
  return max_w_;
}

uint32_t BinBoundary4D::getCount() const {
  return count_;
}
void BinBoundary4D::setLowerBoundX(double min_x) {
  min_x_ = min_x;
}

void BinBoundary4D::setUpperBoundX(double max_x) {
  max_x_ = max_x;
}

void BinBoundary4D::setLowerBoundY(double min_y) {
  min_y_ = min_y;
}

void BinBoundary4D::setUpperBoundY(double max_y) {
  max_y_ = max_y;
}

void BinBoundary4D::setLowerBoundZ(double min_z) {
  min_z_ = min_z;
}

void BinBoundary4D::setUpperBoundZ(double max_z) {
  max_z_ = max_z;
}

void BinBoundary4D::setLowerBoundW(double min_w) {
  min_w_ = min_w;
}

void BinBoundary4D::setUpperBoundW(double max_w) {
  max_w_ = max_w;
}

void BinBoundary4D::setCount(uint32_t count) {
  count_ = count;
}

BinBoundaryResult::BinBoundaryResult(
    std::unique_ptr<SpecialCounts> specialCounts,
    std::variant<BinBoundary1DList, BinBoundary2DList, BinBoundary3DList,
                 BinBoundary4DList>
        boundaries)
    : specialCounts_(std::move(specialCounts)),
      boundaries_(std::move(boundaries)) {}

std::unique_ptr<SpecialCounts> BinBoundaryResult::getSpecialCounts() const {
  return std::make_unique<SpecialCounts>(*specialCounts_);
}

std::unique_ptr<std::variant<BinBoundary1DList, BinBoundary2DList,
                             BinBoundary3DList, BinBoundary4DList>>
BinBoundaryResult::getBoundaries() const {
  return std::make_unique<std::variant<BinBoundary1DList, BinBoundary2DList,
                                       BinBoundary3DList, BinBoundary4DList>>(
      boundaries_);
}
