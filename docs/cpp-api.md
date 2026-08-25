# C++ Library API

[← Back to README](../README.md)

When AirTree is installed, you can use it programmatically through a small set
of public headers under `<airtree/...>`.

## Table of Contents

- [Headers at a Glance](#headers-at-a-glance)
- [Generate API](#generate-api)
  - [1D / 2D / 3D / 4D Convenience Overloads](#1d--2d--3d--4d-convenience-overloads)
  - [`AirTreeOptions`](#airtreeoptions)
  - [Full Example](#full-example-1d-with-explicit-options)
- [Query APIs](#query-apis)
  - [Percentile](#percentile)
  - [TopK](#topk)
  - [MinMax](#minmax)
  - [CDF](#cdf)
  - [BoundingBox (2D / 3D)](#boundingbox-2d--3d)
  - [BinBoundary](#binboundary)
  - [GridQuery](#gridquery)
- [Export API](#export-api)
- [Merge API](#merge-api)
- [Reader API](#reader-api)
- [Linking](#linking)

---

## Headers at a Glance

| Header                                              | Namespace                            | What it provides                            |
| --------------------------------------------------- | ------------------------------------ | ------------------------------------------- |
| `<airtree/core/api/AirTreeGenerator.hpp>`           | `airtree::core::api`                 | Histogram generation from in-memory arrays  |
| `<airtree/query/percentile/Percentile.hpp>`         | `airtree::query::percentile`         | Percentile queries                          |
| `<airtree/query/topk/TopK.hpp>`                     | `airtree::query::topk`               | Top-K bin queries                           |
| `<airtree/query/minmax/MinMax.hpp>`                 | `airtree::query::minmax`             | Min/max count and value queries             |
| `<airtree/query/cdf/CDF.hpp>`                       | `airtree::query::cdf`                | Cumulative distribution function            |
| `<airtree/query/bounding-box/BoundingBox.hpp>`      | `airtree::query::bounding_box`       | 2D / 3D range counting                      |
| `<airtree/query/bin-boundary/BinBoundary.hpp>`      | `airtree::query::bin_boundary`       | Enumerate bin boundaries from a buffer      |
| `<airtree/query/grid/GridQuery.hpp>`                | `airtree::query::grid`               | Grid (re-binned histogram / heatmap) queries |
| `<airtree/export/AirTreeExporter.hpp>`              | `airtree::xport`                     | Export buffers to Arrow / Parquet / CSV     |
| `<airtree/merge/AirTreeMerge.hpp>`                  | `airtree::merge`                     | Merge two compatible histograms             |
| `<airtree/reader/file/Reader.hpp>`                  | `airtree::reader::file`              | Read Parquet / binary files into arrays     |

> **Note** — the directory name `bounding-box` and `bin-boundary` use hyphens;
> the C++ namespaces use underscores (`bounding_box`, `bin_boundary`).

> **Public surface.** Only the headers in the table above are part of the
> public API. Anything else under `<airtree/...>` (subpaths under
> `core/common/`, `core/serdes/`, `<airtree/util/...>`, `<airtree/cli/...>`,
> etc.) is internal and may change without notice; do not include them
> directly.

---

## Generate API

```cpp
#include <airtree/core/api/AirTreeGenerator.hpp>
```

The generator produces an in-memory histogram buffer (`std::vector<char>`)
that can be:

- Written to disk and consumed by the CLI tools (`airtree query …`,
  `airtree-export`, `airtree-merge`)
- Passed straight into the [Query](#query-apis), [Merge](#merge-api), or
  [Export](#export-api) APIs without ever touching the filesystem

### 1D / 2D / 3D / 4D Convenience Overloads

The dimensionality is inferred from the number of array arguments. The element
type is inferred from each `std::vector<T>` (templates accept `double`, `float`,
`int32_t`, `int64_t`).

```cpp
namespace airtree::core::api {

// 1D — auto-detects dimensions = 1, default ConfigType::XP
template <typename T>
std::vector<char> generate(const std::vector<T>& array,
                           AirTreeOptions options = {});

// 2D
template <typename T1, typename T2>
std::vector<char> generate(const std::vector<T1>& a1,
                           const std::vector<T2>& a2,
                           AirTreeOptions options = {});

// 3D
template <typename T1, typename T2, typename T3>
std::vector<char> generate(const std::vector<T1>& a1,
                           const std::vector<T2>& a2,
                           const std::vector<T3>& a3,
                           AirTreeOptions options = {});

// 4D
template <typename T1, typename T2, typename T3, typename T4>
std::vector<char> generate(const std::vector<T1>& a1,
                           const std::vector<T2>& a2,
                           const std::vector<T3>& a3,
                           const std::vector<T4>& a4,
                           AirTreeOptions options = {});

} // namespace airtree::core::api
```

**Examples:**

```cpp
// 1D — defaults: dimensions=1, ConfigType::XP
std::vector<double> data = { /* ... */ };
auto buffer = airtree::core::api::generate(data);

// 2D with explicit ConfigType
std::vector<float> x = { /* ... */ };
std::vector<float> y = { /* ... */ };
auto buf2 = airtree::core::api::generate(x, y,
    airtree::core::api::AirTreeOptions{ .type = airtree::core::api::ConfigType::XP });

// 3D / 4D
auto buf3 = airtree::core::api::generate(v1, v2, v3);          // 3D, XP
auto buf4 = airtree::core::api::generate(v1, v2, v3, v4);      // 4D, XP
```

### `AirTreeOptions`

```cpp
namespace airtree::core::api {

enum class ConfigType {
  XP,    // High-Precision (default)
  XT,    // Tiny (1D only)
  XF,    // Fast
  XNUM,
};

struct AirTreeOptions {
  int        dimensions   = 1;
  ConfigType type         = ConfigType::XP;
  bool       default_mode = true;
};

} // namespace airtree::core::api
```

- `dimensions` — overridden automatically by the convenience overloads to match
  the number of array arguments.
- `type` — the trie variant. `XT` is only valid for 1D.
- `default_mode` — internal mode toggle for trie construction. Leave `true`
  unless you have a specific reason to change it.

### Full Example (1D with explicit options)

```cpp
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
  std::vector<float> temperatures = { 23.5f, 24.1f, 19.8f /* ... */ };

  airtree::core::api::AirTreeOptions opts;
  opts.dimensions = 1;
  opts.type       = airtree::core::api::ConfigType::XF;

  auto histogram = airtree::core::api::generate(temperatures, opts);

  std::cout << "Histogram buffer: " << histogram.size() << " bytes\n";

  std::ofstream out("histogram.airtree", std::ios::binary);
  out.write(histogram.data(), histogram.size());
}
```

The buffer is now ready for any of the [Query APIs](#query-apis),
[Merge](#merge-api), or [Export](#export-api).

---

## Query APIs

All query classes take a histogram buffer (`std::vector<char>` or compatible)
in their constructor. Methods are non-mutating and run directly against the
trie — they do not deserialize into a heavy intermediate structure.

> **Each operator only supports a subset of schemas.** Mismatched buffers throw
> `std::runtime_error("Unsupported dimensions or bit length.")` (or a similar
> message). The supported configs are listed under each operator below.

### Percentile

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`. Throws on 2D / 3D / 4D buffers.

```cpp
#include <airtree/query/percentile/Percentile.hpp>

airtree::query::percentile::Percentile p(histogram_buffer);
double value = p.getPercentile(50.0);   // percentile in 0 – 100
auto r = p.getPercentileWithBounds(99.0); // r.value plus r.lower_bound / r.upper_bound of the native bin it fell in
// Query objects are immutable after construction and may be shared between threads.

// Returns -∞ (negative infinity) if the histogram is empty.
```

### TopK

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`. Throws on 2D / 3D / 4D buffers.

```cpp
#include <airtree/query/topk/TopK.hpp>

airtree::query::topk::TopK tk(histogram_buffer);
auto results = tk.getTopK(10.0);   // k as a percentage of total count

for (const auto& r : results) {
  std::cout << "Bin: [" << r.getLowerBound() << ", " << r.getUpperBound()
            << "], Count: " << r.getCount() << "\n";
}
```

`TopKResult` exposes `getLowerBound()`, `getUpperBound()`, `getCount()`
(`uint32_t`), and `getInternalRepresentation()` (`uint32_t`).

**Semantics — tail-by-mass quantile, not rank-by-count.** `getTopK(k)`
returns the bins covering the top `k`% of total *count*, walked **inward
from the largest values** (`+Inf`, then descending bins, then `-Inf` last).
It is *not* "sort all bins by count and return the top k%" — those two
queries return entirely different bin sets.

For a unimodal distribution centred at zero, `tk.getTopK(1.0)` returns bins
from the upper tail until their cumulative count reaches 1% of the total,
*not* the densest bins around the mode. If you need the densest bins, use
[`MinMax::getMax`](#minmax) (one bin) or post-process the bin counts from
[`BinBoundary`](#binboundary).

### MinMax

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`. All four methods
> (`getMin`, `getMax`, `getMinValue`, `getMaxValue`) throw on 2D / 3D / 4D buffers.

```cpp
#include <airtree/query/minmax/MinMax.hpp>

airtree::query::minmax::MinMax mm(histogram_buffer);

auto minCount = mm.getMin();        // bin(s) with the minimum count
auto maxCount = mm.getMax();        // bin(s) with the maximum count
auto minVal   = mm.getMinValue();   // smallest value present
auto maxVal   = mm.getMaxValue();   // largest value present

for (const auto& r : minCount) {
  std::cout << "Bin: [" << r.getLowerBound() << ", " << r.getUpperBound()
            << "], Count: " << r.getBinCount() << "\n";
}
```

`MinMaxResult` exposes `getLowerBound()`, `getUpperBound()`, and
`getBinCount()` (`uint32_t`).

**Result-vector size.** `getMin()` / `getMax()` return *every* bin sharing
the extremal count. On fine-grained tries (`xP`) this can exceed 100,000
entries. Use a coarser variant (`xF`) or post-process the result if you
need a single representative bin.

**`getMax()` and peak-finding.** For fine-grained tries (`xP`), `getMax()`
is dominated by sampling noise when N / num_bins is small: the returned
bin's midpoint may be 1–2σ from the actual mode. For peak-finding use a
coarser trie (`xF`) or post-process with [`BinBoundary`](#binboundary)
plus a kernel smoother.

### CDF

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`. Throws on 2D / 3D / 4D buffers.

```cpp
#include <airtree/query/cdf/CDF.hpp>

airtree::query::cdf::CDF cdf(histogram_buffer);

double prob_no_interp = cdf.getCDF(42.5);          // interpolate=false (default)
double prob_interp    = cdf.getCDF(42.5, true);    // linear interpolation in-bin
```

Returns the probability mass ≤ the given value (in `[0.0, 1.0]`). Setting
`interpolate=true` linearly interpolates within the matched bin instead of
returning the bin's lower-edge probability.

> **Constructor signature.** Unlike its sister classes (`Percentile`,
> `TopK`, `MinMax`, `BoundingBox`, `BinBoundary`) which take the buffer
> **by value**, `CDF`'s constructor takes a non-const lvalue reference:
> `CDF(std::vector<char>& buffer)`. The buffer is copied into an internal
> member, so its lifetime after construction does not matter — but you
> cannot construct a `CDF` directly from an rvalue (e.g. the return of
> `airtree::core::api::generate(...)`). Bind to a named variable first.

### BoundingBox (2D / 3D)

The most advanced query — fast spatial range counting against multi-dimensional
histograms.

> **Supported configs:** `2DxP` and `3DxP` only. The `Fast` variants (`2DxF`,
> `3DxF`) and 1D / 4D buffers all throw.

```cpp
#include <airtree/query/bounding-box/BoundingBox.hpp>

airtree::query::bounding_box::BoundingBox bb(histogram_buffer);

// 2D
airtree::query::bounding_box::BoundingBoxCoordinate2D box(
    /* x_min */ 10.0, /* x_max */ 20.0,
    /* y_min */ 30.0, /* y_max */ 40.0);

auto [safeResult, totalResult] = bb.getCounts(box);

auto [safeBox,  safeCount]  = safeResult;
auto [totalBox, totalCount] = totalResult;

std::cout << "Safe count:  " << safeCount  << "\n";
std::cout << "Total count: " << totalCount << "\n";

// 3D — same shape, with BoundingBoxCoordinate3D and the 3D overload of
// getCounts(); the result types are BoxCoordinate3DResultPair.
```

**Result semantics:**

- **Safe** — bins fully contained inside the query box.
- **Total** — safe bins + edge bins that may partially overlap the box.
  The true count for the box lies in `[safeCount, totalCount]`.

A `getCountsBatch(...)` overload is also available on `BoundingBox` for
processing many query boxes in one call — see the header.

### BinBoundary

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`, `2DxP`, `3DxP`, `4DxP`. The
> multi-dim `Fast` variants (`2DxF`, `3DxF`, `4DxF`) are not yet supported and
> throw.

When you need the raw bin boundaries of a histogram (e.g., to feed a custom
analysis pipeline) without going through the export tools, use the
`BinBoundary` API. The polymorphic entry point is the unsuffixed
`BinBoundary` class; `BinBoundary1D` / `BinBoundary2D` / `BinBoundary3D` /
`BinBoundary4D` are independent per-bin record value types (not subclasses
or polymorphic) returned inside a `std::variant`.

```cpp
#include <airtree/query/bin-boundary/BinBoundary.hpp>
#include <variant>

namespace bb_ns = airtree::query::bin_boundary;

bb_ns::BinBoundary bb(histogram_buffer);
auto result = bb.generateBinBoundaries();
auto boundaries_ptr = result.getBoundaries();
// std::unique_ptr<std::variant<BinBoundary1DList, BinBoundary2DList,
//                              BinBoundary3DList, BinBoundary4DList>>

if (std::holds_alternative<bb_ns::BinBoundary2DList>(*boundaries_ptr)) {
  const auto& bins = std::get<bb_ns::BinBoundary2DList>(*boundaries_ptr);
  for (const auto& bin : bins) {
    /* bin.getLowerBoundX(), bin.getUpperBoundX(),
       bin.getLowerBoundY(), bin.getUpperBoundY(), bin.getCount() */
  }
}
```

This is the same machinery the [Export API](#export-api) uses internally; you
only need it if you want bin edges without converting to Arrow / Parquet / CSV.

### GridQuery

> **Supported configs:** `2DxP`, `3DxP`, `4DxP`. 1D buffers and the `Fast`
> variants throw.

The multi-dimensional counterpart to a single `BoundingBox` count: instead of
one box you give one region per dimension plus a step count and scaling, and get
back a grid of non-overlapping cells, each with a count — a re-binned histogram
/ heatmap / cube over the region.

```cpp
#include <airtree/query/grid/GridQuery.hpp>

namespace grid = airtree::query::grid;

grid::GridQuery gq(histogram_buffer);   // 2DxP / 3DxP / 4DxP buffer

std::vector<grid::GridAxisSpec> axes = {
    { /*min*/ 0.0, /*max*/ 100.0, /*steps*/ 4, grid::GridScaling::Linear },
    { /*min*/ 0.0, /*max*/ 50.0,  /*steps*/ 2, grid::GridScaling::Linear },
};

// optional second argument: max_cells guard (default 1'000'000)
grid::GridResult res = gq.getGrid(axes);
```

`GridAxisSpec` (one per dimension; the count must match the buffer's
dimensionality):

- `min` / `max` — the axis range. `±inf` opens that end at the data extent and
  surfaces the infinity as its own partition.
- `steps` — partitions to split the finite range into.
- `scaling` — `Linear` (equal-width) or `Multiplicative` (geometric / log-spaced;
  requires a strictly positive resolved interior, else throws).

`GridResult`:

- `axis_intervals[d]` — the partitions on dimension `d`. Each `GridInterval` has
  `kind` (`Finite` / `NegInf` / `PosInf`), `lower`, `upper`, `lower_inclusive`,
  `upper_inclusive`, and `is_edge` (a single bin straddling the requested
  `min`/`max`).
- `counts` — a flat, **row-major (mixed-radix)** array of cell counts;
  `counts.size()` equals the product of the per-axis partition counts.
- `shape()` returns the per-axis partition counts; `materializeRows()` expands
  the flat array into one `GridCell { bounds, count }` per cell (including empty
  cells).

```cpp
// read cell (px, py) of a 2D result
std::size_t ny = res.axis_intervals[1].size();
uint64_t c = res.counts[px * ny + py];
```

**Semantics.** Cell bounds snap to the histogram's internal bin edges, so
reported bounds may differ slightly from the requested values and follow the
native inclusivity (positive bins `[lo, hi)`, negative `(lo, hi]`). A bin
straddling the requested `min`/`max` is surfaced as its own `is_edge` row with
its full count, so the caller can pro-rate it. Zero routes into the finite
partition containing it; NaN is excluded. `getGrid` throws
`std::invalid_argument` on malformed axes and `std::runtime_error` on an
unsupported buffer or if the projected cell count exceeds `max_cells`.

---

## Export API

> **Supported configs:** `1DxT`, `1DxF`, `1DxP`, `2DxP`, `3DxP`, `4DxP`. The
> multi-dim `Fast` variants (`2DxF`, `3DxF`, `4DxF`) are not yet supported and
> throw.

```cpp
#include <airtree/export/AirTreeExporter.hpp>
```

A single function handles the full pipeline (parse → bin boundaries → Arrow
table → write) for any AirTree buffer:

```cpp
namespace airtree::xport {

enum class ExportFormat {
  ARROW,    // Apache Arrow IPC (.arrow)
  PARQUET,  // Apache Parquet (.parquet)
  CSV       // Comma-separated values (.csv)
};

void exportAirTree(const std::vector<char>& buffer,
                   const std::string& output_path,
                   ExportFormat format = ExportFormat::ARROW);

} // namespace airtree::xport
```

> The namespace is **`airtree::xport`** (not `airtree::export` — `export` is a
> reserved keyword in C++).

**Example:**

```cpp
#include <airtree/export/AirTreeExporter.hpp>

std::vector<char> buf = /* generated histogram */;
airtree::xport::exportAirTree(buf, "histogram.parquet",
                              airtree::xport::ExportFormat::PARQUET);
```

The output schema, dimensions, and special-value handling are identical to
the [`airtree-export` CLI](cli.md#airtree-export).

---

## Merge API

```cpp
#include <airtree/merge/AirTreeMerge.hpp>
```

```cpp
namespace airtree::merge {

// Returns the merged histogram as a buffer
std::vector<char> mergeAirTree(const std::vector<char>& buffer1,
                               const std::vector<char>& buffer2);

// Merges and writes the result directly to a file
void mergeAirTree(const std::vector<char>& buffer1,
                  const std::vector<char>& buffer2,
                  const std::string& output_path);

// Merges any number of buffers in one pass
std::vector<char> mergeAirTrees(const std::vector<std::vector<char>>& buffers);

} // namespace airtree::merge
```

All inputs must have **identical configuration** (same schema, same trie
variant). Incompatible inputs throw `std::runtime_error`; an empty list throws
`std::invalid_argument`.

`mergeAirTrees` produces exactly the bytes a pairwise fold would, but reads each
input once: for 1D schemas it streams the N buffers side by side, re-encoding
only the nodes that more than one input populates and copying the rest verbatim.
Prefer it whenever you combine more than two histograms (a range of time
windows, shards of one dataset). Other schemas currently fold pairwise.

**Example:**

```cpp
#include <airtree/merge/AirTreeMerge.hpp>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
  auto read_file = [](const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(f), {});
  };

  auto buf1 = read_file("histogram1.airtree");
  auto buf2 = read_file("histogram2.airtree");

  // Option 1 — get the merged buffer in memory
  auto merged = airtree::merge::mergeAirTree(buf1, buf2);

  // Option 2 — merge directly to a file
  airtree::merge::mergeAirTree(buf1, buf2, "merged_histogram.airtree");

  // Option 3 — many inputs in one pass
  auto range = airtree::merge::mergeAirTrees({buf1, buf2, read_file("histogram3.airtree")});

  std::cout << "Merge complete: " << merged.size() << " bytes\n";
}
```

---

## Reader API

The same reader the CLI uses is exposed publicly, so you can build histograms
straight from Parquet, CSV, or raw binary files in C++:

```cpp
#include <airtree/reader/file/Reader.hpp>

// One column per entry; each entry is one of the four supported numeric
// element types. Declared at namespace scope (not inside
// airtree::reader::file) — alias as-is.
using InputDataVariant = std::variant<std::vector<int32_t>,
                                      std::vector<int64_t>,
                                      std::vector<float>,
                                      std::vector<double>>;
using InputDataVector  = std::vector<InputDataVariant>;

namespace airtree::reader::file {

enum SUPPORTED_FILE_TYPE { AT_BINARY, AT_PARQUET, AT_CSV };
enum SUPPORTED_DATA_TYPE {
  AT_INT32, AT_INT64, AT_FLOAT, AT_DOUBLE, AT_IGNORE
};

// Parse a file into one column per requested entry.
// - AT_BINARY: 'columns' is empty; 'data_type' selects the element type.
// - AT_PARQUET / AT_CSV: 'columns' is required; 'data_type' is ignored
//   (use AT_IGNORE) — element types come from the file's own schema /
//   Arrow type inference.
InputDataVector parse_file(const std::string& path,
                           SUPPORTED_FILE_TYPE file_type,
                           SUPPORTED_DATA_TYPE data_type,
                           const std::vector<std::string>& columns);

} // namespace airtree::reader::file
```

CSV files must have a header row; column names in the `columns` argument are
matched against that header. Accepted numeric types are the same across all
three readers: `int32`, `int64`, `float`, `double`.

Each column comes back as a `std::variant` of the four supported
`std::vector<T>` element types. To feed them into the lower-level
`generate(arrays, options)` overload from
`<airtree/core/api/AirTreeGenerator.hpp>`, wrap each variant in an `FPHArray`
via `std::visit` and `buildFPHArray`:

```cpp
#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/core/common/FPHArray.hpp>
#include <airtree/reader/file/Reader.hpp>

namespace api = airtree::core::api;
namespace rdr = airtree::reader::file;

auto columns = rdr::parse_file("data.parquet", rdr::AT_PARQUET,
                               rdr::AT_IGNORE, {"x", "y"});

std::vector<FPHArray> arrays;
arrays.reserve(columns.size());
for (const auto& col : columns) {
  std::visit([&](const auto& vec) {
    arrays.push_back(buildFPHArray(vec.data(),
                                   static_cast<int>(vec.size())));
  }, col);
}

std::vector<const FPHArray*> ptrs;
ptrs.reserve(arrays.size());
for (const auto& a : arrays) ptrs.push_back(&a);

api::AirTreeOptions opts{ .dimensions = 2,
                          .type       = api::ConfigType::XP };
auto buffer = api::generate(ptrs, opts);
```

> Pulling in the reader transitively pulls in Apache Arrow / Parquet runtime
> dependencies, which the convenience template overloads in
> `AirTreeGenerator.hpp` do not require.

---

## Linking

AirTree installs as a set of **static archives** (`.a`), not a single shared
library. Link the specific archives you need plus their transitive
dependencies directly with `-L` / `-I` and the library list below.

The library prefix is `/opt/airmettle/airtree/<version>/lib/` by default; set
`-L` and `-I` accordingly.

**Transitive dependencies.** The AirTree archives reference (and the link
line must provide) the following on Linux:

| Library                    | Why                                          |
| -------------------------- | -------------------------------------------- |
| `-lsnappy`                 | Compression used by `libarrow.a`             |
| `-lboost_system`           | spdlog / arrow internals                     |
| `-lboost_filesystem`       | spdlog / arrow internals                     |
| `-lboost_thread`           | arrow internals                              |
| `-lssl -lcrypto`           | airtree-core / airtree-util                  |
| `-lzstd -llz4 -lz`         | arrow / parquet compression                  |
| `-lthrift`                 | parquet                                      |
| `-lspdlog`                 | airtree logging                              |
| `-lpthread -ldl -lrt -lm`  | system                                       |

These are usually system-installed on the supported platforms. If your
target system doesn't have them, the build's per-architecture dep cache
(see [Installation → Where Build Output Goes](install.md#where-build-output-goes))
contains compatible builds you can point `-L` at.

**Generate-only (1D – 4D, in-memory data):**
```bash
g++ -std=c++17 your_program.cpp \
  -I/opt/airmettle/airtree/<version>/include \
  -L/opt/airmettle/airtree/<version>/lib \
  -lairtree-core -lairtree-util \
  -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm \
  -o your_program
```

**Generate from a file (Parquet / binary):**
```bash
g++ -std=c++17 your_program.cpp \
  -I/opt/airmettle/airtree/<version>/include \
  -L/opt/airmettle/airtree/<version>/lib \
  -lairtree-reader -lairtree-core -lairtree-util \
  -larrow -lparquet -lthrift -lsnappy -lzstd -llz4 -lz \
  -lboost_system -lboost_filesystem -lboost_thread \
  -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm \
  -o your_program
```

**Querying:**
```bash
... -lairtree-query -lairtree-core -lairtree-util \
    -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm ...
```

**Merging:**
```bash
... -lairtree-merge -lairtree-core -lairtree-util \
    -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm ...
```

**Exporting:**
```bash
... -lairtree-export -lairtree-query -lairtree-core -lairtree-util \
    -larrow -lparquet -lthrift -lsnappy -lzstd -llz4 -lz \
    -lboost_system -lboost_filesystem -lboost_thread \
    -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm ...
```

> Static archives are sensitive to link order — list `airtree-*` archives
> *before* their transitive deps (Arrow, Parquet, etc.) on the command line.

---

## See Also

- [CLI Reference](cli.md) — the same operations from the command line
- [Installation](install.md) — install paths and prerequisites
- [Python Bindings](python.md) — coming soon
