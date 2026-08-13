#!/usr/bin/env python3
"""Write catalog plots from a consolidated AirTree benchmark run directory."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from bench_plot.load import load_generate, load_query, load_systeminfo
from bench_plot.plots import (
    generate_1d_variant_tradeoff,
    generate_cardinality,
    generate_create_vs_serialize,
    generate_dim_scaling,
    generate_insert_mbps,
    generate_insert_points_per_sec,
    generate_trie_vs_input,
    query_1d_families,
    query_heatmap,
    query_latency_overview,
    query_latency_vs_buffersize,
    query_multid,
)


def _parse_formats(raw: str) -> tuple[str, ...]:
    formats = tuple(part.strip() for part in raw.split(",") if part.strip())
    if not formats:
        raise argparse.ArgumentTypeError("at least one format is required")
    return formats


def _footer(run_dir: Path) -> str:
    info = load_systeminfo(run_dir)
    if info:
        return f"{run_dir.name} · {info}"
    return run_dir.name


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "run_dir",
        type=Path,
        help="Directory of consolidated_*.csv files (a generate.sh output dir)",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Output directory (default: <run_dir>/plots)",
    )
    parser.add_argument(
        "--format",
        dest="formats",
        type=_parse_formats,
        default=("png",),
        help="Comma-separated image formats (default: png)",
    )
    args = parser.parse_args(argv)

    run_dir = args.run_dir
    out_dir = args.out if args.out is not None else run_dir / "plots"
    generate_df = load_generate(run_dir)
    query_df = load_query(run_dir)
    footer = _footer(run_dir)

    written: list[Path] = []
    for plot in (
        generate_insert_points_per_sec,
        generate_insert_mbps,
        generate_create_vs_serialize,
        generate_trie_vs_input,
        generate_1d_variant_tradeoff,
        generate_dim_scaling,
        generate_cardinality,
    ):
        written.extend(
            plot(generate_df, out_dir, formats=args.formats, footer=footer)
        )
    for plot in (
        query_latency_overview,
        query_1d_families,
        query_multid,
        query_latency_vs_buffersize,
        query_heatmap,
    ):
        written.extend(
            plot(query_df, out_dir, formats=args.formats, footer=footer)
        )
    for path in written:
        print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
