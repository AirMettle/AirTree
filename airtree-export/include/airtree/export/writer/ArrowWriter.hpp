// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_EXPORT_WRITER_ARROWWRITER_HPP
#define AIRTREE_EXPORT_WRITER_ARROWWRITER_HPP
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>

#include <airtree/core/AirTreeCore_internal.hpp>

namespace airtree::xport::writer {

// Convert Arrow Table to Parquet file
void saveArrowToParquet(const std::shared_ptr<arrow::Table> &table,
                        const std::unique_ptr<SpecialCounts> &specialCounts,
                        const std::string &output_path);

// Convert Arrow Table to CSV file
void saveArrowToCSV(const std::shared_ptr<arrow::Table> &table,
                    const std::unique_ptr<SpecialCounts> &specialCounts,
                    const std::string &output_path);


void saveArrowToFile(const std::shared_ptr<arrow::Table> &table,
                     const std::string &output_path);


} // namespace airtree::xport::writer


#endif // AIRTREE_EXPORT_WRITER_ARROWWRITER_HPP