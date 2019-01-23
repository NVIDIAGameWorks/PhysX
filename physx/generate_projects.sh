#!/bin/bash +x

export PHYSX_ROOT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export PM_PxShared_PATH="$PHYSX_ROOT_DIR/../pxshared"
export PM_CMakeModules_PATH="$PHYSX_ROOT_DIR/../externals/cmakemodules"
export PM_opengllinux_PATH="$PHYSX_ROOT_DIR/../externals/opengl-linux"
export PM_PATHS=$PM_opengllinux_PATH

cd "$( dirname "${BASH_SOURCE[0]}" )"
python ./buildtools/cmake_generate_projects.py $1
status=$?
if [ "$status" -ne "0" ]; then
echo "Error $status"
exit 1
fi