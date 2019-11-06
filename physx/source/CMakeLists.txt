MESSAGE("PhysX Build Platform: " ${TARGET_BUILD_PLATFORM})
MESSAGE("Using CXX Compiler: " ${CMAKE_CXX_COMPILER})

set(NV_USE_GAMEWORKS_OUTPUT_DIRS ON)

# Include the platform specific CMakeLists
include(${PLATFORM_CMAKELISTS})

# generate PxPhysXConfig.h header that will contain PhysX configuration defines like PX_PHYSX_STATIC_LIB
# XXX(phcerdan) the template is actually empty? Maybe populated after? No harm having an empty header meanwhile.
SET(HEADER_GUARD_NAME "CONFIG")
SET(HEADER_CONTENT "")
configure_file(${CMAKEMODULES_PATH}/template/PxIncludeTemplate.h ${PHYSX_ROOT_DIR}/include/PxConfig.h)

### Move libPhysXGPU from source tree to build and install tree:
# libPhysXGPU libraries are in the repository (/bin) but the CUDA code is not open
# XXX(phcerdan) Nvdia does not provides GNU (gcc) GPU libaries, only clang.
# XXX(phcerdan) I guess both compilers are ABI compatible, but not explicitly tested in this codebase
# XXX(phcerdan) GCC in linux is not an option right now upstream

include(GetCompilerAndPlatform)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LIBPATH_SUFFIX "64")
    # Set default exe suffix. Unset on platforms that don't need it. Include underscore since it's optional
    set(EXE_SUFFIX "_64")
else()
    set(LIBPATH_SUFFIX "32")
    # Set default exe suffix. Unset on platforms that don't need it. Include underscore since it's optional
    set(EXE_SUFFIX "_32")
endif()
# platform_bin_name is of type linux.clang, win.x86_64.vc140 etc.
GetPlatformBinName(PLATFORM_BIN_NAME ${LIBPATH_SUFFIX})
message(STATUS "PLATFORM_BIN_NAME: ${PLATFORM_BIN_NAME}")

# set(gpu_source_libs_folder "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_BIN_NAME}/$<CONFIG>")
# set(gpu_source_libs_folder "${CMAKE_SOURCE_DIR}/bin/${PLATFORM_BIN_NAME}/${CMAKE_BUILD_TYPE}")
# Build tree
# file(COPY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_BIN_NAME}/$<CONFIG>
#     DESTINATION "${BUILD_SOURCE_FOLDER}")
# Install tree
install(FILES ${CMAKE_SOURCE_DIR}/bin/${PLATFORM_BIN_NAME}/$<CONFIG>
    # DESTINATION "${PHYSX_INSTALL_PREFIX}/bin/${CMAKE_BUILD_TYPE}")
    DESTINATION "${PHYSX_INSTALL_PREFIX}/bin/$<CONFIG>")
