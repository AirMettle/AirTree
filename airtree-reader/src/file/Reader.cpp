#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <airtree/reader/file/BinaryReader.hpp>
#include <airtree/reader/file/Reader.hpp>
#include <airtree/reader/file/ParquetReader.hpp>
#include <airtree/reader/file/CSVReader.hpp>

using namespace airtree::reader::file;

class FileParserFactory {
public:
  using ParserCreator = std::unique_ptr<Reader> (*)();

  static std::unique_ptr<Reader>
  createParser(const SUPPORTED_FILE_TYPE file_type) {
    if (creators.find(file_type) != creators.end()) {
      return creators[file_type]();
    }
    throw std::runtime_error("Unsupported file type");
  }

private:
  static std::unordered_map<SUPPORTED_FILE_TYPE, ParserCreator> creators;

  static std::unique_ptr<Reader> createBinaryParsers() {
    return std::make_unique<BinaryFileParser>();
  }

  static std::unique_ptr<Reader> createParquetParsers() {
    return std::make_unique<ParquetFileParser>();
  }

  static std::unique_ptr<Reader> createCSVParsers() {
    return std::make_unique<CSVFileParser>();
  }
};

std::unordered_map<SUPPORTED_FILE_TYPE, FileParserFactory::ParserCreator>
    FileParserFactory::creators = {
        {SUPPORTED_FILE_TYPE::AT_BINARY, &FileParserFactory::createBinaryParsers},
        {SUPPORTED_FILE_TYPE::AT_PARQUET,
         &FileParserFactory::createParquetParsers},
        {SUPPORTED_FILE_TYPE::AT_CSV, &FileParserFactory::createCSVParsers}};

InputDataVector airtree::reader::file::parse_file(
    const std::string &file_path,
    airtree::reader::file::SUPPORTED_FILE_TYPE file_type,
    airtree::reader::file::SUPPORTED_DATA_TYPE data_type,
    const std::vector<std::string> &columns) {
  try {
    auto parsers = FileParserFactory::createParser(file_type);

    if (columns.empty()) {
      return parsers->parse(file_path, data_type);
    } else {
      return parsers->parse(file_path, columns);
    }
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
  }

  return {};
}
