// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/query/bounding-box/BoundingBox.hpp>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <unordered_map>

using namespace airtree::query::bounding_box;
using namespace airtree::core::io;

BoundingBox::BoundingBox(std::vector<char> buffer) {
  AirTreeReader reader;
  reader.read(buffer);

  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();
  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_unique<airtree::query::meta::Histogram>(bit_length_);
}

BoxCoordinate2DResultPair
BoundingBox::getCounts(BoundingBoxCoordinate2D box) const {
  if (!histogram_) {
    throw std::runtime_error("Histogram not initialized.");
  }

  if (trie_node_.valueless_by_exception()) {
    throw std::runtime_error("Trie node not initialized.");
  }

  if (dims_ == 2 && bit_length_ == 12) {
    // Handle 2D case with 12-bit length
    return calculateBoundingBox2D(box);
  } else {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
}

BoxCoordinate3DResultPair
BoundingBox::getCounts(BoundingBoxCoordinate3D box) const {
  if (!histogram_) {
    throw std::runtime_error("Histogram not initialized.");
  }

  if (trie_node_.valueless_by_exception()) {
    throw std::runtime_error("Trie node not initialized.");
  }

  if (dims_ == 3 && bit_length_ == 12) {
    // Handle 3D case with 12-bit length
    return calculateBoundingBox3D(box);
  } else {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
}

template <typename NodeType>
uint32_t getCount(const std::unique_ptr<NodeType> &node, uint64_t internal_rep,
                  uint32_t combined_tle) {
  if (!node) {
    throw std::runtime_error("Node is null.");
  }

  // TODO: should we be handling special cases for TLE?

  if constexpr (std::is_same_v<NodeType, TLEoption3_2D>) {
    uint64_t prefix_10 = (internal_rep >> 10) & 0x3FF;
    uint64_t suffix_10 = internal_rep & 0x3FF;

    // if (node->populated.test(combined_tle)) {
    //   std::cout << "Node populated for TLE: " << combined_tle
    //             << ", Internal Rep: " << internal_rep << std::endl;
    // }
    if (node->populated.test(combined_tle)
        && node->nodes[combined_tle]->populated.test(prefix_10)
        && node->nodes[combined_tle]->nodes[prefix_10]->counts[suffix_10] > 0) {
      // std::cout << "Node populated for TLE: " << combined_tle
      // << ", Internal Rep: " << internal_rep << std::endl;
      return node->nodes[combined_tle]->nodes[prefix_10]->counts[suffix_10];
    }
  } else {
    throw std::runtime_error("Unsupported node type for count retrieval.");
  }

  // Implement the logic to get the count from the node
  // std::cout << "Node not populated for TLE: " << combined_tle
  // << ", Internal Rep: " << internal_rep << std::endl;
  return 0; // Placeholder return value
}

template <typename NodeType>
uint32_t getCount3D(const std::unique_ptr<NodeType> &node,
                    uint64_t internal_rep, uint32_t combined_tle) {
  if (!node) {
    throw std::runtime_error("Node is null.");
  }

  if constexpr (std::is_same_v<NodeType, TLE_3D_3x10>) {
    // Check if TLE level is populated
    if (!node->populated.test(combined_tle)) {
      return 0;
    }

    // For ndims=0 case, only TLE level has data
    if (!node->nodes[combined_tle]) {
      // No child nodes, return TLE-level count
      return node->counts[combined_tle];
    }

    // Extract the 10-bit chunks from the 30-bit internal_rep
    // internal_rep can be 10, 20, or 30 bits depending on ndims
    // For ndims=7: 30 bits -> l0_10 (bits 29-20), l1_10 (bits 19-10), l2_10
    // (bits 9-0) For ndims=3,5,6: 20 bits -> l0_10 (bits 19-10), l1_10 (bits
    // 9-0) For ndims=1,2,4: 10 bits -> l0_10 (bits 9-0)

    uint32_t l0_10 = (internal_rep >> 20) & 0x3FF; // Top 10 bits
    uint32_t l1_10 = (internal_rep >> 10) & 0x3FF; // Middle 10 bits
    uint32_t l2_10 = internal_rep & 0x3FF;         // Bottom 10 bits

    // Determine the depth based on which nodes exist
    // Check L0 (always exists if we have child nodes)
    if (!node->nodes[combined_tle]->populated.test(l0_10)) {
      return 0;
    }

    // If no L1 node, this is ndims=1,2,4 case
    if (!node->nodes[combined_tle]->nodes[l0_10]) {
      return node->nodes[combined_tle]->counts[l0_10];
    }

    // Check L1
    if (!node->nodes[combined_tle]->nodes[l0_10]->populated.test(l1_10)) {
      return 0;
    }

    // If no L2 node, this is ndims=3,5,6 case
    if (!node->nodes[combined_tle]->nodes[l0_10]->nodes[l1_10]) {
      return node->nodes[combined_tle]->nodes[l0_10]->counts[l1_10];
    }

    // Check L2 (ndims=7 case)
    return node->nodes[combined_tle]->nodes[l0_10]->nodes[l1_10]->counts[l2_10];

  } else {
    throw std::runtime_error("Unsupported node type for 3D count retrieval.");
  }

  return 0;
}

void BoundingBox::initSafeBox(BoundingBoxBinIndices2D &safe, double in_min_x,
                              double in_min_y, double in_max_x,
                              double in_max_y) const {

  uint32_t min_x_bin = 0;
  uint32_t min_y_bin = 0;
  uint32_t max_x_bin = histogram_->getBinCount() - 1;
  uint32_t max_y_bin = histogram_->getBinCount() - 1;

  // handle special cases for unbounded box
  if (in_min_x == -std::numeric_limits<double>::infinity()) {
    safe.setMinX(min_x_bin);
  } else {
    // Initialize the safe box with the provided coordinates
    min_x_bin = histogram_->getBinIndex(in_min_x);
    safe.setMinX(min_x_bin);
  }
  if (histogram_->getFPNumber(min_x_bin) == in_min_x || min_x_bin == 0) {
    safe.xMinMatchFlag(); // Set the match flag for xMin
  }

  if (in_min_y == -std::numeric_limits<double>::infinity()) {
    safe.setMinY(min_y_bin);
  } else {
    // Initialize the safe box with the provided coordinates
    min_y_bin = histogram_->getBinIndex(in_min_y);
    safe.setMinY(min_y_bin);
  }
  if (histogram_->getFPNumber(min_y_bin) == in_min_y || min_y_bin == 0) {
    safe.yMinMatchFlag(); // Set the match flag for yMin
  }

  if (in_max_x == std::numeric_limits<double>::infinity()) {
    safe.setMaxX(max_x_bin);
  } else {
    max_x_bin = histogram_->getBinIndex(in_max_x);
    double reconstructed_max_x = histogram_->getFPNumber(max_x_bin);
    if (reconstructed_max_x > in_max_x) {
      safe.setMaxX(max_x_bin - 1);
    } else {
      safe.setMaxX(max_x_bin);
    }
  }
  if (histogram_->getFPNumber(max_x_bin) == in_max_x
      || max_x_bin == histogram_->getBinCount() - 1) {
    safe.xMaxMatchFlag(); // Set the match flag for xMax
  }

  if (in_max_y == std::numeric_limits<double>::infinity()) {
    safe.setMaxY(max_y_bin);
  } else {
    max_y_bin = histogram_->getBinIndex(in_max_y);
    double reconstructed_max_y = histogram_->getFPNumber(max_y_bin);
    if (reconstructed_max_y > in_max_y) {
      safe.setMaxY(max_y_bin - 1);
    } else {
      safe.setMaxY(max_y_bin);
    }
  }
  if (histogram_->getFPNumber(max_y_bin) == in_max_y
      || max_y_bin == histogram_->getBinCount() - 1) {
    safe.yMaxMatchFlag(); // Set the match flag for yMax
  }

  // // Ensure the safe box is valid
  // if (safe.getMinX() > safe.getMaxX() || safe.getMinY() > safe.getMaxY()) {
  //   throw std::runtime_error("Invalid bounding box coordinates.");
  // }
}

void BoundingBox::initEdgeBox(BoundingBoxBinIndices2D &edge,
                              BoundingBoxBinIndices2D &safe) const {
  // handle special cases for unbounded box
  if (safe.isXMinMatch()) {
    edge.setMinX(safe.getMinX());
  } else {
    edge.setMinX(safe.getMinX() - 1);
  }

  if (safe.isYMinMatch()) {
    edge.setMinY(safe.getMinY());
  } else {
    edge.setMinY(safe.getMinY() - 1);
  }

  if (safe.isXMaxMatch()) {
    edge.setMaxX(safe.getMaxX());
  } else {
    edge.setMaxX(safe.getMaxX() + 1);
  }

  if (safe.isYMaxMatch()) {
    edge.setMaxY(safe.getMaxY());
  } else {
    edge.setMaxY(safe.getMaxY() + 1);
  }
}

void BoundingBox::initSafeBox3D(BoundingBoxBinIndices3D &safe, double in_min_x,
                                double in_min_y, double in_min_z,
                                double in_max_x, double in_max_y,
                                double in_max_z) const {

  uint32_t min_x_bin = 0;
  uint32_t min_y_bin = 0;
  uint32_t min_z_bin = 0;
  uint32_t max_x_bin = histogram_->getBinCount() - 1;
  uint32_t max_y_bin = histogram_->getBinCount() - 1;
  uint32_t max_z_bin = histogram_->getBinCount() - 1;

  // Handle X dimension
  if (in_min_x == -std::numeric_limits<double>::infinity()) {
    safe.setMinX(min_x_bin);
  } else {
    min_x_bin = histogram_->getBinIndex(in_min_x);
    safe.setMinX(min_x_bin);
  }
  if (histogram_->getFPNumber(min_x_bin) == in_min_x || min_x_bin == 0) {
    safe.xMinMatchFlag();
  }

  if (in_max_x == std::numeric_limits<double>::infinity()) {
    safe.setMaxX(max_x_bin);
  } else {
    max_x_bin = histogram_->getBinIndex(in_max_x);
    double reconstructed_max_x = histogram_->getFPNumber(max_x_bin);
    if (reconstructed_max_x > in_max_x) {
      safe.setMaxX(max_x_bin - 1);
    } else {
      safe.setMaxX(max_x_bin);
    }
  }
  if (histogram_->getFPNumber(max_x_bin) == in_max_x
      || max_x_bin == histogram_->getBinCount() - 1) {
    safe.xMaxMatchFlag();
  }

  // Handle Y dimension
  if (in_min_y == -std::numeric_limits<double>::infinity()) {
    safe.setMinY(min_y_bin);
  } else {
    min_y_bin = histogram_->getBinIndex(in_min_y);
    safe.setMinY(min_y_bin);
  }
  if (histogram_->getFPNumber(min_y_bin) == in_min_y || min_y_bin == 0) {
    safe.yMinMatchFlag();
  }

  if (in_max_y == std::numeric_limits<double>::infinity()) {
    safe.setMaxY(max_y_bin);
  } else {
    max_y_bin = histogram_->getBinIndex(in_max_y);
    double reconstructed_max_y = histogram_->getFPNumber(max_y_bin);
    if (reconstructed_max_y > in_max_y) {
      safe.setMaxY(max_y_bin - 1);
    } else {
      safe.setMaxY(max_y_bin);
    }
  }
  if (histogram_->getFPNumber(max_y_bin) == in_max_y
      || max_y_bin == histogram_->getBinCount() - 1) {
    safe.yMaxMatchFlag();
  }

  // Handle Z dimension
  if (in_min_z == -std::numeric_limits<double>::infinity()) {
    safe.setMinZ(min_z_bin);
  } else {
    min_z_bin = histogram_->getBinIndex(in_min_z);
    safe.setMinZ(min_z_bin);
  }
  if (histogram_->getFPNumber(min_z_bin) == in_min_z || min_z_bin == 0) {
    safe.zMinMatchFlag();
  }

  if (in_max_z == std::numeric_limits<double>::infinity()) {
    safe.setMaxZ(max_z_bin);
  } else {
    max_z_bin = histogram_->getBinIndex(in_max_z);
    double reconstructed_max_z = histogram_->getFPNumber(max_z_bin);
    if (reconstructed_max_z > in_max_z) {
      safe.setMaxZ(max_z_bin - 1);
    } else {
      safe.setMaxZ(max_z_bin);
    }
  }
  if (histogram_->getFPNumber(max_z_bin) == in_max_z
      || max_z_bin == histogram_->getBinCount() - 1) {
    safe.zMaxMatchFlag();
  }
}

void BoundingBox::initEdgeBox3D(BoundingBoxBinIndices3D &edge,
                                BoundingBoxBinIndices3D &safe) const {
  // After refining the safe box to actual data, we always expand by 1 bin
  // unless we're at the histogram boundaries (0 or max)
  uint32_t max_bin = histogram_->getBinCount() - 1;

  // Handle X dimension
  if (safe.getMinX() == 0) {
    edge.setMinX(0);
  } else {
    edge.setMinX(safe.getMinX() - 1);
  }

  if (safe.getMaxX() == max_bin) {
    edge.setMaxX(max_bin);
  } else {
    edge.setMaxX(safe.getMaxX() + 1);
  }

  // Handle Y dimension
  if (safe.getMinY() == 0) {
    edge.setMinY(0);
  } else {
    edge.setMinY(safe.getMinY() - 1);
  }

  if (safe.getMaxY() == max_bin) {
    edge.setMaxY(max_bin);
  } else {
    edge.setMaxY(safe.getMaxY() + 1);
  }

  // Handle Z dimension
  if (safe.getMinZ() == 0) {
    edge.setMinZ(0);
  } else {
    edge.setMinZ(safe.getMinZ() - 1);
  }

  if (safe.getMaxZ() == max_bin) {
    edge.setMaxZ(max_bin);
  } else {
    edge.setMaxZ(safe.getMaxZ() + 1);
  }
}

BoxCoordinate2DResultPair
BoundingBox::calculateBoundingBox2D(const BoundingBoxCoordinate2D &box) const {
  if (!trie_node_.is<TLEoption3_2D>()) {
    throw std::runtime_error("Invalid trie node type for 2D bounding box.");
  }

  const auto &trie_root =
      trie_node_.get_ptr<TLEoption3_2D>(); // std::unique_ptr<TLEoption3_2D>&

  if (dims_ != 2) {
    throw std::runtime_error("Invalid dimensions for 2D bounding box.");
  }

  // Extract input min and max coordinates for each dimension
  double input_x_min = box.getMinX();
  double input_x_max = box.getMaxX();
  double input_y_min = box.getMinY();
  double input_y_max = box.getMaxY();

  BoundingBoxBinIndices2D safe_bin_coords;
  BoundingBoxBinIndices2D total_bin_coords;
  BoundingBoxCoordinate2D safe;
  BoundingBoxCoordinate2D total;
  initSafeBox(
      safe_bin_coords, input_x_min, input_y_min, input_x_max, input_y_max);
  uint32_t safe_count = 0;
  uint32_t total_count = 0;

  bool first_safe_update = true;

  safe.setMaxX(-std::numeric_limits<double>::infinity());
  safe.setMaxY(-std::numeric_limits<double>::infinity());
  for (uint32_t curr_x_bin = safe_bin_coords.getMinX();
       curr_x_bin <= safe_bin_coords.getMaxX(); ++curr_x_bin) {
    // Get the 12 bit internal representations
    uint32_t x_12 = histogram_->getInternalRepresentation(curr_x_bin);
    // capture tle
    uint32_t x_tle = getTLEEncoding((x_12 >> 10) & 0x3); // Get the TLE for x
    uint32_t x_10 = x_12 & 0x3FF;                        // Get the last 10 bits

    for (uint32_t curr_y_bin = safe_bin_coords.getMinY();
         curr_y_bin <= safe_bin_coords.getMaxY(); ++curr_y_bin) {
      // Get the 12 bit internal representations
      uint32_t y_12 = histogram_->getInternalRepresentation(curr_y_bin);
      // capture tle
      uint32_t y_tle = getTLEEncoding((y_12 >> 10) & 0x3); // Get the TLE for y
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = y_12 & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);

      uint32_t count =
          getCount<TLEoption3_2D>(trie_root, internal_rep, combined_tle);
      if (count == 0) {
        continue; // Skip if no count is found
      }

      double fp_x = histogram_->getFPNumber(curr_x_bin);
      double fp_y = histogram_->getFPNumber(curr_y_bin);

      safe_count += count;
      if (first_safe_update) {
        // If this is the first safe point, we can update the min values
        safe.setMinX(fp_x);
        safe.setMinY(fp_y);
        first_safe_update = false;
      }
      safe.setMaxX(std::max(safe.getMaxX(), fp_x));
      safe.setMaxY(std::max(safe.getMaxY(), fp_y));
      continue;
    }
  }

  if (!first_safe_update) {
    // safe was updated, so we need to set safe indices again
    safe_bin_coords.setMinX(histogram_->getBinIndex(safe.getMinX()));
    safe_bin_coords.setMinY(histogram_->getBinIndex(safe.getMinY()));
    safe_bin_coords.setMaxX(histogram_->getBinIndex(safe.getMaxX()));
    safe_bin_coords.setMaxY(histogram_->getBinIndex(safe.getMaxY()));
  }

  // Calculate total box based on refined safe box
  initEdgeBox(total_bin_coords, safe_bin_coords);
  total.setMinX(histogram_->getFPNumber(total_bin_coords.getMinX()));
  total.setMinY(histogram_->getFPNumber(total_bin_coords.getMinY()));
  total.setMaxX(histogram_->getFPNumber(total_bin_coords.getMaxX()));
  total.setMaxY(histogram_->getFPNumber(total_bin_coords.getMaxY()));

  if (safe_bin_coords.isPerfectMatch()) {
    // If the safe box is exactly the input box, we can return early
    BoxCoordinate2DResult safe_box = {safe, safe_count};
    BoxCoordinate2DResult total_box = {total, safe_count};
    return BoxCoordinate2DResultPair{safe_box, total_box};
  }

  // Now count everything in the total region (this includes safe region)
  for (uint32_t curr_x_bin = total_bin_coords.getMinX();
       curr_x_bin <= total_bin_coords.getMaxX(); ++curr_x_bin) {
    // Get the 12 bit internal representations
    uint32_t x_12 = histogram_->getInternalRepresentation(curr_x_bin);
    // capture tle
    uint32_t x_tle = getTLEEncoding((x_12 >> 10) & 0x3); // Get the TLE for x
    uint32_t x_10 = x_12 & 0x3FF;                        // Get the last 10 bits
    for (uint32_t curr_y_bin = total_bin_coords.getMinY();
         curr_y_bin <= total_bin_coords.getMaxY(); ++curr_y_bin) {
      // Get the 12 bit internal representations
      uint32_t y_12 = histogram_->getInternalRepresentation(curr_y_bin);
      // capture tle
      uint32_t y_tle = getTLEEncoding((y_12 >> 10) & 0x3); // Get the TLE for y
      uint32_t combined_tle = (x_tle << 3) | y_tle;
      uint32_t y_10 = y_12 & 0x3FF; // Get the last 10 bits
      uint64_t internal_rep = combine_chunks_10b(x_10, y_10);

      total_count +=
          getCount<TLEoption3_2D>(trie_root, internal_rep, combined_tle);
    }
  }

  auto safe_box = BoxCoordinate2DResult{safe, safe_count};
  auto total_box = BoxCoordinate2DResult{total, total_count};
  return BoxCoordinate2DResultPair{safe_box, total_box};
}

BoxCoordinate3DResultPair
BoundingBox::calculateBoundingBox3D(const BoundingBoxCoordinate3D &box) const {
  if (!trie_node_.is<TLE_3D_3x10>()) {
    throw std::runtime_error("Invalid trie node type for 3D bounding box.");
  }

  const auto &trie_root = trie_node_.get_ptr<TLE_3D_3x10>();

  if (dims_ != 3) {
    throw std::runtime_error("Invalid dimensions for 3D bounding box.");
  }

  // Extract input min and max coordinates for each dimension
  double input_x_min = box.getMinX();
  double input_x_max = box.getMaxX();
  double input_y_min = box.getMinY();
  double input_y_max = box.getMaxY();
  double input_z_min = box.getMinZ();
  double input_z_max = box.getMaxZ();

  BoundingBoxBinIndices3D safe_bin_coords;
  BoundingBoxBinIndices3D total_bin_coords;
  BoundingBoxCoordinate3D safe;
  BoundingBoxCoordinate3D total;

  initSafeBox3D(safe_bin_coords, input_x_min, input_y_min, input_z_min,
                input_x_max, input_y_max, input_z_max);

  uint32_t safe_count = 0;
  uint32_t total_count = 0;

  bool first_safe_update = true;

  safe.setMaxX(-std::numeric_limits<double>::infinity());
  safe.setMaxY(-std::numeric_limits<double>::infinity());
  safe.setMaxZ(-std::numeric_limits<double>::infinity());

  // Iterate through the safe box to count points
  for (uint32_t curr_x_bin = safe_bin_coords.getMinX();
       curr_x_bin <= safe_bin_coords.getMaxX(); ++curr_x_bin) {
    // Get the 12 bit internal representation for x
    uint32_t x_12 = histogram_->getInternalRepresentation(curr_x_bin);
    uint32_t x_tle = getTLEEncoding((x_12 >> 10) & 0x3);
    uint32_t x_10 = x_12 & 0x3FF;

    for (uint32_t curr_y_bin = safe_bin_coords.getMinY();
         curr_y_bin <= safe_bin_coords.getMaxY(); ++curr_y_bin) {
      // Get the 12 bit internal representation for y
      uint32_t y_12 = histogram_->getInternalRepresentation(curr_y_bin);
      uint32_t y_tle = getTLEEncoding((y_12 >> 10) & 0x3);
      uint32_t y_10 = y_12 & 0x3FF;

      for (uint32_t curr_z_bin = safe_bin_coords.getMinZ();
           curr_z_bin <= safe_bin_coords.getMaxZ(); ++curr_z_bin) {
        // Get the 12 bit internal representation for z
        uint32_t z_12 = histogram_->getInternalRepresentation(curr_z_bin);
        uint32_t z_tle = getTLEEncoding((z_12 >> 10) & 0x3);
        uint32_t z_10 = z_12 & 0x3FF;

        // Combine TLE values: 9 bits total (3 bits each)
        uint32_t combined_tle = (x_tle << 6) | (y_tle << 3) | z_tle;

        // Combine internal representations: 30 bits total (10 bits each)
        uint64_t internal_rep = combine_chunks_10b(x_10, y_10, z_10);

        uint32_t count =
            getCount3D<TLE_3D_3x10>(trie_root, internal_rep, combined_tle);

        if (count == 0) {
          continue; // Skip if no count is found
        }

        double fp_x = histogram_->getFPNumber(curr_x_bin);
        double fp_y = histogram_->getFPNumber(curr_y_bin);
        double fp_z = histogram_->getFPNumber(curr_z_bin);

        safe_count += count;
        if (first_safe_update) {
          // If this is the first safe point, we can update the min values
          safe.setMinX(fp_x);
          safe.setMinY(fp_y);
          safe.setMinZ(fp_z);
          first_safe_update = false;
        }
        safe.setMaxX(std::max(safe.getMaxX(), fp_x));
        safe.setMaxY(std::max(safe.getMaxY(), fp_y));
        safe.setMaxZ(std::max(safe.getMaxZ(), fp_z));
      }
    }
  }

  if (!first_safe_update) {
    // safe was updated, so we need to set safe indices again
    safe_bin_coords.setMinX(histogram_->getBinIndex(safe.getMinX()));
    safe_bin_coords.setMinY(histogram_->getBinIndex(safe.getMinY()));
    safe_bin_coords.setMinZ(histogram_->getBinIndex(safe.getMinZ()));
    safe_bin_coords.setMaxX(histogram_->getBinIndex(safe.getMaxX()));
    safe_bin_coords.setMaxY(histogram_->getBinIndex(safe.getMaxY()));
    safe_bin_coords.setMaxZ(histogram_->getBinIndex(safe.getMaxZ()));
  }

  // Check if this is a perfect match before calculating edge box
  if (safe_bin_coords.isPerfectMatch()) {
    // If the safe box perfectly matches the input box, edge box == safe box
    total.setMinX(safe.getMinX());
    total.setMinY(safe.getMinY());
    total.setMinZ(safe.getMinZ());
    total.setMaxX(safe.getMaxX());
    total.setMaxY(safe.getMaxY());
    total.setMaxZ(safe.getMaxZ());

    BoxCoordinate3DResult safe_box = {safe, safe_count};
    BoxCoordinate3DResult total_box = {total, safe_count};
    return BoxCoordinate3DResultPair{safe_box, total_box};
  }

  // Calculate total box based on refined safe box
  initEdgeBox3D(total_bin_coords, safe_bin_coords);
  total.setMinX(histogram_->getFPNumber(total_bin_coords.getMinX()));
  total.setMinY(histogram_->getFPNumber(total_bin_coords.getMinY()));
  total.setMinZ(histogram_->getFPNumber(total_bin_coords.getMinZ()));
  total.setMaxX(histogram_->getFPNumber(total_bin_coords.getMaxX()));
  total.setMaxY(histogram_->getFPNumber(total_bin_coords.getMaxY()));
  total.setMaxZ(histogram_->getFPNumber(total_bin_coords.getMaxZ()));

  // Now count everything in the total region (this includes safe region)
  for (uint32_t curr_x_bin = total_bin_coords.getMinX();
       curr_x_bin <= total_bin_coords.getMaxX(); ++curr_x_bin) {
    uint32_t x_12 = histogram_->getInternalRepresentation(curr_x_bin);
    uint32_t x_tle = getTLEEncoding((x_12 >> 10) & 0x3);
    uint32_t x_10 = x_12 & 0x3FF;

    for (uint32_t curr_y_bin = total_bin_coords.getMinY();
         curr_y_bin <= total_bin_coords.getMaxY(); ++curr_y_bin) {
      uint32_t y_12 = histogram_->getInternalRepresentation(curr_y_bin);
      uint32_t y_tle = getTLEEncoding((y_12 >> 10) & 0x3);
      uint32_t y_10 = y_12 & 0x3FF;

      for (uint32_t curr_z_bin = total_bin_coords.getMinZ();
           curr_z_bin <= total_bin_coords.getMaxZ(); ++curr_z_bin) {
        uint32_t z_12 = histogram_->getInternalRepresentation(curr_z_bin);
        uint32_t z_tle = getTLEEncoding((z_12 >> 10) & 0x3);
        uint32_t z_10 = z_12 & 0x3FF;

        uint32_t combined_tle = (x_tle << 6) | (y_tle << 3) | z_tle;
        uint64_t internal_rep = combine_chunks_10b(x_10, y_10, z_10);

        total_count +=
            getCount3D<TLE_3D_3x10>(trie_root, internal_rep, combined_tle);
      }
    }
  }

  auto safe_box = BoxCoordinate3DResult{safe, safe_count};
  auto total_box = BoxCoordinate3DResult{total, total_count};
  return BoxCoordinate3DResultPair{safe_box, total_box};
}

BatchBoundingBoxResult
BoundingBox::getCountsBatchSimple(const BatchBoundingBoxInput &queries) const {
  BatchBoundingBoxResult results;
  results.reserve(queries.size());

  // Simple implementation
  for (const auto &query : queries) {
    results.push_back(calculateBoundingBox2D(query));
  }

  return results;
}

BatchBoundingBoxResult
BoundingBox::getCountsBatch(const BatchBoundingBoxInput &queries) const {
  if (!histogram_) {
    throw std::runtime_error("Histogram not initialized.");
  }

  if (queries.empty()) {
    return BatchBoundingBoxResult{};
  }

  if (dims_ != 2 || bit_length_ != 12) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }

  if (!trie_node_.is<TLEoption3_2D>()) {
    throw std::runtime_error("Invalid trie node type for 2D bounding box.");
  }

  const auto &trie_root = trie_node_.get_ptr<TLEoption3_2D>();
  BatchBoundingBoxResult results;
  results.reserve(queries.size());

  // Cache for computed bin counts to avoid redundant trie traversals
  std::unordered_map<uint64_t, uint32_t> global_bin_cache;

  // Helper lambda to get count with caching
  auto getCachedCount = [&](uint32_t x_bin, uint32_t y_bin) -> uint32_t {
    uint64_t bin_key = (static_cast<uint64_t>(x_bin) << 32) | y_bin;

    auto it = global_bin_cache.find(bin_key);
    if (it != global_bin_cache.end()) {
      return it->second;
    }

    // Compute the count
    uint32_t x_12 = histogram_->getInternalRepresentation(x_bin);
    uint32_t x_tle = getTLEEncoding((x_12 >> 10) & 0x3);
    uint32_t x_10 = x_12 & 0x3FF;
    uint32_t y_12 = histogram_->getInternalRepresentation(y_bin);
    uint32_t y_tle = getTLEEncoding((y_12 >> 10) & 0x3);
    uint32_t combined_tle = (x_tle << 3) | y_tle;
    uint32_t y_10 = y_12 & 0x3FF;
    uint64_t internal_rep = combine_chunks_10b(x_10, y_10);

    uint32_t count =
        getCount<TLEoption3_2D>(trie_root, internal_rep, combined_tle);
    global_bin_cache[bin_key] = count;
    return count;
  };

  // Process each query
  for (const auto &query : queries) {
    double input_x_min = query.getMinX();
    double input_x_max = query.getMaxX();
    double input_y_min = query.getMinY();
    double input_y_max = query.getMaxY();

    BoundingBoxBinIndices2D safe_bin_coords;
    BoundingBoxCoordinate2D safe;
    BoundingBoxCoordinate2D total;

    initSafeBox(
        safe_bin_coords, input_x_min, input_y_min, input_x_max, input_y_max);

    uint32_t safe_count = 0;
    uint32_t total_count = 0;
    bool first_safe_update = true;

    uint32_t actual_min_x_bin = UINT32_MAX;
    uint32_t actual_max_x_bin = 0;
    uint32_t actual_min_y_bin = UINT32_MAX;
    uint32_t actual_max_y_bin = 0;

    safe.setMaxX(-std::numeric_limits<double>::infinity());
    safe.setMaxY(-std::numeric_limits<double>::infinity());

    // Build safe box using cached counts
    for (uint32_t curr_x_bin = safe_bin_coords.getMinX();
         curr_x_bin <= safe_bin_coords.getMaxX(); ++curr_x_bin) {
      for (uint32_t curr_y_bin = safe_bin_coords.getMinY();
           curr_y_bin <= safe_bin_coords.getMaxY(); ++curr_y_bin) {

        uint32_t count = getCachedCount(curr_x_bin, curr_y_bin);

        if (count == 0) {
          continue;
        }

        double fp_x = histogram_->getFPNumber(curr_x_bin);
        double fp_y = histogram_->getFPNumber(curr_y_bin);

        safe_count += count;

        actual_min_x_bin = std::min(actual_min_x_bin, curr_x_bin);
        actual_max_x_bin = std::max(actual_max_x_bin, curr_x_bin);
        actual_min_y_bin = std::min(actual_min_y_bin, curr_y_bin);
        actual_max_y_bin = std::max(actual_max_y_bin, curr_y_bin);

        if (first_safe_update) {
          safe.setMinX(fp_x);
          safe.setMinY(fp_y);
          first_safe_update = false;
        }
        safe.setMaxX(std::max(safe.getMaxX(), fp_x));
        safe.setMaxY(std::max(safe.getMaxY(), fp_y));
      }
    }

    if (first_safe_update) {
      // No data found
      BoundingBoxCoordinate2D empty_box;
      auto safe_box = BoxCoordinate2DResult{empty_box, 0};
      auto total_box = BoxCoordinate2DResult{empty_box, 0};
      results.push_back(BoxCoordinate2DResultPair{safe_box, total_box});
      continue;
    }

    // Calculate total box and count
    uint32_t total_min_x = (actual_min_x_bin > 0) ? actual_min_x_bin - 1 : 0;
    uint32_t total_max_x =
        std::min(actual_max_x_bin + 1,
                 static_cast<uint32_t>(histogram_->getBinCount() - 1));
    uint32_t total_min_y = (actual_min_y_bin > 0) ? actual_min_y_bin - 1 : 0;
    uint32_t total_max_y =
        std::min(actual_max_y_bin + 1,
                 static_cast<uint32_t>(histogram_->getBinCount() - 1));

    total.setMinX(histogram_->getFPNumber(total_min_x));
    total.setMinY(histogram_->getFPNumber(total_min_y));
    total.setMaxX(histogram_->getFPNumber(total_max_x));
    total.setMaxY(histogram_->getFPNumber(total_max_y));

    total_count = safe_count;

    // Add edge bin counts
    for (uint32_t curr_x_bin = total_min_x; curr_x_bin <= total_max_x;
         ++curr_x_bin) {
      for (uint32_t curr_y_bin = total_min_y; curr_y_bin <= total_max_y;
           ++curr_y_bin) {
        if (curr_x_bin >= actual_min_x_bin && curr_x_bin <= actual_max_x_bin
            && curr_y_bin >= actual_min_y_bin
            && curr_y_bin <= actual_max_y_bin) {
          continue;
        }
        total_count += getCachedCount(curr_x_bin, curr_y_bin);
      }
    }

    auto safe_box = BoxCoordinate2DResult{safe, safe_count};
    auto total_box = BoxCoordinate2DResult{total, total_count};
    results.push_back(BoxCoordinate2DResultPair{safe_box, total_box});
  }

  return results;
}

BoundingBoxCoordinate2D::BoundingBoxCoordinate2D(double min_x, double max_x,
                                                 double min_y, double max_y)
    : min_x_(min_x), max_x_(max_x), min_y_(min_y), max_y_(max_y) {}

double BoundingBoxCoordinate2D::getMinX() const {
  return min_x_;
}

double BoundingBoxCoordinate2D::getMaxX() const {
  return max_x_;
}

double BoundingBoxCoordinate2D::getMinY() const {
  return min_y_;
}

double BoundingBoxCoordinate2D::getMaxY() const {
  return max_y_;
}

void BoundingBoxCoordinate2D::setMinX(double min_x) {
  min_x_ = min_x;
}

void BoundingBoxCoordinate2D::setMaxX(double max_x) {
  max_x_ = max_x;
}

void BoundingBoxCoordinate2D::setMinY(double min_y) {
  min_y_ = min_y;
}

void BoundingBoxCoordinate2D::setMaxY(double max_y) {
  max_y_ = max_y;
}

BoundingBoxBinIndices2D::BoundingBoxBinIndices2D(uint32_t min_x, uint32_t max_x,
                                                 uint32_t min_y, uint32_t max_y)
    : min_x_(min_x), max_x_(max_x), min_y_(min_y), max_y_(max_y) {}

uint32_t BoundingBoxBinIndices2D::getMinX() const {
  return min_x_;
}

uint32_t BoundingBoxBinIndices2D::getMaxX() const {
  return max_x_;
}

uint32_t BoundingBoxBinIndices2D::getMinY() const {
  return min_y_;
}

uint32_t BoundingBoxBinIndices2D::getMaxY() const {
  return max_y_;
}

void BoundingBoxBinIndices2D::setMinX(uint32_t min_x) {
  min_x_ = min_x;
}

void BoundingBoxBinIndices2D::setMaxX(uint32_t max_x) {
  max_x_ = max_x;
}

void BoundingBoxBinIndices2D::setMinY(uint32_t min_y) {
  min_y_ = min_y;
}

void BoundingBoxBinIndices2D::setMaxY(uint32_t max_y) {
  max_y_ = max_y;
}

void BoundingBoxBinIndices2D::xMinMatchFlag() {
  match_flag_ |= 0x01; // Set the first bit for xMin match
}

void BoundingBoxBinIndices2D::xMaxMatchFlag() {
  match_flag_ |= 0x02; // Set the second bit for xMax match
}

void BoundingBoxBinIndices2D::yMinMatchFlag() {
  match_flag_ |= 0x04; // Set the third bit for yMin match
}

void BoundingBoxBinIndices2D::yMaxMatchFlag() {
  match_flag_ |= 0x08; // Set the fourth bit for yMax match
}

bool BoundingBoxBinIndices2D::isXMinMatch() const {
  return match_flag_ & 0x01;
}

bool BoundingBoxBinIndices2D::isXMaxMatch() const {
  return match_flag_ & 0x02;
}

bool BoundingBoxBinIndices2D::isYMinMatch() const {
  return match_flag_ & 0x04;
}

bool BoundingBoxBinIndices2D::isYMaxMatch() const {
  return match_flag_ & 0x08;
}

bool BoundingBoxBinIndices2D::isPerfectMatch() const {
  return (isXMinMatch() && isXMaxMatch() && isYMinMatch() && isYMaxMatch());
}

// BoundingBoxCoordinate3D implementations
BoundingBoxCoordinate3D::BoundingBoxCoordinate3D(double min_x, double max_x,
                                                 double min_y, double max_y,
                                                 double min_z, double max_z)
    : min_x_(min_x), max_x_(max_x), min_y_(min_y), max_y_(max_y), min_z_(min_z),
      max_z_(max_z) {}

double BoundingBoxCoordinate3D::getMinX() const {
  return min_x_;
}

double BoundingBoxCoordinate3D::getMaxX() const {
  return max_x_;
}

double BoundingBoxCoordinate3D::getMinY() const {
  return min_y_;
}

double BoundingBoxCoordinate3D::getMaxY() const {
  return max_y_;
}

double BoundingBoxCoordinate3D::getMinZ() const {
  return min_z_;
}

double BoundingBoxCoordinate3D::getMaxZ() const {
  return max_z_;
}

void BoundingBoxCoordinate3D::setMinX(double min_x) {
  min_x_ = min_x;
}

void BoundingBoxCoordinate3D::setMaxX(double max_x) {
  max_x_ = max_x;
}

void BoundingBoxCoordinate3D::setMinY(double min_y) {
  min_y_ = min_y;
}

void BoundingBoxCoordinate3D::setMaxY(double max_y) {
  max_y_ = max_y;
}

void BoundingBoxCoordinate3D::setMinZ(double min_z) {
  min_z_ = min_z;
}

void BoundingBoxCoordinate3D::setMaxZ(double max_z) {
  max_z_ = max_z;
}

// BoundingBoxBinIndices3D implementations
BoundingBoxBinIndices3D::BoundingBoxBinIndices3D(uint32_t min_x, uint32_t max_x,
                                                 uint32_t min_y, uint32_t max_y,
                                                 uint32_t min_z, uint32_t max_z)
    : min_x_(min_x), max_x_(max_x), min_y_(min_y), max_y_(max_y), min_z_(min_z),
      max_z_(max_z) {}

uint32_t BoundingBoxBinIndices3D::getMinX() const {
  return min_x_;
}

uint32_t BoundingBoxBinIndices3D::getMaxX() const {
  return max_x_;
}

uint32_t BoundingBoxBinIndices3D::getMinY() const {
  return min_y_;
}

uint32_t BoundingBoxBinIndices3D::getMaxY() const {
  return max_y_;
}

uint32_t BoundingBoxBinIndices3D::getMinZ() const {
  return min_z_;
}

uint32_t BoundingBoxBinIndices3D::getMaxZ() const {
  return max_z_;
}

void BoundingBoxBinIndices3D::setMinX(uint32_t min_x) {
  min_x_ = min_x;
}

void BoundingBoxBinIndices3D::setMaxX(uint32_t max_x) {
  max_x_ = max_x;
}

void BoundingBoxBinIndices3D::setMinY(uint32_t min_y) {
  min_y_ = min_y;
}

void BoundingBoxBinIndices3D::setMaxY(uint32_t max_y) {
  max_y_ = max_y;
}

void BoundingBoxBinIndices3D::setMinZ(uint32_t min_z) {
  min_z_ = min_z;
}

void BoundingBoxBinIndices3D::setMaxZ(uint32_t max_z) {
  max_z_ = max_z;
}

void BoundingBoxBinIndices3D::xMinMatchFlag() {
  match_flag_ |= 0x01; // Set the first bit for xMin match
}

void BoundingBoxBinIndices3D::xMaxMatchFlag() {
  match_flag_ |= 0x02; // Set the second bit for xMax match
}

void BoundingBoxBinIndices3D::yMinMatchFlag() {
  match_flag_ |= 0x04; // Set the third bit for yMin match
}

void BoundingBoxBinIndices3D::yMaxMatchFlag() {
  match_flag_ |= 0x08; // Set the fourth bit for yMax match
}

void BoundingBoxBinIndices3D::zMinMatchFlag() {
  match_flag_ |= 0x10; // Set the fifth bit for zMin match
}

void BoundingBoxBinIndices3D::zMaxMatchFlag() {
  match_flag_ |= 0x20; // Set the sixth bit for zMax match
}

bool BoundingBoxBinIndices3D::isXMinMatch() const {
  return match_flag_ & 0x01;
}

bool BoundingBoxBinIndices3D::isXMaxMatch() const {
  return match_flag_ & 0x02;
}

bool BoundingBoxBinIndices3D::isYMinMatch() const {
  return match_flag_ & 0x04;
}

bool BoundingBoxBinIndices3D::isYMaxMatch() const {
  return match_flag_ & 0x08;
}

bool BoundingBoxBinIndices3D::isZMinMatch() const {
  return match_flag_ & 0x10;
}

bool BoundingBoxBinIndices3D::isZMaxMatch() const {
  return match_flag_ & 0x20;
}

bool BoundingBoxBinIndices3D::isPerfectMatch() const {
  return (isXMinMatch() && isXMaxMatch() && isYMinMatch() && isYMaxMatch()
          && isZMinMatch() && isZMaxMatch());
}
