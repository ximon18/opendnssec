#!/bin/bash
set -e -u -x

cd /src
sh autogen.sh
mkdir build || true
cd build
../.devcontainer/reconfigure.sh

# the developer still needs to:
#   make
#   sudo make install
#   echo y | sudo ods-ksmutil setup
#   ods-hsmutil login