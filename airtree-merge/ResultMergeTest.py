import os
import subprocess


def run_result_merge(config):
    command = f"./ResultMerge {config} ../tests/TestData/merge_test/{config}_piece1.bin ../tests/TestData/merge_test/{config}_piece2.bin ./result_{config}.bin"
    print(f"Executing command: {command}")
    try:
        result = subprocess.run(
            command, shell=True, check=True, capture_output=True, text=True
        )
        output = result.stdout
        print(output.splitlines())
        file_1_size = next(
            (line for line in output.splitlines() if "File 1 Size:" in line), None
        )
        file_2_size = next(
            (line for line in output.splitlines() if "File 2 Size:" in line), None
        )
        total_size = next(
            (line for line in output.splitlines() if "Total Size:" in line), None
        )
        time_line = next(
            (line for line in output.splitlines() if "Execution Time:" in line), None
        )
        memory_line = next(
            (line for line in output.splitlines() if "Peak Memory Usage:" in line), None
        )

        print(f"Command executed successfully for config: {config}")
        return file_1_size, file_2_size, total_size, time_line, memory_line

    except subprocess.CalledProcessError as e:
        print(f"An error occurred while executing the command for config: {config}")
        print(e)
    return []


def validate_result(config, failed_list):
    result_file = f"./result_{config}.bin"
    expected_file = f"../tests/TestData/merge_test/Original_{config}.bin"

    with open(result_file, "rb") as rf, open(expected_file, "rb") as ef:
        result_data = rf.read()
        expected_data = ef.read()

        if result_data == expected_data:
            print(f"Validation successful for config: {config}")
        else:
            failed_list.append(config)
            print(f"Validation failed for config: {config}")


if __name__ == "__main__":
    configs = [
        "1DxT",
        "1DxP",
        "1DxF",
        "2DxF",
        "2DxP",
        "3DxF",
        "3DxP",
        "4DxF",
        "4DxP",
        "4DxPc",
    ]  # Add your configurations here
    failed_list = []
    for config in configs:
        print(f"Running test for config: {config}")
        file_1_size, file_2_size, total_size, time, memory = run_result_merge(config)
        print(f"Validating result for config: {config}")
        validate_result(config, failed_list)
        print(f"Test completed for config: {config}")
        with open("results.json", "a") as json_file:
            json_file.write(
                f'{{"config": "{config}", "file_1_size": "{file_1_size}", "file_2_size": "{file_2_size}", "total_size": "{total_size}", "time": "{time}", "memory": "{memory}"}}\n'
            )
    print(f"Failed configurations: {failed_list}")
