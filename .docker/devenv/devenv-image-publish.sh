#!/usr/bin/env bash

# _______________________________________________________
# tdslite devenv image publish script
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   17.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________


SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
    DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done

SCRIPT_ROOT="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
PROJECT_ROOT=$SCRIPT_ROOT/../..

set -eu

source $PROJECT_ROOT/project-metadata.env
PROJECT_METADATA_VERSION=$PROJECT_METADATA_VERSION_MAJOR.$PROJECT_METADATA_VERSION_MINOR.$PROJECT_METADATA_VERSION_REVISION

if docker push $PROJECT_METADATA_DOCKER_REPOSITORY_URL/$PROJECT_METADATA_DOCKER_DEVENV_IMAGE_NAME:$PROJECT_METADATA_VERSION; then
    echo "Image updated successfully."
fi
