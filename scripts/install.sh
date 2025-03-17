DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath $DIR/..)
BUILD_DIR=$ROOT_DIR/build

# We expect a path to drift-os's resources/zero-externals/zero-sdk/ 
# dir as first param
INSTALL_ROOT=$1

# echo "Installing from"
# echo $ROOT_DIR
# echo "to"
# echo $INSTALL_ROOT

# Copy everything from scripts dir to 
cp $BUILD_DIR/libzerodj.a $INSTALL_ROOT || exit 1