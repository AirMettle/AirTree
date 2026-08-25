// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_COUNT_SER_HPP
#define AIRTREE_CORE_SERDES_COUNT_SER_HPP


#include <cstddef>
#include <cstdint>
#include <vector>


[[nodiscard]] std::vector<char> serializeCounts(const uint32_t counts[],
                                                size_t len);
[[nodiscard]] std::vector<uint32_t>
deserializeCounts(const std::vector<char> &buffer, size_t &offset, size_t len);

// One count per set mask bit, ascending, into counts[bit]; false on underflow or a bit >= bins.
[[nodiscard]] bool deserializeCounts(const std::vector<char> &buffer,
                                     size_t &offset, const uint64_t *mask,
                                     size_t bins, uint32_t *counts);

#endif // AIRTREE_CORE_SERDES_COUNT_SER_HPP