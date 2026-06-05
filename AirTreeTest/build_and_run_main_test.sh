#!/bin/bash
set -e

VERSION="1.3.0"
AIRTREE=/opt/airmettle/airtree/$VERSION
DEPS=/home/csanders/AirTree/cmake-build-gnu-11-RelWithDebInfo-nosan/.airmettle/airtree-deps/ubuntu22-x86_64-gnu11-release

ARROW=$DEPS/arrow_ep/1ba46cd284ba9b7dd749c6b68e13665f1af634b239ef715aacadc019d6462dff
BOOST=$DEPS/boost_ep/bf5a83fe8383ce9b809b0c3f4670a16417ab4a7e49aa3d02cdd5da946d271fea
LZ4=$DEPS/lz4_ep/e3ce910b14f9079eb8426ba6d5a1187ff649f824bedd31e0950ba887ca0f09b8
OPENSSL=$DEPS/openssl_ep/a7088b04ed6495b2b47535d5538a546d321b37ee9bd62c667492d47bfc480a3d
ZLIB=$DEPS/zlib_ep/39ed73e409fc24956f6a6a4dfc8d7e710eeb89d6de948b94c801f86af0c65ced
ZSTD=$DEPS/zstd_ep/6b45a4a60e6c261405110ca4db2ae42df90b3a7e5a180e171aa21b6a1aec55db
SPDLOG=$DEPS/spdlog_ep/00e28f52542dbfbc6f8fd7731875de36ea06772f7e5069661c6286b5b635a06e

g++ -std=c++17 main_test.cpp \
    -I$AIRTREE/include \
    -I$SPDLOG/include \
    -L$AIRTREE/lib \
    -L$ARROW/lib \
    -L$BOOST/lib \
    -L$LZ4/lib \
    -L$OPENSSL/lib \
    -L$ZLIB/lib \
    -L$ZSTD/lib \
    -L$SPDLOG/lib \
    -lairtree-query -lairtree-core -lairtree-util \
    -larrow -lparquet -larrow_bundled_dependencies \
    -lboost_system -lboost_filesystem \
    -lspdlog -lssl -lcrypto -llz4 -lz -lzstd \
    -lpthread -ldl -lrt -lm \
    -o main_test

echo "Build OK"
./main_test
