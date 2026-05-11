# CLI Reference

[← Back to README](../README.md)

AirTree ships three command-line tools:

| Binary               | Purpose                                                            |
| -------------------- | ------------------------------------------------------------------ |
| `airtree-cli`        | [Generate](#airtree-cli-generate) and [query](#airtree-cli-query) histograms |
| `AirTreeExport`      | [Export](#airtreeexport) histograms to Arrow / Parquet / CSV       |
| `airtree-merge-cli`  | [Merge](#airtree-merge-cli) two compatible histograms              |

All three are installed under `/opt/airmettle/airtree/<version>/bin/` —
see [Installation](install.md) for how to put them on your `$PATH`.

---

## Table of Contents

- [Histogram Schemas](#histogram-schemas)
- [`airtree-cli generate`](#airtree-cli-generate)
  - [Generate from Parquet](#generate-from-parquet)
  - [Generate from Binary File](#generate-from-binary-file-1d-only)
- [`airtree-cli query`](#airtree-cli-query)
  - [Top-K](#top-k)
  - [Percentile](#percentile)
  - [Min / Max Count](#min--max-count)
  - [Min / Max Value](#min--max-value)
- [`AirTreeExport`](#airtreeexport)
- [`airtree-merge-cli`](#airtree-merge-cli)

---

## Histogram Schemas

A schema selects the dimensionality and the trie variant. Variants trade
precision for size / speed:

| Schema | Dimensions | Variant     | Input Types        |
| ------ | ---------- | ----------- | ------------------ |
| `1DxT` | 1D         | Tiny        | Binary             |
| `1DxF` | 1D         | Fast        | Binary, Parquet    |
| `1DxP` | 1D         | Precise     | Binary, Parquet    |
| `2DxF` | 2D         | Fast        | Parquet            |
| `2DxP` | 2D         | Precise     | Parquet            |
| `3DxF` | 3D         | Fast        | Parquet            |
| `3DxP` | 3D         | Precise     | Parquet            |
| `4DxF` | 4D         | Fast        | Parquet            |
| `4DxP` | 4D         | Precise     | Parquet            |

**Variant guidance**

- **Tiny (`xT`)** — smallest output, lowest fidelity. Available only for 1D.
- **Fast (`xF`)** — fastest to build and query, moderate fidelity.
- **Precise (`xP`)** — highest fidelity, larger output. The default in the
  C++ API.

---

## `airtree-cli generate`

Generate a histogram buffer (`.bin`) from an input file. Two input modes are
supported: **Parquet** (1D – 4D) and **raw binary** (1D only).

### Generate from Parquet

```bash
airtree-cli generate parquet \
  -i /path/to/input.parquet \
  -o /path/to/histogram.bin \
  -s 2DxP \
  -c column_name_1 column_name_2
```

| Flag                | Required | Description                                                   |
| ------------------- | :------: | ------------------------------------------------------------- |
| `-i, --input`       | ✅       | Path to the Parquet file                                      |
| `-o, --output`      | ✅       | Output histogram buffer file                                  |
| `-s, --schema`      | ✅       | One of the schemas listed [above](#histogram-schemas)         |
| `-c, --columns`     | ✅       | Space-separated column names; count must match schema dims    |

**Accepted Arrow column types.** `int32`, `int64`, `float`, `double`. Other
Arrow types (`string`, `decimal`, `timestamp`, etc.) are rejected. Nullable
columns are accepted but null values are skipped — the histogram counts
non-null entries only.

**CSV / TSV input is not accepted.** If your data is in CSV, convert it to
Parquet first:

```python
import pyarrow.csv as csv
import pyarrow.parquet as pq

pq.write_table(csv.read_csv("data.csv"), "data.parquet")
```

**Example — 2D histogram over `price` and `quantity`:**

```bash
airtree-cli generate parquet \
  -i data/sales.parquet \
  -o results/sales_2d.bin \
  -s 2DxP \
  -c price quantity
```

### Generate from Binary File (1D only)

```bash
airtree-cli generate binary \
  -i /path/to/data.bin \
  -o /path/to/histogram.bin \
  -s 1DxP \
  -d double
```

| Flag                | Required | Description                                                   |
| ------------------- | :------: | ------------------------------------------------------------- |
| `-i, --input`       | ✅       | Path to the raw binary file                                   |
| `-o, --output`      | ✅       | Output histogram buffer file                                  |
| `-s, --schema`      | ✅       | Must be a 1D schema (`1DxT`, `1DxF`, or `1DxP`)               |
| `-d, --data-type`   | ✅       | One of: `int32`, `int64`, `float`, `double`                   |

The raw binary file is interpreted as a tightly-packed array of values of the
given type — no header, no metadata.

**Example:**

```bash
airtree-cli generate binary \
  -i data/temperatures.bin \
  -o results/temp_1d.bin \
  -s 1DxF \
  -d float
```

---

## `airtree-cli query`

All queries operate directly on a generated `.bin` histogram. They are extremely
fast — the trie is parsed in place.

```bash
airtree-cli query <subcommand> [options]
```

| Subcommand    | Description                                                                                              | Required Flags         |
| ------------- | -------------------------------------------------------------------------------------------------------- | ---------------------- |
| `topk`        | Top-K% (tail-by-mass quantile, **not** rank-by-count) — see [below](#top-k)                              | `-i`, `-o`, `-k`       |
| `min_count`   | Bins with the minimum count                                                                              | `-i`, `-o`             |
| `max_count`   | Bin with the maximum count — see [precision note](#max_count-on-fine-tries)                              | `-i`, `-o`             |
| `min_value`   | Smallest value present in the histogram                                                                  | `-i`, `-o`             |
| `max_value`   | Largest value present in the histogram                                                                   | `-i`, `-o`             |
| `percentile`  | Computes a percentile (0 – 100)                                                                          | `-i`, `-o`, `-p`       |

**Common flags**

- `-i, --input` — path to the generated `.bin` file
- `-o, --output` — path to write human-readable results

> Currently only the queries listed above are exposed via the CLI. **CDF** and
> **bounding-box** queries are available through the [C++ library](cpp-api.md#query-apis).

### Top-K

```bash
airtree-cli query topk \
  -i results/sales_2d.bin \
  -o results/topk.txt \
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
[`AirTreeExport`](#airtreeexport) output.

### `max_count` on fine tries

For fine-grained tries (`xP`), `max_count` is dominated by sampling noise
when N / num_bins is small: the returned bin's midpoint can sit 1–2σ from
the actual mode and contain only a handful of samples. For peak-finding,
prefer the coarser `xF` variant or post-process exported bin counts.

### Percentile

```bash
airtree-cli query percentile \
  -i results/temp_1d.bin \
  -o results/median.txt \
  -p 50.0
```

`-p` is a percentile expressed in percent (0 – 100).

### Min / Max Count

```bash
airtree-cli query min_count -i histogram.bin -o min_count.txt
airtree-cli query max_count -i histogram.bin -o max_count.txt
```

### Min / Max Value

```bash
airtree-cli query min_value -i histogram.bin -o min_value.txt
airtree-cli query max_value -i histogram.bin -o max_value.txt
```

---

## `AirTreeExport`

Convert an AirTree histogram buffer (`.bin`) into an analysis-ready tabular
format:

- **Apache Arrow** (default, `.arrow` IPC)
- **Parquet** (recommended for large histograms)
- **CSV** (human-readable, with metadata in `#` comment header)

```bash
AirTreeExport <input_histogram.bin> [OPTIONS]
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
AirTreeExport results/sales_2d.bin
# → results/sales_2d.arrow

# Parquet
AirTreeExport results/temp_1d.bin --parquet
# → results/temp_1d.parquet

# CSV with explicit filename
AirTreeExport results/histogram.bin --csv --output ./exports/my_histogram.csv

# Export to a directory (auto-named)
AirTreeExport data/4d_histogram.bin --parquet --output ./exports/
# → exports/4d_histogram.parquet
```

### Loading the Output in Python

```python
import pyarrow.parquet as pq
table = pq.read_table("analysis/histogram.parquet")
print(table)
```

---

## `airtree-merge-cli`

Combine two compatible AirTree histograms into a single histogram. The merge
is **exact** — no approximation — and produces a valid histogram that can be
queried, exported, or merged again.

```bash
airtree-merge-cli <input1.bin> <input2.bin> <output.bin>
```

The two inputs must have **identical configuration** (same schema, same
dimensionality, same trie variant). Mixing schemas raises a clear error.

**Example:**

```bash
# Two 2D histograms from different data batches
airtree-merge-cli batch1_2d.bin batch2_2d.bin merged_2d.bin

# Two 1D histograms from parallel runs
airtree-merge-cli run1_1d.bin run2_1d.bin final_1d.bin
```

**Workflow:**

1. Generate one histogram per shard / batch with the same schema:
   ```bash
   airtree-cli generate parquet -i shard1.parquet -o part1.bin -s 2DxP -c price quantity
   airtree-cli generate parquet -i shard2.parquet -o part2.bin -s 2DxP -c price quantity
   ```
2. Merge them:
   ```bash
   airtree-merge-cli part1.bin part2.bin combined.bin
   ```
3. Use the result like any other histogram — query it, export it, or merge it
   again with more data.

---

## See Also

- [Installation](install.md) — building and installing AirTree
- [C++ Library API](cpp-api.md) — programmatic generate / query / export / merge
- [Python Bindings](python.md) — coming soon
