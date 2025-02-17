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

# Get directory of this script
DIR="$(cd "$(dirname "$0")" && pwd -P)"

# Cmake configure using the toolchain file
cmake -G Ninja -S . -B build --toolchain $DIR/../cmake/toolchain.cmake

# Build local project
cmake --build build --config Release