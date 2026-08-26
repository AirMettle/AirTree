// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/query/meta/PopulatedBins.hpp>
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

MinMax::MinMax(std::span<const char> buffer) {
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

MinMaxResultVector MinMax::getMin() {
  if (!histogram_)
    throw std::runtime_error("Histogram not initialized.");
  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculateMin();
}

MinMaxResultVector MinMax::getMax() {
  if (!histogram_)
    throw std::runtime_error("Histogram not initialized.");
  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculateMax();
}

MinMaxResultVector MinMax::getMinValue() {
  if (!histogram_)
    throw std::runtime_error("Histogram is not initialized.");
  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculateMinValue();
}

MinMaxResultVector MinMax::getMaxValue() {
  if (!histogram_)
    throw std::runtime_error("Histogram is not initialized.");
  if (!populated_ready_) {
    throw std::runtime_error("Unsupported dimensions or bit length.");
  }
  return calculateMaxValue();
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

MinMaxResultVector MinMax::calculateMin() {
  MinMaxResultVector results;
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

  for (const auto &bin : populated_) {
    updateMinResults(results, min_count, histogram_->getBinLowerBound(bin.position),
                     histogram_->getBinUpperBound(bin.position), bin.count);
  }

  return results;
}


MinMaxResultVector MinMax::calculateMax() {
  MinMaxResultVector results;
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

  for (const auto &bin : populated_) {
    updateMaxResults(results, max_count, histogram_->getBinLowerBound(bin.position),
                     histogram_->getBinUpperBound(bin.position), bin.count);
  }

  return results;
}

MinMaxResultVector MinMax::calculateMinValue() {
  MinMaxResultVector results;

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
  for (const auto &bin : populated_) {
    double lowerBound = histogram_->getBinLowerBound(bin.position);
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
    results.emplace_back(lowerBound, histogram_->getBinUpperBound(bin.position), bin.count);
    return results;
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

MinMaxResultVector MinMax::calculateMaxValue() {
  MinMaxResultVector results;

  // Order: +Inf -> Positives -> +0 -> -0 -> Negatives -> -Inf

  // 1. Check Positive Infinity
  if (header_.pos_inf_count > 0) {
    results.emplace_back(std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity(),
                         header_.pos_inf_count);
    return results;
  }

  bool zeros_handled = false;
  const auto &bins = populated_;
  for (auto it = bins.rbegin(); it != bins.rend(); ++it) {
    double lowerBound = histogram_->getBinLowerBound(it->position);
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
    results.emplace_back(lowerBound, histogram_->getBinUpperBound(it->position), it->count);
    return results;
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