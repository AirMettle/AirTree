# Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

import glob
import os
import numpy as np
import pandas as pd

script_path = os.path.abspath(__file__)
script_folder = os.path.dirname(script_path)
project_path = os.path.abspath(os.path.join(script_path, "../.."))


def get_test_data_files():
    # Get all .bin files in the build directory
    test_files = glob.glob(
        os.path.join(project_path, "build", "tests", "**", "*special*.bin"),
        recursive=True,
    )
    print(f"Found {len(test_files)} test data files in the build directory.")
    # Filter out files that contain 'FnTest' in their path
    if len(test_files) != 28:
        raise RuntimeError(
            "Test data files missing or incomplete. "
            "Please generate test data first by uncommenting the Generate_special_export_data "
            "function in tests/FnTest/4-D.cpp and rebuilding the project."
        )


def check_export_executable():
    export_executable = os.path.join(script_folder, "Export")
    if not os.path.isfile(export_executable):
        raise RuntimeError(
            f"Export executable not found at {export_executable}. "
            "Please build it by running './build.sh' in the script folder."
        )
    else:
        print(f"Export executable found at {export_executable}.")


def validate():
    parquet_input_dir = os.path.join(script_folder, "parquet")
    parquet_files = glob.glob(os.path.join(parquet_input_dir, "*.parquet"))
    for parquet_file in parquet_files:
        filename = os.path.basename(parquet_file)
        # print(f"Validating file: {parquet_file}")
        if "1D" in filename:
            # continue
            print(f"Validating 1D file: {parquet_file}")

            table = pd.read_parquet(parquet_file, engine="fastparquet")
            print(table)
            assert len(table) == 5
            # Ensure all counts in each row are 1
            if "counts" not in table.columns:
                raise ValueError(f"'counts' column not found in {parquet_file}")
            if not np.all(table["counts"] == 1):
                raise AssertionError(f"Not all counts are 1 in {parquet_file}")
            expected_values = [np.inf, -np.inf, 0.0, -0.0, np.nan]
            actual_values = table["value"].to_numpy()

            # Check for inf, -inf, 0.0, -0.0, and NaN (order-insensitive)
            def is_equal(a, b):
                if np.isnan(a) and np.isnan(b):
                    return True
                if a == b:
                    # Distinguish 0.0 and -0.0
                    return np.signbit(a) == np.signbit(b)
                return False

            for ev in expected_values:
                if not any(is_equal(ev, av) for av in actual_values):
                    raise AssertionError(
                        f"Expected value {ev} not found in {parquet_file}"
                    )
        elif "2D" in filename:
            continue
            table = pd.read_parquet(parquet_file, engine="fastparquet")
            print(f"Validating 2DxP file: {parquet_file}")
            print(table)
        elif "3D" in filename:
            continue
            table = pd.read_parquet(parquet_file, engine="fastparquet")
            print(f"Validating 3DxP file: {parquet_file}")
            print(table)
        elif "4D" in filename:
            continue
            table = pd.read_parquet(parquet_file, engine="fastparquet")
            print(f"Validating 4DxP file: {parquet_file}")
            print(table)


def test_special_export_data():

    try:
        get_test_data_files()
        print("All special export data files are present.")
    except RuntimeError as e:
        print(f"Validation failed: {e}")
    check_export_executable()

    # Create output directory for parquet files if it doesn't exist
    parquet_output_dir = os.path.join(script_folder, "parquet")
    os.makedirs(parquet_output_dir, exist_ok=True)

    # Get all test data files again
    test_files = glob.glob(
        os.path.join(project_path, "build", "tests", "**", "*special*.bin"),
        recursive=True,
    )

    for test_file in test_files:
        filename = os.path.basename(test_file)
        config_type = filename.split("_")[0]
        output_path = os.path.join(
            parquet_output_dir, filename.replace(".bin", ".parquet")
        )
        cmd = [
            os.path.join(script_folder, "Export"),
            config_type,
            test_file,
            "--parquet",
            "--output",
            output_path,
        ]
        print(f"Running: {' '.join(cmd)}")
        ret = os.system(" ".join(f'"{c}"' if " " in c else c for c in cmd))
        if ret != 0:
            print(f"Export failed for {test_file}")
        else:
            print(f"Parquet file generated at {output_path}")
    validate()


test_special_export_data()
