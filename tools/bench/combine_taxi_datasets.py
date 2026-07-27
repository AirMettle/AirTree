import pandas as pd
import glob
import sys
import os

if len(sys.argv) < 2:
    print("Usage: python combine_parquet.py <folder_path>")
    print("Example: python combine_parquet.py 'nyc_taxi_2025/'")
    sys.exit(1)

folder = sys.argv[1]

# Only match yellow_tripdata_2025_*.parquet files
pattern = f"{folder}/yellow_tripdata_2025-*.parquet"

files = sorted(glob.glob(pattern))

if not files:
    print(f"No files matching 'yellow_tripdata_2025_*.parquet' found in: {folder}")
    sys.exit(1)

print(f"Found {len(files)} files to combine...")

df_list = []
for f in files:
    print(f"Reading {f}...")
    df_list.append(pd.read_parquet(f))

combined = pd.concat(df_list, ignore_index=True)

# Save without compression 
output_file = os.path.join(folder, "yellow_tripdata_2025_combined.parquet")
combined.to_parquet(output_file, compression=None)

for f in files:
    os.remove(f)

print(f"Combined {len(files)} files → {len(combined):,} rows")
print(f"Output saved as: {output_file}")
print(f"Total size: {os.path.getsize(output_file) / (1024*1024*1024):.2f} GB")
