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

enum SUPPORTED_FILE_TYPE { BINARY, PARQUET, CSV };
enum SUPPORTED_DATA_TYPE { INT32, INT64, FLOAT, DOUBLE, IGNORE };

inline std::string to_string(SUPPORTED_DATA_TYPE t) {
  switch (t) {
  case SUPPORTED_DATA_TYPE::INT32:
    return "INT32";
  case SUPPORTED_DATA_TYPE::INT64:
    return "INT64";
  case SUPPORTED_DATA_TYPE::FLOAT:
    return "FLOAT";
  case SUPPORTED_DATA_TYPE::DOUBLE:
    return "DOUBLE";
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
