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



#include "Apex.h"
#include "ApexDefs.h"
#include "ApexScene.h"
#include "ApexSceneTasks.h"
#include "ApexSDKImpl.h"
#include "ApexActor.h"
#include "FrameworkPerfScope.h"
#include "ApexRenderDebug.h"
#include "ModuleIntl.h"
#include "ApexPvdClient.h"
#include "PsTime.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "ScopedPhysXLock.h"
#include "PxRigidActor.h"
#include "cloth/PxCloth.h" // for PxCloth::isCloth()
#endif

#include "ApexUsingNamespace.h"
#include "PsSync.h"
#include "PxTask.h"
#include "PxTaskManager.h"
#include "PxGpuDispatcher.h"
#include "PxCudaContextManager.h"
#include "ApexString.h"

#define USE_FILE_RENDER_DEBUG 0
#define USE_PVD_RENDER_DEBUG 0

#if USE_FILE_RENDER_DEBUG
#include "PxFileRenderDebug.h"
#endif
#if USE_PVD_RENDER_DEBUG
#include "PxPVDRenderDebug.h"
#endif

#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
#include <cuda.h>
class IDirect3DDevice9;
class IDirect3DResource9;
class IDirect3DVertexBuffer9;
#include <cudad3d9.h>
class ID3D10Device;
class ID3D10Resource;
class IDXGIAdapter;
#include <cudad3d10.h>

#include "ApexCudaTest.h"
#include "ApexCudaProfile.h"
#endif

#include "Lock.h"

#if PX_X86
#define PTR_TO_UINT64(x) ((uint64_t)(uint32_t)(x))
#else
#define PTR_TO_UINT64(x) ((uint64_t)(x))
#endif

namespace nvidia
{
namespace apex
{

double ApexScene::mQPC2MilliSeconds = 0.0;

/************
* ApexScene *
************/

ApexScene::ApexScene(const SceneDesc& sceneDesc, ApexSDKImpl* sdk)
	: mApexSDK(sdk)
#if PX_PHYSICS_VERSION_MAJOR == 3
	, mPhysXScene(NULL)
	, mPhysX3Interface(sceneDesc.physX3Interface)
#endif
	, mElapsedTime(0.0f)
	, mSceneRenderDebug(NULL)
	, mOrigSceneMaxIter(1)
	, mOrigSceneSubstepSize(1.0f / 60.0f)
	, mTaskManager(NULL)
	, mSimulating(false)
	, mUseDebugRenderable(sceneDesc.useDebugRenderable)
	, mUsedResource(0.0f)
	, mSumBenefit(0.0f)
	, mPhysXSimulate(NULL)
	, mBetweenstepTasks(NULL)
#if APEX_DURING_TICK_TIMING_FIX
	, mDuringTickComplete(NULL)
#endif
	, mCheckResults(NULL)
	, mFetchResults(NULL)
	, mTotalElapsedMS(0)
	, mTimeRemainder(0.0f)
	, mPhysXRemainder(0.0f)
	, mPhysXSimulateTime(0.0f)
	, mPxLastElapsedTime(0.0f)
	, mPxAccumElapsedTime(0.0f)
	, mPxStepWasValid(false)
	, mFinalStep(false)
#if APEX_CUDA_SUPPORT
	, mUseCuda(sceneDesc.useCuda)
#else
	, mUseCuda(false)
#endif	
	, mCudaKernelCheckEnabled(false)
	, mGravity(0)
{
	mSimulationComplete.set();

#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
	mCudaTestManager = PX_NEW(ApexCudaTestManager)();
	mCudaProfileManager = PX_NEW(ApexCudaProfileManager)();
	mCudaTestManager->setInternalApexScene(this);
	mCudaProfileManager->setInternalApexScene(this);
#else
	mCudaTestManager = 0;
	mCudaProfileManager = 0;
#endif

	// APEX was ignoring the numerator from CounterFrequencyToTensOfNanos, this is OK as long as
	// the numerator is equal to Time::sNumTensOfNanoSecondsInASecond (100,000,000)
	//float ret = (float)((double)(t1 - t0) / (double)Time::getBootCounterFrequency().mDenominator);

	// Let's see if we can use both the numerator and denominator correctly (mostly for linux)
	const CounterFrequencyToTensOfNanos freq = Time::getBootCounterFrequency();
	const double freqMultiplier = (double)freq.mNumerator/(double)freq.mDenominator;

	mQPC2MilliSeconds = freqMultiplier * 0.00001; // from tens of nanos to milliseconds ( x / 100 / 1000)

#if PX_WINDOWS_FAMILY
	if (sceneDesc.debugVisualizeRemotely)
	{
#if USE_FILE_RENDER_DEBUG
		prd = createFileRenderDebug("SceneRenderDebug.bin", false, sceneDesc.debugVisualizeLocally);
#endif
#if USE_PVD_RENDER_DEBUG
		prd = createPVDRenderDebug(sceneDesc.debugVisualizeLocally);
#endif
	}
#endif
	mSceneRenderDebug = sceneDesc.debugInterface;

	/* Create NvParameterized for mDebugRenderParams */
	NvParameterized::Traits* traits = mApexSDK->getParameterizedTraits();
	PX_ASSERT(traits);
	mDebugRenderParams = (DebugRenderParams*)traits->createNvParameterized(DebugRenderParams::staticClassName());
	PX_ASSERT(mDebugRenderParams);

	/* Get mDebugColorParams from ApexSDKImpl */
	mDebugColorParams = (DebugColorParams*)mApexSDK->getDebugColorParams();
	initDebugColorParams();

#if PX_PHYSICS_VERSION_MAJOR == 0
	mTaskManager = PxTaskManager::createTaskManager(*mApexSDK->getErrorCallback(), sceneDesc.cpuDispatcher, sceneDesc.gpuDispatcher);
	mTaskManager->setGpuDispatcher(*sceneDesc.gpuDispatcher);
	if (sceneDesc.cpuDispatcher == NULL)
	{
		mTaskManager->setCpuDispatcher(*mApexSDK->getDefaultThreadPool());
	}
#elif PX_PHYSICS_VERSION_MAJOR == 3
	setPhysXScene(sceneDesc.scene);
#if APEX_CUDA_SUPPORT
	if (sceneDesc.scene != NULL)
	{
		mTaskManager = sceneDesc.scene->getTaskManager();
		if (mTaskManager->getGpuDispatcher())
		{
			PxCudaContextManager* ctx = mTaskManager->getGpuDispatcher()->getCudaContextManager();
			if (ctx && ctx->supportsArchSM30())
			{
				ctx->setUsingConcurrentStreams(false);
			}
		}
	}
#endif
#endif

	allocateTasks();

	createApexStats();
}

ApexScene::~ApexScene()
{
	destroyApexStats();

#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
	PX_DELETE(mCudaTestManager);
	PX_DELETE(mCudaProfileManager);
#endif

	while (mViewMatrices.size())
	{
		PX_FREE(mViewMatrices.popBack());
	}
	while (mProjMatrices.size())
	{
		PX_FREE(mProjMatrices.popBack());
	}
}

// This array is still created because it is handy to have it to initialize
// the individual scene arrays.  The string data is reduced to pointers.  This
// could be improved with macros that do both this and the enums at the same time.
static StatsInfo ApexStatsData[] =
{
	{"NumberOfActors",          StatDataType::INT,   {{0}} },
	{"NumberOfShapes",          StatDataType::INT,   {{0}} },
	{"NumberOfAwakeShapes",     StatDataType::INT,   {{0}} },
	{"NumberOfCpuShapePairs",   StatDataType::INT,   {{0}} },
	{"ApexBeforeTickTime",	    StatDataType::FLOAT, {{0}} },
	{"ApexDuringTickTime",	    StatDataType::FLOAT, {{0}} },
	{"ApexPostTickTime",	    StatDataType::FLOAT, {{0}} },
	{"PhysXSimulationTime",	    StatDataType::FLOAT, {{0}} },
	{"ClothingSimulationTime",  StatDataType::FLOAT, {{0}} },
	{"ParticleSimulationTime",			StatDataType::FLOAT, {{0}} },
	{"TurbulenceSimulationTime",			StatDataType::FLOAT, {{0}} },
	{"PhysXFetchResultTime",    StatDataType::FLOAT, {{0}} },
	{"UserDelayedFetchTime",    StatDataType::FLOAT, {{0}} },
	{"RbThroughput(Mpair/sec)", StatDataType::FLOAT, {{0}} },
	{"IOFX: SimulatedSpriteParticlesCount",	StatDataType::INT, {{0}} },
	{"IOFX: SimulatedMeshParticlesCount",	StatDataType::INT, {{0}} },
	{"VisibleDestructibleChunkCount",		StatDataType::INT, {{0}} },
	{"DynamicDestructibleChunkIslandCount",	StatDataType::INT, {{0}} }
};

PX_COMPILE_TIME_ASSERT(sizeof(ApexStatsData) / sizeof(ApexStatsData[0]) == ApexScene::NumberOfApexStats);

void ApexScene::createApexStats(void)
{
	mApexSceneStats.numApexStats		= NumberOfApexStats;
	mApexSceneStats.ApexStatsInfoPtr	= (StatsInfo*)PX_ALLOC(sizeof(StatsInfo) * ApexScene::NumberOfApexStats, PX_DEBUG_EXP("StatsInfo"));

	for (uint32_t i = 0; i < ApexScene::NumberOfApexStats; i++)
	{
		mApexSceneStats.ApexStatsInfoPtr[i] = ApexStatsData[i];
	}
}

void ApexScene::destroyApexStats(void)
{
	mApexSceneStats.numApexStats = 0;
	if (mApexSceneStats.ApexStatsInfoPtr)
	{
		PX_FREE_AND_RESET(mApexSceneStats.ApexStatsInfoPtr);
	}
}

const SceneStats* ApexScene::getStats(void) const
{
	READ_ZONE();
	return(&mApexSceneStats);
}

void ApexScene::setApexStatValue(int32_t index, StatValue dataVal)
{
	if (mApexSceneStats.ApexStatsInfoPtr)
	{
		mApexSceneStats.ApexStatsInfoPtr[index].StatCurrentValue = dataVal;
	}
}

NvParameterized::Interface* ApexScene::getDebugRenderParams() const
{
	READ_ZONE();
	return mDebugRenderParams;
}

//Module names are case sensitive:
//BasicIos, Clothing, Destructible, Emitter, Iofx
NvParameterized::Interface* ApexScene::getModuleDebugRenderParams(const char* name) const
{
	READ_ZONE();
	NvParameterized::Handle handle(*mDebugRenderParams), memberHandle(*mDebugRenderParams);
	NvParameterized::Interface* refPtr = NULL;
	int size;

	if (mDebugRenderParams->getParameterHandle("moduleName", handle) == NvParameterized::ERROR_NONE)
	{
		handle.getArraySize(size, 0);
		for (int i = 0; i < size; i++)
		{
			if (handle.getChildHandle(i, memberHandle) == NvParameterized::ERROR_NONE)
			{
				memberHandle.getParamRef(refPtr);
				if (strstr(refPtr->className(), name) != 0)
				{
					return refPtr;
				}
			}
		}
	}

	return NULL;
}

uint32_t ApexScene::allocViewMatrix(ViewMatrixType::Enum viewType)
{
	WRITE_ZONE();
	if (mViewMatrices.size() >= 1)
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_OPERATION("instantiating more than %d view matrices is not allowed!", mViewMatrices.size());
		}
	}
	else
	{
		ViewMatrixProperties* v;

		switch (viewType)
		{
		case ViewMatrixType::USER_CUSTOMIZED:
		{
			v = PX_NEW(ViewMatrixLookAt)(PxMat44(PxIdentity),false,true);
		}
		break;
		case ViewMatrixType::LOOK_AT_RH:
		{
			v = PX_NEW(ViewMatrixLookAt)(PxMat44(PxIdentity), false, true);
		}
		break;
		case ViewMatrixType::LOOK_AT_LH:
		{
			v = PX_NEW(ViewMatrixLookAt)(PxMat44(PxIdentity), false, false);
		}
		break;
		default:
			if (!mTotalElapsedMS)
			{
				APEX_INVALID_PARAMETER("Invalid ViewMatrixType!");
			}
			v = NULL;
			break;
		}
		if (v)
		{
			mViewMatrices.pushBack(v);
		}
	}
	return mViewMatrices.size() - 1;
}

uint32_t ApexScene::allocProjMatrix(ProjMatrixType::Enum projType)
{
	WRITE_ZONE();
	if (mProjMatrices.size() >= 1)
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_OPERATION("instantiating more than %d projection matrices is not allowed!", mProjMatrices.size());
		}
	}
	else
	{
		ProjMatrixProperties* p;

		switch (projType)
		{
		case ProjMatrixType::USER_CUSTOMIZED:
		{
			p = PX_NEW(ProjMatrixUserCustomized)(PxMat44(PxIdentity), true, false, 0.1f, 10000.0f, 45.0f, 1024, 640);
		}
		break;
#if 0 //lionel: work in progress
		case ProjMatrixType::PERSPECTIVE_FOV_RH:
		{
			p = PX_NEW(ProjMatrixPerspectiveFOV)(PxMat44(PxIdentity), false, true, true);
		}
		break;
		case ProjMatrixType::PERSPECTIVE_FOV_LH:
		{
			p = PX_NEW(ProjMatrixPerspectiveFOV)(PxMat44(PxIdentity), false, true, false);
		}
		break;
#endif
		default:
			if (!mTotalElapsedMS)
			{
				APEX_INVALID_PARAMETER("Invalid ProjMatrixType!");
			}
			p = NULL;
			break;
		}
		if (p)
		{
			mProjMatrices.pushBack(p);
		}
	}
	return mProjMatrices.size() - 1;
}

uint32_t ApexScene::getNumViewMatrices() const
{
	READ_ZONE();
	return mViewMatrices.size();
}

uint32_t ApexScene::getNumProjMatrices() const
{
	READ_ZONE();
	return mProjMatrices.size();
}

void ApexScene::setViewMatrix(const PxMat44& viewTransform, const uint32_t viewID)
{
	WRITE_ZONE();
	if (viewID >= getNumViewMatrices())
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
		}
	}
	else
	{
		mViewMatrices[viewID]->viewMatrix = viewTransform;

		// create PVD cameras
		pvdsdk::ApexPvdClient* client = mApexSDK->getApexPvdClient();
		if ((client != NULL) && client->isConnected())
		{
			if (!mViewMatrices[viewID]->pvdCreated)
			{
				ApexSimpleString cameraNum;
				ApexSimpleString::itoa(viewID, cameraNum);
				mViewMatrices[viewID]->cameraName = "ApexCamera ";
				mViewMatrices[viewID]->cameraName += cameraNum;
				mViewMatrices[viewID]->pvdCreated = true;
			}

			PxVec3 gravity = getGravity();
			gravity.normalize();
			PxVec3 position = getEyePosition(viewID);
			PxVec3 target = position + getEyeDirection(viewID);

			//pvdBinding->getConnectionManager().setCamera(mViewMatrices[viewID]->cameraName.c_str(), position, -gravity, target);
		}
	}
}

PxMat44 ApexScene::getViewMatrix(const uint32_t viewID) const
{
	READ_ZONE();
	if (viewID < getNumViewMatrices())
	{
		return mViewMatrices[viewID]->viewMatrix;
	}
	else
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
		}
	}
	return PxMat44(PxIdentity);
}

void ApexScene::setProjMatrix(const PxMat44& projTransform, const uint32_t projID)
{
	WRITE_ZONE();
	if (projID >= getNumProjMatrices())
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("projection matrix for projID %d is not initialized! see allocProjMatrix()", projID);
		}
	}
	else
	{
		mProjMatrices[projID]->projMatrix = projTransform;
	}
}

PxMat44 ApexScene::getProjMatrix(const uint32_t projID) const
{
	READ_ZONE();
	if (projID < getNumProjMatrices())
	{
		return mProjMatrices[projID]->projMatrix;
	}
	else
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("projection matrix for projID %d is not initialized! see allocProjMatrix()", projID);
		}
	}
	return PxMat44(PxIdentity);
}

void ApexScene::setUseViewProjMatrix(const uint32_t viewID, const uint32_t projID)
{
	WRITE_ZONE();
	if (viewID >= getNumViewMatrices())
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
		}
	}
	else
	{
		if (projID >= getNumProjMatrices())
		{
			if (!mTotalElapsedMS)
			{
				APEX_INVALID_PARAMETER("projection matrix for projID %d is not initialized! see allocProjMatrix()", projID);
			}
		}
		else
		{
#if 0 //lionel: work in progress
			getColMajColVecArray(mViewMatrices[viewID]->viewMatrix, mViewColMajColVecArray);
			getColMajColVecArray(mProjMatrices[projID]->projMatrix, mProjColMajColVecArray);
			multiplyColMajColVecArray(mViewColMajColVecArray, mProjColMajColVecArray, mViewProjColMajColVecArray);
#endif

#ifndef WITHOUT_DEBUG_VISUALIZE
			if (mSceneRenderDebug)
			{
				RENDER_DEBUG_IFACE(mSceneRenderDebug)->setViewMatrix(mViewMatrices[viewID]->viewMatrix.front());
				RENDER_DEBUG_IFACE(mSceneRenderDebug)->setProjectionMatrix(mProjMatrices[projID]->projMatrix.front());
			}
#endif

#if 0 //lionel: work in progress
			//getColVecMat44(mViewProjColMajColVecArray, mViewProjMatrix);	//lionel: need to test
			//mCurrentViewID = viewID;		//lionel : initialize these. will need these when multiple view and prok matrices is supported
			//mCurrentProjID = projID;
#endif
		}
	}
}
#if 0 //lionel: work in progress
const PxMat44& ApexScene::getViewProjMatrix() const
{
	static PxMat44 vp;
	vp = vp.createIdentity();

	if (mViewProjColMajColVecArray == NULL)
	{
		APEX_INVALID_OPERATION("view-projection matrix is not yet set! see setUseViewProjMatrix()");
	}
	else
	{
		return mViewProjMatrix;
	}
	return vp;
}

void ApexScene::getColMajColVecArray(const PxMat44& colVecMat44, float* const result)
{
	*(PxVec4*)(result + 0) = colVecMat44.column0;
	*(PxVec4*)(result + 4) = colVecMat44.column1;
	*(PxVec4*)(result + 8) = colVecMat44.column2;
	*(PxVec4*)(result + 12) = colVecMat44.column3;
}

void ApexScene::getColVecMat44(const float* const colMajColVecArray, PxMat44& result)
{
	result.column0 = PxVec4(colMajColVecArray + 0);
	result.column1 = PxVec4(colMajColVecArray + 4);
	result.column2 = PxVec4(colMajColVecArray + 8);
	result.column3 = PxVec4(colMajColVecArray + 12);
}

void ApexScene::multiplyColMajColVecArray(const float* const fromSpace, const float* const toSpace, float* const result)
{
	/****************************************
	col vector -> P * V * W * vertexVector
	row vector -> vertexVector * W * V * P
	toSpace * fromSpace
	result = rows of 1stMat * cols of 2ndMat
	****************************************/
	uint32_t id = 0;
	for (uint32_t r = 0; r < 4; ++r)
	{
		for (uint32_t c = 0; c < 4; ++c)
		{
			float dotProduct = 0;
			for (uint32_t k = 0; k < 4; ++k)
			{
				dotProduct += toSpace[k * 4 + r] * fromSpace[k + c * 4];
			}
			result[id++] = dotProduct;
		}
	}
}
#endif

PxVec3 ApexScene::getEyePosition(const uint32_t viewID) const
{
	READ_ZONE();
	if (viewID < getNumViewMatrices())
	{
		PxVec3 pos = (mViewMatrices[viewID]->viewMatrix.inverseRT()).column3.getXYZ();
		return pos;
	}
	else
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
		}
	}
	return PxVec3(0, 0, 0);
}

PxVec3 ApexScene::getEyeDirection(const uint32_t viewID) const
{
	READ_ZONE();
	if (viewID < getNumViewMatrices())
	{
		PxVec3 dir;
		dir.x = mViewMatrices[viewID]->viewMatrix.column0.z;
		dir.y = mViewMatrices[viewID]->viewMatrix.column1.z;
		dir.z = mViewMatrices[viewID]->viewMatrix.column2.z;
		ViewMatrixLookAt* v = static_cast<ViewMatrixLookAt*>(mViewMatrices[viewID]);
		if (v->isRightHand)
		{
			dir = -1 * dir;
		}
		return dir;
	}
	else
	{
		APEX_INVALID_PARAMETER("invalid view matrix ID viewID %d! see allocViewMatrix()", viewID);
	}
	return PxVec3(0, 0, 1);
}

//**********************************

void getEyeTransform(PxMat44 &xform,const PxVec3 &eye,const PxVec3 &forward,const PxVec3 &up)
{
	PxVec3 right = forward.cross(up);
	right.normalize();
	PxVec3 realUp = right.cross(forward);
	realUp.normalize();
	xform = PxMat44(right, realUp, -forward, eye);
	xform = xform.inverseRT();
}


void ApexScene::setViewParams(const PxVec3& eyePosition, const PxVec3& eyeDirection, const PxVec3& worldUpDirection, const uint32_t viewID)
{
	WRITE_ZONE();
	if (viewID >= getNumViewMatrices())
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
		}
	}
	else
	{
		ViewMatrixLookAt* v = static_cast<ViewMatrixLookAt*>(mViewMatrices[viewID]);
		getEyeTransform(v->viewMatrix,eyePosition,eyeDirection,worldUpDirection);
	}
}

void ApexScene::setProjParams(float nearPlaneDistance, float farPlaneDistance, float fieldOfViewDegree, uint32_t viewportWidth, uint32_t viewportHeight, const uint32_t projID)
{
	WRITE_ZONE();
	if (projID >= getNumProjMatrices())
	{
		if (!mTotalElapsedMS)
		{
			APEX_INVALID_PARAMETER("projection matrix for projID %d is not initialized! see allocProjMatrix()", projID);
		}
	}
	else
	{
		if (!mProjMatrices[projID]->isUserCustomized)
		{
			if (!mTotalElapsedMS)
			{
				APEX_INVALID_PARAMETER("projection matrix for projID %d is not a user-customized type! see allocProjMatrix()", projID);
			}
		}
		else
		{
			ProjMatrixUserCustomized* p = static_cast<ProjMatrixUserCustomized*>(mProjMatrices[projID]);
			p->nearPlaneDistance = nearPlaneDistance;
			p->farPlaneDistance = farPlaneDistance;
			p->fieldOfViewDegree = fieldOfViewDegree;
			p->viewportWidth = viewportWidth;
			p->viewportHeight = viewportHeight;
		}
	}
}
#if 0 //lionel: work in progress
const PxMat44& ApexScene::buildViewMatrix(const uint32_t viewID)
{
	if (viewID >= getNumViewMatrices())
	{
		APEX_INVALID_PARAMETER("view matrix for viewID %d is not initialized! see allocViewMatrix()", viewID);
	}
	else
	{
		if (!mViewMatrices[viewID]->isLookAt)
		{
			APEX_INVALID_PARAMETER("view matrix for viewID %d is not a LookAt type! see allocViewMatrix()", viewID);
		}
		else
		{
			ViewMatrixLookAt* v = DYNAMIC_CAST(ViewMatrixLookAt*)(mViewMatrices[viewID]);
			if (v->isRightHand)
			{
				//lionel: todo
				//ensure determinant == +ve
				//set view matrix as well?
			}
			else
			{
				//lionel: todo
				//ensure determinant == -ve
				//set view matrix as well?
			}
		}

	}
	//lionel: temp hack
	static PxMat44 hack;
	return hack;
}

const PxMat44& ApexScene::buildProjMatrix(const uint32_t projID)
{
	if (projID >= getNumProjMatrices())
	{
		APEX_INVALID_PARAMETER("projection matrix for projID %d is not initialized! see allocProjMatrix()", projID);
	}
	else
	{
		if (!mProjMatrices[projID]->isPerspectiveFOV)
		{
			APEX_INVALID_PARAMETER("projection matrix for projID %d is a not a perspective FOV type! see allocProjMatrix()", projID);
		}
		else
		{
			ProjMatrixPerspectiveFOV* p = DYNAMIC_CAST(ProjMatrixPerspectiveFOV*)(mProjMatrices[projID]);
			if (p->isZinvert)
			{
				//lionel: todo
				//set proj matrix as well?
				//D3D projection or OGL projection?
			}
			else
			{
				//lionel: todo
				//set proj matrix as well?
			}
		}
	}
	//lionel: temp hack
	static PxMat44 hack;
	return hack;
}
#endif

void ApexScene::initDebugColorParams()
{
	if (mSceneRenderDebug == NULL)
	{
		return;
	}
	using RENDER_DEBUG::DebugColors;
#ifndef WITHOUT_DEBUG_VISUALIZE
#define INIT_COLOR(_name)																		\
	RENDER_DEBUG_IFACE(mSceneRenderDebug)->setDebugColor(DebugColors::_name, mDebugColorParams->_name);	\
	mColorMap.insert(#_name, DebugColors::_name);

	INIT_COLOR(Default);
	INIT_COLOR(PoseArrows);
	INIT_COLOR(MeshStatic);
	INIT_COLOR(MeshDynamic);
	INIT_COLOR(Shape);
	INIT_COLOR(Text0);
	INIT_COLOR(Text1);
	INIT_COLOR(ForceArrowsLow);
	INIT_COLOR(ForceArrowsNorm);
	INIT_COLOR(ForceArrowsHigh);
	INIT_COLOR(Color0);
	INIT_COLOR(Color1);
	INIT_COLOR(Color2);
	INIT_COLOR(Color3);
	INIT_COLOR(Color4);
	INIT_COLOR(Color5);
	INIT_COLOR(Red);
	INIT_COLOR(Green);
	INIT_COLOR(Blue);
	INIT_COLOR(DarkRed);
	INIT_COLOR(DarkGreen);
	INIT_COLOR(DarkBlue);
	INIT_COLOR(LightRed);
	INIT_COLOR(LightGreen);
	INIT_COLOR(LightBlue);
	INIT_COLOR(Purple);
	INIT_COLOR(DarkPurple);
	INIT_COLOR(Yellow);
	INIT_COLOR(Orange);
	INIT_COLOR(Gold);
	INIT_COLOR(Emerald);
	INIT_COLOR(White);
	INIT_COLOR(Black);
	INIT_COLOR(Gray);
	INIT_COLOR(LightGray);
	INIT_COLOR(DarkGray);
#endif
}

void ApexScene::updateDebugColorParams(const char* color, uint32_t val)
{
	WRITE_ZONE();
#ifndef WITHOUT_DEBUG_VISUALIZE
	RENDER_DEBUG_IFACE(mSceneRenderDebug)->setDebugColor(mColorMap[color], val);
#else
	PX_UNUSED(color);
	PX_UNUSED(val);
#endif
}

// A module may call this SceneIntl interface if the module has been released.
void ApexScene::moduleReleased(ModuleSceneIntl& moduleScene)
{
	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		if (mModuleScenes[i] == &moduleScene)
		{
			mModuleScenes.replaceWithLast(i);
			break;
		}
	}
}

// ApexSDKImpl will call this for each module when ApexScene is first created, and
// again for all new modules loaded after the scene was created.
void ApexScene::moduleCreated(ModuleIntl& module)
{
	ModuleSceneIntl* ms = module.createInternalModuleScene(*this, mSceneRenderDebug);
	if (ms)
	{
		mModuleScenes.pushBack(ms);
#if PX_PHYSICS_VERSION_MAJOR == 3
		ms->setModulePhysXScene(mPhysXScene);
#endif
	}
}



const PxRenderBuffer* ApexScene::getRenderBuffer() const
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	READ_ZONE();
	if (mSimulating)
	{
		APEX_INTERNAL_ERROR("simulation is still running");
	}
	else
	{
#ifndef WITHOUT_DEBUG_VISUALIZE
		if (mUseDebugRenderable && mSceneRenderDebug)
		{
			mSceneRenderDebug->getRenderBuffer(mRenderBuffer);
		}
#endif
	}
	return &mRenderBuffer;
#else
	return 0;
#endif
}

const PxRenderBuffer* ApexScene::getRenderBufferScreenSpace() const
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	READ_ZONE();
	if (mSimulating)
	{
		APEX_INTERNAL_ERROR("simulation is still running");
	}
	else
	{
#ifndef WITHOUT_DEBUG_VISUALIZE
		if (mUseDebugRenderable && mSceneRenderDebug)
		{
			mSceneRenderDebug->getRenderBufferScreenSpace(mRenderBufferScreenSpace);
		}
#endif
	}
	return &mRenderBufferScreenSpace;
#else
	return 0;
#endif	
}


#if PX_PHYSICS_VERSION_MAJOR == 3
void ApexScene::setPhysXScene(PxScene* s)
{
	WRITE_ZONE();
	if (mPhysXScene != s)
	{
		/* Pass along to the module scenes */
		for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
		{
			mModuleScenes[i]->setModulePhysXScene(s);
		}
		mPhysXScene = s;

		updateGravity();


		if (mPhysXScene)
		{
			mTaskManager = mPhysXScene->getTaskManager();
		}
		else
		{
			mTaskManager = NULL;
		}
	}
}
#endif


uint32_t ApexScene::addActor(ApexActor& actor, ApexActor* actorPtr)
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	WRITE_ZONE();
	{
		SCOPED_PHYSX_LOCK_WRITE(this);
		actor.setPhysXScene(mPhysXScene);
	}
#endif
	return ApexContext::addActor(actor , actorPtr);
}

void ApexScene::removeAllActors()
{
	WRITE_ZONE();
	if (mSimulating)
	{
		fetchResults(true, NULL);
	}
	ApexContext::removeAllActors();
}

void ApexScene::destroy()
{
	{

		if (mSimulating)
		{
			fetchResults(true, NULL);
		}

		if (mSceneRenderDebug)
		{
			mSceneRenderDebug = NULL;
		}

		if (mDebugRenderParams)
		{
			mDebugRenderParams->destroy();
			mDebugRenderParams = NULL;
		}

		removeAllActors();
	}

#if PX_PHYSICS_VERSION_MAJOR == 3

	using namespace physx;

	PxScene* physXScene = getPhysXScene();

	// Clean up PhysX objects data
	if (physXScene)
	{
		SCOPED_PHYSX_LOCK_WRITE(physXScene);

		uint32_t zombieActorCount = 0;
		uint32_t zombieShapeCount = 0;
		uint32_t zombieDeformableCount = 0;
		uint32_t zombieParticleSystemCount = 0;
		uint32_t zombieParticleFluidCount = 0;

		uint32_t	nbActors;
		PxActor**		actorArray;

		nbActors	= physXScene->getNbActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC);
		if (nbActors)
		{
			actorArray	= (PxActor**)PX_ALLOC(sizeof(PxActor*) * nbActors, PX_DEBUG_EXP("PxActor*"));
			physXScene->getActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC, actorArray, nbActors);
			for (uint32_t actorIndex = 0; actorIndex < nbActors; ++actorIndex)
			{
				PxRigidActor*	actor	= actorArray[actorIndex]->is<physx::PxRigidActor>();

				uint32_t	nbShapes	= actor->getNbShapes();
				PxShape** shapeArray = (PxShape**)PX_ALLOC(sizeof(PxShape*) * nbShapes, PX_DEBUG_EXP("PxShape*"));
				actor->getShapes(shapeArray, nbShapes);
				for (uint32_t shapeIndex = 0; shapeIndex < nbShapes; ++shapeIndex)
				{
					PxShape* shape = shapeArray[shapeIndex];
					if (mApexSDK->getPhysXObjectInfo(shape))
					{
						mApexSDK->releaseObjectDesc(shape);
						++zombieShapeCount;
					}
				}
				if (mApexSDK->getPhysXObjectInfo(actor))
				{
					mApexSDK->releaseObjectDesc(actor);
					++zombieActorCount;
				}
				PX_FREE(shapeArray);
			}
			PX_FREE(actorArray);
		}


		nbActors = physXScene->getNbActors(PxActorTypeFlag::eCLOTH);
		if (nbActors)
		{
			actorArray	= (PxActor**)PX_ALLOC(sizeof(PxActor*) * nbActors, PX_DEBUG_EXP("PxActor*"));
			physXScene->getActors(PxActorTypeFlag::eCLOTH, actorArray, nbActors);
			for (uint32_t actorIndex = 0; actorIndex < nbActors; ++actorIndex)
			{
				PxCloth* cloth = actorArray[actorIndex]->is<physx::PxCloth>();
				PX_ASSERT(cloth);
				if (mApexSDK->getPhysXObjectInfo(cloth))
				{
					mApexSDK->releaseObjectDesc(cloth);
					++zombieDeformableCount;
				}
			}
			PX_FREE(actorArray);
		}


		nbActors	= physXScene->getNbActors(PxActorTypeFlag::ePARTICLE_SYSTEM);
		if (nbActors)
		{
			actorArray	= (PxActor**)PX_ALLOC(sizeof(PxActor*) * nbActors, PX_DEBUG_EXP("PxActor*"));
			physXScene->getActors(PxActorTypeFlag::ePARTICLE_SYSTEM, actorArray, nbActors);
			for (uint32_t actorIndex = 0; actorIndex < nbActors; ++actorIndex)
			{
				PxParticleSystem* particleSystem = actorArray[actorIndex]->is<physx::PxParticleSystem>();
				PX_ASSERT(particleSystem);
				if (mApexSDK->getPhysXObjectInfo(particleSystem))
				{
					mApexSDK->releaseObjectDesc(particleSystem);
					++zombieParticleSystemCount;
				}
			}
			PX_FREE(actorArray);
		}

		nbActors	= physXScene->getNbActors(PxActorTypeFlag::ePARTICLE_FLUID);
		if (nbActors)
		{
			actorArray	= (PxActor**)PX_ALLOC(sizeof(PxActor*) * nbActors, PX_DEBUG_EXP("PxActor*"));
			physXScene->getActors(PxActorTypeFlag::ePARTICLE_FLUID, actorArray, nbActors);
			for (uint32_t actorIndex = 0; actorIndex < nbActors; ++actorIndex)
			{
				PxParticleFluid* particleFluid = actorArray[actorIndex]->is<physx::PxParticleFluid>();
				PX_ASSERT(particleFluid);
				if (mApexSDK->getPhysXObjectInfo(particleFluid))
				{
					mApexSDK->releaseObjectDesc(particleFluid);
					++zombieParticleFluidCount;
				}
			}
			PX_FREE(actorArray);
		}


		if (zombieDeformableCount)
		{
			APEX_DEBUG_WARNING("Removed %d physX deformable actor descriptor(s) still remaining in destroyed ApexScene.", zombieDeformableCount);
		}
		if (zombieParticleSystemCount)
		{
			APEX_DEBUG_WARNING("Removed %d physX particle system actor descriptor(s) still remaining in destroyed ApexScene.", zombieParticleSystemCount);
		}
		if (zombieParticleFluidCount)
		{
			APEX_DEBUG_WARNING("Removed %d physX particle fluid actor descriptor(s) still remaining in destroyed ApexScene.", zombieParticleFluidCount);
		}
		if (zombieActorCount)
		{
			APEX_DEBUG_WARNING("Removed %d physX actor descriptor(s) still remaining in destroyed ApexScene.", zombieActorCount);
		}
		if (zombieShapeCount)
		{
			APEX_DEBUG_WARNING("Removed %d physX shape descriptor(s) still remaining in destroyed ApexScene.", zombieShapeCount);
		}

	}
#endif
	while (mModuleScenes.size())
	{
		mModuleScenes.back()->release();
	}



	freeTasks();

#if PX_PHYSICS_VERSION_MAJOR == 0
	mTaskManager->release();
#else
	setPhysXScene(NULL);
#endif

	PX_DELETE(this);
}


void ApexScene::updateGravity()
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	WRITE_ZONE();
	if (mPhysXScene == NULL)
	{
		return;
	}
	SCOPED_PHYSX_LOCK_READ(mPhysXScene);

	mGravity = mPhysXScene->getGravity();
#endif
}


void ApexScene::simulate(float elapsedTime, 
						 bool finalStep, 
						 PxBaseTask *completionTask,
						 void* scratchMemBlock, 
						 uint32_t scratchMemBlockSize)
{
	PX_UNUSED(scratchMemBlock);
	PX_UNUSED(scratchMemBlockSize);

	WRITE_ZONE();
	if (mApexSDK->getApexPvdClient())
		mApexSDK->getApexPvdClient()->beginFrame(this);

	PX_PROFILE_ZONE("ApexScene::simulate", GetInternalApexSDK()->getContextId());

	// reset the APEX simulation time timer
	APEX_CHECK_STAT_TIMER("--------- Set mApexSimulateTickCount");
	mApexSimulateTickCount = Time::getCurrentCounterValue();

	mFinalStep = finalStep;

	if (mSimulating)
		return;

#if PX_PHYSICS_VERSION_MAJOR == 3
	if (!mPhysXScene)
		return;
#endif
	{
		updateGravity();
	}

	uint32_t manualSubsteps = 0;
	float substepSize = elapsedTime;

	// Wait for all post-fetchResults() tasks to complete before allowing the next
	// simulation step to continue;
	mSimulationComplete.wait();

#if PX_PHYSICS_VERSION_MAJOR == 3
	// make sure we use the apex user notify... if the application
	// changes their custom one make sure we map to it.
	mUserNotify.setBatchAppNotify(manualSubsteps > 1);
#if APEX_UE4
		// Why do we need this check if we'll return earlier in case when mPhysXScene == NULL ? Line: 1177
	if (getPhysXScene())
#endif
	{
		SCOPED_PHYSX_LOCK_WRITE(this);
		PxSimulationEventCallback* userNotify = getPhysXScene()->getSimulationEventCallback();
		if (userNotify != &mUserNotify)
		{
			mUserNotify.setApplicationNotifier(userNotify);
			getPhysXScene()->setSimulationEventCallback(&mUserNotify);
		}
		PxContactModifyCallback* userContactModify = getPhysXScene()->getContactModifyCallback();
		if (userContactModify != &mUserContactModify)
		{
			mUserContactModify.setApplicationContactModify(userContactModify);
			getPhysXScene()->setContactModifyCallback(&mUserContactModify);
		}
	}
#endif

	mElapsedTime = elapsedTime;
	mPhysXSimulateTime = elapsedTime;
	mFetchResultsReady.reset();
	mSimulationComplete.reset();
	mSimulating = true;

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
		mModuleScenes[i]->simulate(elapsedTime);

	// reset dependcies after mModuleScenes[i]->simulate, so they get a chance
	// to wait for running tasks from last frame
	mTaskManager->resetDependencies();

	/* Submit APEX scene tasks */
	mTaskManager->submitNamedTask(mPhysXSimulate, mPhysXSimulate->getName());
#if APEX_DURING_TICK_TIMING_FIX
	mTaskManager->submitNamedTask(mDuringTickComplete, mDuringTickComplete->getName());
#endif
	mTaskManager->submitNamedTask(mCheckResults, mCheckResults->getName());
	mTaskManager->submitNamedTask(mFetchResults, mFetchResults->getName());

	mPhysXSimulate->setElapsedTime(manualSubsteps > 0 ? substepSize : elapsedTime);

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
		mModuleScenes[i]->submitTasks(elapsedTime, substepSize, PxMax(manualSubsteps, 1u));

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
		mModuleScenes[i]->setTaskDependencies();

	/* Build scene dependency graph */
	mCheckResults->startAfter(mPhysXSimulate->getTaskID());

#if APEX_DURING_TICK_TIMING_FIX
	/**
	*	Tasks that run during the PhysX tick (that start after mPhysXSimulate) should 
	*	"finishBefore" mDuringTickComplete.  
	*/
	mDuringTickComplete->startAfter(mPhysXSimulate->getTaskID());
	mFetchResults->startAfter(mDuringTickComplete->getTaskID());
#endif

	mFetchResults->startAfter(mPhysXSimulate->getTaskID());
	mFetchResults->startAfter(mCheckResults->getTaskID());

	if (manualSubsteps > 1)
	{
		PX_ASSERT(mBetweenstepTasks != NULL);
		mBetweenstepTasks->setSubstepSize(substepSize, manualSubsteps);

		mBetweenstepTasks->setFollower(1, mCheckResults);
		mCheckResults->addReference(); // make sure checkresults waits until the last immediate step
	}
	mPhysXSimulate->setFollowingTask(manualSubsteps > 1 ? mBetweenstepTasks : NULL);

#if PX_PHYSICS_VERSION_MAJOR == 3
	mPhysXSimulate->setScratchBlock(scratchMemBlock, scratchMemBlockSize);
#endif
	mFetchResults->setFollowingTask(completionTask);

	{
		PX_PROFILE_ZONE("ApexScene::TaskManager::startSimulation", GetInternalApexSDK()->getContextId());
		mTaskManager->startSimulation();
	}
}


struct ApexPvdClientEndFrameSender
{
	pvdsdk::ApexPvdClient*	mBinding;
	void*				mInstance;
	ApexPvdClientEndFrameSender(pvdsdk::ApexPvdClient* inBinding, void* inInst)
		: mBinding(inBinding)
		, mInstance(inInst)
	{
	}
	~ApexPvdClientEndFrameSender()
	{
		if (mBinding)
		{
			mBinding->endFrame(mInstance);
		}
	}
};


bool ApexScene::fetchResults(bool block, uint32_t* errorState)
{
	WRITE_ZONE();
#if PX_PHYSICS_VERSION_MAJOR == 3
	if (!mPhysXScene)
	{
		return false;
	}
#endif

	{
		StatValue dataVal;
		if (mFetchResultsReady.wait(0))
		{
			dataVal.Float = 0.0f;    // fetchResults was called before simulation was done
		}
		else
		{
			dataVal.Float = ApexScene::ticksToMilliseconds(mApexSimulateTickCount, Time::getCurrentCounterValue());
		}
		setApexStatValue(UserDelayedFetchTime, dataVal);
	}

	if (checkResults(block) == false || !mSimulating)
	{
		return false;
	}

	//absolutely, at function exit, ensure we send the eof marker.
	//PVD needs the EOF marker sent *after* the last fetch results in order to associate this fetch results
	//with this frame.
	//If you change the order of the next two statements it will confuse PVD and your frame will look tremendously
	//long.
	ApexPvdClientEndFrameSender theEnsureEndFrameIsSent(mApexSDK->getApexPvdClient(), this);
	PX_PROFILE_ZONE("ApexScene::fetchResults", GetInternalApexSDK()->getContextId());

	// reset simulation timer to measure fetchResults time
	APEX_CHECK_STAT_TIMER("--------- Set fetchTime");
	uint64_t fetchTime = Time::getCurrentCounterValue();

	// reset simulation
	mSimulating = false;
	if (errorState)
	{
		*errorState = 0;
	}

	// TODO: Post-FetchResults tasks must set this, if/when we support them.
	mSimulationComplete.set();

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		mModuleScenes[i]->fetchResultsPreRenderLock();
	}

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		mModuleScenes[i]->lockRenderResources();
	}
#if PX_PHYSICS_VERSION_MAJOR == 3
	if (mPhysXScene != NULL)
	{
		PX_PROFILE_ZONE("PhysXScene::fetchResults", GetInternalApexSDK()->getContextId());
		
		SCOPED_PHYSX_LOCK_WRITE(this);
		mPhysXScene->fetchResults(true);
		// SJB TODO3.0
		mPxStepWasValid	= true;
		// Check if PhysX actually ran any substeps. (nbSubSteps is the amount of substeps ran during the last simulation)
		/*float maxTimeStep;
		uint32_t nbSubSteps, maxIter;
		NxTimeStepMethod method;
		mPhysXScene->getTiming(maxTimeStep, maxIter, method, &nbSubSteps);

		mPxStepWasValid = (nbSubSteps > 0);*/

		mPxAccumElapsedTime += mElapsedTime;

		if (mPxStepWasValid)
		{
			mPxLastElapsedTime = mPxAccumElapsedTime;
			mPxAccumElapsedTime = 0.0f;

			if (mTimeRemainder + mPxLastElapsedTime > 0.001f)
			{
				uint32_t elapsedMS = (uint32_t)((1000.0f) * (mTimeRemainder + mPxLastElapsedTime));
				mTotalElapsedMS += elapsedMS;
				mTimeRemainder = (mTimeRemainder + mPxLastElapsedTime) - (float)elapsedMS * 0.001f;
			}
		}

		// restore the application user callbacks.
		mPhysXScene->setSimulationEventCallback(mUserNotify.getApplicationNotifier());
		mPhysXScene->setContactModifyCallback(mUserContactModify.getApplicationContactModify());

		mUserNotify.playBatchedNotifications();
	}
#endif

	{
		StatValue dataVal;
		{
			dataVal.Float = ApexScene::ticksToMilliseconds(fetchTime, Time::getCurrentCounterValue());
			APEX_CHECK_STAT_TIMER("--------- PhysXFetchResultTime (fetchTime)");
		}
		setApexStatValue(PhysXFetchResultTime, dataVal);
	}

	// reset simulation timer to measure fetchResults time
	APEX_CHECK_STAT_TIMER("--------- Set mApexSimulateTickCount");
	mApexSimulateTickCount = Time::getCurrentCounterValue();

	fetchPhysXStats();

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		mModuleScenes[i]->fetchResults();    // update render bounds, trigger callbacks, etc
	}

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		mModuleScenes[i]->unlockRenderResources();
	}

	for (uint32_t i = 0 ; i < mModuleScenes.size() ; i++)
	{
		mModuleScenes[i]->fetchResultsPostRenderUnlock();
	}

	mTaskManager->stopSimulation();

	if (mApexSDK->isApexStatsEnabled())
	{
		fetchApexStats();
	}

#if APEX_CUDA_SUPPORT && !defined(INSTALLER)
	mCudaTestManager->nextFrame();
	mCudaProfileManager->nextFrame();
#endif

	return true;
}



void ApexScene::fetchPhysXStats()
{
	WRITE_ZONE();
	PX_PROFILE_ZONE("ApexScene::fetchPhysXStats", GetInternalApexSDK()->getContextId()); 
	StatValue dataVal;

	// get the number of shapes and add it to the ApexStats
	uint32_t nbShapes = 0;
	uint32_t nbPairs = 0;
	uint32_t nbAwakeShapes = 0;

#if PX_PHYSICS_VERSION_MAJOR == 3
# if USE_MANUAL_ACTOR_LOOP
	uint32_t nbActors = 0;
	static const PxActorTypeSelectionFlags	flags = PxActorTypeSelectionFlag::eRIGID_STATIC
		| PxActorTypeSelectionFlag::eRIGID_DYNAMIC;

	if (mPhysXScene)
	{
		nbActors = mPhysXScene->getNbActors(flags);
	}

	if (nbActors)
	{
		PxActor**	actorArray	= (PxActor**)PxAlloca(sizeof(PxActor*) * nbActors);
		mPhysXScene->getActors(flags, actorArray, nbActors);

		for (uint32_t actorIndex = 0; actorIndex < nbActors; ++actorIndex)
		{
			PxRigidActor* rigidActor	= actorArray[actorIndex]->is<physx::PxRigidActor>();
			if (rigidActor)
			{
				nbShapes += rigidActor->getNbShapes();
			}

			PxRigidDynamic*	dynamic		= actorArray[actorIndex]->is<physx::PxRigidDynamic>();
			if (dynamic && !dynamic->isSleeping())
			{
				nbAwakeShapes	+= dynamic->getNbShapes();
			}
		}
	}
# else
	physx::PxSimulationStatistics sceneStats;
	if (mPhysXScene)
	{
		SCOPED_PHYSX_LOCK_READ(mPhysXScene);
		mPhysXScene->getSimulationStatistics(sceneStats);
		nbShapes = sceneStats.nbDynamicBodies;
		nbAwakeShapes = sceneStats.nbActiveDynamicBodies;
		nbPairs = 0;
		for (PxGeometryType::Enum i = PxGeometryType::eSPHERE; i < PxGeometryType::eGEOMETRY_COUNT; i = (PxGeometryType::Enum)(i + 1))
		{
			nbPairs += sceneStats.getRbPairStats(physx::PxSimulationStatistics::eDISCRETE_CONTACT_PAIRS, PxGeometryType::eCONVEXMESH, i);
		}
	}
# endif
#endif

	dataVal.Int = (int32_t)nbShapes;
	setApexStatValue(NumberOfShapes, dataVal);

	dataVal.Int = (int32_t)nbAwakeShapes;
	setApexStatValue(NumberOfAwakeShapes, dataVal);

	dataVal.Int = (int32_t)nbPairs;
	setApexStatValue(NumberOfCpuShapePairs, dataVal);

	dataVal.Int = 0;
	setApexStatValue(RbThroughput, dataVal);
}



void ApexScene::fetchApexStats()
{
	WRITE_ZONE();
	PX_PROFILE_ZONE("ApexScene::fetchApexStats", GetInternalApexSDK()->getContextId());
	StatValue dataVal;

	// get the number of actors and add it to the ApexStats
	dataVal.Int = (int32_t)mActorArray.size();
	setApexStatValue(NumberOfActors, dataVal);


	uint64_t qpc = Time::getCurrentCounterValue();
	dataVal.Float = ApexScene::ticksToMilliseconds(mApexSimulateTickCount, qpc);
	APEX_CHECK_STAT_TIMER("--------- ApexPostTickTime (mApexSimulateTickCount)");

	APEX_CHECK_STAT_TIMER("--------- Set mApexSimulateTickCount");
	mApexSimulateTickCount = qpc;
	setApexStatValue(ApexPostTickTime, dataVal);

	//ModuleScenes can also generate stats. So let's collect and add those stats here.

	for (uint32_t i = 0; i < mModuleScenes.size(); i++)
	{
		SceneStats* moduleSceneStats;
		moduleSceneStats = mModuleScenes[i]->getStats();

		if (moduleSceneStats)
		{
			//O(n^2), rewrite to use a hash if num stats gets much larger
			for (uint32_t j = 0; j < moduleSceneStats->numApexStats; j++)
			{
				StatsInfo& moduleSceneStat = moduleSceneStats->ApexStatsInfoPtr[j];

				uint32_t k = 0;
				while (k != mApexSceneStats.numApexStats && nvidia::strcmp(mApexSceneStats.ApexStatsInfoPtr[k].StatName, moduleSceneStats->ApexStatsInfoPtr[j].StatName) != 0)
				{
					k++;
				}
				bool found = (k != mApexSceneStats.numApexStats);

				if (found)
				{
					StatsInfo& sceneStat = mApexSceneStats.ApexStatsInfoPtr[k];

					PX_ASSERT(sceneStat.StatType == moduleSceneStat.StatType);

					if (sceneStat.StatType == StatDataType::FLOAT)
					{
						sceneStat.StatCurrentValue.Float += moduleSceneStat.StatCurrentValue.Float;
					}
					else if (sceneStat.StatType == StatDataType::INT)
					{
						sceneStat.StatCurrentValue.Int += moduleSceneStat.StatCurrentValue.Int;
					}
				}
			}
		}
	}
}



bool ApexScene::checkResults(bool block) const
{
//	PX_PROFILE_ZONE("ApexScene::checkResults", GetInternalApexSDK()->getContextId());

	uint32_t waitTime = block ? Sync::waitForever : 0;
	if (!mSimulating)
	{
		return true;
	}
	else
	{
		return mFetchResultsReady.wait(waitTime);
	}
}

void ApexScene::lockRenderResources()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mSceneRenderDebug)
	{
		mSceneRenderDebug->lockRenderResources();
	}
#endif
	checkResults(true);
}

void ApexScene::unlockRenderResources()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mSceneRenderDebug)
	{
		mSceneRenderDebug->unlockRenderResources();
	}
#endif
}

void ApexScene::updateRenderResources(bool rewriteBuffers, void* userRenderData)
{
	URR_SCOPE;

#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(rewriteBuffers);
	PX_UNUSED(userRenderData);
#else
	visualize();

	if (mSceneRenderDebug)
	{
		mSceneRenderDebug->updateRenderResources(rewriteBuffers, userRenderData);
	}
#endif
}

void ApexScene::dispatchRenderResources(UserRenderer& renderer)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderer);
#else
	if (mSceneRenderDebug)
	{
		mSceneRenderDebug->dispatchRenderResources(renderer);
	}
#endif
}

void ApexScene::visualize()
{
#ifndef WITHOUT_DEBUG_VISUALIZE
	if (mSceneRenderDebug && mDebugRenderParams->Enable && mDebugRenderParams->Scale!= 0.0f)
	{
		const physx::PxMat44& savedPose = *RENDER_DEBUG_IFACE(mSceneRenderDebug)->getPoseTyped();
		RENDER_DEBUG_IFACE(mSceneRenderDebug)->setIdentityPose();
		if (mDebugRenderParams->Bounds)
		{
			RENDER_DEBUG_IFACE(mSceneRenderDebug)->setCurrentColor(0xFFFFFF);
			for (uint32_t i = 0; i < mActorArray.size(); ++i)
			{
				ApexActor* actor = mActorArray[i];
				RENDER_DEBUG_IFACE(mSceneRenderDebug)->debugBound(actor->getBounds());
			}
		}

		for (ModuleSceneIntl** it = mModuleScenes.begin(); it != mModuleScenes.end(); ++it)
		{
			(*it)->visualize();
		}
		RENDER_DEBUG_IFACE(mSceneRenderDebug)->setPose(savedPose);
	}
#endif
}

PxBounds3 ApexScene::getBounds() const
{
	READ_ZONE();
#ifdef WITHOUT_DEBUG_VISUALIZE
	PxBounds3 bound = PxBounds3::empty();
#else
	PxBounds3 bound = mSceneRenderDebug->getBounds();
#endif

	return bound;
}


void ApexScene::allocateTasks()
{
	mCheckResults = PX_NEW(CheckResultsTask)(*this);
	mPhysXSimulate = PX_NEW(PhysXSimulateTask)(*this, *mCheckResults);
	mBetweenstepTasks = PX_NEW(PhysXBetweenStepsTask)(*this);
#if APEX_DURING_TICK_TIMING_FIX
	mDuringTickComplete = PX_NEW(DuringTickCompleteTask)(*this);
#endif
	mFetchResults = PX_NEW(FetchResultsTask)(*this);
}

void ApexScene::freeTasks()
{
	if (mPhysXSimulate != NULL)
	{
		delete mPhysXSimulate;
		mPhysXSimulate = NULL;
	}

	if (mBetweenstepTasks != NULL)
	{
		delete mBetweenstepTasks;
		mBetweenstepTasks = NULL;
	}

#if APEX_DURING_TICK_TIMING_FIX
	if (mDuringTickComplete != NULL)
	{
		delete mDuringTickComplete;
		mDuringTickComplete = NULL;
	}
#endif

	if (mCheckResults != NULL)
	{
		delete mCheckResults;
		mCheckResults = NULL;
	}

	if (mFetchResults != NULL)
	{
		delete mFetchResults;
		mFetchResults = NULL;
	}
}

void	ApexScene::setUseDebugRenderable(bool state)
{
	WRITE_ZONE();
	mUseDebugRenderable = state;
	if (mSceneRenderDebug)
	{
#if !defined(WITHOUT_DEBUG_VISUALIZE)
		mSceneRenderDebug->setUseDebugRenderable(state);
#endif
	}
}

uint32_t ApexScene::getSeed()
{
	return (uint32_t)(Time::getCurrentCounterValue() & 0xFFFFFFFF );
	//return IgnoredSeed != mSeed ? mSeed : (uint32_t)(1000 * getElapsedTime());
}

ModuleSceneIntl* ApexScene::getInternalModuleScene(const char* moduleName)
{
	ApexSimpleString str1(moduleName);
	for (uint32_t i = 0; i < mModuleScenes.size(); i++)
	{
		ApexSimpleString str2(mModuleScenes[i]->getModule()->getName());
		if (str1 == str2)
		{
			return mModuleScenes[i];
		}
	}
	return NULL;
}

#if PX_PHYSICS_VERSION_MAJOR == 3

void	ApexScene::addActorPair(PxActor *actor0,PxActor *actor1)
{
	WRITE_ZONE();
	mPairFilter.addPair(PTR_TO_UINT64(actor0),PTR_TO_UINT64(actor1));
}

void	ApexScene::removeActorPair(PxActor *actor0,PxActor *actor1)
{
	WRITE_ZONE();
	mPairFilter.removePair(PTR_TO_UINT64(actor0), PTR_TO_UINT64(actor1));
}

bool	ApexScene::findActorPair(PxActor *actor0,PxActor *actor1) const
{
	READ_ZONE();
	return mPairFilter.findPair(PTR_TO_UINT64(actor0), PTR_TO_UINT64(actor1));
}

MirrorScene *ApexScene::createMirrorScene(nvidia::apex::Scene &mirrorScene,
											MirrorScene::MirrorFilter &mirrorFilter,
											float mirrorStaticDistance,
											float mirrorDynamicDistance,
											float mirrorDistanceThreshold)
{
	WRITE_ZONE();
	MirrorSceneImpl *ms = PX_NEW(MirrorSceneImpl)(*getPhysXScene(),*mirrorScene.getPhysXScene(),mirrorFilter,mirrorStaticDistance,mirrorDynamicDistance,mirrorDistanceThreshold);
	return static_cast< MirrorScene *>(ms);
}

#endif

}
} // end namespace nvidia::apex
