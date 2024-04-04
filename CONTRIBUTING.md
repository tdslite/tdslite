# Contributing to tdslite

There are multiple ways to contribute to tdslite project. You don't have to develop a new feature or submit a bugfix. Even trying `tdslite` with a particular hardware to test whether it works is invaluable.

Contributions are always welcome. Feel free to file issues/feature requests, submit pull requests and test the library with different devices!

## Preparing the development environment

The project ships a docker-compose file that includes everything needed for developing the `tdslite`, including a built-in `Microsoft SQL Server 2022` for testing. The docker-compose file is primarily in place to work with VSCode's Dev Containers extension. While it's not necessary to use `vscode` for developing `tdslite`, I'd recommend it.

### Visual Studio Code

Starting development wih Visual Studio Code is as simple as installing the `VSCode Dev Containers` extension, and selecting `Remote Containers: Open folder in Container` command bar item. In order to install the said extension, simply type the following to your terminal:

```bash
    code --install-extension ms-vscode-remote.remote-containers # replace `code` with `code-insiders` if you're on the insider build
```

To open the project with VSCode in the development environment container:

```bash
    # Go into project folder, and then:
    code workspace.code-workspace # # replace `code` with `code-insiders` if you're on the insider build
```

#### Building the project (vscode)

- Step 0: F1 -> CMake : Select Configure Preset -> (Select a preset, e.g. Configure | GCC 12.2 (dev|debug))
- Step 1: F1 -> CMake: Configure
- Step 2: F1 -> CMake: Build
- Step 3: F1 -> CMake: Run Tests (... or alternatively, press the "Run CTest" UI button)

### Using the development environment without `vscode`

It's possible to use the docker-compose file manually to bring the development environment up by using the following commands:

```bash
    cd .devcontainer/
    # bring the devenv up
    docker-compose up -d
    # start a shell into devenv
    docker exec -it --user dev:dev tdslite-devenv zsh -c "cd /workspace;zsh -i"
    # stop development environment when not needed
    docker-compose stop
    # use the "tdslite-mssql-2022" name to get a shell from the database server instead.
```

#### Building the project (command line)

Replace "gcc-13-dev-debug" with the preset of your choice, see `cmake --list-presets` for all presets available.

- Step -1: Get a shell from `devenv` container: `docker exec -it --user dev:dev tdslite-devenv zsh -c "cd /workspace;zsh -i"`
- Step 0: Configure: `cmake --preset "gcc-13-dev-debug"`
- Step 1: Build: `cmake --build --preset "gcc-13-dev-debug"`
- Step 2: Run all tests: `ctest --preset "gcc-13-dev-debug"`

### Going commando

If you don't want to use the shipped development environment containers, you'll have to install several things to your environment. Take a look into [bootstrap.sh](.docker/devenv/bootstrap.sh) to see how devenv container is bootstrapped. Note: I'd NOT recommend running it on your environment directly.

### Why containers?

`tdslite` project treats development environment as a first class citizen. The development environment itself is a crucial part of any project; it has to be well-defined and consistent. Containers are lightweight, portable way to provide just that. With a well-defined development environment, anybody can start contributing to the project immediately without have to hassle with dependencies and prerequisites.

The Github CI pipeline also uses the exact same image as the development environment, therefore the results and expected outcomes will be consistent everywhere.
