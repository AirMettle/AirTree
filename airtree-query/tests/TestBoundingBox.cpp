#include <gtest/gtest.h>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <airtree/core/utils/TrieManager.hpp>
#include <sys/types.h>
#include <vector>

using namespace airtree::query::percentile;
using namespace airtree::query::bounding_box;

class TestBoundingBox : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TestBoundingBox, TestBoundingBox2DxP_BoxQueryHasPerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  // Define the input bounding box coordinates
  double input_x_min = 10.0;
  double input_x_max = 50.0;
  double input_y_min = 10.0;
  double input_y_max = 50.0;

  // std::cout << "Input bounding box: "
  //           << "[(" << input_x_min << ", " << input_x_max << "), ("
  //           << input_y_min << ", " << input_y_max << ")]" << std::endl;

  TrieManager trie_manager;
  // Populate the histogram with values
  for (size_t x = 0; x < histogram->getBinCount(); ++x) {
    uint32_t internal_rep_x = histogram->getInternalRepresentation(x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (size_t y = 0; y < histogram->getBinCount(); ++y) {
      uint32_t internal_rep_y = histogram->getInternalRepresentation(y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  // Run the bounding box query
  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate2DResultPair result =
      bounding_box.getCounts(BoundingBoxCoordinate2D(
          input_x_min, input_x_max, input_y_min, input_y_max));

  // Capture the results
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  // std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMaxX()
  //           << "), (" << safe.getMinY() << ", " << safe.getMaxY()
  //           << ")] -> count: " << safe_count << std::endl;
  // std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMaxX()
  //           << "), (" << edge.getMinY() << ", " << edge.getMaxY()
  //           << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, 2500);
  EXPECT_EQ(edge_count, 2500);
  EXPECT_EQ(safe.getMinX(), edge.getMinX());
  EXPECT_EQ(safe.getMinY(), edge.getMinY());
  EXPECT_EQ(safe.getMaxX(), edge.getMaxX());
  EXPECT_EQ(safe.getMaxY(), edge.getMaxY());
}

TEST_F(TestBoundingBox,
       TestBoundingBox2DxP_BoxQueryDoesNotHavePerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double input_x_min = 10.125;
  double input_x_max = 49.432;
  double input_y_min = 10.134;
  double input_y_max = 49.0;

  // std::cout << "Input bounding box: "
  //           << "[(x_min: " << input_x_min << ", y_min: " << input_y_min
  //           << "), ("
  //           << "x_max: " << input_x_max << ", y_max: " << input_y_max << ")]"
  //           << std::endl;

  TrieManager trie_manager;
  for (size_t x = 0; x < histogram->getBinCount(); ++x) {
    uint32_t internal_rep_x = histogram->getInternalRepresentation(x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (size_t y = 0; y < histogram->getBinCount(); ++y) {
      uint32_t internal_rep_y = histogram->getInternalRepresentation(y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate2DResultPair result =
      bounding_box.getCounts(BoundingBoxCoordinate2D(
          input_x_min, input_x_max, input_y_min, input_y_max));
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  // std::cout << "Safe box: [(x_min: " << safe.getMinX()
  //           << ", y_min: " << safe.getMinY() << "), (x_max: " <<
  //           safe.getMaxX()
  //           << ", y_max: " << safe.getMaxY() << ")] -> count: " << safe_count
  //           << std::endl;
  // std::cout << "Edge box: [(x_min: " << edge.getMinX()
  //           << ", y_min: " << edge.getMinY() << "), (x_max: " <<
  //           edge.getMaxX()
  //           << ", y_max: " << edge.getMaxY() << ")] -> count: " << edge_count
  //           << std::endl;

  EXPECT_EQ(safe_count, 2304);
  EXPECT_EQ(edge_count, 2500);
  // verify the safe bounding box
  EXPECT_EQ(safe.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_min)));
  EXPECT_EQ(safe.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_min)));
  EXPECT_EQ(safe.getMaxX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_max) - 1));
  EXPECT_EQ(safe.getMaxY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_max) - 1));
  // verify the edge bounding box
  EXPECT_EQ(edge.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_min) - 1));
  EXPECT_EQ(edge.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_min) - 1));
  EXPECT_EQ(edge.getMaxX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_max)));
  EXPECT_EQ(edge.getMaxY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_max)));
}

// This test checks if the bounding box query works correctly when the subset
// region is populated with data points that are not perfectly aligned with the
// histogram bins. It ensures the safe/edge boxes returned are shrunk to just
// the populated region.
TEST_F(TestBoundingBox, TestBoundingBox2DxP_SubsetRegionIsPopulated) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double populate_region_x_min = 20.0;
  double populate_region_x_max = 40.0;
  double populate_region_y_min = 20.0;
  double populate_region_y_max = 40.0;

  // std::cout << "Populating region: "
  //           << "[(" << populate_region_x_min << ", " << populate_region_x_max
  //           << "), (" << populate_region_y_min << ", " <<
  //           populate_region_y_max
  //           << ")]" << std::endl;

  double query_x_min = 10.125;
  double query_x_max = 49.432;
  double query_y_min = 10.134;
  double query_y_max = 49.0;

  // std::cout << "Querying region: "
  //           << "[(" << query_x_min << ", " << query_y_min << "), ("
  //           << query_x_max << ", " << query_y_max << ")]" << std::endl;

  TrieManager trie_manager;
  for (double x = populate_region_x_min; x <= populate_region_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (double y = populate_region_y_min; y <= populate_region_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate2DResultPair result =
      bounding_box.getCounts(BoundingBoxCoordinate2D(
          query_x_min, query_x_max, query_y_min, query_y_max));
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  // std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMaxX()
  //           << "), (" << safe.getMinY() << ", " << safe.getMaxY()
  //           << ")] -> count: " << safe_count << std::endl;
  // std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMaxX()
  //           << "), (" << edge.getMinY() << ", " << edge.getMaxY()
  //           << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, 441);
  EXPECT_EQ(edge_count, 441);
  // verify the safe bounding box
  EXPECT_EQ(safe.getMinX(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_x_min)));
  EXPECT_EQ(safe.getMinY(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_y_min)));
  EXPECT_EQ(safe.getMaxX(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_x_max)));
  EXPECT_EQ(safe.getMaxY(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_y_max)));
  // verify the edge bounding box
  EXPECT_EQ(edge.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_x_min)
                                   - 1));
  EXPECT_EQ(edge.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_y_min)
                                   - 1));
  EXPECT_EQ(edge.getMaxX(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_x_max)
                                   + 1));
  EXPECT_EQ(edge.getMaxY(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_y_max)
                                   + 1));
}

TEST_F(TestBoundingBox, TestBoundingBox2DxP_BoxQueryHasExtremeInputs) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  // Define the input bounding box coordinates
  double input_x_min = -std::numeric_limits<double>::infinity();
  double input_x_max = std::numeric_limits<double>::infinity();
  double input_y_min = -std::numeric_limits<double>::infinity();
  double input_y_max = std::numeric_limits<double>::infinity();

  // std::cout << "Input bounding box: "
  //           << "[(" << input_x_min << ", " << input_x_max << "), ("
  //           << input_y_min << ", " << input_y_max << ")]" << std::endl;

  TrieManager trie_manager;
  // Populate the histogram with values
  for (size_t x = 0; x < histogram->getBinCount(); ++x) {
    uint32_t internal_rep_x = histogram->getInternalRepresentation(x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (size_t y = 0; y < histogram->getBinCount(); ++y) {
      uint32_t internal_rep_y = histogram->getInternalRepresentation(y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }
  uint64_t tle_count = 0;
  auto &trie2DxP = trie_manager.getRoot2DxP();
  for (size_t tle_idx = 0; tle_idx < BINS_64; ++tle_idx) {
    tle_count += trie2DxP.counts[tle_idx];
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  // Run the bounding box query
  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate2DResultPair result =
      bounding_box.getCounts(BoundingBoxCoordinate2D(
          input_x_min, input_x_max, input_y_min, input_y_max));

  // Capture the results
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMaxX()
            << "), (" << safe.getMinY() << ", " << safe.getMaxY()
            << ")] -> count: " << safe_count << std::endl;
  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMaxX()
            << "), (" << edge.getMinY() << ", " << edge.getMaxY()
            << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, tle_count);
  EXPECT_EQ(edge_count, tle_count);
  EXPECT_EQ(safe.getMinX(), edge.getMinX());
  EXPECT_EQ(safe.getMinY(), edge.getMinY());
  EXPECT_EQ(safe.getMaxX(), edge.getMaxX());
  EXPECT_EQ(safe.getMaxY(), edge.getMaxY());
}

// Unbounded bounding box query test
TEST_F(TestBoundingBox,
       TestBoundingBox2DxP_UnboundedBoxQueryHasPerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  // Define the input bounding box coordinates
  double input_x_min = 10.0;
  double input_y_min = 10.0;

  // std::cout << "Input bounding box: "
  //           << "[(" << input_x_min << ", " << input_y_min << ")"
  //           << "]" << std::endl;

  TrieManager trie_manager;
  // Populate the entire histogram with values
  for (size_t x = 0; x < histogram->getBinCount(); ++x) {
    uint32_t internal_rep_x = histogram->getInternalRepresentation(x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (size_t y = 0; y < histogram->getBinCount(); ++y) {
      uint32_t internal_rep_y = histogram->getInternalRepresentation(y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  // Run the bounding box query
  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      input_x_min, std::numeric_limits<double>::infinity(), input_y_min,
      std::numeric_limits<double>::infinity());
  BoxCoordinate2DResultPair result = bounding_box.getCountsUnbounded(query_box);

  // Capture the results
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  // std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
  //           << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
  //           << ")] -> count: " << safe_count << std::endl;
  // std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
  //           << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
  //           << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, 287296);
  EXPECT_EQ(edge_count, 0);
  EXPECT_EQ(safe.getMinX(), 10.0);
  EXPECT_EQ(safe.getMinY(), 10.0);
  EXPECT_EQ(safe.getMaxX(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(safe.getMaxY(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(safe.getMinX(), edge.getMinX());
  EXPECT_EQ(safe.getMinY(), edge.getMinY());
  EXPECT_EQ(safe.getMaxX(), edge.getMaxX());
  EXPECT_EQ(safe.getMaxY(), edge.getMaxY());
}

TEST_F(TestBoundingBox,
       TestBoundingBox2DxP_UnboundedBoxQueryDoesNotHavePerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double input_x_min = 10.125;
  double input_x_max = 49.432;
  double input_y_min = 10.134;
  double input_y_max = 49.0;

  // std::cout << "Input bounding box: "
  //           << "[(" << input_x_min << ", " << input_x_max << "), ("
  //           << input_y_min << ", " << input_y_max << ")]" << std::endl;

  TrieManager trie_manager;
  for (size_t x = 0; x < histogram->getBinCount(); ++x) {
    uint32_t internal_rep_x = histogram->getInternalRepresentation(x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (size_t y = 0; y < histogram->getBinCount(); ++y) {
      uint32_t internal_rep_y = histogram->getInternalRepresentation(y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      input_x_min, std::numeric_limits<double>::infinity(), input_y_min,
      std::numeric_limits<double>::infinity());
  BoxCoordinate2DResultPair result = bounding_box.getCountsUnbounded(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  // std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
  //           << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
  //           << ")] -> count: " << safe_count << std::endl;
  // std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
  //           << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
  //           << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, 286225);
  EXPECT_EQ(edge_count, 1);
  // verify the safe bounding box
  EXPECT_EQ(safe.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_min)));
  EXPECT_EQ(safe.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_min)));
  EXPECT_EQ(safe.getMaxX(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(safe.getMaxY(), std::numeric_limits<double>::infinity());
  // verify the edge bounding box
  EXPECT_EQ(edge.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(input_x_min) - 1));
  EXPECT_EQ(edge.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(input_y_min) - 1));
  EXPECT_EQ(edge.getMaxX(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(edge.getMaxY(), std::numeric_limits<double>::infinity());
}

// This test checks if the bounding box query works correctly when the subset
// region is populated with data points that are not perfectly aligned with the
// histogram bins. It ensures the safe/edge boxes returned are shrunk to just
// the populated region.
TEST_F(TestBoundingBox, TestBoundingBox2DxP_UnboundedSubsetRegionIsPopulated) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double populate_region_x_min = 20.0;
  double populate_region_x_max = 40.0;
  double populate_region_y_min = 20.0;
  double populate_region_y_max = 40.0;

  // std::cout << "Populating region: "
  //           << "[(" << populate_region_x_min << ", " << populate_region_x_max
  //           << "), (" << populate_region_y_min << ", " <<
  //           populate_region_y_max
  //           << ")]" << std::endl;

  double query_x_min = 10.125;
  double query_x_max = std::numeric_limits<double>::infinity();
  double query_y_min = 10.134;
  double query_y_max = std::numeric_limits<double>::infinity();

  // std::cout << "Querying region: "
  //           << "[(" << query_x_min << ", " << query_x_max << "), ("
  //           << query_y_min << ", " << query_y_max << ")]" << std::endl;

  TrieManager trie_manager;
  // Populate a subset of the histogram with values
  for (double x = populate_region_x_min; x <= populate_region_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (double y = populate_region_y_min; y <= populate_region_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      query_x_min, std::numeric_limits<double>::infinity(), query_y_min,
      std::numeric_limits<double>::infinity());
  BoxCoordinate2DResultPair result = bounding_box.getCountsUnbounded(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  // std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
  //           << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
  //           << ")] -> count: " << safe_count << std::endl;
  // std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
  //           << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
  //           << ")] -> count: " << edge_count << std::endl;

  EXPECT_EQ(safe_count, 441);
  EXPECT_EQ(edge_count, 0);
  // verify the safe bounding box
  EXPECT_EQ(safe.getMinX(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_x_min)));
  EXPECT_EQ(safe.getMinY(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_y_min)));
  EXPECT_EQ(safe.getMaxX(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(safe.getMaxY(), std::numeric_limits<double>::infinity());
  // verify the edge bounding box
  EXPECT_EQ(edge.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_x_min)
                                   - 1));
  EXPECT_EQ(edge.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(populate_region_y_min)
                                   - 1));
  EXPECT_EQ(edge.getMaxX(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(edge.getMaxY(), std::numeric_limits<double>::infinity());
}


std::vector<char> readFileToVector(const std::string &filename) {
  std::ifstream file(
      filename, std::ios::binary | std::ios::ate); // open at end to get size
  if (!file) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  std::streamsize size = file.tellg(); // get size
  file.seekg(0, std::ios::beg);        // go back to beginning

  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
    throw std::runtime_error("Failed to read file: " + filename);
  }

  return buffer;
}


TEST_F(TestBoundingBox, DISABLED_TestBoundingBox2DxP_UseSerializedBuffer) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double query_x_min = 291;
  double query_x_max = std::numeric_limits<double>::infinity();
  double query_y_min = 1.01563;
  double query_y_max = std::numeric_limits<double>::infinity();

  std::vector<char> serialized_trie = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-bug/"
      "temp_data/histogram_data_TSLA_price_2d_3.bin");

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      query_x_min, std::numeric_limits<double>::infinity(), query_y_min,
      std::numeric_limits<double>::infinity());
  BoxCoordinate2DResultPair result = bounding_box.getCountsUnbounded(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
            << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
            << ")] -> count: " << safe_count << std::endl;
  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
            << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
            << ")] -> count: " << edge_count << std::endl;
}


TEST_F(TestBoundingBox, DISABLED_TestBoundingBox2DxP_Simulation_1) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  std::vector<char> price_1D = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_price_1.bin");
  auto percentile_price = Percentile(price_1D);
  double query_x_min = percentile_price.getPercentile(1);
  double query_x_max = percentile_price.getPercentile(99);

  std::vector<char> volume_1D = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_volume_2.bin");
  auto percentile_volume = Percentile(volume_1D);
  double query_y_min = percentile_volume.getPercentile(1);
  double query_y_max = percentile_volume.getPercentile(99);

  std::cout << "Querying region: [(query_x_min: " << query_x_min
            << ", query_x_max: " << query_x_max
            << "), (query_y_min: " << query_y_min
            << ", query_y_max: " << query_y_max << ")]" << std::endl;

  std::vector<char> serialized_trie = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_price_2d_3.bin");

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      query_x_min, query_x_max, query_y_min, query_y_max);
  BoxCoordinate2DResultPair result = bounding_box.getCounts(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
            << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
            << ")] -> count: " << safe_count << std::endl;
  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
            << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
            << ")] -> count: " << edge_count << std::endl;
}

TEST_F(TestBoundingBox, DISABLED_TestBoundingBox2DxP_Simulation_2) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  std::vector<char> price_1D = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_price_1.bin");
  auto percentile_price = Percentile(price_1D);
  double query_x_min = percentile_price.getPercentile(45);
  double query_x_max = percentile_price.getPercentile(55);

  std::vector<char> volume_1D = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_volume_2.bin");
  auto percentile_volume = Percentile(volume_1D);
  double query_y_min = percentile_volume.getPercentile(45);
  double query_y_max = percentile_volume.getPercentile(55);

  std::cout << "Querying region: [(query_x_min: " << query_x_min
            << ", query_x_max: " << query_x_max
            << "), (query_y_min: " << query_y_min
            << ", query_y_max: " << query_y_max << ")]" << std::endl;

  std::vector<char> serialized_trie = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_price_2d_3.bin");

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      query_x_min, query_x_max, query_y_min, query_y_max);
  BoxCoordinate2DResultPair result = bounding_box.getCounts(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
            << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
            << ")] -> count: " << safe_count << std::endl;
  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
            << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
            << ")] -> count: " << edge_count << std::endl;
}

TEST_F(TestBoundingBox,
       DISABLED_TestBoundingBox2DxP_Simulation_3_Extreme_Mode) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  // std::vector<char> price_1D = readFileToVector(
  //     "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
  //     "export/data/histogram_data_TSLA_price_1.bin");
  // auto percentile_price = Percentile(price_1D);
  // double query_x_min = percentile_price.getPercentile(45);
  // double query_x_max = percentile_price.getPercentile(55);

  // std::vector<char> volume_1D = readFileToVector(
  //     "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
  //     "export/data/histogram_data_TSLA_volume_2.bin");
  // auto percentile_volume = Percentile(volume_1D);
  // double query_y_min = percentile_volume.getPercentile(45);
  // double query_y_max = percentile_volume.getPercentile(55);
  double input_x_min = -std::numeric_limits<double>::infinity();
  double input_x_max = std::numeric_limits<double>::infinity();
  double input_y_min = -std::numeric_limits<double>::infinity();
  double input_y_max = std::numeric_limits<double>::infinity();

  std::cout << "Querying region: [(query_x_min: " << input_x_min
            << ", query_x_max: " << input_x_max
            << "), (query_y_min: " << input_y_min
            << ", query_y_max: " << input_y_max << ")]" << std::endl;

  std::vector<char> serialized_trie = readFileToVector(
      "/home/rjairaj/workspace/AirMettle/Floating-Point-Histogram-2d-fix/build/"
      "export/data/histogram_data_TSLA_price_2d_3.bin");

  auto hdr_ = airtree::core::common::deserializeHeader(serialized_trie);
  std::size_t offset_ = hdr_.header_length;
  auto root = deserialize_2DxP(serialized_trie, offset_);
  uint64_t tle_count = 0;
  for (size_t tle_idx = 0; tle_idx < BINS_64; ++tle_idx) {
    tle_count += root->counts[tle_idx];
  }

  BoundingBox bounding_box(serialized_trie);
  BoundingBoxCoordinate2D query_box(
      input_x_min, input_x_max, input_y_min, input_y_max);
  BoxCoordinate2DResultPair result = bounding_box.getCounts(query_box);
  BoxCoordinate2DResult safe_box = result.first;
  BoxCoordinate2DResult edge_box = result.second;
  BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);
  EXPECT_EQ(safe_count, tle_count);
  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
            << "), (" << safe.getMaxX() << ", " << safe.getMaxY()
            << ")] -> count: " << safe_count << std::endl;
  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
            << "), (" << edge.getMaxX() << ", " << edge.getMaxY()
            << ")] -> count: " << edge_count << std::endl;
}

TEST_F(TestBoundingBox, TestBoundingBox2DxP_BatchQuery) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double populate_region_x_min = 20.0;
  double populate_region_x_max = 40.0;
  double populate_region_y_min = 20.0;
  double populate_region_y_max = 40.0;

  double query_x_min = 10.125;
  double query_x_max = 49.432;
  double query_y_min = 10.134;
  double query_y_max = 49.0;

  // Populate the same data as in the original test
  // TestBoundingBox2DxP_SubsetRegionIsPopulated
  TrieManager trie_manager;
  for (double x = populate_region_x_min; x <= populate_region_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (double y = populate_region_y_min; y <= populate_region_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);

  // Create batch queries - using the same query for sanity
  BatchBoundingBoxInput batch_queries = {
      // Original query TestBoundingBox2DxP_SubsetRegionIsPopulated
      BoundingBoxCoordinate2D(
          query_x_min, query_x_max, query_y_min, query_y_max),

      // Smaller query that should encompass the same data
      BoundingBoxCoordinate2D(15.0, 45.0, 15.0, 45.0),

      // Exact query that matches populated region
      BoundingBoxCoordinate2D(populate_region_x_min, populate_region_x_max,
                              populate_region_y_min, populate_region_y_max),

      // Query that doesn't overlap with populated region
      BoundingBoxCoordinate2D(50.0, 60.0, 50.0, 60.0),

      // Query that partially overlaps
      BoundingBoxCoordinate2D(35.0, 55.0, 35.0, 55.0)};

  // Execute batch query
  BatchBoundingBoxResult batch_results =
      bounding_box.getCountsBatch(batch_queries);

  EXPECT_EQ(batch_results.size(), 5);

  // Test Query 1: Original query TestBoundingBox2DxP_SubsetRegionIsPopulated
  {
    BoxCoordinate2DResult safe_box = batch_results[0].first;
    BoxCoordinate2DResult edge_box = batch_results[0].second;
    BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
    uint32_t safe_count = std::get<1>(safe_box);
    BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);

    // verify the safe bounding box
    EXPECT_EQ(
        safe.getMinX(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_x_min)));
    EXPECT_EQ(
        safe.getMinY(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_y_min)));
    EXPECT_EQ(
        safe.getMaxX(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_x_max)));
    EXPECT_EQ(
        safe.getMaxY(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_y_max)));

    // verify the edge bounding box
    EXPECT_EQ(
        edge.getMinX(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_x_min) - 1));
    EXPECT_EQ(
        edge.getMinY(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_y_min) - 1));
    EXPECT_EQ(
        edge.getMaxX(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_x_max) + 1));
    EXPECT_EQ(
        edge.getMaxY(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_y_max) + 1));
  }

  // Test Query 2: Smaller query that encompasses all data
  {
    BoxCoordinate2DResult safe_box = batch_results[1].first;
    BoxCoordinate2DResult edge_box = batch_results[1].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);
  }

  // Test Query 3: Exact match query
  {
    BoxCoordinate2DResult safe_box = batch_results[2].first;
    BoxCoordinate2DResult edge_box = batch_results[2].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);
  }

  // Test Query 4: Non-overlapping query
  {
    BoxCoordinate2DResult safe_box = batch_results[3].first;
    BoxCoordinate2DResult edge_box = batch_results[3].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 0);
    EXPECT_EQ(edge_count, 0);
  }

  // Test Query 5: Partially overlapping query
  {
    BoxCoordinate2DResult safe_box = batch_results[4].first;
    BoxCoordinate2DResult edge_box = batch_results[4].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    // Should have partial overlap - the populated region is [20,40] x [20,40]
    // Query is [35,55] x [35,55], so overlap is [35,40] x [35,40] = 6x6 = 36
    // points
    EXPECT_EQ(safe_count, 36);
    EXPECT_EQ(edge_count, 64);
  }

  // Verify that batch results match individual queries
  for (size_t i = 0; i < batch_queries.size(); ++i) {
    BoxCoordinate2DResultPair individual_result =
        bounding_box.getCounts(batch_queries[i]);

    // Compare safe counts
    EXPECT_EQ(std::get<1>(batch_results[i].first),
              std::get<1>(individual_result.first))
        << "Mismatch in safe count for query " << i;

    // Compare edge counts
    EXPECT_EQ(std::get<1>(batch_results[i].second),
              std::get<1>(individual_result.second))
        << "Mismatch in edge count for query " << i;
  }
}

// ============================================================================
// 3D Bounding Box Tests
// ============================================================================

TEST_F(TestBoundingBox, TestBoundingBox3DxP_BoxQueryHasPerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  // Define the input bounding box coordinates
  double input_x_min = 10.0;
  double input_x_max = 50.0;
  double input_y_min = 10.0;
  double input_y_max = 50.0;
  double input_z_min = 10.0;
  double input_z_max = 50.0;

  TrieManager trie_manager;
  // Populate only the region we're querying (not the entire histogram!)
  for (double x = input_x_min; x <= input_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF;
    for (double y = input_y_min; y <= input_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t y_10 = internal_rep_y & 0x3FF;
      for (double z = input_z_min; z <= input_z_max; ++z) {
        uint32_t bin_idx_z = histogram->getBinIndex(z);
        uint32_t internal_rep_z =
            histogram->getInternalRepresentation(bin_idx_z);
        uint32_t z_tle = getTLEEncoding((internal_rep_z >> 10) & 0x3);
        uint32_t z_10 = internal_rep_z & 0x3FF;
        uint32_t combined_tle = (x_tle << 6) | (y_tle << 3) | z_tle;
        uint64_t internal_rep = combine_chunks_10b(x_10, y_10, z_10);
        trie_manager.insert3DxP(combined_tle, internal_rep, 1);
      }
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader3D(6);
  trie_manager.serializeTrie<TLE_3D_3x10>(serialized_trie);

  // Run the bounding box query
  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate3DResultPair result = bounding_box.getCounts(
      BoundingBoxCoordinate3D(input_x_min, input_x_max, input_y_min,
                              input_y_max, input_z_min, input_z_max));

  // Capture the results
  BoxCoordinate3DResult safe_box = result.first;
  BoxCoordinate3DResult edge_box = result.second;
  BoundingBoxCoordinate3D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate3D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  EXPECT_EQ(safe_count, 68921); // 41 * 41 * 41
  EXPECT_EQ(edge_count, 68921);
  EXPECT_EQ(safe.getMinX(), edge.getMinX());
  EXPECT_EQ(safe.getMinY(), edge.getMinY());
  EXPECT_EQ(safe.getMinZ(), edge.getMinZ());
  EXPECT_EQ(safe.getMaxX(), edge.getMaxX());
  EXPECT_EQ(safe.getMaxY(), edge.getMaxY());
  EXPECT_EQ(safe.getMaxZ(), edge.getMaxZ());
}

TEST_F(TestBoundingBox,
       TestBoundingBox3DxP_BoxQueryDoesNotHavePerfectAlignment) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double input_x_min = 10.125;
  double input_x_max = 49.432;
  double input_y_min = 10.134;
  double input_y_max = 49.0;
  double input_z_min = 10.234;
  double input_z_max = 48.9;

  // Populate a reasonable region around the query box
  double populate_x_min = 5.0;
  double populate_x_max = 55.0;
  double populate_y_min = 5.0;
  double populate_y_max = 55.0;
  double populate_z_min = 5.0;
  double populate_z_max = 55.0;

  TrieManager trie_manager;
  for (double x = populate_x_min; x <= populate_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF;
    for (double y = populate_y_min; y <= populate_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t y_10 = internal_rep_y & 0x3FF;
      for (double z = populate_z_min; z <= populate_z_max; ++z) {
        uint32_t bin_idx_z = histogram->getBinIndex(z);
        uint32_t internal_rep_z =
            histogram->getInternalRepresentation(bin_idx_z);
        uint32_t z_tle = getTLEEncoding((internal_rep_z >> 10) & 0x3);
        uint32_t z_10 = internal_rep_z & 0x3FF;
        uint32_t combined_tle = (x_tle << 6) | (y_tle << 3) | z_tle;
        uint64_t internal_rep = combine_chunks_10b(x_10, y_10, z_10);
        trie_manager.insert3DxP(combined_tle, internal_rep, 1);
      }
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader3D(6);
  trie_manager.serializeTrie<TLE_3D_3x10>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate3DResultPair result = bounding_box.getCounts(
      BoundingBoxCoordinate3D(input_x_min, input_x_max, input_y_min,
                              input_y_max, input_z_min, input_z_max));
  BoxCoordinate3DResult safe_box = result.first;
  BoxCoordinate3DResult edge_box = result.second;
  BoundingBoxCoordinate3D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate3D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  // The safe box reflects data within the QUERY RANGE (not the entire populated
  // region) Since we populated at integer values (5, 6, 7, ..., 11, ..., 49,
  // ..., 55), the first bin >= 10.125 is at 11.0, and last bin <= 49.432 is
  // at 49.0
  EXPECT_GE(edge_count, safe_count);

  EXPECT_EQ(safe_count, 54872);
  EXPECT_EQ(edge_count, 64000);

  // The safe box should contain the actual populated data within the query
  // range We populated at integers, so first bin in query range is at 11.0

  // Input values for reference

  std::cout << "Input box: [(" << input_x_min << ", " << input_y_min << ", "
            << input_z_min << "), (" << input_x_max << ", " << input_y_max
            << ", " << input_z_max << ")]" << std::endl;

  std::cout << "Safe count: " << safe_count << ", Edge count: " << edge_count
            << std::endl;
  std::cout << "Safe box: [(" << safe.getMinX() << ", " << safe.getMinY()
            << ", " << safe.getMinZ() << "), (" << safe.getMaxX() << ", "
            << safe.getMaxY() << ", " << safe.getMaxZ() << ")]" << std::endl;


  std::cout << "Edge box: [(" << edge.getMinX() << ", " << edge.getMinY()
            << ", " << edge.getMinZ() << "), (" << edge.getMaxX() << ", "
            << edge.getMaxY() << ", " << edge.getMaxZ() << ")]" << std::endl;

  //
  std::cout << "Histogram bin edges for reference:" << std::endl;

  std::cout << "X bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_x_min));
  // print for y
  std::cout << "Y bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_y_min));
  // print for z
  std::cout << "Z bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_z_min));

  // now for x max values
  std::cout << "X max bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_x_max) - 1);
  // print for y max
  std::cout << "Y max bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_y_max) - 1);
  // print for z max
  std::cout << "Z max bins:" << std::endl;
  std::cout << histogram->getFPNumber(histogram->getBinIndex(input_z_max) - 1);


  // Since we populated sparsely at integers, the safe box shrinks to actual
  // data First populated values within query range
  double first_populated_x = 11.0;
  double first_populated_y = 11.0;
  double first_populated_z = 11.0;
  // Last populated values within query range
  double last_populated_x = 48.0;
  double last_populated_y = 48.0;
  double last_populated_z = 48.0;

  EXPECT_EQ(safe.getMinX(),
            histogram->getFPNumber(histogram->getBinIndex(first_populated_x)));
  EXPECT_EQ(safe.getMinY(),
            histogram->getFPNumber(histogram->getBinIndex(first_populated_y)));
  EXPECT_EQ(safe.getMinZ(),
            histogram->getFPNumber(histogram->getBinIndex(first_populated_z)));
  EXPECT_EQ(safe.getMaxX(),
            histogram->getFPNumber(histogram->getBinIndex(last_populated_x)));
  EXPECT_EQ(safe.getMaxY(),
            histogram->getFPNumber(histogram->getBinIndex(last_populated_y)));
  EXPECT_EQ(safe.getMaxZ(),
            histogram->getFPNumber(histogram->getBinIndex(last_populated_z)));

  // Verify edge box expands by 1 bin from safe box
  uint32_t safe_min_x_bin =
      static_cast<uint32_t>(histogram->getBinIndex(first_populated_x));
  uint32_t safe_min_y_bin =
      static_cast<uint32_t>(histogram->getBinIndex(first_populated_y));
  uint32_t safe_min_z_bin =
      static_cast<uint32_t>(histogram->getBinIndex(first_populated_z));
  uint32_t safe_max_x_bin =
      static_cast<uint32_t>(histogram->getBinIndex(last_populated_x));
  uint32_t safe_max_y_bin =
      static_cast<uint32_t>(histogram->getBinIndex(last_populated_y));
  uint32_t safe_max_z_bin =
      static_cast<uint32_t>(histogram->getBinIndex(last_populated_z));

  EXPECT_EQ(edge.getMinX(), histogram->getFPNumber(
                                safe_min_x_bin > 0 ? safe_min_x_bin - 1 : 0));
  EXPECT_EQ(edge.getMinY(), histogram->getFPNumber(
                                safe_min_y_bin > 0 ? safe_min_y_bin - 1 : 0));
  EXPECT_EQ(edge.getMinZ(), histogram->getFPNumber(
                                safe_min_z_bin > 0 ? safe_min_z_bin - 1 : 0));
  EXPECT_EQ(edge.getMaxX(), histogram->getFPNumber(safe_max_x_bin + 1));
  EXPECT_EQ(edge.getMaxY(), histogram->getFPNumber(safe_max_y_bin + 1));
  EXPECT_EQ(edge.getMaxZ(), histogram->getFPNumber(safe_max_z_bin + 1));
}

TEST_F(TestBoundingBox, TestBoundingBox3DxP_SubsetRegionIsPopulated) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double populate_region_x_min = 20.0;
  double populate_region_x_max = 40.0;
  double populate_region_y_min = 20.0;
  double populate_region_y_max = 40.0;
  double populate_region_z_min = 20.0;
  double populate_region_z_max = 40.0;

  double query_x_min = 10.125;
  double query_x_max = 49.432;
  double query_y_min = 10.134;
  double query_y_max = 49.0;
  double query_z_min = 10.5;
  double query_z_max = 48.9;

  TrieManager trie_manager;
  for (double x = populate_region_x_min; x <= populate_region_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF;
    for (double y = populate_region_y_min; y <= populate_region_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t y_10 = internal_rep_y & 0x3FF;
      for (double z = populate_region_z_min; z <= populate_region_z_max; ++z) {
        uint32_t bin_idx_z = histogram->getBinIndex(z);
        uint32_t internal_rep_z =
            histogram->getInternalRepresentation(bin_idx_z);
        uint32_t z_tle = getTLEEncoding((internal_rep_z >> 10) & 0x3);
        uint32_t z_10 = internal_rep_z & 0x3FF;
        uint32_t combined_tle = (x_tle << 6) | (y_tle << 3) | z_tle;
        uint64_t internal_rep = combine_chunks_10b(x_10, y_10, z_10);
        trie_manager.insert3DxP(combined_tle, internal_rep, 1);
      }
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader3D(6);
  trie_manager.serializeTrie<TLE_3D_3x10>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);
  BoxCoordinate3DResultPair result = bounding_box.getCounts(
      BoundingBoxCoordinate3D(query_x_min, query_x_max, query_y_min,
                              query_y_max, query_z_min, query_z_max));
  BoxCoordinate3DResult safe_box = result.first;
  BoxCoordinate3DResult edge_box = result.second;
  BoundingBoxCoordinate3D safe = std::get<0>(safe_box);
  uint32_t safe_count = std::get<1>(safe_box);
  BoundingBoxCoordinate3D edge = std::get<0>(edge_box);
  uint32_t edge_count = std::get<1>(edge_box);

  EXPECT_EQ(safe_count, 9261); // 21 * 21 * 21
  EXPECT_EQ(edge_count, 9261);
  // verify the safe bounding box
  EXPECT_EQ(safe.getMinX(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_x_min)));
  EXPECT_EQ(safe.getMinY(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_y_min)));
  EXPECT_EQ(safe.getMinZ(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_z_min)));
  EXPECT_EQ(safe.getMaxX(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_x_max)));
  EXPECT_EQ(safe.getMaxY(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_y_max)));
  EXPECT_EQ(safe.getMaxZ(), histogram->getFPNumber(
                                histogram->getBinIndex(populate_region_z_max)));

  // verify the edge bounding box - it expands by 1 bin from the safe box
  uint32_t safe_min_x_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_x_min));
  uint32_t safe_min_y_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_y_min));
  uint32_t safe_min_z_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_z_min));
  uint32_t safe_max_x_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_x_max));
  uint32_t safe_max_y_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_y_max));
  uint32_t safe_max_z_bin =
      static_cast<uint32_t>(histogram->getBinIndex(populate_region_z_max));

  EXPECT_EQ(edge.getMinX(), histogram->getFPNumber(
                                safe_min_x_bin > 0 ? safe_min_x_bin - 1 : 0));
  EXPECT_EQ(edge.getMinY(), histogram->getFPNumber(
                                safe_min_y_bin > 0 ? safe_min_y_bin - 1 : 0));
  EXPECT_EQ(edge.getMinZ(), histogram->getFPNumber(
                                safe_min_z_bin > 0 ? safe_min_z_bin - 1 : 0));
  EXPECT_EQ(edge.getMaxX(), histogram->getFPNumber(safe_max_x_bin + 1));
  EXPECT_EQ(edge.getMaxY(), histogram->getFPNumber(safe_max_y_bin + 1));
  EXPECT_EQ(edge.getMaxZ(), histogram->getFPNumber(safe_max_z_bin + 1));
}

TEST_F(TestBoundingBox, TestBoundingBox2DxP_BatchQuerySimple) {

  auto histogram = std::make_shared<airtree::query::meta::Histogram>(12);

  double populate_region_x_min = 20.0;
  double populate_region_x_max = 40.0;
  double populate_region_y_min = 20.0;
  double populate_region_y_max = 40.0;

  double query_x_min = 10.125;
  double query_x_max = 49.432;
  double query_y_min = 10.134;
  double query_y_max = 49.0;

  // Populate the same data as in the original test
  // TestBoundingBox2DxP_SubsetRegionIsPopulated
  TrieManager trie_manager;
  for (double x = populate_region_x_min; x <= populate_region_x_max; ++x) {
    uint32_t bin_idx_x = histogram->getBinIndex(x);
    uint32_t internal_rep_x = histogram->getInternalRepresentation(bin_idx_x);
    uint32_t x_tle = getTLEEncoding((internal_rep_x >> 10) & 0x3);
    uint32_t x_10 = internal_rep_x & 0x3FF; // Get the last 10 bits
    for (double y = populate_region_y_min; y <= populate_region_y_max; ++y) {
      uint32_t bin_idx_y = histogram->getBinIndex(y);
      uint32_t internal_rep_y = histogram->getInternalRepresentation(bin_idx_y);
      uint32_t y_tle = getTLEEncoding((internal_rep_y >> 10) & 0x3);
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = internal_rep_y & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);
      trie_manager.insert2DxP(combined_tle, internal_rep, 1);
    }
  }

  std::vector<char> serialized_trie = trie_manager.MockTrieHeader2D(6);
  trie_manager.serializeTrie<TLEoption3_2D>(serialized_trie);

  BoundingBox bounding_box(serialized_trie);

  // Create batch queries - using the same query for sanity
  BatchBoundingBoxInput batch_queries = {
      // Original query TestBoundingBox2DxP_SubsetRegionIsPopulated
      BoundingBoxCoordinate2D(
          query_x_min, query_x_max, query_y_min, query_y_max),

      // Smaller query that should encompass the same data
      BoundingBoxCoordinate2D(15.0, 45.0, 15.0, 45.0),

      // Exact query that matches populated region
      BoundingBoxCoordinate2D(populate_region_x_min, populate_region_x_max,
                              populate_region_y_min, populate_region_y_max),

      // Query that doesn't overlap with populated region
      BoundingBoxCoordinate2D(50.0, 60.0, 50.0, 60.0),

      // Query that partially overlaps
      BoundingBoxCoordinate2D(35.0, 55.0, 35.0, 55.0)};

  // Execute batch query
  BatchBoundingBoxResult batch_results =
      bounding_box.getCountsBatchSimple(batch_queries);

  EXPECT_EQ(batch_results.size(), 5);

  // Test Query 1: Original query TestBoundingBox2DxP_SubsetRegionIsPopulated
  {
    BoxCoordinate2DResult safe_box = batch_results[0].first;
    BoxCoordinate2DResult edge_box = batch_results[0].second;
    BoundingBoxCoordinate2D safe = std::get<0>(safe_box);
    uint32_t safe_count = std::get<1>(safe_box);
    BoundingBoxCoordinate2D edge = std::get<0>(edge_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);

    // verify the safe bounding box
    EXPECT_EQ(
        safe.getMinX(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_x_min)));
    EXPECT_EQ(
        safe.getMinY(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_y_min)));
    EXPECT_EQ(
        safe.getMaxX(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_x_max)));
    EXPECT_EQ(
        safe.getMaxY(),
        histogram->getFPNumber(histogram->getBinIndex(populate_region_y_max)));

    // verify the edge bounding box
    EXPECT_EQ(
        edge.getMinX(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_x_min) - 1));
    EXPECT_EQ(
        edge.getMinY(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_y_min) - 1));
    EXPECT_EQ(
        edge.getMaxX(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_x_max) + 1));
    EXPECT_EQ(
        edge.getMaxY(), histogram->getFPNumber(
                            histogram->getBinIndex(populate_region_y_max) + 1));
  }

  // Test Query 2: Smaller query that encompasses all data
  {
    BoxCoordinate2DResult safe_box = batch_results[1].first;
    BoxCoordinate2DResult edge_box = batch_results[1].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);
  }

  // Test Query 3: Exact match query
  {
    BoxCoordinate2DResult safe_box = batch_results[2].first;
    BoxCoordinate2DResult edge_box = batch_results[2].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 441);
    EXPECT_EQ(edge_count, 441);
  }

  // Test Query 4: Non-overlapping query
  {
    BoxCoordinate2DResult safe_box = batch_results[3].first;
    BoxCoordinate2DResult edge_box = batch_results[3].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    EXPECT_EQ(safe_count, 0);
    EXPECT_EQ(edge_count, 0);
  }

  // Test Query 5: Partially overlapping query
  {
    BoxCoordinate2DResult safe_box = batch_results[4].first;
    BoxCoordinate2DResult edge_box = batch_results[4].second;
    uint32_t safe_count = std::get<1>(safe_box);
    uint32_t edge_count = std::get<1>(edge_box);

    // Should have partial overlap - the populated region is [20,40] x [20,40]
    // Query is [35,55] x [35,55], so overlap is [35,40] x [35,40] = 6x6 = 36
    // points
    EXPECT_EQ(safe_count, 36);
    EXPECT_EQ(edge_count, 64);
  }

  // Verify that batch results match individual queries
  for (size_t i = 0; i < batch_queries.size(); ++i) {
    BoxCoordinate2DResultPair individual_result =
        bounding_box.getCounts(batch_queries[i]);

    // Compare safe counts
    EXPECT_EQ(std::get<1>(batch_results[i].first),
              std::get<1>(individual_result.first))
        << "Mismatch in safe count for query " << i;

    // Compare edge counts
    EXPECT_EQ(std::get<1>(batch_results[i].second),
              std::get<1>(individual_result.second))
        << "Mismatch in edge count for query " << i;
  }
}
