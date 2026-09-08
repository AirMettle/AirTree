// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)
#include <airtree/query/meta/PopulatedBins.hpp>

#include <airtree/core/common/ConfigWire.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/EOF.hpp>

#include <stdexcept>

namespace airtree::query::meta {
namespace {

[[noreturn]] void truncated() { throw std::runtime_error("Truncated or malformed AirTree buffer"); }

// One node: its mask, and its counts decoded into `counts` (populated entries only are written).
template <size_t Bins>
void readNode(std::span<const char> buffer, size_t &offset, uint64_t *mask, uint32_t *counts) {
  if (!readPopulatedMask(buffer, offset, mask, Bins) || !deserializeCounts(buffer, offset, mask, Bins, counts))
    truncated();
}

// An interior node whose counts are not needed: its mask, skipping the counts.
template <size_t Bins>
void readMaskOnly(std::span<const char> buffer, size_t &offset, uint64_t *mask) {
  if (!readPopulatedMask(buffer, offset, mask, Bins) || !skipCounts(buffer, offset, mask, Bins))
    truncated();
}

} // namespace

PopulatedBinSet populatedBins(std::span<const char> buffer,
                              const airtree::core::common::AirTreeHeader &header,
                              const Histogram &histogram) {
  using airtree::core::common::ConfigWire;
  PopulatedBinSet out;
  size_t offset = header.header_length;
  if (offset > buffer.size())
    truncated();

  uint64_t rootMask[BINS_256 / 64];
  uint32_t rootCounts[BINS_256];
  readNode<BINS_256>(buffer, offset, rootMask, rootCounts);
  forEachSetBit(rootMask, BINS_256, [&](size_t i) { out.trieCount += rootCounts[i]; });
  out.bins.reserve(out.trieCount < 65536 ? out.trieCount : 65536);

  auto emit = [&](uint64_t code, uint32_t count) {
    if (count > 0)
      out.bins.push_back({histogram.positionOf(code), code, count});
  };

  switch (header.config) {
  case ConfigWire::Config_1D_Tiny: // 256 -> 32
    forEachSetBit(rootMask, BINS_256, [&](size_t i) {
      uint64_t mask;
      uint32_t counts[BINS_32];
      readNode<BINS_32>(buffer, offset, &mask, counts);
      forEachSetBit(&mask, BINS_32, [&](size_t k) { emit((i << 5) | k, counts[k]); });
    });
    break;
  case ConfigWire::Config_1D_Fast: // 256 -> 256
    forEachSetBit(rootMask, BINS_256, [&](size_t i) {
      uint64_t mask[BINS_256 / 64];
      uint32_t counts[BINS_256];
      readNode<BINS_256>(buffer, offset, mask, counts);
      forEachSetBit(mask, BINS_256, [&](size_t k) { emit((i << 8) | k, counts[k]); });
    });
    break;
  case ConfigWire::Config_1D_Precise: // 256 -> 64 -> 64
    forEachSetBit(rootMask, BINS_256, [&](size_t i) {
      uint64_t l1Mask;
      readMaskOnly<BINS_64>(buffer, offset, &l1Mask);
      forEachSetBit(&l1Mask, BINS_64, [&](size_t j) {
        uint64_t mask;
        uint32_t counts[BINS_64];
        readNode<BINS_64>(buffer, offset, &mask, counts);
        forEachSetBit(&mask, BINS_64, [&](size_t k) { emit((i << 12) | (j << 6) | k, counts[k]); });
      });
    });
    break;
  default:
    throw std::runtime_error("populatedBins: not a 1D configuration");
  }
  if (!verifyEndOfFileMarker(buffer, offset))
    truncated();
  sortByPosition(out.bins, histogram.getBinCount());
  return out;
}

} // namespace airtree::query::meta
