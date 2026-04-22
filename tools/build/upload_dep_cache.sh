#!/usr/bin/env bash

# Upload locally-built dependency artifacts to S3 binary cache.
#
# Finds all install directories containing a .dep_cache_hash marker file,
# creates tarballs, and uploads them with a manifest to S3.
# Existing artifacts are skipped (idempotent).
#
# All variables are passed automatically by the CMake upload_dep_cache target:
#   ninja upload_dep_cache
#   cmake --build <build-dir> --target upload_dep_cache
#
# Can also be run standalone (env vars override defaults):
#   AIRTREE_DEPS_CACHE_S3_BUCKET=s3://my-bucket ./tools/build/upload_dep_cache.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"; ROOT_DIR="${ROOT_DIR%/tools*}"
. "$ROOT_DIR/tools/utils/import.sh"
import utils/common_func.sh
import utils/build_utils.sh

_CMAKE_SOURCE_DIR="$CMAKE_SOURCE_DIR"
_CMAKE_BUILD_DIR="$CMAKE_BUILD_DIR"

# Defaults (matched to cmake/Settings.cmake)
AIRTREE_DEPS_CACHE_S3_BUCKET="${AIRTREE_DEPS_CACHE_S3_BUCKET:-s3://airmettle-airtree}"
AIRTREE_DEPS_CACHE_S3_REGION="${AIRTREE_DEPS_CACHE_S3_REGION:-us-east-2}"
AIRTREE_DEPS_CACHE_S3_PREFIX="${AIRTREE_DEPS_CACHE_S3_PREFIX:-deps/v1}"
AIRMETTLE_AIRTREE_DEPENDENCY_ROOT="${AIRMETTLE_AIRTREE_DEPENDENCY_ROOT:-${_CMAKE_BUILD_DIR}/.airmettle/airtree-deps}"

if ! command -v aws &>/dev/null; then
    echo "ERROR: aws CLI not found in PATH." >&2
    exit 1
fi

if [[ ! -d "$AIRMETTLE_AIRTREE_DEPENDENCY_ROOT" ]]; then
    echo "ERROR: Dependency root not found: $AIRMETTLE_AIRTREE_DEPENDENCY_ROOT" >&2
    exit 1
fi

uploaded=0
skipped=0
failed=0

# Find all .dep_cache_hash marker files
while IFS= read -r hash_file; do
    install_dir="$(dirname "$hash_file")"
    file_content="$(cat "$hash_file")"

    # File format: "dep_name hash" (space-separated)
    # Fall back to directory-based name if file only contains hash (legacy format)
    if [[ "$file_content" == *" "* ]]; then
        dep_name="${file_content%% *}"
        dep_hash="${file_content#* }"
    else
        dep_hash="$file_content"
        ep_dir="$(dirname "$install_dir")"
        ep_name="$(basename "$ep_dir")"
        dep_name="${ep_name%_ep}"
        dep_name="${dep_name%_fc}"
    fi

    if [[ -z "$dep_hash" ]]; then
        echo "WARNING: Empty hash file at $hash_file — skipping"
        continue
    fi

    s3_prefix="${AIRTREE_DEPS_CACHE_S3_PREFIX}/${dep_name}/${dep_hash}"
    s3_manifest="${AIRTREE_DEPS_CACHE_S3_BUCKET}/${s3_prefix}.manifest.json"
    s3_tarball="${AIRTREE_DEPS_CACHE_S3_BUCKET}/${s3_prefix}.tar.gz"

    # Check if manifest already exists (idempotent)
    if aws s3 ls "$s3_manifest" --region "$AIRTREE_DEPS_CACHE_S3_REGION" &>/dev/null; then
        echo "SKIP: ${dep_name} (${dep_hash:0:12}...) — already cached"
        skipped=$((skipped + 1))
        continue
    fi

    echo "UPLOAD: ${dep_name} (${dep_hash:0:12}...)"

    # Create tarball of install directory contents
    tmp_dir="$(mktemp -d)"
    tarball_path="${tmp_dir}/${dep_hash}.tar.gz"

    if ! tar -czf "$tarball_path" -C "$install_dir" .; then
        echo "WARNING: Failed to create tarball for ${dep_name} — skipping"
        rm -rf "$tmp_dir"
        failed=$((failed + 1))
        continue
    fi

    # Compute SHA256 of tarball
    tarball_sha256="$(sha256sum "$tarball_path" | awk '{print $1}')"
    tarball_size="$(stat --printf='%s' "$tarball_path" 2>/dev/null || stat -f '%z' "$tarball_path" 2>/dev/null)"

    # Generate manifest
    manifest_path="${tmp_dir}/manifest.json"
    cat > "$manifest_path" <<MANIFEST_EOF
{
    "schema_version": 1,
    "dep_name": "${dep_name}",
    "hash": "${dep_hash}",
    "tarball": "${dep_hash}.tar.gz",
    "tarball_sha256": "${tarball_sha256}",
    "tarball_size_bytes": ${tarball_size},
    "created_at": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "created_by": "$(hostname)"
}
MANIFEST_EOF

    # Upload tarball first, then manifest (manifest-last = atomic publish)
    if ! aws s3 cp "$tarball_path" "$s3_tarball" \
            --region "$AIRTREE_DEPS_CACHE_S3_REGION" --quiet; then
        echo "WARNING: Failed to upload tarball for ${dep_name} — skipping"
        rm -rf "$tmp_dir"
        failed=$((failed + 1))
        continue
    fi

    if ! aws s3 cp "$manifest_path" "$s3_manifest" \
            --region "$AIRTREE_DEPS_CACHE_S3_REGION" --quiet; then
        echo "WARNING: Failed to upload manifest for ${dep_name} — skipping"
        rm -rf "$tmp_dir"
        failed=$((failed + 1))
        continue
    fi

    rm -rf "$tmp_dir"
    uploaded=$((uploaded + 1))
    echo "  OK: ${dep_name} uploaded (${tarball_size} bytes, sha256:${tarball_sha256:0:12}...)"

done < <(find "$AIRMETTLE_AIRTREE_DEPENDENCY_ROOT" -name ".dep_cache_hash" -type f 2>/dev/null)

echo ""
echo "Upload complete: ${uploaded} uploaded, ${skipped} already cached, ${failed} failed"
