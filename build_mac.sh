#!/usr/bin/env sh

HOMEBREW_NO_AUTO_UPDATE=1
brew install ninja
brew upgrade cmake
./cmake_generate.sh build Release
cd build
ninja
