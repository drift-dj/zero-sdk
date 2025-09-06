DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath $DIR/..)
BUILD_DIR=$ROOT_DIR/build

# We expect a path to drift-os's resources/zero-externals/ 
# dir as first param
INSTALL_ROOT=$1

# Copy everything from scripts dir to 
cp $BUILD_DIR/libzerodj.a $INSTALL_ROOT || exit 1
cp $ROOT_DIR/res/zero_atlas-32bit.bmp $INSTALL_ROOT/zero_atlas-32bit.bmp || exit 1
cp $ROOT_DIR/res/device.db $INSTALL_ROOT/device.db || exit 1
cp $ROOT_DIR/res/soundcard.db $INSTALL_ROOT/soundcard.db || exit 1