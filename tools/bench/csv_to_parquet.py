#!/usr/bin/env python3
"""
Convert all CSV files in a directory to Parquet format,
skipping FRED-MD metadata rows (factors and transform) after the header.
"""

import sys
import pandas as pd
from pathlib import Path

def convert_csv_to_parquet(directory_path: str):
    dir_path = Path(directory_path)
    
    if not dir_path.exists() or not dir_path.is_dir():
        print(f"Error: Directory '{directory_path}' does not exist or is not a folder.")
        return
    
    # Find all .csv files
    csv_files = list(dir_path.glob("*.csv"))
    
    if not csv_files:
        print("No .csv files found in the directory.")
        return
    
    print(f"Found {len(csv_files)} CSV file(s). Starting conversion...\n")
    
    for csv_file in csv_files:
        parquet_file = csv_file.with_suffix('.parquet')
        
        try:
            # FRED-MD layout: header, then factors row (index 1), transform row (index 2), then data
            df = pd.read_csv(csv_file, skiprows=[1, 2], parse_dates=['sasdate'])
            
            # Save as Parquet
            df.to_parquet(parquet_file, 
                         compression='snappy', 
                         engine='pyarrow',
                         index=False)
            
            print(f"Converted: {csv_file.name} → {parquet_file.name}  ({len(df)} rows, {len(df.columns)} columns)")
            
        except Exception as e:
            print(f"Failed to convert {csv_file.name}: {e}")
    
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python convert_to_parquet.py <directory_path>")
        sys.exit(1)
    
    convert_csv_to_parquet(sys.argv[1])