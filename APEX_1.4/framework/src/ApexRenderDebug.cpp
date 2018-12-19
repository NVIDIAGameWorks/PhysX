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



#include "ApexUsingNamespace.h"
#include "ApexRenderDebug.h"
#include "ApexRenderable.h"

#pragma warning(disable:4996)
#pragma warning(disable:4100)
#pragma warning(disable:4189)

#if PX_PHYSICS_VERSION_MAJOR == 3
#include <PxRenderBuffer.h>
#endif // PX_PHYSICS_VERSION_MAJOR

#include "RenderDebugInterface.h"
#include "UserRenderer.h"
#include "ApexSDKImpl.h"
#include "PxIntrinsics.h"
#include "PsString.h"
#include "RenderDebugTyped.h"

namespace nvidia
{
namespace apex
{

#if defined(WITHOUT_DEBUG_VISUALIZE)

RenderDebugInterface* createApexRenderDebug(ApexSDKImpl* /*a*/)
{
	return NULL;
}
void releaseApexRenderDebug(RenderDebugInterface* /*n*/) 
{
}

#else

typedef physx::Array< RenderContext >      RenderContextVector;
typedef physx::Array< UserRenderResource*>    RenderResourceVector;


class ApexRenderDebug : public RenderDebugInterface, public ApexRWLockable, public UserAllocated
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ApexRenderDebug(ApexSDKImpl* sdk, RENDER_DEBUG::RenderDebugInterface* iface, bool useRemote)
	: mRenderDebugIface(iface)
	{
		mApexSDK = sdk;
		if (mRenderDebugUntyped == NULL)
		{
			RENDER_DEBUG::RenderDebug::Desc desc;
			desc.runMode = useRemote ? RENDER_DEBUG::RenderDebug::RM_CLIENT : RENDER_DEBUG::RenderDebug::RM_LOCAL;
			desc.foundation = mApexSDK->getFoundation();
			mRenderDebugUntyped = createRenderDebugExport(desc);
		}
		if (mRenderDebugUntyped)
		{
			mRenderDebug = mRenderDebugUntyped->getRenderDebugTyped();
		}
		else
		{
			PX_ASSERT(0);
		}
		mWireFrameMaterial = INVALID_RESOURCE_ID;
		mSolidShadedMaterial = INVALID_RESOURCE_ID;
		mLastRenderSolidCount = 0;
		mLastRenderLineCount = 0;
		mRenderSolidCount = 0;
		mRenderLineCount = 0;
		mUseDebugRenderable = false;
	}

	virtual RENDER_DEBUG::RenderDebugTyped* getRenderDebugInterface()
	{
		PX_ASSERT(mRenderDebugUntyped != NULL && mRenderDebug != NULL);
		return mRenderDebug;
	}

	virtual ~ApexRenderDebug(void)
	{
		if (mRenderDebug)
		{
			mRenderDebugUntyped->release();
			mRenderDebugUntyped = NULL;
		}
		// APEX specific stuff
		{
			RenderResourceVector::Iterator i;
			for (i = mRenderLineResources.begin(); i != mRenderLineResources.end(); ++i)
			{
				UserRenderResource* resource = (*i);
				PX_ASSERT(resource);
				UserRenderVertexBuffer* vbuffer = resource->getVertexBuffer(0);
				PX_ASSERT(vbuffer);
				mApexSDK->getUserRenderResourceManager()->releaseResource(*resource);
				mApexSDK->getUserRenderResourceManager()->releaseVertexBuffer(*vbuffer);
			}
		}
		{
			RenderResourceVector::Iterator i;
			for (i = mRenderSolidResources.begin(); i != mRenderSolidResources.end(); ++i)
			{
				UserRenderResource* resource = (*i);
				PX_ASSERT(resource);
				UserRenderVertexBuffer* vbuffer = resource->getVertexBuffer(0);
				PX_ASSERT(vbuffer);
				mApexSDK->getUserRenderResourceManager()->releaseResource(*resource);
				mApexSDK->getUserRenderResourceManager()->releaseVertexBuffer(*vbuffer);
			}
		}
		mApexSDK->getInternalResourceProvider()->releaseResource(mWireFrameMaterial);
		mApexSDK->getInternalResourceProvider()->releaseResource(mSolidShadedMaterial);
	};

	/*
	\brief Method to support rendering to a legacy PhysX SDK DebugRenderable object instead
	of to the APEX Render Resources API (i.e.: Renderable).

	This method is used to enable or disable the use of a legacy DebugRenderable.  When enabled,
	use the getDebugRenderable() method to get a legacy DebugRenerable object that will contain
	all the debug output.
	*/
	virtual void	setUseDebugRenderable(bool state)
	{
		mUseDebugRenderable = state;
		if (state == false)
		{
#if PX_PHYSICS_VERSION_MAJOR == 3
			mPxDebugTriangles.clear();
			mPxDebugLines.clear();
			mPxDebugTrianglesScreenSpace.clear();
			mPxDebugLinesScreenSpace.clear();
#endif
		}
	}



#if PX_PHYSICS_VERSION_MAJOR == 3

	/*
	\brief Method to support rendering to a legacy PhysX SDK PxRenderBuffer object instead
	of to the APEX Render Resources API (i.e.: Renderable).

	When enabled with a call to setUseDebugRenderable(true), this method will return a legacy
	DebugRenderable object that contains all of the output of the RenderDebug class.
	*/
	virtual void	getRenderBuffer(PhysXRenderBuffer& renderable)
	{
		renderable.mNbPoints = 0;
		renderable.mPoints = NULL;
		renderable.mNbLines = mPxDebugLines.size();
		renderable.mLines = renderable.mNbLines ? &mPxDebugLines[0] : NULL;
		renderable.mNbTriangles = mPxDebugTriangles.size();
		renderable.mTriangles = renderable.mNbTriangles ? &mPxDebugTriangles[0] : NULL;
		renderable.mNbTexts = 0;
		renderable.mTexts = NULL;
	}

	/*
	\brief Method to support rendering to a legacy PhysX SDK PxRenderBuffer object instead
	of to the APEX Render Resources API (i.e.: Renderable).

	When enabled with a call to setUseDebugRenderable(true), this method will return a legacy
	DebugRenderable object that contains all of the output of the RenderDebug class.
	*/
	virtual void	getRenderBufferScreenSpace(PhysXRenderBuffer& renderable)
	{
		renderable.mNbPoints = 0;
		renderable.mPoints = NULL;
		renderable.mNbLines = mPxDebugLinesScreenSpace.size();
		renderable.mLines = renderable.mNbLines ? &mPxDebugLinesScreenSpace[0] : NULL;
		renderable.mNbTriangles = mPxDebugTrianglesScreenSpace.size();
		renderable.mTriangles = renderable.mNbTriangles ? &mPxDebugTrianglesScreenSpace[0] : NULL;
		renderable.mNbTexts = 0;
		renderable.mTexts = NULL;
	}


	virtual void	addDebugRenderable(const physx::PxRenderBuffer& renderBuffer)
	{
		// Points
		mRenderDebug->pushRenderState();

		const uint32_t color          = mRenderDebug->getCurrentColor();;
		const uint32_t arrowColor     = mRenderDebug->getCurrentArrowColor();

		const uint32_t numPoints      = renderBuffer.getNbPoints();
		const physx::PxDebugPoint* points = renderBuffer.getPoints();
		for (uint32_t i = 0; i < numPoints; i++)
		{
			const physx::PxDebugPoint& point = points[i];
			mRenderDebug->setCurrentColor(point.color, arrowColor);
			mRenderDebug->debugPoint(point.pos, 0.01f);
		}

		// Lines
		const uint32_t numLines     = renderBuffer.getNbLines();
		const physx::PxDebugLine* lines = renderBuffer.getLines();
		for (uint32_t i = 0; i < numLines; i++)
		{
			const physx::PxDebugLine& line = lines[i];
			mRenderDebug->debugGradientLine(line.pos0, line.pos1, line.color0, line.color1);
		}

		// Triangles
		const uint32_t numTriangles         = renderBuffer.getNbTriangles();
		const physx::PxDebugTriangle* triangles = renderBuffer.getTriangles();
		for (uint32_t i = 0; i < numTriangles; i++)
		{
			const physx::PxDebugTriangle& triangle = triangles[i];
			mRenderDebug->debugGradientTri(triangle.pos0, triangle.pos1, triangle.pos2, triangle.color0, triangle.color1, triangle.color2);
		}

		// Texts
		const uint32_t numTexts				= renderBuffer.getNbTexts();
		const physx::PxDebugText* texts			= renderBuffer.getTexts();
		for (uint32_t i = 0; i < numTexts; i++)
		{
			const physx::PxDebugText& text = texts[i];
			mRenderDebug->debugText(text.position, text.string);
		}

		mRenderDebug->setCurrentColor(color, arrowColor);

		mRenderDebug->popRenderState();
	}
#endif // PX_PHYSICS_VERSION_MAJOR == 3

	/**
	\brief Release an object instance.

	Calling this will unhook the class and delete it from memory.
	You should not keep any reference to this class instance after calling release
	*/
	virtual void release()
	{
		delete this;
	}

	virtual void lockRenderResources()
	{

	}

	/**
	\brief Unlocks the renderable data of this Renderable actor.

	See locking semantics for xRenderDataProvider::lockRenderResources().
	*/
	virtual void unlockRenderResources()
	{

	}

	/**
	\brief Update the renderable data of this Renderable actor.

	When called, this method will use the UserRenderResourceManager interface to inform the user
	about its render resource needs.  It will also call the writeBuffer() methods of various graphics
	buffers.  It must be called by the user each frame before any calls to dispatchRenderResources().
	If the actor is not being rendered, this function may also be skipped.
	*/
	virtual void updateRenderResources(bool /*rewriteBuffers*/ = false, void* /*userRenderData*/ = 0)
	{
		URR_SCOPE;

		mRenderSolidContexts.clear();
		mRenderLineContexts.clear();

		// Free up the line draw vertex buffer resources if the debug renderer is now using a lot less memory than the last frame.
		if (mRenderLineCount < (mLastRenderLineCount / 2))
		{
			RenderResourceVector::Iterator i;
			for (i = mRenderLineResources.begin(); i != mRenderLineResources.end(); ++i)
			{
				UserRenderResource* resource = (*i);
				PX_ASSERT(resource);
				UserRenderVertexBuffer* vbuffer = resource->getVertexBuffer(0);
				PX_ASSERT(vbuffer);
				mApexSDK->getUserRenderResourceManager()->releaseResource(*resource);
				mApexSDK->getUserRenderResourceManager()->releaseVertexBuffer(*vbuffer);
			}
			mRenderLineResources.clear();
		}
		// free up the solid shaded triangle vertex buffers if the debug renderer is now using a lot less memory than the last frame.
		if (mRenderSolidCount < mLastRenderSolidCount / 2) // if we have less than 1/2 the number of solid shaded triangles we did last frame, free up the resources.
		{
			RenderResourceVector::Iterator i;
			for (i = mRenderSolidResources.begin(); i != mRenderSolidResources.end(); ++i)
			{
				UserRenderResource* resource = (*i);
				PX_ASSERT(resource);
				UserRenderVertexBuffer* vbuffer = resource->getVertexBuffer(0);
				PX_ASSERT(vbuffer);
				mApexSDK->getUserRenderResourceManager()->releaseResource(*resource);
				mApexSDK->getUserRenderResourceManager()->releaseVertexBuffer(*vbuffer);
			}
			mRenderSolidResources.clear();
		}

		mLastRenderSolidCount = mRenderSolidCount;
		mLastRenderLineCount  = mRenderLineCount;
		mRenderSolidCount = 0;
		mRenderLineCount = 0;

#if PX_PHYSICS_VERSION_MAJOR == 3
		mPxDebugLines.clear();
		mPxDebugTriangles.clear();
		mPxDebugLinesScreenSpace.clear();
		mPxDebugTrianglesScreenSpace.clear();
#endif
	}

	virtual void dispatchRenderResources(UserRenderer& renderer)
	{
		mRenderDebug->render(1.0f / 60.0f, mRenderDebugIface);
	}

	virtual void debugRenderLines(uint32_t lcount, const RENDER_DEBUG::RenderDebugVertex* vertices, bool /*useZ*/, bool isScreenSpace)
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
			if (mUseDebugRenderable)
			{
				for (uint32_t i = 0; i < lcount; i++)
				{
					PxVec3 v1( vertices[0].mPos[0], vertices[0].mPos[1], vertices[0].mPos[2] );
					PxVec3 v2( vertices[1].mPos[0], vertices[1].mPos[1], vertices[1].mPos[2] );
					PxDebugLine l(v1,v2,vertices->mColor);
					l.color1 = vertices[1].mColor;
					if ( isScreenSpace )
					{
						mPxDebugLinesScreenSpace.pushBack(l);
					}
					else
					{
						mPxDebugLines.pushBack(l);
					}
					vertices += 2;
				}
			}
			else
#endif
		{
			mRenderLineCount += (lcount * 2);
			if (mWireFrameMaterial == INVALID_RESOURCE_ID)
			{
				const char* mname = mApexSDK->getWireframeMaterial();
				ResID name_space = mApexSDK->getInternalResourceProvider()->createNameSpace(APEX_MATERIALS_NAME_SPACE);
				mWireFrameMaterial = mApexSDK->getInternalResourceProvider()->createResource(name_space, mname, true);
			}

			const uint32_t MAX_LINE_VERTEX = 2048;

			PX_ASSERT((lcount * 2) <= MAX_LINE_VERTEX);

			uint32_t rcount = (uint32_t)mRenderLineContexts.size();
			RenderContext context;

			UserRenderResource* resource;

			if (rcount < mRenderLineResources.size())
			{
				resource = mRenderLineResources[rcount];
			}
			else
			{
				UserRenderResourceDesc resourceDesc;
				UserRenderVertexBufferDesc vbdesc;
				vbdesc.hint = RenderBufferHint::DYNAMIC;
				vbdesc.buffersRequest[RenderVertexSemantic::POSITION] = RenderDataFormat::FLOAT3;
				vbdesc.buffersRequest[RenderVertexSemantic::COLOR]    = RenderDataFormat::B8G8R8A8;
				vbdesc.maxVerts = MAX_LINE_VERTEX;
				resourceDesc.cullMode = RenderCullMode::NONE;

				for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
				{
					PX_ASSERT(vbdesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, vbdesc.buffersRequest[i]));
				}

				UserRenderVertexBuffer* vb = mApexSDK->getUserRenderResourceManager()->createVertexBuffer(vbdesc);
				UserRenderVertexBuffer* vertexBuffers[1] = { vb };
				resourceDesc.vertexBuffers = vertexBuffers;
				resourceDesc.numVertexBuffers = 1;

				resourceDesc.primitives = RenderPrimitiveType::LINES;

				resource = mApexSDK->getUserRenderResourceManager()->createResource(resourceDesc);
				resource->setMaterial(mApexSDK->getInternalResourceProvider()->getResource(mWireFrameMaterial));
				mRenderLineResources.pushBack(resource);
			}

			UserRenderVertexBuffer* vb = resource->getVertexBuffer(0);

			resource->setVertexBufferRange(0, lcount * 2);

			RenderVertexBufferData writeData;
			writeData.setSemanticData(RenderVertexSemantic::POSITION, vertices[0].mPos,   sizeof(RENDER_DEBUG::RenderDebugVertex), RenderDataFormat::FLOAT3);
			writeData.setSemanticData(RenderVertexSemantic::COLOR,   &vertices[0].mColor, sizeof(RENDER_DEBUG::RenderDebugVertex), RenderDataFormat::B8G8R8A8);
			vb->writeBuffer(writeData, 0, lcount * 2);

			context.isScreenSpace = isScreenSpace;
			context.local2world = PxMat44(PxIdentity);
			context.renderResource = 0;
			context.world2local = PxMat44(PxIdentity);

			mRenderLineContexts.pushBack(context);
		}

	}

	virtual void debugRenderTriangles(uint32_t tcount, const RENDER_DEBUG::RenderDebugSolidVertex* vertices, bool /*useZ*/, bool isScreenSpace)
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
			if (mUseDebugRenderable)
			{
				for (uint32_t i = 0; i < tcount; i++)
				{
					PxVec3 v1( vertices[0].mPos[0], vertices[0].mPos[1], vertices[0].mPos[2] );
					PxVec3 v2( vertices[1].mPos[0], vertices[1].mPos[1], vertices[1].mPos[2] );
					PxVec3 v3( vertices[2].mPos[0], vertices[2].mPos[1], vertices[2].mPos[2] );
					PxDebugTriangle t( v1,v2, v3, vertices->mColor );
					t.color1 = vertices[1].mColor;
					t.color2 = vertices[2].mColor;
					if ( isScreenSpace )
					{
						mPxDebugTrianglesScreenSpace.pushBack(t);
					}
					else
					{
						mPxDebugTriangles.pushBack(t);
					}
					vertices += 3;
				}
			}
			else
#endif
		{

			mRenderSolidCount += (tcount * 3);

			if (mSolidShadedMaterial == INVALID_RESOURCE_ID)
			{
				const char* mname = mApexSDK->getSolidShadedMaterial();
				ResID name_space = mApexSDK->getInternalResourceProvider()->createNameSpace(APEX_MATERIALS_NAME_SPACE);
				mSolidShadedMaterial = mApexSDK->getInternalResourceProvider()->createResource(name_space, mname, true);
			}

			const uint32_t MAX_SOLID_VERTEX = 2048;
			PX_ASSERT((tcount * 3) <= MAX_SOLID_VERTEX);

			uint32_t rcount = (uint32_t)mRenderSolidContexts.size();
			RenderContext context;

			UserRenderResource* resource;

			if (rcount < mRenderSolidResources.size())
			{
				resource = mRenderSolidResources[rcount];
			}
			else
			{
				UserRenderResourceDesc renderResourceDesc;
				UserRenderVertexBufferDesc vbdesc;
				vbdesc.hint = RenderBufferHint::DYNAMIC;
				vbdesc.buffersRequest[RenderVertexSemantic::POSITION] = RenderDataFormat::FLOAT3;
				vbdesc.buffersRequest[RenderVertexSemantic::NORMAL] = RenderDataFormat::FLOAT3;
				vbdesc.buffersRequest[RenderVertexSemantic::COLOR]    = RenderDataFormat::B8G8R8A8;
				vbdesc.maxVerts = MAX_SOLID_VERTEX;
				renderResourceDesc.cullMode = RenderCullMode::COUNTER_CLOCKWISE;

				for (uint32_t i = 0; i < RenderVertexSemantic::NUM_SEMANTICS; i++)
				{
					PX_ASSERT(vbdesc.buffersRequest[i] == RenderDataFormat::UNSPECIFIED || vertexSemanticFormatValid((RenderVertexSemantic::Enum)i, vbdesc.buffersRequest[i]));
				}

				UserRenderVertexBuffer* vb = mApexSDK->getUserRenderResourceManager()->createVertexBuffer(vbdesc);
				UserRenderVertexBuffer* vertexBuffers[1] = { vb };
				renderResourceDesc.vertexBuffers = vertexBuffers;
				renderResourceDesc.numVertexBuffers = 1;

				renderResourceDesc.primitives = RenderPrimitiveType::TRIANGLES;
				resource = mApexSDK->getUserRenderResourceManager()->createResource(renderResourceDesc);
				resource->setMaterial(mApexSDK->getInternalResourceProvider()->getResource(mSolidShadedMaterial));
				mRenderSolidResources.pushBack(resource);
			}

			UserRenderVertexBuffer* vb = resource->getVertexBuffer(0);

			resource->setVertexBufferRange(0, tcount * 3);

			RenderVertexBufferData writeData;
			writeData.setSemanticData(RenderVertexSemantic::POSITION, vertices[0].mPos,    sizeof(RENDER_DEBUG::RenderDebugSolidVertex), RenderDataFormat::FLOAT3);
			writeData.setSemanticData(RenderVertexSemantic::NORMAL,   vertices[0].mNormal, sizeof(RENDER_DEBUG::RenderDebugSolidVertex), RenderDataFormat::FLOAT3);
			writeData.setSemanticData(RenderVertexSemantic::COLOR,   &vertices[0].mColor,  sizeof(RENDER_DEBUG::RenderDebugSolidVertex), RenderDataFormat::B8G8R8A8);
			vb->writeBuffer(writeData, 0, tcount * 3);

			context.isScreenSpace = isScreenSpace;
			context.local2world = PxMat44(PxIdentity);
			context.renderResource = 0;
			context.world2local = PxMat44(PxIdentity);

			mRenderSolidContexts.pushBack(context);
		}
	}

	virtual PxBounds3 getBounds() const { return PxBounds3(); }

	virtual void releaseRenderDebug(void)
	{
		release();
	}
private:
	static RENDER_DEBUG::RenderDebug*	mRenderDebugUntyped;	
	RENDER_DEBUG::RenderDebugTyped*		mRenderDebug;
	uint32_t                             mRenderSolidCount;
	uint32_t                             mRenderLineCount;

	uint32_t                             mLastRenderLineCount;
	uint32_t                             mLastRenderSolidCount;
	ApexSDKImpl*	mApexSDK;
	ResID                           mWireFrameMaterial;
	ResID                           mSolidShadedMaterial;
	RenderResourceVector              mRenderLineResources;
	RenderContextVector               mRenderLineContexts;

	RenderResourceVector              mRenderSolidResources;
	RenderContextVector               mRenderSolidContexts;

	bool							  mUseDebugRenderable;
#if PX_PHYSICS_VERSION_MAJOR == 3
	physx::Array<PxDebugLine>		  mPxDebugLines;
	physx::Array<PxDebugTriangle>	  mPxDebugTriangles;
	physx::Array<PxDebugLine>		  mPxDebugLinesScreenSpace;
	physx::Array<PxDebugTriangle>	  mPxDebugTrianglesScreenSpace;
#endif
	RENDER_DEBUG::RenderDebugInterface* mRenderDebugIface;
};

RENDER_DEBUG::RenderDebug*	ApexRenderDebug::mRenderDebugUntyped = NULL;

RenderDebugInterface* createApexRenderDebug(ApexSDKImpl* a, RENDER_DEBUG::RenderDebugInterface* iface, bool useRemote)
{
	return PX_NEW(ApexRenderDebug)(a, iface, useRemote);
}

void releaseApexRenderDebug(RenderDebugInterface* n)
{
	delete static_cast< ApexRenderDebug*>(n);
}

#endif // WITHOUT_DEBUG_VISUALIZE

}
} // end namespace nvidia::apex
