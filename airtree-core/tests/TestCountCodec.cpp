// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/core/common/BooleanArray.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <random>
#include <vector>

namespace {

// The in-place decoder must agree with the vector-returning one on counts and on the byte it stops at.
TEST(CountCodec, InPlaceDecodeMatchesReferenceAndStopsAtPayloadEnd) {
  std::mt19937_64 rng(20260825);
  for (size_t bins : {32u, 64u, 256u, 1024u}) {
    for (int round = 0; round < 200; ++round) {
      const double density = std::uniform_real_distribution<double>(0.0, 1.0)(rng);
      const int width = std::uniform_int_distribution<int>(1, 32)(rng);
      const uint32_t maxCount = width == 32 ? UINT32_MAX : (1u << width) - 1;
      std::vector<uint32_t> counts(bins, 0);
      BooleanArray populated(static_cast<int>((bins + 63) / 64));
      for (size_t i = 0; i < bins; ++i) {
        if (std::bernoulli_distribution(density)(rng)) {
          counts[i] = std::uniform_int_distribution<uint32_t>(1, maxCount)(rng);
          populated.set(static_cast<int>(i), true);
        }
      }
      std::vector<char> buffer = serializeCompactBooleanArray(populated);
      auto packed = serializeCounts(counts.data(), bins);
      buffer.insert(buffer.end(), packed.begin(), packed.end());
      buffer.insert(buffer.end(), 16, static_cast<char>(0xA5)); // next node's bytes

      size_t refOffset = 0;
      auto refWords = deserializeCompactBooleanArray(buffer, refOffset, (bins + 63) / 64);
      BooleanArray refMask(refWords);
      auto refCounts = deserializeCounts(buffer, refOffset, refMask.count());
      std::vector<uint32_t> expected(bins, 0);
      for (size_t i = 0, k = 0; i < bins; ++i)
        if (refMask.get(static_cast<int>(i))) expected[i] = refCounts[k++];

      size_t offset = 0;
      uint64_t mask[16];
      std::vector<uint32_t> actual(bins, 0);
      ASSERT_TRUE(readPopulatedMask(buffer, offset, mask, bins));
      ASSERT_TRUE(deserializeCounts(buffer, offset, mask, bins, actual.data()));
      EXPECT_EQ(actual, counts) << "bins=" << bins << " width=" << width;
      EXPECT_EQ(actual, expected);
      EXPECT_EQ(offset, refOffset) << "bins=" << bins << " width=" << width;
    }
  }
}

TEST(CountCodec, RejectsTruncatedPayloadAndOutOfRangeBits) {
  uint32_t counts[64] = {0};
  counts[3] = 7;
  counts[63] = 1000;
  BooleanArray populated(1);
  populated.set(3, true);
  populated.set(63, true);
  std::vector<char> buffer = serializeCompactBooleanArray(populated);
  auto packed = serializeCounts(counts, 64);
  buffer.insert(buffer.end(), packed.begin(), packed.end());

  size_t offset = 0;
  uint64_t mask[1];
  uint32_t out[64] = {0};
  ASSERT_TRUE(readPopulatedMask(buffer, offset, mask, 64));
  std::vector<char> truncated(buffer.begin(), buffer.end() - 1);
  size_t off2 = offset;
  EXPECT_FALSE(deserializeCounts(truncated, off2, mask, 64, out));
  EXPECT_FALSE(deserializeCounts(buffer, off2 = offset, mask, 32, out)); // bit 63 outside a 32-bin node
  EXPECT_TRUE(deserializeCounts(buffer, off2 = offset, mask, 64, out));
  EXPECT_EQ(out[3], 7u);
  EXPECT_EQ(out[63], 1000u);
  EXPECT_EQ(off2, buffer.size());
}

} // namespace
