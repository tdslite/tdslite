set -eu

ms=$(cmake --list-presets | cut -d'"' -f 2 | awk '!/Available/' | awk NF)
presets=($ms)

for preset in "${presets[@]}"
do
    :
    echo "Testing ${preset} preset..."
    rm -rf /workspace/build
    cmake --preset ${preset} || (echo "Preset ${preset} failed at configure step, exiting!" && exit 1)
    cmake --build --preset ${preset} || (echo "Preset ${preset} failed at build step, exiting!" && exit 1)
    ctest --preset ${preset} || (echo "Preset ${preset} failed at test step, exiting!" && exit 1)
done