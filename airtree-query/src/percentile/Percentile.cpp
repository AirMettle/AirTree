// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/query/meta/PopulatedBins.hpp>
#include <cstring>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/percentile/Percentile.hpp>
#include <cassert>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <type_traits>

using namespace airtree::query::percentile;
using namespace airtree::core::io;

Percentile::Percentile(std::vector<char> buffer) {

  AirTreeReader reader;
  reader.read(buffer);

  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();
  header_ = reader.getHeader();
  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_unique<airtree::query::meta::Histogram>(bit_length_);
}

double Percentile::getPercentile(double percentile) {
  if (!histogram_) {
    throw std::runtime_error("Histogram not initialized.");
  }

  if (trie_node_.valueless_by_exception()) {
    throw std::runtime_error("Trie node not initialized.");
  }

  if (percentile <= 0 || percentile >= 100) {
    throw std::out_of_range(
        "Percentile must be greater than 0 and less than 100.");
  }

  if (dims_ == 1 && bit_length_ == 13) {
    return calculatePercentile<TrieNode_13>(percentile);
  } else if (dims_ == 1 && bit_length_ == 16) {
    return calculatePercentile<TrieNode_16>(percentile);
  } else if (dims_ == 1 && bit_length_ == 20) {
    return calculatePercentile<TrieNode_20>(percentile);
  } else {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
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

template <typename NodeType>
const std::vector<airtree::query::meta::PopulatedBin> &Percentile::populatedBins() {
  if (!populated_ready_) {
    populated_ = airtree::query::meta::populatedBins(trie_node_.get_ptr<NodeType>(), *histogram_);
    populated_ready_ = true;
  }
  return populated_;
}

template <typename NodeType>
double Percentile::calculatePercentile(double percentile) {

  // Total count from trie
  uint32_t trie_count = 0;
  const auto &trie_root = trie_node_.get_ptr<NodeType>();
  for (size_t i = 0; i < trie_root->size(); ++i) {
    trie_count += trie_root->counts[i];
  }

  // Total count including special values (excluding NaN)
  uint32_t total_count = trie_count + header_.pos_zero_count
                         + header_.neg_zero_count + header_.pos_inf_count
                         + header_.neg_inf_count;

  // std::cout << "Total count: " << total_count << std::endl;
  if (total_count == 0) {
    return -std::numeric_limits<double>::infinity();
  }

  double rank = (percentile / 100) * total_count;
  // std::cout << "Rank for percentile " << percentile << ": " << rank
  //           << std::endl;
  uint32_t cumulative_count = 0;

  // Handle negative infinity
  cumulative_count += header_.neg_inf_count;
  if (cumulative_count >= rank) {
    return -std::numeric_limits<double>::infinity();
  }

  const auto &bins = populatedBins<NodeType>();
  const size_t last_position = histogram_->getBinCount() - 1;
  bool zeros_handled = false;
  for (const auto &bin : bins) {
    const double bin_value = histogram_->getFPNumber(bin.position);
    if (!zeros_handled && bin_value >= 0.0) {
      cumulative_count += header_.neg_zero_count;
      if (cumulative_count >= rank) {
        return -0.0;
      }
      cumulative_count += header_.pos_zero_count;
      if (cumulative_count >= rank) {
        return 0.0;
      }
      zeros_handled = true;
    }
    cumulative_count += bin.count;
    if (cumulative_count < rank) {
      continue;
    }
    const uint32_t cumulative_before_bin = cumulative_count - bin.count;
    if (bin.position == last_position) {
      if (rank > cumulative_count && header_.pos_inf_count > 0) {
        return std::numeric_limits<double>::infinity();
      }
      return bin_value;
    }
    const double max_value = histogram_->getFPNumber(bin.position + 1);
    return bin_value
           + (((rank - cumulative_before_bin) / bin.count) * (max_value - bin_value));
  }

  // If zeros haven't been handled yet (all bins were negative or no bins at
  // all), handle them now
  if (!zeros_handled) {
    cumulative_count += header_.neg_zero_count;
    if (cumulative_count >= rank) {
      return -0.0;
    }
    cumulative_count += header_.pos_zero_count;
    if (cumulative_count >= rank) {
      return 0.0;
    }
  }

  // Handle positive infinity (if rank extends beyond all trie values)
  cumulative_count += header_.pos_inf_count;
  if (cumulative_count >= rank) {
    return std::numeric_limits<double>::infinity();
  }

  // If we reach here, it means the percentile was not found - should we throw
  // an error?
  return -std::numeric_limits<double>::infinity();
}
