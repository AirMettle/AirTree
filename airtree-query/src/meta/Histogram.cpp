// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

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
  bins_.resize(1ULL << bitLength);
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
  return (1ULL << bitLength_);
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
  return getFPNumber(index);
}

double Histogram::getBinUpperBound(size_t index) const {
  return getFPNumber(index + 1);
}

uint8_t Histogram::getSignBit(size_t index) const {
  if (index >= bins_.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 1)) & 0x01;
}

uint8_t Histogram::getExponentSignBit(size_t index) const {
  if (index >= bins_.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 2)) & 0x01;
}
