#include <airtree/export/writer/ArrowWriter.hpp>
#include <parquet/arrow/writer.h>
#include <airtree/export/Logger.hpp>
#include <arrow/csv/writer.h>
#include <arrow/io/file.h>
#include <sstream>

using namespace airtree::xport;

namespace airtree::xport::writer {

void saveArrowToParquet(const std::shared_ptr<arrow::Table> &table,
                        const std::unique_ptr<SpecialCounts> &specialCounts,
                        const std::string &output_path) {
  auto outfile = *arrow::io::FileOutputStream::Open(output_path);
  std::shared_ptr<arrow::Schema> schema_with_metadata = table->schema();

  if (specialCounts) {
    auto metadata = std::make_shared<arrow::KeyValueMetadata>();
    // Add special counts as metadata
    metadata->Append("+Inf", std::to_string(specialCounts->posInfCount));
    metadata->Append("-Inf", std::to_string(specialCounts->negInfCount));
    metadata->Append("+0", std::to_string(specialCounts->posZeroCount));
    if (specialCounts->negZeroCount > 0) {
      metadata->Append("-0", std::to_string(specialCounts->negZeroCount));
    }
    metadata->Append("NaN", std::to_string(specialCounts->nanCount));
    schema_with_metadata = schema_with_metadata->WithMetadata(metadata);
  } else {
    SPDLOG_LOGGER_WARN(
        logger(), "No special counts provided, skipping metadata.");
  }

  std::shared_ptr<arrow::Table> table_with_metadata =
      arrow::Table::Make(schema_with_metadata, table->columns());

  parquet::WriterProperties::Builder builder;
  builder.created_by("AirMettle");
  auto writer_properties = builder.build();
  auto arrow_props =
      parquet::ArrowWriterProperties::Builder().store_schema()->build();

  PARQUET_THROW_NOT_OK(parquet::arrow::WriteTable(
      *table_with_metadata, arrow::default_memory_pool(), outfile, 1024,
      writer_properties, arrow_props));
}

void saveArrowToCSV(const std::shared_ptr<arrow::Table> &table,
                    const std::unique_ptr<SpecialCounts> &specialCounts,
                    const std::string &output_path) {
  // Open output file
  auto out_result = arrow::io::FileOutputStream::Open(output_path);
  if (!out_result.ok()) {
    SPDLOG_LOGGER_ERROR(logger(), "Failed to open CSV file for writing: {}",
                        out_result.status().ToString());
    throw std::runtime_error("Failed to open CSV file: " + output_path);
  }
  auto out_stream = out_result.ValueOrDie();

  // Write special counts as comment lines at the top
  if (specialCounts) {
    std::stringstream header;
    header << "# Special Value Counts\n";
    header << "# +Inf: " << specialCounts->posInfCount << "\n";
    header << "# -Inf: " << specialCounts->negInfCount << "\n";
    header << "# +0: " << specialCounts->posZeroCount << "\n";
    if (specialCounts->negZeroCount > 0) {
      header << "# -0: " << specialCounts->negZeroCount << "\n";
    }
    header << "# NaN: " << specialCounts->nanCount << "\n";
    header << "#\n"; // Separator line

    std::string header_str = header.str();
    auto write_result = out_stream->Write(header_str.data(), header_str.size());
    if (!write_result.ok()) {
      SPDLOG_LOGGER_ERROR(logger(), "Failed to write CSV header metadata: {}",
                          write_result.ToString());
      throw std::runtime_error("Failed to write CSV metadata: "
                               + write_result.ToString());
    }
  } else {
    SPDLOG_LOGGER_WARN(
        logger(), "No special counts provided, skipping CSV metadata.");
  }

  // Configure CSV write options
  auto write_options = arrow::csv::WriteOptions::Defaults();
  write_options.include_header = true; // Include column headers

  // Write the table to CSV
  auto write_result =
      arrow::csv::WriteCSV(*table, write_options, out_stream.get());
  if (!write_result.ok()) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Failed to write CSV: {}", write_result.ToString());
    throw std::runtime_error("Failed to write CSV: " + write_result.ToString());
  }

  // Close the output stream
  auto close_result = out_stream->Close();
  if (!close_result.ok()) {
    SPDLOG_LOGGER_ERROR(
        logger(), "Failed to close CSV file: {}", close_result.ToString());
    throw std::runtime_error("Failed to close CSV file: "
                             + close_result.ToString());
  }

  SPDLOG_LOGGER_INFO(logger(), "Successfully wrote CSV to: {}", output_path);
}

void saveArrowToFile(const std::shared_ptr<arrow::Table> &table,
                     const std::string &output_path) {
  auto outfile = *arrow::io::FileOutputStream::Open(output_path);
  auto writer = *arrow::ipc::MakeFileWriter(outfile.get(), table->schema());
  writer->WriteTable(*table);
  writer->Close();
}

} // namespace airtree::xport::writer