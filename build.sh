#!/usr/bin/env bash

emcmake cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
cd build || exit 1
emmake make
cd - || exit
