#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <memory>
#include <parquet/exception.h>
#include <parquet/platform.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <cstring>

#ifdef __linux__
#include <unistd.h>
#include <sys/resource.h>
#endif

// Floating Point Trie project includes

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/common/ConfigRegistry.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>


#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <parquet/arrow/writer.h>

using namespace airtree::query::bin_boundary;

// Exporter for 1DxT
template <typename RootNode>
std::shared_ptr<arrow::Table> export1DToArrow_1DxT(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(const std::vector<char> &,
                                                 size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_1D_Tiny) {
    throw std::invalid_argument("Invalid config for 1D_1x13 structure");
  }
  size_t offset = header.header_length;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders: one column for the reconstructed value, one for its
  // count
  arrow::DoubleBuilder value;
  arrow::UInt32Builder counts;

  // 3) Traverse two‐level trie: top 8 bits, then 5 bits
  for (size_t hi = 0; hi < root->populated.size(); ++hi) {
    if (!root->populated.test(hi))
      continue;
    auto *lvl1 = root->nodes[hi].get();
    if (!lvl1)
      continue;

    for (size_t lo = 0; lo < 32; ++lo) {
      unsigned int cnt = lvl1->counts[lo];
      if (cnt == 0)
        continue;

      // reconstruct the 13-bit code
      unsigned int bits13 = (hi << 5) | lo;
      double v = reConstruct<double>(bits13, /*precision_bits=*/13);

      value.Append(v);
      counts.Append(cnt);
    }
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_256; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;


  // 4) Add special counts from header
  value.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);
  value.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);
  value.Append(+0.0);
  counts.Append(header.pos_zero_count);
  value.Append(-0.0);
  counts.Append(header.neg_zero_count);
  value.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);

  // 5) Finalize Arrow arrays
  std::shared_ptr<arrow::Array> arr_value, arr_counts;
  auto st = value.Finish(&arr_value);
  if (!st.ok())
    throw std::runtime_error("Failed to build value array: " + st.ToString());
  st = counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Failed to build counts array: " + st.ToString());

  // 6) Build schema and table
  auto schema = arrow::schema({arrow::field("value", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {arr_value, arr_counts});
}


template <typename RootNode>
std::shared_ptr<arrow::Table> export1DToArrow_1DxF(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(const std::vector<char> &,
                                                 size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_1D_Fast) {
    throw std::invalid_argument("Invalid config for 1D_1x16 structure");
  }
  size_t offset = header.header_length;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders
  arrow::DoubleBuilder value;
  arrow::UInt32Builder counts;

  // 3) Walk the two‐level trie:
  //    - Level0: 8 bits (0..255)
  //    - Level1: 8 bits (0..255)
  for (size_t hi = 0; hi < root->populated.size(); ++hi) {
    if (!root->populated.test(hi))
      continue;
    auto *lvl1 = root->nodes[hi].get();
    if (!lvl1)
      continue;

    for (size_t lo = 0; lo < BINS_256; ++lo) {
      unsigned int cnt = lvl1->counts[lo];
      if (cnt == 0)
        continue;
      unsigned int bits16 = (hi << 8) | lo;
      double v = reConstruct<double>(bits16, /*precision_bits=*/16);

      value.Append(v);
      counts.Append(cnt);
    }
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_256; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  4) Add special counts from header
  value.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);
  value.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);
  value.Append(+0.0);
  counts.Append(header.pos_zero_count);
  value.Append(-0.0);
  counts.Append(header.neg_zero_count);
  value.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);

  // 5) Finalize Arrow arrays
  std::shared_ptr<arrow::Array> arr_value, arr_counts;
  auto st = value.Finish(&arr_value);
  if (!st.ok())
    throw std::runtime_error("Building value array failed: " + st.ToString());
  st = counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Building counts array failed: " + st.ToString());

  // 6) Build and return the table
  auto schema = arrow::schema({arrow::field("value", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {arr_value, arr_counts});
}

template <typename RootNode>
std::shared_ptr<arrow::Table> export1DToArrow_1DxP(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(const std::vector<char> &,
                                                 size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_1D_Precise) {
    throw std::invalid_argument("Invalid config for 1D_1x20 structure");
  }
  size_t offset = header.header_length;
  // std::cout << "Special numbers in header:" << std::endl;
  // std::cout << "  +Inf: " << header.pos_inf_count << std::endl;
  // std::cout << "  -Inf: " << header.neg_inf_count << std::endl;
  // std::cout << "  +0:   " << header.pos_zero_count << std::endl;
  // std::cout << "  -0:   " << header.neg_zero_count << std::endl;
  // std::cout << "  NaN:  " << header.nan_count << std::endl;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders
  arrow::DoubleBuilder value;
  arrow::UInt32Builder counts;

  // 3) Walk the three‐level trie:
  //    - Level0: 8 bits (0..256)
  //    - Level1: 6 bits (0..64)
  //    - Level2: 6 bits (0..64)
  for (unsigned int hi = 0; hi < BINS_256; ++hi) {
    if (!root->populated.test(hi) || root->counts[hi] == 0)
      continue;
    auto *lvl1 = root->nodes[hi].get();
    if (!lvl1)
      continue;

    for (unsigned int mid = 0; mid < BINS_64; ++mid) {
      if (!lvl1->populated.test(mid) || lvl1->counts[mid] == 0)
        continue;
      auto *lvl2 = lvl1->nodes[mid].get();
      if (!lvl2)
        continue;

      for (unsigned int lo = 0; lo < BINS_64; ++lo) {
        unsigned int cnt = lvl2->counts[lo];
        if (cnt == 0)
          continue;

        // reconstruct the 20-bit code: hi<<12 | mid<<6 | lo
        unsigned int bits20 = (hi << (BITS_6 + BITS_6)) | (mid << BITS_6) | lo;
        double v = reConstruct<double>(bits20, /*precision_bits=*/20);

        value.Append(v);
        counts.Append(cnt);
      }
    }
  }

  // 4) Add special counts from header
  value.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);
  value.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);
  value.Append(+0.0);
  counts.Append(header.pos_zero_count);
  value.Append(-0.0);
  counts.Append(header.neg_zero_count);
  value.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);


  // 5) Finalize Arrow arrays
  std::shared_ptr<arrow::Array> arr_value, arr_counts;
  auto st = value.Finish(&arr_value);
  if (!st.ok())
    throw std::runtime_error("Building value array failed: " + st.ToString());
  st = counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Building counts array failed: " + st.ToString());
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_256; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  6) Build and return the table
  auto schema = arrow::schema({arrow::field("value", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {arr_value, arr_counts});
}

// Exporter for 2DxP (10+10)
template <typename RootNode>
std::shared_ptr<arrow::Table> export2DToArrow_2DxP(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(std::vector<char>, size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_2D_Precise) {
    throw std::invalid_argument("Invalid config for 2D_10 structure");
  }
  size_t offset = header.header_length;
  // std::cout << "Special numbers in header:" << std::endl;
  // std::cout << "  +Inf: " << header.pos_inf_count << std::endl;
  // std::cout << "  -Inf: " << header.neg_inf_count << std::endl;
  // std::cout << "  +0:   " << header.pos_zero_count << std::endl;
  // std::cout << "  -0:   " << header.neg_zero_count << std::endl;
  // std::cout << "  NaN:  " << header.nan_count << std::endl;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders
  arrow::DoubleBuilder dim1, dim2;
  arrow::UInt32Builder counts;

  // 3) Recursive lambda: params = self, node, tle_idx, combined, ndims, level
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, unsigned int ndims,
                      int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // descend into the TLE bucket
      auto *n0 = static_cast<RootNode *>(node);
      auto &child = n0->nodes[tle_idx];
      if (child) {
        self(self, child.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);
      }

    } else if (level == 1) {
      // consume first 10 bits → TrieNode_2D_10
      auto *n1 = static_cast<TrieNode_2D_10 *>(node);
      for (unsigned int b1 = 0; b1 < n1->populated.size(); ++b1) {
        if (!n1->populated.test(b1))
          continue;
        // one‐chunk leaf: exactly one real dim (ndims==1 or ndims==2)
        if (ndims == 1 || ndims == 2) {

          if (ndims == 2) { // dim1 only
            std::bitset<3> TLE_dim1 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits = getprependbits(TLE_dim1);
            unsigned int bits12 = (prependbits.to_ulong() << 10) | b1;
            double v = reConstruct<double>(bits12, 12);
            dim1.Append(v);
            auto tle_dim2 = tle_idx & 0x7;
            if (tle_dim2 == 0) { // NaN
              dim2.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (tle_dim2 == 1) { // +ve Inf
              dim2.Append(std::numeric_limits<double>::infinity());
            } else if (tle_dim2 == 4) { // 0
              dim2.Append(0.0);
            } else if (tle_dim2 == 7) { // -ve Inf
              dim2.Append(-std::numeric_limits<double>::infinity());
            }
          } else { // dim2 only
            std::bitset<3> TLE_dim2 =
                tle_idx & 0x7; // Extract last 3 bits of TLE
            std::bitset<2> prependbits = getprependbits(TLE_dim2);
            unsigned int bits12 = (prependbits.to_ulong() << 10) | b1;
            double v = reConstruct<double>(bits12, 12);
            auto tle_dim1 = (tle_idx >> 3) & 0x7; // Extract first 3 bits of TLE
            if (tle_dim1 == 0) {                  // NaN
              dim1.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (tle_dim1 == 1) { // +ve Inf
              dim1.Append(std::numeric_limits<double>::infinity());
            } else if (tle_dim1 == 4) { // 0
              dim1.Append(0.0);
            } else if (tle_dim1 == 7) { // -ve Inf
              dim1.Append(-std::numeric_limits<double>::infinity());
            }
            dim2.Append(v);
          }
          counts.Append(n1->counts[b1]);
          continue;
        }

        // otherwise descend to level‐2
        auto &c2 = n1->nodes[b1];
        if (c2) {
          self(self, c2.get(), tle_idx, b1, ndims, /*level=*/2);
        }
      }

    } else {
      // level‐2: TrieNode_2D_10_Level1 → final 20‐bit leaf
      auto *n2 = static_cast<TrieNode_2D_10_Level1 *>(node);
      for (size_t b2 = 0; b2 < BINS_1024; ++b2) {
        unsigned int cnt = n2->counts[b2];
        if (cnt == 0)
          continue;
        uint32_t bits20 = uint32_t((combined << 10) | b2);

        auto [c1, c2] = reverse_combine_chunks_10b(bits20);
        std::bitset<3> TLE_dim1 = (tle_idx >> 3) & 0x7;
        std::bitset<2> prependbits_1 = getprependbits(TLE_dim1);
        std::bitset<3> TLE_dim2 = tle_idx & 0x7;
        std::bitset<2> prependbits_2 = getprependbits(TLE_dim2);

        unsigned int bits12_1 = (prependbits_1.to_ulong() << 10) | c1;
        unsigned int bits12_2 = (prependbits_2.to_ulong() << 10) | c2;
        dim1.Append(reConstruct<double>(bits12_1, 12));
        dim2.Append(reConstruct<double>(bits12_2, 12));
        counts.Append(cnt);
      }
    }
  };

  // 4) Kick‐off: one call per non‐empty TLE bucket
  for (unsigned int tle = 0; tle < root->populated.size(); ++tle) {
    if (!root->populated.test(tle) || root->counts[tle] == 0)
      continue;
    unsigned int nd = getNumDims2D(tle);
    traverse(traverse, root, tle, /*combined=*/0, nd, /*level=*/0);
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_64; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  // For ND, we treat both +0 and -0 as a single “special” number 0
  dim1.Append(std::numeric_limits<double>::infinity());
  dim2.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);
  dim1.Append(-std::numeric_limits<double>::infinity());
  dim2.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);
  dim1.Append(0.0);
  dim2.Append(0.0);
  counts.Append(header.pos_zero_count + header.neg_zero_count);
  dim1.Append(std::numeric_limits<double>::quiet_NaN());
  dim2.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);

  // 5) Build and return Arrow table
  std::shared_ptr<arrow::Array> A1, A2, A3;
  auto st = dim1.Finish(&A1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&A2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&A3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {A1, A2, A3});
}

// Exporter for 2DxF (8+8)
// Not finished yet
template <typename RootNode>
std::shared_ptr<arrow::Table> export2DToArrow_2DxF(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(std::vector<char>, size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_2D_Fast) {
    throw std::invalid_argument("Invalid config for 2D_88 structure");
  }
  size_t offset = header.header_length;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders
  arrow::DoubleBuilder dim1, dim2;
  arrow::UInt32Builder counts;

  // 3) Recursive lambda: params = self, node, tle_idx, combined, ndims, level
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, unsigned int ndims,
                      int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // descend into the TLE bucket
      auto *n0 = static_cast<RootNode *>(node);
      auto &child = n0->nodes[tle_idx];
      if (child) {
        self(self, child.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);
      }

    } else if (level == 1) {
      // consume first 8 bits → Node2D_88_l0
      auto *n1 = static_cast<TrieNode_16 *>(node);
      for (size_t b1 = 0; b1 < n1->populated.size(); ++b1) {
        if (!n1->populated.test(b1))
          continue;

        // one‐chunk leaf: exactly one real dim (ndims==1 or ndims==2)
        if (ndims == 1 || ndims == 2) {
          double v = reConstruct<double>(b1, 8);
          if (ndims == 2) { // dim1 only
            dim1.Append(v);
            dim2.AppendNull();
          } else { // dim2 only
            dim1.AppendNull();
            dim2.Append(v);
          }
          counts.Append(n1->counts[b1]);
          continue;
        }

        // otherwise descend to level‐2
        auto &c2 = n1->nodes[b1];
        if (c2) {
          self(self, c2.get(), tle_idx, b1, ndims, /*level=*/2);
        }
      }

    } else {
      // level‐2: Node2D_88_l1 → final 16‐bit leaf
      auto *n2 = static_cast<TrieNode_16_Level1 *>(node);
      for (size_t b2 = 0; b2 < BINS_256; ++b2) {
        unsigned int cnt = n2->counts[b2];
        if (cnt == 0)
          continue;
        unsigned int bits16 = (combined << 8) | b2;
        auto [c1, c2] = reverse_combine_chunks_8b_temp(bits16);

        dim1.Append(reConstruct<double>(c1, 8));
        dim2.Append(reConstruct<double>(c2, 8));
        counts.Append(cnt);
      }
    }
  };

  // 4) Kick‐off: one call per non‐empty TLE bucket
  for (unsigned int tle = 0; tle < root->populated.size(); ++tle) {
    if (!root->populated.test(tle) || root->TLEcounts[tle] == 0)
      continue;
    unsigned int nd = getNumDims2D(tle);
    traverse(traverse, root, tle, /*combined=*/0, nd, /*level=*/0);
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < 64; i++ ) {
  //   if(!root->TLEcounts[i]) continue;
  //   total += root->TLEcounts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  5) Build and return Arrow table
  std::shared_ptr<arrow::Array> A1, A2, A3;
  auto st = dim1.Finish(&A1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&A2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&A3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {A1, A2, A3});
}

// Export for 3DxP (10+10+10)
template <typename RootNode>
std::shared_ptr<arrow::Table> export3DToArrow_3DxP(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(const std::vector<char> &,
                                                 size_t &)) {

  // 1) Deserialize header + trie root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_3D_Precise) {
    throw std::invalid_argument("Invalid config for 3D_3x10 structure");
  }
  size_t offset = header.header_length;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Prepare Arrow builders
  arrow::DoubleBuilder dim1, dim2, dim3;
  arrow::UInt32Builder counts;

  // 3) Recursive traversal lambda
  //    params: self, node, tle_idx (0–511), combined bits, ndims mask, level
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, int ndims, int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // descend into the TLE bucket
      auto *n0 = static_cast<RootNode *>(node);
      auto &child = n0->nodes[tle_idx];
      if (child) {
        self(self, child.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);
      }

    } else if (level == 1) {
      // consume first 10 bits → Node3D_3x10_l0
      auto *n1 = static_cast<Node3D_3x10_l0 *>(node);
      for (size_t i = 0; i < n1->populated.size(); ++i) {
        if (!n1->populated.test(i))
          continue;

        // single-chunk leaf (one dim only)
        if (ndims == 1 || ndims == 2 || ndims == 4) {

          if (ndims == 4) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 6) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            unsigned int bits12 = (prependbits1.to_ulong() << 10) | i;
            double v1 = reConstruct<double>(bits12, 12);
            dim1.Append(v1);
            std::bitset<3> TLE_dim2 =
                (tle_idx >> 3) & 0x7; // Extract next 3 bits of TLE
            std::bitset<3> TLE_dim3 = tle_idx & 0x7; // Extract last
            if (TLE_dim2 == 0) {                     // NaN
              dim2.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim2 == 1) { // +ve Inf
              dim2.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim2 == 4) { // 0
              dim2.Append(0.0);
            } else if (TLE_dim2 == 7) { // -ve Inf
              dim2.Append(-std::numeric_limits<double>::infinity());
            }
            if (TLE_dim3 == 0) { // NaN
              dim3.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim3 == 1) { // +ve Inf
              dim3.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim3 == 4) { // 0
              dim3.Append(0.0);
            } else if (TLE_dim3 == 7) { // -ve Inf
              dim3.Append(-std::numeric_limits<double>::infinity());
            }

          } else if (ndims == 2) {
            std::bitset<3> TLE_dim1 =
                (tle_idx >> 6) & 0x7; // Extract first 3 bits of TLE
            std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim3 =
                tle_idx & 0x7; // Extract last 3 bits of TLE
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            unsigned int bits12 = (prependbits2.to_ulong() << 10) | i;
            double v2 = reConstruct<double>(bits12, 12);
            if (TLE_dim1 == 0) { // NaN
              dim1.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim1 == 1) { // +ve Inf
              dim1.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim1 == 4) { // 0
              dim1.Append(0.0);
            } else if (TLE_dim1 == 7) { // -ve Inf
              dim1.Append(-std::numeric_limits<double>::infinity());
            }
            dim2.Append(v2);
            if (TLE_dim3 == 0) { // NaN
              dim3.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim3 == 1) { // +ve Inf
              dim3.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim3 == 4) { // 0
              dim3.Append(0.0);
            } else if (TLE_dim3 == 7) { // -ve Inf
              dim3.Append(-std::numeric_limits<double>::infinity());
            }
          } else {
            std::bitset<3> TLE_dim1 =
                (tle_idx >> 6) & 0x7; // Extract first 3 bits of TLE
            std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim3 = tle_idx & 0x7;
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            unsigned int bits12 = (prependbits3.to_ulong() << 10) | i;
            double v3 = reConstruct<double>(bits12, 12);
            if (TLE_dim1 == 0) { // NaN
              dim1.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim1 == 1) { // +ve Inf
              dim1.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim1 == 4) { // 0
              dim1.Append(0.0);
            } else if (TLE_dim1 == 7) { // -ve Inf
              dim1.Append(-std::numeric_limits<double>::infinity());
            }
            if (TLE_dim2 == 0) { // NaN
              dim2.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim2 == 1) { // +ve Inf
              dim2.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim2 == 4) { // 0
              dim2.Append(0.0);
            } else if (TLE_dim2 == 7) { // -ve Inf
              dim2.Append(-std::numeric_limits<double>::infinity());
            }
            dim3.Append(v3);
          }
          counts.Append(n1->counts[i]);
          continue;
        }

        // else descend to level 2
        uint64_t nc = (combined << 10) | i;
        auto &c2 = n1->nodes[i];
        if (c2) {
          self(self, c2.get(), tle_idx, nc, ndims, /*level=*/2);
        }
      }

    } else if (level == 2) {
      // consume second 10 bits → Node3D_3x10_l1
      auto *n2 = static_cast<Node3D_3x10_l1 *>(node);
      for (size_t i = 0; i < n2->populated.size(); ++i) {
        if (!n2->populated.test(i))
          continue;

        // two-chunk leaf (two dims)
        if (ndims == 3 || ndims == 5 || ndims == 6) {

          uint32_t bits20 = uint32_t((combined << 10) | i);
          auto [x, y] = reverse_combine_chunks_10b(bits20);
          double v1 = 0, v2 = 0, v3 = 0;
          if (ndims == 3) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim3 = tle_idx & 0x7;
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | x, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | y, 12);
          } else if (ndims == 5) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim3 = tle_idx & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | x, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | y, 12);
          } else {
            std::bitset<3> TLE_dim1 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | x, 12);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | y, 12);
          }
          if (v1 == 0) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 6) & 0x7;
            if (TLE_dim1 == 0) { // NaN
              dim1.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim1 == 1) { // +ve Inf
              dim1.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim1 == 4) { // 0
              dim1.Append(0.0);
            } else if (TLE_dim1 == 7) { // -ve Inf
              dim1.Append(-std::numeric_limits<double>::infinity());
            }
          } else
            dim1.Append(v1);
          if (v2 == 0) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
            if (TLE_dim2 == 0) { // NaN
              dim2.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim2 == 1) { // +ve Inf
              dim2.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim2 == 4) { // 0
              dim2.Append(0.0);
            } else if (TLE_dim2 == 7) { // -ve Inf
              dim2.Append(-std::numeric_limits<double>::infinity());
            }
          } else
            dim2.Append(v2);
          if (v3 == 0) {
            std::bitset<3> TLE_dim3 =
                tle_idx & 0x7;   // Extract last 3 bits of TLE
            if (TLE_dim3 == 0) { // NaN
              dim3.Append(std::numeric_limits<double>::quiet_NaN());
            } else if (TLE_dim3 == 1) { // +ve Inf
              dim3.Append(std::numeric_limits<double>::infinity());
            } else if (TLE_dim3 == 4) { // 0
              dim3.Append(0.0);
            } else if (TLE_dim3 == 7) { // -ve Inf
              dim3.Append(-std::numeric_limits<double>::infinity());
            }
          } else
            dim3.Append(v3);
          counts.Append(n2->counts[i]);
          continue;
        }

        // else descend to level 3
        uint64_t nc = (combined << 10) | i;
        auto &c3 = n2->nodes[i];
        if (c3) {
          self(self, c3.get(), tle_idx, nc, ndims, /*level=*/3);
        }
      }

    } else {
      // consume third 10 bits → Node3D_3x10_l2 → final 30-bit leaf
      auto *n3 = static_cast<Node3D_3x10_l2 *>(node);
      for (size_t i = 0; i < n3->populated.size(); ++i) {
        if (!n3->populated.test(i))
          continue;
        uint32_t cnt = n3->counts[i];
        if (cnt == 0)
          continue;

        uint32_t bits30 = uint32_t((combined << 10) | i);
        auto [x, y, z] = reverse_combine_chunks_10b_3(bits30);
        std::bitset<3> TLE_dim1 = (tle_idx >> 6) & 0x7;
        std::bitset<3> TLE_dim2 = (tle_idx >> 3) & 0x7;
        std::bitset<3> TLE_dim3 = tle_idx & 0x7;
        std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
        std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
        std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
        dim1.Append(
            reConstruct<double>((prependbits1.to_ulong() << 10) | x, 12));
        dim2.Append(
            reConstruct<double>((prependbits2.to_ulong() << 10) | y, 12));
        dim3.Append(
            reConstruct<double>((prependbits3.to_ulong() << 10) | z, 12));
        counts.Append(cnt);
      }
    }
  };

  // 4) Kick off recursion for each non-empty TLE bucket
  for (unsigned int tle = 0; tle < root->populated.size(); ++tle) {
    if (!root->populated.test(tle) || root->counts[tle] == 0)
      continue;
    int nd = getNumDims3D(tle);
    traverse(traverse, root, tle, /*combined=*/0, nd, /*level=*/0);
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_512; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  dim1.Append(std::numeric_limits<double>::infinity());
  dim2.Append(std::numeric_limits<double>::infinity());
  dim3.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);
  dim1.Append(-std::numeric_limits<double>::infinity());
  dim2.Append(-std::numeric_limits<double>::infinity());
  dim3.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);
  dim1.Append(0.0);
  dim2.Append(0.0);
  dim3.Append(0.0);
  counts.Append(header.pos_zero_count + header.neg_zero_count);
  dim1.Append(std::numeric_limits<double>::quiet_NaN());
  dim2.Append(std::numeric_limits<double>::quiet_NaN());
  dim3.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);
  // 5) Finalize Arrow arrays and build table
  std::shared_ptr<arrow::Array> A1, A2, A3, A4;
  auto st = dim1.Finish(&A1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&A2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim3.Finish(&A3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&A4);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("dim3", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {A1, A2, A3, A4});
}

// Export for 3DxF (8+8+8)
// Not finished yet
template <typename RootNode>
std::shared_ptr<arrow::Table> export3DToArrow_3DxF(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(const std::vector<char> &,
                                                 size_t &)) {

  // 1) Deserialize header + root
  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_3D_Fast) {
    throw std::invalid_argument("Invalid config for 3D_888 structure");
  }
  size_t offset = header.header_length;
  auto root_up = deserializeRoot(buffer, offset);
  if (!root_up)
    throw std::runtime_error("Failed to deserialize trie root");
  RootNode *root = root_up.get();

  // 2) Arrow builders
  arrow::DoubleBuilder dim1, dim2, dim3;
  arrow::UInt32Builder counts;

  // 3) Recursive walker
  //    params: self, node, tle_idx, combined_bits, ndims, level
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, int ndims, int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // into the TLE bucket
      auto *r = static_cast<RootNode *>(node);
      auto &child0 = r->nodes[tle_idx];
      if (!child0)
        return;
      self(self, child0.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);

    } else if (level == 1) {
      // consume first 8 bits → Node3D_888_l0
      auto *n0 = static_cast<Node3D_888_l0 *>(node);
      for (size_t b1 = 0; b1 < n0->populated.size(); ++b1) {
        if (!n0->populated.test(b1))
          continue;

        // single‐chunk leaves: exactly one real dim
        if (ndims == 1 || ndims == 2 || ndims == 4) {
          double v = reConstruct<double>(b1, 8);
          if (ndims == 4) {
            dim1.Append(v);
            dim2.AppendNull();
            dim3.AppendNull();
          } else if (ndims == 2) {
            dim1.AppendNull();
            dim2.Append(v);
            dim3.AppendNull();
          } else {
            dim1.AppendNull();
            dim2.AppendNull();
            dim3.Append(v);
          }
          counts.Append(n0->counts[b1]);
          continue;
        }

        // else descend
        uint64_t nc = (combined << 8) | b1;
        auto &c1 = n0->nodes[b1];
        if (c1)
          self(self, c1.get(), tle_idx, nc, ndims, /*level=*/2);
      }

    } else if (level == 2) {
      // consume second 8 bits → Node3D_888_l1
      auto *n1 = static_cast<TrieNode_16 *>(node);
      for (size_t b2 = 0; b2 < n1->populated.size(); ++b2) {
        if (!n1->populated.test(b2))
          continue;

        // two‐chunk leaves: exactly two real dims
        if (ndims == 3 || ndims == 5 || ndims == 6) {
          uint32_t bits16 = uint32_t((combined << 8) | b2);
          auto [x, y] = reverse_combine_chunks_8b(bits16);
          double v1 = 0, v2 = 0, v3 = 0;
          if (ndims == 3) {
            v2 = reConstruct<double>(x, 8);
            v3 = reConstruct<double>(y, 8);
          } else if (ndims == 5) {
            v1 = reConstruct<double>(x, 8);
            v3 = reConstruct<double>(y, 8);
          } else {
            v1 = reConstruct<double>(x, 8);
            v2 = reConstruct<double>(y, 8);
          }
          if (v1 == 0)
            dim1.AppendNull();
          else
            dim1.Append(v1);
          if (v2 == 0)
            dim2.AppendNull();
          else
            dim2.Append(v2);
          if (v3 == 0)
            dim3.AppendNull();
          else
            dim3.Append(v3);
          counts.Append(n1->counts[b2]);
          continue;
        }

        // else descend
        uint64_t nc = (combined << 8) | b2;
        auto &c2 = n1->nodes[b2];
        if (c2)
          self(self, c2.get(), tle_idx, nc, ndims, /*level=*/3);
      }

    } else {
      // consume third 8 bits → Node3D_888_l2 → final 3-chunk leaf
      auto *n2 = static_cast<TrieNode_16_Level1 *>(node);
      for (size_t b3 = 0; b3 < BINS_256; ++b3) {
        uint32_t cnt = n2->counts[b3];
        if (cnt == 0)
          continue;
        uint32_t bits24 = uint32_t((combined << 8) | b3);
        auto [x, y, z] = reverse_combine_chunks_8b_3(bits24);

        dim1.Append(reConstruct<double>(x, 8));
        dim2.Append(reConstruct<double>(y, 8));
        dim3.Append(reConstruct<double>(z, 8));
        counts.Append(cnt);
      }
    }
  };

  // 4) Kick‐off: one call per non‐empty TLE bucket
  for (unsigned int tle = 0; tle < BINS_512; ++tle) {
    if (!root->populated.test(tle) || root->counts[tle] == 0)
      continue;
    int nd = getNumDims3D(tle);
    traverse(traverse, root, tle, /*combined=*/0, nd, /*level=*/0);
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_512; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  5) Finish and return
  std::shared_ptr<arrow::Array> A1, A2, A3, A4;
  auto st = dim1.Finish(&A1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&A2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim3.Finish(&A3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&A4);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("dim3", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {A1, A2, A3, A4});
}

void special_append(arrow::DoubleBuilder &builder, unsigned int tle_idx,
                    size_t dim) {
  if (dim == 1) {
    std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
    if (TLE_dim1 == 0) { // NaN
      builder.Append(std::numeric_limits<double>::quiet_NaN());
    } else if (TLE_dim1 == 1) { // +ve Inf
      builder.Append(std::numeric_limits<double>::infinity());
    } else if (TLE_dim1 == 4) { // 0
      builder.Append(0.0);
    } else if (TLE_dim1 == 7) { // -ve Inf
      builder.Append(-std::numeric_limits<double>::infinity());
    }
  } else if (dim == 2) {
    std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
    if (TLE_dim2 == 0) { // NaN
      builder.Append(std::numeric_limits<double>::quiet_NaN());
    } else if (TLE_dim2 == 1) { // +ve Inf
      builder.Append(std::numeric_limits<double>::infinity());
    } else if (TLE_dim2 == 4) { // 0
      builder.Append(0.0);
    } else if (TLE_dim2 == 7) { // -ve Inf
      builder.Append(-std::numeric_limits<double>::infinity());
    }
  } else if (dim == 3) {
    std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
    if (TLE_dim3 == 0) { // NaN
      builder.Append(std::numeric_limits<double>::quiet_NaN());
    } else if (TLE_dim3 == 1) { // +ve Inf
      builder.Append(std::numeric_limits<double>::infinity());
    } else if (TLE_dim3 == 4) { // 0
      builder.Append(0.0);
    } else if (TLE_dim3 == 7) { // -ve Inf
      builder.Append(-std::numeric_limits<double>::infinity());
    }
  } else if (dim == 4) {
    std::bitset<3> TLE_dim4 = tle_idx & 0x7;
    if (TLE_dim4 == 0) { // NaN
      builder.Append(std::numeric_limits<double>::quiet_NaN());
    } else if (TLE_dim4 == 1) { // +ve Inf
      builder.Append(std::numeric_limits<double>::infinity());
    } else if (TLE_dim4 == 4) { // 0
      builder.Append(0.0);
    } else if (TLE_dim4 == 7) { // -ve Inf
      builder.Append(-std::numeric_limits<double>::infinity());
    }
  }
}

// Export for 4DxP (10+10+10+10)
template <typename RootNode>
std::shared_ptr<arrow::Table> export4DToArrow_4DxP(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(std::vector<char>, size_t &)) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_4D_Precise) {
    throw std::invalid_argument("Invalid config for 4D_4x10 structure");
  }
  size_t offset = header.header_length;

  auto root = deserializeRoot(buffer, offset);
  if (!root)
    throw std::runtime_error("Failed to deserialize trie root");

  // --- builders for each column ---
  arrow::DoubleBuilder dim1, dim2, dim3, dim4;
  arrow::UInt32Builder counts;

  // Recursive lambda: node at 'level', building up 'combined' and dispatching
  // on ndims
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, int ndims, int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // level-0: just descend into the TLE bucket
      auto *r = static_cast<RootNode *>(node);
      auto &child = r->nodes[tle_idx];
      if (!child)
        return;
      self(self, child.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);

    } else if (level == 1) {
      // level-1: Node4D_4x8_l0
      auto *n0 = static_cast<Node4D_4x10_l0 *>(node);
      for (size_t i = 0; i < n0->populated.size(); ++i) {
        if (!n0->populated.test(i))
          continue;

        // --- single-chunk leaf (ndims 1,2,4,8) ---
        if (ndims == 1 || ndims == 2 || ndims == 4 || ndims == 8) {
          if (ndims == 1) {
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            unsigned int bits12 = (prependbits4.to_ulong() << 10) | i;
            double v = reConstruct<double>(bits12, 12);
            special_append(dim1, tle_idx, 1);
            special_append(dim2, tle_idx, 2);
            special_append(dim3, tle_idx, 3);
            dim4.Append(v);
          } else if (ndims == 2) {
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            unsigned int bits12 = (prependbits3.to_ulong() << 10) | i;
            double v = reConstruct<double>(bits12, 12);
            special_append(dim1, tle_idx, 1);
            special_append(dim2, tle_idx, 2);
            dim3.Append(v);
            special_append(dim4, tle_idx, 4);
          } else if (ndims == 4) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            unsigned int bits12 = (prependbits2.to_ulong() << 10) | i;
            double v = reConstruct<double>(bits12, 12);
            special_append(dim1, tle_idx, 1);
            dim2.Append(v);
            special_append(dim3, tle_idx, 3);
            special_append(dim4, tle_idx, 4);
          } else /*8*/ {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            unsigned int bits12 = (prependbits1.to_ulong() << 10) | i;
            double v = reConstruct<double>(bits12, 12);
            dim1.Append(v);
            special_append(dim2, tle_idx, 2);
            special_append(dim3, tle_idx, 3);
            special_append(dim4, tle_idx, 4);
          }
          counts.Append(n0->counts[i]);
          continue;
        }

        if (n0->nodes[i]) {
          self(self, n0->nodes[i].get(), tle_idx, i, ndims, 2);
        }
      }

    } else if (level == 2) {
      // level-2: Node4D_4x8_l1
      auto *n1 = static_cast<Node4D_4x10_l1 *>(node);
      for (size_t i = 0; i < n1->populated.size(); ++i) {
        if (!n1->populated.test(i))
          continue;

        // --- two-chunk leaf (ndims 3,5,6,9,10,12) ---
        if (ndims == 3 || ndims == 5 || ndims == 6 || ndims == 9 || ndims == 10
            || ndims == 12) {
          uint64_t x = (combined << 10) | i;
          auto [c1, c2] = reverse_combine_chunks_10b(x);
          double v1 = 0, v2 = 0, v3 = 0, v4 = 0;

          if (ndims == 3) {
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c1, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c2, 12);
          } else if (ndims == 5) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c1, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c2, 12);
          } else if (ndims == 6) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c1, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c2, 12);
          } else if (ndims == 9) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c2, 12);
          } else if (ndims == 10) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c2, 12);
          } else { // ndims == 12
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c2, 12);
          }

          if (v1 == 0)
            special_append(dim1, tle_idx, 1);
          else
            dim1.Append(v1);
          if (v2 == 0)
            special_append(dim2, tle_idx, 2);
          else
            dim2.Append(v2);
          if (v3 == 0)
            special_append(dim3, tle_idx, 3);
          else
            dim3.Append(v3);
          if (v4 == 0)
            special_append(dim4, tle_idx, 4);
          else
            dim4.Append(v4);
          counts.Append(n1->counts[i]);
          continue;
        }

        // descend level-3
        uint64_t nc = (combined << 10) | i;
        if (n1->nodes[i]) {
          self(self, n1->nodes[i].get(), tle_idx, nc, ndims, 3);
        }
      }

    } else if (level == 3) {
      // level-3: Node4D_4x8_l2
      auto *n2 = static_cast<Node4D_4x10_l2 *>(node);
      for (size_t i = 0; i < n2->populated.size(); ++i) {
        if (!n2->populated.test(i))
          continue;

        // --- three-chunk leaf (ndims 7,11,13,14) ---
        if (ndims == 7 || ndims == 11 || ndims == 13 || ndims == 14) {
          uint64_t x = (combined << 10) | i;
          auto [c1, c2, c3] = reverse_combine_chunks_10b_3(x);
          double v1 = 0, v2 = 0, v3 = 0, v4 = 0;
          if (ndims == 7) {
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c1, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c2, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c3, 12);
          } else if (ndims == 11) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c2, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c3, 12);
          } else if (ndims == 13) {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim4 = tle_idx & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits4 = getprependbits(TLE_dim4);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c2, 12);
            v4 = reConstruct<double>((prependbits4.to_ulong() << 10) | c3, 12);
          } else /*14*/ {
            std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
            std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
            std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
            std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
            std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
            std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
            v1 = reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12);
            v2 = reConstruct<double>((prependbits2.to_ulong() << 10) | c2, 12);
            v3 = reConstruct<double>((prependbits3.to_ulong() << 10) | c3, 12);
          }

          if (v1 == 0)
            special_append(dim1, tle_idx, 1);
          else
            dim1.Append(v1);
          if (v2 == 0)
            special_append(dim2, tle_idx, 2);
          else
            dim2.Append(v2);
          if (v3 == 0)
            special_append(dim3, tle_idx, 3);
          else
            dim3.Append(v3);
          if (v4 == 0)
            special_append(dim4, tle_idx, 4);
          else
            dim4.Append(v4);
          counts.Append(n2->counts[i]);
          continue;
        }

        // descend level-4
        uint64_t nc = (combined << 10) | i;
        if (n2->nodes[i]) {
          self(self, n2->nodes[i].get(), tle_idx, nc, ndims, 4);
        }
      }

    } else {
      // level-4: Node4D_4x10_l3 — final 40-bit leaf
      auto *n3 = static_cast<Node4D_4x10_l3 *>(node);
      for (size_t i = 0; i < BINS_1024; ++i) {
        uint32_t cnt = n3->counts[i];
        if (cnt == 0)
          continue; // only emit non-zero counts

        uint64_t bits = (combined << 10) | i;
        auto [c1, c2, c3, c4] = reverse_combine_chunks_10b_4(bits);
        std::bitset<3> TLE_dim1 = (tle_idx >> 9) & 0x7;
        std::bitset<3> TLE_dim2 = (tle_idx >> 6) & 0x7;
        std::bitset<3> TLE_dim3 = (tle_idx >> 3) & 0x7;
        std::bitset<3> TLE_dim4 = tle_idx & 0x7;

        std::bitset<2> prependbits1 = getprependbits(TLE_dim1);
        std::bitset<2> prependbits2 = getprependbits(TLE_dim2);
        std::bitset<2> prependbits3 = getprependbits(TLE_dim3);
        std::bitset<2> prependbits4 = getprependbits(TLE_dim4);

        dim1.Append(
            reConstruct<double>((prependbits1.to_ulong() << 10) | c1, 12));
        dim2.Append(
            reConstruct<double>((prependbits2.to_ulong() << 10) | c2, 12));
        dim3.Append(
            reConstruct<double>((prependbits3.to_ulong() << 10) | c3, 12));
        dim4.Append(
            reConstruct<double>((prependbits4.to_ulong() << 10) | c4, 12));
        counts.Append(cnt);
      }
    }
  };

  // Drive recursion over all nonempty TLE buckets
  for (unsigned int tle_idx = 0; tle_idx < BINS_4096; ++tle_idx) {
    if (root->populated.test(tle_idx) && root->counts[tle_idx] > 0) {
      int ndims = getNumDims4D(tle_idx);
      traverse(
          traverse, root.get(), tle_idx, /*combined=*/0, ndims, /*level=*/0);
    }
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_4096; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  Finish arrays
  dim1.Append(std::numeric_limits<double>::infinity());
  dim2.Append(std::numeric_limits<double>::infinity());
  dim3.Append(std::numeric_limits<double>::infinity());
  dim4.Append(std::numeric_limits<double>::infinity());
  counts.Append(header.pos_inf_count);

  dim1.Append(-std::numeric_limits<double>::infinity());
  dim2.Append(-std::numeric_limits<double>::infinity());
  dim3.Append(-std::numeric_limits<double>::infinity());
  dim4.Append(-std::numeric_limits<double>::infinity());
  counts.Append(header.neg_inf_count);

  dim1.Append(0.0);
  dim2.Append(0.0);
  dim3.Append(0.0);
  dim4.Append(0.0);
  counts.Append(header.pos_zero_count + header.neg_zero_count);

  dim1.Append(std::numeric_limits<double>::quiet_NaN());
  dim2.Append(std::numeric_limits<double>::quiet_NaN());
  dim3.Append(std::numeric_limits<double>::quiet_NaN());
  dim4.Append(std::numeric_limits<double>::quiet_NaN());
  counts.Append(header.nan_count);


  std::shared_ptr<arrow::Array> a1, a2, a3, a4, a5;
  auto st = dim1.Finish(&a1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&a2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim3.Finish(&a3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim4.Finish(&a4);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&a5);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  // Build table
  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("dim3", arrow::float64()),
                               arrow::field("dim4", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {a1, a2, a3, a4, a5});
}


// Export for 4DxF (8+8+8+8)
// Not finished yet
template <typename RootNode> // 4×8 exporter
std::shared_ptr<arrow::Table> export4DToArrow_4DxF(
    const std::vector<char> &buffer,
    std::unique_ptr<RootNode> (*deserializeRoot)(std::vector<char>, size_t &)) {

  auto header = airtree::core::common::deserializeHeader(buffer);
  if (header.config != airtree::core::common::ConfigWire::Config_4D_Fast) {
    throw std::invalid_argument("Invalid config for 4D_4x8 structure");
  }
  size_t offset = header.header_length;

  auto root = deserializeRoot(buffer, offset);
  if (!root)
    throw std::runtime_error("Failed to deserialize trie root");

  // --- builders for each column ---
  arrow::DoubleBuilder dim1, dim2, dim3, dim4;
  arrow::UInt32Builder counts;

  // Recursive lambda: node at 'level', building up 'combined' and dispatching
  // on ndims
  auto traverse = [&](auto &self, void *node, unsigned int tle_idx,
                      uint64_t combined, int ndims, int level) -> void {
    if (!node)
      return;

    if (level == 0) {
      // level-0: just descend into the TLE bucket
      auto *r = static_cast<RootNode *>(node);
      auto &child = r->nodes[tle_idx];
      if (!child)
        return;
      self(self, child.get(), tle_idx, /*combined=*/0, ndims, /*level=*/1);

    } else if (level == 1) {
      // level-1: Node4D_4x8_l0
      auto *n0 = static_cast<Node4D_4x8_l0 *>(node);
      for (size_t i = 0; i < n0->populated.size(); ++i) {
        if (!n0->populated.test(i))
          continue;

        // --- single-chunk leaf (ndims 1,2,4,8) ---
        if (ndims == 1 || ndims == 2 || ndims == 4 || ndims == 8) {
          double v = reConstruct<double>(i, 8);
          if (ndims == 1) {
            dim1.AppendNull();
            dim2.AppendNull();
            dim3.AppendNull();
            dim4.Append(v);
          } else if (ndims == 2) {
            dim1.AppendNull();
            dim2.AppendNull();
            dim3.Append(v);
            dim4.AppendNull();
          } else if (ndims == 4) {
            dim1.AppendNull();
            dim2.Append(v);
            dim3.AppendNull();
            dim4.AppendNull();
          } else /*8*/ {
            dim1.Append(v);
            dim2.AppendNull();
            dim3.AppendNull();
            dim4.AppendNull();
          }
          counts.Append(n0->counts[i]);
          continue;
        }

        // otherwise descend deeper
        if (n0->nodes[i]) {
          self(self, n0->nodes[i].get(), tle_idx, i, ndims, 2);
        }
      }

    } else if (level == 2) {
      // level-2: Node4D_4x8_l1
      auto *n1 = static_cast<Node4D_4x8_l1 *>(node);
      for (size_t i = 0; i < n1->populated.size(); ++i) {
        if (!n1->populated.test(i))
          continue;

        // --- two-chunk leaf (ndims 3,5,6,9,10,12) ---
        if (ndims == 3 || ndims == 5 || ndims == 6 || ndims == 9 || ndims == 10
            || ndims == 12) {
          uint64_t x = (combined << 8) | i;
          auto [c1, c2] = reverse_combine_chunks_8b(x);
          double v1 = 0, v2 = 0, v3 = 0, v4 = 0;
          if (ndims == 3) {
            v3 = reConstruct<double>(c1, 8);
            v4 = reConstruct<double>(c2, 8);
          } else if (ndims == 5) {
            v2 = reConstruct<double>(c1, 8);
            v4 = reConstruct<double>(c2, 8);
          } else if (ndims == 6) {
            v2 = reConstruct<double>(c1, 8);
            v3 = reConstruct<double>(c2, 8);
          } else if (ndims == 9) {
            v1 = reConstruct<double>(c1, 8);
            v4 = reConstruct<double>(c2, 8);
          } else if (ndims == 10) {
            v1 = reConstruct<double>(c1, 8);
            v3 = reConstruct<double>(c2, 8);
          } else /*12*/ {
            v1 = reConstruct<double>(c1, 8);
            v2 = reConstruct<double>(c2, 8);
          }

          if (v1 == 0)
            dim1.AppendNull();
          else
            dim1.Append(v1);
          if (v2 == 0)
            dim2.AppendNull();
          else
            dim2.Append(v2);
          if (v3 == 0)
            dim3.AppendNull();
          else
            dim3.Append(v3);
          if (v4 == 0)
            dim4.AppendNull();
          else
            dim4.Append(v4);
          counts.Append(n1->counts[i]);
          continue;
        }

        // descend level-3
        uint64_t nc = (combined << 8) | i;
        if (n1->nodes[i]) {
          self(self, n1->nodes[i].get(), tle_idx, nc, ndims, 3);
        }
      }

    } else if (level == 3) {
      // level-3: Node4D_4x8_l2
      auto *n2 = static_cast<TrieNode_16 *>(node);
      for (size_t i = 0; i < n2->populated.size(); ++i) {
        if (!n2->populated.test(i))
          continue;

        // --- three-chunk leaf (ndims 7,11,13,14) ---
        if (ndims == 7 || ndims == 11 || ndims == 13 || ndims == 14) {
          uint64_t x = (combined << 8) | i;
          auto [c1, c2, c3] = reverse_combine_chunks_8b_3(x);
          double v1 = 0, v2 = 0, v3 = 0, v4 = 0;
          if (ndims == 7) {
            v2 = reConstruct<double>(c1, 8);
            v3 = reConstruct<double>(c2, 8);
            v4 = reConstruct<double>(c3, 8);
          } else if (ndims == 11) {
            v1 = reConstruct<double>(c1, 8);
            v3 = reConstruct<double>(c2, 8);
            v4 = reConstruct<double>(c3, 8);
          } else if (ndims == 13) {
            v1 = reConstruct<double>(c1, 8);
            v2 = reConstruct<double>(c2, 8);
            v4 = reConstruct<double>(c3, 8);
          } else /*14*/ {
            v1 = reConstruct<double>(c1, 8);
            v2 = reConstruct<double>(c2, 8);
            v3 = reConstruct<double>(c3, 8);
          }

          if (v1 == 0)
            dim1.AppendNull();
          else
            dim1.Append(v1);
          if (v2 == 0)
            dim2.AppendNull();
          else
            dim2.Append(v2);
          if (v3 == 0)
            dim3.AppendNull();
          else
            dim3.Append(v3);
          if (v4 == 0)
            dim4.AppendNull();
          else
            dim4.Append(v4);
          counts.Append(n2->counts[i]);
          continue;
        }

        // descend level-4
        uint64_t nc = (combined << 8) | i;
        if (n2->nodes[i]) {
          self(self, n2->nodes[i].get(), tle_idx, nc, ndims, 4);
        }
      }

    } else {
      // level-4: Node4D_4x8_l3 — final 40-bit leaf
      auto *n3 = static_cast<TrieNode_16_Level1 *>(node);
      for (size_t i = 0; i < BINS_256; ++i) {
        uint32_t cnt = n3->counts[i];
        if (cnt == 0)
          continue; // only emit non-zero counts

        uint64_t bits = (combined << 8) | i;
        auto [c1, c2, c3, c4] = reverse_combine_chunks_8b_4(bits);

        dim1.Append(reConstruct<double>(c1, 8));
        dim2.Append(reConstruct<double>(c2, 8));
        dim3.Append(reConstruct<double>(c3, 8));
        dim4.Append(reConstruct<double>(c4, 8));
        counts.Append(cnt);
      }
    }
  };

  // Drive recursion over all nonempty TLE buckets
  for (unsigned int tle_idx = 0; tle_idx < BINS_4096; ++tle_idx) {
    if (root->populated.test(tle_idx) && root->counts[tle_idx] > 0) {
      int ndims = getNumDims4D(tle_idx);
      traverse(
          traverse, root.get(), tle_idx, /*combined=*/0, ndims, /*level=*/0);
    }
  }
  // check total counts
  // uint32_t total = 0;
  // for ( int i = 0; i < BINS_4096; i++ ) {
  //   if(!root->counts[i]) continue;
  //   total += root->counts[i];
  // }
  // std::cout << "Total counts: " << total << std::endl;
  //  Finish arrays
  std::shared_ptr<arrow::Array> a1, a2, a3, a4, a5;
  auto st = dim1.Finish(&a1);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim2.Finish(&a2);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim3.Finish(&a3);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = dim4.Finish(&a4);
  if (!st.ok())
    throw std::runtime_error(st.ToString());
  st = counts.Finish(&a5);
  if (!st.ok())
    throw std::runtime_error(st.ToString());

  // Build table
  auto schema = arrow::schema({arrow::field("dim1", arrow::float64()),
                               arrow::field("dim2", arrow::float64()),
                               arrow::field("dim3", arrow::float64()),
                               arrow::field("dim4", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  return arrow::Table::Make(schema, {a1, a2, a3, a4, a5});
}


void saveArrowToParquet(const std::shared_ptr<arrow::Table> &table,
                        const std::unique_ptr<SpecialCounts> &specialCounts,
                        const std::string &output_path) {
  auto outfile = *arrow::io::FileOutputStream::Open(output_path);

  std::shared_ptr<arrow::Schema> schema_with_metadata = table->schema();
  if (specialCounts) {
    auto metadata = std::make_shared<arrow::KeyValueMetadata>();
    // Add special counts as metadata
    metadata->Append("+Inf", std::to_string(specialCounts->posInfCount));
    metadata->Append("-Inf", std::to_string(specialCounts->negInfCount));
    metadata->Append("+0", std::to_string(specialCounts->posZeroCount));
    if (specialCounts->negZeroCount > 0) {
      metadata->Append("-0", std::to_string(specialCounts->negZeroCount));
    }
    metadata->Append("NaN", std::to_string(specialCounts->nanCount));
    schema_with_metadata = schema_with_metadata->WithMetadata(metadata);

  } else {
    std::cout << "No special counts provided, skipping metadata." << std::endl;
  }

  std::shared_ptr<arrow::Table> table_with_metadata =
      arrow::Table::Make(schema_with_metadata, table->columns());

  parquet::WriterProperties::Builder builder;
  builder.created_by("AirMettle");
  auto writer_properties = builder.build();

  auto arrow_props =
      parquet::ArrowWriterProperties::Builder().store_schema()->build();

  PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(
      *table_with_metadata, arrow::default_memory_pool(), outfile, 1024,
      writer_properties, arrow_props));
}

void saveArrowToFile(const std::shared_ptr<arrow::Table> &table,
                     const std::string &output_path) {
  auto outfile = *arrow::io::FileOutputStream::Open(output_path);
  auto writer = *arrow::ipc::MakeFileWriter(outfile.get(), table->schema());
  writer->WriteTable(*table);
  writer->Close();
}

std::vector<char> readBinaryFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    throw std::runtime_error("Failed to open file: " + path);
  return std::vector<char>(std::istreambuf_iterator<char>(in), {});
}

std::shared_ptr<arrow::Table> handle1D(const BinBoundary1DList &list) {
  // Handle 1D bin boundaries

  arrow::DoubleBuilder builder_x_min;
  arrow::DoubleBuilder builder_x_max;
  arrow::UInt32Builder builder_counts;
  for (const auto &boundary : list) {
    builder_x_min.Append(boundary.getLowerBound());
    builder_x_max.Append(boundary.getUpperBound());
    builder_counts.Append(boundary.getCount());
  }

  std::shared_ptr<arrow::Array> arr_x_min, arr_x_max, arr_counts;
  auto st = builder_x_min.Finish(&arr_x_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build left boundary array: "
                             + st.ToString());
  st = builder_x_max.Finish(&arr_x_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build right boundary array: "
                             + st.ToString());
  st = builder_counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Failed to build counts array: " + st.ToString());
  auto schema = arrow::schema({arrow::field("x-min", arrow::float64()),
                               arrow::field("x-max", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});

  auto table = arrow::Table::Make(schema, {arr_x_min, arr_x_max, arr_counts});
  return table;
}

std::shared_ptr<arrow::Table> handle2D(const BinBoundary2DList &list) {
  // Handle 2D bin boundaries
  arrow::DoubleBuilder builder_x_min;
  arrow::DoubleBuilder builder_x_max;
  arrow::DoubleBuilder builder_y_min;
  arrow::DoubleBuilder builder_y_max;
  arrow::UInt32Builder builder_counts;
  for (const auto &boundary : list) {
    builder_x_min.Append(boundary.getLowerBoundX());
    builder_x_max.Append(boundary.getUpperBoundX());
    builder_y_min.Append(boundary.getLowerBoundY());
    builder_y_max.Append(boundary.getUpperBoundY());
    builder_counts.Append(boundary.getCount());
  }

  std::shared_ptr<arrow::Array> arr_x_min, arr_x_max, arr_y_min, arr_y_max,
      arr_counts;

  auto st = builder_x_min.Finish(&arr_x_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-min array: " + st.ToString());
  st = builder_x_max.Finish(&arr_x_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-max array: " + st.ToString());
  st = builder_y_min.Finish(&arr_y_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-min array: " + st.ToString());
  st = builder_y_max.Finish(&arr_y_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-max array: " + st.ToString());
  st = builder_counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Failed to build counts array: " + st.ToString());
  auto schema = arrow::schema({arrow::field("x-min", arrow::float64()),
                               arrow::field("x-max", arrow::float64()),
                               arrow::field("y-min", arrow::float64()),
                               arrow::field("y-max", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});

  auto table = arrow::Table::Make(
      schema, {arr_x_min, arr_x_max, arr_y_min, arr_y_max, arr_counts});
  return table;
}

std::shared_ptr<arrow::Table> handle3D(const BinBoundary3DList &list) {

  arrow::DoubleBuilder builder_x_min;
  arrow::DoubleBuilder builder_x_max;
  arrow::DoubleBuilder builder_y_min;
  arrow::DoubleBuilder builder_y_max;
  arrow::DoubleBuilder builder_z_min;
  arrow::DoubleBuilder builder_z_max;
  arrow::UInt32Builder builder_counts;
  // Handle 3D bin boundaries
  for (const auto &boundary : list) {
    builder_x_min.Append(boundary.getLowerBoundX());
    builder_x_max.Append(boundary.getUpperBoundX());
    builder_y_min.Append(boundary.getLowerBoundY());
    builder_y_max.Append(boundary.getUpperBoundY());
    builder_z_min.Append(boundary.getLowerBoundZ());
    builder_z_max.Append(boundary.getUpperBoundZ());
    builder_counts.Append(boundary.getCount());
  }

  std::shared_ptr<arrow::Array> arr_x_min, arr_x_max, arr_y_min, arr_y_max,
      arr_z_min, arr_z_max, arr_counts;
  auto st = builder_x_min.Finish(&arr_x_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-min array: " + st.ToString());
  st = builder_x_max.Finish(&arr_x_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-max array: " + st.ToString());
  st = builder_y_min.Finish(&arr_y_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-min array: " + st.ToString());
  st = builder_y_max.Finish(&arr_y_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-max array: " + st.ToString());
  st = builder_z_min.Finish(&arr_z_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build z-min array: " + st.ToString());
  st = builder_z_max.Finish(&arr_z_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build z-max array: " + st.ToString());
  st = builder_counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Failed to build counts array: " + st.ToString());

  auto schema = arrow::schema({arrow::field("x-min", arrow::float64()),
                               arrow::field("x-max", arrow::float64()),
                               arrow::field("y-min", arrow::float64()),
                               arrow::field("y-max", arrow::float64()),
                               arrow::field("z-min", arrow::float64()),
                               arrow::field("z-max", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});

  auto table =
      arrow::Table::Make(schema, {arr_x_min, arr_x_max, arr_y_min, arr_y_max,
                                  arr_z_min, arr_z_max, arr_counts});
  return table;
}

std::shared_ptr<arrow::Table> handle4D(const BinBoundary4DList &list) {

  arrow::DoubleBuilder builder_x_min;
  arrow::DoubleBuilder builder_x_max;
  arrow::DoubleBuilder builder_y_min;
  arrow::DoubleBuilder builder_y_max;
  arrow::DoubleBuilder builder_z_min;
  arrow::DoubleBuilder builder_z_max;
  arrow::DoubleBuilder builder_w_min;
  arrow::DoubleBuilder builder_w_max;
  arrow::UInt32Builder builder_counts;

  // Handle 4D bin boundaries
  for (const auto &boundary : list) {
    builder_x_min.Append(boundary.getLowerBoundX());
    builder_x_max.Append(boundary.getUpperBoundX());
    builder_y_min.Append(boundary.getLowerBoundY());
    builder_y_max.Append(boundary.getUpperBoundY());
    builder_z_min.Append(boundary.getLowerBoundZ());
    builder_z_max.Append(boundary.getUpperBoundZ());
    builder_w_min.Append(boundary.getLowerBoundW());
    builder_w_max.Append(boundary.getUpperBoundW());
    builder_counts.Append(boundary.getCount());
  }

  std::shared_ptr<arrow::Array> arr_x_min, arr_x_max, arr_y_min, arr_y_max,
      arr_z_min, arr_z_max, arr_w_min, arr_w_max, arr_counts;
  auto st = builder_x_min.Finish(&arr_x_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-min array: " + st.ToString());
  st = builder_x_max.Finish(&arr_x_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build x-max array: " + st.ToString());
  st = builder_y_min.Finish(&arr_y_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-min array: " + st.ToString());
  st = builder_y_max.Finish(&arr_y_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build y-max array: " + st.ToString());
  st = builder_z_min.Finish(&arr_z_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build z-min array: " + st.ToString());
  st = builder_z_max.Finish(&arr_z_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build z-max array: " + st.ToString());
  st = builder_w_min.Finish(&arr_w_min);
  if (!st.ok())
    throw std::runtime_error("Failed to build w-min array: " + st.ToString());
  st = builder_w_max.Finish(&arr_w_max);
  if (!st.ok())
    throw std::runtime_error("Failed to build w-max array: " + st.ToString());
  st = builder_counts.Finish(&arr_counts);
  if (!st.ok())
    throw std::runtime_error("Failed to build counts array: " + st.ToString());

  auto schema = arrow::schema({arrow::field("x-min", arrow::float64()),
                               arrow::field("x-max", arrow::float64()),
                               arrow::field("y-min", arrow::float64()),
                               arrow::field("y-max", arrow::float64()),
                               arrow::field("z-min", arrow::float64()),
                               arrow::field("z-max", arrow::float64()),
                               arrow::field("w-min", arrow::float64()),
                               arrow::field("w-max", arrow::float64()),
                               arrow::field("counts", arrow::uint32())});
  auto table = arrow::Table::Make(
      schema, {arr_x_min, arr_x_max, arr_y_min, arr_y_max, arr_z_min, arr_z_max,
               arr_w_min, arr_w_max, arr_counts});
  return table;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << "<input_file> [--parquet] [--output <output_file>]"
              << std::endl;
    return 1;
  }

  std::string input_path = argv[1];
  bool export_parquet = false;
  std::string output_path;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--parquet")
      export_parquet = true;
    else if (arg == "--output" && i + 1 < argc)
      output_path = argv[++i];
  }

  std::filesystem::path out_path = output_path;

  if (output_path.empty() || std::filesystem::is_directory(out_path)) {
    std::filesystem::path in(input_path);
    std::string extension = export_parquet ? ".parquet" : ".arrow";
    std::string out_filename = in.stem().string() + extension;
    out_path = output_path.empty() ? in.parent_path() / out_filename
                                   : out_path / out_filename;
  }

  output_path = out_path.string();

  std::vector<char> buffer;
  try {
    buffer = readBinaryFile(input_path);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  try {
    std::shared_ptr<arrow::Table> table;

    auto binBoundaryQuery = BinBoundary(buffer);
    auto binBoundaryRes = binBoundaryQuery.generateBinBoundaries();
    auto binBoundaries = binBoundaryRes.getBoundaries();
    auto specialCounts = binBoundaryRes.getSpecialCounts();

    if (std::holds_alternative<BinBoundary1DList>(*binBoundaries)) {
      const auto &list = std::get<BinBoundary1DList>(*binBoundaries);
      table = handle1D(list);
    } else if (std::holds_alternative<BinBoundary2DList>(*binBoundaries)) {
      const auto &list = std::get<BinBoundary2DList>(*binBoundaries);
      table = handle2D(list);
    } else if (std::holds_alternative<BinBoundary3DList>(*binBoundaries)) {
      const auto &list = std::get<BinBoundary3DList>(*binBoundaries);
      table = handle3D(list);
    } else if (std::holds_alternative<BinBoundary4DList>(*binBoundaries)) {
      const auto &list = std::get<BinBoundary4DList>(*binBoundaries);
      table = handle4D(list);
    } else {
      throw std::runtime_error("Unknown BinBoundary variant type");
    }

    export_parquet ? saveArrowToParquet(table, specialCounts, output_path)
                   : saveArrowToFile(table, output_path);
    std::cout << "Exported successfully to: " << output_path << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Export failed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}