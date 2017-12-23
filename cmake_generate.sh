#!/usr/bin/env bash
# cmake_generate.sh <build-dir> <Debug|Release>

set -x
set -e

BUILD_DIR=${1}
[ -z "$BUILD_DIR" ] && (echo "need to specify BUILD_DIR as first arg"  && exit 1)
shift

BUILD_TYPE=${1}
[ -z "$BUILD_TYPE" ] && (echo "need to specify BUILD_TYPE as second arg" && exit 1)
shift

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR

cmake -GNinja \
  -D CMAKE_BUILD_TYPE=$BUILD_TYPE \
  -D BUILD_SHARED_LIBS=OFF \
  -D CMAKE_INSTALL_PREFIX=install \
  "$@" \
  ..
