#!/usr/bin/env sh

HOMEBREW_NO_AUTO_UPDATE=1 brew install ninja
HOMEBREW_NO_AUTO_UPDATE=1 brew upgrade cmake
./cmake_generate.sh build Release
cd build
ninja
