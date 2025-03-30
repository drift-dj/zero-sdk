DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath $DIR/..)
BUILD_DIR=$ROOT_DIR/build

# Send via sz
echo "uploading $BUILD_DIR/libzerodj.a to $1/usr/lib"
echo "cd /usr/lib" > "$1"
sz --zmodem -y $BUILD_DIR/libzerodj.a > "$1" < "$1" || exit 1

echo "uploading $ROOT_DIR/res/zero_atlas-32bit.bmp to $1/root/res"
echo "cd /root/res" > "$1"
sz --zmodem -y $ROOT_DIR/res/zero_atlas-32bit.bmp > "$1" < "$1" || exit 1