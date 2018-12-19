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



#ifndef __APEX_CUDA_KERNEL_MANAGER__
#define __APEX_CUDA_KERNEL_MANAGER__

#include "ApexDefs.h"
#include "CudaProfileManager.h"

#include "PsMemoryBuffer.h"
#include "ApexString.h"
#include "SceneIntl.h"
#include "PsMutex.h"

namespace nvidia
{
namespace apex
{

class ApexCudaObj;
class ApexCudaFunc;
class ApexCudaProfileManager;

class ApexCudaProfileSession
{
	struct ProfileData
	{
		uint32_t id;
		void* start;
		void* stop;
	};
public:
	ApexCudaProfileSession();
	~ApexCudaProfileSession();

	PX_INLINE void	init(ApexCudaProfileManager* manager)
	{
		mManager = manager;
	}
	void		nextFrame();
	void		start();
	bool		stopAndSave();
	uint32_t		getProfileId(const char* name, const char* moduleName);

	void		onFuncStart(uint32_t id, void* stream);
	void		onFuncFinish(uint32_t id, void* stream);

protected:
	float		flushProfileInfo(ProfileData& pd);

	ApexCudaProfileManager* mManager;
	void*			mTimer;
	nvidia::PsMemoryBuffer	mMemBuf;
	nvidia::Mutex	mLock;
	Array <ProfileData> mProfileDataList;
	float	mFrameStart;
	float	mFrameFinish;
};

/** 
 */
class ApexCudaProfileManager : public CudaProfileManager, public UserAllocated
{
public:
	struct KernelInfo
	{
		ApexSimpleString functionName;
		ApexSimpleString moduleName;
		uint32_t id;
		
		KernelInfo(const char* functionName, const char* moduleName, uint32_t id = 0) 
			: functionName(functionName), moduleName(moduleName), id(id) {}

		bool operator!= (const KernelInfo& ki)
		{
			return		(this->functionName != "*" && this->functionName != ki.functionName) 
					|| 	(this->moduleName != ki.moduleName);
		}
	};

	ApexCudaProfileManager();

	virtual ~ApexCudaProfileManager();

	PX_INLINE void setInternalApexScene(SceneIntl* scene)
	{
		mApexScene = scene;
	}
	void nextFrame();

	// interface for CudaProfileManager
	PX_INLINE void setPath(const char* path)
	{
		mPath = ApexSimpleString(path);
		enable(false);
	}
	void setKernel(const char* functionName, const char* moduleName);
	PX_INLINE void setTimeFormat(TimeFormat tf)
	{
		mTimeFormat = tf;
	}
	void enable(bool state);
	PX_INLINE bool isEnabled() const
	{
		return mState;
	}
		
private:
	bool			mState;
	uint32_t			mSessionCount;
	TimeFormat		mTimeFormat;
	uint32_t			mReservedId;
	ApexSimpleString			mPath;
	Array <KernelInfo>			mKernels;
	ApexCudaProfileSession		mSession;
	SceneIntl*	mApexScene;
	friend class ApexCudaProfileSession;
};

}
} // namespace nvidia::apex

#endif // __APEX_CUDA_KERNEL_MANAGER__
