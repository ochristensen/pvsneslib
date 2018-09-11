#!/usr/bin/env sh

echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" | sudo tee -a /etc/apt/sources.list >/dev/null
sudo -E apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
sudo -E apt-get -yq update
sudo -E apt-get -yq --no-install-suggests --no-install-recommends install cmake ninja-build clang++-6.0
eval "CC=clang-6.0 && CXX=clang++-6.0"
./cmake_generate.sh build Release
cd build
ninja
