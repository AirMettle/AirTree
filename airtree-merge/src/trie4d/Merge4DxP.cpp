// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <vector>

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/merge/trie4d/Merge4DxP.hpp>
#include <airtree/merge/MergeStrategy.hpp>
#include <airtree/merge/Logger.hpp>

using namespace airtree::merge;
using namespace airtree::merge::dim4;

std::vector<char> Merge4DxP::merge(const std::vector<char> &buffer1,
                                   const std::vector<char> &buffer2) {
  auto header1 = airtree::core::common::deserializeHeader(buffer1);
  auto header2 = airtree::core::common::deserializeHeader(buffer2);
  size_t offset1 = header1.header_length;
  size_t offset2 = header2.header_length;
  auto mergedHeader = mergeHeaders(header1, header2);
  std::vector<char> mergedBuffer;
  airtree::core::common::serializeHeader(mergedHeader, mergedBuffer);
  size_t header_end = mergedBuffer.size();

  // Deserialize the root nodes.
  PopulatedBins<BINS_4096> pop0_1, pop0_2;
  auto mergedRoot = Root_merge<TLE_4D_4x10, BINS_4096>(
      buffer1, buffer2, offset1, offset2, pop0_1, pop0_2);
  serialize_4DxP(mergedRoot.get(), mergedBuffer, false);
  mergedRoot.reset();

  // Merge level1 children based on the original populated flags.
  int level = 0;
  int nDims = 0;
  for (size_t i = 0; i < BINS_4096; i++) {
    nDims = getNumDims4D(i);
    if (nDims == 0)
      continue;
    if (nDims == 1 || nDims == 2 || nDims == 4 || nDims == 8)
      level = 1;
    if (nDims == 3 || nDims == 5 || nDims == 6 || nDims == 9 || nDims == 10
        || nDims == 12)
      level = 2;
    if (nDims == 7 || nDims == 11 || nDims == 13 || nDims == 14)
      level = 3;
    if (nDims == 15)
      level = 4;

    if (pop0_1.test(i) && !pop0_2.test(i)) {
      auto temp_node_level1 =
          deserialize_4DxP_l0(buffer1, offset1, level, false);
      serialize_4DxP_l0(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_1024; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_4DxP_l1(buffer1, offset1, level, false);
          auto pop0_level2 = temp_node_level2->populated;
          serialize_4DxP_l1(temp_node_level2.get(), mergedBuffer, false);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_1024; k++) {
            if (pop0_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer1, offset1, level, false);
              auto pop0_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop0_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer1, offset1, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            }
          }
        }
      }
    } else if (pop0_2.test(i) && !pop0_1.test(i)) {
      auto temp_node_level1 =
          deserialize_4DxP_l0(buffer2, offset2, level, false);
      serialize_4DxP_l0(temp_node_level1.get(), mergedBuffer, false);
      auto pop0_level1 = temp_node_level1->populated;
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_1024; j++) {
        if (pop0_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_4DxP_l1(buffer2, offset2, level, false);
          auto pop0_level2 = temp_node_level2->populated;
          serialize_4DxP_l1(temp_node_level2.get(), mergedBuffer, false);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_1024; k++) {
            if (pop0_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer2, offset2, level, false);
              auto pop0_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop0_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer2, offset2, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            }
          }
        }
      }
    } else if (pop0_1.test(i) && pop0_2.test(i)) {
      auto node1_level1 = deserialize_4DxP_l0(buffer1, offset1, level, false);
      auto node2_level1 = deserialize_4DxP_l0(buffer2, offset2, level, false);
      auto pop1_level1 = node1_level1->populated;
      auto pop2_level1 = node2_level1->populated;
      auto temp_node_level1 =
          mergeNode4D_4x10_l0(std::move(node1_level1), std::move(node2_level1));
      serialize_4DxP_l0(temp_node_level1.get(), mergedBuffer, false);
      temp_node_level1.reset();
      if (level == 1)
        continue;
      for (size_t j = 0; j < BINS_1024; j++) {
        if (pop1_level1.test(j) && !pop2_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_4DxP_l1(buffer1, offset1, level, false);
          auto pop1_level2 = temp_node_level2->populated;
          serialize_4DxP_l1(temp_node_level2.get(), mergedBuffer, false);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_1024; k++) {
            if (pop1_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer1, offset1, level, false);
              auto pop1_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop1_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer1, offset1, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            }
          }
        } else if (pop2_level1.test(j) && !pop1_level1.test(j)) {
          auto temp_node_level2 =
              deserialize_4DxP_l1(buffer2, offset2, level, false);
          auto pop2_level2 = temp_node_level2->populated;
          serialize_4DxP_l1(temp_node_level2.get(), mergedBuffer, false);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_1024; k++) {
            if (pop2_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer2, offset2, level, false);
              auto pop2_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop2_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer2, offset2, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            }
          }
        } else if (pop1_level1.test(j) && pop2_level1.test(j)) {
          auto node1_level2 =
              deserialize_4DxP_l1(buffer1, offset1, level, false);
          auto node2_level2 =
              deserialize_4DxP_l1(buffer2, offset2, level, false);
          auto pop1_level2 = node1_level2->populated;
          auto pop2_level2 = node2_level2->populated;
          auto temp_node_level2 = mergeNode4D_4x10_l1(
              std::move(node1_level2), std::move(node2_level2));
          serialize_4DxP_l1(temp_node_level2.get(), mergedBuffer, false);
          temp_node_level2.reset();
          if (level == 2)
            continue;
          for (size_t k = 0; k < BINS_1024; k++) {
            if (pop1_level2.test(k) && !pop2_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer1, offset1, level, false);
              auto pop1_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop1_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer1, offset1, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            } else if (pop2_level2.test(k) && !pop1_level2.test(k)) {
              auto temp_node_level3 =
                  deserialize_4DxP_l2(buffer2, offset2, level, false);
              auto pop2_level3 = temp_node_level3->populated;
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop2_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer2, offset2, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            } else if (pop1_level2.test(k) && pop2_level2.test(k)) {
              auto node1_level3 =
                  deserialize_4DxP_l2(buffer1, offset1, level, false);
              auto node2_level3 =
                  deserialize_4DxP_l2(buffer2, offset2, level, false);
              auto pop1_level3 = node1_level3->populated;
              auto pop2_level3 = node2_level3->populated;
              auto temp_node_level3 = mergeNode4D_4x10_l2(
                  std::move(node1_level3), std::move(node2_level3));
              serialize_4DxP_l2(temp_node_level3.get(), mergedBuffer, false);
              temp_node_level3.reset();
              if (level == 3)
                continue;
              for (size_t l = 0; l < BINS_1024; l++) {
                if (pop1_level3.test(l) && !pop2_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer1, offset1, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                } else if (pop2_level3.test(l) && !pop1_level3.test(l)) {
                  auto temp_node_level4 =
                      deserialize_4DxP_l3(buffer2, offset2, level);
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                } else if (pop1_level3.test(l) && pop2_level3.test(l)) {
                  auto temp_node_level4 = mergeNode4D_4x10_l3(
                      deserialize_4DxP_l3(buffer1, offset1, level),
                      deserialize_4DxP_l3(buffer2, offset2, level));
                  serialize_4DxP_l3(temp_node_level4.get(), mergedBuffer);
                  temp_node_level4.reset();
                }
              }
            }
          }
        }
      }
    }
  }
  add_EOF(mergedBuffer);
  airtree::core::common::finalizeHeader(mergedBuffer, mergedBuffer.size() - header_end);
  SPDLOG_LOGGER_DEBUG(
      logger(), "Serialized merged trie (all levels) successfully");
  return mergedBuffer;
}

std::unique_ptr<Node4D_4x10_l0>
Merge4DxP::mergeNode4D_4x10_l0(std::unique_ptr<Node4D_4x10_l0> node1,
                               std::unique_ptr<Node4D_4x10_l0> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  node2->forEachMut([&](size_t i, uint32_t &count2, auto &child2) {
    node1->populated.set(i);
    node1->countRef(i) += count2;
    auto &child1 = node1->childRef(i);
    child1 = child1 ? mergeNode4D_4x10_l1(std::move(child1), std::move(child2)) : std::move(child2);
  });
  return node1;
}

std::unique_ptr<Node4D_4x10_l1>
Merge4DxP::mergeNode4D_4x10_l1(std::unique_ptr<Node4D_4x10_l1> node1,
                               std::unique_ptr<Node4D_4x10_l1> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  node2->forEachMut([&](size_t i, uint32_t &count2, auto &child2) {
    node1->populated.set(i);
    node1->countRef(i) += count2;
    auto &child1 = node1->childRef(i);
    child1 = child1 ? mergeNode4D_4x10_l2(std::move(child1), std::move(child2)) : std::move(child2);
  });
  return node1;
}

std::unique_ptr<Node4D_4x10_l2>
Merge4DxP::mergeNode4D_4x10_l2(std::unique_ptr<Node4D_4x10_l2> node1,
                               std::unique_ptr<Node4D_4x10_l2> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  node2->forEachMut([&](size_t i, uint32_t &count2, auto &child2) {
    node1->populated.set(i);
    node1->countRef(i) += count2;
    auto &child1 = node1->childRef(i);
    child1 = child1 ? mergeNode4D_4x10_l3(std::move(child1), std::move(child2)) : std::move(child2);
  });
  return node1;
}

std::unique_ptr<Node4D_4x10_l3>
Merge4DxP::mergeNode4D_4x10_l3(std::unique_ptr<Node4D_4x10_l3> node1,
                               std::unique_ptr<Node4D_4x10_l3> node2) {
  if (!node1)
    return node2;
  if (!node2)
    return node1;
  node2->forEach([&](size_t i, uint32_t count2) {
    node1->populated.set(i);
    node1->countRef(i) += count2;
  });
  return node1;
}
