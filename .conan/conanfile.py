# ______________________________________________________
# conanfile of the tdslite project
#
# @file   conanfile.py
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

import sys
import tempfile
from conans import ConanFile, CMake
from pathlib import Path
import os


def load_env_file(*args) -> dict:
    file_path = Path(os.path.realpath(__file__)).parent
    for rp in args:
        path = str(file_path.joinpath(rp))
        try:
            with open(path, 'r') as f:
                return dict(tuple(line.replace('\n', '').replace("\"", "").split('=')) for line
                            in f.readlines() if not line.startswith('#') and not len(line.strip()) == 0)
        except:
            pass
    raise Exception("env file not found in given paths")


class ConanPkgInfo(ConanFile):
    topics = ("sql", "tds", "rdbms", "driver")
    settings = "os", "compiler", "build_type", "arch"

    # Build missing dependency binaries from source
    build_policy = "missing"

    # Do not copy source/ to build/ folder
    no_copy_source = True

    # Configurable option set
    options = {}

    # Default values for option set
    default_options = {}

    # Exported files along with sources
    exports = {
        "../project-metadata.env"
    }

    def requirements(self):
        pass

    def build_requirements(self):
        if self.in_local_cache:
            # We're building the package
            self.generators += ('cmake', 'cmake_find_package')
        else:
            # We're just consuming the package dependencies via CMake
            self.generators += ('cmake_find_package',
                                'virtualrunenv', 'markdown')

        self.build_requires("boost/1.78.0")

    def init(self):
        on_recipe_folder = os.path.join(
            self.recipe_folder, "project-metadata.env")
        on_source_folder = os.path.join(
            self.recipe_folder, "../project-metadata.env")
        project_metadata = load_env_file(on_recipe_folder, on_source_folder)

        self.name = project_metadata["PROJECT_METADATA_NAME"]
        self.version = "{}.{}.{}".format(project_metadata["PROJECT_METADATA_VERSION_MAJOR"],
                                         project_metadata["PROJECT_METADATA_VERSION_MINOR"], project_metadata["PROJECT_METADATA_VERSION_REVISION"])
        self.license = project_metadata["PROJECT_METADATA_LICENSE"]
        self.author = project_metadata["PROJECT_METADATA_AUTHORS"]
        self.url = project_metadata["PROJECT_METADATA_URL"]
        self.description = project_metadata["PROJECT_METADATA_DESCRIPTION"]

    def export_local_package(self, name, version, user, channel):
        recipes_path = os.path.join(self.recipe_folder, "recipes")
        self.run("conan export {} {}/{}@{}/{} ".format(name,
                 name, version, user, channel), cwd=recipes_path)

    def export_sources(self):
        self.copy("*", src="../", excludes=(".docker/*",
                  "build/*", "compile_commands.json"))

    def _configure_cmake(self):
        cmake = CMake(self)
        # Suppress installing or up-to-date messages from cmake install
        cmake.definitions["CMAKE_INSTALL_MESSAGE"] = "NEVER"
        # Only print cmake logs with level equal or above to warning level
        cmake.definitions["CMAKE_MESSAGE_LOG_LEVEL"] = "WARNING"
        cmake.definitions["TDSLITE_PROJECT_DISABLE_UNIT_TEST_TARGETS"] = True
        cmake.definitions["TDSLITE_PROJECT_DISABLE_BENCHMARK_TARGETS"] = True
        cmake.definitions["TDSLITE_PROJECT_DISABLE_EXECUTABLE_TARGETS"] = True
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_GOOGLE_TEST"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_GOOGLE_BENCH"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_GCOV"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_LCOV"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_GCOVR"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_DOXYGEN"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_CLANG_FORMAT"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_CLANG_TIDY"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_CPPCHECK"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_CCACHE"] = False
        cmake.definitions["TDSLITE_PROJECT_TOOLCONF_USE_IWYU"] = False
        cmake.definitions["TDSLITE_PROJECT_MISC_NO_HADOUKEN_BANNER"] = True
        cmake.definitions["TDSLITE_PROJECT_ENABLE_SANITIZERS_ON_DEBUG"] = False
        cmake.definitions["TDSLITE_PROJECT_ENABLE_SANITIZERS_ON_RELEASE"] = False
        cmake.definitions["TDSLITE_PROJECT_STRIP_SYMBOL_AND_RELOCATION_INFO"] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.components["tdslite"].names["cmake_find_package"] = "tdslite"
        self.cpp_info.components["tdslite"].names["cmake_find_package_multi"] = "tdslite"
        self.cpp_info.components["tdslite"].includedirs = "include"


def deploy_local(args):
    script_root = Path(os.path.abspath(__file__)).parent
    profile_file = script_root.joinpath("profiles/GNU")
    source_directory = script_root.parent

    if(len(args) >= 2):
        command_args = args[1:]

    def is_argument_specified(argument):
        return 'command_args' in locals() and argument in command_args

    if is_argument_specified("--remove-existing"):
        os.system(
            "conan remove {}/{}  -f || true".format(ConanPkgInfo.name, ConanPkgInfo.version))

    if is_argument_specified("--via-create"):
        # {}/{}
        # , ConanPkgInfo.name, ConanPkgInfo.version
        os.system("conan create --profile={} {} --build=missing".format(
            str(profile_file.absolute()), str(script_root.absolute())))
    else:
        with tempfile.TemporaryDirectory("conan-build-root", ConanPkgInfo.name) as temporary_build_root:
            # temporary_install_root = tempfile.TemporaryDirectory("conan-install-root", ConanPkgInfo.name)
            os.system("conan install --build=missing --profile={} --install-folder={} {}".format(
                str(profile_file.absolute()), str(temporary_build_root), str(script_root.absolute())))
            if(is_argument_specified("--only-deps")):
                return
            os.system("conan build --source-folder={} --install-folder={} --build-folder={} {}".format(str(
                source_directory), str(temporary_build_root), str(temporary_build_root), str(script_root.absolute())))
            with tempfile.TemporaryDirectory("conan-pkg-root", ConanPkgInfo.name) as temporary_package_root:
                os.system("conan package --source-folder={} --install-folder={} --build-folder={} --package-folder={} {}".format(str(
                    source_directory), str(temporary_build_root), str(temporary_build_root), str(temporary_package_root), str(script_root.absolute())))
                os.system("conan export-pkg --force --profile={} --source-folder={} --build-folder={} {}".format(str(
                    profile_file.absolute()), str(source_directory), str(temporary_build_root), str(script_root.absolute())))

    if is_argument_specified("--prepare-archive"):
        os.system("conan upload --skip-upload --all --confirm --parallel --check {}/{}".format(
            ConanPkgInfo.name, ConanPkgInfo.version))


# Callable
if __name__ == '__main__':
    if(len(sys.argv) >= 2):
        globals()[sys.argv[1]](sys.argv[1:])
    else:
        print("No arguments specified. Available args: `deploy_local`")
