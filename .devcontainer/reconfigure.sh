#!/bin/bash
set -e -u -x
export CFLAGS='-g -O0'
/src/configure --with-pkcs11-softhsm=$(find / -type f -name 'libsofthsm2.so') $*