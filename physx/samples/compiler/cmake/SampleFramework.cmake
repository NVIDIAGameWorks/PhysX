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

#
# Build SampleFramework common
#

FIND_PACKAGE(Targa $ENV{PM_targa_VERSION} CONFIG REQUIRED)

SET(SAMPLEFRAMEWORK_DIR ${PHYSX_ROOT_DIR}/samples/sampleframework)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PHYSX_ROOT_DIR}/samples/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/SampleFramework.cmake)

SET(SAMPLEFRAMEWORK_FILES
	${SAMPLEFRAMEWORK_DIR}/framework/src/nv_dds.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/nv_dds.h
	${SAMPLEFRAMEWORK_DIR}/framework/src/ODBlock.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleApplication.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleAsset.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleAssetManager.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleCommandLine.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleDirManager.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleInputAsset.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleLineDebugRender.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleMaterialAsset.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SamplePointDebugRender.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleTextureAsset.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleTree.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/src/SampleTriangleDebugRender.cpp
	${SAMPLEFRAMEWORK_DIR}/framework/include/FrameworkFoundation.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/ODBlock.h	
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleActor.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleApplication.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleArray.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleAsset.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleAssetManager.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleCommandLine.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleDirManager.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleFilesystem.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleFrameworkInputEventIds.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleInputAsset.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleLineDebugRender.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleMaterialAsset.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SamplePointDebugRender.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleTextureAsset.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleTree.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleTriangleDebugRender.h
	${SAMPLEFRAMEWORK_DIR}/framework/include/SampleXml.h

	${TARGA_SRC_DIR}/targa.cpp
)

ADD_LIBRARY(SampleFramework STATIC
	${SAMPLEFRAMEWORK_PLATFORM_SOURCES}
	
	${SAMPLEFRAMEWORK_FILES}
)

TARGET_INCLUDE_DIRECTORIES(SampleFramework
	PUBLIC ${SAMPLEFRAMEWORK_PLATFORM_INCLUDES}
	PUBLIC ${SAMPLEFRAMEWORK_DIR}/framework/include
	PUBLIC ${SAMPLEFRAMEWORK_DIR}/renderer/include
	PUBLIC ${SAMPLEFRAMEWORK_DIR}/platform/include

	PRIVATE ${PHYSX_ROOT_DIR}/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/common/src
	
	PRIVATE ${PHYSX_ROOT_DIR}/source/fastxml/include

	PRIVATE ${TARGA_INCLUDE_DIR}
)

TARGET_COMPILE_DEFINITIONS(SampleFramework 
	PRIVATE ${SAMPLEFRAMEWORK_COMPILE_DEFS}
)


IF(NV_USE_GAMEWORKS_OUTPUT_DIRS)
	SET_TARGET_PROPERTIES(SampleFramework PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleFramework_static_${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleFramework_static_${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleFramework_static_${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleFramework_static_${CMAKE_RELEASE_POSTFIX}"

		ARCHIVE_OUTPUT_NAME_DEBUG "SampleFramework_static"
		ARCHIVE_OUTPUT_NAME_CHECKED "SampleFramework_static"
		ARCHIVE_OUTPUT_NAME_PROFILE "SampleFramework_static"
		ARCHIVE_OUTPUT_NAME_RELEASE "SampleFramework_static"
	)
ELSE()
	SET_TARGET_PROPERTIES(SampleFramework PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleFramework${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleFramework${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleFramework${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleFramework${CMAKE_RELEASE_POSTFIX}"
	)
ENDIF()


TARGET_LINK_LIBRARIES(SampleFramework
	PRIVATE ${SAMPLEFRAMEWORK_PRIVATE_PLATFORM_LINKED_LIBS} 
	PUBLIC PhysXFoundation SampleToolkit
	PUBLIC ${SAMPLEFRAMEWORK_PLATFORM_LINKED_LIBS}
)

IF(PX_GENERATE_SOURCE_DISTRO)	
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLEFRAMEWORK_PLATFORM_SOURCES})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLEFRAMEWORK_FILES})
ENDIF()