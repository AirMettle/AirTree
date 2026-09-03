// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_COMMON_SPARSENODE_HPP
#define AIRTREE_CORE_COMMON_SPARSENODE_HPP

#include <airtree/core/common/Populated.hpp>
#include <airtree/core/serdes/BooleanArray.hpp>
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

#if defined(__SSE2__) || defined(_M_X64)
#include <emmintrin.h>
#define AIRTREE_SPARSE_SSE2 1
#endif

// trie node that keeps up to 32 populated slots inline and switches to
// full-width arrays beyond that
namespace sparse_node {
constexpr std::size_t kInline = 32;
constexpr uint16_t kEmpty = 0xFFFF;

inline int findSlot(const uint16_t *slots, uint16_t slot) {
#ifdef AIRTREE_SPARSE_SSE2
  const __m128i key = _mm_set1_epi16(static_cast<short>(slot));
  uint64_t mask = 0;
  for (std::size_t i = 0; i < kInline; i += 8) {
    const __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i *>(slots + i));
    mask |= static_cast<uint64_t>(_mm_movemask_epi8(_mm_cmpeq_epi16(v, key))) << (2 * i);
  }
  return mask ? static_cast<int>(std::countr_zero(mask) / 2) : -1;
#else
  int idx = -1;
  for (std::size_t i = 0; i < kInline; ++i)
    idx = slots[i] == slot ? static_cast<int>(i) : idx;
  return idx;
#endif
}
} // namespace sparse_node

template <std::size_t N, class Child>
struct SparseNode {
  struct Entry {
    uint16_t slot;
    uint32_t count;
    std::unique_ptr<Child> child;
  };
  struct Dense {
    uint32_t counts[N] = {0};
    std::unique_ptr<Child> nodes[N];
  };
  struct Ref {
    uint32_t &count;
    std::unique_ptr<Child> &child;
  };

  PopulatedBins<N> populated;
  uint16_t slots[sparse_node::kInline];
  uint32_t counts[sparse_node::kInline] = {0};
  std::unique_ptr<Child> children[sparse_node::kInline];
  std::unique_ptr<Dense> dense;
  uint8_t n = 0;

  SparseNode() { std::memset(slots, 0xFF, sizeof(slots)); }

  Ref at(std::size_t slot) {
    if (dense)
      return {dense->counts[slot], dense->nodes[slot]};
    const int i = sparse_node::findSlot(slots, static_cast<uint16_t>(slot));
    if (i >= 0)
      return {counts[i], children[i]};
    if (n < sparse_node::kInline) {
      slots[n] = static_cast<uint16_t>(slot);
      return {counts[n], children[n++]};
    }
    toDense();
    return {dense->counts[slot], dense->nodes[slot]};
  }
  uint32_t &countRef(std::size_t slot) { return at(slot).count; }
  std::unique_ptr<Child> &childRef(std::size_t slot) { return at(slot).child; }

  uint32_t count(std::size_t slot) const {
    if (dense)
      return dense->counts[slot];
    const int i = sparse_node::findSlot(slots, static_cast<uint16_t>(slot));
    return i >= 0 ? counts[i] : 0;
  }
  Child *child(std::size_t slot) const {
    if (dense)
      return dense->nodes[slot].get();
    const int i = sparse_node::findSlot(slots, static_cast<uint16_t>(slot));
    return i >= 0 ? children[i].get() : nullptr;
  }

  void load(const uint64_t *mask, const uint32_t *values) {
    setPopulated(populated, mask);
    if (populated.count() > sparse_node::kInline) {
      dense = std::make_unique<Dense>();
      forEachSetBit(mask, N, [&](std::size_t i) { dense->counts[i] = values[i]; });
      return;
    }
    forEachSetBit(mask, N, [&](std::size_t i) {
      slots[n] = static_cast<uint16_t>(i);
      counts[n++] = values[i];
    });
  }

  template <class Fn> void forEach(Fn &&fn) const {
    if (dense) {
      forEachSetBit(populated.words, N, [&](std::size_t i) { fn(i, dense->counts[i], dense->nodes[i].get()); });
      return;
    }
    uint8_t order[sparse_node::kInline];
    sortedOrder(order);
    for (std::size_t k = 0; k < n; ++k)
      fn(slots[order[k]], counts[order[k]], children[order[k]].get());
  }
  template <class Fn> void forEachMut(Fn &&fn) {
    if (dense) {
      forEachSetBit(populated.words, N, [&](std::size_t i) { fn(i, dense->counts[i], dense->nodes[i]); });
      return;
    }
    uint8_t order[sparse_node::kInline];
    sortedOrder(order);
    for (std::size_t k = 0; k < n; ++k)
      fn(slots[order[k]], counts[order[k]], children[order[k]]);
  }

private:
  void sortedOrder(uint8_t *order) const {
    for (std::size_t k = 0; k < n; ++k)
      order[k] = static_cast<uint8_t>(k);
    std::sort(order, order + n, [&](uint8_t a, uint8_t b) { return slots[a] < slots[b]; });
  }
  void toDense() {
    dense = std::make_unique<Dense>();
    for (std::size_t k = 0; k < n; ++k) {
      dense->counts[slots[k]] = counts[k];
      dense->nodes[slots[k]] = std::move(children[k]);
    }
  }
};

template <std::size_t N>
struct SparseLeaf {
  struct Entry {
    uint16_t slot;
    uint32_t count;
  };
  struct Dense {
    uint32_t counts[N] = {0};
  };

  PopulatedBins<N> populated;
  uint16_t slots[sparse_node::kInline];
  uint32_t counts[sparse_node::kInline] = {0};
  std::unique_ptr<Dense> dense;
  uint8_t n = 0;

  SparseLeaf() { std::memset(slots, 0xFF, sizeof(slots)); }

  uint32_t &countRef(std::size_t slot) {
    if (dense)
      return dense->counts[slot];
    const int i = sparse_node::findSlot(slots, static_cast<uint16_t>(slot));
    if (i >= 0)
      return counts[i];
    if (n < sparse_node::kInline) {
      slots[n] = static_cast<uint16_t>(slot);
      return counts[n++];
    }
    dense = std::make_unique<Dense>();
    for (std::size_t k = 0; k < n; ++k)
      dense->counts[slots[k]] = counts[k];
    return dense->counts[slot];
  }
  uint32_t count(std::size_t slot) const {
    if (dense)
      return dense->counts[slot];
    const int i = sparse_node::findSlot(slots, static_cast<uint16_t>(slot));
    return i >= 0 ? counts[i] : 0;
  }

  void load(const uint64_t *mask, const uint32_t *values) {
    setPopulated(populated, mask);
    if (populated.count() > sparse_node::kInline) {
      dense = std::make_unique<Dense>();
      forEachSetBit(mask, N, [&](std::size_t i) { dense->counts[i] = values[i]; });
      return;
    }
    forEachSetBit(mask, N, [&](std::size_t i) {
      slots[n] = static_cast<uint16_t>(i);
      counts[n++] = values[i];
    });
  }

  template <class Fn> void forEach(Fn &&fn) const {
    if (dense) {
      forEachSetBit(populated.words, N, [&](std::size_t i) { fn(i, dense->counts[i]); });
      return;
    }
    uint8_t order[sparse_node::kInline];
    for (std::size_t k = 0; k < n; ++k)
      order[k] = static_cast<uint8_t>(k);
    std::sort(order, order + n, [&](uint8_t a, uint8_t b) { return slots[a] < slots[b]; });
    for (std::size_t k = 0; k < n; ++k)
      fn(slots[order[k]], counts[order[k]]);
  }
};

namespace sparse_node {
template <class Node>
inline void bumpSlot(Node &node, unsigned int slot, uint64_t &curr_trie_size) {
  node.populated.set(slot);
  uint32_t &count = node.countRef(slot);
  if (count == 0)
    curr_trie_size += sizeof(typename Node::Entry);
  count++;
}
template <class Child, class Node>
inline Child *descendInto(Node &node, unsigned int slot, uint64_t &curr_trie_size) {
  node.populated.set(slot);
  auto ref = node.at(slot);
  if (ref.count == 0)
    curr_trie_size += sizeof(typename Node::Entry);
  ref.count++;
  if (!ref.child) {
    ref.child = std::make_unique<Child>();
    curr_trie_size += sizeof(Child);
  }
  return ref.child.get();
}
} // namespace sparse_node

#endif // AIRTREE_CORE_COMMON_SPARSENODE_HPP
