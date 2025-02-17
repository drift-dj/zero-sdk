# Get directory of this script
DIR="$(cd "$(dirname "$0")" && pwd -P)"

# Cmake configure using the toolchain file
cmake -G Ninja -S . -B build --toolchain $DIR/../cmake/toolchain.cmake

# Build local project
cmake --build build --config Release