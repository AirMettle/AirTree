// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_BINARYREADER_HPP
#define AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_BINARYREADER_HPP

#include <airtree/reader/file/Reader.hpp>
#include <string>

namespace airtree::reader::file {

// Processor for binary files
class BinaryFileParser : public Reader {
public:
  InputDataVector parse(const std::string &file_path,
                        const SUPPORTED_DATA_TYPE data_type) override;

  virtual ~BinaryFileParser() override;
};

} // namespace airtree::reader::file

#endif // AIRTREE_READER_INCLUDE_AIRTREE_READER_FILE_BINARYREADER_HPP
