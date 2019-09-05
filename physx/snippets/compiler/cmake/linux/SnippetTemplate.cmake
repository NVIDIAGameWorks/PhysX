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
# Build Snippet linux template
#

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
	SET(SNIPPET_COMPILE_DEFS
		# Common to all configurations

		${PHYSX_LINUX_COMPILE_DEFS};

		$<$<CONFIG:debug>:${PHYSX_LINUX_DEBUG_COMPILE_DEFS};>
		$<$<CONFIG:checked>:${PHYSX_LINUX_CHECKED_COMPILE_DEFS};>
		$<$<CONFIG:profile>:${PHYSX_LINUX_PROFILE_COMPILE_DEFS};>
		$<$<CONFIG:release>:${PHYSX_LINUX_RELEASE_COMPILE_DEFS};>
	)
ELSE()
	SET(SNIPPET_COMPILE_DEFS
		# Common to all configurations

		${PHYSX_LINUX_COMPILE_DEFS};RENDER_SNIPPET;

		$<$<CONFIG:debug>:${PHYSX_LINUX_DEBUG_COMPILE_DEFS};>
		$<$<CONFIG:checked>:${PHYSX_LINUX_CHECKED_COMPILE_DEFS};>
		$<$<CONFIG:profile>:${PHYSX_LINUX_PROFILE_COMPILE_DEFS};>
		$<$<CONFIG:release>:${PHYSX_LINUX_RELEASE_COMPILE_DEFS};>
	)
ENDIF()

SET(SNIPPET_PLATFORM_SOURCES
	${PHYSX_ROOT_DIR}/snippets/snippetcommon/ClassicMain.cpp
)

SET(SNIPPET_PLATFORM_INCLUDES

)

IF(${SNIPPET_NAME} STREQUAL "ArticulationLoader")
	LIST(APPEND SNIPPET_PLATFORM_SOURCES
		${TINYXML2_PATH}/tinyxml2.h
		${TINYXML2_PATH}/tinyxml2.cpp
	)
	LIST(APPEND SNIPPET_PLATFORM_INCLUDES
		${TINYXML2_PATH}
	)
ENDIF()

IF(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
	SET(SNIPPET_PLATFORM_LINKED_LIBS
		rt pthread dl
	)
ELSE()
	SET(SNIPPET_PLATFORM_LINKED_LIBS
		SnippetRender GL GLU GLUT X11 rt pthread dl -Wl,-rpath='${ORIGIN}'
	)
ENDIF()

IF(PX_GENERATE_GPU_STATIC_LIBRARIES)
	IF(${SNIPPET_NAME} STREQUAL "ConvexDecomposition")
		LIST(APPEND SNIPPET_PLATFORM_LINKED_LIBS VHACD)
	ENDIF()
ENDIF()


