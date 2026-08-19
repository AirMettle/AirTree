# AirTree Benchmark

**Benchmarking for Hierarchical Multi-Dimensional Histograms.**

`airtree_bench` is a C++ benchmarking utility built on [Google Benchmark](https://github.com/google/benchmark). It measures:

1. **Generate** — trie **create/insert** and **serialize** throughput across supported AirTree histogram schemas. Input data is loaded fully into memory before timing begins.
2. **Query** — existing `airtree-query` APIs on pre-built `.airtree` files (TopK, MinMax, Percentile, Grid, BoundingBox), with stable `query_id` counters for consolidated reporting.

Results are printed to the terminal by default. To write CSV output, pass Google Benchmark flags through the executable (for example, `--benchmark_out=results.csv --benchmark_out_format=csv`).

The end-to-end pipeline in `tools/bench/generate.sh` runs generate, writes selected histograms to disk, runs query benches against those files, consolidates generate and query CSVs, then writes PNG plots when the plot stack is installed.

---

## Supported Schemas

These are the schemas `airtree_bench` can benchmark. Input types reflect what this binary accepts — not the full `airtree` CLI (which also supports CSV).

| Schema | Dimensions | Variant | Input |
| ------ | ---------- | ------- | ----- |
| `1DxT` | 1D | Tiny | Binary, Parquet |
| `1DxF` | 1D | Fast | Binary, Parquet |
| `1DxP` | 1D | Precise | Binary, Parquet |
| `2DxF` / `2DxP` | 2D | Fast / Precise | Parquet |
| `3DxF` / `3DxP` | 3D | Fast / Precise | Parquet |
| `4DxF` / `4DxP` | 4D | Fast / Precise | Parquet |

- **Binary input** (`generate binary`) — 1D schemas only; requires `-d/--data-type` (`int32`, `int64`, `float`, `double`).
- **Parquet input** (`generate parquet`) — all schemas; requires `-c/--columns` with a space-separated list whose length matches the schema dimension.
- **Query input** (`query`) — a `.airtree` file plus a schema that has a registered query suite (see [Query suites by schema](#query-suites-by-schema)).

See [CLI Reference → Histogram Schemas](https://github.com/AirMettle/AirTree/blob/main/docs/cli.md#histogram-schemas) for guidance on choosing a variant (Tiny vs Fast vs Precise).

---

## What Gets Measured

### Generate

For each schema and detected data type, Google Benchmark runs:

| Benchmark | Description |
| --------- | ----------- |
| `CreateAndInsert` | Build the in-memory trie from loaded data |
| `Serialize` | Serialize the trie to a buffer |

Reported counters include throughput (`Points_Per_Second`, insertion speed) and trie size where applicable.

Optional **`--write-airtree <path>`** writes the serialized histogram once after the timed serialize work (untimed side effect). `generate.sh` uses this to produce `.airtree` inputs for the query phase.

### Query

Query fixtures load a pre-serialized `.airtree` buffer (untimed) and time individual `airtree-query` calls. Common counters:

| Counter | Description |
| ------- | ----------- |
| `query_id` | Stable integer identity for the query variant (see [Query ID table](#query-id-table)) |
| `result_size_B` | Size in bytes of the timed call’s return value |
| `BufferSize` | Size of the loaded `.airtree` buffer |
| `Param_k` / `Param_p` / `Param_steps` / `Dims` | Query-specific parameters where applicable |

#### Query suites by schema

| Schema | Allowed `--queries` values |
| ------ | -------------------------- |
| `1DxT`, `1DxF`, `1DxP` | `topk`, `minmax`, `percentile` |
| `2DxP`, `3DxP` | `grid`, `boundingbox` |
| `4DxP` | `grid` |

Other schemas (for example Fast multi-D) have generate benches only; they have no query suite in this build.

#### Query ID table

Each timed query variant records a fixed `query_id` (defined in `QueryFixtureBase.hpp`). Consolidated query CSVs key rows on `Data_set`, `Schema`, and `query_id`.

| `query_id` | Enum / fixture name | What is timed | Parameters / notes |
| ---------- | ------------------- | ------------- | ------------------ |
| `0` | `TopK_k1` | `TopK::getTopK(1)` | `Param_k=1` |
| `1` | `TopK_k5` | `TopK::getTopK(5)` | `Param_k=5` |
| `2` | `TopK_k15` | `TopK::getTopK(15)` | `Param_k=15` |
| `3` | `MinMax_getMin` | `MinMax::getMin()` | — |
| `4` | `MinMax_getMax` | `MinMax::getMax()` | — |
| `5` | `MinMax_getMinValue` | `MinMax::getMinValue()` | — |
| `6` | `MinMax_getMaxValue` | `MinMax::getMaxValue()` | — |
| `7` | `Percentile_p50` | `Percentile::getPercentile(50)` | `Param_p=50` |
| `8` | `Percentile_p90` | `Percentile::getPercentile(90)` | `Param_p=90` |
| `9` | `Grid_steps8` | `GridQuery::getGrid(...)` | 8 linear steps per axis over full extent; `Param_steps=8`, `Dims` = 2/3/4 |
| `10` | `BoundingBox_mid50` | `BoundingBox::getCounts(...)` | Fixed mid-50% box on a nominal `[0,100]` span per axis (`[25,75]`); `Dims` = 2/3 |

Google Benchmark names look like `AirTreeQuery1DxF_TopK/TopK_k5` or `AirTreeQuery2DxP_Grid/Grid_steps8`. The numeric `query_id` is the stable join key across schemas and runs.

---

## Running the Benchmark Executable

After building, the binary is at `<cmake-build-dir>/airtree-bench/airtree_bench`.

### Generate — Parquet (all schemas)

```bash
airtree_bench generate parquet \
  -i /path/to/input.parquet \
  -s 2DxP \
  -c fare_amount tip_amount
```

Optionally persist the histogram for later query benches:

```bash
airtree_bench generate parquet \
  -i /path/to/input.parquet \
  -s 1DxF \
  -c trip_distance \
  --write-airtree /path/to/out.airtree
```

### Generate — Binary (1D only)

```bash
airtree_bench generate binary \
  -i /path/to/input.bin \
  -s 1DxF \
  -d float
```

### Query — existing `.airtree`

```bash
airtree_bench query \
  -i /path/to/hist.airtree \
  -s 1DxF \
  -q topk minmax percentile
```

- `-s/--schema` must match a schema that has a query suite (see table above).
- `-q/--queries` is optional; if omitted, all queries allowed for that schema run.
- Google Benchmark flags (for example `--benchmark_out=... --benchmark_out_format=csv`) must appear **before** the `query` subcommand.

Extra arguments after the subcommand are forwarded to Google Benchmark where applicable (filters, repetitions, output format, etc.).

---

## Helper Scripts

Scripts live in `tools/bench/` and automate full benchmark runs against the shared bench datasets.

### `generate.sh`

End-to-end benchmark driver. From the repo root:

```bash
bash tools/bench/generate.sh
```

Output is written under:

```text
All Benchmark Outputs:
$CMAKE_BUILD_DIR/bench_data/output/<DD-MM-YYYY>/benchmark-<HH:MM>/

Plots Overview:
$CMAKE_BUILD_DIR/bench_data/output/<DD-MM-YYYY>/benchmark-<HH:MM>/plots/index.md
```

#### Pipeline stages

1. **Host setup** — runs `tools/setup/setup.sh` so a fresh machine gets the toolchain (gcc/g++, CMake, Ninja, Python 3.12 + venv) needed to build and plot.
2. **Build** — runs `tools/build/partial_build.sh` to build `airtree_bench`.
3. **Datasets** — `cd`s to `tools/bench`, then ensures `$CMAKE_BUILD_DIR/bench_data` exists; if not, calls `get_bench_datasets.sh` to fetch and prepare inputs. Conversion helpers (`csv_to_parquet.py`, `combine_taxi_datasets.py`) are invoked by absolute path, so they work from any CWD.
4. **Generate (binary)** — for each `*.bin` in `bench_data` and schemas `1DxT`, `1DxF`, `1DxP`, runs `generate binary` with `--data-type float` and writes `<dataset>_<schema>.csv`.
   - For `jane_street` + `1DxT`, also writes `airtree_files/jane_street_1DxT.airtree` via `--write-airtree`.
5. **Generate (parquet)** — for each `*.parquet` in `bench_data` and schemas `1DxF` … `4DxP`, runs `generate parquet` with dataset-specific correlated columns:
   | Dataset pattern | Columns (first *N* for *N*-D) |
   | --------------- | ----------------------------- |
   | `*MD*` (FRED-MD) | `RPI`, `W875RX1`, `RETAILx`, `INDPRO` |
   | `*QD*` (FRED-QD) | `GDPC1`, `DPIC96`, `PCECC96`, `OUTNFB` |
   | `yellow_tripdata*combined.parquet` | `trip_distance`, `fare_amount`, `total_amount`, `tip_amount` |
   - For yellow combined + schemas `1DxF`, `1DxP`, `2DxP`, `3DxP`, `4DxP`, also writes `airtree_files/yellow_tripdata_<schema>.airtree`.
6. **Query** — loads those `.airtree` files and runs query benches to `query_<dataset>_<schema>.csv`. A missing histogram or a failed query binary is a warning; that suite is skipped and the pipeline continues.
   | Histogram | Queries |
   | --------- | ------- |
   | `yellow_tripdata_1DxF`, `yellow_tripdata_1DxP` | `topk` `minmax` `percentile` |
   | `yellow_tripdata_2DxP`, `yellow_tripdata_3DxP` | `grid` `boundingbox` |
   | `yellow_tripdata_4DxP` | `grid` |
   | `jane_street_1DxT` | `topk` `minmax` `percentile` |
7. **Consolidate** — post-processes CSVs in the same output directory:
   - `consolidate_bench_data.sh` → `consolidated_<schema>_data.csv` (generate metrics)
   - `consolidate_query_bench_data.sh` → `consolidated_query_<schema>_data.csv` and `consolidated_query_all_data.csv`
   - `consolidate_systeminfo.sh` → `systeminfo.csv`
8. **Plots** — uses `$CMAKE_BUILD_DIR/venv/bin/python`. If that venv is missing, `generate.sh` creates it via `tools/setup/venv.sh` (Python 3.12 comes from stage 1). If matplotlib / seaborn / pandas cannot be imported, it `pip install`s `tools/bench/requirements-plot.txt` into the venv, then runs `tools/bench/plot_benchmarks.py "$OUTPUT_DIR" --format png,svg` and writes `<run_dir>/plots/` (PNG, SVG, and `index.md`). A failed venv create or pip install: warn and continue (CSVs still count). A plotter crash after those imports succeed fails the run.

#### Consolidated query CSVs

`consolidate_query_bench_data.sh` rewrites each `query_*.csv` so rows are keyed by dataset, schema, and `query_id` (the Google Benchmark `name` column is dropped). Output files:

| File | Contents |
| ---- | -------- |
| `consolidated_query_<schema>_data.csv` | All query rows for one schema |
| `consolidated_query_all_data.csv` | Union of all query schemas |

Header shape (counters may vary by suite):

```text
Data_set,Schema,query_id,iterations,real_time,cpu_time,...,BufferSize,result_size_B,...
```

Use the [Query ID table](#query-id-table) to map `query_id` back to the timed operation.

### Plots

`generate.sh` writes PNG and SVG figures plus `plots/index.md` into `<run_dir>/plots/` after consolidation. Plotting is a Python post-step on those CSVs (same family as the consolidators), not part of `airtree_bench`. On a fresh machine it creates `$CMAKE_BUILD_DIR/venv` if needed and installs `tools/bench/requirements-plot.txt` there. A failed install is a warning, not a failed bench.

To plot an existing run by hand, from the repo root:

```bash
python tools/bench/plot_benchmarks.py <run_dir>
```

Prefer the project venv when plot deps live there:

```bash
$CMAKE_BUILD_DIR/venv/bin/python tools/bench/plot_benchmarks.py <run_dir>
```

Optional flags: `--out <dir>` (default `<run_dir>/plots`) and `--format png,svg` (comma-separated; default `png`). Open `plots/index.md` for the full catalog in one scroll.

**Input files** in `<run_dir>`:

- `consolidated_<schema>_data.csv` — generate (`CreateAndInsert` / `Serialize`)
- `consolidated_query_<schema>_data.csv` — query (per-schema files; not `consolidated_query_all_data.csv`)
- `systeminfo.csv` — optional figure footer (`Ran on …`)

**Output:** `<run_dir>/plots/` (or `--out`).

Figures (one line each):

- `generate_insert_points_per_sec.png` — insert throughput (`Points_Per_Second`) by dataset × schema
- `generate_insert_mbps.png` — same layout, y = `Insertion Speed (MB/s)`
- `generate_create_vs_serialize.png` — CreateAndInsert vs Serialize `real_time` (ms)
- `generate_trie_vs_input.png` — dataset size vs trie size
- `generate_1d_variant_tradeoff.png` — 1DxT / 1DxF / 1DxP insert rate vs trie size
- `generate_dim_scaling.png` — Fast vs Precise across 1D–4D (FRED + yellow combined)
- `generate_cardinality.png` — Distinct Values vs Precise Bins
- `generate_input_profile.png` — 1D input facts (dataset size, value count, distinct values)
- `generate_trie_size.png` — 1DxT / 1DxF / 1DxP trie size
- `generate_precise_bins.png` — 1DxT / 1DxF / 1DxP precise bins
- `generate_avg_bytes_per_bin.png` — 1DxT / 1DxF / 1DxP average bytes per bin
- `query_latency_overview.png` — query latency by [query_id](#query-id-table) label × schema
- `query_1d_families.png` — TopK, MinMax, Percentile for 1DxT / 1DxF / 1DxP
- `query_multid.png` — Grid and BoundingBox for 2DxP / 3DxP / 4DxP
- `query_latency_vs_buffersize.png` — latency vs `.airtree` buffer size
- `query_heatmap.png` — schema × query label latency heatmap

Use the [Query ID table](#query-id-table) to map `query_id` / labels. Empty suites are skipped (logged), not drawn as zeros.

`generate.sh` installs those deps on demand. To install by hand (or to plot an old run on a venv that never plotted):

```bash
$CMAKE_BUILD_DIR/venv/bin/python -m pip install -r tools/bench/requirements-plot.txt
```

### `get_bench_datasets.sh`

Downloads and prepares benchmark inputs into a destination directory:

```bash
bash tools/bench/get_bench_datasets.sh <destination_directory>
```

Sources include FCBench binary datasets (via Google Drive / `gdown`), NYC TLC yellow trip data (combined into `yellow_tripdata_2025_combined.parquet`), and FRED-MD / FRED-QD (CSV converted to Parquet). Helper Python scripts in the same directory (`csv_to_parquet.py`, `combine_taxi_datasets.py`) perform format conversion.

### Other consolidators

| Script | Role |
| ------ | ---- |
| `consolidate_bench_data.sh` | Merge per-dataset generate CSVs into `consolidated_<schema>_data.csv` |
| `consolidate_query_bench_data.sh` | Merge `query_*.csv` into per-schema and all-query consolidated files |
| `consolidate_systeminfo.sh` | Extract system / environment info into `systeminfo.csv` |

---

## Project Layout

| Path | Purpose |
| ---- | ------- |
| `airtree-bench/src/Main.cpp` | CLI entry point (`generate` / `query`) and Google Benchmark integration |
| `airtree-bench/src/generate/` | Per-schema generate (create/insert/serialize) fixtures |
| `airtree-bench/src/query/topk/` | TopK query fixtures (`1DxT` / `1DxF` / `1DxP`) |
| `airtree-bench/src/query/minmax/` | MinMax query fixtures |
| `airtree-bench/src/query/percentile/` | Percentile query fixtures |
| `airtree-bench/src/query/grid/` | Grid query fixtures (`2DxP` / `3DxP` / `4DxP`) |
| `airtree-bench/src/query/boundingbox/` | Bounding-box query fixtures (`2DxP` / `3DxP`) |
| `airtree-bench/include/airtree/bench/query/QueryFixtureBase.hpp` | Shared query fixture base, `QueryId` enum, counters |
| `airtree-bench/include/airtree/bench/BenchPaths.hpp` | Shared write path and preloaded query buffer |
| `tools/bench/generate.sh` | Full generate + query + consolidate + plot pipeline |
| `tools/bench/plot_benchmarks.py` | Plot catalog from consolidated CSVs |
| `tools/bench/bench_plot/` | Loader, style, and plot functions |
| `tools/bench/requirements-plot.txt` | Pinned pandas / matplotlib / seaborn |
| `tools/bench/get_bench_datasets.sh` | Dataset download / prep |
| `tools/bench/consolidate_bench_data.sh` | Generate CSV consolidation |
| `tools/bench/consolidate_query_bench_data.sh` | Query CSV consolidation |
| `tools/bench/consolidate_systeminfo.sh` | System info consolidation |
