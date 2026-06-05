# Changelog

All notable changes to AirTree are listed here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project
follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-06-05

### Added 
- Add README.md for benchmark
- Add more example files
- Add Community helper documentations 

## [1.3.0] - 2026-05-26

### Added
- `airtree query grid` — multi-dimensional grid (re-binned histogram /
  heatmap / cube) query over `2DxP` / `3DxP` / `4DxP` buffers, with a
  matching C++ `GridQuery` API and a `--max-cells` guard against runaway
  outputs.
- `airtree generate csv` — CSV input path for the generator (1D – 4D),
  using Arrow's CSV reader. Comma-separated only.
- Runnable sample (`examples/sales.csv`, 60 rows) plus an end-to-end
  walkthrough in the README that exercises generate → query → export.
- License (PolyForm Noncommercial with a government-institution carve-out)
  and Patent Pending notice (US 20250217930 A1).
- First-time-user documentation pass: install
  verification, uninstall, and troubleshooting sections in `docs/install.md`.

### Changed
- Significant internal refactor across CLI and libraries (no public API
  changes).
- `docs/cpp-api.md` corrected: `parse_file` return type, CDF constructor
  semantics, multi-dim schema coverage per query.


### Fixed
- License file renamed from `LICENSE.md` to plain `LICENSE` (no extension)
  for broader tool compatibility; content de-markdownified.
- CPack `CPACK_RESOURCE_FILE_LICENSE` now resolves to the actual `LICENSE`
  file and is set *before* `include(CPack)`, so .deb / .rpm / .tgz packages
  bundle the license.
- Bounding-box / weighted-average-error fixes in `airtree-query`.

## [1.2.0] - 2026-05-13

- First-pass user-facing documentation under `docs/` (CLI, C++ API, install,
  Python placeholder).
- Windows build support for `airtree-export`.

## [1.1.0] - 2026-05-11

- Config registry for AirTree header dispatch across trie variants.
- CLI / query fixes: output path handling, sign-aware histogram bin
  boundaries, `BoundingBox` safe ≤ total invariant.

## [1.0.0] - 2026-05-11

Initial release.

- Hierarchical multi-dimensional histograms over 1D / 2D / 3D / 4D numeric
  data.
- Trie variants: `xT` (Tiny, 1D-only), `xF` (Fast), `xP` (Precise).
- CLI tools (`airtree`, `airtree-export`, `airtree-merge`) and static C++
  libraries (`airtree-core`, `airtree-query`, `airtree-util`,
  `airtree-reader`, `airtree-merge`, `airtree-export`).
- Generate from Parquet or raw binary; query (percentile, top-k, min/max,
  CDF, bounding-box); export to Arrow / Parquet / CSV; exact merge.
