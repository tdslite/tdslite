# _______________________________________________________
# tdslite devenv dockerfile
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

# Base image
FROM debian:sid-slim as build_temp

# Labels
LABEL version="1.0.0" maintainer="Mustafa Kemal GILOR <mustafagilor@gmail.com>" license="MIT"

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=$USER_UID

COPY .docker/devenv/bootstrap.sh /scripts/bootstrap.sh
COPY .conan/profiles/common /scripts/.conan/profiles/common
COPY .conan/profiles/GNU /scripts/.conan/profiles/GNU
COPY .conan/profiles/Clang /scripts/.conan/profiles/Clang
COPY project-metadata.env /scripts/project-metadata.env
COPY .gdbinit /scripts/.gdbinit

# Configure apt and install packages
RUN chmod +x /scripts/bootstrap.sh && /scripts/bootstrap.sh

FROM build_temp

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog

# Switch to non-root user
USER $USER_UID:$USER_GID
