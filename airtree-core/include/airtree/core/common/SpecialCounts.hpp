// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_SPECIALCOUNTS_HPP
#define AIRTREE_CORE_COMMON_SPECIALCOUNTS_HPP


#include <airtree/core/common/TLE.hpp>
#include <memory>

/**
 * Structure defining the Special Counts from the array.
 *
 * @param posInfCount Count of positive infinity values stored.
 * @param negInfCount Count of negative infinity values stored.
 * @param posZeroCount Count of positive zero values stored.
 * @param negZeroCount Count of negative zero values stored.
 * @param nanCount Count of NaN (Not a Number) values stored.
 */

struct SpecialCounts {
  int posInfCount = 0;
  int negInfCount = 0;
  int posZeroCount = 0;
  int negZeroCount = 0;
  int nanCount = 0;
  int posneg = 0;
  int pospos = 0;
  int negpos = 0;
  int negneg = 0;
};

inline bool isSpecialCase(uint64_t fpNumber, SpecialCounts &counts) {
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

inline bool isSpecialCase_32(uint32_t fpNumber, SpecialCounts &counts) {
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

inline bool update_special_counts(const TLE &tle,
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


#endif // AIRTREE_CORE_COMMON_SPECIALCOUNTS_HPP