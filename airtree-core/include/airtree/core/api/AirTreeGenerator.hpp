// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_CORE_API_AIRTREEGENERATOR_HPP
#define AIRTREE_CORE_API_AIRTREEGENERATOR_HPP

#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/core/common/ConfigWire.hpp>
#include <stdexcept>

namespace airtree::core::api {
enum class ConfigType {
  XP,
  XT,
  XF,
  XNUM,
};

struct AirTreeOptions {
  int dimensions = 1;
  ConfigType type = ConfigType::XP;
};

class AirTreeGenerator {
public:
  virtual ~AirTreeGenerator() = default;
  [[nodiscard]] virtual std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays) const = 0;
};

class AirTreeGeneratorRegistry {
public:
  using Factory = std::function<std::unique_ptr<AirTreeGenerator>()>;

  static AirTreeGeneratorRegistry &instance();

  // Register a generator + its params for a given wire byte. Idempotent: a
  // second registration for the same wire silently overwrites.
  void registerGenerator(airtree::core::common::ConfigWire wire,
                         airtree::core::common::ConfigParams params,
                         Factory factory);

  [[nodiscard]] std::unique_ptr<AirTreeGenerator>
  create(airtree::core::common::ConfigWire wire) const;

  [[nodiscard]] std::optional<airtree::core::common::ConfigParams>
  lookupParams(uint8_t wire) const;

private:
  AirTreeGeneratorRegistry() = default;
  std::unordered_map<uint8_t, airtree::core::common::ConfigParams> params_;
  std::unordered_map<uint8_t, Factory> factories_;
};

class AirTreeGeneratorFactory {
public:
  static std::unique_ptr<AirTreeGenerator>
  create(const AirTreeOptions &options);


  static std::unique_ptr<AirTreeGenerator>
  create(airtree::core::common::ConfigWire wire);
};

[[nodiscard]] inline std::vector<char>
generate(const std::vector<const FPHArray *> &arrays,
         const AirTreeOptions &options) {
  auto generator = AirTreeGeneratorFactory::create(options);
  return generator->generate(arrays);
}

// 1D Convenience Overload
template <typename T>
[[nodiscard]] inline std::vector<char> generate(const std::vector<T> &array,
                                                AirTreeOptions options = {}) {
  options.dimensions = 1; // Auto-correct dimensions just in case!
  FPHArray fph(array);
  return generate({&fph}, options); // Wraps in vector and delegates
}

// 2D Convenience Overload
template <typename T1, typename T2>
[[nodiscard]] inline std::vector<char> generate(const std::vector<T1> &array1,
                                                const std::vector<T2> &array2,
                                                AirTreeOptions options = {}) {
  options.dimensions = 2; // Auto-correct dimensions just in case!
  FPHArray fph1(array1), fph2(array2);
  return generate({&fph1, &fph2}, options);
}

// 3D Convenience Overload...
template <typename T1, typename T2, typename T3>
[[nodiscard]] inline std::vector<char>
generate(const std::vector<T1> &array1, const std::vector<T2> &array2,
         const std::vector<T3> &array3, AirTreeOptions options = {}) {
  options.dimensions = 3;
  FPHArray fph1(array1), fph2(array2), fph3(array3);
  return generate({&fph1, &fph2, &fph3}, options);
}

// 4D Convenience Overload...
template <typename T1, typename T2, typename T3, typename T4>
[[nodiscard]] inline std::vector<char>
generate(const std::vector<T1> &array1, const std::vector<T2> &array2,
         const std::vector<T3> &array3, const std::vector<T4> &array4,
         AirTreeOptions options = {}) {
  options.dimensions = 4;
  FPHArray fph1(array1), fph2(array2), fph3(array3), fph4(array4);
  return generate({&fph1, &fph2, &fph3, &fph4}, options);
}

template <typename Func>
void dispatchFPHArray(const FPHArray &array, Func &&process_func) {
  switch (array.type) {
  case FPH_dtype::Double: {
    process_func(static_cast<const double *>(array.values));
  } break;
  case FPH_dtype::Float: {
    process_func(static_cast<const float *>(array.values));
  } break;
  case FPH_dtype::Int32: {
    process_func(static_cast<const int32_t *>(array.values));
  } break;
  case FPH_dtype::Int64: {
    process_func(static_cast<const int64_t *>(array.values));
  } break;
  default:
    throw std::runtime_error("Unknown data type in FPHArray");
    break;
  }
}

} // namespace airtree::core::api

#endif // AIRTREE_CORE_API_AIRTREEGENERATOR_HPP
