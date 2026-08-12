"""Catalog figures for consolidated AirTree benchmark CSVs."""

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import seaborn as sns

from bench_plot.style import (
    apply_style,
    save_fig,
    schema_hue_order,
    schema_palette,
)


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
