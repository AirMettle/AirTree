"""Shared colors, titles, and save helper for benchmark figures."""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import seaborn as sns

DPI = 180

SCHEMA_ORDER = [
    "1DxT",
    "1DxF",
    "1DxP",
    "2DxF",
    "2DxP",
    "3DxF",
    "3DxP",
    "4DxF",
    "4DxP",
]

# Okabe–Ito plus indigo / wine so all nine schemas stay distinguishable.
SCHEMA_COLORS = {
    "1DxT": "#E69F00",
    "1DxF": "#56B4E9",
    "1DxP": "#009E73",
    "2DxF": "#0072B2",
    "2DxP": "#D55E00",
    "3DxF": "#CC79A7",
    "3DxP": "#F0E442",
    "4DxF": "#332288",
    "4DxP": "#882255",
}


def schema_palette(schemas: list[str] | set[str]) -> dict[str, str]:
    return {name: SCHEMA_COLORS[name] for name in schemas if name in SCHEMA_COLORS}


def schema_hue_order(schemas: list[str] | set[str]) -> list[str]:
    present = set(schemas)
    return [name for name in SCHEMA_ORDER if name in present]


def apply_style(
    fig,
    ax,
    title: str,
    ylabel: str,
    footer: str = "",
    *,
    tighten: bool = True,
) -> None:
    ax.set_title(title)
    ax.set_ylabel(ylabel)
    ax.grid(True, axis="y", linestyle=":", alpha=0.55)
    if ax.get_legend() is not None:
        ax.legend(bbox_to_anchor=(1.02, 1), loc="upper left", borderaxespad=0.0)
    if footer:
        fig.text(0.01, 0.01, footer, fontsize=8, ha="left", va="bottom")
    if tighten:
        fig.tight_layout(rect=(0, 0.04, 1, 1) if footer else None)


def save_fig(fig, path: str | Path, formats: tuple[str, ...] = ("png",)) -> list[Path]:
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    stem = path.with_suffix("")
    written: list[Path] = []
    for fmt in formats:
        out = stem.with_suffix(f".{fmt.lstrip('.')}")
        fig.savefig(out, dpi=DPI, bbox_inches="tight")
        written.append(out)
    plt.close(fig)
    return written


def write_smoke_plot(run_dir: str | Path, out_path: str | Path) -> list[Path]:
    """One non-catalog bar chart so style/save can be checked in isolation."""
    from bench_plot.load import load_generate

    df = load_generate(run_dir)
    df = df[df["fixture"] == "CreateAndInsert"].copy()
    if df.empty:
        raise ValueError(f"no CreateAndInsert rows in {run_dir}")

    fig, ax = plt.subplots(figsize=(8, 5))
    hue_order = schema_hue_order(df["Schema"])
    sns.barplot(
        data=df,
        x="Data_set",
        y="Points_Per_Second",
        hue="Schema",
        hue_order=hue_order,
        palette=schema_palette(hue_order),
        ax=ax,
    )
    ax.set_yscale("log")
    ax.set_xlabel("Dataset")
    apply_style(
        fig,
        ax,
        title="Smoke: insert throughput (Points_Per_Second)",
        ylabel="Points per second",
        footer="phase 5 smoke — not a catalog figure",
    )
    return save_fig(fig, out_path)


if __name__ == "__main__":
    import sys

    repo_bench = Path(__file__).resolve().parent.parent
    if str(repo_bench) not in sys.path:
        sys.path.insert(0, str(repo_bench))

    fixture = repo_bench / "testdata" / "plot_fixture"
    out = fixture / "plots" / "_smoke.png"
    paths = write_smoke_plot(fixture, out)
    for written in paths:
        print(written)
