#!/usr/bin/env sh

sudo -E apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
sudo -E apt-get -yq update
sudo -E apt-get -yq --no-install-suggests --no-install-recommends install cmake ninja-build g++-7
./cmake_generate.sh build Release
cd build
ninja
