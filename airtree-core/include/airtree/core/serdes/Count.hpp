// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_COUNT_SER_HPP
#define AIRTREE_CORE_SERDES_COUNT_SER_HPP


#include <span>
#include <cstddef>
#include <cstdint>
#include <vector>


[[nodiscard]] std::vector<char> serializeCounts(const uint32_t counts[],
                                                size_t len);
[[nodiscard]] std::vector<uint32_t>
deserializeCounts(std::span<const char> buffer, size_t &offset, size_t len);

// One count per set mask bit, ascending, into counts[bit]; false on underflow or a bit >= bins.
[[nodiscard]] bool deserializeCounts(std::span<const char> buffer,
                                     size_t &offset, const uint64_t *mask,
                                     size_t bins, uint32_t *counts);

// Appends the count payload (width byte + packed counts) for the set mask bits; same bytes as
// serializeCounts(counts, bins) when counts are non-zero exactly at the mask bits.
void serializeCounts(const uint64_t *mask, size_t bins, const uint32_t *counts,
                     std::vector<char> &out);
void serializeCountsPacked(const uint32_t *values, std::size_t n, std::vector<char> &out);

// Advances past a count payload without decoding it; false on underflow.
[[nodiscard]] bool skipCounts(std::span<const char> buffer, size_t &offset,
                              const uint64_t *mask, size_t bins);

#endif // AIRTREE_CORE_SERDES_COUNT_SER_HPP