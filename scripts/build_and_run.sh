#! /bin/bash
set -e

BUILD_DIRECTORY="build"
CLEAN={CLEAN:-false}

if [ ! -d "$BUILD_DIRECTORY" ]; then
  mkdir -p "$BUILD_DIRECTORY"
fi

cd "$BUILD_DIRECTORY"

cmake ..

cmake --build .

./so_project
