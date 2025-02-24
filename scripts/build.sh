DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath $DIR/..)
BUILD_DIR=$ROOT_DIR/build
CONFIG_FLAGS=" -G Ninja -S $ROOT_DIR -B $BUILD_DIR --toolchain $DIR/../cmake/toolchain.cmake"
BUILD_FLAGS="--config Release --target all --parallel"

# Parse input arguments
while getopts "c" opt; do
  case ${opt} in
    c )
      BUILD_FLAGS=$BUILD_FLAGS" --clean-first"
      ;;
    \? )
      echo "Usage: cmd [-c]"
      exit 1
      ;;
  esac
done

if [ ! -d "$BUILD_DIR" ]; then
  echo "Running CMake configuration..."
  cmake $CONFIG_FLAGS
fi

# Build local project
echo "Building project..."
cmake --build $BUILD_DIR $BUILD_FLAGS