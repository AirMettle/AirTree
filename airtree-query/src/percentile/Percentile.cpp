// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/AirTreeHeader.hpp>
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

Percentile::Percentile(std::span<const char> buffer) {
  header_ = airtree::core::common::deserializeHeader(buffer);
  const auto params = airtree::core::common::configParams(header_);
  dims_ = params.dims;
  bit_length_ = params.bit_length;
  bin_count_ = 1ULL << bit_length_;
  histogram_ = std::make_unique<airtree::query::meta::Histogram>(bit_length_);

  if (dims_ == 1 && (bit_length_ == 13 || bit_length_ == 16 || bit_length_ == 20)) {
    // The bins come straight from the bytes; no trie is built. Objects are immutable after this.
    auto set = airtree::query::meta::populatedBins(buffer, header_, *histogram_);
    populated_ = std::move(set.bins);
    trie_count_ = set.trieCount;
    populated_ready_ = true;
  }
}

double Percentile::getPercentile(double percentile) {
  return getPercentileWithBounds(percentile).value;
}

PercentileResult Percentile::getPercentileWithBounds(double percentile) {
  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculatePercentile(percentile);
}

PercentileResult Percentile::calculatePercentile(double percentile) {

  // Total count from trie
  const uint32_t trie_count = trie_count_;

  // Total count including special values (excluding NaN)
  uint32_t total_count = trie_count + header_.pos_zero_count
                         + header_.neg_zero_count + header_.pos_inf_count
                         + header_.neg_inf_count;

  // std::cout << "Total count: " << total_count << std::endl;
  if (total_count == 0) {
    return {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
  }

  double rank = (percentile / 100) * total_count;
  // std::cout << "Rank for percentile " << percentile << ": " << rank
  //           << std::endl;
  uint32_t cumulative_count = 0;

  // Handle negative infinity
  cumulative_count += header_.neg_inf_count;
  if (cumulative_count >= rank) {
    return {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
  }

  const auto &bins = populated_;
  const size_t last_position = histogram_->getBinCount() - 1;
  bool zeros_handled = false;
  for (const auto &bin : bins) {
    const double bin_value = histogram_->getFPNumber(bin.position);
    if (!zeros_handled && bin_value >= 0.0) {
      cumulative_count += header_.neg_zero_count;
      if (cumulative_count >= rank) {
        return {-0.0, -0.0, -0.0};
      }
      cumulative_count += header_.pos_zero_count;
      if (cumulative_count >= rank) {
        return {0.0, 0.0, 0.0};
      }
      zeros_handled = true;
    }
    cumulative_count += bin.count;
    if (cumulative_count < rank) {
      continue;
    }
    const uint32_t cumulative_before_bin = cumulative_count - bin.count;
    const double inf = std::numeric_limits<double>::infinity();
    const double lower = bin_value < 0.0
        ? (bin.position == 0 ? -inf : histogram_->getFPNumber(bin.position - 1))
        : bin_value;
    const double upper = bin.position == last_position ? inf
        : bin_value < 0.0 ? bin_value : histogram_->getFPNumber(bin.position + 1);
    if (bin.position == last_position) {
      if (rank > cumulative_count && header_.pos_inf_count > 0) {
        return {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
      }
      return {bin_value, lower, upper};
    }
    const double max_value = histogram_->getFPNumber(bin.position + 1);
    return {bin_value + (((rank - cumulative_before_bin) / bin.count) * (max_value - bin_value)), lower, upper};
  }

  // If zeros haven't been handled yet (all bins were negative or no bins at
  // all), handle them now
  if (!zeros_handled) {
    cumulative_count += header_.neg_zero_count;
    if (cumulative_count >= rank) {
      return {-0.0, -0.0, -0.0};
    }
    cumulative_count += header_.pos_zero_count;
    if (cumulative_count >= rank) {
      return {0.0, 0.0, 0.0};
    }
  }

  // Handle positive infinity (if rank extends beyond all trie values)
  cumulative_count += header_.pos_inf_count;
  if (cumulative_count >= rank) {
    return {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
  }

  // If we reach here, it means the percentile was not found - should we throw
  // an error?
  return {-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};
}
