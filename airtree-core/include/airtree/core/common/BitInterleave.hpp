#ifndef AIRTREE_CORE_COMMON_BITINTERLEAVE_HPP
#define AIRTREE_CORE_COMMON_BITINTERLEAVE_HPP

#include <cstdint>
unsigned int interleave_2D(unsigned int x, unsigned int y);
unsigned int interleave_2D_10bit(unsigned int x, unsigned int y);
unsigned int interleave_3D_888(unsigned int x, unsigned int y, unsigned int z);
unsigned int interleavebits_2D(unsigned int x, unsigned int y);
unsigned int interleavebits_3D(unsigned int x, unsigned int y, unsigned int z);
uint64_t interleavebits_4D(unsigned int w, unsigned int x, unsigned int y,
                           unsigned int z);
unsigned int interleave_4D_8888(unsigned int x, unsigned int y, unsigned int z,
                                unsigned int w);


#endif // AIRTREE_CORE_COMMON_BITINTERLEAVE_HPP