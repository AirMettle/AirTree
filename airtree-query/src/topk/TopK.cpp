// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/meta/PopulatedBins.hpp>
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

TopK::TopK(std::span<const char> buffer) {
  header_ = airtree::core::common::deserializeHeader(buffer);
  const auto params = airtree::core::common::configParams(header_);
  dims_ = params.dims;
  bit_length_ = params.bit_length;
  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_shared<airtree::query::meta::Histogram>(bit_length_);

  if (dims_ == 1 && (bit_length_ == 13 || bit_length_ == 16 || bit_length_ == 20)) {
    // The bins come straight from the bytes; no trie is built. Objects are immutable after this.
    auto set = airtree::query::meta::populatedBins(buffer, header_, *histogram_);
    populated_ = std::move(set.bins);
    trie_count_ = set.trieCount;
    populated_ready_ = true;
  }
}

TopKResultVector TopK::getTopK(double k) {
  if (!histogram_) {
    SPDLOG_LOGGER_ERROR(logger(), "Histogram not initialized.");
    throw std::runtime_error("Histogram not initialized.");
  }


  if (k <= 0 || k > 100.0) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Invalid k value: {}. Must be 0 < k <= 100.", k);
    throw std::out_of_range(
        "k must be a percentage greater than 0 and less than or equal to 100.");
  }

  if (!populated_ready_) {
    SPDLOG_LOGGER_ERROR(logger(), "Unsupported dimensions or bit length: {}x{}",
                        dims_, bit_length_);
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return fetchTopK(k);
}

TopKResultVector TopK::fetchTopK(double k) {

  uint64_t total_count_u64 = header_.pos_inf_count + header_.neg_inf_count
                             + header_.pos_zero_count + header_.neg_zero_count;


  const auto &bins = populated_;
  for (const auto &bin : bins) {
    total_count_u64 += bin.count;
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
  for (auto it = bins.rbegin(); it != bins.rend(); ++it) {
    double lower_bound = histogram_->getBinLowerBound(it->position);
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
    cumulative_count += it->count;
    double upper_bound = histogram_->getBinUpperBound(it->position);
    top_k_bins.emplace_back(
        lower_bound, upper_bound, it->count, static_cast<uint32_t>(it->code));
    if (cumulative_count >= n) {
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
