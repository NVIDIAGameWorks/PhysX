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



#ifndef __GROUND_EMITTER_ACTOR_IMPL_H__
#define __GROUND_EMITTER_ACTOR_IMPL_H__

#include "GroundEmitterActor.h"
#include "GroundEmitterAssetImpl.h"
#include "EmitterScene.h"
#include "ApexActor.h"
#include "ApexFIFO.h"
#include "ApexSharedUtils.h"
#include "PsUserAllocated.h"
#include "PxPlane.h"

namespace nvidia
{
namespace emitter
{

template<typename T>
struct ApexVec2
{
	T x;
	T y;
};

class InjectorData : public UserAllocated
{
public:
	virtual ~InjectorData()
	{
	}

	IosInjectorIntl* 				mInjector;  // For emitting
	float						mObjectRadius;

	ApexSimpleString			iofxAssetName;
	ApexSimpleString			iosAssetName;

	physx::Array<IosNewObject>	particles;

	class InjectTask : public PxTask
	{
	public:
		InjectTask() {}
		void run();
		const char* getName() const
		{
			return "GroundEmitterActor::InjectTask";
		}
		GroundEmitterActorImpl* mActor;
		InjectorData* mData;
	};
	void initTask(GroundEmitterActorImpl& actor, InjectorData& data)
	{
		mTask.mActor = &actor;
		mTask.mData = &data;
	}

	InjectTask					mTask;
};


struct MaterialData
{
	physx::Array<uint32_t>  injectorIndices;
	physx::Array<float>  accumWeights;
	physx::Array<float>  maxSlopeAngles;

	uint32_t chooseIOFX(float& maxSlopeAngle, QDSRand& rand)
	{
		PX_ASSERT(injectorIndices.size() == accumWeights.size());

		float u = rand.getScaled(0, accumWeights.back());
		for (uint32_t i = 0; i < accumWeights.size(); i++)
		{
			if (u <= accumWeights[i])
			{
				maxSlopeAngle = maxSlopeAngles[i];
				return injectorIndices[i];
			}
		}

		PX_ALWAYS_ASSERT();
		return (uint32_t) - 1;
	}
};


class GroundEmitterActorImpl : public GroundEmitterActor, public EmitterActorBase, public ApexResourceInterface, public ApexResource, public ApexRWLockable
{
private:
	GroundEmitterActorImpl& operator=(const GroundEmitterActorImpl&);

public:
	APEX_RW_LOCKABLE_BOILERPLATE

	GroundEmitterActorImpl(const GroundEmitterActorDesc&, GroundEmitterAssetImpl&, ResourceList&, EmitterScene&);
	~GroundEmitterActorImpl();

	Renderable* 			getRenderable()
	{
		return NULL;
	}
	Actor* 				getActor()
	{
		return this;
	}
	void						removeActorAtIndex(uint32_t index);

	void						getLodRange(float& min, float& max, bool& intOnly) const;
	float				getActiveLod() const;
	void						forceLod(float lod);
	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		ApexActor::setEnableDebugVisualization(state);
	}

	/* ApexResourceInterface, ApexResource */
	void						release();
	uint32_t				getListIndex() const
	{
		return m_listIndex;
	}
	void						setListIndex(class ResourceList& list, uint32_t index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* EmitterActorBase */
	void                        destroy();
	Asset*		        getOwner() const;
	void						visualize(RenderDebugInterface& renderDebug);

	void						setPhysXScene(PxScene*);
	PxScene*					getPhysXScene() const
	{
		return mPxScene;
	}
	PxScene* 					mPxScene;

	void						submitTasks();
	void						setTaskDependencies();
	void						fetchResults();

	GroundEmitterAsset*       getEmitterAsset() const;

	const PxMat44		getPose() const;
	void						setPose(const PxMat44& pos);

	const PxVec3& 		getPosition() const
	{
		return mLocalPlayerPosition;
	}
	void				        setPosition(const PxVec3& pos);
	const PxTransform& getRotation() const
	{
		return mPose;
	}
	void				setRotation(const PxTransform& rotation);
	const PxVec3 &getVelocityLow() const
	{
		return mAsset->getVelocityLow();
	}
	const PxVec3 &getVelocityHigh() const
	{
		return mAsset->getVelocityHigh();
	}
	const float &getLifetimeLow() const
	{
		return mAsset->getLifetimeLow();
	}
	const float &getLifetimeHigh() const
	{
		return mAsset->getLifetimeHigh();
	}
	uint32_t				getRaycastCollisionGroups() const
	{
		READ_ZONE();
		return mRaycastCollisionGroups;
	}
	void						setRaycastCollisionGroups(uint32_t g)
	{
		WRITE_ZONE();
		mRaycastCollisionGroups = g;
	}

	const physx::PxFilterData*			getRaycastCollisionGroupsMask() const
	{
		READ_ZONE();
		return mShouldUseGroupsMask ? &mRaycastCollisionGroupsMask : NULL;
	}
	void						setRaycastCollisionGroupsMask(physx::PxFilterData*);

	void						setMaterialLookupCallback(MaterialLookupCallback* mlc)
	{
		WRITE_ZONE();
		mMaterialCallback = mlc;
	}
	MaterialLookupCallback* 	getMaterialLookupCallback() const
	{
		READ_ZONE();
		return mMaterialCallback;
	}

	void						setAttachActor(PxActor* a)
	{
		WRITE_ZONE();
		mAttachActor = a;
	}
	const PxActor* 				getAttachActor() const
	{
		READ_ZONE();
		return mAttachActor;
	}
	PxActor* 					mAttachActor;

	void						setAttachRelativePosition(const PxVec3& pos)
	{
		WRITE_ZONE();
		mAttachRelativePosition = pos;
	}

	const PxVec3& 		getAttachRelativePosition() const
	{
		READ_ZONE();
		return mAttachRelativePosition;
	}

	bool						addMeshForGroundMaterial(const MaterialFactoryMappingDesc&, const EmitterLodParamDesc&);
	void						setDensity(const float& d)
	{
		READ_ZONE();
		mDensity = d;
	}
	void                        setMaxRaycastsPerFrame(uint32_t m)
	{
		WRITE_ZONE();
		mMaxNumRaycastsPerFrame = m;
	}
	void                        setRaycastHeight(float h)
	{
		WRITE_ZONE();
		mRaycastHeight = h;
	}
	void						setSpawnHeight(float h)
	{
		WRITE_ZONE();
		mSpawnHeight = h;
	}
	void                        setRadius(float);
	void						setPreferredRenderVolume(nvidia::apex::RenderVolume*);


	const float &getDensity() const
	{
		READ_ZONE();
		return mDensity;
	}
	float				getRadius() const
	{
		READ_ZONE();
		return mRadius;
	}
	uint32_t				getMaxRaycastsPerFrame() const
	{
		READ_ZONE();
		return mMaxNumRaycastsPerFrame;
	}
	float				getRaycastHeight() const
	{
		READ_ZONE();
		return mRaycastHeight;
	}
	float				getSpawnHeight() const
	{
		READ_ZONE();
		return mSpawnHeight;
	}

	void							setSeed(uint32_t seed)
	{
		mRand.setSeed(seed);
	}

protected:
	/* internal methods */
	void                        submitRaycasts();
	void                        injectParticles(InjectorData& data);
	void                        refreshCircle(bool edgeOnly);
	void						tick();

	bool						onRaycastQuery(uint32_t nbHits, const physx::PxRaycastHit* hits);

	GroundEmitterAssetImpl* 		mAsset;
	EmitterScene* 				mScene;

	/* Actor modifiable versions of asset data */
	float 				mDensity;
	float				mRadius;
	uint32_t				mMaxNumRaycastsPerFrame;
	float				mRaycastHeight;
	float				mSpawnHeight;
	PxVec3				mLocalUpDirection;
	PxTransform		mPose;
	PxTransform		mInversePose;
	uint32_t				mRaycastCollisionGroups;
	physx::PxFilterData			mRaycastCollisionGroupsMask;

	PxVec3				mAttachRelativePosition;

	MaterialLookupCallback* 	mMaterialCallback;
	physx::Array<MaterialLookupCallback::MaterialRequest> mMaterialRequestArray;

	/* Actor local data */
	Bank<MaterialData, uint16_t> mPerMaterialData;
	physx::Array<InjectorData*>	mInjectorList;
	physx::Array<InstancedObjectSimulationIntl*> mIosList;
	float				mGridCellSize;
	uint32_t				mTotalElapsedTimeMs;

	struct RaycastVisInfo
	{
		PxVec3 rayStart;
		uint32_t  timeSubmittedMs;	// running time in milliseconds (should last for 50 days or so)
	};
	physx::Array<RaycastVisInfo>	mVisualizeRaycastsList;


	// use FIFO so it does the earliest (that are probably the closest to the player first)
	ApexFIFO<PxVec3>		mToRaycast;
	physx::Array<uint32_t>  mCellLastRefreshSteps;
	uint32_t                mSimulationSteps;
	ApexVec2<int32_t>		mNextGridCell;

	/* runtime state members */

	PxVec3				mLocalPlayerPosition;
	PxVec3				mWorldPlayerPosition;
	PxVec3				mOldLocalPlayerPosition; // used to determine when to clear per cell step counts
	float                mStepsize;
	PxPlane				mHorizonPlane;

	float				mCurrentDensity;
	float                mRadius2;
	float                mCircleArea;
	bool                        mShouldUseGroupsMask;
	float                mMaxStepSize;
	bool                        mRefreshFullCircle;

	nvidia::Mutex				mInjectorDataLock;

	nvidia::QDSRand				mRand;

	class GroundEmitterTickTask : public PxTask
	{
	public:
		GroundEmitterTickTask(GroundEmitterActorImpl& actor) : mActor(actor) {}

		const char* getName() const
		{
			return "GroundEmitterActor::TickTask";
		}
		void run()
		{
			mActor.tick();
		}

	protected:
		GroundEmitterActorImpl& mActor;

	private:
		GroundEmitterTickTask& operator=(const GroundEmitterTickTask&);
	};
	GroundEmitterTickTask 		mTickTask;

	friend class InjectorData::InjectTask;
	friend class QueryReport;
};

}
} // end namespace nvidia

#endif
