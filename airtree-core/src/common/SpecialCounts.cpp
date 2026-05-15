// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/SpecialCounts.hpp>


bool isSpecialCase(uint64_t fpNumber, SpecialCounts &counts) {
  uint64_t isInfOrNan = (fpNumber & (0x7FFULL << 52)) == (0x7FFULL << 52);

  uint64_t isZero = !(fpNumber & ~(1ULL << 63));

  if (!(isInfOrNan || isZero))
    return false;
  uint64_t signBit = ((fpNumber >> 63) & 1);
  if (isInfOrNan) {
    uint64_t isNaN = (fpNumber & 0xFFFFFFFFFFFFF);

    if (isNaN) {
      counts.nanCount++;
    } else {
      counts.posInfCount += !signBit;
      counts.negInfCount += signBit;
    }
  } else {
    counts.posZeroCount += !signBit;
    counts.negZeroCount += signBit;
  }

  return true;
}

bool isSpecialCase_32(uint32_t fpNumber, SpecialCounts &counts) {
  uint32_t isInfOrNan = (fpNumber & (0xFFU << 23)) == (0xFFU << 23);
  uint32_t isZero = !(fpNumber & ~(1U << 31));

  if (!(isInfOrNan || isZero)) {
    return false;
  }
  uint32_t signBit = (fpNumber >> 31) & 1U;

  if (isInfOrNan) {
    uint32_t isNaN = (fpNumber & 0x7FFFFF);

    if (isNaN) {
      counts.nanCount++;
    } else {
      counts.posInfCount += !signBit;
      counts.negInfCount += signBit;
    }
  } else {
    counts.posZeroCount += !signBit;
    counts.negZeroCount += signBit;
  }

  return true;
}

bool update_special_counts(const TLE &tle,
                           std::unique_ptr<SpecialCounts> &counts) {
  if (!counts) {
    return false; // Return false if counts pointer is null
  }

  switch (tle.encoding) {
  case 0: // NaN
    if (counts) {
      counts->nanCount++;
    }
    return true;
  case 1: // Positive Infinity
    if (counts) {
      counts->posInfCount++;
    }
    return true;
  case 4: // Zero
    if (counts) {
      counts->posZeroCount++;
    }
    return true;
  case 7: // Negative Infinity
    if (counts) {
      counts->negInfCount++;
    }
    return true;
  default:
    return false;
  }
}
