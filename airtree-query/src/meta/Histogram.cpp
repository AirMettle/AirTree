#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/meta/Histogram.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

using namespace airtree::query::meta;

uint64_t HistogramMetadata::getInternalRepresentation() const {
  return internal_representation_;
}

void HistogramMetadata::setInternalRepresentation(uint64_t representation) {
  internal_representation_ = representation;
}

Histogram::Histogram(uint64_t bitLength) : bitLength_(bitLength) {
  // Initialize bins with default values
  bins_.resize(1u << bitLength);
  for (uint64_t i = 0; i < bins_.size(); ++i) {
    double binValue = reConstruct<double>(i, bitLength);
    auto binMetadata = HistogramMetadata();
    binMetadata.setInternalRepresentation(i);

    // Set the bin value and metadata
    bins_[i] = {binValue, binMetadata};
  }

  std::sort(bins_.begin(), bins_.end(),
            [](const std::pair<double, HistogramMetadata> &a,
               const std::pair<double, HistogramMetadata> &b) {
              return a.first < b.first;
            });
}

size_t Histogram::getBinIndex(double value) const {

  auto idx = std::lower_bound(
      bins_.begin(), bins_.end(), std::make_pair(value, HistogramMetadata()),
      [](const std::pair<double, HistogramMetadata> &a,
         const std::pair<double, HistogramMetadata> &b) {
        return a.first < b.first;
      });

  // The value is guaranteed to be in the bins_ vector because the histogram is
  // constructed to cover all possible values.
  return std::distance(bins_.begin(), idx);
}

uint64_t Histogram::getBinCount() const {
  return (1 << bitLength_);
}

std::vector<std::pair<double, HistogramMetadata>> Histogram::getBins() const {
  return bins_;
}

uint64_t Histogram::getInternalRepresentation(size_t index) const {
  return bins_[index].second.getInternalRepresentation();
}

double Histogram::getFPNumber(size_t index) const {
  if (index >= bins_.size()) {
    return std::numeric_limits<double>::infinity();
  }
  return bins_[index].first;
}

double Histogram::getBinLowerBound(size_t index) const {
  // The trie's reConstruct rounds magnitude down (zeroes truncated low
  // bits). For positive bin centres that lands on the lower edge of the
  // input range; for negative bin centres it lands on the upper edge
  // (closer to zero). Sorting by value puts negative bins to the left,
  // so the true lower edge for a negative bin is the previous bin's
  // reconstructed value.
  if (index < bins_.size() && bins_[index].first < 0.0) {
    if (index == 0) {
      return -std::numeric_limits<double>::infinity();
    }
    return bins_[index - 1].first;
  }
  return getFPNumber(index);
}

double Histogram::getBinUpperBound(size_t index) const {
  if (index < bins_.size() && bins_[index].first < 0.0) {
    return bins_[index].first;
  }
  return getFPNumber(index + 1);
}

uint8_t Histogram::getSignBit(size_t index) const {
  if (index < 0 || index >= bins_.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 1)) & 0x01;
}

uint8_t Histogram::getExponentSignBit(size_t index) const {
  if (index < 0 || index >= bins_.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 2)) & 0x01;
}
