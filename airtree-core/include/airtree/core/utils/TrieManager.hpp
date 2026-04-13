#ifndef TRIE_MANAGER_H
#define TRIE_MANAGER_H

#include <airtree/core/AirTreeCore_internal.hpp>
#include <cstdint>
#include <memory>
#include <iostream>

enum class DistributionMethod {
  SINGLE, // Insert all count into a specified child bucket.
  EVEN,   // Evenly distribute count among all child buckets.
  RANDOM  // Distribute count using randomly generated weights.
};

enum class Quadrant {
  POS_POS, // 0,0: positive sign and positive exponent; level0 indices 0 to 63.
  POS_NEG, // 0,1: positive sign and negative exponent; indices 64 to 127.
  NEG_POS, // 1,0: negative sign and positive exponent; indices 128 to 191.
  NEG_NEG  // 1,1: negative sign and negative exponent; indices 192 to 255.
};

enum class Level0Distribution {
  EVEN,       // Uniform distribution (each bucket gets equal count).
  LEFT_SKEW,  // Left skewed: buckets with lower indices get higher weights.
  RIGHT_SKEW, // Right skewed: buckets with higher indices get higher weights.
  NORMAL_SKEW // Normal (bell-curve) distribution centered in the middle.
};

struct QuadrantRegion {
  Quadrant quadrant;
  uint32_t count;
  Level0Distribution level0Dist = Level0Distribution::EVEN; // Default to EVEN.
};


struct Roaring20Bucket {
  unsigned int level0Index;
  // For each populated Level1 bucket, store the Level1 index and its Level2
  // counts.
  std::vector<std::pair<unsigned int, std::vector<uint32_t>>> level1Buckets;
};

class TrieManager {
public:
  TrieManager();


  // Inserts a value into the two-level 13Colonies(1DxT) trie structure.
  //
  // Parameters:
  //   index0       - Index at level 0 of the trie.
  //   count        - Number of elements to insert.
  //   method       - Distribution method used for insertion.
  //   targetChild  - (Optional) Specific child index at level 2 where the
  //   insertion should occur.
  //                  Defaults to 0 if not specified.
  void insert1DxT(unsigned int index, uint32_t count,
                  DistributionMethod method = DistributionMethod::SINGLE,
                  unsigned int targetChild = 0);


  template <typename NODE_TYPE> void serializeTrie(std::vector<char> &buffer);
  // Inserts a value into the two-level Apoll16(1DxF) trie structure.
  //
  // Parameters:
  //   index0       - Index at level 0 of the trie.
  //   count        - Number of elements to insert.
  //   method       - Distribution method used for insertion.
  //   targetChild  - (Optional) Specific child index at level 1 where the
  //   insertion should occur.
  //                  Defaults to 0 if not specified.
  void insert1DxF(unsigned int index, uint32_t count,
                  DistributionMethod method = DistributionMethod::SINGLE,
                  unsigned int targetChild = 0);

  // Inserts a value into the three-level Roaring20 (1DxP) trie structure.
  //
  // Parameters:
  //   index0       - Index at level 0 of the trie.
  //   index1       - Index at level 1 of the trie.
  //   count        - Number of elements to insert.
  //   method       - Distribution method used for insertion.
  //   targetChild  - (Optional) Specific child index at level 2 where the
  //   insertion should occur.
  //                  Defaults to 0 if not specified.
  void insert1DxP(unsigned int index0, unsigned int index1, uint32_t count,
                  DistributionMethod method, unsigned int targetChild = 0);

  /**
   * Inserts a value into the two-level Roaring20 (2DxP) trie structure.
   * Parameters:
   *   index0       - Index at level 0 of the trie.
   *   index1       - Index at level 1 of the trie.
   *   count        - Number of elements to insert.
   *   method       - Distribution method used for insertion.
   *   targetChild  - (Optional) Specific child index at level 2 where the
   *   insertion should occur.
   *                  Defaults to 0 if not specified.
   */
  void insert2DxP(uint32_t combinedTLE, uint32_t combined20Bits,
                  uint32_t count);

  /**
   * Inserts a value into the three-dimensional (3DxP) trie structure.
   * Parameters:
   *   combinedTLE   - Combined TLE value (9 bits for 3 dimensions)
   *   combined30Bits - Combined internal representation (30 bits max)
   *   count         - Number of elements to insert.
   */
  void insert3DxP(uint32_t combinedTLE, uint64_t combined30Bits,
                  uint32_t count);

  // Distributes the 'count' across the 64 Level0 buckets
  // in the specified quadrant according to the chosen Level0Distribution
  // pattern, and then calls the corresponding lower-level insertion function
  // depending on precisionBits.
  //   - precisionBits 13: calls insert1DxT (1DxT configuration)
  //   - precisionBits 16: calls insert1DxF (1DxF configuration)
  //   - precisionBits 20: calls insert1DxP (1DxP configuration)
  // 'method' is used for the level-1 distribution.
  void
  insertByQuadrant(Quadrant quadrant, uint32_t count, DistributionMethod method,
                   int precisionBits,
                   Level0Distribution level0Dist = Level0Distribution::EVEN);

  // Overload: Accepts multiple (Quadrant, count) pairs.
  void insertByQuadrant(const std::vector<QuadrantRegion> &regions,
                        DistributionMethod method, int precisionBits);


  // Accessor methods to retrieve populated buckets.
  std::vector<std::pair<unsigned int, std::vector<uint32_t>>>
  getPopulatedBuckets1DxT() const;
  std::vector<std::pair<unsigned int, std::vector<uint32_t>>>
  getPopulatedBuckets1DxF() const;
  std::vector<Roaring20Bucket> getPopulatedBuckets1DxP() const;


  // Retrieve pointers to the root nodes so they can be passed to other
  // functions.
  const TrieNode_13 &getRoot1DxT() const;

  uint64_t getTrieSize1DxT() const;

  const TrieNode_16 &getRoot1DxF() const;

  uint64_t getTrieSize1DxF() const;

  const TrieNode_20 &getRoot1DxP() const;

  uint64_t getTrieSize1DxP() const;


  const TLEoption3_2D &getRoot2DxP() const;

  uint64_t getTrieSize2DxP() const;

  const TLE_3D_3x10 &getRoot3DxP() const;

  uint64_t getTrieSize3DxP() const;

  // Print functions for each trie.
  void print1DxT() const;
  void print1DxF() const;
  void print1DxP() const;

  // Generate a mock trie header for testing purposes.
  std::vector<char> MockTrieHeader(int precisionBits, bool default_mode = true,
                                   const SpecialCounts &specialCounts =
                                       SpecialCounts{10, 5, 100, 80, 3}) const;

  std::vector<char>
  MockTrieHeader2D(int precisionBits, bool default_mode = true,
                   const SpecialCounts &specialCounts = SpecialCounts{
                       10, 5, 100, 80, 3}) const;

  std::vector<char>
  MockTrieHeader3D(int precisionBits, bool default_mode = true,
                   const SpecialCounts &specialCounts = SpecialCounts{
                       10, 5, 100, 80, 3}) const;

private:
  std::unique_ptr<TrieNode_13> root13;
  uint64_t trieSize13;

  std::unique_ptr<TrieNode_16> root16;
  uint64_t trieSize16;

  std::unique_ptr<TrieNode_20> root20;
  uint64_t trieSize20;

  std::unique_ptr<TLEoption3_2D> root2DxP;
  uint64_t trieSize2DxP;

  std::unique_ptr<TLE_3D_3x10> root3DxP;
  uint64_t trieSize3DxP;
};


template <typename NODE_TYPE>
void TrieManager::serializeTrie(std::vector<char> &buffer) {

  // Serialize the node's data into the buffer.
  if constexpr (std::is_same_v<NODE_TYPE, TrieNode_13>) {
    serialize_1DxT(root13.get(), buffer);
  } else if constexpr (std::is_same_v<NODE_TYPE, TrieNode_16>) {
    serialize_1DxF(root16.get(), buffer);
  } else if constexpr (std::is_same_v<NODE_TYPE, TrieNode_20>) {
    serialize_1DxP(root20.get(), buffer);
  } else if constexpr (std::is_same_v<NODE_TYPE, TLEoption3_2D>) {
    serialize_2DxP(root2DxP.get(), buffer);
  } else if constexpr (std::is_same_v<NODE_TYPE, TLE_3D_3x10>) {
    serialize_3DxP(root3DxP.get(), buffer);
  } else {
    std::cout << "Unknown node type for serialization" << std::endl;
    return;
  }
}

#endif // TRIE_MANAGER_H