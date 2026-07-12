#!/bin/bash
set -euo pipefail

cd $(dirname $0)"/../.."

# Data files are resolved at runtime relative to the current working directory
# (see GetDataFilePath in src/cdogs/utils.c), so build the bundle with the data
# dir pointing at the working directory. The launcher scripts in the packaged
# bundle chdir into the bundle root before exec'ing, which makes the game find
# its data next to the binary.
cmake -S . -B build-output -DCMAKE_BUILD_TYPE=Release -DCDOGS_DATA_DIR=./
cmake --build build-output -j"$(( $(nproc) > 1 ? $(nproc) - 1 : 1 ))"
