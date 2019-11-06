##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions
## are met:
##  * Redistributions of source code must retain the above copyright
##    notice, this list of conditions and the following disclaimer.
##  * Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimer in the
##    documentation and/or other materials provided with the distribution.
##  * Neither the name of NVIDIA CORPORATION nor the names of its
##    contributors may be used to endorse or promote products derived
##    from this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
## EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
## PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Copyright (c) 2018-2019 NVIDIA Corporation. All rights reserved.
## Copyright (c) 2019 Pablo Hernandez-Cerdan

cmake_minimum_required(VERSION 3.13)
project(PhysX C CXX)
cmake_policy(SET CMP0057 NEW) # Enable IN_LIST
cmake_policy(SET CMP0077 NEW) # option() does nothing when variable is alredy set

# PhysXSDK options:
option(PX_BUILDSNIPPETS "Generate the snippets" OFF)
option(PX_BUILDPUBLICSAMPLES "Generate the samples" OFF)
option(PX_CMAKE_SUPPRESS_REGENERATION "Disable zero_check projects" OFF)
# PhysX options:
option(PX_SCALAR_MATH "Disable SIMD math" OFF)
option(PX_GENERATE_STATIC_LIBRARIES "Generate static libraries" OFF)
option(PX_EXPORT_LOWLEVEL_PDB "Export low level pdb's" OFF)
option(PX_GENERATE_GPU_PROJECTS_ONLY "Generate GPU projects only. (Untested)" OFF)
mark_as_advanced(PX_GENERATE_GPU_PROJECTS_ONLY)

set(PUBLIC_RELEASE 1)
# Enable folder properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES debug checked profile release)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
        "Reset config to what we need"
        FORCE)

    set(CMAKE_SHARED_LINKER_FLAGS_CHECKED "")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "")

    # XXX(phcerdan) PDB are not for consumers, ignore this line
    # Build PDBs for all configurations.
    # set(CMAKE_SHARED_LINKER_FLAGS "/DEBUG")
endif()

# Disable zero_check projects. The default for Switch and XboxOne is ON.
if(PX_CMAKE_SUPPRESS_REGENERATION)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif()

### Set PHYSX_ROOT_DIR to CMAKE_SOURCE_DIR
if(DEFINED PHYSX_ROOT_DIR)
    message(WARNING "PHYSX_ROOT_DIR is externally defined, but it will be overwritten in this CMakeLists. DEPRECATED")
    message("PHYSX_ROOT_DIR (externally set --not used--): ${PHYSX_ROOT_DIR}")
    message("PHYSX_ROOT_DIR (currently set): ${CMAKE_SOURCE_DIR}")
endif()
set(PHYSX_ROOT_DIR ${CMAKE_SOURCE_DIR})

### Set TARGET_BUILD_PLATFORM using CMAKE_SYSTEM_NAME
# for compatibility with current CMake files,
# for cross-complation, CMAKE_SYSTEM_NAME can be set when running cmake
if(DEFINED TARGET_BUILD_PLATFORM)
    if(TARGET_BUILD_PLATFORM STREQUAL "switch" OR
       TARGET_BUILD_PLATFORM STREQUAL "playstation" OR
       TARGET_BUILD_PLATFORM STREQUAL "ios")
       message(FATAL_ERROR "switch, playstation and ios builds are not valid because have not been tested. Use official repository for these.")
    endif()
    message(INFO "TARGET_BUILD_PLATFORM (externally set --not used--): ${TARGET_BUILD_PLATFORM}")
endif()

set(TARGET_BUILD_PLATFORM "")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TARGET_BUILD_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_BUILD_PLATFORM "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TARGET_BUILD_PLATFORM "mac")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_BUILD_PLATFORM "android")
endif()

### Set CMake folders
set(CMAKEMODULES_PATH ${CMAKE_SOURCE_DIR}/../externals/cmakemodules/
    CACHE INTERNAL "Path to CMakeModules")
set(CMAKEMODULES_NAME "CMakeModules" CACHE INTERNAL "CMakeModules name")
set(CMAKEMODULES_VERSION "1.27" CACHE INTERNAL "CMakeModules version from generation batch")
# CMAKE_MODULE_PATH is empty by default
list(APPEND CMAKE_MODULE_PATH ${CMAKEMODULES_PATH})

### Set platform specific files
set(PROJECT_CMAKE_FILES_DIR source/compiler/cmake)
# The following files define all the flags specific to platforms, compilers and build configurations.
# The file is included in the source/ subdirectory
set(PLATFORM_CMAKE_FILES_DIR "${PHYSX_ROOT_DIR}/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}")
set(PLATFORM_CMAKELISTS "${PLATFORM_CMAKE_FILES_DIR}/CMakeLists.txt")

SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

# INSTALL PATHS
# XXX(phcerdan) CMAKE_INSTALL_PREFIX by definition points to the root folder of the installation
# the upstream python configuration sets PhysX in the INSTALL_PREFIX,
# but then hack it to install PxShared in parallel to PhysX.
SET(PHYSX_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/PhysX CACHE INTERNAL "Install path to install PhysX")
set(PXSHARED_PATH ${CMAKE_SOURCE_DIR}/../pxshared CACHE INTERNAL "Path to PxShared source")
set(PXSHARED_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/PxShared CACHE INTERNAL "Path to install PxShared")

# Set PX_ROOT_LIB_DIR (used in cmake files)
string(TOLOWER ${CMAKE_C_COMPILER_ID} compiler_id)
# No need to add TARGET_BUILD_PLATFORM and compiler_id in the folder structure
# set(PX_ROOT_LIB_DIR "${PHYSX_INSTALL_PREFIX}/bin/${TARGET_BUILD_PLATFORM}\.${compiler_id}")
set(PX_ROOT_LIB_DIR "${PHYSX_INSTALL_PREFIX}/bin")

# We add CMakeLists.txt in the source folders, following standard CMake practices
# Add PhysX SDK Source code to solution
set(BUILD_SOURCE_FOLDER ${CMAKE_CURRENT_BINARY_DIR}/sdk_source_bin)
add_subdirectory(source ${BUILD_SOURCE_FOLDER})

if(PX_BUILDSNIPPETS)
    # TODO(phcerdan) this is the part to simplify
    # Add Snippets projects into the solution
    add_subdirectory(${PHYSX_ROOT_DIR}/snippets/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_snippets_bin)

    message("Added Snippets")
endif()

if(PX_BUILDPUBLICSAMPLES)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
       CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # TODO(phcerdan) this is the part to simplify
        # Add samples projects into the solution
        add_subdirectory(${PHYSX_ROOT_DIR}/samples/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_samples_bin)

        message("Added Samples")
    endif()
endif()

