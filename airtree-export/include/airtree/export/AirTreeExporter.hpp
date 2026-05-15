// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_EXPORT_AIRTREEEXPORTER_HPP
#define AIRTREE_EXPORT_AIRTREEEXPORTER_HPP

#include <vector>
#include <string>

namespace airtree::xport {

/**
 * @brief Export format options for AirTree data
 */
enum class ExportFormat {
  ARROW,   // Apache Arrow IPC format (.arrow)
  PARQUET, // Apache Parquet format (.parquet)
  CSV      // Comma-separated values (.csv)
};

/**
 * @brief Export AirTree histogram data from a buffer to a file in the specified
 * format
 *
 * This function handles the complete export pipeline:
 * - Parses the AirTree buffer
 * - Generates bin boundaries automatically
 * - Detects dimensionality (1D, 2D, 3D, or 4D)
 * - Converts to Arrow table format
 * - Writes to the specified output format
 *
 * @param buffer Binary buffer containing AirTree histogram data
 * @param output_path Path to the output file
 * @param format Export format (ARROW, PARQUET, or CSV)
 * @throws std::runtime_error if export fails at any stage
 */
void exportAirTree(const std::vector<char> &buffer,
                   const std::string &output_path,
                   ExportFormat format = ExportFormat::ARROW);

} // namespace airtree::xport

#endif // AIRTREE_EXPORT_AIRTREEEXPORTER_HPP
