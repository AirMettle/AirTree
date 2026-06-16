# AirTree Bench

**Benchmarking for Hierarchical Multi-Dimensional Histograms.**

`airtree_bench` is a C++ benchmarking utility built on [Google Benchmark](https://github.com/google/benchmark). It measures trie **create/insert** and **serialize** throughput across all supported AirTree histogram schemas. Input data is loaded fully into memory before timing begins.

Results are printed to the terminal by default. To write CSV output, pass Google Benchmark flags through the executable (for example, `--benchmark_out=results.csv --benchmark_out_format=csv`).

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

See [CLI Reference → Histogram Schemas](https://github.com/AirMettle/AirTree/blob/main/docs/cli.md#histogram-schemas) for guidance on choosing a variant (Tiny vs Fast vs Precise).

---

## What Gets Measured

For each schema and detected data type, Google Benchmark runs:

| Benchmark | Description |
| --------- | ----------- |
| `CreateAndInsert` | Build the in-memory trie from loaded data |
| `Serialize` | Serialize the trie to a buffer |

Reported counters include throughput (`Points_Per_Second`, `Speed` on 1D schemas) and trie size where applicable.

---

## Running the Benchmark Executable

After building, the binary is at `<cmake-build-dir>/airtree-bench/airtree_bench`.

### Parquet (all schemas)

```bash
airtree_bench generate parquet \
  -i /path/to/input.parquet \
  -s 2DxP \
  -c fare_amount tip_amount
```

### Binary (1D only)

```bash
airtree_bench generate binary \
  -i /path/to/input.bin \
  -s 1DxF \
  -d double
```

Extra arguments are forwarded to Google Benchmark (filters, repetitions, output format, etc.).

---

## Helper Scripts

Scripts live in `tools/bench/` and automate a full benchmark run against the NYC Taxi dataset.

### `generate.sh`

End-to-end benchmark driver. From the repo root:

```bash
bash tools/bench/generate.sh
```

The script:

1. Runs `tools/build/partial_build.sh` to build `airtree_bench`.
2. Ensures `$CMAKE_BUILD_DIR/bench_data` exists; if not, calls `data.sh` to fetch the dataset.
3. Loops over all nine schemas: `1DxT`, `1DxF`, `1DxP`, `2DxF`, `2DxP`, `3DxF`, `3DxP`, `4DxF`, `4DxP`.
4. For each schema, selects the first *N* columns by dimension from:
   `trip_distance`, `fare_amount`, `tip_amount`, `congestion_surcharge`.
5. Invokes `airtree_bench generate parquet` against `yellow_tripdata_2024-01.parquet`.
6. Prints all metrics to the terminal.

### `data.sh`

Downloads benchmark data into a destination directory:

```bash
bash tools/bench/data.sh <destination_directory>
```

Fetches `yellow_tripdata_2024-01.parquet` (NYC Taxi Data, January 2024) from the TLC public dataset.

---

## Project Layout

| Path | Purpose |
| ---- | ------- |
| `airtree-bench/src/Main.cpp` | CLI entry point and Google Benchmark integration |
| `airtree-bench/src/generate/` | Per-schema benchmark fixtures |
| `airtree-bench/include/airtree/bench/` | Shared benchmark headers and configs |
| `tools/bench/generate.sh` | Automated full-schema benchmark run |
| `tools/bench/data.sh` | Dataset download helper |