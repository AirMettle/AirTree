// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/topk/TopK.hpp>
#include <airtree/query/Logger.hpp>

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sys/types.h>
#include <tuple>
#include <vector>
#include <limits>

using namespace airtree::query::topk;
using namespace airtree::core::io;

// Sentinel for special values not in the Trie
constexpr uint32_t SPECIAL_VAL_REP = std::numeric_limits<uint32_t>::max();

TopK::TopK(std::vector<char> buffer) : buffer_(std::move(buffer)) {

  AirTreeReader reader;
  reader.read(buffer_);

  dims_ = reader.getDims();
  bit_length_ = reader.getBitLength();
  trie_node_ = reader.getType();
  header_ = reader.getHeader();

  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_shared<airtree::query::meta::Histogram>(bit_length_);
}

TopKResultVector TopK::getTopK(double k) {
  if (!histogram_) {
    SPDLOG_LOGGER_ERROR(logger(), "Histogram not initialized.");
    throw std::runtime_error("Histogram not initialized.");
  }

  if (trie_node_.valueless_by_exception()) {
    SPDLOG_LOGGER_ERROR(logger(), "Trie node not initialized.");
    throw std::runtime_error("Trie node not initialized.");
  }

  if (k <= 0 || k > 100.0) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Invalid k value: {}. Must be 0 < k <= 100.", k);
    throw std::out_of_range(
        "k must be a percentage greater than 0 and less than or equal to 100.");
  }

  if (dims_ == 1 && bit_length_ == 13) {
    return fetchTopK<TrieNode_13>(k);
  } else if (dims_ == 1 && bit_length_ == 16) {
    return fetchTopK<TrieNode_16>(k);
  } else if (dims_ == 1 && bit_length_ == 20) {
    return fetchTopK<TrieNode_20>(k);
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Unsupported dimensions or bit length: {}x{}",
                        dims_, bit_length_);
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

template <typename NodeType> TopKResultVector TopK::fetchTopK(double k) {

  uint64_t total_count_u64 = header_.pos_inf_count + header_.neg_inf_count
                             + header_.pos_zero_count + header_.neg_zero_count;

  const auto &trie_root = trie_node_.get_ptr<NodeType>();

  auto histogram_bins = histogram_->getBins();
  uint64_t histogram_bin_size = histogram_bins.size();

  for (size_t i = 0; i < histogram_bin_size; ++i) {
    uint64_t internal_rep =
        histogram_bins[i].second.getInternalRepresentation();
    total_count_u64 += getCount(trie_root, internal_rep);
  }

  if (total_count_u64 == 0) {
    return {};
  }

  TopKResultVector top_k_bins;
  uint64_t n = static_cast<uint64_t>(total_count_u64 * (k / 100.0));
  if (n == 0 && k > 0)
    n = 1;

  uint64_t cumulative_count = 0;

  // Check Positive Infinity
  if (header_.pos_inf_count > 0) {
    cumulative_count += header_.pos_inf_count;
    top_k_bins.emplace_back(std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::infinity(),
                            header_.pos_inf_count, SPECIAL_VAL_REP);

    if (cumulative_count >= n)
      return top_k_bins;
  }

  bool zeros_handled = false;

  // Iterate bins backwards (highest to lowest values)
  size_t idx = histogram_bin_size;
  while (idx > 0) {
    size_t bin_idx = idx - 1;
    idx--;

    double lower_bound = histogram_->getBinLowerBound(bin_idx);

    if (lower_bound < 0.0 && !zeros_handled) {
      if (header_.pos_zero_count > 0) {
        cumulative_count += header_.pos_zero_count;
        top_k_bins.emplace_back(
            0.0, 0.0, header_.pos_zero_count, SPECIAL_VAL_REP);
        if (cumulative_count >= n)
          return top_k_bins;
      }
      if (header_.neg_zero_count > 0) {
        cumulative_count += header_.neg_zero_count;
        top_k_bins.emplace_back(
            -0.0, -0.0, header_.neg_zero_count, SPECIAL_VAL_REP);
        if (cumulative_count >= n)
          return top_k_bins;
      }
      zeros_handled = true;
    }

    uint64_t internal_rep = histogram_->getInternalRepresentation(bin_idx);
    uint32_t bin_count = getCount<NodeType>(trie_root, internal_rep);

    if (bin_count == 0) {
      continue; // Skip empty bins
    }

    cumulative_count += bin_count;
    double upper_bound = histogram_->getBinUpperBound(bin_idx);

    top_k_bins.emplace_back(
        lower_bound, upper_bound, bin_count, (uint32_t)internal_rep);

    if (cumulative_count >= n) {
      SPDLOG_LOGGER_DEBUG(
          logger(),
          "Reached top-k threshold at index: {} with cumulative "
          "count: {} >= {}",
          bin_idx, cumulative_count, n);
      return top_k_bins;
    }
  }

  // Check zeros if not encountered during iteration
  if (!zeros_handled) {
    if (header_.pos_zero_count > 0) {
      cumulative_count += header_.pos_zero_count;
      top_k_bins.emplace_back(
          0.0, 0.0, header_.pos_zero_count, SPECIAL_VAL_REP);
      if (cumulative_count >= n)
        return top_k_bins;
    }
    if (header_.neg_zero_count > 0) {
      cumulative_count += header_.neg_zero_count;
      top_k_bins.emplace_back(
          -0.0, -0.0, header_.neg_zero_count, SPECIAL_VAL_REP);
      if (cumulative_count >= n)
        return top_k_bins;
    }
  }

  // Check Negative Infinity
  if (header_.neg_inf_count > 0) {
    cumulative_count += header_.neg_inf_count;
    top_k_bins.emplace_back(-std::numeric_limits<double>::infinity(),
                            -std::numeric_limits<double>::infinity(),
                            header_.neg_inf_count, SPECIAL_VAL_REP);
    if (cumulative_count >= n)
      return top_k_bins;
  }

  return top_k_bins;
}

TopKResult::TopKResult(double lower_bound, double upper_bound, uint32_t count,
                       uint32_t internal_rep)
    : lower_bound_(lower_bound), upper_bound_(upper_bound), count_(count),
      internal_rep_(internal_rep) {}

double TopKResult::getLowerBound() const {
  return lower_bound_;
}

double TopKResult::getUpperBound() const {
  return upper_bound_;
}

uint32_t TopKResult::getCount() const {
  return count_;
}

uint32_t TopKResult::getInternalRepresentation() const {
  return internal_rep_;
}

void TopKResult::setLowerBound(double lower_bound) {
  lower_bound_ = lower_bound;
}

void TopKResult::setUpperBound(double upper_bound) {
  upper_bound_ = upper_bound;
}

void TopKResult::setCount(uint32_t count) {
  count_ = count;
}

void TopKResult::setInternalRepresentation(uint32_t internal_rep) {
  internal_rep_ = internal_rep;
}
