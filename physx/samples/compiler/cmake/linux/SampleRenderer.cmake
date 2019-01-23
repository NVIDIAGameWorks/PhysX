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
# Build SampleRenderer
#

find_package(OpenGL $ENV{PM_OpenGL_VERSION} CONFIG REQUIRED) # Pull in OpenGL and GLUT

SET(SAMPLERENDERER_COMPILE_DEFS
	# Common to all configurations

	${PHYSX_LINUX_COMPILE_DEFS};

	$<$<CONFIG:debug>:${PHYSX_LINUX_DEBUG_COMPILE_DEFS};>
	$<$<CONFIG:checked>:${PHYSX_LINUX_CHECKED_COMPILE_DEFS};>
	$<$<CONFIG:profile>:${PHYSX_LINUX_PROFILE_COMPILE_DEFS};>
	$<$<CONFIG:release>:${PHYSX_LINUX_RELEASE_COMPILE_DEFS};>
)

SET(SAMPLE_NULLRENDERER_DIR ${PHYSX_ROOT_DIR}/samples/sampleframework/renderer/src/null)
SET(SAMPLERENDERER_NULLRENDERER_SOURCES
	${SAMPLE_NULLRENDERER_DIR}/NULLRenderer.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRenderer.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererDirectionalLight.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererDirectionalLight.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererIndexBuffer.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererIndexBuffer.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererInstanceBuffer.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererInstanceBuffer.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererMaterial.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererMaterial.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererMesh.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererMesh.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererSpotLight.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererSpotLight.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererTexture2D.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererTexture2D.h
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererVertexBuffer.cpp
	${SAMPLE_NULLRENDERER_DIR}/NULLRendererVertexBuffer.h
)


SET(SAMPLE_OGLRENDERER_DIR ${PHYSX_ROOT_DIR}/samples/sampleframework/renderer/src/ogl)
SET(SAMPLERENDERER_OGLRENDERER_SOURCES
	${SAMPLE_OGLRENDERER_DIR}/OGLRenderer.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRenderer.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererDirectionalLight.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererDirectionalLight.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererIndexBuffer.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererIndexBuffer.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererInstanceBuffer.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererInstanceBuffer.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererMaterial.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererMaterial.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererMesh.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererMesh.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererSpotLight.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererSpotLight.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererTexture2D.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererTexture2D.h
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererVertexBuffer.cpp
	${SAMPLE_OGLRENDERER_DIR}/OGLRendererVertexBuffer.h
)

SET(SAMPLERENDERER_PLATFORM_SOURCES
	${SAMPLERENDERER_NULLRENDERER_SOURCES}
	${SAMPLERENDERER_OGLRENDERER_SOURCES}
)

SET(SAMPLERENDERER_PLATFORM_LINKED_LIBS SamplePlatform glew Cg CgGL GL X11 Xxf86vm)