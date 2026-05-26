// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/common/FPHArray.hpp>


size_t getFPHTypeSize(FPH_dtype type) {
  switch (type) {
  case FPH_dtype::Double:
    return sizeof(double);
  case FPH_dtype::Float:
    return sizeof(float);
  case FPH_dtype::Int32:
    return sizeof(int32_t);
  case FPH_dtype::Int64:
    return sizeof(int64_t);
  default:
    return 0; // throw an exception here
  }
}