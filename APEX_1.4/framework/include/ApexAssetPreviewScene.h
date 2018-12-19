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



#ifndef APEX_ASSET_PREVIEW_SCENE_H
#define APEX_ASSET_PREVIEW_SCENE_H

#include "Apex.h"
#include "ApexResource.h"
#include "PsUserAllocated.h"
#include "ApexSDKImpl.h"
#include "AssetPreviewScene.h"
#include "ModuleIntl.h"
#include "ApexContext.h"
#include "PsMutex.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxScene.h"
#include "PxRenderBuffer.h"
#endif

#include "ApexSceneUserNotify.h"

#include "PsSync.h"
#include "PxTask.h"
#include "PxTaskManager.h"

#include "ApexGroupsFiltering.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace apex
{

class ApexAssetPreviewScene : public AssetPreviewScene, public ApexRWLockable, public UserAllocated
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ApexAssetPreviewScene(ApexSDKImpl* sdk);
	virtual ~ApexAssetPreviewScene() {}

	//Sets the view matrix. Should be called whenever the view matrix needs to be updated.
	virtual void					setCameraMatrix(const PxMat44& viewTransform);
		
	//Returns the view matrix set by the user for the given viewID.
	virtual PxMat44			getCameraMatrix() const;

	virtual void					setShowFullInfo(bool showFullInfo);

	virtual bool					getShowFullInfo() const;

	virtual void					release();

	void							destroy();

private:
	ApexSDKImpl*						mApexSDK;

	PxMat44					mCameraMatrix;				// the pose for the preview rendering
	bool							mShowFullInfo;
};

}
} // end namespace nvidia::apex

#endif // APEX_ASSET_PREVIEW_SCENE_H
