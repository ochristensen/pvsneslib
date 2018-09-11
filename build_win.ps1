rm -r -fo build
mkdir build
cd build

cmake -GNinja \
  -D CMAKE_BUILD_TYPE=$BUILD_TYPE \
  -D BUILD_SHARED_LIBS=OFF \
  -D CMAKE_INSTALL_PREFIX=install \
  ..

ninja
