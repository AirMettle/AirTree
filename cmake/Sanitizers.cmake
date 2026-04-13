include_guard(GLOBAL)

# Resolve sanitizer flags from AIRTREE_SANITIZER option
set(AIRTREE_SANITIZER_FLAGS "")
set(AIRTREE_SANITIZER_USES_ASAN OFF)

if(AIRTREE_SANITIZER STREQUAL "asan")
    set(AIRTREE_SANITIZER_FLAGS "-fsanitize=address")
    set(AIRTREE_SANITIZER_USES_ASAN ON)
elseif(AIRTREE_SANITIZER STREQUAL "ubsan")
    set(AIRTREE_SANITIZER_FLAGS "-fsanitize=undefined")
elseif(AIRTREE_SANITIZER STREQUAL "asan_ubsan")
    set(AIRTREE_SANITIZER_FLAGS "-fsanitize=address,undefined")
    set(AIRTREE_SANITIZER_USES_ASAN ON)
elseif(NOT AIRTREE_SANITIZER STREQUAL "nosan")
    message(FATAL_ERROR "Unknown AIRTREE_SANITIZER value: '${AIRTREE_SANITIZER}'. Use: nosan, asan, ubsan, asan_ubsan")
endif()

if(AIRTREE_SANITIZER_FLAGS)
    message(STATUS "Sanitizer enabled: ${AIRTREE_SANITIZER} (flags: ${AIRTREE_SANITIZER_FLAGS})")

    add_compile_options(${AIRTREE_SANITIZER_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls)
    add_link_options(${AIRTREE_SANITIZER_FLAGS})

    # UBSan: abort on undefined behavior instead of silently continuing
    if(AIRTREE_SANITIZER STREQUAL "ubsan" OR AIRTREE_SANITIZER STREQUAL "asan_ubsan")
        add_compile_options(-fno-sanitize-recover=undefined)
        add_compile_definitions(UBSAN_ENABLED)
    endif()

    if(AIRTREE_SANITIZER_USES_ASAN)
        add_compile_definitions(ASAN_ENABLED)
        message(STATUS "ASan active — mimalloc will be disabled (conflicts with ASan's malloc interception)")
    endif()
endif()
