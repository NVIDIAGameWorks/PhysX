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
# Build SamplePlatform
#

IF(NOT DIRECTX_INCLUDE_DIRS)
	IF($ENV{PM_DirectXSDK_VERSION})
		find_package(DirectXSDK $ENV{PM_DirectXSDK_VERSION} CONFIG REQUIRED)
	ELSE()
		IF(EXISTS $ENV{DXSDK_DIR})
			SET(DIRECTX_INCLUDE_DIRS $ENV{DXSDK_DIR}/Include)
		ELSE()
			MESSAGE("For samples compilation please install DXSDK.")
		ENDIF()
	ENDIF()
	SET(DIRECTX_INCLUDE_DIRS ${DIRECTX_INCLUDE_DIRS} CACHE INTERNAL "DirectX SDK include path")
ENDIF()

SET(SAMPLEPLATFORM_COMPILE_DEFS
	# Common to all configurations

	${PHYSX_WINDOWS_COMPILE_DEFS};${SAMPLES_WINDOWS_COMPILE_DEFS};

	$<$<CONFIG:debug>:${PHYSX_WINDOWS_DEBUG_COMPILE_DEFS};>
	$<$<CONFIG:checked>:${PHYSX_WINDOWS_CHECKED_COMPILE_DEFS};>
	$<$<CONFIG:profile>:${PHYSX_WINDOWS_PROFILE_COMPILE_DEFS};>
	$<$<CONFIG:release>:${PHYSX_WINDOWS_RELEASE_COMPILE_DEFS};>
)

SET(SAMPLEPLATFORM_PLATFORM_SOURCES
	${PHYSX_ROOT_DIR}/samples/sampleframework/platform/src/windows/WindowsSamplePlatform.cpp
	${PHYSX_ROOT_DIR}/samples/sampleframework/platform/src/windows/WindowsSampleUserInput.cpp
	${PHYSX_ROOT_DIR}/samples/sampleframework/platform/include/windows/WindowsSampleUserInputIds.h
	${PHYSX_ROOT_DIR}/samples/sampleframework/platform/include/windows/WindowsSampleUserInput.h
	${PHYSX_ROOT_DIR}/samples/sampleframework/platform/include/windows/WindowsSamplePlatform.h

)

SET(SAMPLEPLATFORM_PLATFORM_INCLUDES
	${DIRECTX_INCLUDE_DIRS}
)
