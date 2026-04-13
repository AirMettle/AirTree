include_guard(GLOBAL)

# DepsCache.cmake — Per-dependency content-addressed binary caching backed by S3.
#
# Public functions:
#   airtree_dep_compute_hash()  — compute DAG-aware hash for a dependency
#   airtree_dep_try_cache()     — all-in-one: compute hash, resolve install dir, try S3 download
#
# Install directories are keyed by hash:
#   {dep}_ep/{hash}/        (e.g. zlib_ep/44bc2f8e.../lib/, .../include/)
#
# Multiple versions coexist on disk. Switching branches/versions is instant
# if the hash was previously built or downloaded.
#
# Requires DepsRegistry.cmake to be included first.

# ============================================================================
# Argument validation helper
# ============================================================================
macro(_airtree_check_required_args _func_name)
    foreach(_arg IN ITEMS ${ARGN})
        if(NOT DEFINED ARG_${_arg} OR "${ARG_${_arg}}" STREQUAL "")
            message(FATAL_ERROR "[DepsCache] ${_func_name}(): missing required argument ${_arg}")
        endif()
    endforeach()
endmacro()

# ============================================================================
# Configure-time checks
# ============================================================================

set(_AIRTREE_DEPS_CACHE_AVAILABLE FALSE)
set(_AIRTREE_DEPS_CACHE_COUNTER 0 CACHE INTERNAL "")

if(AIRTREE_DEPS_CACHE_ENABLED)
    if(NOT AIRTREE_DEPS_CACHE_S3_BUCKET)
        message(WARNING "[DepsCache] AIRTREE_DEPS_CACHE_S3_BUCKET is empty. Disabling dependency cache.")
        set(AIRTREE_DEPS_CACHE_ENABLED OFF CACHE BOOL "" FORCE)
    else()
        find_program(_AIRTREE_AWS_CLI aws)
        if(NOT _AIRTREE_AWS_CLI)
            message(WARNING "[DepsCache] aws CLI not found. Disabling dependency cache.")
            set(AIRTREE_DEPS_CACHE_ENABLED OFF CACHE BOOL "" FORCE)
        else()
            # Quick S3 connectivity check (30s timeout)
            execute_process(
                COMMAND ${_AIRTREE_AWS_CLI} s3 ls "${AIRTREE_DEPS_CACHE_S3_BUCKET}/${AIRTREE_DEPS_CACHE_S3_PREFIX}/"
                        --region "${AIRTREE_DEPS_CACHE_S3_REGION}"
                RESULT_VARIABLE _s3_check_result
                OUTPUT_QUIET
                ERROR_VARIABLE _s3_check_error
                TIMEOUT 30
            )
            if(NOT _s3_check_result EQUAL 0)
                message(WARNING "[DepsCache] Cannot reach S3 bucket "
                    "${AIRTREE_DEPS_CACHE_S3_BUCKET} (exit code: ${_s3_check_result}). "
                    "Disabling dependency cache for this run.\n"
                    "AWS CLI error: ${_s3_check_error}")
                set(AIRTREE_DEPS_CACHE_ENABLED OFF CACHE BOOL "" FORCE)
            else()
                set(_AIRTREE_DEPS_CACHE_AVAILABLE TRUE)
                message(STATUS "[DepsCache] S3 cache enabled: "
                    "${AIRTREE_DEPS_CACHE_S3_BUCKET}/${AIRTREE_DEPS_CACHE_S3_PREFIX}/")
            endif()
        endif()
    endif()
endif()

# ============================================================================
# airtree_dep_compute_hash()
# ============================================================================
# Compute the DAG-aware hash for a dependency.
#
# Usage:
#   airtree_dep_compute_hash(
#     DEP_NAME     zlib
#     DEP_VERSION  "${ZLIB_VERSION}"
#     RECIPE_FILE  "${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_zlib.cmake"
#     DEPENDS      ""
#     OUT_VAR      _hash
#   )
#
function(airtree_dep_compute_hash)
    cmake_parse_arguments(ARG "" "DEP_NAME;DEP_VERSION;RECIPE_FILE;OUT_VAR" "DEPENDS" ${ARGN})
    _airtree_check_required_args("airtree_dep_compute_hash" DEP_NAME DEP_VERSION RECIPE_FILE OUT_VAR)

    # Hash the recipe file
    file(SHA256 "${ARG_RECIPE_FILE}" _recipe_hash)

    # Collect transitive dependency hashes (already computed and stored in GLOBAL properties)
    set(_dep_hashes "")
    if(ARG_DEPENDS)
        list(SORT ARG_DEPENDS)
        foreach(_dep IN LISTS ARG_DEPENDS)
            get_property(_dep_hash GLOBAL PROPERTY "AIRTREE_DEP_HASH_${_dep}")
            if(NOT _dep_hash)
                message(FATAL_ERROR
                    "[DepsCache] Dependency hash for '${_dep}' not found. "
                    "Ensure ${_dep} is processed before ${ARG_DEP_NAME} in CMakeLists.txt.")
            endif()
            list(APPEND _dep_hashes "${_dep}:${_dep_hash}")
        endforeach()
    endif()
    list(JOIN _dep_hashes "|" _dep_hashes_str)

    # Build canonical hash input string
    set(_hash_input "")
    string(APPEND _hash_input "name:${ARG_DEP_NAME}\n")
    string(APPEND _hash_input "version:${ARG_DEP_VERSION}\n")
    string(APPEND _hash_input "compiler:${_COMPILER_ID}${_COMPILER_MAJOR}\n")
    string(APPEND _hash_input "arch:${_ARCH}\n")
    string(APPEND _hash_input "os:${_DISTRO_NAME}${_DISTRO_MAJOR}\n")
    string(APPEND _hash_input "build_type:${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}\n")
    string(APPEND _hash_input "shared_libs:${AIRMETTLE_AIRTREE_USE_SHARED_LIBS}\n")
    string(APPEND _hash_input "sanitizer:${AIRTREE_SANITIZER}\n")
    string(APPEND _hash_input "cxx_flags:${CMAKE_CXX_FLAGS}\n")
    string(APPEND _hash_input "c_flags:${CMAKE_C_FLAGS}\n")
    string(APPEND _hash_input "recipe:${_recipe_hash}\n")
    string(APPEND _hash_input "deps:${_dep_hashes_str}\n")

    # Compute final hash
    string(SHA256 _final_hash "${_hash_input}")

    # Store globally for downstream deps to reference
    set_property(GLOBAL PROPERTY "AIRTREE_DEP_HASH_${ARG_DEP_NAME}" "${_final_hash}")

    set(${ARG_OUT_VAR} "${_final_hash}" PARENT_SCOPE)

    message(STATUS "[DepsCache] ${ARG_DEP_NAME} hash: ${_final_hash}")
endfunction()

# ============================================================================
# airtree_dep_cache_download()
# ============================================================================
# Download and extract a cached artifact from S3.
# Returns: sets ${OUT_VAR} to TRUE/FALSE in parent scope.
#
function(airtree_dep_cache_download)
    cmake_parse_arguments(ARG "" "DEP_NAME;HASH;INSTALL_DIR;OUT_VAR" "" ${ARGN})
    _airtree_check_required_args("airtree_dep_cache_download" DEP_NAME HASH INSTALL_DIR OUT_VAR)

    set(${ARG_OUT_VAR} FALSE PARENT_SCOPE)

    set(_s3_prefix "${AIRTREE_DEPS_CACHE_S3_PREFIX}/${ARG_DEP_NAME}/${ARG_HASH}")
    set(_s3_manifest "${AIRTREE_DEPS_CACHE_S3_BUCKET}/${_s3_prefix}.manifest.json")
    set(_s3_tarball  "${AIRTREE_DEPS_CACHE_S3_BUCKET}/${_s3_prefix}.tar.gz")

    set(_tmp_dir "${ARG_INSTALL_DIR}/../_cache_tmp_${ARG_DEP_NAME}_${ARG_HASH}")
    file(REMOVE_RECURSE "${_tmp_dir}")
    file(MAKE_DIRECTORY "${_tmp_dir}")

    # Download manifest (failure is a normal cache miss, not an error)
    execute_process(
        COMMAND ${_AIRTREE_AWS_CLI} s3 cp "${_s3_manifest}" "${_tmp_dir}/manifest.json"
                --region "${AIRTREE_DEPS_CACHE_S3_REGION}"
        RESULT_VARIABLE _dl_result
        OUTPUT_QUIET ERROR_VARIABLE _s3_error
        TIMEOUT 30
    )
    if(NOT _dl_result EQUAL 0)
        message(STATUS "[DepsCache] ${ARG_DEP_NAME}: S3 MISS (no manifest)")
        file(REMOVE_RECURSE "${_tmp_dir}")
        return()
    endif()

    # Parse expected SHA256 and size from manifest
    file(READ "${_tmp_dir}/manifest.json" _manifest_content)
    string(JSON _expected_sha256 GET "${_manifest_content}" "tarball_sha256")
    string(JSON _tarball_size ERROR_VARIABLE _size_err GET "${_manifest_content}" "tarball_size_bytes")
    if(_tarball_size AND NOT _size_err)
        math(EXPR _size_mb "(${_tarball_size} + 524288) / 1048576")  # round to nearest MB
        if(_size_mb LESS 1)
            math(EXPR _size_kb "(${_tarball_size} + 512) / 1024")
            message(STATUS "[DepsCache] ${ARG_DEP_NAME}: downloading ${_size_kb} KB from S3...")
        else()
            message(STATUS "[DepsCache] ${ARG_DEP_NAME}: downloading ${_size_mb} MB from S3...")
        endif()
    endif()

    # Download tarball (manifest exists so tarball should too — failure is unexpected)
    execute_process(
        COMMAND ${_AIRTREE_AWS_CLI} s3 cp "${_s3_tarball}" "${_tmp_dir}/artifact.tar.gz"
                --region "${AIRTREE_DEPS_CACHE_S3_REGION}"
        RESULT_VARIABLE _dl_result
        OUTPUT_QUIET ERROR_VARIABLE _s3_error
        TIMEOUT 300
    )
    if(NOT _dl_result EQUAL 0)
        message(WARNING "[DepsCache] Failed to download tarball for ${ARG_DEP_NAME}: ${_s3_error}")
        file(REMOVE_RECURSE "${_tmp_dir}")
        return()
    endif()

    # Verify SHA256
    file(SHA256 "${_tmp_dir}/artifact.tar.gz" _actual_sha256)
    if(NOT "${_actual_sha256}" STREQUAL "${_expected_sha256}")
        message(WARNING "[DepsCache] SHA256 MISMATCH for ${ARG_DEP_NAME}! "
            "Expected: ${_expected_sha256}, Got: ${_actual_sha256}. Discarding.")
        file(REMOVE_RECURSE "${_tmp_dir}")
        return()
    endif()

    # Extract to a staging dir, then atomically rename to final install dir.
    # This prevents partial installs from interrupted extraction.
    set(_staging_dir "${ARG_INSTALL_DIR}_extracting")
    file(REMOVE_RECURSE "${_staging_dir}")
    file(MAKE_DIRECTORY "${_staging_dir}")

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf "${_tmp_dir}/artifact.tar.gz"
        WORKING_DIRECTORY "${_staging_dir}"
        RESULT_VARIABLE _extract_result
        ERROR_VARIABLE _extract_error
    )
    if(NOT _extract_result EQUAL 0)
        message(WARNING "[DepsCache] Failed to extract tarball for ${ARG_DEP_NAME}: ${_extract_error}")
        file(REMOVE_RECURSE "${_tmp_dir}" "${_staging_dir}")
        return()
    endif()

    # Write marker file into staging dir before the atomic rename
    file(WRITE "${_staging_dir}/.dep_cache_hash" "${ARG_DEP_NAME} ${ARG_HASH}")

    # Atomic rename (same filesystem = same parent dir)
    file(RENAME "${_staging_dir}" "${ARG_INSTALL_DIR}")

    # Cleanup temp
    file(REMOVE_RECURSE "${_tmp_dir}")

    set(${ARG_OUT_VAR} TRUE PARENT_SCOPE)
endfunction()

# ============================================================================
# airtree_dep_try_cache()
# ============================================================================
# All-in-one: compute hash, resolve hash-keyed install dir, check local, try S3.
#
# The install directory is keyed by hash: {EP_DIR}/{hash}/
# Multiple versions coexist on disk — no deletion on hash mismatch.
#
# Usage:
#   airtree_dep_try_cache(
#     DEP_NAME     zlib
#     EP_DIR       "${_DEPS_DIR}/${_EP_BASE}"
#     CACHE_HIT    _cache_hit       # output: TRUE if ready to use (local or S3)
#     INSTALL_DIR  _install_dir     # output: hash-keyed install path
#   )
#
function(airtree_dep_try_cache)
    cmake_parse_arguments(ARG "" "DEP_NAME;EP_DIR;CACHE_HIT;INSTALL_DIR" "" ${ARGN})
    _airtree_check_required_args("airtree_dep_try_cache" DEP_NAME EP_DIR CACHE_HIT INSTALL_DIR)

    # Progress counter
    math(EXPR _counter "${_AIRTREE_DEPS_CACHE_COUNTER} + 1")
    set(_AIRTREE_DEPS_CACHE_COUNTER ${_counter} CACHE INTERNAL "" FORCE)
    get_property(_total GLOBAL PROPERTY AIRTREE_DEP_TOTAL_COUNT)

    # Look up version and build-time deps from registry
    get_property(_version_var GLOBAL PROPERTY "AIRTREE_DEP_VERSION_VAR_${ARG_DEP_NAME}")
    get_property(_build_deps  GLOBAL PROPERTY "AIRTREE_DEP_DEPS_${ARG_DEP_NAME}")

    set(_version "${${_version_var}}")

    # Compute hash (always — even when cache is OFF, to determine install dir)
    airtree_dep_compute_hash(
        DEP_NAME     "${ARG_DEP_NAME}"
        DEP_VERSION  "${_version}"
        RECIPE_FILE  "${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_${ARG_DEP_NAME}.cmake"
        DEPENDS      ${_build_deps}
        OUT_VAR      _hash
    )

    # Hash-keyed install directory
    set(_install_dir "${ARG_EP_DIR}/${_hash}")
    set(${ARG_INSTALL_DIR} "${_install_dir}" PARENT_SCOPE)

    # Store globally so downstream deps can resolve cross-dep paths
    set_property(GLOBAL PROPERTY "AIRTREE_DEP_INSTALL_DIR_${ARG_DEP_NAME}" "${_install_dir}")

    # Store DEP_NAME keyed by EP directory name so airtree_dep_mark_built() can look it up
    get_filename_component(_ep_dir_name "${ARG_EP_DIR}" NAME)
    set_property(GLOBAL PROPERTY "AIRTREE_DEP_NAME_FOR_EP_${_ep_dir_name}" "${ARG_DEP_NAME}")

    # Check if a COMPLETE local install exists for this hash.
    # .dep_cache_hash is the completion marker — written only after:
    #   1. A successful S3 download (by airtree_dep_cache_download)
    #   2. A successful source build (by airtree_dep_write_hash_marker target)
    # The directory may exist without this file if a prior configure created
    # it via file(MAKE_DIRECTORY) but the build hasn't completed yet.
    set(_prefix "[DepsCache] (${_counter}/${_total})")

    if(EXISTS "${_install_dir}/.dep_cache_hash")
        message(STATUS "${_prefix} ${ARG_DEP_NAME}: local hit")
        set(${ARG_CACHE_HIT} TRUE PARENT_SCOPE)
        return()
    endif()

    # No local install for this hash — try S3 cache (no separate existence
    # check; airtree_dep_cache_download handles missing manifests directly,
    # eliminating a TOCTOU race and saving an S3 request on cache hits).
    if(_AIRTREE_DEPS_CACHE_AVAILABLE)
        message(STATUS "${_prefix} ${ARG_DEP_NAME}: checking S3...")
        airtree_dep_cache_download(
            DEP_NAME    "${ARG_DEP_NAME}"
            HASH        "${_hash}"
            INSTALL_DIR "${_install_dir}"
            OUT_VAR     _download_ok
        )
        if(_download_ok)
            message(STATUS "${_prefix} ${ARG_DEP_NAME}: restored from S3")
            set(${ARG_CACHE_HIT} TRUE PARENT_SCOPE)
            return()
        endif()
        # Download failed or manifest not found — fall through to source build
    endif()

    set(${ARG_CACHE_HIT} FALSE PARENT_SCOPE)
endfunction()

# ============================================================================
# airtree_dep_mark_built()
# ============================================================================
# Write the .dep_cache_hash marker after a source build completes.
# Call this in the source-build branch of each Dependency_*.cmake.
#
# Usage:
#   airtree_dep_mark_built(EP_TARGET zlib_ep INSTALL_DIR "${_INSTALL_DIR}")
#
function(airtree_dep_mark_built)
    cmake_parse_arguments(ARG "" "EP_TARGET;INSTALL_DIR" "" ${ARGN})
    _airtree_check_required_args("airtree_dep_mark_built" EP_TARGET INSTALL_DIR)

    get_filename_component(_hash "${ARG_INSTALL_DIR}" NAME)

    # Look up the canonical dep name (e.g. googlebench_ep → gbenchmark)
    get_property(_dep_name GLOBAL PROPERTY "AIRTREE_DEP_NAME_FOR_EP_${ARG_EP_TARGET}")
    if(NOT _dep_name)
        message(FATAL_ERROR
            "[DepsCache] airtree_dep_mark_built(): no dep name registered for EP target '${ARG_EP_TARGET}'. "
            "Ensure airtree_dep_try_cache() was called before airtree_dep_mark_built().")
    endif()

    # Write "dep_name hash" to a temp file at configure time (in the build dir).
    # At build time we copy it — avoids all shell quoting issues with sh -c.
    set(_hash_file "${CMAKE_CURRENT_BINARY_DIR}/_dep_cache_hashes/${ARG_EP_TARGET}.hash")
    file(WRITE "${_hash_file}" "${_dep_name} ${_hash}")

    # Create a target that copies .dep_cache_hash after the EP build completes.
    # This marker tells airtree_dep_try_cache that the install is complete.
    add_custom_target(${ARG_EP_TARGET}_cache_marker ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ARG_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "${_hash_file}" "${ARG_INSTALL_DIR}/.dep_cache_hash"
        COMMENT "Writing cache marker for ${ARG_EP_TARGET}"
    )
    add_dependencies(${ARG_EP_TARGET}_cache_marker ${ARG_EP_TARGET})
endfunction()

# ============================================================================
# upload_dep_cache target
# ============================================================================
# Usage: cmake --build <build-dir> --target upload_dep_cache
#   or:  ninja upload_dep_cache
#
add_custom_target(upload_dep_cache
    COMMAND ${CMAKE_COMMAND} -E env
        "AIRTREE_DEPS_CACHE_S3_BUCKET=${AIRTREE_DEPS_CACHE_S3_BUCKET}"
        "AIRTREE_DEPS_CACHE_S3_REGION=${AIRTREE_DEPS_CACHE_S3_REGION}"
        "AIRTREE_DEPS_CACHE_S3_PREFIX=${AIRTREE_DEPS_CACHE_S3_PREFIX}"
        "AIRMETTLE_AIRTREE_DEPENDENCY_ROOT=${AIRMETTLE_AIRTREE_DEPENDENCY_ROOT}"
        bash "${CMAKE_SOURCE_DIR}/tools/build/upload_dep_cache.sh"
    COMMENT "Uploading dependency cache to S3..."
    VERBATIM
)
