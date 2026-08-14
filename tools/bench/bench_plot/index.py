"""Write plots/index.md so a run's figures read as one scroll."""

from __future__ import annotations

from pathlib import Path

# (section, stem, caption) — only stems that were actually written are embedded.
FIGURES: tuple[tuple[str, str, str], ...] = (
    (
        "Generate",
        "generate_insert_points_per_sec",
        "Insert throughput (`Points_Per_Second`) by dataset × schema",
    ),
    (
        "Generate",
        "generate_insert_mbps",
        "Insert throughput (`Insertion Speed (MB/s)`) by dataset × schema",
    ),
    (
        "Generate",
        "generate_create_vs_serialize",
        "CreateAndInsert vs Serialize wall time (ms)",
    ),
    (
        "Generate",
        "generate_trie_vs_input",
        "Dataset size vs trie size",
    ),
    (
        "Generate",
        "generate_1d_variant_tradeoff",
        "1DxT / 1DxF / 1DxP insert rate vs trie size",
    ),
    (
        "Generate",
        "generate_dim_scaling",
        "Fast vs Precise insert rate across 1D–4D (FRED + yellow combined)",
    ),
    (
        "Generate",
        "generate_cardinality",
        "Distinct Values vs Precise Bins",
    ),
    (
        "Generate",
        "generate_input_profile",
        "1D input facts (size, row count, distinct)",
    ),
    (
        "Generate",
        "generate_trie_size",
        "1D trie size",
    ),
    (
        "Generate",
        "generate_precise_bins",
        "1D precise bins",
    ),
    (
        "Generate",
        "generate_avg_bytes_per_bin",
        "1D average bytes per bin",
    ),
    (
        "Query",
        "query_latency_overview",
        "Query latency by query label × schema",
    ),
    (
        "Query",
        "query_1d_families",
        "TopK, MinMax, Percentile for 1DxT / 1DxF / 1DxP",
    ),
    (
        "Query",
        "query_multid",
        "Grid and BoundingBox for 2DxP / 3DxP / 4DxP",
    ),
    (
        "Query",
        "query_latency_vs_buffersize",
        "Latency vs `.airtree` buffer size",
    ),
    (
        "Query",
        "query_heatmap",
        "Schema × query label latency heatmap",
    ),
)


def write_index(
    out_dir: str | Path,
    written: list[Path],
    footer: str = "",
) -> Path:
    """Embed written catalog figures. Skipped stems are omitted, not listed as empty."""
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    by_stem: dict[str, dict[str, str]] = {}
    for path in written:
        by_stem.setdefault(path.stem, {})[path.suffix.lstrip(".").lower()] = path.name

    lines = ["# AirTree benchmark plots", ""]
    if footer:
        lines.extend([footer, ""])

    current_section: str | None = None
    for section, stem, caption in FIGURES:
        fmts = by_stem.get(stem)
        if not fmts:
            continue
        if section != current_section:
            lines.extend([f"## {section}", ""])
            current_section = section
        svg = fmts.get("svg")
        heading = f"### {caption}"
        if svg:
            heading += f" ([svg]({svg}))"
        lines.append(heading)
        lines.append("")
        image = fmts.get("png") or svg
        if image:
            lines.append(f"![{stem}]({image})")
            lines.append("")

    path = out_dir / "index.md"
    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
    return path
