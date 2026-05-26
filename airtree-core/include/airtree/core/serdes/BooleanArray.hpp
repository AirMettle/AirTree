// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP
#define AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

#include <airtree/core/common/BooleanArray.hpp>

[[nodiscard]] std::vector<char>
serializeCompactBooleanArray(const BooleanArray &array);
[[nodiscard]] std::vector<uint64_t>
deserializeCompactBooleanArray(const std::vector<char> &buffer, size_t &offset,
                               std::size_t len);


#endif // AIRTREE_CORE_SERDES_BOOLEANARRAYSER_HPP