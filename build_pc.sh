#! /bin/bash

cmake -S . -B build_pc -DCMAKE_BUILD_TYPE=Release
cmake --build build_pc --target all
