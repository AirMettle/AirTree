// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/meta/PopulatedBins.hpp>
#include <airtree/query/cdf/CDF.hpp>
#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/Logger.hpp>
#include <limits>
#include <cmath>
#include <spdlog/spdlog.h>

using namespace airtree::query::cdf;
using namespace airtree::core::io;

CDF::CDF(std::span<const char> buffer) {
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

double CDF::getCDF(double value, bool interpolate) {

  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculateCDF(value, interpolate);
}

double CDF::calculateCDF(double value, bool interpolate) {
  uint32_t total_count = trie_count_;

  total_count += header_.pos_zero_count + header_.neg_zero_count
                 + header_.pos_inf_count + header_.neg_inf_count;

  if (total_count == 0)
    return 0.0;

  if (total_count > 0 && value == std::numeric_limits<double>::infinity())
    return 1.0;

  double cumulative = static_cast<double>(header_.neg_inf_count);

  if (value == -std::numeric_limits<double>::infinity())
    return cumulative / total_count;

  const auto &bins = populated_;
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
