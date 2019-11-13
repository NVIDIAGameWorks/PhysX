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
# Change options for CMAKE_BUILD_TYPE and set default.
# Same values than CMAKE_CONFIGURATION_TYPES
# From https://gitlab.kitware.com/cmake/cmake/issues/19401
get_property(multiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT multiConfig AND NOT DEFINED CMAKE_BUILD_TYPE)
    set(default_build_type "release")
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE ${default_build_type} CACHE STRING "Choose the type of build.")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "release" "profile" "checked" "debug")
endif()

file(READ "version.txt" PHYSX_VERSION)
project(PhysX
    LANGUAGES C CXX
    VERSION ${PHYSX_VERSION})
message(STATUS "PhysX VERSION: ${PHYSX_VERSION}")
cmake_policy(SET CMP0057 NEW) # Enable IN_LIST
cmake_policy(SET CMP0077 NEW) # option() does nothing when variable is alredy set

# PhysXSDK options:
option(PX_BUILDSNIPPETS "Generate the snippets" OFF)
option(PX_BUILDPUBLICSAMPLES "Generate the samples" OFF)
option(PX_CMAKE_SUPPRESS_REGENERATION "Disable zero_check projects" OFF)
# PhysX options:
option(PX_SCALAR_MATH "Disable SIMD math" OFF)
option(PX_GENERATE_STATIC_LIBRARIES "Generate static libraries" OFF)
if(NOT DEFINED BUILD_SHARED_LIBS)
    if(PX_GENERATE_STATIC_LIBRARIES)
        set(BUILD_SHARED_LIBS OFF)
    else()
        set(BUILD_SHARED_LIBS ON)
    endif()
else()
    if(BUILD_SHARED_LIBS EQUAL PX_GENERATE_STATIC_LIBRARIES)
        message(FATAL_ERROR "Contradictory options: BUILD_SHARED_LIBS and PX_GENERATE_STATIC_LIBRARIES have both the same value: ${BUILD_SHARED_LIBS}")
    endif()
endif()
message(STATUS "BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "  PX_GENERATE_STATIC_LIBRARIES: ${PX_GENERATE_STATIC_LIBRARIES}")
option(PX_EXPORT_LOWLEVEL_PDB "Export low level pdb's" OFF)
option(PX_GENERATE_GPU_PROJECTS_ONLY "Generate GPU projects only. (Untested)" OFF)
mark_as_advanced(PX_GENERATE_GPU_PROJECTS_ONLY)

set(PUBLIC_RELEASE OFF)
# Enable folder properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES debug checked profile release)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
        "Reset config to what we need"
        FORCE)

    set(CMAKE_SHARED_LINKER_FLAGS_CHECKED "")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILE "")

endif()

# Disable zero_check projects. The default for Switch and XboxOne is ON.
if(PX_CMAKE_SUPPRESS_REGENERATION)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif()

### Set PHYSX_ROOT_DIR to PROJECT_SOURCE_DIR
if(DEFINED PHYSX_ROOT_DIR)
    message(WARNING "PHYSX_ROOT_DIR is externally defined, but it will be overwritten in this CMakeLists. DEPRECATED")
    message("PHYSX_ROOT_DIR (externally set --not used--): ${PHYSX_ROOT_DIR}")
    message("PHYSX_ROOT_DIR (currently set): ${PROJECT_SOURCE_DIR}")
endif()
set(PHYSX_ROOT_DIR ${PROJECT_SOURCE_DIR})

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
set(CMAKEMODULES_PATH ${PROJECT_SOURCE_DIR}/../externals/cmakemodules/
    CACHE INTERNAL "Path to CMakeModules")
set(CMAKEMODULES_NAME "CMakeModules" CACHE INTERNAL "CMakeModules name")
set(CMAKEMODULES_VERSION "1.27" CACHE INTERNAL "CMakeModules version from generation batch")
# CMAKE_MODULE_PATH is empty by default
list(APPEND CMAKE_MODULE_PATH ${CMAKEMODULES_PATH})

### Set platform specific files
set(PROJECT_CMAKE_FILES_DIR source/compiler/cmake)
set(PROJECT_CMAKE_FILES_ABSOLUTE_DIR ${PHYSX_ROOT_DIR}/${PROJECT_CMAKE_FILES_DIR})
# The following files define all the flags specific to platforms, compilers and build configurations.
# The file is included in the source/ subdirectory
set(PLATFORM_CMAKELISTS "${PHYSX_ROOT_DIR}/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/CMakeLists.txt")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# INSTALL PATHS
# XXX(phcerdan) CMAKE_INSTALL_PREFIX by definition has to point to the root folder of the installation
# but the upstream python configuration adds PhysX to the CMAKE_INSTALL_PREFIX, which is not standard,
# and then hack it to install PxShared in parallel to PhysX. Solved here:
set(PHYSX_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/PhysX CACHE INTERNAL "Install path to install PhysX")
set(PXSHARED_PATH ${PROJECT_SOURCE_DIR}/../pxshared CACHE INTERNAL "Path to PxShared source")
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

# TODO(phcerdan) Snippets and Samples are not integrated with the new CMake procedure
# But check the project_using_physx for hints on how to integrate it.
if(PX_BUILDSNIPPETS)
    # Add Snippets projects into the solution
    add_subdirectory(${PHYSX_ROOT_DIR}/snippets/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_snippets_bin)

    message("Added Snippets")
endif()

if(PX_BUILDPUBLICSAMPLES)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR
       CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Add samples projects into the solution
        add_subdirectory(${PHYSX_ROOT_DIR}/samples/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_samples_bin)

        message("Added Samples")
    endif()
endif()

