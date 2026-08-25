// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/merge/NWayMerge.hpp>
#include <airtree/merge/Logger.hpp>
#include <airtree/core/common/AirTreeHeader.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <airtree/core/serdes/Count.hpp>
#include <airtree/core/serdes/EOF.hpp>
#include <airtree/core/common/Bins.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

using airtree::core::common::AirTreeHeader;
using airtree::core::common::ConfigWire;

namespace airtree::merge {
namespace {

// A node is [mask words][width byte][packed counts]; a file is the header followed by the
// nodes in depth-first order and an EOF marker. `levels` = bins per level, root first.
struct Cursor {
  const std::vector<char> *buf;
  size_t offset;
};

struct LevelScratch {
  size_t words = 0;
  std::vector<uint64_t> masks; // one mask per input, indexed [input * words]
  std::vector<uint64_t> merged;
  std::vector<uint32_t> acc, tmp;
  std::vector<uint32_t> sub; // inputs that populate the child being merged
};

[[noreturn]] void corrupt(const char *what) {
  SPDLOG_LOGGER_ERROR(logger(), "mergeAirTrees: {}", what);
  throw std::runtime_error(std::string("mergeAirTrees: ") + what);
}

bool skipSubtree(const std::vector<size_t> &levels, size_t level,
                 const std::vector<char> &buf, size_t &offset) {
  uint64_t mask[64];
  const size_t bins = levels[level];
  if (!readPopulatedMask(buf, offset, mask, bins) || !skipCounts(buf, offset, mask, bins))
    return false;
  if (level + 1 == levels.size())
    return true;
  bool ok = true;
  forEachSetBit(mask, bins, [&](size_t) {
    if (ok)
      ok = skipSubtree(levels, level + 1, buf, offset);
  });
  return ok;
}

// Merges the subtrees at the current cursor of every input in `have`, appending to `out`.
void mergeSubtree(const std::vector<size_t> &levels, size_t level,
                  std::vector<Cursor> &cursors, const std::vector<uint32_t> &have,
                  std::vector<LevelScratch> &scratch, std::vector<char> &out) {
  const size_t bins = levels[level];
  if (have.size() == 1) { // unshared subtree: canonical encoding, copy the bytes verbatim
    Cursor &c = cursors[have[0]];
    const size_t start = c.offset;
    if (!skipSubtree(levels, level, *c.buf, c.offset))
      corrupt("truncated input");
    out.insert(out.end(), c.buf->begin() + start, c.buf->begin() + c.offset);
    return;
  }
  LevelScratch &L = scratch[level];
  const size_t words = L.words;
  std::fill(L.merged.begin(), L.merged.end(), 0);
  std::fill(L.acc.begin(), L.acc.end(), 0);
  for (uint32_t k : have) {
    Cursor &c = cursors[k];
    uint64_t *mask = &L.masks[k * words];
    if (!readPopulatedMask(*c.buf, c.offset, mask, bins)
        || !deserializeCounts(*c.buf, c.offset, mask, bins, L.tmp.data()))
      corrupt("truncated input");
    for (size_t w = 0; w < words; ++w)
      L.merged[w] |= mask[w];
    forEachSetBit(mask, bins, [&](size_t i) { L.acc[i] += L.tmp[i]; });
  }
  writePopulatedMask(L.merged.data(), bins, out);
  serializeCounts(L.merged.data(), bins, L.acc.data(), out);
  if (level + 1 == levels.size())
    return;
  forEachSetBit(L.merged.data(), bins, [&](size_t i) {
    L.sub.clear();
    for (uint32_t k : have)
      if ((L.masks[k * words + i / 64] >> (i % 64)) & 1u)
        L.sub.push_back(k);
    mergeSubtree(levels, level + 1, cursors, L.sub, scratch, out);
  });
}

} // namespace

std::vector<size_t> streamingLevels(ConfigWire config) {
  switch (config) {
  case ConfigWire::Config_1D_Tiny:
    return {BINS_256, BINS_32};
  case ConfigWire::Config_1D_Fast:
    return {BINS_256, BINS_256};
  case ConfigWire::Config_1D_Precise:
    return {BINS_256, BINS_64, BINS_64};
  default:
    return {};
  }
}

std::vector<char> mergeStreaming(const std::vector<std::vector<char>> &buffers,
                                 const std::vector<AirTreeHeader> &headers,
                                 const std::vector<size_t> &levels) {
  const size_t n = buffers.size();
  std::vector<Cursor> cursors(n);
  for (size_t k = 0; k < n; ++k)
    cursors[k] = {&buffers[k], headers[k].header_length};
  std::vector<LevelScratch> scratch(levels.size());
  for (size_t l = 0; l < levels.size(); ++l) {
    auto &L = scratch[l];
    L.words = (levels[l] + 63) / 64;
    L.masks.assign(n * L.words, 0);
    L.merged.assign(L.words, 0);
    L.acc.assign(levels[l], 0);
    L.tmp.assign(levels[l], 0);
    L.sub.reserve(n);
  }
  AirTreeHeader merged = headers[0];
  for (size_t k = 1; k < n; ++k) {
    merged.trie_count += headers[k].trie_count;
    merged.pos_inf_count += headers[k].pos_inf_count;
    merged.neg_inf_count += headers[k].neg_inf_count;
    merged.pos_zero_count += headers[k].pos_zero_count;
    merged.neg_zero_count += headers[k].neg_zero_count;
    merged.nan_count += headers[k].nan_count;
  }
  std::vector<char> out;
  size_t total = 0;
  for (const auto &b : buffers)
    total += b.size();
  out.reserve(total);
  airtree::core::common::serializeHeader(merged, out);
  const size_t header_end = out.size();

  std::vector<uint32_t> all(n);
  for (size_t k = 0; k < n; ++k)
    all[k] = static_cast<uint32_t>(k);
  mergeSubtree(levels, 0, cursors, all, scratch, out);
  for (auto &c : cursors)
    if (!verifyEndOfFileMarker(*c.buf, c.offset))
      corrupt("input has no EOF marker where the trie ends");

  const int32_t eof = -1;
  const auto *eofBytes = reinterpret_cast<const char *>(&eof);
  out.insert(out.end(), eofBytes, eofBytes + sizeof(eof));
  airtree::core::common::finalizeHeader(out, out.size() - header_end);
  return out;
}

std::vector<char> mergeAirTrees(const std::vector<std::vector<char>> &buffers) {
  if (buffers.empty())
    throw std::invalid_argument("mergeAirTrees: no buffers");
  std::vector<AirTreeHeader> headers;
  headers.reserve(buffers.size());
  for (const auto &b : buffers)
    headers.push_back(airtree::core::common::deserializeHeader(b));
  for (const auto &h : headers)
    if (h.config != headers[0].config)
      throw std::runtime_error("mergeAirTrees: buffer configuration mismatch");
  if (buffers.size() == 1)
    return buffers[0];

  const auto levels = streamingLevels(headers[0].config);
  if (!levels.empty())
    return mergeStreaming(buffers, headers, levels);
  SPDLOG_LOGGER_DEBUG(logger(), "mergeAirTrees: pairwise fold for config 0x{:02x}",
                      static_cast<uint8_t>(headers[0].config));
  std::vector<char> acc = buffers[0];
  for (size_t k = 1; k < buffers.size(); ++k)
    acc = mergeAirTree(acc, buffers[k]);
  return acc;
}

} // namespace airtree::merge
