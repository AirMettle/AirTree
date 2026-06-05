#!/bin/bash
set -e

# ========================= CONFIGURATION =========================
AIRTREE_SRC=/home/csanders/AirTree

# Core and Query source include directories
CORE_INCLUDE=$AIRTREE_SRC/airtree-core/include
QUERY_INCLUDE=$AIRTREE_SRC/airtree-query/include
UTIL_INC=$AIRTREE_SRC/airtree-util/include/gen-src/include/airtree/util/logging/
UTIL_INC2=$AIRTREE_SRC/airtree-util/include
INCLUDE_DIR=$AIRTREE_SRC/cmake-build-gnu-11-RelWithDebInfo-nosan/_CPack_Packages/Linux/TGZ/airtree-1.3.0-Linux/include
LIB_DIR=$AIRTREE_SRC/cmake-build-gnu-11-RelWithDebInfo-nosan/_CPack_Packages/Linux/TGZ/airtree-1.3.0-Linux/lib
# spdlog (from your deps)
DEPS=/home/csanders/AirTree/cmake-build-gnu-11-RelWithDebInfo-nosan/.airmettle/airtree-deps/ubuntu22-x86_64-gnu11-release
SPDLOG=$AIRTREE_SRC/cmake-build-gnu-11-RelWithDebInfo-nosan/.airmettle/airtree-deps/ubuntu22-x86_64-gnu11-release/spdlog_ep/00e28f52542dbfbc6f8fd7731875de36ea06772f7e5069661c6286b5b635a06e/include
SPDLOG_LIB=$AIRTREE_SRC/cmake-build-gnu-11-RelWithDebInfo-nosan/.airmettle/airtree-deps/ubuntu22-x86_64-gnu11-release/spdlog_ep/00e28f52542dbfbc6f8fd7731875de36ea06772f7e5069661c6286b5b635a06e/lib
echo "Using CORE_INCLUDE  : $CORE_INCLUDE"
echo "Using QUERY_INCLUDE : $QUERY_INCLUDE"
echo "Using SPDLOG        : $SPDLOG"


# ========================= BUILD =========================
echo "Building test_top_k.cpp ..."

g++ -std=c++17 test_top_k.cpp \
    -I$INCLUDE_DIR \
    -I$SPDLOG \
    -L$LIB_DIR \
    -L$SPDLOG_LIB \
    -lairtree-query \
    -lspdlog \
    -lpthread -ldl -lrt -lm \
    -o test_top_k

echo "Build successful!"
echo "Running test..."
./test_top_k