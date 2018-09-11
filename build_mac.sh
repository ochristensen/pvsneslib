#!/usr/bin/env sh

brew install cmake ninja
./cmake_generate.sh build Release
cd build
ninja
