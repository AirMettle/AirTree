#ifndef AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP
#define AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP


#include <airtree/core/common/TLE.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <utility>

std::pair<TLE, unsigned int> internal_8bit(const FPHArray &array,
                                           const uint64_t &i,
                                           const bool &default_mode);
std::pair<TLE, unsigned int> internal_10bit(const FPHArray &array,
                                            const uint64_t &i,
                                            const bool &default_mode);


unsigned int createInternal8Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal8Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal10Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal10Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal13Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal13Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal16Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal16Bit_32(uint32_t fpNumber, bool default_mode);
unsigned int createInternal20Bit(uint64_t fpNumber, bool default_mode);
unsigned int createInternal20Bit_32(uint32_t fpNumber, bool default_mode);

#endif // AIRTREE_CORE_COMMON_INTERNAL_ENCODING_HPP