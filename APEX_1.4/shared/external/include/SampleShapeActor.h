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


#ifndef __SAMPLE_SHAPE_ACTOR_H__
#define __SAMPLE_SHAPE_ACTOR_H__

#include "SampleActor.h"
#include "ApexDefs.h"
#include "PxActor.h"
#include "PxRigidDynamic.h"
#include "PxScene.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "ScopedPhysXLock.h"
#endif
#include "RenderDebugInterface.h"
#include <Renderer.h>
#include <RendererMeshContext.h>


#if PX_PHYSICS_VERSION_MAJOR == 0
namespace physx
{
namespace apex
{
/**
\brief A scoped object for acquiring/releasing read/write lock of a PhysX scene
*/
class ScopedPhysXLockRead
{
public:
	/**
	\brief Ctor
	*/
	ScopedPhysXLockRead(PxScene *scene,const char *fileName,int lineno) : mScene(scene)
	{
		if ( mScene )
		{
			mScene->lockRead(fileName, (physx::PxU32)lineno);
		}
	}
	~ScopedPhysXLockRead()
	{
		if ( mScene )
		{
			mScene->unlockRead();
		}
	}
private:
	PxScene* mScene;
};

/**
\brief A scoped object for acquiring/releasing read/write lock of a PhysX scene
*/
class ScopedPhysXLockWrite
{
public:
	/**
	\brief Ctor
	*/
	ScopedPhysXLockWrite(PxScene *scene,const char *fileName,int lineno) : mScene(scene)
	{
		if ( mScene ) 
		{
			mScene->lockWrite(fileName, (physx::PxU32)lineno);
		}
	}
	~ScopedPhysXLockWrite()
	{
		if ( mScene )
		{
			mScene->unlockWrite();
		}
	}
private:
	PxScene* mScene;
};

}; // end apx namespace
}; // end physx namespace


#if defined(_DEBUG) || defined(PX_CHECKED)
#define SCOPED_PHYSX_LOCK_WRITE(x) physx::apex::ScopedPhysXLockWrite _wlock(x,__FILE__,__LINE__);
#else
#define SCOPED_PHYSX_LOCK_WRITE(x) physx::apex::ScopedPhysXLockWrite _wlock(x,"",__LINE__);
#endif

#if defined(_DEBUG) || defined(PX_CHECKED)
#define SCOPED_PHYSX_LOCK_READ(x) physx::apex::ScopedPhysXLockRead _rlock(x,__FILE__,__LINE__);
#else
#define SCOPED_PHYSX_LOCK_READ(x) physx::apex::ScopedPhysXLockRead _rlock(x,"",__LINE__);
#endif

#endif // PX_PHYSICS_VERSION_MAJOR



class SampleShapeActor : public SampleFramework::SampleActor
{
public:
	SampleShapeActor(nvidia::apex::RenderDebugInterface* rdebug)
		: mBlockId(-1)
		, mApexRenderDebug(rdebug)
		, mRenderer(NULL)
		, mPhysxActor(NULL)
	{
	}

	virtual ~SampleShapeActor(void)
	{
		if (mApexRenderDebug != NULL)
		{
			RENDER_DEBUG_IFACE(mApexRenderDebug)->reset(mBlockId);
		}

		if (mPhysxActor)
		{
			SCOPED_PHYSX_LOCK_WRITE(mPhysxActor->getScene());
			mPhysxActor->release();
		}
	}

	physx::PxTransform getPose() const
	{
		return physx::PxTransform(mTransform);
	}

	void setPose(const physx::PxTransform& pose)
	{
		mTransform = physx::PxMat44(pose);
		if (mPhysxActor)
		{
			SCOPED_PHYSX_LOCK_WRITE(mPhysxActor->getScene());
			if (physx::PxRigidDynamic* rd = mPhysxActor->is<physx::PxRigidDynamic>())
			{
				rd->setGlobalPose(this->convertToPhysicalCoordinates(mTransform));
			}
		}
		if (mApexRenderDebug != NULL)
		{
			RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupPose(mBlockId, mTransform);
		}
	}

	virtual void tick(float dtime, bool rewriteBuffers = false)
	{
		if (mPhysxActor)
		{
			physx::PxRigidDynamic* rd = mPhysxActor->is<physx::PxRigidDynamic>();
			SCOPED_PHYSX_LOCK_READ(mPhysxActor->getScene());
			if (rd && !rd->isSleeping())
			{
				mTransform = this->convertToGraphicalCoordinates(rd->getGlobalPose());
				if (mApexRenderDebug != NULL)
				{
					RENDER_DEBUG_IFACE(mApexRenderDebug)->setDrawGroupPose(mBlockId, mTransform);
				}
			}
		}
	}

	physx::PxActor* getPhysXActor()
	{
		return mPhysxActor;
	}

	virtual void render(bool /*rewriteBuffers*/ = false)
	{
		if (mRenderer)
		{
			mRenderer->queueMeshForRender(mRendererMeshContext);
		}
	}

protected:
	int32_t								mBlockId;
	nvidia::apex::RenderDebugInterface*			mApexRenderDebug;
	SampleRenderer::Renderer*			mRenderer;
	SampleRenderer::RendererMeshContext	mRendererMeshContext;
	physx::PxMat44						mTransform; 
	physx::PxActor*						mPhysxActor;

private:
	virtual physx::PxMat44		convertToGraphicalCoordinates(const physx::PxTransform & physicsPose) const
	{
		return physx::PxMat44(physicsPose);
	}

	virtual physx::PxTransform	convertToPhysicalCoordinates(const physx::PxMat44 & graphicsPose) const
	{
		return physx::PxTransform(graphicsPose);
	}
};

#endif
