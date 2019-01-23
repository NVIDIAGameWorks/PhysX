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

IF(NOT DEFINED PHYSX_WINDOWS_COMPILE_DEFS)
	MESSAGE(FATAL ERROR "Samples uses the PhysX compile defs, and they're not defined when they need to be.")
ENDIF()

IF (NOT DEFINED PHYSX_CXX_FLAGS)
	MESSAGE(FATAL ERROR "Samples uses the PhysX CXX flags, and they're not defined when they need to be.")
ENDIF()

IF (MSVC12)
	SET(ADDL_WARNINGS "/wd4512")
ENDIF()

IF(NOT CMAKE_CL_64)
	# disable CUDA stuff for MSVC15 and MSVC14 win32
	IF(NOT PX_GENERATE_GPU_PROJECTS)
		SET(PHYSX_CUDA_DEFS "DISABLE_CUDA_PHYSX")
	ENDIF()
ENDIF()
SET(SAMPLES_WINDOWS_COMPILE_DEFS ${PHYSX_CUDA_DEFS};${PHYSX_LIBTYPE_DEFS})

# Get the CXX Flags from the Cached variables set by the PhysX CMakeLists
SET(CMAKE_CXX_FLAGS "${PHYSX_CXX_FLAGS}  /wd4245 /wd4512 /wd4996 /wd4244 /wd4100 /wd4456 /wd4189 /wd4505 /wd4389 /EHsc ${ADDL_WARNINGS}")

SET(CMAKE_CXX_FLAGS_DEBUG   "${PHYSX_CXX_FLAGS_DEBUG}")
SET(CMAKE_CXX_FLAGS_CHECKED ${PHYSX_CXX_FLAGS_CHECKED})
SET(CMAKE_CXX_FLAGS_PROFILE ${PHYSX_CXX_FLAGS_PROFILE})
SET(CMAKE_CXX_FLAGS_RELEASE ${PHYSX_CXX_FLAGS_RELEASE})

# Build PDBs for all configurations
SET(CMAKE_SHARED_LINKER_FLAGS "/DEBUG")

# Don't redefine PHYSX_WINDOWS_COMPILE_DEFS

# Get the PhysXDevice path

SET(SLN_PHYSXDEVICE_PATH "$ENV{PM_PhysXDevice_PATH}/bin/x86/")

# Include ConfigureFileMT to expose that function
INCLUDE(ConfigureFileMt)


# Include all of the projects
INCLUDE(SampleBase.cmake)
INCLUDE(SampleFramework.cmake)
INCLUDE(SampleRenderer.cmake)
INCLUDE(SamplePlatform.cmake)
INCLUDE(SamplesToolkit.cmake)

SET_PROPERTY(TARGET SampleToolkit PROPERTY FOLDER "PhysX Samples")
SET_PROPERTY(TARGET SampleBase PROPERTY FOLDER "PhysX Samples")
SET_PROPERTY(TARGET SampleFramework PROPERTY FOLDER "PhysX Samples")
SET_PROPERTY(TARGET SamplePlatform PROPERTY FOLDER "PhysX Samples")
SET_PROPERTY(TARGET SampleRenderer PROPERTY FOLDER "PhysX Samples")

IF(PX_BUILDSAMPLES)
	INCLUDE(SampleHelloWorld.cmake)
	INCLUDE(SampleBridges.cmake)
	INCLUDE(SampleCharacterController.cmake)
	INCLUDE(SampleCustomGravity.cmake)
	INCLUDE(SampleDebugRendering.cmake)
	INCLUDE(SampleInstancing.cmake)
	INCLUDE(SampleLargeWorld.cmake)
	INCLUDE(SampleNorthPole.cmake)
	INCLUDE(SampleRaycastCCD.cmake)
	INCLUDE(SampleSubmarine.cmake)
	INCLUDE(SampleVehicle.cmake)
	INCLUDE(Samples.cmake)

	SET_PROPERTY(TARGET SampleHelloWorld PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleBridges PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleCharacterController PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleCustomGravity PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleDebugRendering PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleInstancing PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleLargeWorld PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleNorthPole PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleRaycastCCD PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleSubmarine PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET SampleVehicle PROPERTY FOLDER "PhysX Samples")
	SET_PROPERTY(TARGET Samples PROPERTY FOLDER "PhysX Samples")
ENDIF()

IF(PX_BUILDPUBLICSAMPLES)
	INCLUDE(Samples.cmake)
	SET_PROPERTY(TARGET Samples PROPERTY FOLDER "PhysX Samples")
ENDIF()

