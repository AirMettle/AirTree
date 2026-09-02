// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <algorithm>
#include <airtree/core/common/BitInterleave.hpp>
#include <airtree/core/Logger.hpp>
#ifdef _MSC_VER
#include <intrin.h>
#endif

using namespace airtree::core;


unsigned int interleave_2D_10bit(unsigned int x, unsigned int y) {
  if (x > 0x3FF || y > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 10 bits.");
    return 0;
  }

  unsigned int result = 0;
  for (int i = 0; i < 10; ++i) {
    unsigned int bit_x = (x >> (9 - i)) & 1;
    unsigned int bit_y = (y >> (9 - i)) & 1;

    result |= (bit_x << (19 - 2 * i));
    result |= (bit_y << (18 - 2 * i));
  }

  return result;
}

unsigned int interleave_2D(unsigned int x, unsigned int y) {
  if (x > 0xFF || y > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: Input values exceed 8 bits.");
    return 0;
  }

  unsigned int result = 0;
  for (int i = 0; i < 8; ++i) {

    unsigned int bit_x = (x >> (7 - i)) & 1;
    unsigned int bit_y = (y >> (7 - i)) & 1;

    result |= (bit_x << (15 - 2 * i));
    result |= (bit_y << (14 - 2 * i));
  }

  return result;
}

unsigned int createInternalAndInterleave_2D(uint64_t fpNumber1,
                                            uint64_t fpNumber2) {
  uint64_t exponent1 = (fpNumber1 >> 52) & 0x7FF;
  uint64_t exponent2 = (fpNumber2 >> 52) & 0x7FF;

  bool placed_signedExponentBit1 = ((exponent1 < 1023) ? 1 : 0);
  bool placed_signedExponentBit2 = ((exponent2 < 1023) ? 1 : 0);

  uint64_t adjustedExponent1 =
      placed_signedExponentBit1 ? (1023 - exponent1) : (exponent1 - 1023);
  uint64_t adjustedExponent2 =
      placed_signedExponentBit2 ? (1023 - exponent2) : (exponent2 - 1023);

  if (placed_signedExponentBit1) {
    adjustedExponent1 = ((adjustedExponent1 - 1) & 0x7FF);
  }
  if (placed_signedExponentBit2) {
    adjustedExponent2 = ((adjustedExponent2 - 1) & 0x7FF);
  }

  // Update fpNumber with the adjusted exponent
  uint64_t maskedNumber1 =
      (fpNumber1 & 0x000FFFFFFFFFFFFF) | (adjustedExponent1 << 52);
  uint64_t maskedNumber2 =
      (fpNumber2 & 0x000FFFFFFFFFFFFF) | (adjustedExponent2 << 52);

  // Calculate the position of the first significant bit using assembly or
  // intrinsic
  uint64_t firstOnePos1;
#ifdef _MSC_VER
  unsigned long index1;
  _BitScanReverse64(&index1, maskedNumber1);
  firstOnePos1 = index1;
#elif defined(x86_ARCH)
  __asm__("bsrq %1, %0" : "=r"(firstOnePos1) : "r"(maskedNumber1) : "cc");
#elif defined(ARM_ARCH)
  __asm__("clz %0, %1" : "=r"(firstOnePos1) : "r"(maskedNumber1) : "cc");
  firstOnePos1 = 63 - firstOnePos1;
#endif
  int zeroCount1 = std::min(63 - static_cast<int>(firstOnePos1) - 2, 15);

  uint64_t precisionBits1 = ((maskedNumber1 >> (firstOnePos1 - 6)) & 0x3F);

  uint64_t firstOnePos2;
#ifdef _MSC_VER
  unsigned long index2;
  _BitScanReverse64(&index2, maskedNumber2);
  firstOnePos2 = index2;
#elif defined(x86_ARCH)
  __asm__("bsrq %1, %0" : "=r"(firstOnePos2) : "r"(maskedNumber2) : "cc");
#elif defined(ARM_ARCH)
  __asm__("clz %0, %1" : "=r"(firstOnePos2) : "r"(maskedNumber2) : "cc");
  firstOnePos2 = 63 - firstOnePos2;
#endif
  int zeroCount2 = std::min(63 - static_cast<int>(firstOnePos2) - 2, 15);

  uint64_t precisionBits2 = ((maskedNumber2 >> (firstOnePos2 - 6)) & 0x3F);

  unsigned int interleavedM = 0;
  unsigned int interleavedP = 0;

  for (int i = 0; i < 4; ++i) {
    interleavedM |= (((zeroCount1 >> (2 - i)) & 1) << (2 * (2 - i) + 1))
                    | (((zeroCount2 >> (2 - i)) & 1) << (2 * (2 - i)));
  }

  // Correctly interleave P parts (6 bits each)
  for (int i = 0; i < 6; ++i) {
    interleavedP |= (((precisionBits1 >> (4 - i)) & 1) << (2 * (4 - i) + 1))
                    | (((precisionBits2 >> (4 - i)) & 1) << (2 * (4 - i)));
  }

  // Combine interleaved M and P parts
  unsigned int result = (interleavedM << 12) | interleavedP;

  // std::cout<< "Result: " << std::bitset<16>(result) << std::endl;

  return result;
}


unsigned int interleave_3D_888(unsigned int x, unsigned int y, unsigned int z) {
  if (x > 0xFF || y > 0xFF || z > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Input values exceed 8 bits.");
    return 0;
  }

  unsigned int result = 0;
  for (int i = 0; i < 8; ++i) {
    unsigned int bit_x = (x >> (7 - i)) & 1;
    unsigned int bit_y = (y >> (7 - i)) & 1;
    unsigned int bit_z = (z >> (7 - i)) & 1;

    // Interleave the bits in the result
    result |= (bit_x << (23 - 3 * i));
    result |= (bit_y << (22 - 3 * i));
    result |= (bit_z << (21 - 3 * i));
  }

  return result;
}


unsigned int interleavebits_2D(unsigned int x, unsigned int y) {
  // Validate inputs
  if (x > 0x3FF || y > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Input values exceed 10 bits.");
    return 0;
  }

  // std::cout << "X: " << std::bitset<8>(x) << " Y: " << std::bitset<8>(y)
  //           << std::endl;

  // Extract M and P parts
  unsigned int M0 = (x >> 6) & 0x1F; // Get the top 4 bits of x
  unsigned int P0 = x & 0x3F;        // Get the bottom 6 bits of x
  unsigned int M1 = (y >> 6) & 0x1F; // Get the top 4 bits of y
  unsigned int P1 = y & 0x3F;        // Get the bottom 6 bits of y

  unsigned int interleavedM = 0;
  unsigned int interleavedP = 0;

  // Correctly interleave M parts (4 bits each)
  for (int i = 0; i < 4; ++i) {
    interleavedM |= (((M0 >> (2 - i)) & 1) << (2 * (2 - i) + 1))
                    | (((M1 >> (2 - i)) & 1) << (2 * (2 - i)));
  }

  // Correctly interleave P parts (6 bits each)
  for (int i = 0; i < 6; ++i) {
    interleavedP |= (((P0 >> (4 - i)) & 1) << (2 * (4 - i) + 1))
                    | (((P1 >> (4 - i)) & 1) << (2 * (4 - i)));
  }

  // Combine interleaved M and P parts
  unsigned int result = (interleavedM << 12) | interleavedP;

  return result;
}

unsigned int interleavebits_3D(unsigned int x, unsigned int y, unsigned int z) {
  // Validate inputs
  if (x > 0x3FF || y > 0x3FF || z > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Input values exceed 10 bits.");
    return 0;
  }

  // Extract M and P parts
  unsigned int M0 = (x >> 6) & 0x1F; // Get the top 4 bits of x
  unsigned int P0 = x & 0x3F;        // Get the bottom 6 bits of x
  unsigned int M1 = (y >> 6) & 0x1F; // Get the top 4 bits of y
  unsigned int P1 = y & 0x3F;        // Get the bottom 6 bits of y
  unsigned int M2 = (z >> 6) & 0x1F; // Get the top 4 bits of z
  unsigned int P2 = z & 0x3F;        // Get the bottom 6 bits of z

  // std::cout << "M0: " << std::bitset<4>(M0) << std::endl;
  // std::cout << "P0: " << std::bitset<6>(P0) << std::endl;
  // std::cout << "M1: " << std::bitset<4>(M1) << std::endl;
  // std::cout << "P1: " << std::bitset<6>(P1) << std::endl;
  // std::cout << "M2: " << std::bitset<4>(M2) << std::endl;
  // std::cout << "P2: " << std::bitset<6>(P2) << std::endl;

  unsigned int interleavedM = 0;
  unsigned int interleavedP = 0;

  interleavedM |= (((M0 >> 3) & 1) << 11) | (((M1 >> 3) & 1) << 10)
                  | (((M2 >> 3) & 1) << 9); // a0b0c0
  interleavedM |= (((M0 >> 2) & 1) << 8) | (((M1 >> 2) & 1) << 7)
                  | (((M2 >> 2) & 1) << 6); // a1b1c1
  interleavedM |= (((M0 >> 1) & 1) << 5) | (((M1 >> 1) & 1) << 4)
                  | (((M2 >> 1) & 1) << 3); // a2b2c2
  interleavedM |= (((M0 >> 0) & 1) << 2) | (((M1 >> 0) & 1) << 1)
                  | (((M2 >> 0) & 1) << 0); // a3b3c3

  // for (int i = 0; i < 4; ++i) {
  //   interleavedM |= (((M0 >> (2 - i)) & 1) << (2 * (2 - i) + 1)) | (((M1 >>
  //   (2 - i)) & 1) << (2 * (2 - i))) | (((M2 >> (2 - i)) & 1) << (2 * (2 - i)
  //   - 1));
  // }

  // for (int i = 0; i < 3; ++i) {
  //   auto bitM0 = (M0 >> i) & 1;
  //   auto bitM1 = (M1 >> i) & 1;
  //   auto bitM2 = (M2 >> i) & 1;
  //   interleavedM |= (bitM2 << (3 * i)) | (bitM1 << (3 * i + 1)) | (bitM0 <<
  //   (3 * i + 2));
  // }

  interleavedP |= (((P0 >> 5) & 1) << 17) | (((P1 >> 5) & 1) << 16)
                  | (((P2 >> 5) & 1) << 15); // a0b0c0
  interleavedP |= (((P0 >> 4) & 1) << 14) | (((P1 >> 4) & 1) << 13)
                  | (((P2 >> 4) & 1) << 12); // a0b0c0
  interleavedP |= (((P0 >> 3) & 1) << 11) | (((P1 >> 3) & 1) << 10)
                  | (((P2 >> 3) & 1) << 9); // a1b1c1
  interleavedP |= (((P0 >> 2) & 1) << 8) | (((P1 >> 2) & 1) << 7)
                  | (((P2 >> 2) & 1) << 6); // a2b2c2
  interleavedP |= (((P0 >> 1) & 1) << 5) | (((P1 >> 1) & 1) << 4)
                  | (((P2 >> 1) & 1) << 3); // a3b3c3
  interleavedP |= (((P0 >> 0) & 1) << 2) | (((P1 >> 0) & 1) << 1)
                  | (((P2 >> 0) & 1) << 0); // a4b4c4

  // for (int i = 0; i < 6; ++i) {
  //   interleavedP |= (((P0 >> (4 - i)) & 1) << (2 * (4 - i) + 1)) | (((P1 >>
  //   (4 - i)) & 1) << (2 * (4 - i))) | (((P2 >> (4 - i)) & 1) << (2 * (4 - i)
  //   + 1));
  // }

  // for (int i = 0; i < 5; ++i) {
  //   auto bitP0 = (P0 >> i) & 1;
  //   auto bitP1 = (P1 >> i) & 1;
  //   auto bitP2 = (P2 >> i) & 1;
  //   interleavedP |= (bitP2 << (3 * i)) | (bitP1 << (3 * i + 1)) | (bitP0 <<
  //   (3 * i + 2));
  // }

  // Shift interleavedM left by 10 bits to make space for the 10 bits of
  // interleavedP

  return (interleavedM << 18) | interleavedP;
}

unsigned int interleave_4D_8888(unsigned int x, unsigned int y, unsigned int z,
                                unsigned int w) {
  if (x > 0xFF || y > 0xFF || z > 0xFF || w > 0xFF) {
    SPDLOG_LOGGER_ERROR(logger(), "Input values exceed 8 bits.");
    return 0;
  }

  unsigned int result = 0;
  for (int i = 0; i < 8; ++i) {
    // Extract the i-th bit from x, y, z, and w
    unsigned int bit_x = (x >> (7 - i)) & 1;
    unsigned int bit_y = (y >> (7 - i)) & 1;
    unsigned int bit_z = (z >> (7 - i)) & 1;
    unsigned int bit_w = (w >> (7 - i)) & 1;

    // Interleave the bits in the result
    result |= (bit_x << (31 - 4 * i)); // Place x's bit in the correct position
    result |= (bit_y << (30 - 4 * i)); // Place y's bit in the correct position
    result |= (bit_z << (29 - 4 * i)); // Place z's bit in the correct position
    result |= (bit_w << (28 - 4 * i)); // Place w's bit in the correct position
  }

  return result;
}

uint64_t interleavebits_4D(unsigned int w, unsigned int x, unsigned int y,
                           unsigned int z) {
  // Validate inputs
  if (w > 0x3FF || x > 0x3FF || y > 0x3FF || z > 0x3FF) {
    SPDLOG_LOGGER_ERROR(logger(), "Input values exceed 10 bits.");
    return 0; // Adjust based on how you want to handle errors
  }

  uint64_t interleaved = 0; // to accomodate - 40 bits

  // Manually interleave each bit for all 40 bits.
  interleaved |= ((uint64_t)(w >> 9) & 1) << 39;
  interleaved |= ((uint64_t)(x >> 9) & 1) << 38;
  interleaved |= ((uint64_t)(y >> 9) & 1) << 37;
  interleaved |= ((uint64_t)(z >> 9) & 1) << 36;

  interleaved |= ((uint64_t)(w >> 8) & 1) << 35;
  interleaved |= ((uint64_t)(x >> 8) & 1) << 34;
  interleaved |= ((uint64_t)(y >> 8) & 1) << 33;
  interleaved |= ((uint64_t)(z >> 8) & 1) << 32;

  interleaved |= ((uint64_t)(w >> 7) & 1) << 31;
  interleaved |= ((uint64_t)(x >> 7) & 1) << 30;
  interleaved |= ((uint64_t)(y >> 7) & 1) << 29;
  interleaved |= ((uint64_t)(z >> 7) & 1) << 28;

  interleaved |= ((uint64_t)(w >> 6) & 1) << 27;
  interleaved |= ((uint64_t)(x >> 6) & 1) << 26;
  interleaved |= ((uint64_t)(y >> 6) & 1) << 25;
  interleaved |= ((uint64_t)(z >> 6) & 1) << 24;

  interleaved |= ((uint64_t)(w >> 5) & 1) << 23;
  interleaved |= ((uint64_t)(x >> 5) & 1) << 22;
  interleaved |= (((uint64_t)y >> 5) & 1) << 21;
  interleaved |= ((uint64_t)(z >> 5) & 1) << 20;

  interleaved |= ((uint64_t)(w >> 4) & 1) << 19;
  interleaved |= ((uint64_t)(x >> 4) & 1) << 18;
  interleaved |= ((uint64_t)(y >> 4) & 1) << 17;
  interleaved |= ((uint64_t)(z >> 4) & 1) << 16;

  interleaved |= ((uint64_t)(w >> 3) & 1) << 15;
  interleaved |= ((uint64_t)(x >> 3) & 1) << 14;
  interleaved |= ((uint64_t)(y >> 3) & 1) << 13;
  interleaved |= ((uint64_t)(z >> 3) & 1) << 12;

  interleaved |= ((uint64_t)(w >> 2) & 1) << 11;
  interleaved |= ((uint64_t)(x >> 2) & 1) << 10;
  interleaved |= ((uint64_t)(y >> 2) & 1) << 9;
  interleaved |= ((uint64_t)(z >> 2) & 1) << 8;

  interleaved |= ((uint64_t)(w >> 1) & 1) << 7;
  interleaved |= ((uint64_t)(x >> 1) & 1) << 6;
  interleaved |= ((uint64_t)(y >> 1) & 1) << 5;
  interleaved |= ((uint64_t)(z >> 1) & 1) << 4;

  interleaved |= ((uint64_t)(w >> 0) & 1) << 3;
  interleaved |= ((uint64_t)(x >> 0) & 1) << 2;
  interleaved |= ((uint64_t)(y >> 0) & 1) << 1;
  interleaved |= ((uint64_t)(z >> 0) & 1) << 0;

  // std::cout << "10-bit values:\n"
  //           << "w: " << std::bitset<10>(w) << "\n"
  //           << "x: " << std::bitset<10>(x) << "\n"
  //           << "y: " << std::bitset<10>(y) << "\n"
  //           << "z: " << std::bitset<10>(z) << "\n";
  // std::cout << "Interleaved 40-bit: " << std::bitset<40>(interleaved) <<
  // std::endl;

  return interleaved;
}