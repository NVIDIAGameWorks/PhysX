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



#ifndef __GROUND_EMITTER_ASSET_PREVIEW_H__
#define __GROUND_EMITTER_ASSET_PREVIEW_H__

#include "ApexPreview.h"

#include "ApexSDKIntl.h"
#include "GroundEmitterPreview.h"
#include "RenderDebugInterface.h"
#include "GroundEmitterAssetImpl.h"
#include "ApexRWLockable.h"

namespace nvidia
{
namespace emitter
{

class GroundEmitterAssetPreview : public GroundEmitterPreview, public ApexResource, public ApexPreview, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	GroundEmitterAssetPreview(const GroundEmitterPreviewDesc& pdesc, const GroundEmitterAssetImpl& asset, ApexSDK* myApexSDK, AssetPreviewScene* previewScene) :
		mApexSDK(myApexSDK),
		mApexRenderDebug(0),
		mScale(pdesc.mScale),
		mAsset(&asset),
		mPreviewScene(previewScene),
		mGroupID(0)
	{
#ifndef WITHOUT_DEBUG_VISUALIZE
		setPose(pdesc.mPose);
		drawEmitterPreview();
#endif
	};

	bool                isValid() const
	{
		return mApexRenderDebug != NULL;
	}
	void				drawEmitterPreview(void);
	void				destroy();

	void				setPose(const PxMat44& pose);	// Sets the preview instance's pose.  This may include scaling.
	const PxMat44	getPose() const;

	// from RenderDataProvider
	void lockRenderResources(void);
	void unlockRenderResources(void);
	void updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);

	// from Renderable.h
	void dispatchRenderResources(UserRenderer& renderer);
	PxBounds3 getBounds(void) const;

	// from ApexResource.h
	void	release(void);

private:
	~GroundEmitterAssetPreview();

	AuthObjTypeID					mModuleID;					// the module ID of Emitter.
	UserRenderResourceManager*	mRrm;						// pointer to the users render resource manager
	ApexSDK*						mApexSDK;					// pointer to the APEX SDK
	RenderDebugInterface*				mApexRenderDebug;			// Pointer to the RenderLines class to draw the
	PxTransform				mPose;						// the pose for the preview rendering
	float					mScale;
	const GroundEmitterAssetImpl*       mAsset;
	int32_t                    mGroupID;
	AssetPreviewScene*		mPreviewScene;

	void							setScale(float scale);
};

}
} // end namespace nvidia

#endif // __APEX_EMITTER_ASSET_PREVIEW_H__
