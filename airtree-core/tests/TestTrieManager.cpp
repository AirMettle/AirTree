// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <gtest/gtest.h>
#include <airtree/core/utils/TrieManager.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>

TEST(TrieManagerTest, EXAMPLE_1DxT_SINGLE) {

  // Create a TrieManager object.
  TrieManager trieManager;

  // Insert 100 into 1DxT (13Colonies) trie at level0 index 10 using SINGLE
  // distribution, targeting Level1 bucket 3.
  trieManager.insert1DxT(10, 100, DistributionMethod::SINGLE, 3);


  // Retrieve populated buckets. The result is a vector of pairs, where each
  // pair contains the level0 index and the counts of the populated Level1
  // buckets.
  auto buckets = trieManager.getPopulatedBuckets1DxT();
  ASSERT_EQ(buckets.size(), 1u); // u here basically means unsigned int
  EXPECT_EQ(buckets[0].first, 10u);

  const std::vector<uint32_t> &level1Counts = buckets[0].second;
  ASSERT_EQ(level1Counts.size(), BINS_32);
  for (unsigned int i = 0; i < BINS_32; i++) {
    if (i == 3)
      EXPECT_EQ(level1Counts[i], 100u);
    else
      EXPECT_EQ(level1Counts[i], 0u);
  }
}

// 1DxT EVEN insertion test.
TEST(TrieManagerTest, EXAMPLE_1DxT_EVEN) {
  TrieManager trieManager;
  // Insert 65 into 1DxT trie at level0 index 20 using EVEN distribution.
  // With BINS_32 == 32, base = 65/32 = 2 and remainder = 1.
  trieManager.insert1DxT(20, 65, DistributionMethod::EVEN);

  auto buckets = trieManager.getPopulatedBuckets1DxT();
  ASSERT_EQ(buckets.size(), 1u);
  EXPECT_EQ(buckets[0].first, 20u);

  const std::vector<uint32_t> &level1Counts = buckets[0].second;
  ASSERT_EQ(level1Counts.size(), BINS_32);
  for (unsigned int i = 0; i < BINS_32; i++) {
    // The first bucket (i == 0) gets an extra 1.
    if (i < 1)
      EXPECT_EQ(level1Counts[i], 3u);
    else
      EXPECT_EQ(level1Counts[i], 2u);
  }
}

TEST(TrieManagerTest, EXAMPLE_1DxT_RANDOM) {
  // if you want to fix the randommess of the test, you can use
  // srand(some_value) before insertion , it can be reporduced

  // std::srand(13); // seed for reproducibility
  TrieManager trieManager;
  uint32_t insertedCount = 100;
  trieManager.insert1DxT(30, insertedCount, DistributionMethod::RANDOM);

  auto buckets = trieManager.getPopulatedBuckets1DxT();
  ASSERT_EQ(buckets.size(), 1u);
  EXPECT_EQ(buckets[0].first, 30u);

  const std::vector<uint32_t> &level1Counts = buckets[0].second;
  uint32_t sum = 0;
  for (uint32_t count : level1Counts)
    sum += count;
  EXPECT_EQ(sum, insertedCount);

  for (size_t i = 0; i < level1Counts.size(); i++) {
    if (level1Counts[i] != 0) {
      std::cout << "Child bin " << i << " is populated with count "
                << level1Counts[i] << std::endl;
    }
  }
}

// Test for multiple insertions in the Apollo16(1DxF) trie.
TEST(TrieManagerTest, EXAMPLE_1DxF_MULTIPLE_INSERTIONS) {
  TrieManager trieManager;
  // Insert different values at different Level0 indices.
  // Index 15: SINGLE insertion (50 into target child 2).
  trieManager.insert1DxF(15, 50, DistributionMethod::SINGLE, 2);

  // Index 25: EVEN insertion (256 count; with BINS_256=256, each
  // gets 1).
  trieManager.insert1DxF(25, 256, DistributionMethod::EVEN);

  // Index 35: RANDOM insertion (300 count; verify the total equals 300).
  trieManager.insert1DxF(35, 300, DistributionMethod::RANDOM);

  // ONCE YOU ARE DONE WITH INSERTION INTO TRIE YOU CAN CALL getRoot#Config
  // METHOD TO Retrive root and pass it along other functions we have in the
  // code

  // Mock the trie header (pass in the total bits for the config so that header
  // can be adjusted accordingly.)
  std::vector<char> headerBuffer = trieManager.MockTrieHeader(16);
  trieManager.serializeTrie<TrieNode_13>(headerBuffer);

  //   Write(headerBuffer, "1DxF_16");


  auto buckets = trieManager.getPopulatedBuckets1DxF();
  // Expect three populated Level0 buckets.
  EXPECT_EQ(buckets.size(), 3u);

  bool found15 = false, found25 = false, found35 = false;
  for (const auto &bucket : buckets) {
    if (bucket.first == 15) {
      found15 = true;
      const auto &counts = bucket.second;
      // For SINGLE, target child 2 gets 50.
      EXPECT_EQ(counts[2], 50u);
      uint32_t sum = 0;
      for (auto c : counts)
        sum += c;
      EXPECT_EQ(sum, 50u);
    }
    if (bucket.first == 25) {
      found25 = true;
      const auto &counts = bucket.second;
      uint32_t sum = 0;
      for (auto c : counts) {
        EXPECT_EQ(c, 1u);
        sum += c;
      }
      EXPECT_EQ(sum, 256u);
    }
    if (bucket.first == 35) {
      found35 = true;
      const auto &counts = bucket.second;
      uint32_t sum = 0;
      for (auto c : counts)
        sum += c;
      EXPECT_EQ(sum, 300u);
    }
  }
  EXPECT_TRUE(found15 && found25 && found35);
}

// Test for multiple insertions in the Roaring20 trie.
TEST(TrieManagerTest, Roaring20MultipleInsertion) {
  TrieManager trieManager;
  // Insert multiple values at different indices using SINGLE distribution at
  // Level2. For each call, we specify the target Level2 bucket where the count
  // should go. Insert into Level0 index 5, Level1 index 3, target Level2 index
  // 10, count = 200.
  trieManager.insert1DxP(5, 3, 200, DistributionMethod::SINGLE, 10);
  // Insert into Level0 index 7, Level1 index 2, target Level2 index 15, count =
  // 300.
  trieManager.insert1DxP(7, 2, 300, DistributionMethod::SINGLE, 15);
  // Insert another into Level0 index 5, Level1 index 4, target Level2 index 20,
  // count = 150.
  trieManager.insert1DxP(5, 4, 150, DistributionMethod::SINGLE, 20);

  auto buckets = trieManager.getPopulatedBuckets1DxP();
  // We expect two Level0 entries: one at index 5 and one at index 7.
  EXPECT_EQ(buckets.size(), 2u);

  bool found5 = false, found7 = false;
  for (const auto &bucket : buckets) {
    if (bucket.level0Index == 5) {
      found5 = true;
      // For Level0 index 5, we expect two Level1 buckets: indices 3 and 4.
      int countLevel1 = 0;
      for (const auto &level1 : bucket.level1Buckets) {
        if (level1.first == 3) {
          uint32_t sum = 0;
          for (auto c : level1.second)
            sum += c;
          EXPECT_EQ(sum, 200u);
          // Verify that only Level2 bin at index 10 is populated.
          for (size_t i = 0; i < level1.second.size(); i++) {
            if (i == 10)
              EXPECT_EQ(level1.second[i], 200u);
            else
              EXPECT_EQ(level1.second[i], 0u);
          }
          countLevel1++;
        }
        if (level1.first == 4) {
          uint32_t sum = 0;
          for (auto c : level1.second)
            sum += c;
          EXPECT_EQ(sum, 150u);
          // Verify that only Level2 bin at index 20 is populated.
          for (size_t i = 0; i < level1.second.size(); i++) {
            if (i == 20)
              EXPECT_EQ(level1.second[i], 150u);
            else
              EXPECT_EQ(level1.second[i], 0u);
          }
          countLevel1++;
        }
      }
      EXPECT_EQ(countLevel1, 2);
    }
    if (bucket.level0Index == 7) {
      found7 = true;
      // For Level0 index 7, we expect one Level1 bucket: index 2 with sum =
      // 300.
      EXPECT_EQ(bucket.level1Buckets.size(), 1u);
      const auto &level1 = bucket.level1Buckets[0];
      EXPECT_EQ(level1.first, 2u);
      uint32_t sum = 0;
      for (auto c : level1.second)
        sum += c;
      EXPECT_EQ(sum, 300u);
      // Verify that only Level2 bin at index 15 is populated.
      for (size_t i = 0; i < level1.second.size(); i++) {
        if (i == 15)
          EXPECT_EQ(level1.second[i], 300u);
        else
          EXPECT_EQ(level1.second[i], 0u);
      }
    }
  }
  EXPECT_TRUE(found5 && found7);
}

TEST(TrieManagerTest, InsertOneQuadrants) {
  TrieManager trieManager;
  // Insert a total count of 100 into quadrant POS_POS using precision 13.
  trieManager.insertByQuadrant(
      Quadrant::POS_POS, 100, DistributionMethod::EVEN, 13);

  // Retrieve the buckets from the 1DxT configuration.
  auto buckets = trieManager.getPopulatedBuckets1DxT();
  // We expect 64 buckets (0 to 63) to be populated.
  EXPECT_EQ(buckets.size(), 64u);

  uint32_t totalCount = 0;
  for (const auto &bucket : buckets) {
    // Ensure bucket indices are within POS_POS quadrant.
    EXPECT_GE(bucket.first, 0u);
    EXPECT_LT(bucket.first, 64u);

    uint32_t bucketSum = 0;
    for (auto c : bucket.second) {
      bucketSum += c;
    }
    totalCount += bucketSum;
  }
  EXPECT_EQ(totalCount, 100u);
}

TEST(TrieManagerTest, InsertTwoQuadrants) {
  TrieManager trieManager;
  // Prepare two quadrant/count pairs:
  // - POS_POS: count = 200, should affect indices 0 to 63.
  // - NEG_NEG: count = 300, should affect indices 192 to 255.
  std::vector<QuadrantRegion> regions = {
      {Quadrant::POS_POS, 200}, {Quadrant::NEG_NEG, 300}};
  trieManager.insertByQuadrant(regions, DistributionMethod::EVEN, 16);

  auto buckets = trieManager.getPopulatedBuckets1DxF();
  // Expect 64 buckets for POS_POS and 64 buckets for NEG_NEG, total 128
  // buckets.
  EXPECT_EQ(buckets.size(), 128u);

  uint32_t totalCountPosPos = 0;
  uint32_t totalCountNegNeg = 0;
  for (const auto &bucket : buckets) {
    uint32_t bucketSum = 0;
    for (auto c : bucket.second)
      bucketSum += c;
    if (bucket.first < 64) { // POS_POS quadrant
      totalCountPosPos += bucketSum;
    } else if (bucket.first >= 192 && bucket.first < 256) { // NEG_NEG quadrant
      totalCountNegNeg += bucketSum;
    } else {
      FAIL() << "Unexpected bucket index: " << bucket.first;
    }
  }
  EXPECT_EQ(totalCountPosPos, 200u);
  EXPECT_EQ(totalCountNegNeg, 300u);
}

TEST(TrieManagerTest, InsertThreeQuadrants) {
  TrieManager trieManager;
  // Prepare three quadrant/count pairs:
  // - POS_POS: count = 100 (indices 0-63)
  // - POS_NEG: count = 150 (indices 64-127)
  // - NEG_NEG: count = 250 (indices 192-255)
  std::vector<QuadrantRegion> regions = {{Quadrant::POS_POS, 100},
                                         {Quadrant::POS_NEG, 150},
                                         {Quadrant::NEG_NEG, 250}};
  trieManager.insertByQuadrant(regions, DistributionMethod::EVEN, 20);

  // Retrieve buckets from the 1DxP (Roaring20) configuration.
  auto buckets = trieManager.getPopulatedBuckets1DxP();
  // We expect 64 buckets per inserted quadrant, total 192 buckets.
  EXPECT_EQ(buckets.size(), 192u);

  uint32_t totalCountPosPos = 0;
  uint32_t totalCountPosNeg = 0;
  uint32_t totalCountNegNeg = 0;

  // For each bucket, sum the counts from all its level1 buckets.
  for (const auto &bucket : buckets) {
    uint32_t bucketSum = 0;
    for (const auto &lvl1Pair : bucket.level1Buckets) {
      // lvl1Pair.first is the Level1 index, and lvl1Pair.second is a vector of
      // Level2 counts.
      for (uint32_t c : lvl1Pair.second) {
        bucketSum += c;
      }
    }
    // Distribute sums based on the level0 index.
    if (bucket.level0Index < 64) {
      totalCountPosPos += bucketSum;
    } else if (bucket.level0Index >= 64 && bucket.level0Index < 128) {
      totalCountPosNeg += bucketSum;
    } else if (bucket.level0Index >= 192 && bucket.level0Index < 256) {
      totalCountNegNeg += bucketSum;
    }
  }
  EXPECT_EQ(totalCountPosPos, 100u);
  EXPECT_EQ(totalCountPosNeg, 150u);
  EXPECT_EQ(totalCountNegNeg, 250u);
}

TEST(TrieManagerTest, InsertOneQuadrant_LeftSkew) {
  TrieManager trieManager;
  // Insert 100 counts into quadrant POS_POS using precision 13 (1DxT)
  // with a LEFT_SKEW distribution (more weight to lower indices).
  trieManager.insertByQuadrant(Quadrant::POS_POS, 100, DistributionMethod::EVEN,
                               13, Level0Distribution::LEFT_SKEW);

  auto buckets = trieManager.getPopulatedBuckets1DxT();
  // Expect 64 buckets (indices 0 to 63) for POS_POS.
  EXPECT_EQ(buckets.size(), 64u);

  uint32_t totalCount = 0;
  uint32_t leftmostCount = 0, rightmostCount = 0;
  for (const auto &bucket : buckets) {
    // All bucket indices for POS_POS should be in [0, 63].
    EXPECT_GE(bucket.first, 0u);
    EXPECT_LT(bucket.first, 64u);

    uint32_t bucketSum = 0;
    for (auto c : bucket.second)
      bucketSum += c;
    totalCount += bucketSum;
    if (bucket.first == 0) {
      leftmostCount = bucketSum;
    }
    if (bucket.first == 63) {
      rightmostCount = bucketSum;
    }
  }
  EXPECT_EQ(totalCount, 100u);
  // For LEFT_SKEW, expect the leftmost bucket to have more counts than the
  // rightmost.
  EXPECT_GT(leftmostCount, rightmostCount);
}

TEST(TrieManagerTest, InsertOneQuadrant_RightSkew) {
  TrieManager trieManager;
  // Insert 100 counts into quadrant POS_POS using precision 13 (1DxT)
  // with a RIGHT_SKEW distribution (more weight to higher indices).
  trieManager.insertByQuadrant(Quadrant::POS_POS, 100, DistributionMethod::EVEN,
                               13, Level0Distribution::RIGHT_SKEW);

  auto buckets = trieManager.getPopulatedBuckets1DxT();
  EXPECT_EQ(buckets.size(), 64u);

  uint32_t totalCount = 0;
  uint32_t leftmostCount = 0, rightmostCount = 0;
  for (const auto &bucket : buckets) {
    EXPECT_GE(bucket.first, 0u);
    EXPECT_LT(bucket.first, 64u);

    uint32_t bucketSum = 0;
    for (auto c : bucket.second)
      bucketSum += c;
    totalCount += bucketSum;
    if (bucket.first == 0) {
      leftmostCount = bucketSum;
    }
    if (bucket.first == 63) {
      rightmostCount = bucketSum;
    }
  }
  EXPECT_EQ(totalCount, 100u);
  // For RIGHT_SKEW, expect the rightmost bucket to have more counts than the
  // leftmost.
  EXPECT_GT(rightmostCount, leftmostCount);
}

TEST(TrieManagerTest, InsertOneQuadrant_NormalSkew) {
  TrieManager trieManager;
  // Insert 100 counts into quadrant POS_POS using precision 13 (1DxT)
  // with a NORMAL_SKEW (bell curve) distribution.
  trieManager.insertByQuadrant(Quadrant::POS_POS, 100, DistributionMethod::EVEN,
                               13, Level0Distribution::NORMAL_SKEW);

  auto buckets = trieManager.getPopulatedBuckets1DxT();
  EXPECT_EQ(buckets.size(), 64u);

  uint32_t totalCount = 0;
  uint32_t midCount = 0, leftCount = 0, rightCount = 0;
  // Assume the middle bucket is around index 31.
  for (const auto &bucket : buckets) {
    EXPECT_GE(bucket.first, 0u);
    EXPECT_LT(bucket.first, 64u);

    uint32_t bucketSum = 0;
    for (auto c : bucket.second)
      bucketSum += c;
    totalCount += bucketSum;
    if (bucket.first == 0) {
      leftCount = bucketSum;
    }
    if (bucket.first == 31) {
      midCount = bucketSum;
    }
    if (bucket.first == 63) {
      rightCount = bucketSum;
    }
  }
  EXPECT_EQ(totalCount, 100u);
  // For a bell curve, expect the middle bucket to have more counts than the
  // edges.
  EXPECT_GT(midCount, leftCount);
  EXPECT_GT(midCount, rightCount);
}

TEST(TrieManagerTest, InsertAllQuadrants_DifferentStrategies) {
  TrieManager trieManager;
  // Insert into each quadrant with a different Level0 distribution strategy.
  trieManager.insertByQuadrant(Quadrant::POS_POS, 100, DistributionMethod::EVEN,
                               20, Level0Distribution::EVEN);
  trieManager.insertByQuadrant(Quadrant::POS_NEG, 150, DistributionMethod::EVEN,
                               20, Level0Distribution::LEFT_SKEW);
  trieManager.insertByQuadrant(Quadrant::NEG_POS, 200, DistributionMethod::EVEN,
                               20, Level0Distribution::RIGHT_SKEW);
  trieManager.insertByQuadrant(Quadrant::NEG_NEG, 250, DistributionMethod::EVEN,
                               20, Level0Distribution::NORMAL_SKEW);

  auto buckets = trieManager.getPopulatedBuckets1DxP();
  // We expect 256 Level0 buckets to be populated in total (64 per quadrant).
  EXPECT_EQ(buckets.size(), 256u);

  uint32_t totalPosPos = 0;
  uint32_t totalPosNeg = 0;
  uint32_t totalNegPos = 0;
  uint32_t totalNegNeg = 0;

  for (const auto &bucket : buckets) {
    uint32_t bucketSum = 0;
    // Iterate over the level1Buckets vector.
    for (const auto &lvl1Pair : bucket.level1Buckets) {
      for (uint32_t c : lvl1Pair.second) {
        bucketSum += c;
      }
    }
    if (bucket.level0Index < 64) {
      totalPosPos += bucketSum;
    } else if (bucket.level0Index >= 64 && bucket.level0Index < 128) {
      totalPosNeg += bucketSum;
    } else if (bucket.level0Index >= 128 && bucket.level0Index < 192) {
      totalNegPos += bucketSum;
    } else if (bucket.level0Index >= 192 && bucket.level0Index < 256) {
      totalNegNeg += bucketSum;
    }
  }
  EXPECT_EQ(totalPosPos, 100u);
  EXPECT_EQ(totalPosNeg, 150u);
  EXPECT_EQ(totalNegPos, 200u);
  EXPECT_EQ(totalNegNeg, 250u);
}
