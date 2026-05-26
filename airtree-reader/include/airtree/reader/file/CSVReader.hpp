// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_CSVREADER_HPP
#define AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_CSVREADER_HPP

#include <airtree/reader/file/Reader.hpp>
#include <string>

namespace airtree::reader::file {

// Processor for CSV files
class CSVFileParser : public Reader {
public:
  InputDataVector parse(const std::string &file_path,
                        const std::vector<std::string> &columns) override;
};

} // namespace airtree::reader::file

#endif // AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_CSVREADER_HPP
