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



#ifndef ASSET_PREVIEW_SCENE_H
#define ASSET_PREVIEW_SCENE_H

/*!
\file
\brief classes Scene, SceneStats, SceneDesc
*/

#include "ApexDesc.h"
#include "Renderable.h"
#include "Context.h"
#include "foundation/PxVec3.h"
#include <ApexDefs.h>

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxFiltering.h"
#endif
namespace physx
{
	class PxActor;
	class PxScene;
	class PxRenderBuffer;
	
	class PxCpuDispatcher;
	class PxGpuDispatcher;
	class PxTaskManager;
	class PxBaseTask;
}

namespace NvParameterized
{
class Interface;
}

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


/**
\brief An APEX class for 
*/
class AssetPreviewScene : public ApexInterface
{
public:
	/**
	\brief Sets the view matrix. Should be called whenever the view matrix needs to be updated.
	*/
	virtual void					setCameraMatrix(const PxMat44& viewTransform) = 0;

	/**
	\brief Returns the view matrix set by the user for the given viewID.
	*/
	virtual PxMat44					getCameraMatrix() const = 0;

	/**
	\brief Sets whether the asset preview should simply show asset names or many other parameter values
	*/
	virtual void					setShowFullInfo(bool showFullInfo) = 0;

	/**
	\brief Get the bool which determines whether the asset preview shows just asset names or parameter values
	*/
	virtual bool					getShowFullInfo() const = 0;
};


PX_POP_PACK
}
} // end namespace nvidia::apex

#endif // ASSET_PREVIEW_SCENE_H
