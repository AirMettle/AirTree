# CLI Reference

[← Back to README](../README.md)

AirTree ships three command-line tools:

| Binary               | Purpose                                                            |
| -------------------- | ------------------------------------------------------------------ |
| `airtree`            | [Generate](#airtree-generate), [query](#airtree-query), [merge](#airtree-merge), and [export](#airtree-export) histograms |
| `airtree-export`     | [Export](#airtree-export) histograms to Arrow / Parquet / CSV (standalone) |
| `airtree-merge`      | [Merge](#airtree-merge) two compatible histograms (standalone)     |

All three are installed under `/opt/airmettle/airtree/<version>/bin/` —
see [Installation](install.md) for how to put them on your `$PATH`.

---

## Table of Contents

- [Histogram Schemas](#histogram-schemas)
- [`airtree generate`](#airtree-generate)
  - [Generate from Parquet](#generate-from-parquet)
  - [Generate from CSV](#generate-from-csv)
  - [Generate from Binary File](#generate-from-binary-file-1d-only)
- [`airtree query`](#airtree-query)
  - [Top-K](#top-k)
  - [Percentile](#percentile)
  - [Min / Max Count](#min--max-count)
  - [Min / Max Value](#min--max-value)
  - [Grid](#grid)
- [`airtree-export`](#airtree-export)
- [`airtree-merge`](#airtree-merge)

---

## Histogram Schemas

A schema selects the dimensionality and the trie variant. Variants trade
precision for size / speed:

| Schema | Dimensions | Variant     | Input Types          |
| ------ | ---------- | ----------- | -------------------- |
| `1DxT` | 1D         | Tiny        | Binary, Parquet, CSV |
| `1DxF` | 1D         | Fast        | Binary, Parquet, CSV |
| `1DxP` | 1D         | Precise     | Binary, Parquet, CSV |
| `2DxF` \* | 2D      | Fast        | Parquet, CSV         |
| `2DxP` | 2D         | Precise     | Parquet, CSV         |
| `3DxF` \* | 3D      | Fast        | Parquet, CSV         |
| `3DxP` | 3D         | Precise     | Parquet, CSV         |
| `4DxF` \* | 4D      | Fast        | Parquet, CSV         |
| `4DxP` | 4D         | Precise     | Parquet, CSV         |

\* `2DxF`, `3DxF`, `4DxF` currently support **generate** and **merge** only.
[Export](#airtree-export) and queries — both CLI and the C++
[`BoundingBox`](cpp-api.md#boundingbox-2d--3d) / [`BinBoundary`](cpp-api.md#binboundary)
APIs — throw on these schemas. Use the matching `xP` variant if you need
analysis paths.

**Variant guidance**

- **Tiny (`xT`)** — smallest output, lowest fidelity. Available only for 1D.
- **Fast (`xF`)** — fastest to build and query, moderate fidelity.
- **Precise (`xP`)** — highest fidelity, larger output. The default in the
  C++ API.

---

## `airtree generate`

Generate a histogram buffer (`.airtree`) from an input file. Three input modes
are supported: **Parquet** (1D – 4D), **CSV** (1D – 4D), and **raw binary**
(1D only).

### Generate from Parquet

```bash
airtree generate parquet \
  -i /path/to/input.parquet \
  -o /path/to/histogram.airtree \
  -s 2DxP \
  -c column_name_1 column_name_2
```

| Flag                | Required | Description                                                   |
| ------------------- | :------: | ------------------------------------------------------------- |
| `-i, --input`       | Yes      | Path to the Parquet file                                      |
| `-o, --output`      | Yes      | Output histogram buffer file                                  |
| `-s, --schema`      | Yes      | One of the schemas listed [above](#histogram-schemas)         |
| `-c, --columns`     | Yes      | Space-separated column names; count must match schema dims    |

**Accepted Arrow column types.** `int32`, `int64`, `float`, `double`. Other
Arrow types (`string`, `decimal`, `timestamp`, etc.) are rejected. Nullable
columns are accepted but null values are skipped — the histogram counts
non-null entries only.

**TSV input is not accepted.** Convert TSV to CSV (or Parquet) first.

**Example — 2D histogram over `price` and `quantity`:**

```bash
airtree generate parquet \
  -i data/sales.parquet \
  -o results/sales_2d.airtree \
  -s 2DxP \
  -c price quantity
```

### Generate from CSV

```bash
airtree generate csv \
  -i /path/to/input.csv \
  -o /path/to/histogram.airtree \
  -s 2DxP \
  -c column_name_1 column_name_2
```

| Flag                | Required | Description                                                   |
| ------------------- | :------: | ------------------------------------------------------------- |
| `-i, --input`       | Yes      | Path to the CSV file (header row required)                    |
| `-o, --output`      | Yes      | Output histogram buffer file                                  |
| `-s, --schema`      | Yes      | One of the schemas listed [above](#histogram-schemas)         |
| `-c, --columns`     | Yes      | Space-separated column names; count must match schema dims    |

**Accepted column types** — same as Parquet: `int32`, `int64`, `float`,
`double`. The CSV reader uses Arrow's type inference; if a column doesn't
auto-detect to one of these numeric types it will be rejected.

**Example — 2D histogram over `price` and `quantity` from a CSV:**

```bash
airtree generate csv \
  -i data/sales.csv \
  -o results/sales_2d.airtree \
  -s 2DxP \
  -c price quantity
```

### Generate from Binary File (1D only)

```bash
airtree generate binary \
  -i /path/to/data.bin \
  -o /path/to/histogram.airtree \
  -s 1DxP \
  -d double
```

| Flag                | Required | Description                                                   |
| ------------------- | :------: | ------------------------------------------------------------- |
| `-i, --input`       | Yes      | Path to the raw binary file                                   |
| `-o, --output`      | Yes      | Output histogram buffer file                                  |
| `-s, --schema`      | Yes      | Must be a 1D schema (`1DxT`, `1DxF`, or `1DxP`)               |
| `-d, --data-type`   | Yes      | One of: `int32`, `int64`, `float`, `double`                   |

The raw binary file is interpreted as a tightly-packed array of values of the
given type — no header, no metadata.

**Example:**

```bash
airtree generate binary \
  -i data/temperatures.bin \
  -o results/temp_1d.airtree \
  -s 1DxF \
  -d float
```

---

## `airtree query`

All queries operate directly on a generated `.airtree` histogram. They are extremely
fast — the trie is parsed in place.

> **Most CLI queries are 1D-only.** The scalar subcommands (`topk`, `min_count`,
> `max_count`, `min_value`, `max_value`, `percentile`) support only 1D histograms
> (`1DxT`, `1DxF`, `1DxP`) and throw *"Unsupported dimensions or bit length."* on a
> 2D / 3D / 4D buffer. The exception is [`grid`](#grid), which queries the
> multi-dimensional Precise variants (`2DxP`, `3DxP`, `4DxP`). For 2D / 3D
> point-range counts there is also the C++
> [`BoundingBox`](cpp-api.md#boundingbox-2d--3d) API.

```bash
airtree query <subcommand> [options]
```

| Subcommand    | Description                                                                                              | Required Flags         |
| ------------- | -------------------------------------------------------------------------------------------------------- | ---------------------- |
| `topk`        | Top-K% (tail-by-mass quantile, **not** rank-by-count) — see [below](#top-k)                              | `-i`, `-o`, `-k`       |
| `min_count`   | Bins with the minimum count                                                                              | `-i`, `-o`             |
| `max_count`   | Bin with the maximum count — see [precision note](#max_count-on-fine-tries)                              | `-i`, `-o`             |
| `min_value`   | Smallest value present in the histogram                                                                  | `-i`, `-o`             |
| `max_value`   | Largest value present in the histogram                                                                   | `-i`, `-o`             |
| `percentile`  | Computes a percentile (0 – 100)                                                                          | `-i`, `-o`, `-p`       |
| `grid`        | Re-binned histogram / heatmap over a region of a `2DxP` / `3DxP` / `4DxP` buffer — see [below](#grid)    | `-i`, `-o`, `-a`       |

**Common flags**

- `-i, --input` — path to the generated `.airtree` file
- `-o, --output` — path to write the results (CSV)

**Output format.** `topk`, `min_count`, `max_count`, `min_value`, and
`max_value` write a CSV with a `lower,upper,count` header followed by one row
per result bin. `percentile` writes a `percentile,value` header and a single
row. `grid` writes its own multi-column CSV — see [Grid](#grid). Bin bounds are
written at full `double` precision so they round-trip exactly.

> Currently only the queries listed above are exposed via the CLI. **CDF** and
> **bounding-box** queries are available through the [C++ library](cpp-api.md#query-apis).

### Top-K

```bash
airtree query topk \
  -i results/sales_2d.airtree \
  -o results/topk.csv \
  -k 10.0
```

`-k` is a percentage (0 – 100), not a count.

**Semantics — tail-by-mass, not rank-by-count.** `topk` returns the bins
covering the top `k`% of total *count*, walked **inward from the largest
values** (`+Inf`, then descending bins, then `-Inf` last). It is *not* a
"sort all bins by count and take the top k%" operation — those two queries
return entirely different bin sets.

For a unimodal distribution (e.g. a Gaussian centred at zero), `topk -k 1.0`
returns bins from the upper tail until their cumulative count reaches 1% of
the total — *not* the densest bins around the mode. If you want the densest
bins, use `max_count` (one bin) or post-process the
[`airtree-export`](#airtree-export) output.

### `max_count` on fine tries

For fine-grained tries (`xP`), `max_count` is dominated by sampling noise
when N / num_bins is small: the returned bin's midpoint can sit 1–2σ from
the actual mode and contain only a handful of samples. For peak-finding,
prefer the coarser `xF` variant or post-process exported bin counts.

### Percentile

```bash
airtree query percentile \
  -i results/temp_1d.airtree \
  -o results/median.csv \
  -p 50.0
```

`-p` is a percentile expressed in percent (0 – 100).

### Min / Max Count

```bash
airtree query min_count -i histogram.airtree -o min_count.csv
airtree query max_count -i histogram.airtree -o max_count.csv
```

### Min / Max Value

```bash
airtree query min_value -i histogram.airtree -o min_value.csv
airtree query max_value -i histogram.airtree -o max_value.csv
```

### Grid

The only multi-dimensional CLI query. It subdivides a region into a grid of
non-overlapping cells and writes one CSV row per cell — a 2D heatmap or 3D / 4D
cube at a resolution you choose. Supported on `2DxP`, `3DxP`, and `4DxP`
buffers.

```bash
# 2D heatmap: x in [0,100] split into 4, y in [0,50] split into 2
airtree query grid \
  -i sales.airtree \
  -o grid.csv \
  -a 0:100:4 \
  -a 0:50:2
```

Pass one `-a/--axis` per dimension (the count must match the buffer's
dimensionality), each formatted as `min:max:steps[:scaling]`:

- `min` / `max` — the axis range. Use `inf` / `-inf` for an open end; the grid
  then starts/ends at the data extent and surfaces the infinity as its own row.
- `steps` — number of partitions to split the finite range into.
- `scaling` (optional) — `linear` (default, equal-width steps) or `mult`
  (geometric / log-spaced steps; requires a strictly positive range).

Other flags:

- `--max-cells` — guard on the total number of output cells (default
  `1000000`); the query throws rather than allocate beyond it.

**Output.** A CSV carrying the full interval semantics per dimension, then the
count. Each dimension contributes six columns: `dimN_min`, `dimN_max`,
`dimN_min_inclusive`, `dimN_max_inclusive`, `dimN_kind`, `dimN_is_edge`.

```
dim0_min,dim0_max,dim0_min_inclusive,dim0_max_inclusive,dim0_kind,dim0_is_edge,dim1_min,dim1_max,dim1_min_inclusive,dim1_max_inclusive,dim1_kind,dim1_is_edge,count
0,25,true,false,finite,false,0,25,true,false,finite,false,239
0,25,true,false,finite,false,25,50,true,false,finite,false,256
...
```

- `min` / `max` — snapped to the histogram's internal bin edges (so they may not
  equal the exact requested values).
- `min_inclusive` / `max_inclusive` — native inclusivity (positive bins
  `[lo, hi)`, negative `(lo, hi]`).
- `kind` — `finite`, `-inf`, or `+inf` (the infinity rows appear when an axis end
  is `inf` / `-inf`).
- `is_edge` — `true` for a bin straddling the requested `min` / `max`, reported
  with its full count so you can pro-rate it.

Empty cells are included.

```bash
# 3D, with an open upper end on z and log-spaced x
airtree query grid -i cube.airtree -o cube.csv \
  -a 1:1000:5:mult -a 0:50:10 -a 0:inf:4
```

---

## `airtree-export`

Convert an AirTree histogram buffer (`.airtree`) into an analysis-ready tabular
format:

- **Apache Arrow** (default, `.arrow` IPC)
- **Parquet** (recommended for large histograms)
- **CSV** (human-readable, with metadata in `#` comment header)

```bash
airtree-export <input_histogram.airtree> [OPTIONS]
```

| Option              | Description                                  | Default                                   |
| ------------------- | -------------------------------------------- | ----------------------------------------- |
| `--parquet`         | Export to Parquet                            | —                                         |
| `--csv`             | Export to CSV                                | —                                         |
| `--output <path>`   | Output file or directory                     | Same dir as input, with matching extension|
| *(no format flag)*  | Export to Arrow IPC (`.arrow`)               | Arrow                                     |

- Specify at most one of `--parquet` or `--csv`.
- If `--output` points to a directory, the file is auto-named from the input
  stem plus the format's extension.
- Works with any AirTree histogram (1D – 4D, any schema).

### What's in the Exported Table

One row per bin, with columns adjusted for dimensionality:

- **1D**: `x-min`, `x-max`, `counts`
- **2D**: `x-min`, `x-max`, `y-min`, `y-max`, `counts`
- **3D**: `x-min`, `x-max`, `y-min`, `y-max`, `z-min`, `z-max`, `counts`
- **4D**: `x-min`, `x-max`, `y-min`, `y-max`, `z-min`, `z-max`, `w-min`,
  `w-max`, `counts`

### Special Value Handling

Floating-point edge cases (`+Inf`, `-Inf`, `+0`, `-0`, `NaN`) are preserved as
**metadata** rather than rows:

- **Parquet / Arrow** — key-value entries on the schema metadata.
- **CSV** — `#`-prefixed comment lines at the top of the file.

Downstream tools that read schema metadata (`pyarrow`, `pandas` via Arrow, etc.)
get the full statistical picture.

### Examples

```bash
# Default Arrow export (fastest, most compact)
airtree-export results/sales_2d.airtree
# → results/sales_2d.arrow

# Parquet
airtree-export results/temp_1d.airtree --parquet
# → results/temp_1d.parquet

# CSV with explicit filename
airtree-export results/histogram.airtree --csv --output ./exports/my_histogram.csv

# Export to a directory (auto-named)
airtree-export data/4d_histogram.airtree --parquet --output ./exports/
# → exports/4d_histogram.parquet
```

#### Via `airtree`

The same operations are exposed as `airtree export` — identical flags, identical
behavior:

```bash
# Default Arrow export
airtree export results/sales_2d.airtree

# Parquet
airtree export results/temp_1d.airtree --parquet

# CSV with explicit filename
airtree export results/histogram.airtree --csv --output ./exports/my_histogram.csv

# Export to a directory (auto-named)
airtree export data/4d_histogram.airtree --parquet --output ./exports/
```

### Loading the Output in Python

```python
import pyarrow.parquet as pq
table = pq.read_table("analysis/histogram.parquet")
print(table)
```

---

## `airtree-merge`

Combine two compatible AirTree histograms into a single histogram. The merge
is **exact** — no approximation — and produces a valid histogram that can be
queried, exported, or merged again.

```bash
airtree-merge <input1.airtree> <input2.airtree> <output.airtree>
```

The two inputs must have **identical configuration** (same schema, same
dimensionality, same trie variant). Mixing schemas raises a clear error.

**Example:**

```bash
# Two 2D histograms from different data batches
airtree-merge batch1_2d.airtree batch2_2d.airtree merged_2d.airtree

# Two 1D histograms from parallel runs
airtree-merge run1_1d.airtree run2_1d.airtree final_1d.airtree
```

#### Via `airtree`

Equivalent invocations through `airtree`:

```bash
airtree merge batch1_2d.airtree batch2_2d.airtree merged_2d.airtree
airtree merge run1_1d.airtree run2_1d.airtree final_1d.airtree
```

**Workflow:**

1. Generate one histogram per shard / batch with the same schema:
   ```bash
   airtree generate parquet -i shard1.parquet -o part1.airtree -s 2DxP -c price quantity
   airtree generate parquet -i shard2.parquet -o part2.airtree -s 2DxP -c price quantity
   ```
2. Merge them (either form works):
   ```bash
   airtree-merge part1.airtree part2.airtree combined.airtree
   # or
   airtree merge part1.airtree part2.airtree combined.airtree
   ```
3. Use the result like any other histogram — query it, export it, or merge it
   again with more data.

---

## See Also

- [Installation](install.md) — building and installing AirTree
- [C++ Library API](cpp-api.md) — programmatic generate / query / export / merge
- [Python Bindings](python.md) — coming soon
