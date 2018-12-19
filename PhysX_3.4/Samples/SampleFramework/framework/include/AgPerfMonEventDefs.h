//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


// This file is used to define a list of AgPerfMon events.
//
// This file is included exclusively by AgPerfMonEventSrcAPI.h
// and by AgPerfMonEventSrcAPI.cpp, for the purpose of building
// an enumeration (enum xx) and an array of strings ()
// that contain the list of events.
//
// This file should only contain event definitions, using the
// DEFINE_EVENT macro.  E.g.:
//
//     DEFINE_EVENT(sample_name_1)
//     DEFINE_EVENT(sample_name_2)
//     DEFINE_EVENT(sample_name_3)


// Statistics from the fluid mesh packet cooker
DEFINE_EVENT(renderFunction)
DEFINE_EVENT(SampleRendererVBwriteBuffer)
DEFINE_EVENT(SampleOnTickPreRender)
DEFINE_EVENT(SampleOnTickPostRender)
DEFINE_EVENT(SampleOnRender)
DEFINE_EVENT(SampleOnDraw)
DEFINE_EVENT(D3D9Renderer_createVertexBuffer)
DEFINE_EVENT(Renderer_render)
DEFINE_EVENT(Renderer_render_depthOnly)
DEFINE_EVENT(Renderer_render_deferred)
DEFINE_EVENT(Renderer_render_lit)
DEFINE_EVENT(Renderer_render_unlit)
DEFINE_EVENT(Renderer_renderMeshes)
DEFINE_EVENT(Renderer_renderDeferredLights)
DEFINE_EVENT(D3D9RendererMesh_renderIndices)
DEFINE_EVENT(D3D9RendererMesh_renderVertices)
DEFINE_EVENT(D3D9Renderer_createIndexBuffer)
DEFINE_EVENT(D3D9RendererMesh_renderVerticesInstanced)
DEFINE_EVENT(D3D9Renderer_createInstanceBuffer)
DEFINE_EVENT(D3D9Renderer_createTexture2D)
DEFINE_EVENT(D3D9Renderer_createTarget)
DEFINE_EVENT(D3D9Renderer_createMaterial)
DEFINE_EVENT(D3D9Renderer_createMesh)
DEFINE_EVENT(D3D9Renderer_createLight)
DEFINE_EVENT(D3D9RendererMesh_renderIndicesInstanced)
DEFINE_EVENT(OGLRenderer_createVertexBuffer)
DEFINE_EVENT(OGLRenderer_createIndexBuffer)
DEFINE_EVENT(OGLRenderer_createInstanceBuffer)
DEFINE_EVENT(OGLRenderer_createTexture2D)
DEFINE_EVENT(OGLRenderer_createTarget)
DEFINE_EVENT(OGLRenderer_createMaterial)
DEFINE_EVENT(OGLRenderer_createMesh)
DEFINE_EVENT(OGLRenderer_createLight)
DEFINE_EVENT(OGLRendererMaterial_compile_vertexProgram)
DEFINE_EVENT(OGLRendererMaterial_load_vertexProgram)
DEFINE_EVENT(OGLRendererMaterial_compile_fragmentProgram)
DEFINE_EVENT(OGLRendererMaterial_load_fragmentProgram)
DEFINE_EVENT(OGLRendererVertexBufferBind)
DEFINE_EVENT(OGLRendererSwapBuffers)
DEFINE_EVENT(writeBufferSemanticStride)
DEFINE_EVENT(writeBufferfixUV)
DEFINE_EVENT(writeBufferConvertFromApex)
DEFINE_EVENT(writeBufferGetFormatSemantic)
DEFINE_EVENT(writeBufferlockSemantic)
DEFINE_EVENT(OGLRendererVertexBufferLock)
DEFINE_EVENT(Renderer_meshRenderLast)
DEFINE_EVENT(Renderer_atEnd)
DEFINE_EVENT(renderMeshesBindMeshContext)
DEFINE_EVENT(renderMeshesFirstIf)
DEFINE_EVENT(renderMeshesSecondIf)
DEFINE_EVENT(renderMeshesThirdIf)
DEFINE_EVENT(renderMeshesForthIf)
DEFINE_EVENT(OGLRendererBindMeshContext)
DEFINE_EVENT(OGLRendererBindMeshcg)
DEFINE_EVENT(cgGLSetMatrixParameter)
DEFINE_EVENT(D3D9RenderVBlock)
DEFINE_EVENT(D3D9RenderVBunlock)
DEFINE_EVENT(D3D9RenderIBlock)
DEFINE_EVENT(D3D9RenderIBunlock)
