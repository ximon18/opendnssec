#!/bin/bash
set -e -u -x
/src/configure --with-pkcs11-softhsm=$(find / -type f -name 'libsofthsm2.so') $*