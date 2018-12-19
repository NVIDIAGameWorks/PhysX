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
## Copyright (c) 2018 NVIDIA Corporation. All rights reserved.

cmake_minimum_required(VERSION 3.12)

project(PhysXSDK C CXX)

OPTION(PX_BUILDSNIPPETS "Generate the snippets" OFF)
OPTION(PX_BUILDSAMPLES "Generate the samples" OFF)
OPTION(PX_CMAKE_SUPPRESS_REGENERATION "Disable zero_check projects" OFF)

IF(NOT DEFINED CMAKEMODULES_VERSION)
	SET(CMAKEMODULES_PATH $ENV{PM_CMakeModules_PATH} CACHE INTERNAL "Path to CMakeModules")
	SET(CMAKEMODULES_NAME "CMakeModules" CACHE INTERNAL "CMakeModules name")
	SET(CMAKEMODULES_VERSION "1.27" CACHE INTERNAL "CMakeModules version from generation batch")
ENDIF()

SET(CMAKE_MODULE_PATH ${CMAKEMODULES_PATH})

# This is required to be defined by external callers!
IF(NOT DEFINED PHYSX_ROOT_DIR)
	MESSAGE(FATAL_ERROR "PHYSX_ROOT_DIR variable wasn't set.")
ENDIF()

IF(NOT EXISTS ${PHYSX_ROOT_DIR})
	MESSAGE(FATAL_ERROR "PHYSX_ROOT_DIR variable was invalid.")
ENDIF()

SET(PUBLIC_RELEASE 1)

# Enable folder properties
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# Disable zero_check projects. The default for Switch and XboxOne is ON.
IF(PX_CMAKE_SUPPRESS_REGENERATION)
	SET(CMAKE_SUPPRESS_REGENERATION true)
ENDIF()

# Add PhysX SDK Source code to solution
ADD_SUBDIRECTORY(${PHYSX_ROOT_DIR}/source/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_source_bin)

MESSAGE("Added PhysX")

# Set folder PhysX SDK to all SDK source projects
SET_PROPERTY(TARGET PhysX PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXCharacterKinematic PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXCommon PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXCooking PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXExtensions PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXVehicle PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET LowLevel PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET LowLevelAABB PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET LowLevelDynamics PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET SceneQuery PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET SimulationController PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET FastXml PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXPvdSDK PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXTask PROPERTY FOLDER "PhysX SDK")
SET_PROPERTY(TARGET PhysXFoundation PROPERTY FOLDER "PhysX SDK")

IF(PX_BUILDSNIPPETS)
	# Add Snippets projects into the solution
	ADD_SUBDIRECTORY(${PHYSX_ROOT_DIR}/snippets/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_snippets_bin)

	MESSAGE("Added Snippets")

	SET_PROPERTY(TARGET SnippetArticulation PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetBVHStructure PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetContactModification PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetContactReport PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetContactReportCCD PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetConvexMeshCreate PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetCustomJoint PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetDeformableMesh PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetHelloWorld PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetImmediateMode PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetJoint PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetMBP PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetMultiThreading PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetNestedScene PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetPrunerSerialization PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetRaycastCCD PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetSerialization PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetSplitFetchResults PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetSplitSim PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetStepper PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetToleranceScale PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetTriangleMeshCreate PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetUtils PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicle4W PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicleContactMod PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicleMultiThreading PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicleNoDrive PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicleScale PROPERTY FOLDER "Snippets")
	SET_PROPERTY(TARGET SnippetVehicleTank PROPERTY FOLDER "Snippets")
	
	IF(TARGET_BUILD_PLATFORM STREQUAL "windows")
		SET_PROPERTY(TARGET SnippetConvert PROPERTY FOLDER "Snippets")
		SET_PROPERTY(TARGET SnippetLoadCollection PROPERTY FOLDER "Snippets")
		SET_PROPERTY(TARGET SnippetDelayLoadHook PROPERTY FOLDER "Snippets")	
		SET_PROPERTY(TARGET SnippetRender PROPERTY FOLDER "Snippets")
		SET_PROPERTY(TARGET SnippetHelloGRB PROPERTY FOLDER "Snippets")
	ENDIF()
	IF(TARGET_BUILD_PLATFORM STREQUAL "linux")
		SET_PROPERTY(TARGET SnippetConvert PROPERTY FOLDER "Snippets")
		SET_PROPERTY(TARGET SnippetLoadCollection PROPERTY FOLDER "Snippets")
		IF(${CMAKE_LIBRARY_ARCHITECTURE} STREQUAL "aarch64-linux-gnu")
		ELSE()
			SET_PROPERTY(TARGET SnippetHelloGRB PROPERTY FOLDER "Snippets")
		ENDIF()
	ENDIF()

	IF(TARGET_BUILD_PLATFORM STREQUAL "windows" OR TARGET_BUILD_PLATFORM STREQUAL "linux" OR TARGET_BUILD_PLATFORM STREQUAL "xboxone" OR TARGET_BUILD_PLATFORM STREQUAL "ps4" OR TARGET_BUILD_PLATFORM STREQUAL "switch")
		SET_PROPERTY(TARGET SnippetCustomProfiler PROPERTY FOLDER "Snippets")	
	ENDIF()

	IF(TARGET_BUILD_PLATFORM STREQUAL "windows" OR TARGET_BUILD_PLATFORM STREQUAL "linux" OR TARGET_BUILD_PLATFORM STREQUAL "mac")
		IF (PX_BUILDVHACD)
			SET_PROPERTY(TARGET SnippetConvexDecomposition PROPERTY FOLDER "Snippets")
		ENDIF()
	ENDIF()

ENDIF()

IF(PX_BUILDPUBLICSAMPLES)
	IF(TARGET_BUILD_PLATFORM STREQUAL "windows")
		# Add samples projects into the solution
		ADD_SUBDIRECTORY(${PHYSX_ROOT_DIR}/samples/compiler/cmake ${CMAKE_CURRENT_BINARY_DIR}/sdk_samples_bin)

		MESSAGE("Added Samples")

		# Set folder PhysX Samples to all SDK Samples projects
		SET_PROPERTY(TARGET Samples PROPERTY FOLDER "PhysX Samples")

		SET_PROPERTY(TARGET SampleToolkit PROPERTY FOLDER "PhysX Samples")
		SET_PROPERTY(TARGET SampleBase PROPERTY FOLDER "PhysX Samples")
		SET_PROPERTY(TARGET SampleFramework PROPERTY FOLDER "PhysX Samples")
		SET_PROPERTY(TARGET SamplePlatform PROPERTY FOLDER "PhysX Samples")
		SET_PROPERTY(TARGET SampleRenderer PROPERTY FOLDER "PhysX Samples")

	ENDIF()
ENDIF()