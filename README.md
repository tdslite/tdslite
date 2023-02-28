<p align="center">
  <img width="600" height="213" src="./.misc/markdown-assets/img/banner.png">
</p>

----
<p align="center">
<img src="https://img.shields.io/badge/contributions-welcome-orange.svg"/> <img src="https://img.shields.io/badge/license-MIT-blue.svg" href="https://opensource.org/licenses/MIT"/> <img src="https://img.shields.io/github/issues/mustafakemalgilor/tdslite.svg" href="https://github.com/mustafakemalgilor/tdslite/issues"/> <img src="https://img.shields.io/github/stars/mustafakemalgilor/tdslite"/>
</p>
<p align="center">
    <i>Lightweight</i> MS-TDS implementation written in pure C++11, that can work with just <strong>2kB</strong> of SRAM!
<br />
</p>

----

 Built around C++'s `zero cost / only pay for what you use` mantra. The implementation is based on Microsoft's `MS-TDS: Tabular Data Stream Protocol` technical specification (revision number 33).

----

## Key features

- Header-only
- Pure C++11
- Intuitive API
- *Zero* external dependencies
- Permissive open-source license (MIT)
- Suitable for embedded development
- Low memory footprint
- Decoupled networking design that can be easily extended
- Callback based design
- Supports networking interfaces that implement Arduino's `EthernetClient`.
- Supports
  - ... query execution `driver.execute_query(...)`
  - ... queries with parameters `driver.execute_rpc(...)`
  - ... reading result sets

----

## The design

`tdslite` is header-only pure C++11 code, written with platform independence in mind.

----

## The goal

The main design goal of the project is to enable a wide variety of constrained devices (e.g. embedded, IoT) to talk to a TDS server (e.g. Microsoft SQL Server). The aim is not to implement the TDS standard down to every single letter. Most of the use cases do not require many legacy features that the TDS standard carries along, so the remaining feature set that TDS offer will be implemented on an `as-needed` basis, rather than being implemented upfront.

`tdslite` is not embedded-centric, but embedded-suitable. `tdslite` is agnostic from any particular architecture, platform, environment or standard library implementation (*except for a few basic things).

----

## Collaborating

The project ships a docker-compose file that includes everything needed for developing the `tdslite`, including a built-in `Microsoft SQL Server 2017` for testing. The docker-compose file is primarily in place to work with VSCode's Dev Containers extension. Starting development wih Visual Studio Code is as simple as installing the `VSCode Dev Containers` extension, and selecting `Remote Containers: Open folder in Container` command bar item. In order to install the said extension, simply type the following to your terminal:

```bash
    code --install-extension ms-vscode-remote.remote-containers # replace `code` with `code-insiders` if you're on the insider build
```

To open the project with VSCode in the development environment container:

```bash
    # Go into project folder, and then:
    code workspace.code-workspace # # replace `code` with `code-insiders` if you're on the insider build
```

It's also possible to use the docker-compose file manually to bring the development environment up by using the following commands:

```bash
    cd .devcontainer/
    # bring the devenv up
    docker-compose up -d
    # start a shell into devenv
    docker exec -it --user dev:dev tdslite-devenv zsh -c "cd /workspace;zsh -i"
    # stop development environment when not needed
    docker-compose stop
    # use the "tdslite-mssql-2017" name to get a shell from the database server instead.
```

The Github CI pipeline uses the exact same image as the development environment, therefore the results and expected outcomes will be consistent everywhere.

### The tech stack

The below are the tools and libraries that used for developing this project:

- `CMake`: build system generator for the project
- `Docker`: The development environment containers
- `googletest`: unit and integration tests
- `Conan`: used as dependency manager, fetches dependencies for tests (e.g. Boost, googletest)
- `Boost`: Used for the ASIO-based `test` network implementation
- `clang-format`: Code formatting
- `pio`: Platform.IO CLI, used for HW CI tests

### Building the project (vscode)

Step 0: F1 -> CMake : Select Configure Preset -> (Select a preset, e.g. Configure | GCC 12.2 (dev|debug))
Step 1: F1 -> CMake: Configure
Step 2: F1 -> CMake: Build

### Running all tests (vscode)

Step 0: Build the project ^^
Step 1: F1 -> CMake: Run Tests (... or alternatively, press the "Run CTest" UI button)

### Building the projec, running all tests (command line)

Step 0: Configure

```bash
    cmake --preset "gcc-12-dev-debug" 
    # replace "gcc-12-dev-debug" with the preset of your choice, `cmake --list-presets`
```

Step 1: Build

```bash
    cmake --build --preset "gcc-12-dev-debug"
```

Step 2: Run all tests

```bash
    ctest --preset "gcc-12-dev-debug"
```

----

## Motivation

tdslite is the successor of the **arduino-mssql** project. The project aims to have a lightweight, production-ready implementation that can work in embedded environments.

----

## Dependencies

tdslite is a self-contained library with no external dependencies, and it does not rely on C++ standard library to function. Instead, needed functionality from C++ standard library is implemented from scratch as needed (e.g., type_traits, span). Therefore, the only requirement to use this library is **to have a decent, C++11 compliant compiler**.

----

## Getting started

Take a look into examples below to get your hands dirty!

[➤ 01 - Initialize Library](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/01-initialize-library)

Example code illustrating how to initialize the library.

[➤ 02 - CRUD with tdslite](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/02-create-insert-select-delete)

The four basic operations: Create, read, update, delete.

[➤ 03 - Read rows from table](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/03-select-rows)

Retrieve a result set with a query (a.k.a `SELECT`ing rows)

[➤ 04 - Library callback functions](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/04-callbacks)

How to use callback functions to read info/error messages sent by the server.

[➤ 05 - Execute query with parameters](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/05-query-with-parameters)

Learn how to use and bind parameters in queries with tdslite.

[➤ 06 - Custom memory allocator functions](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/06-custom-malloc)

Learn how to set user-defined malloc/free functions for tdslite's memory allocation.

----

## Contributing

Contributions are always welcome. Feel free to file issues/feature requests, submit pull requests and test the library with different devices!

----

Built with ❤︎ by [mkg](https://www.twitter.com/mgilor) and [contributors](https://github.com/mustafakemalgilor/tdslite/graphs/contributors)
