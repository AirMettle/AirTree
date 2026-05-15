// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_PARQUETREADER_HPP
#define AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_PARQUETREADER_HPP

#include <airtree/reader/file/Reader.hpp>
#include <string>

namespace airtree::reader::file {

// Processor for Parquet files
class ParquetFileParser : public Reader {
public:
  InputDataVector parse(const std::string &file_path,
                        const std::vector<std::string> &columns) override;

  // virtual ~ParquetFileParser();
};

} // namespace airtree::reader::file

#endif // AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_PARQUETREADER_HPP
