#!/bin/bash

set -e

mkdir build
cd build

echo ">> Config CMake for ($COMPILER) compiler"

case $COMPILER in
  "clang7")
    cmake ../ -DNOJEM=0 -DCMAKE_C_FLAGS_DEBUG="-DNDEBUG" -DCMAKE_CXX_FLAGS_DEBUG="-DNDEBUG" -DCMAKE_INSTALL_PREFIX=check_install -DCMAKE_C_COMPILER=clang-7 -DCMAKE_CXX_COMPILER=clang++-7
    ;;

  "clang10")
    cmake ../ -DNOJEM=0 -DCMAKE_C_FLAGS_DEBUG="-DNDEBUG" -DCMAKE_CXX_FLAGS_DEBUG="-DNDEBUG" -DCMAKE_INSTALL_PREFIX=check_install -DCMAKE_C_COMPILER=clang-10 -DCMAKE_CXX_COMPILER=clang++-10
    ;;

  *)
    echo "Unknown compiler $COMPILER"
    exit 1
    ;;
esac

cd ..
