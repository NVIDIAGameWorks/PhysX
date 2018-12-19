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



#ifndef APEX_SCENE_H
#define APEX_SCENE_H

#include "Apex.h"
#include "ApexResource.h"
#include "PsUserAllocated.h"
#include "ApexSDKImpl.h"
#include "SceneIntl.h"
#include "ModuleIntl.h"
#include "ApexContext.h"

#include "PsMutex.h"
#include "PsThread.h"
#include "PairFilter.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "PxScene.h"
#include "PxRenderBuffer.h"
#endif

#include "MirrorSceneImpl.h"
#include "ApexSceneUserNotify.h"

#include "PsSync.h"
#include "PxTask.h"
#include "PxTaskManager.h"

#include "ApexGroupsFiltering.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

class PxDefaultSpuDispatcher;


namespace nvidia
{
namespace apex
{

class ApexCudaTestManager;
class ApexCudaProfileManager;

// Tasks forward declarations
class PhysXSimulateTask;
class PhysXBetweenStepsTask;

#if APEX_DURING_TICK_TIMING_FIX
class DuringTickCompleteTask;
#endif

class CheckResultsTask;
class FetchResultsTask;


class ApexScene : public SceneIntl, public ApexContext, public ApexRWLockable, public UserAllocated
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	/* == Public Scene interface == */
	void					simulate(float elapsedTime, 
									 bool finalStep = true, 
									 PxBaseTask *completionTask = NULL,
									 void* scratchMemBlock = 0, 
									 uint32_t scratchMemBlockSize = 0);

	bool					fetchResults(bool block, uint32_t* errorState);
	void					fetchPhysXStats();
	void					fetchApexStats();
	bool					checkResults(bool block) const;

	void					initDebugColorParams();
	void					updateDebugColorParams(const char* color, uint32_t val);
	NvParameterized::Interface* getDebugRenderParams() const;
	NvParameterized::Interface* getModuleDebugRenderParams(const char* name) const;

	uint32_t				allocViewMatrix(ViewMatrixType::Enum);
	uint32_t				allocProjMatrix(ProjMatrixType::Enum);
	uint32_t				getNumViewMatrices() const;
	uint32_t				getNumProjMatrices() const;

	void					setViewMatrix(const PxMat44& viewTransform, const uint32_t viewID = 0);
	PxMat44					getViewMatrix(const uint32_t viewID = 0) const;
	void					setProjMatrix(const PxMat44& projTransform, const uint32_t projID = 0);
	PxMat44					getProjMatrix(const uint32_t projID = 0) const;

	void					setUseViewProjMatrix(const uint32_t viewID = 0, const uint32_t projID = 0);
#if 0 //lionel: work in progress
	const PxMat44& 	getViewProjMatrix() const;
#endif
	void					setViewParams(const PxVec3& eyePosition, 
										  const PxVec3& eyeDirection, 
										  const PxVec3& worldUpDirection, 
										  const uint32_t viewID = 0);

	void					setProjParams(float nearPlaneDistance, 
										  float farPlaneDistance, 
										  float fieldOfViewDegree, 
										  uint32_t viewportWidth, 
										  uint32_t viewportHeight, 
										  const uint32_t projID = 0);

	PxVec3					getEyePosition(const uint32_t viewID = 0) const;
	PxVec3					getEyeDirection(const uint32_t viewID = 0) const;


#if 0 //lionel: work in progress
	const PxMat44& 	buildViewMatrix(const uint32_t viewID = 0);
	const PxMat44& 	buildProjMatrix(const uint32_t projID = 0);
	//viewportToWorld?, worldToViewport? (and screenspace)
	const SceneCalculator* const calculate()
	{
		return mSceneCalculator;
	}
#endif

	float					getElapsedTime() const
	{
		return mElapsedTime;
	}

	const SceneStats*		getStats(void) const;
	void					createApexStats(void);
	void					destroyApexStats(void);
	void					setApexStatValue(int32_t index, StatValue dataVal);


	bool					isSimulating() const
	{
		return mSimulating;
	}
	bool					physXElapsedTime(float& dt) const
	{
		dt = mPxLastElapsedTime;
		return mPxStepWasValid;
	}
	float					getPhysXSimulateTime() const
	{
		return mPhysXSimulateTime;
	}

	PxVec3					getGravity() const
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
		if (mPhysXScene)
		{
			mPhysXScene->lockRead();
			PxVec3 ret = mPhysXScene->getGravity();
			mPhysXScene->unlockRead();
			return ret;
		}
#endif
		return mGravity;
	}

	void					setGravity(const PxVec3& gravity)
	{
		mGravity = gravity;
#if PX_PHYSICS_VERSION_MAJOR == 3
		if (mPhysXScene)
		{
			mPhysXScene->lockWrite();
			mPhysXScene->setGravity(gravity);
			mPhysXScene->unlockWrite();
		}
#endif
	}

	void					release()
	{
		mApexSDK->releaseScene(this);
	}

	/* == Public Context interface == */
	ApexContext* 			getApexContext()
	{
		return DYNAMIC_CAST(ApexContext*)(this);
	}
	void					removeAllActors();
	RenderableIterator* createRenderableIterator()
	{
		return ApexContext::createRenderableIterator();
	}
	void releaseRenderableIterator(RenderableIterator& iter)
	{
		ApexContext::releaseRenderableIterator(iter);
	}
	uint32_t				addActor(ApexActor& actor, ApexActor* actorPtr = NULL);

	/* == Renderable interface == */
	void					lockRenderResources();
	void					unlockRenderResources();
	void					updateRenderResources(bool rewriteBuffers = false, void* userRenderData = 0);
	void					dispatchRenderResources(UserRenderer& renderer);
	PxBounds3				getBounds() const;

	void					visualize();

	/* == SceneIntl interface == */
	void					moduleReleased(ModuleSceneIntl&);
	virtual PxTaskManager*	getTaskManager() const
	{
		READ_ZONE();
		return mTaskManager;
	};

	uint32_t				getTotalElapsedMS() const
	{
		return mTotalElapsedMS;
	}

	bool					isFinalStep() const
	{
		return mFinalStep;
	}

#if PX_PHYSICS_VERSION_MAJOR == 3
	virtual void lockRead(const char *fileName,uint32_t lineno)
	{
		if (mPhysXScene != NULL)
		{
			mPhysXScene->lockRead(fileName,lineno);
		}
	}

	virtual void lockWrite(const char *fileName,uint32_t lineno)
	{
		if (mPhysXScene != NULL)
		{
			mPhysXScene->lockWrite(fileName,lineno);
		}
	}


	virtual void unlockRead ()
	{
		if (mPhysXScene != NULL)
		{
			mPhysXScene->unlockRead();
		}
	}

	virtual void unlockWrite ()
	{
		if (mPhysXScene != NULL)
		{
			mPhysXScene->unlockWrite();
		}
	}

	virtual void	addActorPair(PxActor *actor0,PxActor *actor1);
	virtual void	removeActorPair(PxActor *actor0,PxActor *actor1);
	virtual bool	findActorPair(PxActor *actor0,PxActor *actor1) const;
#endif

	virtual void	addBoundingBox(const PxBounds3& bounds, UserBoundingBoxFlags::Enum flags) 
	{
		WRITE_ZONE();
		mBBs.pushBack(UserDefinedBoundingBox(bounds, flags));
	}

	virtual const PxBounds3 getBoundingBox(const uint32_t index) const
	{
		READ_ZONE();
		PX_ASSERT(index < mBBs.size());
		if(index < mBBs.size()) 
		{
			return mBBs[index].bb;
		}
		return PxBounds3(PxVec3(0.0f), PxVec3(0.0f));
	}

	virtual UserBoundingBoxFlags::Enum getBoundingBoxFlags(const uint32_t index) const
	{
		READ_ZONE();
		PX_ASSERT(index < mBBs.size());
		if(index < mBBs.size()) 
		{
			return mBBs[index].flags;
		}
		return UserBoundingBoxFlags::NONE;
	}

	virtual uint32_t getBoundingBoxCount() const
	{
		READ_ZONE();
		return mBBs.size();
	}

	virtual void  removeBoundingBox(const uint32_t index)
	{
		WRITE_ZONE();
		PX_ASSERT(index < mBBs.size());
		if(index < mBBs.size()) 
		{
			mBBs.remove(index);
		}
	}

	virtual void  removeAllBoundingBoxes()
	{
		WRITE_ZONE();
		mBBs.clear();
	}

	void					allocateTasks();
	void					freeTasks();
	void					setUseDebugRenderable(bool state);

	ApexScene(const SceneDesc& desc, ApexSDKImpl* sdk);
	~ApexScene();

	void					moduleCreated(ModuleIntl&);
	void					destroy();

	const PxRenderBuffer*	getRenderBuffer() const;
	const PxRenderBuffer*	getRenderBufferScreenSpace() const;

#if PX_PHYSICS_VERSION_MAJOR == 3

	virtual MirrorScene *createMirrorScene(nvidia::apex::Scene &mirrorScene,
		MirrorScene::MirrorFilter &mirrorFilter,
		float mirrorStaticDistance,
		float mirrorDynamicDistance,
		float mirrorDistanceThreshold);


	void						setPhysXScene(PxScene* s);
	PxScene*					getPhysXScene() const
	{
		READ_ZONE();
		return mPhysXScene;
	}

	PxScene*					mPhysXScene;
	
	mutable PhysXRenderBuffer	mRenderBuffer;
	mutable PhysXRenderBuffer	mRenderBufferScreenSpace;

	void					addModuleUserNotifier(physx::PxSimulationEventCallback& notify)
	{
		mUserNotify.addModuleNotifier(notify);
	}
	void					removeModuleUserNotifier(physx::PxSimulationEventCallback& notify)
	{
		mUserNotify.removeModuleNotifier(notify);
	}
	void					addModuleUserContactModify(physx::PxContactModifyCallback& contactModify)
	{
		mUserContactModify.addModuleContactModify(contactModify);
	}
	void					removeModuleUserContactModify(physx::PxContactModifyCallback& contactModify)
	{
		mUserContactModify.removeModuleContactModify(contactModify);
	}

	ApexSceneUserNotify			mUserNotify;
	ApexSceneUserContactModify	mUserContactModify;

	PhysX3Interface* getApexPhysX3Interface() const
	{
		return mPhysX3Interface;
	}

	PhysX3Interface*		mPhysX3Interface;
#endif

	ModuleSceneIntl* getInternalModuleScene(const char* moduleName);
	
	PX_INLINE void* getCudaTestManager() const
	{
		return mCudaTestManager;
	}
	PX_INLINE ApexCudaTestManager& getApexCudaTestManager()
	{
		return *mCudaTestManager;
	}
	PX_INLINE void* getCudaProfileManager() const
	{
		return mCudaProfileManager;
	}
	bool					isUsingCuda() const 
	{
		return mUseCuda;
	}

	virtual void setCudaKernelCheckEnabled(bool enabled)
	{
		mCudaKernelCheckEnabled = enabled;
	}
	virtual bool getCudaKernelCheckEnabled() const
	{
		return mCudaKernelCheckEnabled;
	}

	float						mElapsedTime;
	ApexSDKImpl*				mApexSDK;
	Array<ModuleSceneIntl*>		mModuleScenes;
	RenderDebugInterface* 		mSceneRenderDebug;

	uint32_t					mOrigSceneMaxIter;
	float						mOrigSceneSubstepSize;

	PxTaskManager*				mTaskManager;

	Mutex						mPhysXLock;

	bool						mSimulating;
	bool						mUseDebugRenderable;
	float						mUsedResource;
	float						mSumBenefit;
	mutable Sync				mFetchResultsReady;
	Sync						mSimulationComplete;

	PhysXSimulateTask*			mPhysXSimulate;
	PhysXBetweenStepsTask*		mBetweenstepTasks;
#if APEX_DURING_TICK_TIMING_FIX
	DuringTickCompleteTask*		mDuringTickComplete;
#endif
	CheckResultsTask*			mCheckResults;
	FetchResultsTask*			mFetchResults;

	uint32_t					mTotalElapsedMS;
	float						mTimeRemainder;
	float						mPhysXRemainder;
	float						mPhysXSimulateTime;

	float						mPxLastElapsedTime;
	float						mPxAccumElapsedTime;
	bool						mPxStepWasValid;
	bool						mFinalStep;

	bool						mUseCuda;


	static double				mQPC2MilliSeconds;
	static PX_INLINE float	ticksToMilliseconds(uint64_t t0, uint64_t t1)
	{
		return (float)((double)(t1 - t0) * mQPC2MilliSeconds);
	}

	uint64_t					mApexSimulateTickCount;
	uint64_t					mPhysXSimulateTickCount;

	static const uint32_t IgnoredSeed = UINT32_MAX;

	uint32_t getSeed();
	
private:
	void updateGravity();

	SceneStats					mApexSceneStats;

	mutable ApexCudaTestManager*	mCudaTestManager;
	mutable ApexCudaProfileManager*	mCudaProfileManager;
	bool						mCudaKernelCheckEnabled;

	/* transforms info */
#if 0 //lionel: work in progress
	void						getColMajColVecArray(const PxMat44& colVecMat44, float* const result);
	void						getColVecMat44(const float* const colMajColVecArray, PxMat44& result);
	void						multiplyColMajColVecArray(const float* const fromSpace, const float* const toSpace, float* const result);
	float						mViewColMajColVecArray[16];
	float						mProjColMajColVecArray[16];
	float						mViewProjColMajColVecArray[16];
#endif
	struct ViewMatrixProperties : public UserAllocated
	{
		ViewMatrixProperties() {}
		~ViewMatrixProperties() {}
		ViewMatrixProperties(PxMat44 v, bool l) :
			viewMatrix(v), isLookAt(l), pvdCreated(false) 	{}

		PxMat44					viewMatrix;
		bool					isUserCustomized;
		bool					isLookAt;
		bool					pvdCreated;

		ApexSimpleString		cameraName;
	};

	struct ViewMatrixLookAt : public ViewMatrixProperties
	{
		ViewMatrixLookAt() {}
		~ViewMatrixLookAt() {}
		ViewMatrixLookAt(PxMat44 v, bool l, bool r) :
			ViewMatrixProperties(v, l), isRightHand(r) {}

		bool					isRightHand;
	};

	struct ProjMatrixProperties : public UserAllocated
	{
		ProjMatrixProperties() {}
		~ProjMatrixProperties() {}
		ProjMatrixProperties(PxMat44 p, bool u, bool f) :
			projMatrix(p), isUserCustomized(u), isPerspectiveFOV(f)  {}

		PxMat44					projMatrix;
		bool					isUserCustomized;
		bool					isPerspectiveFOV;
	};

	struct ProjMatrixUserCustomized : public ProjMatrixProperties
	{
		ProjMatrixUserCustomized() {}
		~ProjMatrixUserCustomized() {}
		ProjMatrixUserCustomized(PxMat44 p, bool u, bool f, float near, float far, float fov, uint32_t w, uint32_t h) :
			ProjMatrixProperties(p, u, f), nearPlaneDistance(near), farPlaneDistance(far), fieldOfViewDegree(fov), viewportWidth(w), viewportHeight(h) {}

		float					nearPlaneDistance;
		float					farPlaneDistance;
		float					fieldOfViewDegree;
		uint32_t				viewportWidth;			//only one instance?
		uint32_t				viewportHeight;			//only one instance?
	};

	struct ProjMatrixPerspectiveFOV : public ProjMatrixProperties
	{
		ProjMatrixPerspectiveFOV() {}
		~ProjMatrixPerspectiveFOV() {}
		ProjMatrixPerspectiveFOV(PxMat44 p, bool u, bool f, bool i) :
			ProjMatrixProperties(p, u, f), isZinvert(i) {}

		bool					isZinvert;
	};

	Array<ViewMatrixProperties*>	mViewMatrices;
	Array<ProjMatrixProperties*>	mProjMatrices;
	PxMat44							mViewProjMatrix;
#if 0 //lionel: work in progress
	uint32_t					mCurrentViewID;
	uint32_t					mCurrentProjID;
	SceneCalculator*			mSceneCalculator;
	friend class SceneCalculator;
	//class SceneCalculator
	//{
	//public:
	//	SceneCalculator():s(NULL) {}
	//	~SceneCalculator() {s=NULL;}
	//	float distanceFromEye(PxVec3 to) {return (-s->getEyePosition(0) + to).magnitude();}	//lionel: use currentIDs when multiple matrices allowed
	//	friend class ApexScene;
	//private:
	//	void construct(const physx::ApexScene * scene) {s = scene;}
	//	void destruct() {s = NULL;}
	//	const physx::ApexScene * s;
	//};
#endif

	DebugRenderParams*								mDebugRenderParams;
	DebugColorParams*								mDebugColorParams;
	HashMap<const char*, RENDER_DEBUG::DebugColors::Enum>	mColorMap;

	PxVec3							mGravity;

	PairFilter						mPairFilter;

	struct UserDefinedBoundingBox
	{
		PxBounds3 bb;
		UserBoundingBoxFlags::Enum	flags;

		UserDefinedBoundingBox(const PxBounds3& _bb, UserBoundingBoxFlags::Enum _flags) :
																		bb(_bb), flags(_flags) {}
	};
	Array<UserDefinedBoundingBox>	mBBs;
};


}
} // end namespace nvidia::apex

#endif // APEX_SCENE_H
