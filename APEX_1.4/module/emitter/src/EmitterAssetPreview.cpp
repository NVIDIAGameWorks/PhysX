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



#include "EmitterPreview.h"
#include "EmitterAssetPreview.h"

#define ASSET_INFO_XPOS					(-0.9f)	// left position of the asset info
#define ASSET_INFO_YPOS					( 0.9f)	// top position of the asset info
#define DEBUG_TEXT_HEIGHT				(0.4f)	// in screen space

namespace nvidia
{
namespace emitter
{

bool EmitterAssetPreview::isValid() const
{
	return mApexRenderDebug != NULL;
}

void EmitterAssetPreview::drawEmitterPreview(void)
{
	WRITE_ZONE();
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (!mApexRenderDebug)
	{
		return;
	}

	//asset preview init
	if (mGroupID == 0)
	{
		mGroupID = RENDER_DEBUG_IFACE(mApexRenderDebug)->beginDrawGroup(PxMat44(PxIdentity));
		mAsset->mGeom->drawPreview(mScale, mApexRenderDebug);
		RENDER_DEBUG_IFACE(mApexRenderDebug)->endDrawGroup();
	}

	toggleDrawPreview();
	setDrawGroupsPose();
#endif
}

void	EmitterAssetPreview::drawPreviewAssetInfo(void)
{
	if (!mApexRenderDebug)
	{
		return;
	}
		
	char buf[128];
	buf[sizeof(buf) - 1] = 0;

	ApexSimpleString myString;
	ApexSimpleString floatStr;
	uint32_t lineNum = 0;

	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(mApexRenderDebug)->pushRenderState();
	//	RENDER_DEBUG_IFACE(&mApexRenderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::ScreenSpace);
	RENDER_DEBUG_IFACE(mApexRenderDebug)->addToCurrentState(RENDER_DEBUG::DebugRenderState::NoZbuffer);
	RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentTextScale(2.0f);
	RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(mApexRenderDebug)->getDebugColor(DebugColors::Yellow));

	// asset name
	APEX_SPRINTF_S(buf, sizeof(buf) - 1, "%s %s", mAsset->getObjTypeName(), mAsset->getName());
	drawInfoLine(lineNum++, buf);
	lineNum++;
			
	RENDER_DEBUG_IFACE(mApexRenderDebug)->popRenderState();
}


void		EmitterAssetPreview::drawInfoLine(uint32_t lineNum, const char* str)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(lineNum);
	PX_UNUSED(str);
#else
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(mApexRenderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(mApexRenderDebug)->getDebugColor(DebugColors::Green));
	PxVec3 textLocation = mPose.getPosition();
	PxMat44 cameraMatrix = mPreviewScene->getCameraMatrix();
	textLocation += cameraMatrix.column1.getXYZ() * (ASSET_INFO_YPOS - (lineNum * DEBUG_TEXT_HEIGHT));
	RENDER_DEBUG_IFACE(mApexRenderDebug)->debugText(textLocation, str);	
#endif
}


void EmitterAssetPreview::destroy(void)
{
	ApexPreview::destroy();
	delete this;
}

EmitterAssetPreview::~EmitterAssetPreview(void)
{
}

void EmitterAssetPreview::setScale(float scale)
{
	WRITE_ZONE();
	mScale = scale;
	drawEmitterPreview();
}

void EmitterAssetPreview::setPose(const PxMat44& pose)
{
	WRITE_ZONE();
	mPose = pose;
	setDrawGroupsPose();
}

const PxMat44 EmitterAssetPreview::getPose() const
{
	READ_ZONE();
	return mPose;
}

void EmitterAssetPreview::setDrawGroupsPose()
{
	if (mApexRenderDebug)
	{
		RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupPose(mGroupID, mPose);
	}
}

void EmitterAssetPreview::toggleDrawPreview()
{
	if (mApexRenderDebug)
	{
		//asset preview set visibility
		RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupVisible(mGroupID, true);
	}
}


// from RenderDataProvider
void EmitterAssetPreview::lockRenderResources(void)
{
	ApexRenderable::renderDataLock();
}

void EmitterAssetPreview::unlockRenderResources(void)
{
	ApexRenderable::renderDataUnLock();
}

void EmitterAssetPreview::updateRenderResources(bool /*rewriteBuffers*/, void* /*userRenderData*/)
{
	if (mApexRenderDebug)
	{
		mApexRenderDebug->updateRenderResources();
	}
}

// from Renderable.h
void EmitterAssetPreview::dispatchRenderResources(UserRenderer& renderer)
{
	if (mApexRenderDebug)
	{
		drawPreviewAssetInfo();
		mApexRenderDebug->dispatchRenderResources(renderer);
	}
}

PxBounds3 EmitterAssetPreview::getBounds(void) const
{
	READ_ZONE();
	return mApexRenderDebug->getBounds();
}

void EmitterAssetPreview::release(void)
{
	if (mInRelease)
	{
		return;
	}
	mInRelease = true;
	const_cast<EmitterAssetImpl*>(mAsset)->releaseEmitterPreview(*this);
}

	EmitterAssetPreview::EmitterAssetPreview(const EmitterPreviewDesc& pdesc, const EmitterAssetImpl& asset, AssetPreviewScene* previewScene, ApexSDK* myApexSDK) :
		mApexSDK(myApexSDK),
		mScale(pdesc.mScale),
		mAsset(&asset),
		mPreviewScene(previewScene),
		mGroupID(0),
		mApexRenderDebug(0)
	{
#ifndef WITHOUT_DEBUG_VISUALIZE
		setPose(pdesc.mPose);
		drawPreviewAssetInfo();
		drawEmitterPreview();
#endif
	};

}
} // namespace nvidia::apex
