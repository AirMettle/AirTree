// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/query/meta/PopulatedBins.hpp>
#include <airtree/query/cdf/CDF.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/Logger.hpp>
#include <limits>
#include <cmath>
#include <spdlog/spdlog.h>

using namespace airtree::query::cdf;
using namespace airtree::core::io;

CDF::CDF(std::vector<char> &buffer) {
  AirTreeReader reader;
  reader.read(buffer);

  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();
  header_ = reader.getHeader();
  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_shared<airtree::query::meta::Histogram>(bit_length_);

  if (dims_ == 1) { // extract the populated bins now: objects are immutable after construction
    switch (bit_length_) {
    case 13: populatedBins<TrieNode_13>(); break;
    case 16: populatedBins<TrieNode_16>(); break;
    case 20: populatedBins<TrieNode_20>(); break;
    default: break;
    }
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
  }
  return 0;
}

double CDF::getCDF(double value, bool interpolate) {
  if (trie_node_.valueless_by_exception()) {
    throw std::runtime_error("Trie node not initialized.");
  }

  if (dims_ == 1 && bit_length_ == 13) {
    return calculateCDF<TrieNode_13>(value, interpolate);
  } else if (dims_ == 1 && bit_length_ == 16) {
    return calculateCDF<TrieNode_16>(value, interpolate);
  } else if (dims_ == 1 && bit_length_ == 20) {
    return calculateCDF<TrieNode_20>(value, interpolate);
  } else {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
}

template <typename NodeType>
const std::vector<airtree::query::meta::PopulatedBin> &CDF::populatedBins() {
  if (!populated_ready_) {
    populated_ = airtree::query::meta::populatedBins(trie_node_.get_ptr<NodeType>(), *histogram_);
    populated_ready_ = true;
  }
  return populated_;
}

template <typename NodeType>
double CDF::calculateCDF(double value, bool interpolate) {
  const auto &trie_root = trie_node_.get_ptr<NodeType>();

  uint32_t total_count = 0;
  for (size_t i = 0; i < trie_root->size(); ++i) {
    total_count += trie_root->counts[i];
  }

  total_count += header_.pos_zero_count + header_.neg_zero_count
                 + header_.pos_inf_count + header_.neg_inf_count;

  if (total_count == 0)
    return 0.0;

  if (total_count > 0 && value == std::numeric_limits<double>::infinity())
    return 1.0;

  double cumulative = static_cast<double>(header_.neg_inf_count);

  if (value == -std::numeric_limits<double>::infinity())
    return cumulative / total_count;

  const auto &bins = populatedBins<NodeType>();
  const size_t n_bins = histogram_->getBinCount();
  bool neg_zeros_handled = false;
  bool pos_zeros_handled = false;
  bool includes_neg_zero = (value >= 0.0);
  bool includes_pos_zero =
      (value > 0.0) || (value == 0.0 && !std::signbit(value));
  for (const auto &bin : bins) {
    const double bin_value = histogram_->getFPNumber(bin.position);
    const uint32_t bin_count = bin.count;
    bool bin_is_neg_zero_or_greater = (bin_value >= 0.0);
    bool bin_is_pos_zero_or_greater =
        (bin_value > 0.0) || (bin_value == 0.0 && !std::signbit(bin_value));
    if (!neg_zeros_handled && bin_is_neg_zero_or_greater) {
      neg_zeros_handled = true;
      if (includes_neg_zero) {
        cumulative += header_.neg_zero_count;
      }
    }
    if (!pos_zeros_handled && bin_is_pos_zero_or_greater) {
      pos_zeros_handled = true;
      if (includes_pos_zero) {
        cumulative += header_.pos_zero_count;
      }
    }
    const double next_value = (bin.position + 1 < n_bins)
                                  ? histogram_->getFPNumber(bin.position + 1)
                                  : std::numeric_limits<double>::infinity();
    if (value < bin_value) {
      break;
    }
    if (value >= next_value) {
      cumulative += bin_count;
    } else {
      if (!interpolate) {
        break;
      } else {
        double fraction = (value - bin_value) / (next_value - bin_value);
        cumulative += fraction * bin_count;
        break;
      }
    }
  }

  if (!neg_zeros_handled && includes_neg_zero) {
    cumulative += header_.neg_zero_count;
  }

  if (!pos_zeros_handled && includes_pos_zero) {
    cumulative += header_.pos_zero_count;
  }

  return cumulative / total_count;
}
