# FIXME: Proper implementation, please!


SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
    DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

SCRIPT_ROOT="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
PROJECT_ROOT=$SCRIPT_ROOT/..
BUILD_FOLDER=$PROJECT_ROOT/build
PACK_TMP=$BUILD_FOLDER/arduino-libpack-root/tdslite
rm -rf $BUILD_FOLDER/arduino-libpack-root &> /dev/null || true
mkdir -p $PACK_TMP
cd $PROJECT_ROOT
pio pkg pack -o $PACK_TMP/tdslite.tar.gz
cd $PACK_TMP
tar xzf tdslite.tar.gz
mv tdslite.tar.gz $BUILD_FOLDER/arduino-libpack-root
cd $BUILD_FOLDER/arduino-libpack-root
zip -q -r tdslite.zip tdslite/
ls -lrah tdslite.zip
