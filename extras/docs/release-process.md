# tdslite new version release process

- Create a new branch named "release/version"
- Bump the "version" field in library.json file
- Bump the "version" field in library.properties file
- Bump the PROJECT_METADATA_VERSION field in project-metadata.env file
- Commit the changes & push the branch
- Merge the branch
- Tag the commit with the version
- Add a new release to github releases page

## Update platform.io

- ^ Do the steps above first ^
- bash extras/prep-lib.sh
- pio account login
- pio pkg publish /workspace/build/arduino-libpack-root/tdslite.tar.gz