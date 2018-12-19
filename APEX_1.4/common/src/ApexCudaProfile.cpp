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



#include "ApexDefs.h"
#if APEX_CUDA_SUPPORT && !defined(INSTALLER)

#include "ApexCudaProfile.h"
#include "ApexCudaWrapper.h"
#include <cuda.h>
#include "ModuleIntl.h"
#include "ApexSDKHelpers.h"

namespace nvidia
{
namespace apex
{

	ApexCudaProfileSession::ApexCudaProfileSession()
		: mTimer(NULL)
		, mFrameStart(PX_MAX_F32)
		, mFrameFinish(0.f)
	{
		mMemBuf.setEndianMode(nvidia::PsMemoryBuffer::ENDIAN_LITTLE);
	}
	ApexCudaProfileSession::~ApexCudaProfileSession()
	{
		if (mTimer)
		{
			CUT_SAFE_CALL(cuEventDestroy((CUevent)mTimer));
		}
	}

	void ApexCudaProfileSession::nextFrame()
	{
		mFrameStart = PX_MAX_F32;
		mFrameFinish = 0.f;
		float sumElapsed = 0.f;
		for (uint32_t i = 0; i < mProfileDataList.size(); i++)
		{
			sumElapsed += flushProfileInfo(mProfileDataList[i]);
		}
		
		// Write frame as fictive event
		uint32_t op = 1, id = 0;
		uint64_t start = static_cast<uint64_t>(mFrameStart * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&start, sizeof(start));
		mMemBuf.write(&id, sizeof(id));

		op = 2;
		uint64_t stop = static_cast<uint64_t>(mFrameFinish * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stop, sizeof(stop));
		mMemBuf.write(&id, sizeof(id));

		// Write summary of elapsed gpu kernel time as event
		op = 1, id = 1;
		start = static_cast<uint64_t>(mFrameStart * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&start, sizeof(start));
		mMemBuf.write(&id, sizeof(id));

		op = 2;
		stop = static_cast<uint64_t>((mFrameStart + sumElapsed) * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stop, sizeof(stop));
		mMemBuf.write(&id, sizeof(id));

		mProfileDataList.clear();
	}
	
	void ApexCudaProfileSession::start()
	{
		if (!mManager || !mManager->mApexScene) return;
		
		mLock.lock();

		mMemBuf.seekWrite(0);
		uint32_t op = 0, sz, id = 0;
		const char* frameEvent = "Frame"; sz = sizeof(frameEvent);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&sz, sizeof(sz));
		mMemBuf.write(frameEvent, sz);
		mMemBuf.write(&id, sizeof(id));
		
		const char* summaryElapsed = "Summary of elapsed time"; sz = sizeof(summaryElapsed);
		id = 1;
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&sz, sizeof(sz));
		mMemBuf.write(summaryElapsed, sz);
		mMemBuf.write(&id, sizeof(id));

		//Register kernels
		for (uint32_t i = 0; i < mManager->mKernels.size(); i++)
		{
			ApexCudaProfileManager::KernelInfo& ki = mManager->mKernels[i];
			sz = ki.functionName.size();
			mMemBuf.write(&op, sizeof(op));
			mMemBuf.write(&sz, sizeof(sz));
			mMemBuf.write(ki.functionName.c_str(), sz);
			mMemBuf.write(&ki.id, sizeof(ki.id));
			
			ModuleSceneIntl* moduleScene = mManager->mApexScene->getInternalModuleScene(ki.moduleName.c_str());
			ApexCudaObj* obj = NULL;
			if (moduleScene)
			{
				obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
			}
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::FUNCTION)				
				{				
					if (ApexSimpleString(DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName()) == ki.functionName)
					{
						DYNAMIC_CAST(ApexCudaFunc*)(obj)->setProfileSession(this);
						break;
					}
				}
				obj = obj->next();
			}
		}

		{
			PxCudaContextManager* ctx = mManager->mApexScene->getTaskManager()->getGpuDispatcher()->getCudaContextManager();
			PxScopedCudaLock s(*ctx);

			//Run timer
			if (mTimer == NULL)
			{
				CUT_SAFE_CALL(cuEventCreate((CUevent*)&mTimer, CU_EVENT_DEFAULT));
			}
			CUT_SAFE_CALL(cuEventRecord((CUevent)mTimer, 0));
		}		
		mLock.unlock();
	}

	uint32_t ApexCudaProfileSession::getProfileId(const char* name, const char* moduleName)
	{
		Array <ApexCudaProfileManager::KernelInfo>::Iterator it 
			= mManager->mKernels.find(ApexCudaProfileManager::KernelInfo(name, moduleName));
		if (it != mManager->mKernels.end())
		{
			return it->id;
		}
		return 0;
	}

	void ApexCudaProfileSession::onFuncStart(uint32_t id, void* stream)
	{
		mLock.lock();
		CUevent start;
		CUevent stop;

		CUT_SAFE_CALL(cuEventCreate(&start, CU_EVENT_DEFAULT));
		CUT_SAFE_CALL(cuEventCreate(&stop, CU_EVENT_DEFAULT));

		CUT_SAFE_CALL(cuEventRecord(start, (CUstream)stream));

		ProfileData data;
		data.id = id;
		data.start = start;
		data.stop = stop;
		mProfileDataList.pushBack(data);
		
	}
	void ApexCudaProfileSession::onFuncFinish(uint32_t id, void* stream)
	{
		PX_UNUSED(id);
		ProfileData& data = mProfileDataList.back();
		PX_ASSERT(data.id == id);

		CUT_SAFE_CALL(cuEventRecord((CUevent)data.stop, (CUstream)stream));
		
		mLock.unlock();
	}

	float ApexCudaProfileSession::flushProfileInfo(ProfileData& pd)
	{
		CUevent start = (CUevent)pd.start;
		CUevent stop = (CUevent)pd.stop;

		uint32_t op = 1;
		float startTf = 0.f, stopTf = 0.f;
		uint64_t startT = 0, stopT = 0;
		CUT_SAFE_CALL(cuEventSynchronize(start));		
		CUT_SAFE_CALL(cuEventElapsedTime(&startTf, (CUevent)mTimer, start));		
		startT = static_cast<uint64_t>(startTf * mManager->mTimeFormat) ;
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&startT, sizeof(startT));
		mMemBuf.write(&pd.id, sizeof(pd.id));

		op = 2;
		CUT_SAFE_CALL(cuEventSynchronize((CUevent)stop));
		CUT_SAFE_CALL(cuEventElapsedTime(&stopTf, (CUevent)mTimer, (CUevent)stop));
		stopT = static_cast<uint64_t>(stopTf * mManager->mTimeFormat);
		mMemBuf.write(&op, sizeof(op));
		mMemBuf.write(&stopT, sizeof(stopT));
		mMemBuf.write(&pd.id, sizeof(pd.id));

		CUT_SAFE_CALL(cuEventDestroy((CUevent)start));
		CUT_SAFE_CALL(cuEventDestroy((CUevent)stop));

		mFrameStart = PxMin(mFrameStart, startTf);
		mFrameFinish = PxMax(mFrameFinish, stopTf);
		return stopTf - startTf;
	}

	bool ApexCudaProfileSession::stopAndSave()
	{
		if (!mManager || !mManager->mApexScene) return false;

		//unregister functions
		for (uint32_t i = 0; i < mManager->mKernels.size(); i++)
		{
			ApexCudaProfileManager::KernelInfo& ki = mManager->mKernels[i];
			
			ModuleSceneIntl* moduleScene = mManager->mApexScene->getInternalModuleScene(ki.moduleName.c_str());
			ApexCudaObj* obj = NULL;
			if (moduleScene)
			{
				obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
			}
			while(obj)
			{
				if (obj->getType() == ApexCudaObj::FUNCTION)				
				{				
					if (ApexSimpleString(DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName()) == ki.functionName)
					{
						DYNAMIC_CAST(ApexCudaFunc*)(obj)->setProfileSession(NULL);
						break;
					}
				}
				obj = obj->next();
			}
		}

		//save to file
		ApexSimpleString path(mManager->mPath);
		path += ApexSimpleString("profileSesion_");
		path += ApexSimpleString(mManager->mSessionCount, 3);
		FILE* saveFile = fopen(path.c_str(), "wb");
		if (saveFile)
		{
			fwrite(mMemBuf.getWriteBuffer(), mMemBuf.getWriteBufferSize(), 1, saveFile);
			return !fclose(saveFile);
		}
		return false;
	}

	ApexCudaProfileManager::ApexCudaProfileManager()
		: mState(false)
		, mTimeFormat(NANOSECOND)
		, mSessionCount(0)
		, mReservedId(2)
	{
		mSession.init(this);
	}

	ApexCudaProfileManager::~ApexCudaProfileManager()
	{
	}

	void ApexCudaProfileManager::setKernel(const char* functionName, const char* moduleName)
	{
		if (mKernels.find(KernelInfo(functionName, moduleName)) == mKernels.end())
		{
			if (ApexSimpleString(functionName) == "*")
			{
				//Add all function registered in module
				ModuleSceneIntl* moduleScene = mApexScene->getInternalModuleScene(moduleName);
				ApexCudaObj* obj = NULL;
				if (moduleScene)
				{
					obj = static_cast<ApexCudaObj*>(moduleScene->getHeadCudaObj());
				}
				while(obj)
				{
					if (obj->getType() == ApexCudaObj::FUNCTION)				
					{
						const char* name = DYNAMIC_CAST(ApexCudaFunc*)(obj)->getName();
						if (mKernels.find(KernelInfo(name, moduleName)) == mKernels.end())
						{
							mKernels.pushBack(KernelInfo(name, moduleName, mKernels.size() + mReservedId));
						}
					}
					obj = obj->next();
				}
			}
			else
			{
				mKernels.pushBack(KernelInfo(functionName, moduleName, mKernels.size() + mReservedId));
			}
			enable(false);
		}
	}

	void ApexCudaProfileManager::enable(bool state)
	{
		if (state != mState)
		{
			if (state)
			{
				mSession.start();
				mSessionCount++;
			}
			else
			{
				mSession.stopAndSave();
			}
		}
		mState = state;
	}

	void ApexCudaProfileManager::nextFrame()
	{
		if (mApexScene && mState)
		{
			mSession.nextFrame();
		}
	}
}
} // namespace nvidia::apex

#endif
