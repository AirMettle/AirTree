# AirTree Benchmark 

**Benchmarking for Hierarchical Multi-Dimensional Histograms.**

AirTree-bench is a high-performance C++ benchmarking utility that measures the throughput and utilization of AirTree trie structures on datasets. By loading data entirely into memory first, it calculates and saves precise performance metrics into a CSV file. 

---

## Supported Schemas

| Schema | Dimensions | Variant | Input            |
| ------ | ---------- | ------- | ---------------- |
| `1DxT` | 1D         | Tiny    | Binary           |
| `1DxF` | 1D         | Fast    | Binary, Parquet  |
| `1DxP` | 1D         | Precise | Binary, Parquet  |
| `2DxF` / `2DxP` | 2D | Fast / Precise | Parquet  |
| `3DxF` / `3DxP` | 3D | Fast / Precise | Parquet  |
| `4DxF` / `4DxP` | 4D | Fast / Precise | Parquet  |

See [CLI Reference → Histogram Schemas](docs/cli.md#histogram-schemas) for
guidance on choosing a variant.

## Run 1D Benchmarks (Binary Input)

```bash
# Create the output directory
mkdir -p benchmark_results

# Run a precise 1D benchmark and save CSV output to benchmark_results folder 
# Ensure that benchmark_out flags are placed immediately after the executable to align with format requirements 
./cmake-build-gnu-11-RelWithDebInfo-nosan/airtree-bench/airtree_bench \
 --benchmark_out=benchmark_results/wesad_chest_f64_1DxP.csv \
 --benchmark_out_format=csv \
 generate binary \
 -i ./FCBench/wesad_chest_f64 \
 -s 1DxP \
 -d double 
