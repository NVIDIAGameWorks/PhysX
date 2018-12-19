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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#ifndef APEX_H
#define APEX_H

/**
\file
\brief The top level include file for all of the APEX API.

Include this whenever you want to use anything from the APEX API
in a source file.
*/

#include "foundation/Px.h"

#include "ApexUsingNamespace.h"


namespace NvParameterized
{
class Traits;
class Interface;
class Serializer;
};


#include "foundation/PxPreprocessor.h"
#include "foundation/PxSimpleTypes.h"
#include "foundation/PxAssert.h"
#include "foundation/PxBounds3.h"
#include "foundation/PxVec2.h"
#include "foundation/PxVec3.h"


// APEX public API:
// In general, APEX public headers will not be included 'alone', so they
// should not include their prerequisites.

#include "ApexDefs.h"
#include "ApexDesc.h"
#include "ApexInterface.h"
#include "ApexSDK.h"

#include "Actor.h"
#include "Context.h"
#include "ApexNameSpace.h"
#include "PhysXObjectDesc.h"
#include "RenderDataProvider.h"
#include "Renderable.h"
#include "AssetPreview.h"
#include "Asset.h"
#include "RenderContext.h"
#include "Scene.h"
#include "ApexSDKCachedData.h"
#include "IProgressListener.h"
#include "Module.h"
#include "IosAsset.h"

#include "RenderDataFormat.h"
#include "RenderBufferData.h"
#include "UserRenderResourceManager.h"
#include "UserRenderVertexBufferDesc.h"
#include "UserRenderInstanceBufferDesc.h"
#include "UserRenderSpriteBufferDesc.h"
#include "UserRenderIndexBufferDesc.h"
#include "UserRenderBoneBufferDesc.h"
#include "UserRenderResourceDesc.h"
#include "UserRenderSurfaceBufferDesc.h"
#include "UserRenderSurfaceBuffer.h"
#include "UserRenderResource.h"
#include "UserRenderVertexBuffer.h"
#include "UserRenderInstanceBuffer.h"
#include "UserRenderSpriteBuffer.h"
#include "UserRenderIndexBuffer.h"
#include "UserRenderBoneBuffer.h"
#include "UserRenderer.h"

#include "VertexFormat.h"
#include "RenderMesh.h"
#include "RenderMeshActorDesc.h"
#include "RenderMeshActor.h"
#include "RenderMeshAsset.h"
#include "ResourceCallback.h"
#include "ResourceProvider.h"

#endif // APEX_H
