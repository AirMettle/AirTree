// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#ifndef AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHPATHS_HPP
#define AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHPATHS_HPP

#include <string>
#include <vector>

namespace airtree::bench {

// Shared paths/buffers for generate write-side-effect and query benches.
// Filled by Main before RunSpecifiedBenchmarks().
struct BenchPaths {
  // If non-empty, generate Serialize fixtures write the serialized buffer once
  // (untimed) to this path, then clear the string.
  inline static std::string write_airtree_path;

  // Preloaded .airtree bytes for query fixtures (loaded untimed in Main).
  inline static std::vector<char> query_buffer;

  // Schema name for the current query run (e.g. "1DxF").
  inline static std::string query_schema;

  // Preloaded .airtree buffers (same schema) for merge fixtures (loaded untimed in Main).
  inline static std::vector<std::vector<char>> merge_buffers;
};

} // namespace airtree::bench

#endif // AIRTREE_BENCH_INCLUDE_AIRTREE_BENCH_BENCHPATHS_HPP