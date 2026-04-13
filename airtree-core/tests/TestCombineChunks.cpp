#include <bitset>
#include <gtest/gtest.h>
#include <airtree/core/AirTreeCore_internal.hpp>

TEST(CombineChunkTest2D, TestCase1) {
  unsigned int x = 0b1010101001;
  unsigned int y = 0b1111111111;
  unsigned int expected_result = 0b10101111101001111111;

  EXPECT_EQ(combine_chunks_10b(x, y), expected_result);
}

TEST(CombineChunkTest2D, TestCase2) {
  unsigned int x = 0b1010101010;
  unsigned int y = 0b0101010101;
  unsigned int expected_result = 0b10100101101010010101;

  EXPECT_EQ(combine_chunks_10b(x, y), expected_result);
}

TEST(CombineChunkTest2D, TestCase3) {
  unsigned int x = 0b11010101010;
  unsigned int y = 0b1111111111;

  // The function is expected to return 0 due to the error in `x`
  unsigned int expected_result = 0;

  EXPECT_EQ(combine_chunks_10b(x, y), expected_result);
}

TEST(CombineChunkTest3D, TestCase1) {
  unsigned int x = 0b1010101010;
  unsigned int y = 0b1111001101;
  unsigned int z = 0b1010101010;

  unsigned int expected_result = 0b101011111010101010001101101010;

  EXPECT_EQ(combine_chunks_10b(x, y, z), expected_result);
}

TEST(CombineChunkTest4D, TestCase1) {
  unsigned int x = 0b1010101010;
  unsigned int y = 0b1111001101;
  unsigned int z = 0b1010101010;
  unsigned int w = 0b1111001101;

  uint64_t expected_result = 0b1010111110101111101010001101101010001101;

  EXPECT_EQ(combine_chunks_10b(x, y, z, w), expected_result);
}
