// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_POPULATED_HPP
#define AIRTREE_CORE_COMMON_POPULATED_HPP

#include <bit>
#include <cstddef>
#include <cstdint>

template <std::size_t N>
struct PopulatedBins {
  static constexpr std::size_t kWords = (N + 63) / 64;
  uint64_t words[kWords] = {};

  bool test(std::size_t i) const { return (words[i >> 6] >> (i & 63)) & 1; }
  bool operator[](std::size_t i) const { return test(i); }
  void set(std::size_t i) { words[i >> 6] |= uint64_t{1} << (i & 63); }
  void reset() {
    for (auto &w : words)
      w = 0;
  }
  std::size_t count() const {
    std::size_t n = 0;
    for (auto w : words)
      n += std::popcount(w);
    return n;
  }
  PopulatedBins operator|(const PopulatedBins &o) const {
    PopulatedBins r;
    for (std::size_t w = 0; w < kWords; ++w)
      r.words[w] = words[w] | o.words[w];
    return r;
  }
};

#endif // AIRTREE_CORE_COMMON_POPULATED_HPP
