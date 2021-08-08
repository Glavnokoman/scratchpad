#!/usr/bin/bash
# link "./active" to Debug clang build
set -xe

BUILD_DIR=/tmp/build/scratchpad/clang_x64/dbg
SCRIPT_PATH=$(dirname $(realpath -s $0))
PROJECT_PATH=$(realpath ${SCRIPT_PATH}/..)

[ -L ./active ] && rm ./active
mkdir -p ${BUILD_DIR} && ln -fs ${BUILD_DIR} ./active

cd ${BUILD_DIR}
export CXXFLAGS="-fsanitize=address -Wall"
export LDFLAGS=-fsanitize=address
cmake -G Ninja \
	-DCMAKE_BUILD_TYPE=Debug \
   -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
   -DCMAKE_CXX_COMPILER=clang++ \
   ${PROJECT_PATH}
 
ln -fs ${BUILD_DIR}/compile_commands.json ${PROJECT_PATH}/compile_commands.json

# cmake --build ./
