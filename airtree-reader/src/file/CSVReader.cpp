// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <arrow/array.h>
#include <arrow/chunked_array.h>
#include <arrow/csv/reader.h>
#include <arrow/io/file.h>
#include <arrow/table.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>

#include <cstdint>
#include <iostream>

#include <airtree/reader/file/CSVReader.hpp>
#include <airtree/reader/Logger.hpp>

using namespace airtree::reader::file;
using namespace airtree::reader;


template <typename ArrowArrayType, typename T>
std::vector<T>
readColumnData(const std::shared_ptr<arrow::ChunkedArray> &chunked_array) {
  std::vector<T> data_vector;

  if (!chunked_array || chunked_array->num_chunks() == 0) {
    SPDLOG_LOGGER_ERROR(logger(), "Empty column or no data found.");
    return {};
  }

  data_vector.reserve(chunked_array->length());

  for (int chunk_idx = 0; chunk_idx < chunked_array->num_chunks(); ++chunk_idx) {
    auto array = arrow::internal::checked_pointer_cast<ArrowArrayType>(
        chunked_array->chunk(chunk_idx));
    for (int64_t j = 0; j < array->length(); ++j) {
      data_vector.push_back(static_cast<T>(array->Value(j)));
    }
  }

  return data_vector;
}

// parser for CSV files
InputDataVector CSVFileParser::parse(const std::string &file_path,
                                     const std::vector<std::string> &columns) {
  InputDataVector input_data_vector;

  // Open the CSV file
  arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> infile_result =
      arrow::io::ReadableFile::Open(file_path);
  if (!infile_result.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error opening CSV file: {}",
                        infile_result.status().message());
    return {};
  }

  const std::shared_ptr<arrow::io::ReadableFile> &infile = *infile_result;

  // Configure CSV read options
  auto read_options = arrow::csv::ReadOptions::Defaults();
  auto parse_options = arrow::csv::ParseOptions::Defaults();
  auto convert_options = arrow::csv::ConvertOptions::Defaults();

  // Create CSV reader
  auto result = arrow::csv::TableReader::Make(arrow::io::default_io_context(),
                                              infile, read_options,
                                              parse_options, convert_options);
  if (!result.ok()) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error creating CSV reader: {}", result.status().message());
    return {};
  }
  std::shared_ptr<arrow::csv::TableReader> reader = std::move(*result);

  // Read the table
  auto table_result = reader->Read();
  // Close eagerly
  (void)infile->Close();
  if (!table_result.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error reading table from CSV file: {}",
                        table_result.status().message());
    return {};
  }
  std::shared_ptr<arrow::Table> table = std::move(*table_result);

  std::shared_ptr<arrow::Schema> schema = table->schema();
  for (const auto &column : columns) {
    int colIdx = schema->GetFieldIndex(column);
    if (colIdx == -1) {
      SPDLOG_LOGGER_ERROR(logger(), "Column {} not found in CSV file", column);
      return {};
    }

    std::shared_ptr<arrow::Field> field = schema->field(colIdx);
    std::shared_ptr<arrow::DataType> type = field->type();
    std::shared_ptr<arrow::ChunkedArray> chunked_array = table->column(colIdx);

    switch (type->id()) {
    case arrow::Type::INT32:
      input_data_vector.emplace_back(
          readColumnData<arrow::Int32Array, int32_t>(chunked_array));
      break;
    case arrow::Type::INT64:
      input_data_vector.emplace_back(
          readColumnData<arrow::Int64Array, int64_t>(chunked_array));
      break;
    case arrow::Type::FLOAT:
      input_data_vector.emplace_back(
          readColumnData<arrow::FloatArray, float>(chunked_array));
      break;
    case arrow::Type::DOUBLE:
      input_data_vector.emplace_back(
          readColumnData<arrow::DoubleArray, double>(chunked_array));
      break;
    default:
      SPDLOG_LOGGER_ERROR(logger(), "Unsupported data type in CSV file");
      return {};
    }
  }

  return input_data_vector;
}
