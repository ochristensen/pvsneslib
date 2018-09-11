#!/usr/bin/env sh

curl -L https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-win.zip > ninja-win.zip
unzip -oqq ninja-win.zip
export PATH="$(pwd)":$PATH
./cmake_generate.sh build Release
cd build
../ninja
