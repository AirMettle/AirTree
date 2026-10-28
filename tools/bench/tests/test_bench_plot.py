"""Loader and CLI smoke tests for bench_plot (Phase 12)."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pandas as pd
import pytest

BENCH_DIR = Path(__file__).resolve().parents[1]
FIXTURE_DIR = BENCH_DIR / "testdata" / "plot_fixture"
if str(BENCH_DIR) not in sys.path:
    sys.path.insert(0, str(BENCH_DIR))

from bench_plot.index import write_index
from bench_plot.load import LoadError, load_generate, load_query
from bench_plot.style import save_fig

CATALOG_PNGS = (
    "generate_insert_points_per_sec.png",
    "generate_insert_mbps.png",
    "generate_create_vs_serialize.png",
    "generate_trie_vs_input.png",
    "generate_1d_variant_tradeoff.png",
    "generate_dim_scaling.png",
    "generate_cardinality.png",
    "generate_input_profile.png",
    "generate_trie_size.png",
    "generate_precise_bins.png",
    "generate_avg_bytes_per_bin.png",
    "query_latency_overview.png",
    "query_1d_families.png",
    "query_multid.png",
    "query_latency_vs_buffersize.png",
    "query_heatmap.png",
)
# Fixture generate CSVs are 1D-only; dim scaling skips cleanly.
FIXTURE_REQUIRED_PNGS = tuple(
    name for name in CATALOG_PNGS if name != "generate_dim_scaling.png"
)

RAW_GB_CSV = """2026-07-29T12:31:14-06:00
Running /path/to/airtree_bench
Run on (16 X 2300 MHz CPU s)
name,iterations,real_time,cpu_time,time_unit
"AirTreeBench1DxF/CreateAndInsert_float",1,14494.7,14488,ns
"""


def test_generate_schema_fixture_and_ns_to_ms() -> None:
    df = load_generate(FIXTURE_DIR)
    assert set(df["Schema"].unique()) == {"1DxF", "1DxP"}
    assert set(df["fixture"].unique()) == {"CreateAndInsert", "Serialize"}

    fred = df[
        (df["Schema"] == "1DxF")
        & (df["Data_set"] == "2025-08-FRED-MD")
        & (df["fixture"] == "CreateAndInsert")
    ]
    assert len(fred) == 1
    got = float(fred.iloc[0]["real_time_ms"])
    expected = 14494.7 * 1e-6
    assert got == pytest.approx(expected)
    assert expected == pytest.approx(0.0144947)

    serialize = df[df["fixture"] == "Serialize"]
    assert not serialize.empty
    assert serialize["Points_Per_Second"].isna().all()


def test_query_id_zero_label_and_result_size() -> None:
    df = load_query(FIXTURE_DIR)
    topk = df[df["query_id"] == 0]
    assert not topk.empty
    assert topk.iloc[0]["query_label"] == "TopK k=1"
    assert "result_size_B" in df.columns
    assert "result_size" not in df.columns
    assert int(topk.iloc[0]["result_size_B"]) == 33936


def test_query_id_derived_from_legacy_name(tmp_path: Path) -> None:
    csv = tmp_path / "consolidated_query_1DxF_data.csv"
    csv.write_text(
        "Data_set,Schema,name,iterations,real_time,cpu_time,time_unit,"
        "BufferSize,result_size\n"
        "yellow_tripdata,1DxF,AirTreeQuery1DxF_TopK/TopK_k1,1,100,100,ns,6680,8\n"
        "yellow_tripdata,1DxF,AirTreeQuery1DxF_Percentile/Percentile_p99,1,200,200,ns,6680,8\n"
    )
    df = load_query(tmp_path)
    topk = df[df["name"].str.endswith("TopK_k1")]
    assert int(topk.iloc[0]["query_id"]) == 0
    assert topk.iloc[0]["query_label"] == "TopK k=1"
    p99 = df[df["name"].str.endswith("Percentile_p99")]
    assert pd.isna(p99.iloc[0]["query_id"])
    assert p99.iloc[0]["query_label"] == "Percentile_p99"


def test_raw_google_benchmark_csv_rejected(tmp_path: Path) -> None:
    raw = tmp_path / "consolidated_1DxF_data.csv"
    raw.write_text(RAW_GB_CSV)
    with pytest.raises(LoadError, match="not a consolidated generate CSV"):
        load_generate(tmp_path)


def test_cli_smoke_writes_expected_pngs(tmp_path: Path) -> None:
    out_dir = tmp_path / "plots"
    result = subprocess.run(
        [
            sys.executable,
            str(BENCH_DIR / "plot_benchmarks.py"),
            str(FIXTURE_DIR),
            "--out",
            str(out_dir),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    assert result.returncode == 0, result.stderr
    written = {path.name for path in out_dir.glob("*.png")}
    missing = set(FIXTURE_REQUIRED_PNGS) - written
    assert not missing, f"missing PNGs: {sorted(missing)}"
    extra = written - set(CATALOG_PNGS)
    assert not extra, f"unexpected PNGs: {sorted(extra)}"
    for name in FIXTURE_REQUIRED_PNGS:
        path = out_dir / name
        assert path.stat().st_size > 0, name
    assert "generate_dim_scaling.png" not in written
    index = (out_dir / "index.md").read_text()
    assert "generate_insert_points_per_sec.png" in index
    assert "generate_dim_scaling" not in index
    assert "![" in index


def test_write_index_embeds_written_figures_only(tmp_path: Path) -> None:
    png = tmp_path / "generate_insert_points_per_sec.png"
    svg = tmp_path / "generate_insert_points_per_sec.svg"
    png.write_bytes(b"png")
    svg.write_bytes(b"<svg/>")
    index = write_index(
        tmp_path,
        [png, svg],
        footer="benchmark-12:08 · Ran on test",
    )
    text = index.read_text()
    assert "benchmark-12:08" in text
    assert "generate_insert_points_per_sec.png" in text
    assert "[svg](generate_insert_points_per_sec.svg)" in text
    assert "generate_dim_scaling" not in text
    assert "query_heatmap" not in text


def test_save_fig_png_and_svg(tmp_path: Path) -> None:
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots()
    ax.plot([0, 1], [0, 1])
    written = save_fig(fig, tmp_path / "demo.png", formats=("png", "svg"))
    names = {path.name for path in written}
    assert names == {"demo.png", "demo.svg"}
    assert all(path.stat().st_size > 0 for path in written)
