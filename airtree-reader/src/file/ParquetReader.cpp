#include <arrow/array.h>
#include <arrow/chunked_array.h>
#include <arrow/io/file.h>
#include <arrow/table.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>

#include <cstdint>
#include <iostream>
#include <parquet/arrow/reader.h>

#include <airtree/reader/file/ParquetReader.hpp>
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

  auto array = arrow::internal::checked_pointer_cast<ArrowArrayType>(
      chunked_array->chunk(0));
  data_vector.reserve(array->length());

  for (int64_t j = 0; j < array->length(); ++j) {
    data_vector.push_back(static_cast<T>(array->Value(j)));
  }

  return data_vector;
}

// parser for parquet files
InputDataVector
ParquetFileParser::parse(const std::string &file_path,
                         const std::vector<std::string> &columns) {
  InputDataVector input_data_vector;

  // Open the Parquet file
  arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> infile_result =
      arrow::io::ReadableFile::Open(file_path);
  if (!infile_result.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error opening Parquet file: {}",
                        infile_result.status().message());
    return {};
  }

  const std::shared_ptr<arrow::io::ReadableFile> &infile = *infile_result;
  auto result = parquet::arrow::OpenFile(infile, arrow::default_memory_pool());
  if (!result.ok()) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Error opening Parquet file: {}", result.status().message());
    return {};
  }
  std::unique_ptr<parquet::arrow::FileReader> reader = std::move(*result);


  std::shared_ptr<arrow::Table> table;
  auto status = reader->ReadTable(&table);
  if (!status.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Error reading table from Parquet file: {}",
                        status.message());
    return {};
  }

  std::shared_ptr<arrow::Schema> schema = table->schema();
  for (const auto &column : columns) {
    int colIdx = schema->GetFieldIndex(column);
    if (colIdx == -1) {
      SPDLOG_LOGGER_ERROR(
          logger(), "Column {} not found in Parquet file", column);
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
      SPDLOG_LOGGER_ERROR(logger(), "Unsupported data type in Parquet file");
      return {};
    }
  }

  return input_data_vector;
}
