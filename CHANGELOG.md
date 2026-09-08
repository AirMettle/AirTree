# Changelog

All notable changes to AirTree are listed here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project
follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.8.0] - 2026-09-08

### Added
- Per-value insert for the 2D schemas: `encode_2DxP` / `encode_2DxF`,
  `createAndInsert_2DxP` / `createAndInsert_2DxF`, with the node creation and
  `insertintoTLETrie_*` functions now declared in the schema headers. The batch
  generators call the same per-value path.

### Changed
- Inserts increment only the leaf counter and set a populated bit on a
  slot's first visit; parent counts are rolled up once after the loop
  (`rollUpCounts`, with `rollUpNode` in `NodeOps.hpp`). On the taxi data the
  1D inserts are 13 to 17 percent faster, 3DxF and 4DxP 10, 2DxF, 2DxP and
  4DxF 5 to 7, 3DxP 3. Output bytes unchanged.
- `BinBoundary` walks each node's populated bins by set bit instead of
  testing every slot; generation is 20 to 50 percent faster across the
  configurations.
- 1DxT and 1DxF allocate all 256 level-1 nodes up front, so the per-value
  insert is two increments with no populated test or allocation branch and
  the loops drop the size-threshold check. The in-memory trie is a fixed
  35 KB for 1DxT and 265 KB for 1DxF; the bench's Trie Size counter reports
  those figures. Both inserts are about a third faster on the taxi data.
  1DxP preallocates level 1 the same way and allocates level-2 nodes on first
  use; its size is computed once after the loop.
- The encoders, the special-value checks and the 1D per-value inserts are
  inline in their headers; `InternalEncoding.cpp` and `SpecialCounts.cpp` are
  gone. The insert loops no longer make three out-of-line calls per value;
  1D insert is 10 to 13 percent faster, 2D to 4D 4 to 19 percent.
- The `createInternal*Bit` encoders adjust the exponent with a sign fold
  instead of a branch, and the 8-bit ones clamp and shift with conditional
  moves. Same codes for every input, checked exhaustively for the 32-bit
  variants; the xF inserts are 13 to 26 percent faster on the taxi data.
- `Percentile`, `CDF`, `MinMax` and `TopK` read their populated bins straight
  from the buffer (`meta::populatedBins(span, header, histogram)`) instead of
  deserializing the trie into nodes and walking it; the objects hold the bins
  and the header only. Same answers; construction is about 3x faster on a
  small buffer and the object no longer keeps the trie in memory.
- Buffer readers (`deserializeHeader`, `AirTreeReader::read`, the `deserialize_*`
  / `processBuffer_*` functions, the mask and count decoders) and the query
  constructors (`Percentile`, `CDF`, `MinMax`, `TopK`, `GridQuery`,
  `BoundingBox`, `BinBoundary`) take `std::span<const char>` instead of
  `std::vector<char>`. A `std::vector<char>` converts implicitly, so callers
  are unchanged; the buffer is no longer copied to construct a query object
  (only `BinBoundary` keeps its own copy, because it walks the bytes later).
- Serializers write each node straight into the output buffer (mask, width,
  packed counts) instead of building it from temporaries; the mask-driven
  count packer sizes its output once. Same bytes as before for every
  configuration; a small window serializes several times faster.

### Removed
- The `default_mode` flag. It was always true, nothing could set it to false
  and the readers never consulted it, so the encoders now always apply the
  default exponent adjustment. `AirTreeOptions::default_mode` and the trailing
  `bool` parameter on `generate_*`, `execCreateAndInsert_*`, `execSerializ*`,
  `createInternal*Bit`, `internal_8bit` / `internal_10bit`, `encode_2Dx*` and
  the per-value insert helpers are gone.

### Fixed
- `airtree_bench` forwards Google Benchmark flags given after the subcommand;
  they were dropped before. The generate fixtures free the previous trie
  outside the timed region, and `generate.sh` runs `BENCH_REPS` repetitions
  (default 5).
- `BinBoundary` no longer prints the buffer's dimension count and
  configuration to stdout when constructed.

## [1.7.0] - 2026-08-25

### Changed
- Generated buffers carry the observation count in the header's `trie_count`
  field (previously always 0); merges sum it. Files written by earlier
  versions still read; their field stays 0.
- `Percentile`, `CDF`, `MinMax` and `TopK` extract their populated bins at
  construction and no longer keep a copy of the input buffer, so a query
  object is immutable after construction and safe to share between threads.
- Per-operation log lines in generate and merge are `DEBUG` (were `INFO`);
  `airtree::util::logging::setLevel` adjusts every AirTree logger at run time.

## [1.6.3] - 2026-08-25

Query-path performance release. Nothing about the on-disk format changed:
files written by earlier versions read, merge and query identically, and
merges remain byte-identical to a histogram generated from all the raw
values.

### Added
- `airtree::merge::mergeAirTrees(buffers)` — merges any number of buffers
  of one configuration in a single streaming pass: inputs are walked side
  by side in depth-first order, nodes populated by more than one input are
  decoded, summed and re-encoded, subtrees present in only one input are
  copied verbatim. Produces exactly the bytes a pairwise fold would. 1D
  schemas stream; other schemas fold pairwise for now. Documented in
  `docs/cpp-api.md`.
- `Percentile::getPercentileWithBounds(p)` returning `PercentileResult`
  `{value, lower_bound, upper_bound}` — the quantile together with the
  native bin it fell in (`[lower, upper)` for non-negative bins,
  `(lower, upper]` for negative ones; `lower == upper == value` for
  `-inf`, `±0`, `+inf`). Same walk as `getPercentile`, which now delegates
  to it.
- `meta::Histogram::sortedValues(bitLength)` and `positionOf(code)` —
  access to the shared per-bit-length bin table.
- Serialization helpers on the public headers, used by the reader and the
  merge: `readPopulatedMask`, `writePopulatedMask`, `forEachSetBit`,
  `setPopulated` (`serdes/BooleanArray.hpp`); mask-driven
  `deserializeCounts` / `serializeCounts` and `skipCounts`
  (`serdes/Count.hpp`). The vector-returning forms remain.
- `airtree-bench`: benchmark fixtures with stable `query_id`s for every
  query — including construction-only and construction-plus-query
  ("cold") variants, `CDF`, `BinBoundary`, an `AirTreeReader::read`
  fixture, and a `merge` subcommand (`Merge_pair`, `Merge_fold`,
  `Merge_nway`); `result_size_B` counter; README query-id table.
- `tools/bench`: one-command benchmark pipeline (`generate.sh`) that
  fetches the public datasets (`get_bench_datasets.sh`), converts them,
  runs generate and query benchmarks for every schema and consolidates the
  CSV and system-information outputs.

### Changed
- `reConstruct` (bin code → floating-point value) is bit arithmetic on the
  IEEE layout instead of building and parsing a bit string. Bit-exact with
  the previous implementation for all codes of every supported bit length,
  double and float (`TestReconstructEquivalence`).
- `meta::Histogram` builds its sorted bin table once per process per bit
  length and shares it (immutable, mutex-guarded cache) instead of
  rebuilding it in every `Percentile` / `CDF` / `MinMax` / `TopK` /
  `BinBoundary` object. Constructing a 1D query object on a 4 KB buffer:
  261 ms → 18 µs.
- `Percentile`, `CDF`, `MinMax` and `TopK` walk only the populated bins,
  extracted from the trie once per object and sorted by table position,
  instead of the full 2^bits table (`PopulatedBins.hpp`). Warm p99 on a
  60 s window: 2.6 ms → 2 µs; `TopK` / `MinMax` similar. Verified against
  the previous full-table walks bit for bit (`TestSparseWalk`).
- Deserialization decodes each node's populated mask and counts directly
  into the node — no temporary vectors, no per-slot `BooleanArray::get`,
  whole-word refill bounded by the node's payload. All 1D/2D/3D/4D readers
  and `BinBoundary` use it. Reading a 4 KB 1DxP buffer: 68 → 18 µs; a
  30 KB buffer: 363 → 101 µs; an 11 KB 2DxP buffer: 84 → 11 µs
  (`TestCountCodec` pins the new decoder to the old one).
- `mergeAirTree(a, b)` validates inputs from their headers instead of fully
  deserializing both, and for 1D schemas runs the streaming merge:
  22 ms → 25 µs per pair; a 15-file fold 365 ms → 0.7 ms (0.15 ms with
  `mergeAirTrees`).
- Per-node `INFO` log lines inside the merge loops are now `TRACE`; a
  merge no longer writes hundreds of megabytes of log at the default level.
- `meta::Histogram` rejects bit lengths other than 12, 13, 16 and 20 with
  `std::invalid_argument` instead of building a meaningless table.

### Fixed
- 2D and 4D deserializer entry points took the buffer by value, copying it
  on every read.
- `airtree-bench` int32 / int64 generate fixtures instantiated the float
  templates and always skipped.
- MSVC `/W4 /WX`: no GCC-only builtins or shadowed locals in the new code.

### CI
- Ubuntu setup scripts install the deadsnakes PPA from a signing key
  committed in `tools/setup/keys/` (fingerprint
  `F23C5A6CF475977595C89F51BA6932366A755776`) with a `signed-by` sources
  entry, instead of `add-apt-repository`, so setup no longer depends on
  Launchpad's key service.

## [1.6.2] - 2026-07-09

### Fixed
- Windows: the OpenSSL dependency's `nmake` steps run with the MSVC
  toolset's `bin` directory on `PATH`; the dependency-cache configuration
  id no longer derives a "distro major" from `CMAKE_SYSTEM_VERSION` on
  Windows (empty under the toolchain file, which split the cache between
  build legs); nlohmann_json is built without its test suite.

## [1.6.1] - 2026-07-09

### Changed
- Python documentation updated for the `pyairtree` release on PyPI.

### Fixed
- Explicit `Threads::Threads` (pthreads) linkage for `airtree-export` and
  its tests, needed by the manylinux wheel build.
- macOS: curl's optional features (nghttp2, LDAP, libssh2, GSSAPI) are
  forced off so `libcurl.a` is self-contained and reproducible regardless
  of what is installed on the build host.

## [1.6.0] - 2026-07-02

### Fixed
- Count serialization overflow: bit-packed count streams now accumulate
  through 64-bit buffers (previously 32-bit), so bins with large counts
  no longer corrupt on serialize/deserialize. `minimumBits` and the
  max-count scan are now unsigned to match the 32-bit count range.

### CI
- CentOS container installs `zstd` alongside git/tar/gzip, so the
  dependency cache resolves to the right compressor and restores at the
  start of the job.

## [1.5.0] - 2026-06-12

- Fixed version handling for debian/rpm packages 
- add support for windows-22 via msvc compiler
- add support for macOS-26 via clang compiler 
- Fixed Warning as Errors on both windows-22 and macOS-26, making the codebase more robust
- Added example CMakeLists.txt in example/ for reference 
- enable ci/cd for ubuntu22.04 , ubuntu22.04-arm , centos-stream9 , windows-22 , macOS-26


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
