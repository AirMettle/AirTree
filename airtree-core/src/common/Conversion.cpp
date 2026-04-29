#include <airtree/core/common/Conversion.hpp>
#ifdef _MSC_VER
#include <immintrin.h>
#endif

float int32_to_float(int32_t value) {
  float result;

#ifdef _MSC_VER
  result = _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(), value));
#elif defined(x86_ARCH)
  __asm__(
      "cvtsi2ss %1, %0" // Convert int32 to float32
      : "=x"(result)    // Output: xmm register holding the result
      : "r"(value) // Input: general-purpose register holding the int32 value
  );
#elif ARM_ARCH
  __asm__(
      "scvtf %s0, %w1" // Convert int32 to float32
      : "=w"(result)   // Output: scalar floating-point register
      : "r"(value) // Input: general-purpose register holding the int32 value
  );
#endif

  return result;
}

float int64_to_float(int64_t value) {
  float result;

#ifdef _MSC_VER
  result = _mm_cvtss_f32(_mm_cvtsi64_ss(_mm_setzero_ps(), value));
#elif defined(x86_ARCH)
  __asm__(
      "cvtsi2ss %1, %0" // Convert int64 to float32
      : "=x"(result)    // Output: xmm register holding the result
      : "r"(value) // Input: general-purpose register holding the int64 value
  );
#elif ARM_ARCH
  __asm__(
      "scvtf %s0, %x1" // Convert int64 to float32
      : "=w"(result)   // Output: scalar floating-point register
      : "r"(value) // Input: general-purpose register holding the int64 value
  );
#endif

  return result;
}

double int32_to_double(int32_t value) {
  double result;
#ifdef _MSC_VER
  result = _mm_cvtsd_f64(_mm_cvtsi32_sd(_mm_setzero_pd(), value));
#elif defined(x86_ARCH)
  asm("cvtsi2sd %1, %0" // Convert int32 to double (64-bit)
      : "=x"(result)    // Output: xmm register holding the result
      : "r"(value) // Input: general-purpose register holding the int32 value
  );
#elif ARM_ARCH
  asm("scvtf %d0, %w1" // Convert int32 to double (d0 = double precision)
      : "=w"(result)   // Output: scalar floating-point register
      : "r"(value) // Input: general-purpose register holding the int32 value
  );
#endif
  return result;
}

double int64_to_double(int64_t value) {
  double result;
#ifdef _MSC_VER
  result = _mm_cvtsd_f64(_mm_cvtsi64_sd(_mm_setzero_pd(), value));
#elif defined(x86_ARCH)
  asm("cvtsi2sd %1, %0" // Convert int64 to double (64-bit)
      : "=x"(result)    // Output: xmm register holding the result
      : "r"(value) // Input: general-purpose register holding the int64 value
  );
#elif ARM_ARCH
  asm("scvtf %d0, %x1" // Convert int64 to double (x1 = 64-bit register)
      : "=w"(result)   // Output: scalar floating-point register
      : "r"(value) // Input: general-purpose register holding the int64 value
  );
#endif
  return result;
}