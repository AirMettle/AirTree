#!/usr/bin/env bash
set -euo pipefail

# Find repo root
ROOT=$(git rev-parse --show-toplevel)

# Locate the actual Git directory (handles worktrees too)
GIT_DIR=$(git rev-parse --git-dir)
# If it's a relative path (e.g. “.git”), make it absolute
if [[ "$GIT_DIR" != /* ]]; then
  GIT_DIR="$ROOT/$GIT_DIR"
fi

HOOKS_SRC="$ROOT/scripts/hooks"
HOOKS_DST="$GIT_DIR/hooks"


if [[ ! -d "$HOOKS_SRC" ]]; then
  echo "ERROR: hooks source dir not found at $HOOKS_SRC" >&2
  exit 1
fi
mkdir -p "$HOOKS_DST"

echo "Installing Git hooks from $HOOKS_SRC → $HOOKS_DST"
for hook in "$HOOKS_SRC"/*; do
  name=$(basename "$hook")
  ln -sf "$hook" "$HOOKS_DST/$name"
  chmod +x "$hook"
  echo "  ↳ $name → $HOOKS_DST/$name"
done

echo "Done. Your shared hooks are now active."
