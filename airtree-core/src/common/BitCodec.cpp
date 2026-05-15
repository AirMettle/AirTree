// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/BitCodec.hpp>
#include <airtree/core/Logger.hpp>


using namespace airtree::core;

uint64_t combine_chunks_10b(unsigned int x, unsigned int y) {
  if (x > 0x3FF || y > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 10 bits.");
    return 0;
  }

  // Extract first 4 bits from both x and y
  unsigned int first_4_bits_x = (x >> 6) & 0xF;
  unsigned int first_4_bits_y = (y >> 6) & 0xF;

  // Extract last 6 bits from both x and y
  unsigned int last_6_bits_x = x & 0x3F;
  unsigned int last_6_bits_y = y & 0x3F;

  // Combine into a 20-bit number
  uint64_t result = 0;

  // Place first 4 bits of x in bits 19-16 of the result
  result |= (first_4_bits_x << 16);

  // Place first 4 bits of y in bits 15-12 of the result
  result |= (first_4_bits_y << 12);

  // Place last 6 bits of x in bits 11-6 of the result
  result |= (last_6_bits_x << 6);

  // Place last 6 bits of y in bits 5-0 of the result
  result |= last_6_bits_y;

  return result;
}

uint64_t combine_chunks_10b(unsigned int x, unsigned int y, unsigned int z) {
  if (x > 0x3FF || y > 0x3FF || z > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 10 bits.");
    return 0;
  }

  // Extract first 4 bits from both x and y
  uint64_t first_4_bits_x = (x >> 6) & 0xF;
  uint64_t first_4_bits_y = (y >> 6) & 0xF;
  uint64_t first_4_bits_z = (z >> 6) & 0xF;

  // Extract last 6 bits from both x and y
  uint64_t last_6_bits_x = x & 0x3F;
  uint64_t last_6_bits_y = y & 0x3F;
  uint64_t last_6_bits_z = z & 0x3F;

  // Combine into a 20-bit number
  uint64_t result = 0;

  // Place first 4 bits of x in bits 29-26 of the result
  result |= (first_4_bits_x << 26);

  // Place first 4 bits of y in bits 25-22 of the result
  result |= (first_4_bits_y << 22);

  // Place first 4 bits of z in bits 21-18 of the result
  result |= (first_4_bits_z << 18);

  // Place last 6 bits of x in bits 17-12 of the result
  result |= (last_6_bits_x << 12);

  // Place last 6 bits of y in bits 11-6 of the result
  result |= (last_6_bits_y << 6);

  // Place last 6 bits of z in bits 5-0 of the result
  result |= last_6_bits_z;

  return result;
}

uint64_t combine_chunks_10b(uint64_t x, uint64_t y, uint64_t z, uint64_t w) {
  if (w > 0x3FF || x > 0x3FF || y > 0x3FF || z > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 10 bits.");
    return 0;
  }

  // Extract first 4 bits from w, x, y, and z
  uint64_t first_4_bits_x = (x >> 6) & 0xF;
  uint64_t first_4_bits_y = (y >> 6) & 0xF;
  uint64_t first_4_bits_z = (z >> 6) & 0xF;
  uint64_t first_4_bits_w = (w >> 6) & 0xF;

  // Extract last 6 bits from w, x, y, and z
  uint64_t last_6_bits_x = x & 0x3F;
  uint64_t last_6_bits_y = y & 0x3F;
  uint64_t last_6_bits_z = z & 0x3F;
  uint64_t last_6_bits_w = w & 0x3F;

  // Combine into a 40-bit number
  uint64_t result = 0;

  // Place first 4 bits of w in bits 39-36 of the result
  result |= (first_4_bits_x << 36);

  // Place first 4 bits of x in bits 35-32 of the result
  result |= (first_4_bits_y << 32);

  // Place first 4 bits of y in bits 31-28 of the result
  result |= (first_4_bits_z << 28);

  // Place first 4 bits of z in bits 27-24 of the result
  result |= (first_4_bits_w << 24);

  // Place last 6 bits of w in bits 23-18 of the result
  result |= (last_6_bits_x << 18);

  // Place last 6 bits of x in bits 17-12 of the result
  result |= (last_6_bits_y << 12);

  // Place last 6 bits of y in bits 11-6 of the result
  result |= (last_6_bits_z << 6);

  // Place last 6 bits of z in bits 5-0 of the result
  result |= last_6_bits_w;

  return result;
}

unsigned int combine_chunks_8b(unsigned int x, unsigned int y) {
  if (x > 0xFF || y > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 8 bits.");
    return 0;
  }

  // Extract first 3 bits from both x and y
  unsigned int first_3_bits_x = (x >> 5) & 0x7;
  unsigned int first_3_bits_y = (y >> 5) & 0x7;

  // Extract last 5 bits from both x and y
  unsigned int last_5_bits_x = x & 0x1F;
  unsigned int last_5_bits_y = y & 0x1F;

  // Combine into a 16-bit number
  unsigned int result = 0;

  // Place first 3 bits of x in bits 15-13 of the result
  result |= (first_3_bits_x << 13);

  // Place first 3 bits of y in bits 12-10 of the result
  result |= (first_3_bits_y << 10);

  // Place last 5 bits of x in bits 9-5 of the result
  result |= (last_5_bits_x << 5);

  // Place last 5 bits of y in bits 4-0 of the result
  result |= last_5_bits_y;

  return result;
}

unsigned int combine_chunks_8b_temp(unsigned int x, unsigned int y) {
  if (x > 0xFF || y > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 8 bits.");
    return 0;
  }

  // Extract first 3 bits from both x and y
  unsigned int first_3_bits_x = (x >> 5) & 0x7;
  unsigned int first_3_bits_y = (y >> 5) & 0x7;

  // Extract the first bit of the last 5 bits of x and y
  unsigned int first_bit_last_5_bits_x = (x >> 4) & 0x1;
  unsigned int first_bit_last_5_bits_y = (y >> 4) & 0x1;

  // Extract the last 4 bits of x and y
  unsigned int last_4_bits_x = x & 0xF;
  unsigned int last_4_bits_y = y & 0xF;

  // Combine into a 16-bit number
  unsigned int result = 0;

  // Place first 3 bits of x in bits 15-13 of the result
  result |= (first_3_bits_x << 13);

  // Place first 3 bits of y in bits 12-10 of the result
  result |= (first_3_bits_y << 10);

  // Place the first bit of the last 5 bits of x in bit 9 of the result
  result |= (first_bit_last_5_bits_x << 9);

  // Place the first bit of the last 5 bits of y in bit 8 of the result
  result |= (first_bit_last_5_bits_y << 8);

  // Place last 4 bits of x in bits 7-4 of the result
  result |= (last_4_bits_x << 4);

  // Place last 4 bits of y in bits 3-0 of the result
  result |= last_4_bits_y;

  return result;
}

unsigned int combine_chunks_8b(unsigned int x, unsigned int y, unsigned int z) {
  if (x > 0xFF || y > 0xFF || z > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 8 bits.");
    return 0;
  }

  // Extract first 3 bits from both x, y, and z
  unsigned int first_3_bits_x = (x >> 5) & 0x7;
  unsigned int first_3_bits_y = (y >> 5) & 0x7;
  unsigned int first_3_bits_z = (z >> 5) & 0x7;

  // Extract last 5 bits from both x, y, and z
  unsigned int last_5_bits_x = x & 0x1F;
  unsigned int last_5_bits_y = y & 0x1F;
  unsigned int last_5_bits_z = z & 0x1F;

  // Combine into a 24-bit number
  unsigned int result = 0;

  // Place first 3 bits of x in bits 23-21 of the result
  result |= (first_3_bits_x << 21);

  // Place first 3 bits of y in bits 20-18 of the result
  result |= (first_3_bits_y << 18);

  // Place first 3 bits of z in bits 17-15 of the result
  result |= (first_3_bits_z << 15);

  // Place last 5 bits of x in bits 14-10 of the result
  result |= (last_5_bits_x << 10);

  // Place last 5 bits of y in bits 9-5 of the result
  result |= (last_5_bits_y << 5);

  // Place last 5 bits of z in bits 4-0 of the result
  result |= last_5_bits_z;

  return result;
}

unsigned int combine_chunks_8b(unsigned int w, unsigned int x, unsigned int y,
                               unsigned int z) {
  if (w > 0xFF || x > 0xFF || y > 0xFF || z > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 8 bits.");
    return 0;
  }

  // Extract first 3 bits from w, x, y, and z
  unsigned int first_3_bits_w = (w >> 5) & 0x7;
  unsigned int first_3_bits_x = (x >> 5) & 0x7;
  unsigned int first_3_bits_y = (y >> 5) & 0x7;
  unsigned int first_3_bits_z = (z >> 5) & 0x7;

  // Extract last 5 bits from w, x, y, and z
  unsigned int last_5_bits_w = w & 0x1F;
  unsigned int last_5_bits_x = x & 0x1F;
  unsigned int last_5_bits_y = y & 0x1F;
  unsigned int last_5_bits_z = z & 0x1F;

  // Combine into a 32-bit number
  unsigned int result = 0;

  // Place first 3 bits of w in bits 31-29 of the result
  result |= (first_3_bits_w << 29);

  // Place first 3 bits of x in bits 28-26 of the result
  result |= (first_3_bits_x << 26);

  // Place first 3 bits of y in bits 25-23 of the result
  result |= (first_3_bits_y << 23);

  // Place first 3 bits of z in bits 22-20 of the result
  result |= (first_3_bits_z << 20);

  // Place last 5 bits of w in bits 19-15 of the result
  result |= (last_5_bits_w << 15);

  // Place last 5 bits of x in bits 14-10 of the result
  result |= (last_5_bits_x << 10);

  // Place last 5 bits of y in bits 9-5 of the result
  result |= (last_5_bits_y << 5);

  // Place last 5 bits of z in bits 4-0 of the result
  result |= last_5_bits_z;

  return result;
}

ChunkPair reverse_combine_chunks_8b(unsigned int result) {
  ChunkPair pair = {0, 0};

  // Extract first 3 bits of x from bits 15-13 of result
  unsigned int first_3_bits_x = (result >> 13) & 0x7;

  // Extract first 3 bits of y from bits 12-10 of result
  unsigned int first_3_bits_y = (result >> 10) & 0x7;

  // Extract last 5 bits of x from bits 9-5 of result
  unsigned int last_5_bits_x = (result >> 5) & 0x1F;

  // Extract last 5 bits of y from bits 4-0 of result
  unsigned int last_5_bits_y = result & 0x1F;

  // reConstruct x: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  pair.x = (first_3_bits_x << 5) | last_5_bits_x;

  // reConstruct y: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  pair.y = (first_3_bits_y << 5) | last_5_bits_y;

  // Verify that x and y are within 8-bit range (optional, for robustness)
  if (pair.x > 0xFF || pair.y > 0xFF) {
    std::fprintf(stderr, "Error: Extracted values exceed 8 bits.\n");
    pair.x = 0;
    pair.y = 0;
  }

  return pair;
}

ChunkTriple reverse_combine_chunks_8b_3(unsigned int result) {
  ChunkTriple triple = {0, 0, 0};

  // Extract first 3 bits of x from bits 23-21 of result
  unsigned int first_3_bits_x = (result >> 21) & 0x7;

  // Extract first 3 bits of y from bits 20-18 of result
  unsigned int first_3_bits_y = (result >> 18) & 0x7;

  // Extract first 3 bits of z from bits 17-15 of result
  unsigned int first_3_bits_z = (result >> 15) & 0x7;

  // Extract last 5 bits of x from bits 14-10 of result
  unsigned int last_5_bits_x = (result >> 10) & 0x1F;

  // Extract last 5 bits of y from bits 9-5 of result
  unsigned int last_5_bits_y = (result >> 5) & 0x1F;

  // Extract last 5 bits of z from bits 4-0 of result
  unsigned int last_5_bits_z = result & 0x1F;

  // reConstruct x: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  triple.x = (first_3_bits_x << 5) | last_5_bits_x;

  // reConstruct y: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  triple.y = (first_3_bits_y << 5) | last_5_bits_y;

  // reConstruct z: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  triple.z = (first_3_bits_z << 5) | last_5_bits_z;

  // Verify that x, y, z are within 8-bit range
  if (triple.x > 0xFF || triple.y > 0xFF || triple.z > 0xFF) {
    std::fprintf(stderr, "Error: Extracted values exceed 8 bits.\n");
    triple.x = 0;
    triple.y = 0;
    triple.z = 0;
  }

  return triple;
}

ChunkQuad reverse_combine_chunks_8b_4(unsigned int result) {
  ChunkQuad quad = {0, 0, 0, 0};

  // Extract first 3 bits of w from bits 31-29 of result
  unsigned int first_3_bits_w = (result >> 29) & 0x7;

  // Extract first 3 bits of x from bits 28-26 of result
  unsigned int first_3_bits_x = (result >> 26) & 0x7;

  // Extract first 3 bits of y from bits 25-23 of result
  unsigned int first_3_bits_y = (result >> 23) & 0x7;

  // Extract first 3 bits of z from bits 22-20 of result
  unsigned int first_3_bits_z = (result >> 20) & 0x7;

  // Extract last 5 bits of w from bits 19-15 of result
  unsigned int last_5_bits_w = (result >> 15) & 0x1F;

  // Extract last 5 bits of x from bits 14-10 of result
  unsigned int last_5_bits_x = (result >> 10) & 0x1F;

  // Extract last 5 bits of y from bits 9-5 of result
  unsigned int last_5_bits_y = (result >> 5) & 0x1F;

  // Extract last 5 bits of z from bits 4-0 of result
  unsigned int last_5_bits_z = result & 0x1F;

  // reConstruct w: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  quad.w = (first_3_bits_w << 5) | last_5_bits_w;

  // reConstruct x: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  quad.x = (first_3_bits_x << 5) | last_5_bits_x;

  // reConstruct y: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  quad.y = (first_3_bits_y << 5) | last_5_bits_y;

  // reConstruct z: first 3 bits (shifted to bits 7-5) | last 5 bits (bits 4-0)
  quad.z = (first_3_bits_z << 5) | last_5_bits_z;

  // Verify that w, x, y, z are within 8-bit range
  if (quad.w > 0xFF || quad.x > 0xFF || quad.y > 0xFF || quad.z > 0xFF) {
    std::fprintf(stderr, "Error: Extracted values exceed 8 bits.\n");
    quad.w = 0;
    quad.x = 0;
    quad.y = 0;
    quad.z = 0;
  }

  return quad;
}

ChunkPair reverse_combine_chunks_10b(uint64_t result) {
  ChunkPair pair = {0, 0};

  // result was built as a 20-bit value → valid range: 0 .. (2^20−1)
  if (result > 0xFFFFF) {
    std::fprintf(stderr, "Error: result exceeds 20 bits.\n");
    return pair;
  }

  // Extract each field from its bit-position:
  unsigned int first4_x = (result >> 16) & 0xF; // bits 19..16
  unsigned int first4_y = (result >> 12) & 0xF; // bits 15..12
  unsigned int last6_x = (result >> 6) & 0x3F;  // bits 11..6
  unsigned int last6_y = result & 0x3F;         // bits 5..0

  // reConstruct the original 10-bit values:
  pair.x = (first4_x << 6) | last6_x;
  pair.y = (first4_y << 6) | last6_y;

  return pair;
}

ChunkTriple reverse_combine_chunks_10b_3(uint64_t result) {
  ChunkTriple tri = {0, 0, 0};

  // valid range: 0 .. (2^30−1)
  if (result > 0x3FFFFFFF) {
    std::fprintf(stderr, "Error: result exceeds 30 bits.\n");
    return tri;
  }

  unsigned int first4_x = (result >> 26) & 0xF; // bits 29..26
  unsigned int first4_y = (result >> 22) & 0xF; // bits 25..22
  unsigned int first4_z = (result >> 18) & 0xF; // bits 21..18
  unsigned int last6_x = (result >> 12) & 0x3F; // bits 17..12
  unsigned int last6_y = (result >> 6) & 0x3F;  // bits 11..6
  unsigned int last6_z = result & 0x3F;         // bits 5..0

  tri.x = (first4_x << 6) | last6_x;
  tri.y = (first4_y << 6) | last6_y;
  tri.z = (first4_z << 6) | last6_z;

  return tri;
}

ChunkQuad reverse_combine_chunks_10b_4(uint64_t result) {
  ChunkQuad quad = {0, 0, 0, 0};

  // valid range: 0 .. (2^40−1)
  if (result > 0xFFFFFFFFFFULL) {
    std::fprintf(stderr, "Error: result exceeds 40 bits.\n");
    return quad;
  }

  unsigned int first4_x = (result >> 36) & 0xF; // bits 39..36
  unsigned int first4_y = (result >> 32) & 0xF; // bits 35..32
  unsigned int first4_z = (result >> 28) & 0xF; // bits 31..28
  unsigned int first4_w = (result >> 24) & 0xF; // bits 27..24
  unsigned int last6_x = (result >> 18) & 0x3F; // bits 23..18
  unsigned int last6_y = (result >> 12) & 0x3F; // bits 17..12
  unsigned int last6_z = (result >> 6) & 0x3F;  // bits 11..6
  unsigned int last6_w = result & 0x3F;         // bits 5..0

  quad.x = (first4_x << 6) | last6_x;
  quad.y = (first4_y << 6) | last6_y;
  quad.z = (first4_z << 6) | last6_z;
  quad.w = (first4_w << 6) | last6_w;

  return quad;
}

ChunkPair reverse_combine_chunks_8b_temp(unsigned int result) {
  ChunkPair pair = {0, 0};

  // first 3 bits of x → bits 15..13
  unsigned int first_3_bits_x = (result >> 13) & 0x7;
  // first 3 bits of y → bits 12..10
  unsigned int first_3_bits_y = (result >> 10) & 0x7;

  // first bit of the “last 5” for x → bit 9
  unsigned int first_bit_last5_x = (result >> 9) & 0x1;
  // first bit of the “last 5” for y → bit 8
  unsigned int first_bit_last5_y = (result >> 8) & 0x1;

  // remaining low-4 bits for x → bits 7..4
  unsigned int last4_bits_x = (result >> 4) & 0xF;
  // remaining low-4 bits for y → bits 3..0
  unsigned int last4_bits_y = result & 0xF;

  // reconstruct x,y: 3 high bits | 1 next bit | 4 low bits
  pair.x = (first_3_bits_x << 5) | (first_bit_last5_x << 4) | last4_bits_x;
  pair.y = (first_3_bits_y << 5) | (first_bit_last5_y << 4) | last4_bits_y;

  return pair;
}