// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <airtree/core/utils/Utils.hpp>

const std::string dim1_file_path = "../../tests/TestData/dim1_vx.bin";
const std::string dim2_file_path = "../../tests/TestData/dim2_vy.bin";
const std::string dim3_file_path = "../../tests/TestData/dim3_vz.bin";
const std::string dim4_file_path = "../../tests/TestData/dim4_rho.bin";


const std::vector<double> dim1_data = readBinaryFile(dim1_file_path);
const std::vector<double> dim2_data = readBinaryFile(dim2_file_path);
const std::vector<double> dim3_data = readBinaryFile(dim3_file_path);
const std::vector<double> dim4_data = readBinaryFile(dim4_file_path);

const FPHArray array0 =
    buildFPHArray(dim1_data.data(), static_cast<int>(dim1_data.size()));
const FPHArray array1 =
    buildFPHArray(dim2_data.data(), static_cast<int>(dim2_data.size()));
const FPHArray array2 =
    buildFPHArray(dim3_data.data(), static_cast<int>(dim3_data.size()));
const FPHArray array3 =
    buildFPHArray(dim4_data.data(), static_cast<int>(dim4_data.size()));


class SerializationTest : public ::testing::Test {
protected:
  void SetUp() override {
    // The SetUp method is currently empty because there is no special
    // initialization required for this specific test suite.
  }


  void TearDown() override {
    // The TearDown method is empty because there are no resources to clean up
    // after the tests in this suite.
  }
};


TEST_F(SerializationTest, VerifiesSerializationAndDeserialization) {
  uint32_t counts[5] = {255, 65535, 16777215, 12343, 12345};
  std::vector<char> buffer = serializeCounts(counts, 5);
  size_t offset = 0;
  std::vector<uint32_t> deserializedCounts =
      deserializeCounts(buffer, offset, 5);
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(counts[i], deserializedCounts[i]);
  }
}

bool compareNodesl3(const std::unique_ptr<Node4D_4x10_l3> &node1,
                    const std::unique_ptr<Node4D_4x10_l3> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
  }
  return true;
}

bool compareNodesl2(const std::unique_ptr<Node4D_4x10_l2> &node1,
                    const std::unique_ptr<Node4D_4x10_l2> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }

    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl3(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodesl1(const std::unique_ptr<Node4D_4x10_l1> &node1,
                    const std::unique_ptr<Node4D_4x10_l1> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl2(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodesl0(const std::unique_ptr<Node4D_4x10_l0> &node1,
                    const std::unique_ptr<Node4D_4x10_l0> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl1(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

TEST_F(SerializationTest, VerifiesDeserializationConstructsCorrectTrie_4x10) {

  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLE_4D_4x10> root = execCreateAndInsert_4D_4x10(
      array0, array1, array2, array3, curr_trie_size, specialCounts, true);


  std::vector<char> buffer =
      execSerialize_4D_4x10(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_4DxP(buffer);

  std::unique_ptr<TLE_4D_4x10> deserializedRoot = std::move(result.first);

  for (size_t i = 0; i < BINS_4096; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->counts[i], deserializedRoot->counts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodesl0(root->nodes[i], deserializedRoot->nodes[i]), true);
    }
  }
}

bool compareNodesl3(const std::unique_ptr<TrieNode_16_Level1> &node1,
                    const std::unique_ptr<TrieNode_16_Level1> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
  }
  return true;
}

bool compareNodesl2(const std::unique_ptr<TrieNode_16> &node1,
                    const std::unique_ptr<TrieNode_16> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }

    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl3(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodesl1(const std::unique_ptr<Node4D_4x8_l1> &node1,
                    const std::unique_ptr<Node4D_4x8_l1> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl2(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodesl0(const std::unique_ptr<Node4D_4x8_l0> &node1,
                    const std::unique_ptr<Node4D_4x8_l0> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodesl1(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

TEST_F(SerializationTest, VerifySerializationAndDeserialization_4x8) {
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLE_4D_4x8> root = execCreateAndInsert_4D_4x8(
      array0, array1, array2, array3, curr_trie_size, specialCounts, true);
  std::vector<char> buffer =
      execSerialize_4D_4x8(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_4DxF(buffer);

  std::unique_ptr<TLE_4D_4x8> deserializedRoot = std::move(result.first);

  for (size_t i = 0; i < BINS_4096; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->counts[i], deserializedRoot->counts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodesl0(root->nodes[i], deserializedRoot->nodes[i]), true);
    }
  }
}

bool compareNodes3Dl2(const std::unique_ptr<Node3D_3x10_l2> &node1,
                      const std::unique_ptr<Node3D_3x10_l2> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
  }
  return true;
}

bool compareNodes3Dl1(const std::unique_ptr<Node3D_3x10_l1> &node1,
                      const std::unique_ptr<Node3D_3x10_l1> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodes3Dl2(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodes3Dl0(const std::unique_ptr<Node3D_3x10_l0> &node1,
                      const std::unique_ptr<Node3D_3x10_l0> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodes3Dl1(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

TEST_F(SerializationTest, VerifiesDeserializationConstructsCorrectTrie_3x10) {
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLE_3D_3x10> root = execCreateAndInsert_3D_3x10(
      array0, array1, array2, curr_trie_size, specialCounts, true);
  std::vector<char> buffer =
      execSerialize_3D_3x10(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_3DxP(buffer);

  std::unique_ptr<TLE_3D_3x10> deserializedRoot = std::move(result.first);

  for (size_t i = 0; i < BINS_512; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->counts[i], deserializedRoot->counts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodes3Dl0(root->nodes[i], deserializedRoot->nodes[i]), true);
    }
  }
}

bool compareNodes3Dl2(const std::unique_ptr<TrieNode_16_Level1> &node1,
                      const std::unique_ptr<TrieNode_16_Level1> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
  }
  return true;
}

bool compareNodes3Dl1(const std::unique_ptr<TrieNode_16> &node1,
                      const std::unique_ptr<TrieNode_16> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodes3Dl2(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

bool compareNodes3Dl0(const std::unique_ptr<Node3D_888_l0> &node1,
                      const std::unique_ptr<Node3D_888_l0> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      EXPECT_EQ(compareNodes3Dl1(node1->nodes[i], node2->nodes[i]), true);
    }
  }
  return true;
}

TEST_F(SerializationTest, VerifiesDeserializationConstructsCorrectTrie_3x8) {
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLE_3D_888> root = execCreateAndInsert_3D_888(
      array0, array1, array2, curr_trie_size, specialCounts, true);
  std::vector<char> buffer =
      execSerialize_3D_888(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_3DxF(buffer);

  std::unique_ptr<TLE_3D_888> deserializedRoot = std::move(result.first);

  for (size_t i = 0; i < BINS_512; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->counts[i], deserializedRoot->counts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodes3Dl0(root->nodes[i], deserializedRoot->nodes[i]), true);
    }
  }
}

bool compareNodes2Dl1(const std::unique_ptr<TrieNode_16_Level1> &node1,
                      const std::unique_ptr<TrieNode_16_Level1> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2Dl1: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
  }
  return true;
}

bool compareNodes2Dl0(const std::unique_ptr<TrieNode_16> &node1,
                      const std::unique_ptr<TrieNode_16> &node2) {
  for (size_t i = 0; i < BINS_256; i++) {

    if (node1->populated[i] != node2->populated[i]) {
      std::cout << "compareNodes2Dl0: Mismatch at index " << i
                << ": node1->populated[" << i << "] = " << node1->populated[i]
                << ", node2->populated[" << i << "] = " << node2->populated[i]
                << std::endl;
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2Dl0: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      if (!compareNodes2Dl1(node1->nodes[i], node2->nodes[i])) {
        std::cout << "compareNodes2Dl0: Mismatch in child nodes at index " << i
                  << std::endl;
        return false;
      }
    } else if (node1->nodes[i] != node2->nodes[i]) {
      std::cout << "compareNodes2Dl0: One of the nodes is null at index " << i
                << ": node1->nodes[" << i
                << "] = " << (node1->nodes[i] ? "not null" : "null")
                << ", node2->nodes[" << i
                << "] = " << (node2->nodes[i] ? "not null" : "null")
                << std::endl;
      return false;
    }
  }
  return true;
}


TEST_F(SerializationTest, VerifiesDeserializationConstructsCorrectTrie_2x8) {
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLETrieNode_2D> root = execCreateAndInsert_2D(
      array0, array1, curr_trie_size, specialCounts, true);
  std::vector<char> buffer =
      execSerialize_2D(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_2DxF(buffer);

  std::unique_ptr<TLETrieNode_2D> deserializedRoot = std::move(result.first);

  if (deserializedRoot == nullptr) {
    std::cout << "Deserialized root is null" << std::endl;
  }

  for (size_t i = 0; i < BINS_64; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->TLEcounts[i], deserializedRoot->TLEcounts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodes2Dl0(root->nodes[i], deserializedRoot->nodes[i]), true);
    }
  }
}

bool compareNodes2D866l2(const std::unique_ptr<TrieNode_20_Level2> &node1,
                         const std::unique_ptr<TrieNode_20_Level2> &node2) {
  for (size_t i = 0; i < BINS_64; i++) {
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes4D5x8l4: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
  }
  return true;
}

bool compareNodes2D866l1(const std::unique_ptr<TrieNode_20_Level1> &node1,
                         const std::unique_ptr<TrieNode_20_Level1> &node2) {
  for (size_t i = 0; i < BINS_64; i++) {
    if (node1->populated[i] != node2->populated[i]) {
      std::cout << "compareNodes2D866l1: Mismatch at index " << i
                << ": node1->populated[" << i << "] = " << node1->populated[i]
                << ", node2->populated[" << i << "] = " << node2->populated[i]
                << std::endl;
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2D866l1: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      if (!compareNodes2D866l2(node1->nodes[i], node2->nodes[i])) {
        std::cout << "compareNodes2D866l1: Mismatch in child "
                  << "nodes at index " << i << std::endl;
        return false;
      }
    } else if (node1->nodes[i] != node2->nodes[i]) {
      std::cout << "compareNodes2D866l1: One of the nodes is "
                << "null at index " << i << ": node1->nodes[" << i
                << "] = " << (node1->nodes[i] ? "not null" : "null")
                << ", node2->nodes[" << i
                << "] = " << (node2->nodes[i] ? "not null" : "null")
                << std::endl;
      return false;
    }
  }
  return true;
}

bool compareNodes2D866l0(const std::unique_ptr<TrieNode_20> &node1,
                         const std::unique_ptr<TrieNode_20> &node2) {

  for (size_t i = 0; i < BINS_256; i++) {

    if (node1->populated[i] != node2->populated[i]) {
      std::cout << "compareNodes2D8l660: Mismatch at index " << i
                << ": node1->populated[" << i << "] = " << node1->populated[i]
                << ", node2->populated[" << i << "] = " << node2->populated[i]
                << std::endl;
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2D8l660: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      if (!compareNodes2D866l1(node1->nodes[i], node2->nodes[i])) {
        std::cout << "compareNodes2D8l660: Mismatch in child "
                  << "nodes at index " << i << std::endl;
        return false;
      }
    } else if (node1->nodes[i] != node2->nodes[i]) {
      std::cout << "compareNodes2D8l660: One of the nodes is "
                << "null at index " << i << ": node1->nodes[" << i
                << "] = " << (node1->nodes[i] ? "not null" : "null")
                << ", node2->nodes[" << i
                << "] = " << (node2->nodes[i] ? "not null" : "null")
                << std::endl;
      return false;
    }
  }
  return true;
}

bool compareNodes2D2x10l1(const std::unique_ptr<TrieNode_2D_10_Level1> &node1,
                          const std::unique_ptr<TrieNode_2D_10_Level1> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2D2x10l1: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
  }
  return true;
}

bool compareNodes2D2x10l0(const std::unique_ptr<TrieNode_2D_10> &node1,
                          const std::unique_ptr<TrieNode_2D_10> &node2) {
  for (size_t i = 0; i < BINS_1024; i++) {

    if (node1->populated[i] != node2->populated[i]) {
      std::cout << "compareNodes2D2x10l0: Mismatch at index " << i
                << ": node1->populated[" << i << "] = " << node1->populated[i]
                << ", node2->populated[" << i << "] = " << node2->populated[i]
                << std::endl;
      return false;
    }
    if (node1->counts[i] != node2->counts[i]) {
      std::cout << "compareNodes2D2x10l0: Mismatch at index " << i
                << ": node1->counts[" << i << "] = " << node1->counts[i]
                << ", node2->counts[" << i << "] = " << node2->counts[i]
                << std::endl;
      return false;
    }
    if (node1->nodes[i] != nullptr && node2->nodes[i] != nullptr) {
      if (!compareNodes2D2x10l1(node1->nodes[i], node2->nodes[i])) {
        std::cout << "compareNodes2D2x10l0: Mismatch in child nodes at index "
                  << i << std::endl;
        return false;
      }
    } else if (node1->nodes[i] != node2->nodes[i]) {
      std::cout << "compareNodes2D2x10l0: One of the nodes is null at index "
                << i << ": node1->nodes[" << i
                << "] = " << (node1->nodes[i] ? "not null" : "null")
                << ", node2->nodes[" << i
                << "] = " << (node2->nodes[i] ? "not null" : "null")
                << std::endl;
      return false;
    }
  }
  return true;
}


TEST_F(SerializationTest, VerifiesDeserializationConstructsCorrectTrie_2x10) {
  uint64_t curr_trie_size = 0;
  std::unique_ptr<SpecialCounts> specialCounts =
      std::make_unique<SpecialCounts>();
  std::unique_ptr<TLEoption3_2D> root = execCreateAndInsert_2D_2x10(
      array0, array1, curr_trie_size, specialCounts, true);
  std::vector<char> buffer =
      execSerialize_2D_2x10(root.get(), curr_trie_size, specialCounts, true);

  auto result = processBuffer_2DxP(buffer);

  std::unique_ptr<TLEoption3_2D> deserializedRoot = std::move(result.first);

  if (deserializedRoot == nullptr) {
    std::cout << "Deserialized root is null" << std::endl;
  }

  for (size_t i = 0; i < BINS_64; i++) {
    EXPECT_EQ(root->populated[i], deserializedRoot->populated[i]);
    EXPECT_EQ(root->counts[i], deserializedRoot->counts[i]);

    if (root->nodes[i] != nullptr && deserializedRoot->nodes[i] != nullptr) {
      EXPECT_EQ(
          compareNodes2D2x10l0(root->nodes[i], deserializedRoot->nodes[i]),
          true);
    }
  }
}
