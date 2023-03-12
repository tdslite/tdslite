# FIXME: Proper implementation, please!


SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
    DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

SCRIPT_ROOT="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
PROJECT_ROOT=$SCRIPT_ROOT/../../..
BUILD_FOLDER=$PROJECT_ROOT/build
ARDUINO_LIBPACK_ROOT=$BUILD_FOLDER/arduino-libpack-root
rm -r $ARDUINO_LIBPACK_ROOT
mkdir -p $ARDUINO_LIBPACK_ROOT/tdslite/src
cd $ARDUINO_LIBPACK_ROOT/tdslite
touch tdslite.h
cp $SCRIPT_ROOT/src/library.properties .
cp $SCRIPT_ROOT/src/tdslite.h src/
cp -r $PROJECT_ROOT/tdslite/include/* src/
cp -r $PROJECT_ROOT/tdslite-net/tdslite-net-base/include/* src/
cp -r $PROJECT_ROOT/tdslite-net/tdslite-net-arduino/include/* src/
cd $ARDUINO_LIBPACK_ROOT
rm tdslite.zip || true
zip -r tdslite.zip tdslite/
ls -lrah tdslite.zip

