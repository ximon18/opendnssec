#-------------------------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See https://go.microsoft.com/fwlink/?linkid=2090316 for license information.
#-------------------------------------------------------------------------------------------------------------

FROM ubuntu:19.10

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# This Dockerfile adds a non-root 'vscode' user with sudo access. However, for Linux,
# this user's GID/UID must match your local user UID/GID to avoid permission issues
# with bind mounts. Update USER_UID / USER_GID if yours is not 1000. See
# https://aka.ms/vscode-remote/containers/non-root-user for details.
ARG USERNAME=vscode
ARG USER_UID=1001
ARG USER_GID=$USER_UID

# Configure apt and install packages
RUN apt-get update \
    && apt-get -y install --no-install-recommends apt-utils dialog 2>&1 \
    # 
    # Verify git, process tools, lsb-release (useful for CLI installs) installed
    && apt-get -y install git iproute2 procps lsb-release \
    #
    # Install C++ tools
    && apt-get -y install build-essential cmake cppcheck valgrind \
    #
    # Create a non-root user to use if preferred - see https://aka.ms/vscode-remote/containers/non-root-user.
    && groupadd --gid $USER_GID $USERNAME \
    && useradd -s /bin/bash --uid $USER_UID --gid $USER_GID -m $USERNAME \
    # [Optional] Add sudo support for the non-root user
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME\
    && chmod 0440 /etc/sudoers.d/$USERNAME

# Install OpenDNSSEC 2.1.x dependencies
RUN apt-get -y install --no-install-recommends \
    # OpenDNSSec 2.1.4 dependencies
    # Based on: https://github.com/opendnssec/opendnssec/blob/2.1.4/README.md#building-from-the-source-code-respository
    automake \
    autoconf \
    libtool \
    # Based on: https://wiki.opendnssec.org/display/DOCS20/Quick+start+guide
    libsqlite3-dev \
    libxml2-dev \
    libldns-dev \
    softhsm2 \
    # Java is required when building from sources (for relaxNG) \
    openjdk-14-jre \
    # Install syslog as this is where OpenDNSSec 1.4 and SoftHSM send their logging
    rsyslog \
    # Install locales so that we can generate the en_US.UTF-8 missing locale
    locales \
    # Additional packages needed
    # prevents configure syntax error near unexpected token `LIBUNWIND` caused
    # by missing PKG_CHECK_MODULES m4 macro.
    pkg-config

# Setup SoftHSM v2.
# Based on: https://wiki.opendnssec.org/display/SoftHSMDOCS/SoftHSM+Documentation+v2
#
# softhsm2-util will just fail with 'ERROR: Could not initialize the library' if its default token directory does not exist.
# Try and find out where it should be and ensure that it exists.
RUN TOKEN_DIR=$(find /etc -type f -name softhsm2.conf -exec grep -E '^directories.tokendir\s*=' {} \; | cut -d '=' -f 2) && echo "Creating SoftHSM v2 default token director '$TOKEN_DIR' if it doesn't already exist." && mkdir -p $TOKEN_DIR

# Initialize SoftHSM
ARG SOFTHSM_TOKEN_LABEL=OpenDNSSEC
ARG SOFTHSM_PIN=1234
ARG SOFTHSM_SO_PIN=5678
RUN echo "Initializing SoftHSM with SO pin '${SOFTHSM_SO_PIN}' and pin '${SOFTHSM_PIN}'"
RUN softhsm2-util --init-token --slot 0 --label "${SOFTHSM_TOKEN_LABEL}" --so-pin "${SOFTHSM_SO_PIN}" --pin "${SOFTHSM_PIN}"
#
# End setup SoftHSM v2.

# Prevent missing LC_ALL and LANG warnings when executing autogen.sh
# See also: http://jaredmarkell.com/docker-and-locales/
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# Create a dir for VS code to mount the source code into
WORKDIR /src

# For testing cmocka test framework
RUN apt-get install -y libcmocka0 libcmocka-dev

RUN apt-get -y install --no-install-recommends \
    libunwind-dev

# Clean up
RUN apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=
