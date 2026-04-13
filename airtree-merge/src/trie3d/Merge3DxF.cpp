#include <vector>

#include <airtree/merge/trie3d/Merge3DxF.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim3;

std::vector<char> Merge3DxF::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  size_t offset1 = 0, offset2 = 0;
  std::vector<char> mergedBuffer;

  // Merge headers.
  trie_header header1 = deserializeTrieHeader(buffer1, offset1);
  trie_header header2 = deserializeTrieHeader(buffer2, offset2);
  trie_header mergedHeader = mergeHeaders(header1, header2);
  serializeTrieHeader(mergedHeader, mergedBuffer);

  // Deserialize the root nodes.
  std::bitset<BINS_512> pop0_1, pop0_2;
  auto mergedRoot = Root_merge<TLE_3D_888, BINS_512>(
      buffer1, buffer2, offset1, offset2, pop0_1, pop0_2);
  serialize_3DxF(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();

  // Merge level1 children based on the original populated flags.
  int level = 0;
  int nDims = 0;
  for (size_t i = 0; i < BINS_512; i++) {
    nDims = getNumDims3D(i);
    if (nDims == 0)
      continue;
    if (nDims == 1 || nDims == 2 || nDims == 4)
      level = 1;
    if (nDims == 3 || nDims == 5 || nDims == 6)
      level = 2;
    if (nDims == 7)
      level = 3;
    if (pop0_1.test(i) && !pop0_2.test(i)) {
      auto temp_node_level1 =
          deserialize_3DxF_l0(buffer1, offset1, level, false);
      serialize_3DxF_l0(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_3DxF_l1(buffer1, offset1, level, false);
          auto pop0_level2 = temp_node_level2->populated;
          serializeTrieNode_16_ND(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_256; k++) {
            if (pop0_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer1, offset1, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            }
          }
        }
      }
    } else if (pop0_2.test(i) && !pop0_1.test(i)) {
      auto temp_node_level1 =
          deserialize_3DxF_l0(buffer2, offset2, level, false);
      serialize_3DxF_l0(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_3DxF_l1(buffer2, offset2, level, false);
          auto pop0_level2 = temp_node_level2->populated;
          serializeTrieNode_16_ND(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_256; k++) {
            if (pop0_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer2, offset2, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            }
          }
        }
      }
    } else if (pop0_1.test(i) && pop0_2.test(i)) {
      auto node1_level1 = deserialize_3DxF_l0(buffer1, offset1, level, false);
      auto node2_level1 = deserialize_3DxF_l0(buffer2, offset2, level, false);
      auto pop1_level1 = node1_level1->populated;
      auto pop2_level1 = node2_level1->populated;
      auto temp_node_level1 =
          mergeNode3D_888_l0(std::move(node1_level1), std::move(node2_level1));
      serialize_3DxF_l0(temp_node_level1.get(), mergedBuffer, false);
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_256; j++) {
        if (pop1_level1.test(j) && !pop2_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_3DxF_l1(buffer1, offset1, level, false);
          auto pop1_level2 = temp_node_level2->populated;
          serializeTrieNode_16_ND(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_256; k++) {
            if (pop1_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer1, offset1, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            }
          }
        } else if (pop2_level1.test(j) && !pop1_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_3DxF_l1(buffer2, offset2, level, false);
          auto pop2_level2 = temp_node_level2->populated;
          serializeTrieNode_16_ND(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_256; k++) {
            if (pop2_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer2, offset2, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            }
          }
        } else if (pop1_level1.test(j) && pop2_level1.test(j)) {
          auto node1_level2 =
              deserialize_3DxF_l1(buffer1, offset1, level, false);
          auto node2_level2 =
              deserialize_3DxF_l1(buffer2, offset2, level, false);
          auto pop1_level2 = node1_level2->populated;
          auto pop2_level2 = node2_level2->populated;
          auto temp_node_level2 =
              mergeTrieNode16(std::move(node1_level2), std::move(node2_level2));
          serializeTrieNode_16_ND(temp_node_level2.get(), mergedBuffer);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_256; k++) {
            if (pop1_level2.test(k) && !pop2_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer1, offset1, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            } else if (pop2_level2.test(k) && !pop1_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_3DxF_l2(buffer2, offset2, level);
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            } else if (pop1_level2.test(k) && pop2_level2.test(k)) {
              auto temp_node_level3 = mergeTrieNode16Level1(
                  std::move(deserialize_3DxF_l2(buffer1, offset1, level)),
                  std::move(deserialize_3DxF_l2(buffer2, offset2, level)));
              serialize_1DxF_l1(temp_node_level3.get(), mergedBuffer);
              temp_node_level3.reset();
            }
          }
        }
      }
    }
  }
  add_EOF(mergedBuffer);
  SPDLOG_LOGGER_INFO(
      logger(), "Serialized merged trie (all levels) successfully");
  return mergedBuffer;
}

std::unique_ptr<Node3D_888_l0>
Merge3DxF::mergeNode3D_888_l0(std::unique_ptr<Node3D_888_l0> node1,
                              std::unique_ptr<Node3D_888_l0> node2) {
  SPDLOG_LOGGER_INFO(logger(), "Entering mergeNode3D_888_l0");
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  for (size_t i = 0; i < BINS_256; ++i) {
    SPDLOG_LOGGER_INFO(logger(), "Merging 3DxF l0 index {}", i);
    node1->counts[i] += node2->counts[i];
    if (node2->populated.test(i)) {
      if (!node1->populated.test(i)) {
        SPDLOG_LOGGER_INFO(logger(), "3DxF l0: adopting child at index {}", i);
        node1->populated.set(i);
        node1->nodes[i] = std::move(node2->nodes[i]);
      } else {
        SPDLOG_LOGGER_INFO(
            logger(), "3DxF l0: merging children at index {}", i);
        // Here we use the existing merge for TrieNode_16.
        node1->nodes[i] = mergeTrieNode16(
            std::move(node1->nodes[i]), std::move(node2->nodes[i]));
      }
    }
  }
  return node1;
}
