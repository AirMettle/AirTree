# pyairtree

Python bindings for [AirTree](https://airmettle.com) — a high-performance library for building, compressing, and querying multi-dimensional histograms over floating-point data.

Turn large numeric datasets into compact `.airtree` histograms and run fast statistical queries without keeping the full raw data in memory.

**Required Notice:** Copyright AirMettle, Inc. (https://airmettle.com)

---

## Installation

```bash
pip install pyairtree
```

### Supported platforms

| Platform | Architecture | Python    |
|----------|--------------|------------|
| Linux    | x86_64, ARM  | 3.8 – 3.12 |
| Windows  | amd64        | 3.8 – 3.12 |


NumPy is installed automatically as a dependency.

### Verify the install

```python
import pyairtree
print(pyairtree.__version__)
```

---

## Quick start

Build a 1D histogram from a NumPy array, save it, and query percentiles:

```python
import numpy as np
import pyairtree

# Sample data
data = np.random.randn(100_000)

# Build a compressed histogram (1D, high precision)
buffer = pyairtree.generate(data)

# Save to disk
pyairtree.write(buffer, "histogram.airtree")

# Query percentiles
p = pyairtree.Percentile(buffer)
print(f"Median: {p.get_percentile(50):.4f}")
print(f"95th percentile: {p.get_percentile(95):.4f}")
```

#### Load data directly from .npy or single-array .npz files
```python
buffer = pyairtree.generate("samples/data.npy")
```

---

## Core workflow

A typical workflow has three steps:

1. Generate — compress numeric arrays into an in-memory histogram buffer
2. Store — write the buffer to a .airtree file
3. Query — run statistics or spatial queries on the buffer or file

```python
import pyairtree

# 1. Generate
buffer = pyairtree.generate(x, y, options)

# 2. Store
pyairtree.write(buffer, "output.airtree")

# 3. Query
reader = pyairtree.read("output.airtree")
p = pyairtree.Percentile(buffer)
```

---

## Generating histograms

### Unified generate() API (recommended)

Pass one NumPy array per dimension. Use AirTreeOptions to control dimensionality and precision:

```python
import numpy as np
import pyairtree

x = np.random.randn(50_000)
y = np.random.randn(50_000)

options = pyairtree.AirTreeOptions()
options.dimensions = 2
options.type = pyairtree.ConfigType.XP   # highest precision (default for most use cases)

buffer = pyairtree.generate(x, y, options)
```

#### Configuration types

| Type          | Precision                  | Best for                       |
|---------------|----------------------------|--------------------------------|
| ConfigType.XT | 13-bit                     | Large datasets, fastest builds |
| ConfigType.XF | 16-bit                     | General-purpose balance        |
| ConfigType.XP | 20-bit (1D) / 10-bit (2D+) | Highest accuracy               |

#### Dimensionality

```python
# 1D
buf_1d = pyairtree.generate(a, options)

# 2D
buf_2d = pyairtree.generate(a, b, options)

# 3D
buf_3d = pyairtree.generate(a, b, c, options)

# 4D
buf_4d = pyairtree.generate(a, b, c, d, options)
```

Set options.dimensions to match the number of arrays you pass.

### Named generation functions

Convenience functions are also available for explicit configs:

```python
arr = pyairtree.buildFPHArray(data)

# 1D
pyairtree.generate_1DxT(arr)   # 13-bit
pyairtree.generate_1DxF(arr)   # 16-bit
pyairtree.generate_1DxP(arr)   # 20-bit

# 2D / 3D / 4D
pyairtree.generate_2DxP(arr_x, arr_y)
pyairtree.generate_3DxP(arr_x, arr_y, arr_z)
pyairtree.generate_4DxP(arr_x, arr_y, arr_z, arr_w)
```

#### Supported array types

NumPy arrays with dtype float64, float32, int64, or int32.

---

## File I/O

### Write a histogram

```python
pyairtree.write(buffer, "histogram.airtree")
```

### Read a histogram

```python
reader = pyairtree.read("histogram.airtree")

print(reader.dims)        # number of dimensions
print(reader.bit_length)  # encoding precision
print(reader.type)        # configuration type

header = reader.header
print(header.version)
print(header.nan_count)
```

---

### Query operations

#### Percentile (1D)

```python
p = pyairtree.Percentile(buffer)

median = p.get_percentile(50.0)
p95    = p.get_percentile(95.0)
```

#### Min / Max (1D)

```python
mm = pyairtree.MinMax(buffer)
print(mm.get_min(), mm.get_max())
```

#### Top-K (1D)

Return the bins with the highest counts:

```python
topk = pyairtree.TopK(buffer)
results = topk.top_k(10.0)   # top 10% of bins by count

for r in results:
    print(r.lower_bound, r.upper_bound, r.count)
```

#### CDF (1D)

```python
cdf = pyairtree.CDF(buffer)
values = cdf.get_cdf()
```

#### Bounding box (2D)

Count values inside a rectangular region:

```python
bbox_query = pyairtree.BoundingBox(buffer_2d)

region = pyairtree.BoundingBoxCoordinate2D(
    min_x=0.0, max_x=1.0,
    min_y=0.0, max_y=1.0,
)

safe, edge = bbox_query.get_counts(region)
(safe_coords, safe_count) = safe
(edge_coords, edge_count) = edge
```

#### Grid query (1D)

Evaluate the histogram over a regular grid:

```python
gq = pyairtree.GridQuery(buffer)
result = gq.get_grid(spec)   # pass a GridSpec with min, max, steps
rows = result.materialize_rows()
```

---

#### Export

Export histogram data to common interchange formats:

```python
pyairtree.export_tree(buffer, "output.arrow", pyairtree.ExportFormat.ARROW)
pyairtree.export_tree(buffer, "output.parquet", pyairtree.ExportFormat.PARQUET)
pyairtree.export_tree(buffer, "output.csv", pyairtree.ExportFormat.CSV)
```

---

#### Choosing a configuration

| Use case                                  | Suggested config               |
|-------------------------------------------|--------------------------------|
| Exploratory analysis on large 1D datasets | ConfigType.XT or generate_1DxT |
| Production 1D analytics                   | ConfigType.XF or generate_1DxF |
| High-precision 1D requirements            | ConfigType.XP or generate_1DxP |
| 2D spatial / correlation data             | generate_2DxP                  |
| 3D / 4D multi-variate data                | generate_3DxP / generate_4DxP  |

---

#### Error handling

```python
import pyairtree
import numpy as np

try:
    pyairtree.generate(np.array(["not", "numeric"]))
except (TypeError, RuntimeError) as e:
    print(f"Generation failed: {e}")

try:
    pyairtree.Percentile(two_d_buffer)   # Percentile is 1D only
except RuntimeError as e:
    print(f"Query failed: {e}")
```

---

## License

AirMettle Noncommercial License

This software is provided for noncommercial use only. Government use requires a separate commercial license.

• Personal, research, educational, and qualifying nonprofit use is permitted
• Commercial or government use requires a license from AirMettle

For commercial licensing, support, or enterprise usage:

support@airmettle.com

Patent Pending — U.S. Patent Application Publication No. 2025/0217930 A1 (https://patents.google.com/patent/US20250217930A1)

See the bundled LICENSE file for full terms.

---

Links

- Homepage: https://airmettle.com
- Support: support@airmettle.com