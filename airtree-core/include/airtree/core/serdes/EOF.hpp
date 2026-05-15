// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_SERDES_EOF_HPP
#define AIRTREE_CORE_SERDES_EOF_HPP


#include <cstddef>
#include <vector>


[[nodiscard]] bool verifyEndOfFileMarker(const std::vector<char> &buffer,
                                         size_t &offset);


#endif // AIRTREE_CORE_SERDES_EOF_HPP