#ifndef AIRTREE_CORE_COMMON_FPHARRAY_HPP
#define AIRTREE_CORE_COMMON_FPHARRAY_HPP


#include <cstddef>
#include <cstdint>
#include <vector>
#include <type_traits>


enum class FPH_dtype {
  Double,
  Float,
  Int32,
  Int64

};

struct FPHArray {
  const void *values{};
  int length{};
  FPH_dtype type;

  FPHArray() = default;
  // Keeping this around for legacy reasons.
  FPHArray(const void *vals, int len, FPH_dtype t)
      : values(vals), length(len), type(t) {}

  template <typename T>
  FPHArray(const std::vector<T> &vals)
      : values(vals.data()), length(vals.size()) {
    if constexpr (std::is_same_v<T, double>)
      type = FPH_dtype::Double;
    else if constexpr (std::is_same_v<T, float>)
      type = FPH_dtype::Float;
    else if constexpr (std::is_same_v<T, int32_t>)
      type = FPH_dtype::Int32;
    else if constexpr (std::is_same_v<T, int64_t>)
      type = FPH_dtype::Int64;
    else
      static_assert(sizeof(T) == 0, "Unsupported data type for FPHArray");
  }
};

inline FPHArray buildFPHArray(const double *values, int length) {
  return {values, length, FPH_dtype::Double};
}

inline FPHArray buildFPHArray(const float *values, int length) {
  return {values, length, FPH_dtype::Float};
}

inline FPHArray buildFPHArray(const int32_t *values, int length) {
  return {values, length, FPH_dtype::Int32};
}

inline FPHArray buildFPHArray(const int64_t *values, int length) {
  return {values, length, FPH_dtype::Int64};
}

size_t getFPHTypeSize(FPH_dtype type);

#endif // AIRTREE_CORE_COMMON_FPHARRAY_HPP