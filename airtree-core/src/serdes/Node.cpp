// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/core/serdes/Node.hpp>

#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>

void writeNode(const uint64_t *mask, std::size_t bins, const uint32_t *counts,
               std::vector<char> &out) {
  writePopulatedMask(mask, bins, out);
  serializeCounts(mask, bins, counts, out);
}

void writeEndOfFileMarker(std::vector<char> &out) {
  out.insert(out.end(), 4, static_cast<char>(0xFF)); // int32 -1
}
