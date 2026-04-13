#ifndef AIRTREE_CORE_SERDES_COUNT_SER_HPP
#define AIRTREE_CORE_SERDES_COUNT_SER_HPP


#include <cstddef>
#include <cstdint>
#include <vector>


[[nodiscard]] std::vector<char> serializeCounts(const uint32_t counts[],
                                                size_t len);
[[nodiscard]] std::vector<uint32_t>
deserializeCounts(const std::vector<char> &buffer, size_t &offset, size_t len);

#endif // AIRTREE_CORE_SERDES_COUNT_SER_HPP