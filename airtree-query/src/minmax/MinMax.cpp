// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <cstdint>
#include <cstring>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/minmax/MinMax.hpp>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

using namespace airtree::query::minmax;
using namespace airtree::core::io;

MinMax::MinMax(std::vector<char> buffer) {

  AirTreeReader reader;
  reader.read(buffer);

  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();

  header_ = reader.getHeader();

  bin_count_ = 1 << bit_length_;
  histogram_ = std::make_unique<airtree::query::meta::Histogram>(bit_length_);
}

MinMaxResultVector MinMax::getMin() {
  if (!histogram_)
    throw std::runtime_error("Histogram not initialized.");
  if (trie_node_.valueless_by_exception())
    throw std::runtime_error("Trie node not initialized.");

  if (dims_ == 1 && bit_length_ == 13)
    return calculateMin<TrieNode_13>();
  else if (dims_ == 1 && bit_length_ == 16)
    return calculateMin<TrieNode_16>();
  else if (dims_ == 1 && bit_length_ == 20)
    return calculateMin<TrieNode_20>();
  else
    throw std::runtime_error("Unsupported dimensions or bit length.");
}

MinMaxResultVector MinMax::getMax() {
  if (!histogram_)
    throw std::runtime_error("Histogram not initialized.");
  if (trie_node_.valueless_by_exception())
    throw std::runtime_error("Trie node not initialized.");

  if (dims_ == 1 && bit_length_ == 13)
    return calculateMax<TrieNode_13>();
  else if (dims_ == 1 && bit_length_ == 16)
    return calculateMax<TrieNode_16>();
  else if (dims_ == 1 && bit_length_ == 20)
    return calculateMax<TrieNode_20>();
  else
    throw std::runtime_error("Unsupported dimensions or bit length.");
}

MinMaxResultVector MinMax::getMinValue() {
  if (!histogram_)
    throw std::runtime_error("Histogram is not initialized.");
  if (trie_node_.valueless_by_exception())
    throw std::runtime_error("Trie node not initialized.");

  if (dims_ == 1 && bit_length_ == 13)
    return calculateMinValue<TrieNode_13>();
  else if (dims_ == 1 && bit_length_ == 16)
    return calculateMinValue<TrieNode_16>();
  else if (dims_ == 1 && bit_length_ == 20)
    return calculateMinValue<TrieNode_20>();
  else
    throw std::runtime_error("Unsupported dimensions or bit length.");
}

MinMaxResultVector MinMax::getMaxValue() {
  if (!histogram_)
    throw std::runtime_error("Histogram is not initialized.");
  if (trie_node_.valueless_by_exception())
    throw std::runtime_error("Trie node not initialized.");

  if (dims_ == 1 && bit_length_ == 13)
    return calculateMaxValue<TrieNode_13>();
  else if (dims_ == 1 && bit_length_ == 16)
    return calculateMaxValue<TrieNode_16>();
  else if (dims_ == 1 && bit_length_ == 20)
    return calculateMaxValue<TrieNode_20>();
  else
    throw std::runtime_error("Unsupported dimensions or bit length.");
}

template <typename NodeType>
inline uint32_t getCount(const std::unique_ptr<NodeType> &trie,
                         uint64_t internal_rep) {

  if constexpr (std::is_same_v<NodeType, TrieNode_13>) {
    uint64_t prefix_8 = (internal_rep >> 5) & 0xFF;
    uint64_t prefix_5 = internal_rep & 0x1F;

    if (trie->populated.test(prefix_8)
        && trie->nodes[prefix_8]->counts[prefix_5] > 0) {
      return trie->nodes[prefix_8]->counts[prefix_5];
    }
  } else if constexpr (std::is_same_v<NodeType, TrieNode_16>) {
    uint64_t prefix_8 = (internal_rep >> 8) & 0xFF;
    uint64_t suffix_8 = internal_rep & 0xFF;

    if (trie->populated.test(prefix_8)
        && trie->nodes[prefix_8]->counts[suffix_8] > 0) {
      return trie->nodes[prefix_8]->counts[suffix_8];
    }
  } else if constexpr (std::is_same_v<NodeType, TrieNode_20>) {
    uint64_t prefix_8 = (internal_rep >> 12) & 0xFF;
    uint64_t mid_6 = (internal_rep >> 6) & 0x3F;
    uint64_t suffix_6 = internal_rep & 0x3F;

    if (trie->populated.test(prefix_8)
        && trie->nodes[prefix_8]->populated.test(mid_6)
        && trie->nodes[prefix_8]->nodes[mid_6]->counts[suffix_6] > 0) {
      return trie->nodes[prefix_8]->nodes[mid_6]->counts[suffix_6];
    }
  } else {
    throw std::runtime_error("Unsupported node type for count retrieval.");
  }

  return 0;
}

// Helper to update min frequency results
void updateMinResults(MinMaxResultVector &results, uint32_t &min_count,
                      double val_low, double val_high, uint32_t count) {
  if (count == 0)
    return;
  if (count < min_count) {
    results.clear();
    min_count = count;
    results.emplace_back(val_low, val_high, count);
  } else if (count == min_count) {
    results.emplace_back(val_low, val_high, count);
  }
}

// Helper to update max frequency results
void updateMaxResults(MinMaxResultVector &results, uint32_t &max_count,
                      double val_low, double val_high, uint32_t count) {
  if (count == 0)
    return;
  if (count > max_count) {
    results.clear();
    max_count = count;
    results.emplace_back(val_low, val_high, count);
  } else if (count == max_count) {
    results.emplace_back(val_low, val_high, count);
  }
}

template <typename NodeType> MinMaxResultVector MinMax::calculateMin() {
  MinMaxResultVector results;
  const auto &trie_root = trie_node_.get_ptr<NodeType>();
  uint32_t min_count = std::numeric_limits<uint32_t>::max();

  // NOTE: We INCLUDE special values in frequency analysis.
  // If +inf is the most/least common value, the user needs to know.
  updateMinResults(results, min_count, -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   header_.neg_inf_count);
  updateMinResults(results, min_count, -0.0, -0.0, header_.neg_zero_count);
  updateMinResults(results, min_count, 0.0, 0.0, header_.pos_zero_count);
  updateMinResults(results, min_count, std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   header_.pos_inf_count);

  for (size_t i = 0; i < histogram_->getBinCount(); ++i) {
    uint64_t internal_rep = histogram_->getInternalRepresentation(i);
    uint32_t count = getCount<NodeType>(trie_root, internal_rep);

    double lowerBound = histogram_->getBinLowerBound(i);
    double upperBound = histogram_->getBinUpperBound(i);
    updateMinResults(results, min_count, lowerBound, upperBound, count);
  }

  return results;
}


template <typename NodeType> MinMaxResultVector MinMax::calculateMax() {
  MinMaxResultVector results;
  const auto &trie_root = trie_node_.get_ptr<NodeType>();
  uint32_t max_count = 0;

  // NOTE: We INCLUDE special values in frequency analysis.
  updateMaxResults(results, max_count, -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   header_.neg_inf_count);
  updateMaxResults(results, max_count, -0.0, -0.0, header_.neg_zero_count);
  updateMaxResults(results, max_count, 0.0, 0.0, header_.pos_zero_count);
  updateMaxResults(results, max_count, std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   header_.pos_inf_count);

  for (size_t i = 0; i < histogram_->getBinCount(); ++i) {
    uint64_t internal_rep = histogram_->getInternalRepresentation(i);
    uint32_t count = getCount<NodeType>(trie_root, internal_rep);

    double lowerBound = histogram_->getBinLowerBound(i);
    double upperBound = histogram_->getBinUpperBound(i);
    updateMaxResults(results, max_count, lowerBound, upperBound, count);
  }

  return results;
}

template <typename NodeType> MinMaxResultVector MinMax::calculateMinValue() {
  MinMaxResultVector results;
  const auto &trie_root = trie_node_.get_ptr<NodeType>();

  // Order: -Inf -> Negatives -> -0 -> +0 -> Positives -> +Inf

  // 1. Check Negative Infinity
  if (header_.neg_inf_count > 0) {
    results.emplace_back(-std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(),
                         header_.neg_inf_count);
    return results;
  }

  bool zeros_handled = false;

  // 2. Iterate bins (Negatives -> Positives)
  for (size_t i = 0; i < histogram_->getBinCount(); ++i) {
    double lowerBound = histogram_->getBinLowerBound(i);

    // Transition to non-negatives: check zeros
    if (lowerBound >= 0.0 && !zeros_handled) {
      if (header_.neg_zero_count > 0) {
        results.emplace_back(-0.0, -0.0, header_.neg_zero_count);
        return results;
      }
      if (header_.pos_zero_count > 0) {
        results.emplace_back(0.0, 0.0, header_.pos_zero_count);
        return results;
      }
      zeros_handled = true;
    }

    uint64_t internal_rep = histogram_->getInternalRepresentation(i);
    uint32_t count = getCount<NodeType>(trie_root, internal_rep);
    if (count > 0) {
      double upperBound = histogram_->getBinUpperBound(i);
      results.emplace_back(lowerBound, upperBound, count);
      return results;
    }
  }

  // Post-loop check for zeros (if all bins were negative)
  if (!zeros_handled) {
    if (header_.neg_zero_count > 0) {
      results.emplace_back(-0.0, -0.0, header_.neg_zero_count);
      return results;
    }
    if (header_.pos_zero_count > 0) {
      results.emplace_back(0.0, 0.0, header_.pos_zero_count);
      return results;
    }
  }

  // 3. Check Positive Infinity
  if (header_.pos_inf_count > 0) {
    results.emplace_back(std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity(),
                         header_.pos_inf_count);
    return results;
  }

  // Return empty if trie is truly empty
  return results;
}

template <typename NodeType> MinMaxResultVector MinMax::calculateMaxValue() {
  MinMaxResultVector results;
  const auto &trie_root = trie_node_.get_ptr<NodeType>();

  // Order: +Inf -> Positives -> +0 -> -0 -> Negatives -> -Inf

  // 1. Check Positive Infinity
  if (header_.pos_inf_count > 0) {
    results.emplace_back(std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity(),
                         header_.pos_inf_count);
    return results;
  }

  bool zeros_handled = false;
  size_t total_bins = histogram_->getBinCount();

  // 2. Iterate bins BACKWARDS (Positives -> Negatives)
  if (total_bins > 0) {
    for (size_t i = total_bins; i-- > 0;) {
      double lowerBound = histogram_->getBinLowerBound(i);

      // Transition to negatives: check zeros
      if (lowerBound < 0.0 && !zeros_handled) {
        if (header_.pos_zero_count > 0) {
          results.emplace_back(0.0, 0.0, header_.pos_zero_count);
          return results;
        }
        if (header_.neg_zero_count > 0) {
          results.emplace_back(-0.0, -0.0, header_.neg_zero_count);
          return results;
        }
        zeros_handled = true;
      }

      uint64_t internal_rep = histogram_->getInternalRepresentation(i);
      uint32_t count = getCount<NodeType>(trie_root, internal_rep);
      if (count > 0) {
        double upperBound = histogram_->getBinUpperBound(i);
        results.emplace_back(lowerBound, upperBound, count);
        return results;
      }
    }
  }

  // Post-loop check for zeros
  if (!zeros_handled) {
    if (header_.pos_zero_count > 0) {
      results.emplace_back(0.0, 0.0, header_.pos_zero_count);
      return results;
    }
    if (header_.neg_zero_count > 0) {
      results.emplace_back(-0.0, -0.0, header_.neg_zero_count);
      return results;
    }
  }

  // 3. Check Negative Infinity
  if (header_.neg_inf_count > 0) {
    results.emplace_back(-std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(),
                         header_.neg_inf_count);
    return results;
  }

  return results;
}

MinMaxResult::MinMaxResult(double lowerBound, double upperBound,
                           uint32_t bin_count)
    : lowerBound_(lowerBound), upperBound_(upperBound), bin_count_(bin_count) {}

double MinMaxResult::getLowerBound() const {
  return lowerBound_;
}
double MinMaxResult::getUpperBound() const {
  return upperBound_;
}
uint32_t MinMaxResult::getBinCount() const {
  return bin_count_;
}

void MinMaxResult::setLowerBound(double lowerBound) {
  lowerBound_ = lowerBound;
}
void MinMaxResult::setUpperBound(double upperBound) {
  upperBound_ = upperBound;
}
void MinMaxResult::setBinCount(uint32_t bin_count) {
  bin_count_ = bin_count;
}