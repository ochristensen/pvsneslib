#!/usr/bin/env sh

export DEBIAN_FRONTEND=noninteractive
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" | sudo tee -a /etc/apt/sources.list >/dev/null
sudo -E apt-add-repository -y "ppa:ubuntu-toolchain-r/test"
sudo -E apt-get -yq update
sudo -E apt-get -yq --no-install-suggests --no-install-recommends install cmake ninja-build clang++-6.0
export CC=clang-6.0
export CXX=clang++-6.0
./cmake_generate.sh build Release
cd build
ninja
