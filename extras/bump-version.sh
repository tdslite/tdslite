#!/usr/bin/env bash

set -e

a=( ${1//./ } )
VER_MAJOR="${a[0]}"
VER_MINOR="${a[1]}"
VER_PATCH="${a[2]}"

echo "Version is ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}"

CURRENT_BRANCH=$(git -C /workspace rev-parse --abbrev-ref HEAD)
MAIN_BRANCH=$(basename $(git -C /workspace symbolic-ref --short refs/remotes/origin/HEAD))
if [ "${CURRENT_BRANCH}" != "${MAIN_BRANCH}" ]; then
    echo "Not in \`${MAIN_BRANCH}\` branch, please switch."
    exit 1
fi

if ! git diff-index --quiet HEAD --; then
    echo "Dirty state, please have a clean working tree!"
    exit 2
fi

git checkout -b release/${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}

# Update library.json
sed  -i '/"version": /s/"\([0-9.]*\)*",$/'"\""${1}"\",/" /workspace/library.json

# Update library.properties
sed  -i '/version=/s/\([0-9.]*\)*$/'"${1}/" /workspace/library.properties

# Update project-metadata.json
sed  -i '/PROJECT_METADATA_VERSION_MAJOR=/s/"\([0-9.]*\)*"$/'"\"${VER_MAJOR}\"/" /workspace/project-metadata.env
sed  -i '/PROJECT_METADATA_VERSION_MINOR=/s/"\([0-9.]*\)*"$/'"\"${VER_MINOR}\"/" /workspace/project-metadata.env
sed  -i '/PROJECT_METADATA_VERSION_REVISION=/s/"\([0-9.]*\)*"$/'"\"${VER_PATCH}\"/" /workspace/project-metadata.env

git add /workspace/library.json
git add /workspace/library.properties
git add /workspace/project-metadata.env
git diff --cached
git commit -S -s -m "Bump the version number to ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH} for the next release"



