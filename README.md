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

| Schema | Dimensions | Variant | Input            |
| ------ | ---------- | ------- | ---------------- |
| `1DxT` | 1D         | Tiny    | Binary           |
| `1DxF` | 1D         | Fast    | Binary, Parquet  |
| `1DxP` | 1D         | Precise | Binary, Parquet  |
| `2DxF` \* / `2DxP` | 2D | Fast / Precise | Parquet  |
| `3DxF` \* / `3DxP` | 3D | Fast / Precise | Parquet  |
| `4DxF` \* / `4DxP` | 4D | Fast / Precise | Parquet  |

\* Multi-dim `Fast` variants currently support **generate** and **merge** only.
The export pipeline and CLI / library queries do not yet handle them.

See [CLI Reference → Histogram Schemas](docs/cli.md#histogram-schemas) for
guidance on choosing a variant.

## Quick Start

Build and install (Ubuntu / Debian):

```bash
./tools/build/build.sh
sudo dpkg -i cmake-build-*/airtree-*-Linux.deb
export PATH="/opt/airmettle/airtree/<version>/bin:$PATH"
```

Generate a 1D histogram and run a query on it (CLI queries are currently 1D-only — see
[CLI Reference → `airtree query`](docs/cli.md#airtree-query)):

```bash
# Build a 1D histogram over a single column
airtree generate parquet \
  -i data/sales.parquet \
  -o price.airtree \
  -s 1DxP \
  -c price

# Bins covering the top 10% of count mass (from largest values inward)
airtree query topk -i price.airtree -o topk.txt -k 10.0
```

Generate a 2D histogram and export it for downstream analysis:

```bash
airtree generate parquet \
  -i data/sales.parquet \
  -o sales.airtree \
  -s 2DxP \
  -c price quantity

airtree export sales.airtree --parquet --output ./out/
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

## References

- AirTree Patent: *US Patent No. TODO* <!-- TODO: actual patent number / link -->
- License: *TODO* <!-- TODO: link to non-commercial license file -->
