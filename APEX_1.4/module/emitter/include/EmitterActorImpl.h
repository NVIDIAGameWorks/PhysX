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



#ifndef __EMITTER_ACTOR_IMPL_H__
#define __EMITTER_ACTOR_IMPL_H__

#include "Apex.h"

#include "EmitterActor.h"
#include "ApexActor.h"
#include "EmitterScene.h"
#include "EmitterLodParamDesc.h"
#include "ApexRWLockable.h"
#include "ApexRand.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace apex
{
class EmitterAsset;
}
namespace emitter
{
class EmitterAssetImpl;
class EmitterGeomBase;

class EmitterActorImpl : public EmitterActor, public EmitterActorBase, public ApexResourceInterface, public ApexResource, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	/* EmitterActor methods */
	EmitterActorImpl(const EmitterActorDesc&, EmitterAssetImpl&, ResourceList&, EmitterScene&);
	~EmitterActorImpl();

	EmitterAsset*             getEmitterAsset() const;
	PxMat44					getGlobalPose() const
	{
		READ_ZONE();
		return PxMat44(mPose);
	}
	void							setCurrentPose(const PxTransform& pose)
	{
		WRITE_ZONE();
		mPose = pose;
	}
	void							setCurrentPosition(const PxVec3& pos)
	{
		WRITE_ZONE();
		mPose.p = pos;
	}
	Renderable* 				getRenderable()
	{
		READ_ZONE();
		return NULL;
	}
	Actor* 					getActor()
	{
		READ_ZONE();
		return this;
	}
	void							removeActorAtIndex(uint32_t index);

	void							getLodRange(float& min, float& max, bool& intOnly) const;
	float					getActiveLod() const;
	void							forceLod(float lod);
	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		WRITE_ZONE();
		ApexActor::setEnableDebugVisualization(state);
	}

	EmitterGeomExplicit* 			isExplicitGeom();

	const EmitterLodParamDesc&	getLodParamDesc() const
	{
		READ_ZONE();
		return mLodParams;
	}
	void							setLodParamDesc(const EmitterLodParamDesc& d);

	/* ApexResourceInterface, ApexResource */
	void				            release();
	uint32_t				    getListIndex() const
	{
		READ_ZONE();
		return m_listIndex;
	}
	void				            setListIndex(class ResourceList& list, uint32_t index)
	{
		WRITE_ZONE();
		m_list = &list;
		m_listIndex = index;
	}

	/* EmitterActorBase */
	void                            destroy();
	Asset*		            getOwner() const;
	void							visualize(RenderDebugInterface& renderDebug);

	void							setPhysXScene(PxScene* s)
	{
		WRITE_ZONE();
		mPxScene = s;
	}
	PxScene*						getPhysXScene() const
	{
		READ_ZONE();
		return mPxScene;
	}
	PxScene*						mPxScene;

	void							submitTasks();
	void							setTaskDependencies();
	void							fetchResults();

	void							setDensity(const float& r)
	{
		WRITE_ZONE();
		mDensity = r;
	}

	void							setRate(const float& r)
	{
		WRITE_ZONE();
		mRate = r;
	}

	void							setVelocityLow(const PxVec3& r)
	{
		WRITE_ZONE();
		mVelocityLow = r;
	}

	void							setVelocityHigh(const PxVec3& r)
	{
		WRITE_ZONE();
		mVelocityHigh = r;
	}

	void							setLifetimeLow(const float& r)
	{
		WRITE_ZONE();
		mLifetimeLow = r;
	}

	void							setLifetimeHigh(const float& r)
	{
		WRITE_ZONE();
		mLifetimeHigh = r;
	}

	void							getRate(float& r) const
	{
		READ_ZONE();
		r = mRate;
	}

	void							setOverlapTestCollisionGroups(uint32_t g)
	{
		WRITE_ZONE();
		mOverlapTestCollisionGroups = g;
	}

	uint32_t					getOverlapTestCollisionGroups() const
	{
		READ_ZONE();
		return mOverlapTestCollisionGroups;
	}

	virtual void				setDensityGridPosition(const PxVec3 &pos);

	virtual void setApexEmitterValidateCallback(EmitterValidateCallback *callback) 
	{
		WRITE_ZONE();
		mEmitterValidateCallback = callback;
	}

	PX_DEPRECATED void setObjectScale(float scale)
	{
		WRITE_ZONE();
		setCurrentScale(scale);
	}

	PX_DEPRECATED float getObjectScale(void) const
	{
		READ_ZONE();
		return getObjectScale();
	}

	virtual void				setCurrentScale(float scale) 
	{
		WRITE_ZONE();
		mObjectScale = scale;
		if (mInjector)
		{
			mInjector->setObjectScale(mObjectScale);
		}
	}

	virtual float				getCurrentScale(void) const
	{
		READ_ZONE();
		return mObjectScale;
	}

	void							startEmit(bool persistent);
	void							stopEmit();
	bool							isEmitting() const
	{
		READ_ZONE();
		return mDoEmit;
	}

	void							emitAssetParticles(bool enable)
	{
		WRITE_ZONE();
		mEmitAssetParticles = enable;
	}
	bool							getEmitAssetParticles() const
	{
		READ_ZONE();
		return mEmitAssetParticles;
	}

	void							setAttachActor(PxActor* a)
	{
		WRITE_ZONE();
		mAttachActor = a;
	}
	const PxActor* 					getAttachActor() const
	{
		READ_ZONE();
		return mAttachActor;
	}
	PxActor* 						mAttachActor;

	void							setAttachRelativePose(const PxTransform& p)
	{
		WRITE_ZONE();
		mAttachRelativePose = p;
	}

	const PxMat44			getAttachRelativePose() const
	{
		READ_ZONE();
		return PxMat44(mAttachRelativePose);
	}
	float					getObjectRadius() const
	{
		READ_ZONE();
		return mObjectRadius;
	}
	void							setPreferredRenderVolume(nvidia::apex::RenderVolume*);

	static PxVec3			random(const PxVec3& low, const PxVec3& high, QDSRand& rand);
	static float				random(const float& low, const float& high, QDSRand& rand);

	void							setSeed(uint32_t seed)
	{
		mRand.setSeed(seed);
	}

	virtual uint32_t					getSimParticlesCount() const
	{
		READ_ZONE();
		return	mInjector->getSimParticlesCount();
	}

	virtual uint32_t					getActiveParticleCount() const;

protected:
	uint32_t					computeNbEmittedFromRate(float dt, float currate);
	void							emitObjects(const PxTransform& pose, uint32_t toEmitNum, bool useFullVolume);
	void							emitObjects(const physx::Array<PxVec3>& positions);
	void							tick();

	EmitterValidateCallback	*mEmitterValidateCallback;
	IosInjectorIntl* 					mInjector;
	InstancedObjectSimulationIntl		*mIOS;
	EmitterAssetImpl* 		    	mAsset;
	EmitterScene* 					mScene;
	PxBounds3				mOverlapAABB;
	PxBounds3				mLastNonEmptyOverlapAABB;

	float						mObjectScale;
	/* runtime state */
	PxTransform				mPose;
	physx::Array<PxTransform>	mPoses;
	bool						mFirstStartEmitCall;
	bool						mDoEmit;
	bool						mEmitAssetParticles;
	bool                        mPersist;
	float						mRemainder;
	float						mEmitterVolume;
	bool						mIsOldPoseInitialized;
	PxTransform					mOldPose;
	float						mDensity;
	float						mRate;
	PxVec3				mVelocityLow;
	PxVec3				mVelocityHigh;
	float						mLifetimeLow;
	float						mLifetimeHigh;
	float						mObjectRadius;
	float						mEmitDuration;
	float						mDescEmitDuration;
	uint32_t					mOverlapTestCollisionGroups;
	bool						mShouldUseGroupsMask;
	physx::Array<IosNewObject>		mNewObjectArray;
	physx::Array<PxVec3>	mNewPositions;
	physx::Array<PxVec3>	mNewVelocities;
	physx::Array<uint32_t>			mNewUserData;

	EmitterGeomBase*				mExplicitGeom;
	EmitterLodParamDesc		mLodParams;
	PxTransform					mAttachRelativePose;

	nvidia::QDSRand				mRand;

	class ApexEmitterTickTask : public PxTask
	{
	private:
		ApexEmitterTickTask& operator=(const ApexEmitterTickTask&);

	public:
		ApexEmitterTickTask(EmitterActorImpl& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "EmitterActorImpl::TickTask";
		}
		void run()
		{
			mActor.tick();
		}

	protected:
		EmitterActorImpl& mActor;
	};
	ApexEmitterTickTask 				mTickTask;

	friend class EmitterScene;
};

}
} // end namespace nvidia

#endif
