# Parse input arguments
CLEAN_BUILD=false
while getopts "c" opt; do
  case ${opt} in
    c )
      CLEAN_BUILD=true
      ;;
    \? )
      echo "Usage: cmd [-c]"
      exit 1
      ;;
  esac
done

# Perform a clean build if -c is specified
if [ "$CLEAN_BUILD" = true ]; then
  echo "Cleaning build directory..."
  rm -rf build
fi

# Get directory of the script (not the current working directory)
DIR="$(cd "$(dirname "$0")" && pwd -P)"
BUILD_DIR=$(realpath $DIR/../build)
ROOT_DIR=$(realpath $DIR/..)

# Cmake configure using the toolchain file
echo "Running CMake configuration..."
cmake -G Ninja -S $ROOT_DIR -B $BUILD_DIR --toolchain $DIR/../cmake/toolchain.cmake

# Build local project
cmake --build $BUILD_DIR --config Release