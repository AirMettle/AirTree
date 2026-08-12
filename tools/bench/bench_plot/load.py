"""Load consolidated generate and query benchmark CSVs from a run directory."""

from __future__ import annotations

import re
import sys
from pathlib import Path

import pandas as pd

GENERATE_GLOB = "consolidated_*_data.csv"
QUERY_GLOB = "consolidated_query_*_data.csv"
QUERY_ALL_NAME = "consolidated_query_all_data.csv"
SYSTEMINFO_NAME = "systeminfo.csv"
SCHEMA_FROM_NAME = re.compile(r"^consolidated_(.+)_data\.csv$")
NAME_RE = re.compile(
    r"^AirTreeBench[^/]+/"
    r"(?P<fixture>CreateAndInsert|Serialize)_"
    r"(?P<dtype>float|double)"
)

# From QueryFixtureBase.hpp / airtree-bench/README.md. Do not invent new IDs.
QUERY_ID_LABELS: dict[int, str] = {
    0: "TopK k=1",
    1: "TopK k=5",
    2: "TopK k=15",
    3: "MinMax getMin",
    4: "MinMax getMax",
    5: "MinMax getMinValue",
    6: "MinMax getMaxValue",
    7: "Percentile p50",
    8: "Percentile p90",
    9: "Grid steps=8",
    10: "BoundingBox mid-50%",
}

_QUERY_EMPTY_COLUMNS = [
    "Data_set",
    "Schema",
    "query_id",
    "query_label",
    "result_size_B",
    "real_time_ms",
    "cpu_time_ms",
]


class LoadError(ValueError):
    """Consolidated CSV is missing, malformed, or not the expected format."""


def _ms_per_unit(time_unit: str) -> float:
    unit = str(time_unit).strip().lower()
    if unit == "ns":
        return 1e-6
    if unit in ("us", "µs"):
        return 1e-3
    if unit == "ms":
        return 1.0
    if unit == "s":
        return 1e3
    raise LoadError(f"unknown time_unit: {time_unit!r}")


def _schema_from_filename(path: Path) -> str:
    match = SCHEMA_FROM_NAME.match(path.name)
    if not match:
        raise LoadError(f"cannot parse schema from filename: {path.name}")
    return match.group(1)


def _assert_consolidated_generate(path: Path) -> None:
    with path.open(newline="") as handle:
        first = handle.readline()
    if not first:
        raise LoadError(f"empty file: {path}")
    header = first.strip()
    if not header.startswith("Data_set,"):
        raise LoadError(
            f"{path} is not a consolidated generate CSV "
            f"(expected header starting with Data_set, got {header[:80]!r})"
        )


def load_generate(run_dir: str | Path) -> pd.DataFrame:
    """Return one frame of generate rows from consolidated_<schema>_data.csv files.

    Skips consolidated_query_*.csv. Adds Schema (from the filename), fixture,
    dtype, real_time_ms, and cpu_time_ms. Does not fill missing counters with 0.
    """
    run_dir = Path(run_dir)
    if not run_dir.is_dir():
        raise LoadError(f"not a directory: {run_dir}")

    frames: list[pd.DataFrame] = []
    for path in sorted(run_dir.glob(GENERATE_GLOB)):
        if path.name.startswith("consolidated_query_"):
            continue
        _assert_consolidated_generate(path)
        schema = _schema_from_filename(path)
        df = pd.read_csv(path)
        if "Data_set" not in df.columns or "name" not in df.columns:
            raise LoadError(f"{path} missing Data_set or name column")
        parsed = df["name"].astype(str).str.extract(NAME_RE)
        if parsed["fixture"].isna().any():
            bad = df.loc[parsed["fixture"].isna(), "name"].tolist()
            raise LoadError(f"{path} has unparseable name(s): {bad}")

        out = df.copy()
        out["Schema"] = schema
        out["fixture"] = parsed["fixture"].to_numpy()
        out["dtype"] = parsed["dtype"].to_numpy()
        factors = out["time_unit"].map(_ms_per_unit)
        out["real_time_ms"] = out["real_time"] * factors
        out["cpu_time_ms"] = out["cpu_time"] * factors
        frames.append(out)

    if not frames:
        raise LoadError(f"no consolidated generate CSVs in {run_dir}")
    return pd.concat(frames, ignore_index=True)


def _query_label(query_id: object) -> str:
    if pd.isna(query_id):
        return ""
    qid = int(query_id)
    return QUERY_ID_LABELS.get(qid, f"query_id={qid}")


def _assert_consolidated_query(path: Path) -> None:
    with path.open(newline="") as handle:
        first = handle.readline()
    if not first:
        raise LoadError(f"empty file: {path}")
    header = first.strip()
    if not header.startswith("Data_set,"):
        raise LoadError(
            f"{path} is not a consolidated query CSV "
            f"(expected header starting with Data_set, got {header[:80]!r})"
        )


def _alias_result_size(df: pd.DataFrame) -> pd.DataFrame:
    if "result_size_B" not in df.columns and "result_size" in df.columns:
        return df.rename(columns={"result_size": "result_size_B"})
    return df


def load_query(run_dir: str | Path) -> pd.DataFrame:
    """Return query rows from consolidated_query_<schema>_data.csv files.

    Skips consolidated_query_all_data.csv. Adds query_label and real_time_ms.
    Aliases result_size to result_size_B. Missing query files yield an empty
    frame (no exception).
    """
    run_dir = Path(run_dir)
    if not run_dir.is_dir():
        raise LoadError(f"not a directory: {run_dir}")

    frames: list[pd.DataFrame] = []
    for path in sorted(run_dir.glob(QUERY_GLOB)):
        if path.name == QUERY_ALL_NAME:
            continue
        _assert_consolidated_query(path)
        df = pd.read_csv(path)
        required = {"Data_set", "Schema", "query_id"}
        missing = required - set(df.columns)
        if missing:
            raise LoadError(f"{path} missing columns: {sorted(missing)}")
        out = _alias_result_size(df.copy())
        out["query_label"] = out["query_id"].map(_query_label)
        if "time_unit" in out.columns and "real_time" in out.columns:
            factors = out["time_unit"].map(_ms_per_unit)
            out["real_time_ms"] = out["real_time"] * factors
            if "cpu_time" in out.columns:
                out["cpu_time_ms"] = out["cpu_time"] * factors
        frames.append(out)

    if not frames:
        return pd.DataFrame(columns=_QUERY_EMPTY_COLUMNS)
    return pd.concat(frames, ignore_index=True)


def load_systeminfo(run_dir: str | Path) -> str:
    """One footer line from systeminfo.csv, or empty if the file is absent."""
    path = Path(run_dir) / SYSTEMINFO_NAME
    if not path.is_file():
        return ""
    df = pd.read_csv(path)
    if "Ran on" not in df.columns:
        return ""
    values = df["Ran on"].dropna().astype(str).str.strip()
    values = values[values != ""]
    if values.empty:
        return ""
    return f"Ran on {values.iloc[0]}"


def _probe(run_dir: Path) -> None:
    df = load_generate(run_dir)
    print(f"shape={df.shape}")
    print(f"schemas={sorted(df['Schema'].unique())}")
    print("fixture counts:")
    print(df["fixture"].value_counts().to_string())

    serialize = df[df["fixture"] == "Serialize"]
    nan_ok = serialize["Points_Per_Second"].isna().any() if not serialize.empty else False
    print(f"serialize_rows={len(serialize)} serialize_points_per_sec_has_nan={nan_ok}")

    fred = df[
        (df["Schema"] == "1DxF")
        & (df["Data_set"] == "2025-08-FRED-MD")
        & (df["fixture"] == "CreateAndInsert")
    ]
    if fred.empty:
        raise SystemExit("probe expected FRED-MD 1DxF CreateAndInsert row")
    got = float(fred.iloc[0]["real_time_ms"])
    expected = 14494.7 * 1e-6
    if abs(got - expected) > 1e-12:
        raise SystemExit(f"ns->ms failed: got {got}, expected {expected}")
    print(f"ns_to_ms_ok real_time_ms={got}")

    qdf = load_query(run_dir)
    print(f"query_shape={qdf.shape}")
    print(f"query_schemas={sorted(qdf['Schema'].unique()) if not qdf.empty else []}")
    topk = qdf[qdf["query_id"] == 0]
    if not topk.empty:
        label = topk.iloc[0]["query_label"]
        if label != "TopK k=1":
            raise SystemExit(f"query_id 0 label failed: {label!r}")
        print(f"query_id_0_label={label}")
    if "Dims" in qdf.columns and "Param_steps" in qdf.columns:
        multi = qdf[qdf["Schema"] == "2DxP"]
        print(f"query_2DxP_has_dims={not multi.empty and multi['Dims'].notna().any()}")
    print(f"systeminfo={load_systeminfo(run_dir)!r}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <run_dir>", file=sys.stderr)
        raise SystemExit(2)
    _probe(Path(sys.argv[1]))
