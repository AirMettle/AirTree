// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/query/meta/Histogram.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>

using namespace airtree::query::meta;

uint64_t HistogramMetadata::getInternalRepresentation() const {
  return internal_representation_;
}

void HistogramMetadata::setInternalRepresentation(uint64_t representation) {
  internal_representation_ = representation;
}

struct Histogram::Table {
  std::vector<Bin> bins;      // sorted by value
  std::vector<double> values;   // bins[i].first
  std::vector<uint32_t> position; // code -> index in bins
};

namespace {

// The bit lengths reConstruct knows how to decode; anything else is a bad header.
bool supportedBitLength(uint64_t bitLength) {
  return bitLength == 12 || bitLength == 13 || bitLength == 16 || bitLength == 20;
}

std::shared_ptr<const Histogram::Table> buildTable(uint64_t bitLength) {
  if (!supportedBitLength(bitLength)) {
    throw std::invalid_argument("Histogram: unsupported bit length "
                                + std::to_string(bitLength));
  }
  auto table = std::make_shared<Histogram::Table>();
  const uint64_t n = 1ULL << bitLength;
  table->bins.resize(n);
  for (uint64_t i = 0; i < n; ++i) {
    HistogramMetadata metadata;
    metadata.setInternalRepresentation(i);
    table->bins[i] = {reConstruct<double>(static_cast<unsigned int>(i),
                                          static_cast<int>(bitLength)),
                      metadata};
  }
  std::sort(table->bins.begin(), table->bins.end(),
            [](const Histogram::Bin &a, const Histogram::Bin &b) {
              return a.first < b.first;
            });
  table->values.reserve(n);
  table->position.assign(n, 0);
  for (size_t i = 0; i < n; ++i) {
    table->values.push_back(table->bins[i].first);
    table->position[table->bins[i].second.getInternalRepresentation()] = static_cast<uint32_t>(i);
  }
  return table;
}

std::shared_ptr<const Histogram::Table> tableFor(uint64_t bitLength) {
  static std::mutex mutex;
  static std::map<uint64_t, std::shared_ptr<const Histogram::Table>> cache;
  std::lock_guard<std::mutex> lock(mutex);
  auto it = cache.find(bitLength);
  if (it == cache.end()) {
    it = cache.emplace(bitLength, buildTable(bitLength)).first;
  }
  return it->second;
}

} // namespace

Histogram::Histogram(uint64_t bitLength)
    : bitLength_(bitLength), table_(tableFor(bitLength)) {}

const std::vector<double> &Histogram::sortedValues(uint64_t bitLength) {
  return tableFor(bitLength)->values;
}

size_t Histogram::positionOf(uint64_t code) const {
  return table_->position[code];
}

size_t Histogram::getBinIndex(double value) const {
  const auto &values = table_->values;
  auto idx = std::lower_bound(values.begin(), values.end(), value);
  // The value is guaranteed to be in the table because the histogram is
  // constructed to cover all possible values.
  return static_cast<size_t>(std::distance(values.begin(), idx));
}

uint64_t Histogram::getBinCount() const {
  return (1ULL << bitLength_);
}

const std::vector<Histogram::Bin> &Histogram::getBins() const {
  return table_->bins;
}

uint64_t Histogram::getInternalRepresentation(size_t index) const {
  return table_->bins[index].second.getInternalRepresentation();
}

double Histogram::getFPNumber(size_t index) const {
  if (index >= table_->bins.size()) {
    return std::numeric_limits<double>::infinity();
  }
  return table_->bins[index].first;
}

double Histogram::getBinLowerBound(size_t index) const {
  return getFPNumber(index);
}

double Histogram::getBinUpperBound(size_t index) const {
  return getFPNumber(index + 1);
}

uint8_t Histogram::getSignBit(size_t index) const {
  if (index >= table_->bins.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 1)) & 0x01;
}

uint8_t Histogram::getExponentSignBit(size_t index) const {
  if (index >= table_->bins.size()) {
    throw std::out_of_range("Index out of bounds");
  }
  uint64_t internal_rep = getInternalRepresentation(index);
  return (internal_rep >> (bitLength_ - 2)) & 0x01;
}
