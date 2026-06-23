#!/bin/bash

set -euo pipefail

BENCH_DATA_DIR="/home/csanders/AirTree/cmake-build-gnu-11-RelWithDebInfo-nosan/bench_data"

echo "========================================"
echo "Cleaning up gdown and bench_data"
echo "========================================"

# -------------------------
# 1. Uninstall gdown
# -------------------------
echo ""
echo "Attempting to uninstall gdown..."

# Try uv first (most modern)
if command -v uv >/dev/null 2>&1; then
    if uv tool list | grep -q gdown; then
        echo "→ Uninstalling gdown with uv..."
        uv tool uninstall gdown || true
    fi
fi

# Try pipx
if command -v pipx >/dev/null 2>&1; then
    if pipx list | grep -q gdown; then
        echo "→ Uninstalling gdown with pipx..."
        pipx uninstall gdown || true
    fi
fi

# Try pip (user install)
if command -v python3 >/dev/null 2>&1; then
    if python3 -m pip show gdown >/dev/null 2>&1; then
        echo "→ Uninstalling gdown with pip (user)..."
        python3 -m pip uninstall -y gdown || true
    fi
elif command -v python >/dev/null 2>&1; then
    if python -m pip show gdown >/dev/null 2>&1; then
        echo "→ Uninstalling gdown with pip (user)..."
        python -m pip uninstall -y gdown || true
    fi
fi

# Final check
if command -v gdown >/dev/null 2>&1; then
    echo "⚠️  Warning: gdown is still installed or available in PATH"
else
    echo "✓ gdown has been uninstalled (or was not installed)"
fi

# -------------------------
# 2. Remove bench_data directory
# -------------------------
echo ""
if [[ -d "$BENCH_DATA_DIR" ]]; then
    echo "Removing directory: $BENCH_DATA_DIR"
    rm -rf "$BENCH_DATA_DIR"
    echo "✓ Directory deleted"
else
    echo "Directory does not exist: $BENCH_DATA_DIR"
fi

echo ""
echo "Cleanup complete."
