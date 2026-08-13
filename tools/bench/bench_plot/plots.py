"""Catalog figures for consolidated AirTree benchmark CSVs."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.colors import LogNorm

from bench_plot.style import (
    SCHEMA_COLORS,
    apply_style,
    save_fig,
    schema_hue_order,
    schema_palette,
)

_1D_SCHEMAS = ("1DxT", "1DxF", "1DxP")
_DIM_DATASETS = (
    "2025-08-FRED-MD",
    "2025-08-FRED-QD",
    "yellow_tripdata_2025_combined",
)
_SCHEMA_DIM_RE = re.compile(r"^([1-4])Dx([FP])$")
_VARIANT_LABEL = {"F": "Fast", "P": "Precise"}
_VARIANT_COLORS = {
    "Fast": SCHEMA_COLORS["1DxF"],
    "Precise": SCHEMA_COLORS["1DxP"],
}
_VARIANT_ORDER = ("Fast", "Precise")
_DIM_ORDER = (1, 2, 3, 4)
_1D_QUERY_FAMILIES = (
    ("TopK", frozenset({0, 1, 2})),
    ("MinMax", frozenset({3, 4, 5, 6})),
    ("Percentile", frozenset({7, 8})),
)
_MULTI_QUERY_IDS = frozenset({9, 10})
_QUERY_FAMILY_BY_ID = {
    0: "TopK",
    1: "TopK",
    2: "TopK",
    3: "MinMax",
    4: "MinMax",
    5: "MinMax",
    6: "MinMax",
    7: "Percentile",
    8: "Percentile",
    9: "Grid",
    10: "BoundingBox",
}
_QUERY_FAMILY_ORDER = ("TopK", "MinMax", "Percentile", "Grid", "BoundingBox")
_MULTI_SCHEMA_RE = re.compile(r"^[2-4]Dx[FP]$")


def _skip(name: str, reason: str) -> list[Path]:
    print(f"skip {name}: {reason}", file=sys.stderr)
    return []


def _dataset_order(df) -> list[str]:
    if "Dataset Size MB" not in df.columns:
        return sorted(df["Data_set"].astype(str).unique())
    sizes = df.groupby("Data_set", observed=True)["Dataset Size MB"].median()
    return sizes.sort_values().index.astype(str).tolist()


def _rotate_xlabels(ax) -> None:
    ax.tick_params(axis="x", labelrotation=35)
    for label in ax.get_xticklabels():
        label.set_ha("right")


def _query_label_order(df) -> list[str]:
    if "query_id" in df.columns:
        ranks = (
            df.dropna(subset=["query_label"])
            .groupby("query_label", observed=True)["query_id"]
            .min()
            .sort_values()
        )
        return ranks.index.astype(str).tolist()
    from bench_plot.load import QUERY_ID_LABELS

    present = set(df["query_label"].astype(str))
    ordered = [
        label for label in QUERY_ID_LABELS.values() if label in present
    ]
    extras = sorted(present - set(ordered))
    return ordered + extras


def _query_family(query_id: object) -> str:
    try:
        return _QUERY_FAMILY_BY_ID[int(query_id)]
    except (TypeError, ValueError, KeyError):
        return "other"


def _is_multid_schema(schema: object) -> bool:
    return bool(_MULTI_SCHEMA_RE.fullmatch(str(schema)))


def _create_and_insert(df):
    return df[df["fixture"] == "CreateAndInsert"].copy()


def _require_columns(df, columns: tuple[str, ...]) -> str | None:
    missing = [col for col in columns if col not in df.columns]
    if missing:
        return f"missing {missing[0]!r}" if len(missing) == 1 else f"missing {missing}"
    return None


def _positive(df, columns: tuple[str, ...]):
    out = df
    for col in columns:
        out = out[out[col] > 0]
    return out


def _dataset_family(name: object) -> str:
    text = str(name)
    if text.startswith("2025-08-FRED"):
        return "FRED"
    if text.startswith("yellow_tripdata"):
        return "taxi"
    if text.startswith("astro_"):
        return "astro"
    if text.startswith("num_"):
        return "numeric"
    return text


def _schema_dim_variant(schema: object) -> tuple[int, str] | None:
    match = _SCHEMA_DIM_RE.fullmatch(str(schema))
    if not match:
        return None
    return int(match.group(1)), _VARIANT_LABEL[match.group(2)]


def _finish_multipanel(fig, title: str, footer: str, *, top: float = 0.93) -> None:
    fig.suptitle(title)
    if footer:
        fig.text(0.01, 0.01, footer, fontsize=8, ha="left", va="bottom")
        fig.tight_layout(rect=(0, 0.04, 1, top))
    else:
        fig.tight_layout(rect=(0, 0, 1, top))


def _grouped_schema_bar(
    df,
    *,
    y: str,
    ylabel: str,
    title: str,
    name: str,
    out_dir: str | Path,
    formats: tuple[str, ...],
    footer: str,
) -> list[Path]:
    order = _dataset_order(df)
    hue_order = schema_hue_order(df["Schema"])
    width = max(8.0, 0.65 * len(order))
    fig, ax = plt.subplots(figsize=(width, 5.5))
    sns.barplot(
        data=df,
        x="Data_set",
        y=y,
        hue="Schema",
        order=order,
        hue_order=hue_order,
        palette=schema_palette(hue_order),
        errorbar=None,
        ax=ax,
    )
    ax.set_yscale("log")
    ax.set_xlabel("Dataset (ordered by size)")
    _rotate_xlabels(ax)
    apply_style(fig, ax, title=title, ylabel=ylabel, footer=footer)
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def generate_insert_points_per_sec(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Grouped bar, log y: CreateAndInsert Points_Per_Second by dataset × schema."""
    name = "generate_insert_points_per_sec"
    insert = generate_df[generate_df["fixture"] == "CreateAndInsert"].copy()
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    insert = insert.dropna(subset=["Points_Per_Second"])
    if insert.empty:
        return _skip(name, "no Points_Per_Second values")
    return _grouped_schema_bar(
        insert,
        y="Points_Per_Second",
        ylabel="Points per second",
        title="Insert throughput falls orders of magnitude from FRED to large sets",
        name=name,
        out_dir=out_dir,
        formats=formats,
        footer=footer,
    )


def generate_insert_mbps(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Grouped bar, log y: CreateAndInsert Insertion Speed (MB/s) by dataset × schema."""
    name = "generate_insert_mbps"
    col = "Insertion Speed (MB/s)"
    insert = generate_df[generate_df["fixture"] == "CreateAndInsert"].copy()
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    if col not in insert.columns:
        return _skip(name, f"missing {col!r}")
    insert = insert.dropna(subset=[col])
    if insert.empty:
        return _skip(name, f"no {col!r} values")
    return _grouped_schema_bar(
        insert,
        y=col,
        ylabel="Insertion speed (MB/s)",
        title="Insert MB/s also drops on large high-cardinality sets",
        name=name,
        out_dir=out_dir,
        formats=formats,
        footer=footer,
    )


def generate_create_vs_serialize(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Two-panel grouped bar: CreateAndInsert vs Serialize real_time_ms (log y)."""
    name = "generate_create_vs_serialize"
    if "real_time_ms" not in generate_df.columns:
        return _skip(name, "missing real_time_ms")
    frame = generate_df.dropna(subset=["real_time_ms"]).copy()
    create = frame[frame["fixture"] == "CreateAndInsert"]
    serialize = frame[frame["fixture"] == "Serialize"]
    if create.empty and serialize.empty:
        return _skip(name, "no CreateAndInsert or Serialize times")

    order_src = create if not create.empty else serialize
    order = _dataset_order(order_src)
    schemas = set(frame["Schema"])
    hue_order = schema_hue_order(schemas)
    width = max(10.0, 0.7 * len(order))
    fig, axes = plt.subplots(1, 2, figsize=(width, 5.5), sharey=True)
    panels = (
        (axes[0], create, "CreateAndInsert"),
        (axes[1], serialize, "Serialize"),
    )
    for ax, data, label in panels:
        if data.empty:
            ax.set_title(f"{label}: no rows")
            ax.set_yscale("log")
            continue
        sns.barplot(
            data=data,
            x="Data_set",
            y="real_time_ms",
            hue="Schema",
            order=order,
            hue_order=hue_order,
            palette=schema_palette(hue_order),
            errorbar=None,
            ax=ax,
        )
        ax.set_yscale("log")
        ax.set_xlabel("Dataset (ordered by size)")
        ax.set_ylabel("Wall time (ms)")
        _rotate_xlabels(ax)
        apply_style(
            fig, ax, title=label, ylabel="Wall time (ms)", footer="", tighten=False
        )

    fig.suptitle("Serialize is cheap next to CreateAndInsert on large sets")
    if footer:
        fig.text(0.01, 0.01, footer, fontsize=8, ha="left", va="bottom")
        fig.tight_layout(rect=(0, 0.04, 1, 0.93))
    else:
        fig.tight_layout(rect=(0, 0, 1, 0.93))
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def generate_trie_vs_input(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Log–log scatter: Dataset Size MB vs Trie Size (Bytes), CreateAndInsert."""
    name = "generate_trie_vs_input"
    x_col = "Dataset Size MB"
    y_col = "Trie Size (Bytes)"
    insert = _create_and_insert(generate_df)
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    missing = _require_columns(insert, (x_col, y_col))
    if missing:
        return _skip(name, missing)
    insert = insert.dropna(subset=[x_col, y_col, "Schema"])
    insert = _positive(insert, (x_col, y_col))
    if insert.empty:
        return _skip(name, f"no positive {x_col!r} / {y_col!r} values")

    insert = insert.copy()
    insert["family"] = insert["Data_set"].map(_dataset_family)
    hue_order = schema_hue_order(insert["Schema"])
    fig, ax = plt.subplots(figsize=(8.5, 5.5))
    sns.scatterplot(
        data=insert,
        x=x_col,
        y=y_col,
        hue="Schema",
        hue_order=hue_order,
        palette=schema_palette(hue_order),
        style="family",
        s=70,
        ax=ax,
    )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Dataset size (MB)")
    apply_style(
        fig,
        ax,
        title="Trie size tracks input size; Precise is larger than Fast at the same set",
        ylabel="Trie size (bytes)",
        footer=footer,
    )
    ax.grid(True, which="both", linestyle=":", alpha=0.55)
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def generate_1d_variant_tradeoff(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Two-panel bar: 1DxT/F/P insert rate vs trie size. Renders whichever 1D variants exist."""
    name = "generate_1d_variant_tradeoff"
    rate_col = "Points_Per_Second"
    size_col = "Trie Size (Bytes)"
    insert = _create_and_insert(generate_df)
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    missing = _require_columns(insert, (rate_col, size_col))
    if missing:
        return _skip(name, missing)
    one_d = insert[insert["Schema"].isin(_1D_SCHEMAS)].copy()
    if one_d.empty:
        return _skip(name, "no 1DxT / 1DxF / 1DxP rows")
    one_d = one_d.dropna(subset=[rate_col, size_col])
    one_d = _positive(one_d, (rate_col, size_col))
    if one_d.empty:
        return _skip(name, f"no positive {rate_col!r} / {size_col!r} values")

    order = _dataset_order(one_d)
    hue_order = schema_hue_order(one_d["Schema"])
    width = max(10.0, 0.7 * len(order))
    fig, axes = plt.subplots(1, 2, figsize=(width, 5.5), sharex=True)
    panels = (
        (axes[0], rate_col, "Insert rate (points/s)", True),
        (axes[1], size_col, "Trie size (bytes)", True),
    )
    for ax, y, ylabel, log_y in panels:
        sns.barplot(
            data=one_d,
            x="Data_set",
            y=y,
            hue="Schema",
            order=order,
            hue_order=hue_order,
            palette=schema_palette(hue_order),
            errorbar=None,
            ax=ax,
        )
        if log_y:
            ax.set_yscale("log")
        ax.set_xlabel("Dataset (ordered by size)")
        _rotate_xlabels(ax)
        apply_style(fig, ax, title=ylabel, ylabel=ylabel, footer="", tighten=False)

    _finish_multipanel(
        fig,
        "Tiny / Fast / Precise trade insert rate for a larger trie on 1D",
        footer,
    )
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def generate_dim_scaling(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Grouped bar: Fast vs Precise insert rate across 1D–4D on FRED and taxi only."""
    name = "generate_dim_scaling"
    y_col = "Points_Per_Second"
    insert = _create_and_insert(generate_df)
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    missing = _require_columns(insert, (y_col,))
    if missing:
        return _skip(name, missing)

    frame = insert[insert["Data_set"].isin(_DIM_DATASETS)].copy()
    if frame.empty:
        return _skip(
            name,
            "no FRED-MD / FRED-QD / yellow_tripdata_2025_combined rows",
        )
    parts = frame["Schema"].map(_schema_dim_variant)
    frame = frame.loc[parts.notna()].copy()
    if frame.empty:
        return _skip(name, "no Fast/Precise 1D–4D rows for dim-scaling datasets")
    parsed = [_schema_dim_variant(s) for s in frame["Schema"]]
    frame["dim"] = [p[0] for p in parsed]
    frame["variant"] = [p[1] for p in parsed]
    frame["dim_label"] = frame["dim"].map(lambda d: f"{d}D")
    frame = frame.dropna(subset=[y_col])
    frame = _positive(frame, (y_col,))
    if frame.empty:
        return _skip(name, f"no positive {y_col!r} values")
    if int(frame["dim"].max()) < 2:
        return _skip(name, "no 2D–4D Fast/Precise rows (fixture may be 1D-only)")

    datasets = [d for d in _dataset_order(frame) if d in set(frame["Data_set"])]
    width = max(10.0, 4.0 * len(datasets))
    fig, axes = plt.subplots(1, len(datasets), figsize=(width, 5.5), sharey=True)
    if len(datasets) == 1:
        axes = [axes]
    hue_order = [v for v in _VARIANT_ORDER if v in set(frame["variant"])]
    palette = {v: _VARIANT_COLORS[v] for v in hue_order}
    dim_labels = [f"{d}D" for d in _DIM_ORDER]
    for ax, dataset in zip(axes, datasets):
        data = frame[frame["Data_set"] == dataset]
        sns.barplot(
            data=data,
            x="dim_label",
            y=y_col,
            hue="variant",
            order=dim_labels,
            hue_order=hue_order,
            palette=palette,
            errorbar=None,
            ax=ax,
        )
        ax.set_yscale("log")
        ax.set_xlabel("Dimensions")
        apply_style(
            fig,
            ax,
            title=dataset,
            ylabel="Points per second",
            footer="",
            tighten=False,
        )

    _finish_multipanel(
        fig,
        "Insert throughput falls as dimensionality grows from 1D to 4D",
        footer,
    )
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def generate_cardinality(
    generate_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Scatter: Distinct Values vs Precise Bins, color = schema."""
    name = "generate_cardinality"
    x_col = "Distinct Values"
    y_col = "Precise Bins"
    insert = _create_and_insert(generate_df)
    if insert.empty:
        return _skip(name, "no CreateAndInsert rows")
    missing = _require_columns(insert, (x_col, y_col))
    if missing:
        return _skip(name, missing)
    insert = insert.dropna(subset=[x_col, y_col, "Schema"])
    insert = _positive(insert, (x_col, y_col))
    if insert.empty:
        return _skip(name, f"no positive {x_col!r} / {y_col!r} values")

    hue_order = schema_hue_order(insert["Schema"])
    fig, ax = plt.subplots(figsize=(8.5, 5.5))
    sns.scatterplot(
        data=insert,
        x=x_col,
        y=y_col,
        hue="Schema",
        hue_order=hue_order,
        palette=schema_palette(hue_order),
        s=70,
        ax=ax,
    )
    lo = min(insert[x_col].min(), insert[y_col].min())
    hi = max(insert[x_col].max(), insert[y_col].max())
    ax.plot([lo, hi], [lo, hi], ls="--", c="0.45", lw=1, zorder=0)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Distinct values")
    apply_style(
        fig,
        ax,
        title="Precise Bins stay far below Distinct Values on high-cardinality sets",
        ylabel="Precise bins",
        footer=footer,
    )
    ax.grid(True, which="both", linestyle=":", alpha=0.55)
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def query_latency_overview(
    query_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Grouped bar, log y: real_time_ms by query_label, hue = Schema."""
    name = "query_latency_overview"
    if query_df is None or getattr(query_df, "empty", True):
        return _skip(name, "no query rows")
    missing = _require_columns(query_df, ("query_label", "Schema", "real_time_ms"))
    if missing:
        return _skip(name, missing)
    frame = query_df.dropna(subset=["query_label", "Schema", "real_time_ms"]).copy()
    frame = _positive(frame, ("real_time_ms",))
    if frame.empty:
        return _skip(name, "no positive real_time_ms values")

    order = _query_label_order(frame)
    hue_order = schema_hue_order(frame["Schema"])
    width = max(11.0, 1.15 * len(order))
    fig, ax = plt.subplots(figsize=(width, 5.5))
    sns.barplot(
        data=frame,
        x="query_label",
        y="real_time_ms",
        hue="Schema",
        order=order,
        hue_order=hue_order,
        palette=schema_palette(hue_order),
        errorbar=None,
        ax=ax,
    )
    ax.set_yscale("log")
    ax.set_xlabel("Query")
    _rotate_xlabels(ax)
    apply_style(
        fig,
        ax,
        title="Query latency spans microseconds to a multi-D grid outlier",
        ylabel="Wall time (ms)",
        footer=footer,
    )
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def query_1d_families(
    query_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Three-facet bar: TopK, MinMax, Percentile for 1DxT/F/P."""
    name = "query_1d_families"
    if query_df is None or getattr(query_df, "empty", True):
        return _skip(name, "no query rows")
    missing = _require_columns(
        query_df, ("query_id", "query_label", "Schema", "real_time_ms")
    )
    if missing:
        return _skip(name, missing)
    frame = query_df.dropna(
        subset=["query_id", "query_label", "Schema", "real_time_ms"]
    ).copy()
    frame = frame[frame["Schema"].isin(_1D_SCHEMAS)]
    family_ids = set().union(*(ids for _, ids in _1D_QUERY_FAMILIES))
    frame = frame[frame["query_id"].isin(family_ids)]
    frame = _positive(frame, ("real_time_ms",))
    if frame.empty:
        return _skip(name, "no 1DxT/F/P TopK/MinMax/Percentile rows")

    hue_order = schema_hue_order(frame["Schema"])
    fig, axes = plt.subplots(1, 3, figsize=(14.0, 5.5), sharey=True)
    for ax, (family, ids) in zip(axes, _1D_QUERY_FAMILIES):
        data = frame[frame["query_id"].isin(ids)]
        if data.empty:
            ax.set_title(f"{family}: no rows")
            ax.set_yscale("log")
            continue
        order = _query_label_order(data)
        sns.barplot(
            data=data,
            x="query_label",
            y="real_time_ms",
            hue="Schema",
            order=order,
            hue_order=hue_order,
            palette=schema_palette(hue_order),
            errorbar=None,
            ax=ax,
        )
        ax.set_yscale("log")
        ax.set_xlabel(family)
        _rotate_xlabels(ax)
        apply_style(
            fig, ax, title=family, ylabel="Wall time (ms)", footer="", tighten=False
        )

    _finish_multipanel(
        fig,
        "Precise 1D queries cost more than Tiny/Fast across TopK, MinMax, Percentile",
        footer,
    )
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def query_multid(
    query_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Grid and BoundingBox for 2D–4D schemas. Missing ops stay absent, not zero."""
    name = "query_multid"
    if query_df is None or getattr(query_df, "empty", True):
        return _skip(name, "no query rows")
    missing = _require_columns(
        query_df, ("query_id", "query_label", "Schema", "real_time_ms")
    )
    if missing:
        return _skip(name, missing)
    frame = query_df.dropna(
        subset=["query_id", "query_label", "Schema", "real_time_ms"]
    ).copy()
    frame = frame[
        frame["query_id"].isin(_MULTI_QUERY_IDS) & frame["Schema"].map(_is_multid_schema)
    ]
    frame = _positive(frame, ("real_time_ms",))
    if frame.empty:
        return _skip(name, "no 2D–4D Grid/BoundingBox rows")

    ops = (
        (9, "Grid steps=8"),
        (10, "BoundingBox mid-50%"),
    )
    fig, axes = plt.subplots(1, 2, figsize=(11.0, 5.5), sharey=True)
    for ax, (qid, label) in zip(axes, ops):
        data = frame[frame["query_id"] == qid]
        if data.empty:
            ax.set_title(f"{label}: no rows")
            ax.set_yscale("log")
            continue
        hue_order = schema_hue_order(data["Schema"])
        sns.barplot(
            data=data,
            x="Schema",
            y="real_time_ms",
            hue="Schema",
            order=hue_order,
            hue_order=hue_order,
            palette=schema_palette(hue_order),
            errorbar=None,
            dodge=False,
            ax=ax,
        )
        ax.set_yscale("log")
        ax.set_xlabel("Schema")
        apply_style(
            fig, ax, title=label, ylabel="Wall time (ms)", footer="", tighten=False
        )

    _finish_multipanel(
        fig,
        "Grid cost grows with dimension; 4DxP has no BoundingBox",
        footer,
    )
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def query_latency_vs_buffersize(
    query_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Log–log scatter: BufferSize vs real_time_ms, color = family, marker = schema."""
    name = "query_latency_vs_buffersize"
    if query_df is None or getattr(query_df, "empty", True):
        return _skip(name, "no query rows")
    missing = _require_columns(
        query_df, ("BufferSize", "real_time_ms", "Schema", "query_id")
    )
    if missing:
        return _skip(name, missing)
    frame = query_df.dropna(
        subset=["BufferSize", "real_time_ms", "Schema", "query_id"]
    ).copy()
    frame = _positive(frame, ("BufferSize", "real_time_ms"))
    if frame.empty:
        return _skip(name, "no positive BufferSize / real_time_ms values")

    frame["family"] = frame["query_id"].map(_query_family)
    family_order = [f for f in _QUERY_FAMILY_ORDER if f in set(frame["family"])]
    extras = sorted(set(frame["family"]) - set(family_order))
    family_order = family_order + extras
    style_order = schema_hue_order(frame["Schema"])
    fig, ax = plt.subplots(figsize=(8.5, 5.5))
    sns.scatterplot(
        data=frame,
        x="BufferSize",
        y="real_time_ms",
        hue="family",
        hue_order=family_order,
        style="Schema",
        style_order=style_order,
        s=70,
        ax=ax,
    )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Buffer size (bytes)")
    apply_style(
        fig,
        ax,
        title="Queries are slower on larger .airtree buffers; Grid sits above 1D families",
        ylabel="Wall time (ms)",
        footer=footer,
    )
    ax.grid(True, which="both", linestyle=":", alpha=0.55)
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)


def query_heatmap(
    query_df,
    out_dir: str | Path,
    formats: tuple[str, ...] = ("png",),
    footer: str = "",
) -> list[Path]:
    """Schema × query_label heatmap of real_time_ms. Missing cells stay blank, not 0."""
    name = "query_heatmap"
    if query_df is None or getattr(query_df, "empty", True):
        return _skip(name, "no query rows")
    missing = _require_columns(query_df, ("query_label", "Schema", "real_time_ms"))
    if missing:
        return _skip(name, missing)
    frame = query_df.dropna(subset=["query_label", "Schema", "real_time_ms"]).copy()
    frame = _positive(frame, ("real_time_ms",))
    if frame.empty:
        return _skip(name, "no positive real_time_ms values")

    pivot = frame.pivot_table(
        index="Schema",
        columns="query_label",
        values="real_time_ms",
        aggfunc="median",
    )
    row_order = schema_hue_order(pivot.index)
    col_order = _query_label_order(
        frame[frame["query_label"].isin(pivot.columns)]
    )
    pivot = pivot.reindex(index=row_order, columns=col_order)
    lo = float(pivot.min().min())
    hi = float(pivot.max().max())
    if lo != lo or hi != hi:  # NaN: no numeric cells
        return _skip(name, "no heatmap cells")
    if hi <= lo:
        hi = lo * 10.0

    height = max(3.8, 0.55 * len(row_order) + 2.0)
    width = max(10.0, 0.95 * len(col_order) + 2.5)
    fig, ax = plt.subplots(figsize=(width, height))
    sns.heatmap(
        pivot,
        ax=ax,
        mask=pivot.isna(),
        annot=True,
        fmt=".3g",
        norm=LogNorm(vmin=lo, vmax=hi),
        cmap="cividis",
        linewidths=0.4,
        linecolor="0.9",
        cbar_kws={"label": "Wall time (ms)"},
    )
    ax.set_xlabel("Query")
    ax.set_ylabel("Schema")
    ax.set_title("Where queries are expensive (blank is missing, not zero)")
    ax.tick_params(axis="x", labelrotation=35)
    for label in ax.get_xticklabels():
        label.set_ha("right")
    if footer:
        fig.text(0.01, 0.01, footer, fontsize=8, ha="left", va="bottom")
        fig.tight_layout(rect=(0, 0.06, 1, 1))
    else:
        fig.tight_layout()
    return save_fig(fig, Path(out_dir) / f"{name}.png", formats)
