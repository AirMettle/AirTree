// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP
#define AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP

#include <bit>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <airtree/core/common/BooleanArray.hpp>

[[nodiscard]] std::vector<char>
serializeCompactBooleanArray(const BooleanArray &array);
[[nodiscard]] std::vector<uint64_t>
deserializeCompactBooleanArray(const std::vector<char> &buffer, size_t &offset,
                               std::size_t len);


// Populated-mask words for `bins` slots, as written by serializeCompactBooleanArray; false on underflow.
[[nodiscard]] bool readPopulatedMask(const std::vector<char> &buffer,
                                     size_t &offset, uint64_t *words,
                                     std::size_t bins);

// Appends the mask words for `bins` slots (inverse of readPopulatedMask).
void writePopulatedMask(const uint64_t *words, std::size_t bins,
                        std::vector<char> &out);

template <typename Fn>
void forEachSetBit(const uint64_t *words, std::size_t bins, Fn &&fn) {
  for (std::size_t w = 0; w < (bins + 63) / 64; ++w) {
    for (uint64_t m = words[w]; m != 0; m &= m - 1) {
      fn(w * 64 + static_cast<std::size_t>(std::countr_zero(m)));
    }
  }
}

template <std::size_t N>
void setPopulated(std::bitset<N> &populated, const uint64_t *words) {
  forEachSetBit(words, N, [&](std::size_t i) { populated.set(i); });
}

#endif // AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP