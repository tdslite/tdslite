#!/usr/bin/env bash

# _______________________________________________________
# tdslite devenv bootstrap script
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
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

LLVM_VERSION=14

# Enable abort on error
set -eu
set -o xtrace

# _________________________________________________________
# APT
# _________________________________________________________
readonly apt_command='apt-get'
readonly apt_args='-y install --no-install-recommends'

# Packages to be installed via apt
readonly apt_package_list=(
    # Prerequisites
    apt-utils dialog sudo curl
    # Editors
    nano vim
    # Shell & locale
    zsh locales less
    # Verify ssh, git, git-lfs process tools, lsb-release (useful for CLI installs) installed
    ssh git git-extras git-lfs iproute2 procps lsb-release
    # Install LLVM Toolchain, version 13
    llvm-${LLVM_VERSION} lld-${LLVM_VERSION} clang-${LLVM_VERSION} libc++-${LLVM_VERSION}-dev libc++abi-${LLVM_VERSION}-dev libunwind-${LLVM_VERSION}-dev clang-format-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} clangd-${LLVM_VERSION}
    # Install debugger, build generator & dependency resolution and build accelarator tools
    gdb make ninja-build autoconf automake libtool m4 cmake ccache rpm
    # Install python & pip
    python3 python3-pip python3-dev
    # Install static analyzers, formatting, tidying,
    iwyu cppcheck
    # Debugging/tracing
    valgrind
    # Documentation & graphing
    doxygen doxygen-doxyparse graphviz
    # Miscallenaous utilities
    bash-completion
    # Debugging tools
    strace ltrace
)

# _________________________________________________________
# PIP
# _________________________________________________________
readonly pip_command='pip3'
readonly pip_args='install'
# Packages to be installed via pip
readonly pip_package_list=(
    conan==1.45.0
    requests
    gcovr
)

# _________________________________________________________
# CONAN
# _________________________________________________________
readonly conan_command='conan'
# Packages to be installed via Conan
readonly conan_package_list=(
    gtest/1.11.0
    benchmark/1.5.3
)

echo "Current user is $(whoami)"

# _________________________________________________________
# Utility functions
# _________________________________________________________

# username useruid usergid
function add_user {
    ( groupadd --gid ${3} ${1} && useradd -s /bin/bash --uid ${2} --gid ${3} -m ${1} ) || return $?
    return 0
}
# username
function make_user_sudoer {
    ( echo ${1} ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/${1} && chmod 0440 /etc/sudoers.d/${1} ) || return $?
    return 0
}

# username
function switch_to_user {
    ( su - $1 ) || return $?
    return 0
}
# no params
function apt_cleanup {
    ( sudo ${apt_command} autoremove -y && sudo  ${apt_command} clean -y && sudo rm -rf /var/lib/apt/lists/* ) || return $?
    return 0
}
# no params
function pip_cleanup {
    ( pip cache purge ) || return $?
    ( sudo pip cache purge ) || return $?
    return 0
}

function conan_cleanup {
    sudo su ${1} -c "conan remove '*' -s -b -f"
}

#;
# cleanup()
# perform package manager cleanup and remove temporary files
# @return void
#"
function cleanup {
    #apt_cleanup
    pip_cleanup $1
    conan_cleanup $1
    # # Clean all temporary files
    sudo rm -rf /tmp/*
}

#;
# install_zsh_and_oh_my_zsh()
# Installs zsh shell and oh-my-zsh enhancement package
# @param user: user to install bash-git-prompt-for
# @return error code
#"
function install_zsh_and_oh_my_zsh {
    
    # Uncomment en_US.UTF-8 for inclusion in generation
    sudo sed -i 's/^# *\(en_US.UTF-8\)/\1/' /etc/locale.gen
    
    # Generate locale
    sudo locale-gen
    
    # Export env vars
    echo "export LC_ALL=en_US.UTF-8" >> /home/${1}/.bashrc
    echo "export LANG=en_US.UTF-8" >> /home/${1}/.bashrc
    echo "export LANGUAGE=en_US.UTF-8" >> /home/${1}/.bashrc
    
    # Install "oh my zsh" --unattended
    sudo su ${1} -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh) \"\" --unattended"
    
    # Update default shell for current user to zsh
    sudo chsh -s $(which zsh) ${1}
    
    # Update default shell for root user to zsh
    sudo chsh -s $(which zsh)
    
    # Copy oh-my-zsh config beforehand
    # sudo su ${1} -c "cp /home/${1}/.oh-my-zsh/templates/zshrc.zsh-template /home/${1}/.zshrc"
    
    # Replace the default theme with `jonathan` theme
    sudo sed -i 's/^ZSH_THEME="robbyrussell"/ZSH_THEME="jonathan"/' /home/${1}/.zshrc
    
    # Install oh-my-zsh plugins
    
    # Autosuggestions: https://github.com/zsh-users/zsh-autosuggestions
    # It suggests commands as you type based on history and completions.
    git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-/home/${1}/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
    
    # hacker-quotes git clone https://github.com/oldratlee/hacker-quotes
    git clone https://github.com/oldratlee/hacker-quotes.git ${ZSH_CUSTOM:-/home/${1}/.oh-my-zsh/custom}/plugins/hacker-quotes
    
    # ls: https://github.com/zpm-zsh/ls
    # Zsh plugin for ls. It improves the output of ls
    git clone https://github.com/zpm-zsh/ls.git ${ZSH_CUSTOM:-/home/${1}/.oh-my-zsh/custom}/plugins/ls
    
    # Enable plugins
    sudo sed -i 's/^plugins=(git)/plugins=(git git-auto-fetch zsh-autosuggestions ls hacker-quotes)/' /home/${1}/.zshrc
    
    return $?
}

function put_gdbinit_file {
    sudo su ${1} -c "cp ${SCRIPT_ROOT}/.gdbinit /home/${1}/.gdbinit"
}

function add_apt_repositories {
    # LLVM
    apt -y update && apt -y install --no-install-recommends ca-certificates wget gnupg2 && update-ca-certificates
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
    echo "deb http://apt.llvm.org/unstable/ llvm-toolchain main" > /etc/apt/sources.list.d/llvm.list
}

function install_apt_packages {
    echo "Installing apt packages..."
    ( ${apt_command} update && ${apt_command} ${apt_args} ${apt_package_list[@]} ) || return $?
    return 0
}

function install_pip_packages {
    echo "Installing pip packages..."
    ( sudo ${pip_command} ${pip_args} ${pip_package_list[@]} ) || return $?
    return 0
}


function conan_init {
    echo "Initializing conan"
    
    # Create all missing configuration files
    sudo su ${1} -c "conan config init"
    
    # Create default profile
    sudo su ${1} -c "conan profile new default --detect"
    return 0
}

#;
# install_conan_packages()
# install conan packages
# @param user: user to install conan packages for
# @return error code
#"
function install_conan_packages {
    echo "Installing conan packages..."
    
    # Install conan packages
    for pkg in "${conan_package_list[@]}"
    do
        (sudo -u ${1} ${conan_command} install --profile=$SCRIPT_ROOT/.conan/profiles/GNU ${pkg}@_/_ --build missing) || return $?
        (sudo -u ${1} ${conan_command} install --profile=$SCRIPT_ROOT/.conan/profiles/Clang ${pkg}@_/_ --build missing) || return $?
    done
    
    (sudo -u ${1} python3 $SCRIPT_ROOT/.conan/conanfile.py init_remotes) || return $?
    # Install conan packages into container
    # Install conan deps for GCC
    (sudo -u ${1} conan install $SCRIPT_ROOT/.conan/conanfile.py -pr=$SCRIPT_ROOT/.conan/profiles/GNU --build=missing -if /tmp) || return $?
    # Install conan deps for clang
    (sudo -u ${1} conan install $SCRIPT_ROOT/.conan/conanfile.py -pr=$SCRIPT_ROOT/.conan/profiles/Clang --build=missing -if /tmp) || return $?
    return 0
}

function adjust_symlinks {
    # Remove existing symlinks
    sudo rm /usr/bin/gcc 2>/dev/null || true
    sudo rm /usr/bin/g++ 2>/dev/null || true
    sudo rm /usr/bin/gcov 2>/dev/null || true
    sudo rm /usr/bin/python 2>/dev/null || true
    
    # LLVM Toolchain Related Removes
    sudo rm /usr/bin/clangd 2>/dev/null || true
    sudo rm /usr/bin/clang-format 2>/dev/null || true
    sudo rm /usr/bin/clang-tidy 2>/dev/null || true
    sudo rm /usr/bin/clang 2>/dev/null || true
    sudo rm /usr/bin/clang++ 2>/dev/null || true
    sudo rm /usr/bin/llvm-cov 2>/dev/null || true
    sudo rm /usr/bin/lld 2>/dev/null || true
    sudo rm /usr/bin/ld.lld 2>/dev/null || true
    
    # Create new symlinks
    sudo ln -sf /usr/bin/python3 /usr/bin/python
    
    # GCC symlink
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gcc /toolchains/x86_64-centos6-linux-gnu/bin/gcc
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/gcc /usr/bin/gcc
    
    # G++ symlink
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-g++ /toolchains/x86_64-centos6-linux-gnu/bin/g++
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/g++ /usr/bin/g++
    
    # (mgilor): Required for `gcovr`
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gcov /toolchains/x86_64-centos6-linux-gnu/bin/gcov
    sudo ln -sf /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gcov /usr/bin/gcov
    
    # (mgilor): Workaround for `gdb` bug
    sudo mv /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gdb /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gdb-orig
    sudo ln -sf /usr/bin/gdb /toolchains/x86_64-centos6-linux-gnu/bin/x86_64-centos6-linux-gnu-gdb
    
    # Conan
    sudo ln -sf /usr/local/bin/conan /usr/bin/conan
    
    # LLVM Toolchain Related
    sudo ln -sf /usr/bin/clangd-${LLVM_VERSION} /usr/bin/clangd
    sudo ln -sf /usr/bin/clang-format-${LLVM_VERSION} /usr/bin/clang-format
    sudo ln -sf /usr/bin/clang-tidy-${LLVM_VERSION} /usr/bin/clang-tidy
    sudo ln -sf /usr/bin/clang-${LLVM_VERSION} /usr/bin/clang
    sudo ln -sf /usr/bin/clang++-${LLVM_VERSION} /usr/bin/clang++
    sudo ln -sf /usr/bin/llvm-cov-${LLVM_VERSION} /usr/bin/llvm-cov
    sudo ln -sf /usr/bin/lld-${LLVM_VERSION} /usr/bin/lld
    sudo ln -sf /usr/bin/ld.lld-${LLVM_VERSION} /usr/bin/ld.lld
}


function post_install {
    sudo sh -c "git lfs install --skip-smudge"
    git lfs install --skip-smudge
    
    # Add one time notice
    tee -a /home/${1}/.zshrc << EOF
if [ -t 1 ] && [ ! -f \$HOME/.config/devenv/first-run-notice ]; then
  echo -e "You are now running in development environment container! This container is based on Debian Sid and contains all the development tools the project requires.\nYou can start coding right away!"
  mkdir -p \$HOME/.config/devenv
  touch \$HOME/.config/devenv/first-run-notice
fi
EOF
    # Set environment variables
    tee -a /home/${1}/.zshrc << EOF
export ASAN_OPTIONS="color=always detect_odr_violation=0"
export LSAN_OPTIONS="suppressions=/workspace/.lsan-suppressions"
EOF
}

# _________________________________________________________
# Main routine
# _________________________________________________________

(
    add_apt_repositories \
    &&
    install_apt_packages \
    &&
    add_user ${USERNAME} ${USER_UID} ${USER_GID} \
    &&
    make_user_sudoer $USERNAME \
    &&
    switch_to_user $USERNAME \
    &&
    install_pip_packages $USERNAME \
    &&
    conan_init $USERNAME \
    &&
    install_zsh_and_oh_my_zsh $USERNAME \
    &&
    put_gdbinit_file $USERNAME \
    &&
    adjust_symlinks \
    &&
    install_conan_packages $USERNAME \
    &&
    cleanup $USERNAME \
    &&
    post_install $USERNAME
) || exit 1

echo "Current user is $(whoami)"



