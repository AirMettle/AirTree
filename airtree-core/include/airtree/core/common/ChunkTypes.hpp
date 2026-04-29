#ifndef AIRTREE_CORE_COMMON_CHUNK_TYPES_HPP
#define AIRTREE_CORE_COMMON_CHUNK_TYPES_HPP

#include <cstdint>

typedef struct {
  unsigned int x;
  unsigned int y;
} ChunkPair;

typedef struct {
  unsigned int x;
  unsigned int y;
  unsigned int z;
} ChunkTriple;

typedef struct {
  unsigned int x;
  unsigned int y;
  unsigned int z;
  unsigned int w;
} ChunkQuad;

#endif // AIRTREE_CORE_COMMON_CHUNK_TYPES_HPP