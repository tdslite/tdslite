# tdslite new version release process

Releases are automated with two GitHub workflows:

1. Go to **Actions → Release - Prepare → Run workflow** and enter the new
   version (e.g. `0.8.1`). This bumps the version in `library.json`,
   `library.properties` and `project-metadata.env` on a new `release/<version>`
   branch and opens a PR.
2. Review the PR, wait for CI, and merge it. **Release - Publish** then:
   - tags the merge commit as `v<version>`
   - creates a GitHub release with generated notes, with `tdslite.zip` and
     `tdslite.tar.gz` attached
   - publishes the package to the PlatformIO registry

The Arduino Library Manager picks up new tags automatically.

## One-time setup

- **Settings → Actions → General → Workflow permissions**: enable
  "Allow GitHub Actions to create and approve pull requests".
- `PLATFORMIO_AUTH_TOKEN` secret (from `pio account token`). If it is not set,
  the PlatformIO publish step is skipped.
- Optional: a `RELEASE_TOKEN` secret (a fine-grained PAT with
  `contents: write` and `pull-requests: write`). PRs opened with the default
  `GITHUB_TOKEN` don't trigger CI. Without this secret, close and reopen the
  release PR to start CI.

## Manual fallback

- `bash extras/bump-version.sh <version>` (in the devenv container, on `main`)
- Push the branch, open a PR and merge it
- `git tag -a v<version> -m v<version> && git push origin v<version>`
- `gh release create v<version> --generate-notes`
- `bash extras/prep-lib.sh`, then
  `pio pkg publish build/arduino-libpack-root/tdslite.tar.gz`
