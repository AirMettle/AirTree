// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_READER_HPP
#define AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_READER_HPP

#include <string>
#include <variant>
#include <vector>

using InputDataVariant =
    std::variant<std::vector<int32_t>, std::vector<int64_t>, std::vector<float>,
                 std::vector<double>>;

using InputDataVector = std::vector<InputDataVariant>;

namespace airtree::reader::file {

// FIXME: Prefixing these enums with AT_ because of name collision when adding windows support. Frustrating but
// can simplify this later.
enum SUPPORTED_FILE_TYPE { AT_BINARY, AT_PARQUET, AT_CSV };
enum SUPPORTED_DATA_TYPE { AT_INT32, AT_INT64, AT_FLOAT, AT_DOUBLE, AT_IGNORE };

inline std::string to_string(SUPPORTED_DATA_TYPE t) {
  switch (t) {
  case SUPPORTED_DATA_TYPE::AT_INT32:
    return "INT32";
  case SUPPORTED_DATA_TYPE::AT_INT64:
    return "INT64";
  case SUPPORTED_DATA_TYPE::AT_FLOAT:
    return "FLOAT";
  case SUPPORTED_DATA_TYPE::AT_DOUBLE:
    return "DOUBLE";
  case SUPPORTED_DATA_TYPE::AT_IGNORE:
    return "IGNORE";
  }
  return "UNKNOWN";
}

// Base class for file processors
class Reader {
public:
  virtual ~Reader() = default;

  [[nodiscard]] virtual InputDataVector
  parse([[maybe_unused]] const std::string &file_path,
        [[maybe_unused]] SUPPORTED_DATA_TYPE data_type) {
    return {};
  }

  [[nodiscard]] virtual InputDataVector
  parse([[maybe_unused]] const std::string &file_path,
        [[maybe_unused]] const std::vector<std::string> &columns) {
    return {};
  }
};

[[nodiscard]] InputDataVector
parse_file(const std::string &file_path, SUPPORTED_FILE_TYPE file_type,
           SUPPORTED_DATA_TYPE data_type,
           const std::vector<std::string> &columns);

} // namespace airtree::reader::file

#endif // AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_READER_HPP
