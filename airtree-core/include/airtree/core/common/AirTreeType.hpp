#ifndef AIRTREE_CORE_COMMON_AIRTREETYPE_HPP
#define AIRTREE_CORE_COMMON_AIRTREETYPE_HPP

#include <airtree/core/serdes/trie1d/1DxF.hpp>
#include <airtree/core/serdes/trie1d/1DxP.hpp>
#include <airtree/core/serdes/trie1d/1DxT.hpp>
#include <airtree/core/schema/trie2d/2DxF.hpp>
#include <airtree/core/schema/trie2d/2DxP.hpp>
#include <airtree/core/schema/trie3d/3DxF.hpp>
#include <airtree/core/schema/trie3d/3DxP.hpp>
#include <airtree/core/schema/trie4d/4DxP.hpp>
#include <airtree/core/schema/trie4d/4DxF.hpp>

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace airtree::core::common {

/**
 * AirTreeType
 * --------
 * A small type-erased holder around a std::variant of std::unique_ptrs to the
 * current supported node types.
 *
 * Key points:
 *  - Default state is std::monostate (clearly "empty").
 *  - Each alternative stores exclusive ownership (std::unique_ptr<T>).
 *  - Access patterns are explicit about ownership:
 *      * get<U>()      -> MOVE OUT (transfer ownership, variant is left with
 * nullptr for U)
 *      * borrow<U>()   -> raw pointer (non-owning, nullptr if wrong/empty)
 *      * get_ptr<U>()  -> reference to the stored unique_ptr (owning handle
 * in-place)
 *
 * Failure behavior:
 *  - get<U>() never throws; returns empty unique_ptr if wrong/empty.
 *  - borrow<U>() never throws; returns nullptr if wrong/empty.
 *  - get_ptr<U>() throws std::bad_variant_access if the active alternative is
 * not std::unique_ptr<U>.
 */
class AirTreeType {
public:
  // The underlying discriminated union. First alternative is std::monostate to
  // represent "no value".
  using Variant =
      std::variant<std::monostate, std::unique_ptr<TrieNode_13>,
                   std::unique_ptr<TrieNode_16>, std::unique_ptr<TrieNode_20>,
                   std::unique_ptr<TLETrieNode_2D>,
                   std::unique_ptr<TLEoption3_2D>, std::unique_ptr<TLE_3D_888>,
                   std::unique_ptr<TLE_3D_3x10>, std::unique_ptr<TLE_4D_4x8>,
                   std::unique_ptr<TLE_4D_4x10>>;

  // Default-constructs to monostate (empty).
  AirTreeType() = default;

  /**
   * Construct from a unique_ptr<U> of an allowed type U.
   * This sets the active variant alternative to std::unique_ptr<U>.
   */
  template <typename U> explicit AirTreeType(std::unique_ptr<U> ptr) {
    set(std::move(ptr));
  }

  // ---------- Type queries ----------

  /**
   * Returns true if the active alternative is std::unique_ptr<U> AND the
   * pointer is non-null. Never throws.
   */
  template <typename U> [[nodiscard]] bool is() const noexcept {
    if (!std::holds_alternative<std::unique_ptr<U>>(type_))
      return false;
    const auto &up = std::get<std::unique_ptr<U>>(type_);
    return static_cast<bool>(up);
  }

  /**
   * True if the variant is "valueless by exception" (per std::variant
   * semantics). This should be rare; indicates an exception occurred during a
   * previous operation.
   */
  [[nodiscard]] bool valueless_by_exception() const noexcept {
    return type_.valueless_by_exception();
  }

  /**
   * Returns true if we currently hold any non-null unique_ptr (i.e., not
   * monostate and not null). Never throws.
   */
  [[nodiscard]] bool has_value() const noexcept {
    return std::visit(
        [](auto const &alt) -> bool {
          using Alt = std::decay_t<decltype(alt)>;
          if constexpr (std::is_same_v<Alt, std::monostate>) {
            return false; // empty
          } else {
            return static_cast<bool>(alt); // true if unique_ptr<T> is non-null
          }
        },
        type_);
  }

  // ---------- Accessors ----------

  /**
   * get<U>()
   * Ownership transfer accessor.
   *
   * If the active alternative is std::unique_ptr<U>:
   *   - Moves the unique_ptr<U> OUT of the variant and returns it.
   *   - The stored unique_ptr<U> becomes nullptr.
   * Otherwise:
   *   - Returns an empty unique_ptr<U>.
   *
   * Never throws (uses std::get_if).
   *
   * Use this ONLY when you truly intend to consume the node and take ownership.
   */
  template <typename U> [[nodiscard]] std::unique_ptr<U> get() noexcept {
    auto *p = std::get_if<std::unique_ptr<U>>(&type_);
    if (!p || !*p)
      return {};
    return std::move(*p); // move out; variant now holds nullptr for U
  }

  /**
   * borrow<U>()
   * Non-owning accessor.
   *
   * Returns a raw pointer to U if the active alternative is std::unique_ptr<U>
   * AND it is non-null. Returns nullptr if wrong alternative or null.
   *
   * Never throws.
   *
   * Use when you just need to read/use the node without taking ownership.
   */
  template <typename U> [[nodiscard]] U *borrow() noexcept {
    auto *p = std::get_if<std::unique_ptr<U>>(&type_);
    return (p && *p) ? p->get() : nullptr;
  }

  /// const overload of borrow<U>()
  template <typename U> [[nodiscard]] const U *borrow() const noexcept {
    auto const *p = std::get_if<std::unique_ptr<U>>(&type_);
    return (p && *p) ? p->get() : nullptr;
  }

  /**
   * get_ptr<U>()
   * Owning-handle accessor.
   *
   * Returns a reference to the stored std::unique_ptr<U>. This DOES NOT
   * transfer ownership. Throws std::bad_variant_access if the active
   * alternative isn't std::unique_ptr<U>.
   *
   * Use when you need to operate on the owning handle in-place (e.g., reset(),
   * replace(), .get()).
   */
  template <typename U> [[nodiscard]] std::unique_ptr<U> &get_ptr() & {
    return std::get<std::unique_ptr<U>>(
        type_); // may throw std::bad_variant_access
  }

  /// const overload of get_ptr<U>()
  template <typename U>
  [[nodiscard]] const std::unique_ptr<U> &get_ptr() const & {
    return std::get<std::unique_ptr<U>>(
        type_); // may throw std::bad_variant_access
  }

  // ---------- Mutators ----------

  /**
   * set<U>(unique_ptr<U> ptr)
   * Replace whatever we hold with a unique_ptr<U>.
   * Constrained to allowed node types via static_assert.
   */
  template <typename U> void set(std::unique_ptr<U> ptr) {
    static_assert(
        std::disjunction_v<
            std::is_same<U, TrieNode_13>, std::is_same<U, TrieNode_16>,
            std::is_same<U, TrieNode_20>, std::is_same<U, TLETrieNode_2D>,
            std::is_same<U, TLEoption3_2D>, std::is_same<U, TLE_3D_888>,
            std::is_same<U, TLE_3D_3x10>, std::is_same<U, TLE_4D_4x8>,
            std::is_same<U, TLE_4D_4x10>>,
        "Type not allowed in AirTreeType");
    type_ = std::move(ptr);
  }

  /**
   * emplace<U>(args...)
   * Convenience: construct a U and set it in one shot.
   * Example: auto holder = AirTreeType::emplace<TrieNode_20>(ctor_args...);
   */
  template <typename U, typename... Args>
  [[nodiscard]] static AirTreeType emplace(Args &&...args) {
    AirTreeType t;
    t.set(std::make_unique<U>(std::forward<Args>(args)...));
    return t;
  }

  /**
   * reset()
   * Release any owned object (if present) and return to monostate (empty).
   * Never throws.
   */
  void reset() noexcept {
    // First, if we currently hold a unique_ptr<T>, reset it (to release
    // ownership).
    std::visit(
        [](auto &alt) {
          using Alt = std::decay_t<decltype(alt)>;
          if constexpr (!std::is_same_v<Alt, std::monostate>) {
            alt.reset(); // no-op on null; releases if non-null
          }
        },
        type_);
    // Then set the variant back to monostate as the active alternative.
    type_.emplace<0>(); // 0 == index of std::monostate
  }

  /**
   * visit(F)
   * Visitor that exposes a raw pointer to the stored node (or nullptr if
   * empty/monostate). Useful to unify code paths when the concrete type isn't
   * known at callsite, but you only need a pointer to the base API.
   *
   * Example:
   *   trie_node_.visit([&](auto* p) {
   *       if (!p) { ... }
   *       // use p->...
   *   });
   */
  template <typename F> decltype(auto) visit(F &&f) {
    return std::visit(
        [&](auto &alt) -> decltype(auto) {
          using Alt = std::decay_t<decltype(alt)>;
          if constexpr (std::is_same_v<Alt, std::monostate>) {
            return f(static_cast<void *>(nullptr));
          } else {
            return f(alt.get()); // raw pointer (may be nullptr)
          }
        },
        type_);
  }

private:
  Variant type_; // the discriminated union holding our owning handle
};

} // namespace airtree::core::common

#endif // AIRTREE_CORE_COMMON_AIRTREETYPE_HPP
