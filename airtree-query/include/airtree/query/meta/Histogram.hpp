// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_HISTOGRAM_HPP
#define AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_HISTOGRAM_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace airtree::query::meta {

class HistogramMetadata {
public:
  uint64_t getInternalRepresentation() const;
  void setInternalRepresentation(uint64_t representation);

private:
  uint64_t internal_representation_;
};

/**
 * Class representing an AirMettle histogram for floating-point values.
 * The histogram is constructed based on the bit length and contains bins
 * with associated metadata.
 * The bit length represents the length of the value's internal representation.
 * For example 1DxT has a bit length of 13 since the input values float32,
 * float64, int32, and int64 are filed into 13 bit representations.
 * The histogram can be used to retrieve bins, their internal representations,
 * and the actual (close approximation) floating-point numbers they represent.
 * The bin table depends only on the bit length and is shared by all instances.
 */
class Histogram {
public:
  using Bin = std::pair<double, HistogramMetadata>;

  Histogram(uint64_t bitLength);

  /**
   * Get the number of bins in the histogram.
   * @return The number of bins.
   */
  uint64_t getBinCount() const;

  /**
   * Get the bins of the histogram, sorted by value.
   * Each bin is a pair of a floating-point number and its associated metadata.
   * @return The shared, immutable bin table.
   */
  const std::vector<Bin> &getBins() const;

  /**
   * Get the index of the bin that contains the specified value.
   * @param value The value to find the bin for.
   * @return The index of the bin containing the value.
   */
  size_t getBinIndex(double value) const;

  /**
   * Get the internal representation of the bin at the specified index.
   * @param index The index of the bin.
   * @return The internal representation of the bin.
   */
  uint64_t getInternalRepresentation(size_t index) const;

  /**
   * Get the floating-point number represented by the bin at the specified
   * index.
   * @param index The index of the bin.
   * @return The floating-point number represented by the bin.
   */
  double getFPNumber(size_t index) const;

  /**
   * Get the smallest value that can be filed into the bin at the specified
   * index.
   * @param index The index of the bin.
   * @return The lower bound of the bin.
   */
  double getBinLowerBound(size_t index) const;

  /**
   * Get the largest value that can be filed into the bin at the specified
   * index.
   * @param index The index of the bin.
   * @return The upper bound of the bin.
   */
  double getBinUpperBound(size_t index) const;

  /**
   * Returns the sign bit of the bin at the specified index.
   * A sign bit of 0 indicates a positive value, and a sign bit of 1 indicates
   * a negative value.
   * @param index The index of the bin.
   * @return The sign bit of the bin.
   */
  uint8_t getSignBit(size_t index) const;

  /**
   * Returns the exponent sign bit of the bin at the specified index.
   * An exponent sign bit of 0 indicates a positive exponent, and an
   * exponent sign bit of 1 indicates a negative exponent.
   * @param index The index of the bin.
   * @return The exponent sign bit of the bin.
   */
  uint8_t getExponentSignBit(size_t index) const;

  /** All bin values for a bit length, sorted ascending. */
  static const std::vector<double> &sortedValues(uint64_t bitLength);

  struct Table;

private:
  uint64_t bitLength_;
  std::shared_ptr<const Table> table_;
};

} // namespace airtree::query::meta

#endif // AIRTREE_QUERY_INCLUDE_AIRTREE_QUERY_META_HISTOGRAM_HPP
