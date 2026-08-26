// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_EOF_HPP
#define AIRTREE_CORE_SERDES_EOF_HPP


#include <span>
#include <cstddef>
#include <vector>


[[nodiscard]] bool verifyEndOfFileMarker(std::span<const char> buffer,
                                         size_t &offset);


#endif // AIRTREE_CORE_SERDES_EOF_HPP