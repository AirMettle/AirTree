// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_BITCODEC_HPP
#define AIRTREE_CORE_COMMON_BITCODEC_HPP

#include <cstdint>
#include <airtree/core/common/ChunkTypes.hpp>

uint64_t combine_chunks_10b(unsigned int x, unsigned int y);
uint64_t combine_chunks_10b(unsigned int x, unsigned int y, unsigned int z);
uint64_t combine_chunks_10b(uint64_t x, uint64_t y, uint64_t z, uint64_t w);

unsigned int combine_chunks_8b(unsigned int x, unsigned int y);
unsigned int combine_chunks_8b_temp(unsigned int x, unsigned int y);
unsigned int combine_chunks_8b(unsigned int x, unsigned int y, unsigned int z);
unsigned int combine_chunks_8b(unsigned int x, unsigned int y, unsigned int z,
                               unsigned int w);

ChunkPair reverse_combine_chunks_8b(unsigned int result);
ChunkTriple reverse_combine_chunks_8b_3(unsigned int result);
ChunkQuad reverse_combine_chunks_8b_4(unsigned int result);
ChunkPair reverse_combine_chunks_10b(uint64_t result);
ChunkTriple reverse_combine_chunks_10b_3(uint64_t result);
ChunkQuad reverse_combine_chunks_10b_4(uint64_t result);
ChunkPair reverse_combine_chunks_8b_temp(unsigned int result);


#endif // AIRTREE_CORE_COMMON_BITCODEC_HPP