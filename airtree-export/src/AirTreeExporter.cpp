// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/export/AirTreeExporter.hpp>
#include <airtree/export/core/ArrowTableBuilder.hpp>
#include <airtree/export/writer/ArrowWriter.hpp>
#include <airtree/export/Logger.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <stdexcept>

using namespace airtree::query::bin_boundary;

namespace airtree::xport {

void exportAirTree(const std::vector<char> &buffer,
                   const std::string &output_path, ExportFormat format) {

  // Step 1: Create BinBoundary query from buffer
  auto binBoundaryQuery = BinBoundary(buffer);

  // Step 2: Generate bin boundaries
  auto binBoundaryRes = binBoundaryQuery.generateBinBoundaries();
  auto binBoundaries = binBoundaryRes.getBoundaries();
  auto specialCounts = binBoundaryRes.getSpecialCounts();

  // Step 3: Convert to Arrow table based on dimensionality
  std::shared_ptr<arrow::Table> table;

  if (std::holds_alternative<BinBoundary1DList>(*binBoundaries)) {
    const auto &list = std::get<BinBoundary1DList>(*binBoundaries);
    table = core::toArrowTable(list);
  } else if (std::holds_alternative<BinBoundary2DList>(*binBoundaries)) {
    const auto &list = std::get<BinBoundary2DList>(*binBoundaries);
    table = core::toArrowTable(list);
  } else if (std::holds_alternative<BinBoundary3DList>(*binBoundaries)) {
    const auto &list = std::get<BinBoundary3DList>(*binBoundaries);
    table = core::toArrowTable(list);
  } else if (std::holds_alternative<BinBoundary4DList>(*binBoundaries)) {
    const auto &list = std::get<BinBoundary4DList>(*binBoundaries);
    table = core::toArrowTable(list);
  } else {
    SPDLOG_LOGGER_ERROR(logger(), "Unknown BinBoundary variant type");
    throw std::runtime_error("Unknown BinBoundary variant type");
  }

  // Step 4: Export to the specified format
  switch (format) {
  case ExportFormat::PARQUET:
    writer::saveArrowToParquet(table, specialCounts, output_path);
    break;
  case ExportFormat::CSV:
    writer::saveArrowToCSV(table, specialCounts, output_path);
    break;
  case ExportFormat::ARROW:
  default:
    writer::saveArrowToFile(table, output_path);
    break;
  }

  SPDLOG_LOGGER_INFO(logger(), "Exported successfully to: {}", output_path);
}

} // namespace airtree::xport
