// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/utils/TrieManager.hpp>
#include <airtree/core/common/TLE.hpp>
#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <airtree/core/schema/trie3d/3DxP.hpp>
#include <airtree/core/serdes/trie3d/3DxP.hpp>
#include <cstring>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <cmath>


TrieManager::TrieManager()
    : root13(std::make_unique<TrieNode_13>()), trieSize13(sizeof(TrieNode_13)),
      root16(std::make_unique<TrieNode_16>()), trieSize16(sizeof(TrieNode_16)),
      root20(std::make_unique<TrieNode_20>()), trieSize20(sizeof(TrieNode_20)),
      root2DxP(std::make_unique<TLEoption3_2D>()),
      trieSize2DxP(sizeof(TLEoption3_2D)),
      root3DxP(std::make_unique<TLE_3D_3x10>()),
      trieSize3DxP(sizeof(TLE_3D_3x10)) {}


// --------------------
// 13 Insertion
// --------------------
void TrieManager::insert1DxT(unsigned int index, uint32_t count,
                             DistributionMethod method,
                             unsigned int targetChild) {
  // Check that the index fits in 8 bits.
  if (index >= BINS_256) {
    throw std::out_of_range("Index for 13 trie must be less than 256");
  }

  // Create Level1 node if it doesn't exist.
  if (!root13->populated.test(index)) {
    root13->populated.set(index);
    root13->nodes[index] = std::make_unique<TrieNode_13_Level1>();
    trieSize13 += sizeof(TrieNode_13_Level1);
  }
  // Update top-level count.
  root13->counts[index] += count;
  TrieNode_13_Level1 *child = root13->nodes[index].get();

  // Distribute the count according to the method.
  switch (method) {
  case DistributionMethod::SINGLE:
    if (targetChild < BINS_32)
      child->counts[targetChild] += count;
    break;

  case DistributionMethod::EVEN: {
    uint32_t base = count / BINS_32;
    uint32_t remainder = count % BINS_32;
    for (size_t i = 0; i < BINS_32; i++) {
      child->counts[i] += base + (i < remainder ? 1 : 0);
    }
    break;
  }

  case DistributionMethod::RANDOM: {
    std::vector<double> randomWeights(BINS_32);
    double totalWeight = 0;
    for (size_t i = 0; i < BINS_32; i++) {
      double weight = static_cast<double>(std::rand()) / RAND_MAX;
      randomWeights[i] = weight;
      totalWeight += weight;
    }
    uint32_t distributedSum = 0;
    for (size_t i = 0; i < BINS_32; i++) {
      uint32_t childCount =
          static_cast<uint32_t>((randomWeights[i] / totalWeight) * count);
      child->counts[i] += childCount;
      distributedSum += childCount;
    }
    if (distributedSum < count)
      child->counts[0] += (count - distributedSum);
    break;
  }
  }
}

// --------------------
// Apollo16 Insertion
// --------------------
void TrieManager::insert1DxF(unsigned int index, uint32_t count,
                             DistributionMethod method,
                             unsigned int targetChild) {
  // Check that the index fits in 8 bits.
  if (index >= BINS_256) {
    throw std::out_of_range("Index for Apollo16 trie must be less than 256");
  }

  // Create Level1 node if it doesn't exist.
  if (!root16->populated.test(index)) {
    root16->populated.set(index);
    root16->nodes[index] = std::make_unique<TrieNode_16_Level1>();
    trieSize16 += sizeof(TrieNode_16_Level1);
  }
  // Update top-level count.
  root16->counts[index] += count;
  std::unique_ptr<TrieNode_16_Level1> &child = root16->nodes[index];

  // Distribute the count according to the method.
  switch (method) {
  case DistributionMethod::SINGLE:
    if (targetChild < BINS_256)
      child->counts[targetChild] += count;
    break;

  case DistributionMethod::EVEN: {
    uint32_t base = count / BINS_256;
    uint32_t remainder = count % BINS_256;
    for (size_t i = 0; i < BINS_256; i++) {
      child->counts[i] += base + (i < remainder ? 1 : 0);
    }
    break;
  }

  case DistributionMethod::RANDOM: {
    std::vector<double> randomWeights(BINS_256);
    double totalWeight = 0;
    for (size_t i = 0; i < BINS_256; i++) {
      double weight = static_cast<double>(std::rand()) / RAND_MAX;
      randomWeights[i] = weight;
      totalWeight += weight;
    }
    uint32_t distributedSum = 0;
    for (size_t i = 0; i < BINS_256; i++) {
      uint32_t childCount =
          static_cast<uint32_t>((randomWeights[i] / totalWeight) * count);
      child->counts[i] += childCount;
      distributedSum += childCount;
    }
    if (distributedSum < count)
      child->counts[0] += (count - distributedSum);
    break;
  }
  }
}

// --------------------
// Roaring20 Insertion
// --------------------
void TrieManager::insert1DxP(unsigned int index0, unsigned int index1,
                             uint32_t count, DistributionMethod method,
                             unsigned int targetChild) {
  // Check that indices are within their allowed ranges.
  if (index0 >= BINS_256) {
    throw std::out_of_range("Roaring20 Level0 index must be less than 256");
  }
  if (index1 >= BINS_64) {
    throw std::out_of_range("Roaring20 Level1 index must be less than BINS_64");
  }

  // Create the Level1 node if needed.
  if (!root20->populated.test(index0)) {
    root20->populated.set(index0);
    root20->nodes[index0] = std::make_unique<TrieNode_20_Level1>();
    trieSize20 += sizeof(TrieNode_20_Level1);
  }
  // Increment the count at Level0.
  root20->counts[index0] += count;
  auto &level1Node = root20->nodes[index0];

  // Create the Level2 node if needed.
  if (!level1Node->populated.test(index1)) {
    level1Node->populated.set(index1);
    level1Node->nodes[index1] = std::make_unique<TrieNode_20_Level2>();
    trieSize20 += sizeof(TrieNode_20_Level2);
  }
  // Increment the count at Level1.
  level1Node->counts[index1] += count;
  auto &level2Node = level1Node->nodes[index1];

  // Now distribute the count into Level2 using the specified method.
  switch (method) {
  case DistributionMethod::SINGLE:
    if (targetChild < BINS_64)
      level2Node->counts[targetChild] += count;
    break;

  case DistributionMethod::EVEN: {
    uint32_t base = count / BINS_64;
    uint32_t remainder = count % BINS_64;
    for (size_t i = 0; i < BINS_64; i++) {
      level2Node->counts[i] += base + (i < remainder ? 1 : 0);
    }
    break;
  }

  case DistributionMethod::RANDOM: {
    std::vector<double> randomWeights(BINS_64);
    double totalWeight = 0;
    for (size_t i = 0; i < BINS_64; i++) {
      double weight = static_cast<double>(std::rand()) / RAND_MAX;
      randomWeights[i] = weight;
      totalWeight += weight;
    }
    uint32_t distributedSum = 0;
    for (size_t i = 0; i < BINS_64; i++) {
      uint32_t childCount =
          static_cast<uint32_t>((randomWeights[i] / totalWeight) * count);
      level2Node->counts[i] += childCount;
      distributedSum += childCount;
    }
    if (distributedSum < count)
      level2Node->counts[0] += (count - distributedSum);
    break;
  }
  }
}

void TrieManager::insert2DxP(uint32_t combinedTLE, uint32_t combined20Bits,
                             uint32_t count) {
  auto [dimensionInfos, specialCounts] = deconstructTLE(combinedTLE, 2);

  // std::cout << "Inserting into 2DxP with combinedTLE: " << combinedTLE
  //           << ", combined20Bits: " << combined20Bits << ", count: " << count
  //           << ", specialCounts: " << specialCounts
  //           << std::endl;
  if (dimensionInfos.size() != 2) {
    throw std::invalid_argument("Expected exactly 2 dimensions for 2DxP");
  }

  if (!root2DxP) {
    throw std::runtime_error("Root is null in insertintoTLETrie_2D_option3.");
  }

  if (!root2DxP->populated.test(combinedTLE)) {
    root2DxP->populated.set(combinedTLE);
  }

  root2DxP->counts[combinedTLE]++;

  if (specialCounts == 2) {
    return;
  }

  if (specialCounts == 1) {
    if (!root2DxP->nodes[combinedTLE]) {
      root2DxP->nodes[combinedTLE] = std::make_unique<TrieNode_2D_10>();
      trieSize2DxP += sizeof(TrieNode_2D_10);
    }

    if (!root2DxP->nodes[combinedTLE]->populated.test(combined20Bits)) {
      root2DxP->nodes[combinedTLE]->populated.set(combined20Bits);
    }
    root2DxP->nodes[combinedTLE]->counts[combined20Bits]++;
    return;
  }

  unsigned int first10 = (combined20Bits >> 10) & 0x3FF;
  unsigned int last10 = combined20Bits & 0x3FF;

  if (!root2DxP->nodes[combinedTLE]) {
    root2DxP->nodes[combinedTLE] = std::make_unique<TrieNode_2D_10>();
    trieSize2DxP += sizeof(TrieNode_2D_10);
  }

  if (!root2DxP->nodes[combinedTLE]->populated.test(first10)) {
    root2DxP->nodes[combinedTLE]->populated.set(first10);
    root2DxP->nodes[combinedTLE]->nodes[first10] =
        std::make_unique<TrieNode_2D_10_Level1>();
    trieSize2DxP += sizeof(TrieNode_2D_10_Level1);
  }

  root2DxP->nodes[combinedTLE]->counts[first10]++;
  root2DxP->nodes[combinedTLE]->nodes[first10]->counts[last10]++;
}

// Helper: Given a quadrant, returns the starting level0 index for that region.
static unsigned int quadrantToStartIndex(Quadrant q) {
  switch (q) {
  case Quadrant::POS_POS:
    return 0; // indices 0 to 63
  case Quadrant::POS_NEG:
    return 64; // indices 64 to 127
  case Quadrant::NEG_POS:
    return 128; // indices 128 to 191
  case Quadrant::NEG_NEG:
    return 192; // indices 192 to 255
  default:
    return 0; // should not happen
  }
}


// --------------------
// AUTOMATIC INSERTION
// --------------------
void TrieManager::insertByQuadrant(Quadrant quadrant, uint32_t count,
                                   DistributionMethod method, int precisionBits,
                                   Level0Distribution level0Dist) {
  const unsigned int regionSize = 64; // 256 / 4 = 64 buckets per quadrant.
  unsigned int startIndex = quadrantToStartIndex(quadrant);

  // Compute weights for each bucket based on the chosen distribution.
  std::vector<double> weights(regionSize, 0.0);
  switch (level0Dist) {
  case Level0Distribution::EVEN:
    for (unsigned int i = 0; i < regionSize; i++)
      weights[i] = 1.0;
    break;
  case Level0Distribution::LEFT_SKEW:
    for (unsigned int i = 0; i < regionSize; i++)
      weights[i] = static_cast<double>(regionSize - i);
    break;
  case Level0Distribution::RIGHT_SKEW:
    for (unsigned int i = 0; i < regionSize; i++)
      weights[i] = static_cast<double>(i + 1);
    break;
  case Level0Distribution::NORMAL_SKEW: {
    double center = (regionSize - 1) / 2.0;
    double sigma = regionSize / 6.0;
    for (unsigned int i = 0; i < regionSize; i++) {
      double diff = i - center;
      weights[i] = std::exp(-(diff * diff) / (2 * sigma * sigma));
    }
  } break;
  default:
    throw std::invalid_argument("Invalid Level0Distribution");
  }

  double totalWeight = 0.0;
  for (double w : weights)
    totalWeight += w;

  std::vector<uint32_t> bucketCounts(regionSize, 0);
  uint32_t distributedSum = 0;
  std::vector<double> fractions(regionSize, 0.0);
  for (unsigned int i = 0; i < regionSize; i++) {
    double rawCount = (weights[i] / totalWeight) * count;
    bucketCounts[i] = static_cast<uint32_t>(std::floor(rawCount));
    fractions[i] = rawCount - bucketCounts[i];
    distributedSum += bucketCounts[i];
  }
  uint32_t diff = count - distributedSum;
  while (diff > 0) {
    unsigned int maxIndex = 0;
    double maxFrac = fractions[0];
    for (unsigned int i = 1; i < regionSize; i++) {
      if (fractions[i] > maxFrac) {
        maxFrac = fractions[i];
        maxIndex = i;
      }
    }
    bucketCounts[maxIndex]++;
    fractions[maxIndex] = 0.0; // Reset so we don't add repeatedly.
    diff--;
  }

  // **Force insertion for all 64 buckets, even if the bucket count is 0.**
  for (unsigned int i = 0; i < regionSize; i++) {
    unsigned int index = startIndex + i;
    switch (precisionBits) {
    case 13:
      insert1DxT(index, bucketCounts[i], method, 0);
      break;
    case 16:
      insert1DxF(index, bucketCounts[i], method, 0);
      break;
    case 20:
      insert1DxP(index, 0, bucketCounts[i], method, 0);
      break;
    default:
      throw std::invalid_argument(
          "Invalid precisionBits provided. Use 13, 16, or 20.");
    }
  }
}


void TrieManager::insertByQuadrant(const std::vector<QuadrantRegion> &regions,
                                   DistributionMethod method,
                                   int precisionBits) {
  for (const auto &region : regions) {
    // Call the single quadrant insertion using the provided distribution for
    // each region.
    insertByQuadrant(region.quadrant, region.count, method, precisionBits,
                     region.level0Dist);
  }
}


// --------------------
// 1DxT: Retrieve Populated Buckets
// --------------------
std::vector<std::pair<unsigned int, std::vector<uint32_t>>>
TrieManager::getPopulatedBuckets1DxT() const {
  std::vector<std::pair<unsigned int, std::vector<uint32_t>>> buckets;
  for (unsigned int i = 0; i < BINS_256; i++) {
    if (root13->populated.test(i)) {
      std::vector<uint32_t> lowerCounts;
      for (size_t j = 0; j < BINS_32; j++) {
        lowerCounts.push_back(root13->nodes[i]->counts[j]);
      }
      buckets.emplace_back(i, lowerCounts);
    }
  }
  return buckets;
}

// --------------------
// 1DxF: Retrieve Populated Buckets
// --------------------
std::vector<std::pair<unsigned int, std::vector<uint32_t>>>
TrieManager::getPopulatedBuckets1DxF() const {
  std::vector<std::pair<unsigned int, std::vector<uint32_t>>> buckets;
  for (unsigned int i = 0; i < BINS_256; i++) {
    if (root16->populated.test(i)) {
      std::vector<uint32_t> lowerCounts;
      for (size_t j = 0; j < BINS_256; j++) {
        lowerCounts.push_back(root16->nodes[i]->counts[j]);
      }
      buckets.emplace_back(i, lowerCounts);
    }
  }
  return buckets;
}

// --------------------
// 1DxP: Retrieve Populated Buckets
// --------------------
std::vector<Roaring20Bucket> TrieManager::getPopulatedBuckets1DxP() const {
  std::vector<Roaring20Bucket> buckets;
  for (unsigned int i = 0; i < BINS_256; i++) {
    if (root20->populated.test(i)) {
      Roaring20Bucket bucket;
      bucket.level0Index = i;
      // Iterate over Level1 buckets.
      for (unsigned int j = 0; j < BINS_64; j++) {
        // Only include populated Level1 buckets.
        if (root20->nodes[i] && root20->nodes[i]->populated.test(j)) {
          std::vector<uint32_t> level2Counts;
          // Collect counts from the corresponding Level2 node.
          if (root20->nodes[i]->nodes[j]) {
            for (unsigned int k = 0; k < BINS_64; k++) {
              level2Counts.push_back(root20->nodes[i]->nodes[j]->counts[k]);
            }
          }
          bucket.level1Buckets.emplace_back(j, level2Counts);
        }
      }
      buckets.push_back(bucket);
    }
  }
  return buckets;
}


// --------------------
// Print Functions
// --------------------
void TrieManager::print1DxT() const {
  auto buckets = getPopulatedBuckets1DxT();
  std::cout << "13Colonies(1DxT) Trie:" << std::endl;
  for (const auto &bucket : buckets) {
    std::cout << "Level0 index " << bucket.first << ": ";
    for (const auto count : bucket.second)
      std::cout << count << " ";
    std::cout << std::endl;
  }
}

void TrieManager::print1DxF() const {
  auto buckets = getPopulatedBuckets1DxF();
  std::cout << "Apollo16(1DxF) Trie:" << std::endl;
  for (const auto &bucket : buckets) {
    std::cout << "Level0 index " << bucket.first << ": ";
    for (const auto count : bucket.second)
      std::cout << count << " ";
    std::cout << std::endl;
  }
}

void TrieManager::print1DxP() const {
  auto buckets = getPopulatedBuckets1DxP();
  std::cout << "Roaring20(1DxP) Trie:" << std::endl;
  for (const auto &bucket : buckets) {
    std::cout << "Level0 index " << bucket.level0Index << ":" << std::endl;
    for (const auto &pair : bucket.level1Buckets) {
      std::cout << "   Level1 index " << pair.first << ": ";
      for (const auto count : pair.second)
        std::cout << count << " ";
      std::cout << std::endl;
    }
  }
}

const TrieNode_13 &TrieManager::getRoot1DxT() const {
  return *root13;
}

uint64_t TrieManager::getTrieSize1DxT() const {
  return trieSize13;
}

const TrieNode_16 &TrieManager::getRoot1DxF() const {
  return *root16;
}

uint64_t TrieManager::getTrieSize1DxF() const {
  return trieSize16;
}

const TrieNode_20 &TrieManager::getRoot1DxP() const {
  return *root20;
}

uint64_t TrieManager::getTrieSize1DxP() const {
  return trieSize20;
}

const TLEoption3_2D &TrieManager::getRoot2DxP() const {
  return *root2DxP;
}

uint64_t TrieManager::getTrieSize2DxP() const {
  return trieSize2DxP;
}

std::vector<char>
TrieManager::MockTrieHeader(int precisionBits, bool default_mode,
                            const SpecialCounts &specialCounts) const {
  using namespace airtree::core;
using namespace airtree::core::common;

  ConfigWire config;
  if (precisionBits == 13)
    config = ConfigWire::Config_1D_Tiny;
  else if (precisionBits == 16)
    config = ConfigWire::Config_1D_Fast;
  else if (precisionBits == 20)
    config = ConfigWire::Config_1D_Precise;
  else
    throw std::invalid_argument("Unknown precisionBits for 1D MockTrieHeader");

  auto header = makeHeader(config, {0, 0, 0, 0}, 1,
                           specialCounts.posInfCount,
                           specialCounts.negInfCount,
                           specialCounts.posZeroCount,
                           specialCounts.negZeroCount,
                           specialCounts.nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);
  return buffer;
}

std::vector<char>
TrieManager::MockTrieHeader2D(int precisionBits, bool default_mode,
                              const SpecialCounts &specialCounts) const {
  using namespace airtree::core;
using namespace airtree::core::common;

  int node_width = 4 + precisionBits;
  ConfigWire config;
  if (node_width == 8)
    config = ConfigWire::Config_2D_Fast;
  else if (node_width == 10)
    config = ConfigWire::Config_2D_Precise;
  else
    throw std::invalid_argument("Unknown node_width for 2D MockTrieHeader");

  auto header = makeHeader(config, {0, 0, 0, 0}, 1,
                           specialCounts.posInfCount,
                           specialCounts.negInfCount,
                           specialCounts.posZeroCount,
                           specialCounts.negZeroCount,
                           specialCounts.nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);
  return buffer;
}

// --------------------
// 3DxP Methods
// --------------------
void TrieManager::insert3DxP(uint32_t combinedTLE, uint64_t combined30Bits,
                             uint32_t count) {
  if (combinedTLE >= BINS_512) {
    throw std::out_of_range("combinedTLE for 3DxP must be less than 512");
  }

  // Set TLE level as populated
  if (!root3DxP->populated.test(combinedTLE)) {
    root3DxP->populated.set(combinedTLE);
  }
  root3DxP->counts[combinedTLE] += count;

  // Extract l0_10, l1_10, l2_10 from the combined value
  uint32_t l0_10 = (combined30Bits >> 20) & 0x3FF;
  uint32_t l1_10 = (combined30Bits >> 10) & 0x3FF;
  uint32_t l2_10 = combined30Bits & 0x3FF;

  // Create Level 0 node if needed
  if (!root3DxP->nodes[combinedTLE]) {
    root3DxP->nodes[combinedTLE] = std::make_unique<Node3D_3x10_l0>();
    trieSize3DxP += sizeof(Node3D_3x10_l0);
  }

  // Set Level 0 populated
  if (!root3DxP->nodes[combinedTLE]->populated.test(l0_10)) {
    root3DxP->nodes[combinedTLE]->populated.set(l0_10);
  }
  root3DxP->nodes[combinedTLE]->counts[l0_10] += count;

  // Create Level 1 node if needed
  if (!root3DxP->nodes[combinedTLE]->nodes[l0_10]) {
    root3DxP->nodes[combinedTLE]->nodes[l0_10] =
        std::make_unique<Node3D_3x10_l1>();
    trieSize3DxP += sizeof(Node3D_3x10_l1);
  }

  // Set Level 1 populated
  if (!root3DxP->nodes[combinedTLE]->nodes[l0_10]->populated.test(l1_10)) {
    root3DxP->nodes[combinedTLE]->nodes[l0_10]->populated.set(l1_10);
  }
  root3DxP->nodes[combinedTLE]->nodes[l0_10]->counts[l1_10] += count;

  // Create Level 2 node if needed
  if (!root3DxP->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]) {
    root3DxP->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10] =
        std::make_unique<Node3D_3x10_l2>();
    trieSize3DxP += sizeof(Node3D_3x10_l2);
  }

  // Set Level 2 populated and count
  if (!root3DxP->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.test(
          l2_10)) {
    root3DxP->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->populated.set(
        l2_10);
  }
  root3DxP->nodes[combinedTLE]->nodes[l0_10]->nodes[l1_10]->counts[l2_10] +=
      count;
}

const TLE_3D_3x10 &TrieManager::getRoot3DxP() const {
  return *root3DxP;
}

uint64_t TrieManager::getTrieSize3DxP() const {
  return trieSize3DxP;
}

std::vector<char>
TrieManager::MockTrieHeader3D(int precisionBits, bool default_mode,
                              const SpecialCounts &specialCounts) const {
using namespace airtree::core;
using namespace airtree::core::common;

  int node_width = 4 + precisionBits;
  ConfigWire config;
  if (node_width == 8)
    config = ConfigWire::Config_3D_Fast;
  else if (node_width == 10)
    config = ConfigWire::Config_3D_Precise;
  else
    throw std::invalid_argument("Unknown node_width for 3D MockTrieHeader");

  auto header = makeHeader(config, {0, 0, 0, 0}, 1,
                           specialCounts.posInfCount,
                           specialCounts.negInfCount,
                           specialCounts.posZeroCount,
                           specialCounts.negZeroCount,
                           specialCounts.nanCount);

  std::vector<char> buffer;
  serializeHeader(header, buffer);
  finalizeHeader(buffer, 0);
  return buffer;
}
