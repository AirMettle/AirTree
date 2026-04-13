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

/**
 * This function is used to check if a number is special or not
 *
 * @param fpNumber The number to check.
 * @param counts The counts of special cases.
 * @return True if the number is special, false otherwise.
 */
bool isSpecialCase(uint64_t fpNumber, SpecialCounts &counts);
bool isSpecialCase_32(uint32_t fpNumber, SpecialCounts &counts);


bool update_special_counts(const TLE &tle,
                           std::unique_ptr<SpecialCounts> &counts);


#endif // AIRTREE_CORE_COMMON_SPECIALCOUNTS_HPP