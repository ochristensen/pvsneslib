#!/usr/bin/env sh

curl -L https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-win.zip > ninja-win.zip
unzip -o ninja-win.zip
export tools_ver=$(cat /c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/Enterprise/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt)
export CC=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/Enterprise/VC/Tools/$tools_ver/bin/Hostx64/x64/cl.exe
export CXX=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/Enterprise/VC/Tools/$tools_ver/bin/Hostx64/x64/cl.exe
./cmake_generate.sh build Release
cd build
../ninja
