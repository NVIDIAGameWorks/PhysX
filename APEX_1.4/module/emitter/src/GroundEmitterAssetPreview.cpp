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



#include "RenderDebugInterface.h"
#include "GroundEmitterPreview.h"
#include "GroundEmitterAssetPreview.h"
#include "ApexPreview.h"
#include "ApexUsingNamespace.h"
#include "WriteCheck.h"
#include "ReadCheck.h"

namespace nvidia
{
namespace emitter
{

void GroundEmitterAssetPreview::drawEmitterPreview(void)
{
	WRITE_ZONE();
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}
	PxVec3 tmpUpDirection(0.0f, 1.0f, 0.0f);

	using RENDER_DEBUG::DebugColors;

	//asset preview init
	if (mGroupID == 0)
	{
		mGroupID = RENDER_DEBUG_IFACE(mApexRenderDebug)->beginDrawGroup(PxMat44(PxIdentity));
		// Cylinder that describes the refresh radius, upDirection, and spawnHeight
		RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(mApexRenderDebug)->getDebugColor(DebugColors::Green));
		RENDER_DEBUG_IFACE(mApexRenderDebug)->debugCylinder(
		    PxVec3(0.0f),
		    tmpUpDirection * (mAsset->getSpawnHeight() + mAsset->getRaycastHeight() + 0.01f),
		    mAsset->getRadius());

		// Ray that describes the raycast spawn height
		RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(mApexRenderDebug)->getDebugColor(DebugColors::Yellow), 
															RENDER_DEBUG_IFACE(mApexRenderDebug)->getDebugColor(DebugColors::Yellow));
		RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentArrowSize(mScale);
		RENDER_DEBUG_IFACE(mApexRenderDebug)->debugRay(tmpUpDirection * mAsset->getRaycastHeight(), PxVec3(0.0f));
		RENDER_DEBUG_IFACE(mApexRenderDebug)->endDrawGroup();
	}

	//asset preview set pose
	PxMat44 groupPose = mPose;
	RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupPose(mGroupID, groupPose);

	//asset preview set visibility
	RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupVisible(mGroupID, true);
#endif
}

void GroundEmitterAssetPreview::destroy(void)
{
	mApexRenderDebug = NULL;
	ApexPreview::destroy();
	delete this;
}

GroundEmitterAssetPreview::~GroundEmitterAssetPreview(void)
{
}

void GroundEmitterAssetPreview::setPose(const PxMat44& pose)
{
	WRITE_ZONE();
	mPose = PxTransform(pose);
	drawEmitterPreview();
}

void GroundEmitterAssetPreview::setScale(float scale)
{
	WRITE_ZONE();
	mScale = scale;
	drawEmitterPreview();
}

const PxMat44 GroundEmitterAssetPreview::getPose() const
{
	READ_ZONE();
	return(mPose);
}

// from RenderDataProvider
void GroundEmitterAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void GroundEmitterAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void GroundEmitterAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	mApexRenderDebug->updateRenderResources();
}

// from Renderable.h
void GroundEmitterAssetPreview::dispatchRenderResources(UserRenderer& renderer)
{
	mApexRenderDebug->dispatchRenderResources(renderer);
}

PxBounds3 GroundEmitterAssetPreview::getBounds(void) const
{
	return mApexRenderDebug->getBounds();
}

void GroundEmitterAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	const_cast<GroundEmitterAssetImpl*>(mAsset)->releaseEmitterPreview(*this);
}

}
} // namespace nvidia::apex
