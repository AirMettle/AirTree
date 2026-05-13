# Python Bindings

[← Back to README](../README.md)

> **Coming soon.** Python bindings for AirTree are in development and will
> be documented here once released. The API is expected to mirror the
> [C++ Library API](cpp-api.md) — generate, query, merge, and export — with
> NumPy / pandas-friendly inputs and outputs.

In the meantime:

- Use the [CLI tools](cli.md) directly from a Python `subprocess` call, then
  load the exported Arrow / Parquet output via `pyarrow` or `pandas`.
- Generate a histogram with `airtree generate ...`, export it with
  `airtree-export ... --parquet`, and read the resulting Parquet file:

  ```python
  import pyarrow.parquet as pq
  table = pq.read_table("histogram.parquet")
  df = table.to_pandas()
  ```

This page will be updated when the bindings ship. Track progress in the issue
tracker, or reach out to the AirMettle team if you want early access.
