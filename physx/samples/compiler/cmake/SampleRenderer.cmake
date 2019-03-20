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
# Build SampleRenderer common
#

SET(SAMPLERENDERER_DIR ${PHYSX_ROOT_DIR}/samples/sampleframework/renderer)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PHYSX_ROOT_DIR}/samples/${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/SampleRenderer.cmake)

SET(SAMPLERENDERER_FILES
	${SAMPLERENDERER_DIR}/src/Renderer.cpp
	${SAMPLERENDERER_DIR}/src/RendererBoxShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererCapsuleShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererColor.cpp
	${SAMPLERENDERER_DIR}/src/RendererDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererDirectionalLight.cpp
	${SAMPLERENDERER_DIR}/src/RendererDirectionalLightDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererGridShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererIndexBuffer.cpp
	${SAMPLERENDERER_DIR}/src/RendererIndexBufferDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererInstanceBuffer.cpp
	${SAMPLERENDERER_DIR}/src/RendererInstanceBufferDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererLight.cpp
	${SAMPLERENDERER_DIR}/src/RendererLightDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererMaterial.cpp
	${SAMPLERENDERER_DIR}/src/RendererMaterialDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererMaterialInstance.cpp
	${SAMPLERENDERER_DIR}/src/RendererMesh.cpp
	${SAMPLERENDERER_DIR}/src/RendererMeshContext.cpp
	${SAMPLERENDERER_DIR}/src/RendererMeshDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererMeshShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererProjection.cpp
	${SAMPLERENDERER_DIR}/src/RendererShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererSpotLight.cpp
	${SAMPLERENDERER_DIR}/src/RendererSpotLightDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererTarget.cpp
	${SAMPLERENDERER_DIR}/src/RendererTargetDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererTerrainShape.cpp
	${SAMPLERENDERER_DIR}/src/RendererTexture.cpp
	${SAMPLERENDERER_DIR}/src/RendererTexture2D.cpp
	${SAMPLERENDERER_DIR}/src/RendererTexture2DDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererTextureDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererVertexBuffer.cpp
	${SAMPLERENDERER_DIR}/src/RendererVertexBufferDesc.cpp
	${SAMPLERENDERER_DIR}/src/RendererWindow.cpp	
)

SET(SAMPLERENDERER_HEADERS	
	${SAMPLERENDERER_DIR}/include/GLIncludes.h
	${SAMPLERENDERER_DIR}/include/Renderer.h
	${SAMPLERENDERER_DIR}/include/RendererBoxShape.h
	${SAMPLERENDERER_DIR}/include/RendererCapsuleShape.h
	${SAMPLERENDERER_DIR}/include/RendererColor.h
	${SAMPLERENDERER_DIR}/include/RendererConfig.h
	${SAMPLERENDERER_DIR}/include/RendererDesc.h
	${SAMPLERENDERER_DIR}/include/RendererDirectionalLight.h
	${SAMPLERENDERER_DIR}/include/RendererDirectionalLightDesc.h
	${SAMPLERENDERER_DIR}/include/RendererFoundation.h
	${SAMPLERENDERER_DIR}/include/RendererGridShape.h
	${SAMPLERENDERER_DIR}/include/RendererIndexBuffer.h
	${SAMPLERENDERER_DIR}/include/RendererIndexBufferDesc.h
	${SAMPLERENDERER_DIR}/include/RendererInstanceBuffer.h
	${SAMPLERENDERER_DIR}/include/RendererInstanceBufferDesc.h
	${SAMPLERENDERER_DIR}/include/RendererInteropableBuffer.h
	${SAMPLERENDERER_DIR}/include/RendererLight.h
	${SAMPLERENDERER_DIR}/include/RendererLightDesc.h
	${SAMPLERENDERER_DIR}/include/RendererMaterial.h
	${SAMPLERENDERER_DIR}/include/RendererMaterialDesc.h
	${SAMPLERENDERER_DIR}/include/RendererMaterialInstance.h
	${SAMPLERENDERER_DIR}/include/RendererMemoryMacros.h
	${SAMPLERENDERER_DIR}/include/RendererMesh.h
	${SAMPLERENDERER_DIR}/include/RendererMeshContext.h
	${SAMPLERENDERER_DIR}/include/RendererMeshDesc.h
	${SAMPLERENDERER_DIR}/include/RendererMeshShape.h
	${SAMPLERENDERER_DIR}/include/RendererProjection.h
	${SAMPLERENDERER_DIR}/include/RendererShape.h
	${SAMPLERENDERER_DIR}/include/RendererSpotLight.h
	${SAMPLERENDERER_DIR}/include/RendererSpotLightDesc.h
	${SAMPLERENDERER_DIR}/include/RendererTarget.h
	${SAMPLERENDERER_DIR}/include/RendererTargetDesc.h
	${SAMPLERENDERER_DIR}/include/RendererTerrainShape.h
	${SAMPLERENDERER_DIR}/include/RendererTexture.h
	${SAMPLERENDERER_DIR}/include/RendererTexture2D.h
	${SAMPLERENDERER_DIR}/include/RendererTexture2DDesc.h
	${SAMPLERENDERER_DIR}/include/RendererTextureDesc.h
	${SAMPLERENDERER_DIR}/include/RendererUtils.h
	${SAMPLERENDERER_DIR}/include/RendererVertexBuffer.h
	${SAMPLERENDERER_DIR}/include/RendererVertexBufferDesc.h
	${SAMPLERENDERER_DIR}/include/RendererWindow.h
)

ADD_LIBRARY(SampleRenderer STATIC
	${SAMPLERENDERER_PLATFORM_SOURCES}

	${SAMPLERENDERER_FILES}
	${SAMPLERENDERER_HEADERS}
)

TARGET_INCLUDE_DIRECTORIES(SampleRenderer
	PRIVATE ${SAMPLERENDERER_PLATFORM_INCLUDES}
	
	PRIVATE ${SAMPLERENDERER_DIR}/include
	
	PRIVATE ${PHYSX_ROOT_DIR}/samples/sampleframework/platform/include
	PRIVATE ${PHYSX_ROOT_DIR}/samples/sampleframework/framework/include

	PRIVATE ${PHYSX_ROOT_DIR}/include
	PRIVATE ${PHYSX_ROOT_DIR}/source/common/src

)

TARGET_COMPILE_DEFINITIONS(SampleRenderer 
	PRIVATE ${SAMPLERENDERER_COMPILE_DEFS}
)


IF(NV_USE_GAMEWORKS_OUTPUT_DIRS)
	SET_TARGET_PROPERTIES(SampleRenderer PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleRenderer_static_${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleRenderer_static_${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleRenderer_static_${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleRenderer_static_${CMAKE_RELEASE_POSTFIX}"

		ARCHIVE_OUTPUT_NAME_DEBUG "SampleRenderer_static"
		ARCHIVE_OUTPUT_NAME_CHECKED "SampleRenderer_static"
		ARCHIVE_OUTPUT_NAME_PROFILE "SampleRenderer_static"
		ARCHIVE_OUTPUT_NAME_RELEASE "SampleRenderer_static"
	)
ELSE()
	SET_TARGET_PROPERTIES(SampleRenderer PROPERTIES 
		COMPILE_PDB_NAME_DEBUG "SampleRenderer${CMAKE_DEBUG_POSTFIX}"
		COMPILE_PDB_NAME_CHECKED "SampleRenderer${CMAKE_CHECKED_POSTFIX}"
		COMPILE_PDB_NAME_PROFILE "SampleRenderer${CMAKE_PROFILE_POSTFIX}"
		COMPILE_PDB_NAME_RELEASE "SampleRenderer${CMAKE_RELEASE_POSTFIX}"
	)
ENDIF()


TARGET_LINK_LIBRARIES(SampleRenderer 
	PUBLIC SampleToolkit
	PUBLIC ${SAMPLERENDERER_PLATFORM_LINKED_LIBS}
)

IF(PX_GENERATE_SOURCE_DISTRO)	
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLERENDERER_PLATFORM_SOURCES})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLERENDERER_FILES})
	LIST(APPEND SOURCE_DISTRO_FILE_LIST ${SAMPLERENDERER_HEADERS})
ENDIF()