# AirTree

**Hierarchical Multi-Dimensional Histograms.**

AirTree is a high-performance C++ library and CLI suite for building compact,
queryable histogram buffers from large datasets. It supports 1D – 4D data via
specialized trie-based structures optimized for both memory footprint and
query speed, and provides exact merging across shards or batches.

---

## Documentation

| Doc | What's in it |
| --- | --- |
| [Installation & Build](docs/install.md) | Supported platforms, prerequisites, build script, install layout |
| [CLI Reference](docs/cli.md) | `airtree`, `airtree-export`, `airtree-merge` — every flag and example |
| [C++ Library API](docs/cpp-api.md) | Generate, query, merge, export, and reader headers |
| [Python Bindings](docs/python.md) | *Coming soon* |

## What's in the Box

| Component             | Type         | Purpose                                            |
| --------------------- | ------------ | -------------------------------------------------- |
| `airtree`             | CLI          | Generate, query, merge, and export histograms      |
| `airtree-export`      | CLI          | Export histograms to Arrow / Parquet / CSV         |
| `airtree-merge`       | CLI          | Merge two compatible histograms                    |
| `libairtree-*.a`      | Static libs  | C++ libraries — core, query, merge, export, reader |
| `<airtree/...>`       | Headers      | Public C++ API                                     |

See [Installation](docs/install.md) for paths and packaging details.

## Supported Schemas

| Schema | Dimensions | Variant | Input                  |
| ------ | ---------- | ------- | ---------------------- |
| `1DxT` | 1D         | Tiny    | Binary, Parquet, CSV   |
| `1DxF` | 1D         | Fast    | Binary, Parquet, CSV   |
| `1DxP` | 1D         | Precise | Binary, Parquet, CSV   |
| `2DxF` \* / `2DxP` | 2D | Fast / Precise | Parquet, CSV |
| `3DxF` \* / `3DxP` | 3D | Fast / Precise | Parquet, CSV |
| `4DxF` \* / `4DxP` | 4D | Fast / Precise | Parquet, CSV |

\* Multi-dim `Fast` variants currently support **generate** and **merge** only.
The export pipeline and CLI / library queries do not yet handle them.

See [CLI Reference → Histogram Schemas](docs/cli.md#histogram-schemas) for
guidance on choosing a variant.

## Quick Start

Build and install (Ubuntu / Debian):

```bash
./tools/build/build.sh
sudo dpkg -i cmake-build-*/airtree-*-Linux.deb
export PATH="/opt/airmettle/airtree/$(cat version.txt)/bin:$PATH"
airtree --version    # confirms the install
```

A tiny sample (`examples/sales.csv`, 60 rows with `price` and `quantity`
columns) ships with the repo. Scalar CLI queries are 1D; the
[`grid`](docs/cli.md#grid) query covers `2DxP` / `3DxP` / `4DxP`.

```bash
# Build a 1D histogram over the price column
airtree generate csv -i examples/sales.csv -o price.airtree -s 1DxP -c price

# Bins covering the top 10% of total count mass, walked from the largest
# values inward — surfaces the high-end sales:
airtree query topk -i price.airtree -o topk.csv -k 10.0
cat topk.csv
# lower,upper,count
# 312,312.125,1
# 210.5,210.53125,1
# 145,145.03125,1
# 89.984375,90,1
# 67.25,67.265625,1
# 55.75,55.7578125,1

# Median price
airtree query percentile -i price.airtree -o median.csv -p 50.0
cat median.csv
# percentile,value
# 50,13.7509765625
```

Build a 2D histogram over `price × quantity`, then run a grid query or export
the bins for downstream analysis:

```bash
airtree generate csv -i examples/sales.csv -o sales2d.airtree -s 2DxP \
  -c price quantity

# 5×3 grid over the [0,50] × [0,30] region
airtree query grid -i sales2d.airtree -o grid.csv -a 0:50:5 -a 0:30:3

# Or export every bin to CSV / Parquet / Arrow
airtree export sales2d.airtree --csv --output sales2d_bins.csv
```

Merge histograms from parallel batches:

```bash
airtree merge batch1.airtree batch2.airtree merged.airtree
```

> `airtree merge` / `airtree export` are equivalent to the standalone
> `airtree-merge` / `airtree-export` binaries — same flags, same behavior.
> Pick whichever fits your scripts.

Use it from C++:

```cpp
#include <airtree/core/api/AirTreeGenerator.hpp>

std::vector<double> data = { /* ... */ };
auto buffer = airtree::core::api::generate(data);   // 1D, default ConfigType::XP
```

For full programmatic usage (querying, merging, exporting from C++) see the
[C++ Library API](docs/cpp-api.md).

## Patent & Licensing

**Patent Pending** — U.S. Patent Application Publication No.
[2025/0217930 A1](https://patents.google.com/patent/US20250217930A1).

As noted in the [LICENSE](LICENSE) file, this software is provided for
**non-commercial use only**.

For commercial licensing, support, or enterprise usage, please contact
**support@airmettle.com**.

Cloud services powered by this technology are launching in **June 2026**,
starting with **Azure**.
