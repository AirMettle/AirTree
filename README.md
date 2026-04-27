# AirTree Histogram Generator

**AirTree** is a high-performance C++ library and CLI tool for generating compact, queryable histogram buffers from large datasets. It supports 1D to 4D data using specialized trie-based structures optimized for speed and memory efficiency.

This section focuses **exclusively on histogram generation** (the `generate` flow). Query functionality (top-k, percentiles, min/max, etc.) will be covered in a later section.

---

## Installation Assumptions

- The `airtree` CLI binary is installed and available in your `$PATH`.
- The C++ headers (`<airtree/...>`) and libraries are installed in standard system paths (e.g., `/usr/local/include/airtree` and `/usr/local/lib`).
- You have a C++17 (or later) compiler and the necessary dependencies (CLI11, Arrow/Parquet for the reader, etc.).

---

## CLI Usage

Generate a histogram buffer directly from the command line using either **Parquet** (multi-dimensional) or **binary** (1D only) input files.

### Supported Histogram Configurations (Schemas)

| Schema   | Dimensions | Description                  | Input Type     |
|----------|------------|------------------------------|----------------|
| `1DxT`   | 1D         | 1D Tiny variant              | Binary only    |
| `1DxF`   | 1D         | 1D Fast variant              | Binary / Parquet |
| `1DxP`   | 1D         | 1D Precise variant           | Binary / Parquet |
| `2DxP`   | 2D         | 2D Precise variant           | Parquet only   |
| `2DxF`   | 2D         | 2D Fast variant              | Parquet only   |
| `3DxF`   | 3D         | 3D Fast variant              | Parquet only   |
| `3DxP`   | 3D         | 3D Precise variant           | Parquet only   |
| `4DxF`   | 4D         | 4D Fast variant              | Parquet only   |
| `4DxP`   | 4D         | 4D Precise variant           | Parquet only   |

### 1. Generate from Parquet (Recommended for multi-dimensional data)

```bash
airtree generate parquet \
  -i /path/to/input.parquet \
  -o /path/to/histogram.bin \
  -s 2DxP \
  -c column_name_1 column_name_2
```

```
Options:

-i, --input (required) — Path to Parquet file
-o, --output (required) — Output histogram buffer file
-s, --schema (required) — One of the supported schemas above
-c, --columns (required) — Space-separated list of column names (must match the schema dimension)
-e, --e2e (optional) — Enable end-to-end mode: also generate interactive plots via fph_deserialize
```

Example (2D histogram):
```bash
airtree generate parquet \
  -i data/sales.parquet \
  -o results/sales_2d.bin \
  -s 2DxP \
  -c "price" "quantity"
```

---

### 2. Generate from Binary File (1D only)
```bash
airtree generate binary \
  -i /path/to/data.bin \
  -o /path/to/histogram.bin \
  -s 1DxP \
  -d double
```

```
Options:

-i, --input (required) — Path to raw binary file
-o, --output (required) — Output histogram buffer file
-s, --schema (required) — Must be a 1D schema (1DxT, 1DxF, or 1DxP)
-d, --data-type (required) — One of: int32, int64, float, double
```

Example:

```bash
airtree generate binary \
  -i data/temperatures.bin \
  -o results/temp_1d.bin \
  -s 1DxF \
  -d float
```

---

## Generated AirTree Histogram Buffer

The generated file (histogram.bin) is a compact binary buffer containing the AirTree histogram. It can be:

- Written to disk
- Used directly in queries (via the query subcommands)

--- 

## C++ Library API

When AirTree is installed as a library, you can generate histograms programmatically with zero-copy, type-safe convenience functions.

### Header
```C++
#include <airtree/core/api/AirTreeGenerator.hpp>
```

### Main API

```C++
namespace airtree::core::api {

struct AirTreeOptions {
  int dimensions = 1;
  ConfigType type = ConfigType::XP;  // XP, XT, XF, or XNUM
  bool default_mode = true;
};

std::vector<char> generate(
    const std::vector<double>& array1,
    const AirTreeOptions& options);

} // namespace airtree::core::api
```

---

### 1D

```C++
std::vector<double> data = {1.2, 3.4, 5.6, /* ... */};

// Auto-detects 1D + default XP config
auto buffer = airtree::core::api::generate(data);
```

### 2D
```C++
std::vector<float> x = { /* ... */ };
std::vector<float> y = { /* ... */ };

auto buffer = airtree::core::api::generate(x, y, AirTreeOptions{
    .dimensions = 2,
    .type = ConfigType::XP
});
```

### 3D / 4D
```C++
auto buffer = airtree::core::api::generate(vec1, vec2, vec3);  // 3D, defaults to XP
auto buffer = airtree::core::api::generate(vec1, vec2, vec3, vec4);  // 4D, defaults to XP
```

--- 

### Supported ConfigType values:
```
ConfigType::XP — High Precision variant (Default)
ConfigType::XT — Most compact variant (1D only)
ConfigType::XF — Fastest variant
```

### Full Example (1D with explicit options)

```C++
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <vector>
#include <iostream>

int main() {
  std::vector<float> temperatures = {23.5f, 24.1f, 19.8f, /* ... */};

  airtree::core::api::AirTreeOptions opts;
  opts.dimensions = 1;
  opts.type = airtree::core::api::ConfigType::XF;

  std::vector<char> histogram = airtree::core::api::generate(temperatures, opts);

  std::cout << "Generated histogram buffer of size: " << histogram.size() << " bytes\n";

  // Save to file if desired
  std::ofstream out("histogram.bin", std::ios::binary);
  out.write(histogram.data(), histogram.size());
  return 0;
}
```

### Linking
```bash
g++ -std=c++17 your_program.cpp -lairtree -o your_program
```

### Next Steps

The generated .bin file is ready for querying (top-k, percentile, min/max, etc.).


Happy histogram building! 🎯


# AirTree Query Guide

**AirTree** provides a powerful set of query operations on pre-generated histogram buffers (the `.bin` files produced by the `generate` commands). All queries are extremely fast because they operate directly on the compact trie structure.

This section focuses **exclusively on querying** generated histograms. It covers both the **CLI** (where implemented) and the **C++ library API**.

---

## Prerequisites

- A valid AirTree histogram file (e.g., `histogram.bin`) generated with any supported schema (`1DxT`, `1DxF`, `1DxP`, `2DxP`, `2DxF`, etc.).
- The `airtree` CLI binary is in your `$PATH`.
- For library usage: headers (`<airtree/query/...>`) and libraries are installed in standard system paths.

---

## CLI Usage

All queries are accessed via the `query` subcommand:

```bash
airtree query <subcommand> [options]
```

|Command | Description | Required Flags | Output |
---------|-------------|----------------|--------|
|topk |    Returns the top-k% bins (by count) | -i, -o, -k | Bin ranges + counts |
|min_count | Bins with the minimum count | -i, -o | Bin ranges + counts |
|max_count | Bins with the maximum count | -i, -o | Bin ranges + counts |
|min_value | Smallest value that appears in the histogram | -i, -o | Bin range + count |
|max_value | Largest value that appears in the histogram | -i, -o | Bin range + count |
|percentile | Computes a specific percentile (0–100) | -i, -o, -p | Single percentile value |


### Common Flags

- -i, --input (required) — Path to the generated histogram .bin file
- -o, --output (required) — Path to write human-readable results


## Examples

### Top-K query (top 10% bins):

```bash    
airtree query topk \
  -i results/sales_2d.bin \
  -o results/topk.txt \
  -k 10.0
```

### Percentile query (median):
```bash
airtree query percentile \
  -i results/temp_1d.bin \
  -o results/median.txt \
  -p 50.0
```

### Min/Max count queries:
```bash
airtree query min_count -i histogram.bin -o min_count.txt
airtree query max_count -i histogram.bin -o max_count.txt
```

### Min/Max value queries:
```bash
airtree query min_value -i histogram.bin -o min_value.txt
airtree query max_value -i histogram.bin -o max_value.txt
```

## C++ Library API

All query classes live in the `airtree::query` namespace and take a histogram buffer in their constructor.

### 1. Percentile
```C++
#include <airtree/query/percentile/Percentile.hpp>

airtree::query::percentile::Percentile p(histogram_buffer);
double value = p.getPercentile(50.0);  // 0–100
// Returns -∞ if histogram is empty
```

### 2. TopK
```C++
#include <airtree/query/topk/TopK.hpp>

airtree::query::topk::TopK tk(histogram_buffer);
auto results = tk.getTopK(10.0);  // k = percentage of total elements

for (const auto& r : results) {
  std::cout << "Bin: [" << r.getLowerBound() << ", " << r.getUpperBound()
            << "], Count: " << r.getCount() << "\n";
}
```

### 3. MinMax (min_count, max_count, min_value, max_value)
```C++
#include <airtree/query/minmax/MinMax.hpp>

airtree::query::minmax::MinMax mm(histogram_buffer);

// Bins with minimum count
auto minCount = mm.getMin();

// Bins with maximum count
auto maxCount = mm.getMax();

// Smallest set value
auto minVal = mm.getMinValue();

// Largest set value
auto maxVal = mm.getMaxValue();

for (const auto& r : minCount) {
  std::cout << "Bin: [" << r.getLowerBound() << ", " << r.getUpperBound()
            << "], Count: " << r.getBinCount() << "\n";
}
```    
MinMaxResult provides getLowerBound(), getUpperBound(), and getBinCount().

### 4. CDF (Cumulative Distribution Function)
```C++
#include <airtree/query/cdf/CDF.hpp>

airtree::query::cdf::CDF cdf(histogram_buffer);
double prob = cdf.getCDF(42.5, true);  // value, interpolate=true/false
// Returns probability mass ≤ the given value (0.0–1.0)
```

### 5. BoundingBox (2D & 3D)
This is the most advanced query, ideal for spatial range counting.
```C++
#include <airtree/query/bounding_box/BoundingBox.hpp>

airtree::query::bounding_box::BoundingBox bb(histogram_buffer);

// 2D example
airtree::query::bounding_box::BoundingBoxCoordinate2D box(10.0, 20.0, 30.0, 40.0);
auto [safeResult, totalResult] = bb.getCounts(box);

auto [safeBox, safeCount] = safeResult;
auto [totalBox, totalCount] = totalResult;

std::cout << "Safe box count: " << safeCount << "\n";
std::cout << "Total box count (incl. edges): " << totalCount << "\n";

// 3D Support
// Use BoundingBoxCoordinate3D, getCounts() with 3D box, and BatchBoundingBoxResult3D.
```

#### Result semantics:

- Safe box: Fully contained bins (guaranteed inside the query box)
- Total box: Safe box + edge bins (possible partial overlaps)


## Linking the Library
```bash
g++ -std=c++17 your_query.cpp -lairtree -o your_query
```

Happy querying! 📊

# AirTree Export Guide

**AirTree Export** converts a compact AirTree histogram buffer (the `.bin` file produced by `airtree generate`) into standard, analysis-ready tabular formats:

- **Apache Arrow** (default)  
- **Parquet** (highly recommended for large histograms)  
- **CSV** (human-readable with special-value metadata)

This makes it simple to load bin-level histogram data into Python (pandas, pyarrow), R, Spark, Excel, or any Arrow-compatible tool.

---

## CLI Usage

The export functionality is provided by the standalone `airtree-export` command (installed alongside the main `airtree` CLI).

```bash
airtree-export <input_histogram.bin> [OPTIONS]
```

Supported Options

|Option | Description | Default |
|-------|-------------|---------|
|--parquet | Export to Parquet format | — |
| --csv | Export to CSV format | — |
| --output <path> | Output file or directory (optional) | Same directory as input, with matching extension |
| (no format flag) | Export to Arrow IPC format (.arrow) | Arrow |

- You can specify only one format flag (--parquet or --csv).
- If --output points to a directory, the tool automatically names the file based on the input stem + extension (.arrow / .parquet / .csv).
- Works with any AirTree histogram (1D–4D, any supported schema).

## Examples
### 1. Default Arrow export (fastest, most compact):
```bash
airtree-export results/sales_2d.bin
# Creates: results/sales_2d.arrow
```

### 2. Export to Parquet:
```bash
airtree-export results/temp_1d.bin --parquet
# Creates: results/temp_1d.parquet
```

### 3. Export to CSV with custom filename:
```bash
airtree-export results/histogram.bin --csv --output ./exports/my_histogram.csv
```

### 4. Export to a directory (auto-named):
```bash
airtree-export data/4d_histogram.bin --parquet --output ./exports/
# Creates: exports/4d_histogram.parquet
```

## What You Get in the Exported File

The output table contains one row per histogram bin with the following columns (automatically adjusted for dimensionality):

- 1D: x-min, x-max, counts
- 2D: x-min, x-max, y-min, y-max, counts
- 3D: x-min, x-max, y-min, y-max, z-min, z-max, counts
- 4D: x-min, x-max, y-min, y-max, z-min, z-max, w-min, w-max, counts

## Special Value Handling

Floating-point edge cases (+Inf, -Inf, +0, -0, NaN) are preserved and exported as metadata:

- Parquet: Stored as key-value metadata in the Arrow schema (+Inf, -Inf, +0, -0, NaN)
- CSV: Written as comment lines (#) at the very top of the file
- Arrow: Included in the schema metadata (readable by Arrow readers)

This ensures downstream tools have complete statistical information.

## Quick Start Workflow

Generate a histogram:
```bash
airtree generate parquet -i data.parquet -o histogram.bin -s 2DxP -c price quantity
```

Export it to your preferred format:
```bash
airtree-export histogram.bin --parquet --output analysis/
```

Load in Python (example):
```python
import pyarrow.parquet as pq
table = pq.read_table("analysis/histogram.parquet")
print(table)
```

Tip: Parquet is usually the best choice for performance and size when working with large multi-dimensional histograms.

Happy exporting! 📤

# AirTree Merge Guide

**AirTree Merge** allows you to combine two compatible AirTree histogram buffers (the `.bin` files produced by `airtree generate`) into a single histogram.  

This is useful for:
- Merging histograms from different data batches, shards, or runs
- Incremental histogram construction
- Combining results from parallel processing

The merge operation is **exact** (no approximation) and produces a valid AirTree histogram that can be queried, exported, or merged again.

---

## Prerequisites

- Two AirTree histogram files (`.bin`) that were generated with the **exact same configuration** (same schema: e.g., both `2DxP`, both `1DxF`, etc.).
- The `airtree-merge-cli` binary (or the library) installed and available.

**Important**: Merging histograms with different configurations (different dimensions or different trie variants) is **not supported** and will throw a clear error.

---

## CLI Usage

A standalone executable called **`airtree-merge-cli`** is provided for command-line merging.

```bash
airtree-merge-cli <input_histogram1.bin> <input_histogram2.bin> <output_histogram.bin>
```

### Example
```bash
# Merge two 2D histograms from different data batches
airtree-merge-cli batch1_2d.bin batch2_2d.bin merged_2d.bin
```

## Merge two 1D histograms
```bash
airtree-merge-cli run1_1d.bin run2_1d.bin final_1d.bin
```

What happens:

- The tool reads both input files
- Validates that they have identical configurations
- Performs the merge
- Writes the resulting histogram to the output file
- Logs progress, sizes, and success/failure

The output file is a fully valid AirTree histogram buffer, ready for querying or exporting.

## C++ Library API
When using AirTree as a library, merging is extremely simple via the public API in the airtree::merge namespace.

### Header
```C++
#include <airtree/merge/AirTreeMerge.hpp>
```

### Public Functions
```C++
namespace airtree::merge {

// Returns the merged histogram as a vector<char>
std::vector<char> mergeAirTree(
    const std::vector<char>& buffer1,
    const std::vector<char>& buffer2);

// Merges and writes the result directly to a file
void mergeAirTree(
    const std::vector<char>& buffer1,
    const std::vector<char>& buffer2,
    const std::string& output_path);

} // namespace airtree::merge
```

### Simple Library Example
```C++
#include <airtree/merge/AirTreeMerge.hpp>
#include <fstream>
#include <vector>
#include <iostream>

int main() {
  // Helper lambda to read a binary histogram file
  auto read_file = [](const std::string& path) -> std::vector<char> {
    std::ifstream f(path, std::ios::binary);
    return std::vector<char>((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
  };

  auto buf1 = read_file("histogram1.bin");
  auto buf2 = read_file("histogram2.bin");

  // Option 1: Get merged buffer in memory
  std::vector<char> merged = airtree::merge::mergeAirTree(buf1, buf2);

  // Option 2: Merge and write directly to file
  airtree::merge::mergeAirTree(buf1, buf2, "merged_histogram.bin");

  std::cout << "Merge complete! Output size: " << merged.size() << " bytes\n";
  return 0;
}
```

### Linking

```bash
g++ -std=c++17 your_program.cpp -lairtree -o your_program
```

### Quick Workflow

- Generate two histograms with the same schema:
```bash
airtree generate ... -o part1.bin -s 2DxP ...
airtree generate ... -o part2.bin -s 2DxP ...
```

- Merge them:
```bash
airtree-merge-cli part1.bin part2.bin combined.bin
```

- Use the result:
  - Query it (airtree query ...)
  - Export it (airtree-export combined.bin --parquet)
  - Merge it again with more data

That’s it. The merge API is intentionally minimal and robust — just two valid AirTree histograms of the same configuration, and you get one merged histogram.

## References
- AirTree Patent: [US Patent No. TODO: Actual patent number](TODO: link to actual patent)
- LICENSE: TODO: Link to actual non-commercial license file
