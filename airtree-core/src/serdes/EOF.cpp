// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <cstdint>
#include <cstring>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/Logger.hpp>

using namespace airtree::core;


bool verifyEndOfFileMarker(std::span<const char> buffer, size_t &offset) {
  if (offset + sizeof(int32_t) > buffer.size()) {
    SPDLOG_LOGGER_ERROR(logger(), "Insufficient data for end marker.");
    return false;
  }

  int32_t endOfFileMarker;
  std::memcpy(&endOfFileMarker, &buffer[offset], sizeof(int32_t));
  if (endOfFileMarker == -1) {
    offset += sizeof(int32_t);
    return true;
  } else {
    SPDLOG_LOGGER_ERROR(
        logger(),
        "NODE DESERIALIZATION ERROR : End marker not found at expected "
        "position.");
    return false;
  }
}