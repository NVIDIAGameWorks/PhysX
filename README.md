# NVIDIA PhysX SDK 4.1

# Improved CMake integration (contributed by @phcerdan)
- Provide PhysXConfig.cmake and exported targets file for build and install tree.
- Other projects can use find_package(PhysX) where PhysX_DIR can be a build tree or an install tree.
- The implementation only adds two new CMakeLists.txt that do not collide with
the existing build procedure of Nvidia. Instead of using python and the generate_projects scripts, now it solely uses CMake in a standard way.
- This allows PhysX to be used with modern standards of CMake, making it compatible
   with FetchContent (add_subdirectory) and ExternalProject (find_package) commands.
- Snippets and Samples have not been integrated into the new build procedure.
- But added a simpler project to show find_package(PhysX) usage.
- The original build procedure is maintained compatible for easier integration with future upstream changes from NVidia.

## Example of CMake usage

# Build and optionally install PhysX with just CMake:
```bash
mkdir PhysX;
git clone https://github.com/phcerdan/PhysX src
mkdir build; cd build;
cmake -GNinja -DCMAKE_BUILD_TYPE:STRING=release -DCMAKE_INSTALL_PREFIX:PATH=/tmp/physx ../src
ninja install
```

# Your project using PhysX (example added)

```cmake
find_package(PhysX REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE PhysX::PhysXCommon PhysX::PhysXExtensions)
```

You can also use FetchContent, or ExternalProjects for handling PhysX automatically.

When building your project, just provide `PhysX_DIR` to where the PhysXConfig.cmake is located (it could be from a build or an install tree)
```bash
cmake -GNinja -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -DPhysX_DIR:PATH=/tmp/physx/PhysX/bin/cmake/physx ../src
```

## Using FetchContent (building at configure time)

For now, the `CONFIGURATION_TYPES` of Nvidia PhysX are not default.
In order to "map" the standard CMake build/config types between a project and PhysX
we have to do extra work.
The following `CMake` gist configure and build PhysX using the variable `PHYSX_CONFIG_TYPE` (default to `release`)
to build PhysX with the chosen config type.
Also provides a install script to install PhysX with the project.
TODO(phcerdan): this installation script might require more flexibility and testing.
See [conversation](https://discourse.cmake.org/t/mapping-cmake-build-type-and-cmake-configuration-types-between-project-subdirectories/192/2)
that inspired this approach (thanks to @craig.scott)

```cmake
  # Fetch physx (CMake phcerdan branch)
  include(FetchContent)
  FetchContent_Declare(
    physx
    GIT_REPOSITORY https://github.com/phcerdan/PhysX
    GIT_TAG cmake_for_easier_integration
  )
  FetchContent_GetProperties(physx)
  if(NOT physx_POPULATED)
    message(STATUS "  Populating PhysX...")
    FetchContent_Populate(physx)
    message(STATUS "  Configuring PhysX...")
    execute_process(
      COMMAND ${CMAKE_COMMAND}
        -S ${physx_SOURCE_DIR}/physx/
        -B ${physx_BINARY_DIR}
        -DCMAKE_GENERATOR=${CMAKE_GENERATOR}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
      WORKING_DIRECTORY ${physx_BINARY_DIR}
      COMMAND_ECHO STDOUT
      # OUTPUT_FILE       ${physx_BINARY_DIR}/configure_output.log
      # ERROR_FILE        ${physx_BINARY_DIR}/configure_output.log
      RESULT_VARIABLE   result_config
      )
    if(result_config)
        message(FATAL_ERROR "Failed PhysX configuration")
        # see configuration log at:\n    ${physx_BINARY_DIR}/configure_output.log")
    endif()
    # PhysX is always on release mode, but can be explicitly changed by user:
    set(PHYSX_CONFIG_TYPE "release" CACHE INTERNAL "Config/build type for PhysX")
    message(STATUS "Building PhysX... with CONFIG: ${PHYSX_CONFIG_TYPE}")
    execute_process(
      COMMAND ${CMAKE_COMMAND}
      --build ${physx_BINARY_DIR}
      --config ${PHYSX_CONFIG_TYPE}
      WORKING_DIRECTORY ${physx_BINARY_DIR}
      COMMAND_ECHO STDOUT
      # OUTPUT_FILE       ${physx_BINARY_DIR}/build_output.log
      # ERROR_FILE        ${physx_BINARY_DIR}/build_output.log
      RESULT_VARIABLE   result_build
      )
    message(STATUS "  PhysX build complete")
    if(result_build)
        message(FATAL_ERROR "Failed PhysX build")
        # see build log at:\n    ${physx_BINARY_DIR}/build_output.log")
    endif()
    # add_subdirectory(${physx_SOURCE_DIR}/physx ${physx_BINARY_DIR})
  endif()
  # create rule to install PhysX when installing this project
  install (CODE "
  execute_process(
    COMMAND ${CMAKE_COMMAND}
    --build ${physx_BINARY_DIR}
    --config ${PHYSX_CONFIG_TYPE}
    --target install
    WORKING_DIRECTORY ${physx_BINARY_DIR}
    COMMAND_ECHO STDOUT
    )")
  # find_package works
  find_package(PhysX REQUIRED
    PATHS ${physx_BINARY_DIR}/sdk_source_bin
    NO_DEFAULT_PATH
    )
```


Copyright (c) 2019 NVIDIA Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
 * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
 * Neither the name of NVIDIA CORPORATION nor the names of its
   contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Introduction

Welcome to the NVIDIA PhysX SDK source code repository. This depot includes the PhysX SDK and the Kapla Demo application.

The NVIDIA PhysX SDK is a scalable multi-platform physics solution supporting a wide range of devices, from smartphones to high-end multicore CPUs and GPUs. PhysX is already integrated into some of the most popular game engines, including Unreal Engine, and Unity3D. [PhysX SDK on developer.nvidia.com](https://developer.nvidia.com/physx-sdk).

## Documentation

Please see [Release Notes](http://gameworksdocs.nvidia.com/PhysX/4.1/release_notes.html) for updates pertaining to the latest version.

The full set of documentation can also be found in the repository under physx/documentation or online at http://gameworksdocs.nvidia.com/simulation.html 

Platform specific information can be found here:
* [Microsoft Windows](http://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/windows/readme_windows.html)
* [Linux](http://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/linux/readme_linux.html)
* [Google Android ARM](http://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/android/readme_android.html)
* [Apple macOS](http://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/mac/readme_mac.html)
* [Apple iOS](http://gameworksdocs.nvidia.com/PhysX/4.1/documentation/platformreadme/ios/readme_ios.html)
 

## Quick Start Instructions

Requirements:
* Python 2.7.6 or later
* CMake 3.12 or later

To begin, clone this repository onto your local drive.  Then change directory to physx/, run ./generate_projects.[bat|sh] and follow on-screen prompts.  This will let you select a platform specific solution to build.  You can then open the generated solution file with your IDE and kick off one or more configuration builds.

To build and run the Kapla Demo see [kaplademo/README.md](kaplademo/README.md).

## Acknowledgements

This depot contains external third party open source software copyright their respective owners.  See [kaplademo/README.md](kaplademo/README.md) and [externals/README.md](externals/README.md) for details.
