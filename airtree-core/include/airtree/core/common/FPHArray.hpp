#ifndef AIRTREE_CORE_COMMON_FPHARRAY_HPP
#define AIRTREE_CORE_COMMON_FPHARRAY_HPP


#include <cstddef>
#include <cstdint>


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
  FPHArray(const void *vals, int len, FPH_dtype t)
      : values(vals), length(len), type(t) {}
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