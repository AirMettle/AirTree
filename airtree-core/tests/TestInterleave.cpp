#include <bitset>
#include <gtest/gtest.h>
#include <iostream>

#include <airtree/core/AirTreeCore_internal.hpp>


TEST(InterleaveTest_2D_10bit, HandleValidInputs) {
  unsigned int x = 0b1010101000;
  unsigned int y = 0b1111111111;
  unsigned int expected_result =
      0b11011101110111010101; // Expected interleaved result

  EXPECT_EQ(interleave_2D_10bit(x, y), expected_result);
}


TEST(InterleaveTest_2D, HandlesValidInputs) {
  unsigned int x = 0b10101010;
  unsigned int y = 0b11111111;
  unsigned int expected_result = 0b1101110111011101;

  EXPECT_EQ(interleave_2D(x, y), expected_result);
}

TEST(InterleaveTest_2D, HandlesInputExceeding8Bits) {
  unsigned int x = 0x1FF; // Exceeds 8 bits
  unsigned int y = 0b11111111;
  unsigned int expected_result = 0; // Should return 0 due to error handling

  EXPECT_EQ(interleave_2D(x, y), expected_result);
}

TEST(InterleaveTest_2D, HandlesZeroInput) {
  unsigned int x = 0b00000000;
  unsigned int y = 0b00000000;
  unsigned int expected_result = 0b0000000000000000;

  EXPECT_EQ(interleave_2D(x, y), expected_result);
}

TEST(InterleaveTest_2D, HandlesMaxValues) {
  unsigned int x = 0b11111111;
  unsigned int y = 0b11111111;
  unsigned int expected_result = 0b1111111111111111;

  EXPECT_EQ(interleave_2D(x, y), expected_result);
}


// 3D Interleave Test

TEST(InterleaveTest_3D, HandlesValidInputs) {
  unsigned int x = 0b10101010;
  unsigned int y = 0b11001100;
  unsigned int z = 0b11110000;
  unsigned int expected_result =
      0b111011101001110010100000; // Expected interleaved result

  EXPECT_EQ(interleave_3D_888(x, y, z), expected_result);
}

TEST(InterleaveTest_3D, HandlesInputExceeding8Bits) {
  unsigned int x = 0x1FF; // Exceeds 8 bits
  unsigned int y = 0b11111111;
  unsigned int z = 0b00000000;
  unsigned int expected_result = 0; // Should return 0 due to error handling

  EXPECT_EQ(interleave_3D_888(x, y, z), expected_result);
}

TEST(InterleaveTest_3D, HandlesZeroInput) {
  unsigned int x = 0b00000000;
  unsigned int y = 0b00000000;
  unsigned int z = 0b00000000;
  unsigned int expected_result =
      0b000000000000000000000000; // Expected interleaved result

  EXPECT_EQ(interleave_3D_888(x, y, z), expected_result);
}

TEST(InterleaveTest_3D, HandlesMaxValues) {
  unsigned int x = 0b11111111;
  unsigned int y = 0b11111111;
  unsigned int z = 0b11111111;
  unsigned int expected_result =
      0b111111111111111111111111; // Expected interleaved result

  EXPECT_EQ(interleave_3D_888(x, y, z), expected_result);
}

// 4D Interleave Test

TEST(InterleaveTest_4D, HandlesValidInputs) {
  unsigned int x = 0b10101010;
  unsigned int y = 0b11001100;
  unsigned int z = 0b11110000;
  unsigned int w = 0b00001111;
  unsigned int expected_result =
      0b11100110101000101101010110010001; // Expected interleaved result

  EXPECT_EQ(interleave_4D_8888(x, y, z, w), expected_result);
}

TEST(InterleaveTest_4D, HandlesInputExceeding8Bits) {
  unsigned int x = 0x1FF; // Exceeds 8 bits
  unsigned int y = 0b11111111;
  unsigned int z = 0b00000000;
  unsigned int w = 0b00000000;
  unsigned int expected_result = 0; // Should return 0 due to error handling

  EXPECT_EQ(interleave_4D_8888(x, y, z, w), expected_result);
}

TEST(InterleaveTest_4D, HandlesZeroInput) {
  unsigned int x = 0b00000000;
  unsigned int y = 0b00000000;
  unsigned int z = 0b00000000;
  unsigned int w = 0b00000000;
  unsigned int expected_result = 0;

  EXPECT_EQ(interleave_4D_8888(x, y, z, w), expected_result);
}

TEST(InterleaveTest_4D, HandlesMaxValues) {
  unsigned int x = 0b11111111;
  unsigned int y = 0b11111111;
  unsigned int z = 0b11111111;
  unsigned int w = 0b11111111;
  unsigned int expected_result = 0b11111111111111111111111111111111;

  EXPECT_EQ(interleave_4D_8888(x, y, z, w), expected_result);
}
