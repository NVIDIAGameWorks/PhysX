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
# Build SampleBase common
#

SET(SAMPLEBASE_DIR ${PHYSX_ROOT_DIR}/samples/samplebase/)

# One of the test files reaches into this directory for an include
SET(UTILS_SRC_DIR "${PHYSX_ROOT_DIR}/test/common/Utils")

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PHYSX_ROOT_DIR}/samples/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/SampleBase.cmake)

SET(SAMPLEBASE_FILES
	${SAMPLEBASE_DIR}/AcclaimLoader.cpp
	${SAMPLEBASE_DIR}/AcclaimLoader.h
	${SAMPLEBASE_DIR}/Dummy.cpp
	${SAMPLEBASE_DIR}/InputEventBuffer.cpp
	${SAMPLEBASE_DIR}/InputEventBuffer.h
	${SAMPLEBASE_DIR}/PhysXSample.cpp
	${SAMPLEBASE_DIR}/PhysXSample.h
	${SAMPLEBASE_DIR}/PhysXSampleApplication.cpp
	${SAMPLEBASE_DIR}/PhysXSampleApplication.h
	${SAMPLEBASE_DIR}/Picking.cpp
	${SAMPLEBASE_DIR}/Picking.h
	${SAMPLEBASE_DIR}/RawLoader.cpp
	${SAMPLEBASE_DIR}/RawLoader.h
	${SAMPLEBASE_DIR}/RaycastCCD.cpp
	${SAMPLEBASE_DIR}/RaycastCCD.h
	${SAMPLEBASE_DIR}/RenderBaseActor.cpp
	${SAMPLEBASE_DIR}/RenderBaseActor.h
	${SAMPLEBASE_DIR}/RenderBaseObject.cpp
	${SAMPLEBASE_DIR}/RenderBaseObject.h
	${SAMPLEBASE_DIR}/RenderBoxActor.cpp
	${SAMPLEBASE_DIR}/RenderBoxActor.h
	${SAMPLEBASE_DIR}/RenderCapsuleActor.cpp
	${SAMPLEBASE_DIR}/RenderCapsuleActor.h
	${SAMPLEBASE_DIR}/RenderGridActor.cpp
	${SAMPLEBASE_DIR}/RenderGridActor.h
	${SAMPLEBASE_DIR}/RenderMaterial.cpp
	${SAMPLEBASE_DIR}/RenderMaterial.h
	${SAMPLEBASE_DIR}/RenderMeshActor.cpp
	${SAMPLEBASE_DIR}/RenderMeshActor.h
	${SAMPLEBASE_DIR}/RenderPhysX3Debug.cpp
	${SAMPLEBASE_DIR}/RenderPhysX3Debug.h
	${SAMPLEBASE_DIR}/RenderSphereActor.cpp
	${SAMPLEBASE_DIR}/RenderSphereActor.h
	${SAMPLEBASE_DIR}/RenderTexture.cpp
	${SAMPLEBASE_DIR}/RenderTexture.h
	${SAMPLEBASE_DIR}/SampleAllocator.cpp
	${SAMPLEBASE_DIR}/SampleAllocator.h
	${SAMPLEBASE_DIR}/SampleAllocatorSDKClasses.h
	${SAMPLEBASE_DIR}/SampleArray.h
	${SAMPLEBASE_DIR}/SampleBaseInputEventIds.h
	${SAMPLEBASE_DIR}/SampleCamera.cpp
	${SAMPLEBASE_DIR}/SampleCamera.h
	${SAMPLEBASE_DIR}/SampleCameraController.cpp
	${SAMPLEBASE_DIR}/SampleCameraController.h
	${SAMPLEBASE_DIR}/SampleCharacterHelpers.cpp
	${SAMPLEBASE_DIR}/SampleCharacterHelpers.h
	${SAMPLEBASE_DIR}/SampleConsole.cpp
	${SAMPLEBASE_DIR}/SampleConsole.h
	${SAMPLEBASE_DIR}/SampleInputMappingAsset.cpp
	${SAMPLEBASE_DIR}/SampleInputMappingAsset.h
	${SAMPLEBASE_DIR}/SampleMain.cpp
	${SAMPLEBASE_DIR}/SampleMouseFilter.cpp
	${SAMPLEBASE_DIR}/SampleMouseFilter.h
	${SAMPLEBASE_DIR}/SamplePreprocessor.h
	${SAMPLEBASE_DIR}/SampleRandomPrecomputed.cpp
	${SAMPLEBASE_DIR}/SampleRandomPrecomputed.h
	${SAMPLEBASE_DIR}/SampleStepper.cpp
	${SAMPLEBASE_DIR}/SampleStepper.h
	${SAMPLEBASE_DIR}/SampleUserInputDefines.h
	${SAMPLEBASE_DIR}/SampleUtils.h
	${SAMPLEBASE_DIR}/Test.h
	${SAMPLEBASE_DIR}/TestGeometryHelpers.cpp
	${SAMPLEBASE_DIR}/TestGeometryHelpers.h
	${SAMPLEBASE_DIR}/TestGroup.cpp
	${SAMPLEBASE_DIR}/TestGroup.h
	${SAMPLEBASE_DIR}/TestMotionGenerator.cpp
	${SAMPLEBASE_DIR}/TestMotionGenerator.h
	${SAMPLEBASE_DIR}/wavefront.cpp
	${SAMPLEBASE_DIR}/wavefront.h
)

ADD_LIBRARY(SampleBase STATIC
	${SAMPLEBASE_FILES}
)

TARGET_INCLUDE_DIRECTORIES(SampleBase

	PRIVATE ${PHYSX_ROOT_DIR}/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/common/src
	
	PRIVATE ${PHYSX_ROOT_DIR}/samples/sampleframework/framework/include
	PRIVATE ${PHYSX_ROOT_DIR}/samples/sampleframework/renderer/include
	PRIVATE ${PHYSX_ROOT_DIR}/samples/sampleframework/platform/include
	
	PRIVATE ${UTILS_SRC_DIR}

	PRIVATE ${PHYSX_ROOT_DIR}/source/common/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/common/src
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/contact
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/common
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/convex
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/distance
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/gjk
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/intersection
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/mesh
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/Ice
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/hf
	PRIVATE ${PHYSX_ROOT_DIR}/source/geomutils/src/pcm
)

TARGET_COMPILE_DEFINITIONS(SampleBase 
	PRIVATE ${SAMPLEBASE_COMPILE_DEFS}
)


IF(NV_USE_GAMEWORKS_OUTPUT_DIRS)
	SET_TARGET_PROPERTIES(SampleBase PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleBase_static_${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleBase_static_${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleBase_static_${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleBase_static_${CMAKE_RELEASE_POSTFIX}"

		ARCHIVE_OUTPUT_NAME_DEBUG "SampleBase_static"
		ARCHIVE_OUTPUT_NAME_CHECKED "SampleBase_static"
		ARCHIVE_OUTPUT_NAME_PROFILE "SampleBase_static"
		ARCHIVE_OUTPUT_NAME_RELEASE "SampleBase_static"
	)
ELSE()
	SET_TARGET_PROPERTIES(SampleBase PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleBase${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleBase${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleBase${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleBase${CMAKE_RELEASE_POSTFIX}"
	)
ENDIF()


TARGET_LINK_LIBRARIES(SampleBase 
	PUBLIC SampleToolkit
	PUBLIC ${SAMPLEBASE_PLATFORM_LINKED_LIBS}
)

IF(PX_GENERATE_SOURCE_DISTRO)	
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLEBASE_FILES})
ENDIF()