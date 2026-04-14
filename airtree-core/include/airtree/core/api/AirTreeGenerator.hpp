#ifndef AIRTREE_CORE_API_AIRTREEGENERATOR_HPP
#define AIRTREE_CORE_API_AIRTREEGENERATOR_HPP

#include <vector>
#include <memory>

class FPHArray; // Forward declaration of FPHArray

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
  bool default_mode = true;
};

class AirTreeGenerator {
public:
  virtual ~AirTreeGenerator() = default;
  [[nodiscard]] virtual std::vector<char>
  generate(const std::vector<const FPHArray *> &arrays,
           bool default_mode) const = 0;
};

class AirTreeGeneratorFactory {
public:
  static std::unique_ptr<AirTreeGenerator>
  create(const AirTreeOptions &options);
};

[[nodiscard]] inline std::vector<char>
generate(const std::vector<const FPHArray *> &arrays,
         const AirTreeOptions &options) {
  auto generator = AirTreeGeneratorFactory::create(options);
  return generator->generate(arrays, options.default_mode);
}

// 1D Convenience Overload
[[nodiscard]] inline std::vector<char> generate(const FPHArray &array,
                                                AirTreeOptions options = {}) {
  options.dimensions = 1;             // Auto-correct dimensions just in case!
  return generate({&array}, options); // Wraps in vector and delegates
}

// 2D Convenience Overload
[[nodiscard]] inline std::vector<char> generate(const FPHArray &array1,
                                                const FPHArray &array2,
                                                AirTreeOptions options = {}) {
  options.dimensions = 2;
  return generate({&array1, &array2}, options);
}

// 3D Convenience Overload...
[[nodiscard]] inline std::vector<char> generate(const FPHArray &array1,
                                                const FPHArray &array2,
                                                const FPHArray &array3,
                                                AirTreeOptions options = {}) {
  options.dimensions = 3;
  return generate({&array1, &array2, &array3}, options);
}

// 4D Convenience Overload...
[[nodiscard]] inline std::vector<char>
generate(const FPHArray &array1, const FPHArray &array2, const FPHArray &array3,
         const FPHArray &array4, AirTreeOptions options = {}) {
  options.dimensions = 4;
  return generate({&array1, &array2, &array3, &array4}, options);
}

} // namespace airtree::core::api

#endif // AIRTREE_CORE_API_AIRTREEGENERATOR_HPP
