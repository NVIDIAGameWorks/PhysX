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



#ifndef __APEX_CUDA_TEST__
#define __APEX_CUDA_TEST__

#include "ApexDefs.h"
#include "CudaTestManager.h"

#include "PsMemoryBuffer.h"
#include "ApexString.h"
#include "ApexMirroredArray.h"
#include "SceneIntl.h"

#include "ApexCudaDefs.h"

namespace nvidia
{
namespace apex
{

struct ApexCudaMemRefBase;
class ApexCudaObj;
class ApexCudaFunc;
struct ApexCudaFuncParams;
class ApexCudaTexRef;
class ApexCudaSurfRef;

const uint32_t ApexCudaTestFileVersion = 103;

namespace apexCudaTest
{

struct MemRef
{
	ApexSimpleString	name;
	const void*	gpuPtr;
	size_t		size;
	int32_t		dataOffset;
	uint32_t		bufferOffset;	
	uint32_t		fpType; // Floating point type, if 0 - not a float, else if 4 - float, else if 8 - double

	MemRef(const void* gpuPtr, size_t size, int32_t dataOffset, uint32_t bufferOffset, uint32_t fpType = 0) 
		: gpuPtr(gpuPtr),  size(size), dataOffset(dataOffset), bufferOffset(bufferOffset), fpType(fpType) {}
};

enum ObjTypeEnum
{
	OBJ_TYPE_NONE = 0,
	OBJ_TYPE_TEX_REF_MEM = 1,
	OBJ_TYPE_CONST_MEM = 2,
	OBJ_TYPE_SURF_REF = 4,
	OBJ_TYPE_TEX_REF_ARR = 5
};

enum KernelTypeEnum
{
	KT_SYNC,
	KT_FREE,
	KT_FREE2D,
	KT_FREE3D,
	KT_BOUND
};

}

/** Read cuda kernel context from specified file. Run kernel ant compare output with results from file
*/
class ApexCudaTestKernelContextReader : public UserAllocated
{
	struct Dim3
	{
		int x,y,z;
	};

	struct TexRef
	{
		ApexCudaTexRef* cudaTexRef;
		uint32_t memRefIdx;
		ApexCudaArray* cudaArray;
	};

	struct SurfRef
	{
		ApexCudaSurfRef* cudaSurfRef;
		ApexCudaArray* cudaArray;
		ApexCudaMemFlags::Enum flags;
	};

	struct ArrayRef
	{
		ApexSimpleString	name;
		ApexCudaArray*		cudaArray;
		const uint8_t*		bufferPtr;
		uint32_t			size;

		ArrayRef(const char* name, ApexCudaArray* cudaArray, const uint8_t* bufferPtr, uint32_t size)
		{
			this->name = name;
			this->cudaArray = cudaArray;
			this->bufferPtr = bufferPtr;
			this->size = size;
		}
	};

	struct ParamRef
	{
		ApexSimpleString name;
		uint32_t value;
	};

public:
	ApexCudaTestKernelContextReader(const char* path, SceneIntl* scene);
	~ApexCudaTestKernelContextReader();

	bool	runKernel();

private:
	ApexCudaArray* loadCudaArray();

	void	loadContext(ApexCudaFuncParams& params);
	void	loadTexRef(uint32_t& memOffset, bool bBindToArray);
	void	loadSurfRef();
	void	loadConstMem();
	uint32_t	getParamSize();
	void	loadParam(uint32_t& memOffset, ApexCudaFuncParams& params);

	bool	compare(const uint8_t* resData, const uint8_t* refData, size_t size, uint32_t fpType, const char* name);
	void	dumpParams(char* str);

	nvidia::PsMemoryBuffer*		mMemBuf;

	uint32_t				mCudaObjOffset;
	uint32_t				mParamOffset;

	int					mCuOffset;
	void*				mCuStream;

	ApexSimpleString	mName;
	ApexSimpleString	mModuleName;
	uint32_t				mFrame;
	uint32_t				mCallPerFrame;

	uint32_t				mFuncInstId;
	uint32_t				mSharedSize;
	Dim3				mBlockDim;
	Dim3				mGridDim;
	apexCudaTest::KernelTypeEnum mKernelType;
	uint32_t				mThreadCount[3];
	uint32_t				mBlockCountY;

	ApexCudaObj*		mHeadCudaObj;
	ApexCudaFunc*		mFunc;

	SceneIntl*		mApexScene;
	Array <uint8_t*>		mArgSeq;
	ApexMirroredArray <uint8_t>		mTmpArray;
	PxGpuCopyDescQueue	mCopyQueue;

	Array <apexCudaTest::MemRef>	mInMemRefs;
	Array <apexCudaTest::MemRef>	mOutMemRefs;
	Array <ArrayRef>	mInArrayRefs;
	Array <ArrayRef>	mOutArrayRefs;

	Array <TexRef>		mTexRefs;
	Array <SurfRef>		mSurfRefs;

	uint32_t			mCudaArrayCount;
	ApexCudaArray*		mCudaArrayList;

	Array <ParamRef>	mParamRefs;
};

/** Extract context data from CudaModuleScene about cuda kernel and save it to specified file
*/
class ApexCudaTestKernelContext : public UserAllocated
{
	struct ArrayRef
	{
		CUarray		cuArray;
		uint32_t	bufferOffset;

		ArrayRef(CUarray cuArray, uint32_t bufferOffset)
		{
			this->cuArray = cuArray;
			this->bufferOffset = bufferOffset;
		}
	};

public:
	ApexCudaTestKernelContext(const char* path, const char* functionName, const char* moduleName, uint32_t frame, uint32_t callPerFrame, bool isWriteForNonSuccessfulKernel, bool isKernelForSave);
	~ApexCudaTestKernelContext();

	bool saveToFile();

	PX_INLINE void setCuStream(void* cuStream)
	{
		mCuStream = cuStream;
	}

	void				startObjList();
	void				finishObjList();

	void				setFreeKernel(uint32_t threadCount);
	void				setFreeKernel(uint32_t threadCountX, uint32_t threadCountY);
	void				setFreeKernel(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t blockCountY);
	void				setBoundKernel(uint32_t threadCount);
	void				setSyncKernel();

	void				setBlockDim(uint32_t x, uint32_t y, uint32_t z);
	void				setGridDim(uint32_t x, uint32_t y);

	void				setSharedSize(uint32_t size);
	void				setFuncInstId(int id);

	void				addParam(const char* name, uint32_t align, const void *val, size_t size, int isMemRef = 0, int dataOffset = 0, uint32_t fpType = 0);
	void				addTexRef(const char* name, const void* mem, size_t size, CUarray arr);
	void				addSurfRef(const char* name, CUarray arr, ApexCudaMemFlags::Enum flags);
	void				addConstMem(const char* name, const void* mem, size_t size);
	void				setKernelStatus();

private:
	void				copyMemRefs();
	void				copyArrayRefs();

	uint32_t			addCuArray(CUarray cuArray);

	void				completeCudaObjsBlock();
	void				completeCallParamsBlock();

	PX_INLINE uint32_t	advanceMemBuf(uint32_t size, uint32_t align = 4);
	PX_INLINE void		copyToMemBuf(const apexCudaTest::MemRef& memRef);
	PX_INLINE void		copyToMemBuf(const ArrayRef& arrayRef);

	void*				mCuStream;

	uint32_t				mVersion;
	uint32_t				mFrame;
	uint32_t				mCallPerFrame;
	ApexSimpleString	mName;
	ApexSimpleString	mErrorCode;
	ApexSimpleString	mModuleName;
	ApexSimpleString	mPath;
	nvidia::PsMemoryBuffer		mMemBuf;

	uint32_t				mCudaObjsOffset;
	uint32_t				mCallParamsOffset;
	
	uint32_t				mCudaObjsCounter;
	uint32_t				mCallParamsCounter;

	Array <ArrayRef>	mArrayRefs;
	Array <apexCudaTest::MemRef> mMemRefs;

	bool				mIsCompleteContext;
	bool				mIsWriteForNonSuccessfulKernel;
	bool				mIsContextForSave;
};


/** Class get information what kernels should be tested and give directive for creation ApexCudaTestContext
 */
class ApexCudaTestManager : public CudaTestManager, public UserAllocated
{
	struct KernelInfo
	{
		ApexSimpleString functionName;
		ApexSimpleString moduleName;
		uint32_t callCount;
		
		KernelInfo(const char* functionName, const char* moduleName) 
			: functionName(functionName), moduleName(moduleName), callCount(0) {}

		bool operator!= (const KernelInfo& ki)
		{
			return this->functionName != ki.functionName || this->moduleName != ki.moduleName;
		}
	};

public:
	
	ApexCudaTestManager();
	virtual ~ApexCudaTestManager();

	PX_INLINE void setInternalApexScene(SceneIntl* scene)
	{
		mApexScene = scene;
	}
	void nextFrame();
	ApexCudaTestKernelContext* isTestKernel(const char* functionName, const char* moduleName);

	// interface for CudaTestManager
	PX_INLINE void setWritePath(const char* path)
	{
		mPath = ApexSimpleString(path);
	}
	void setWriteForFunction(const char* functionName, const char* moduleName);

	PX_INLINE void setMaxSamples(uint32_t maxSamples)
	{
		mMaxSamples = maxSamples;
	}
	void setFrames(uint32_t numFrames, const uint32_t* frames)
	{
		for(uint32_t i = 0; i < numFrames && mSampledFrames.size() < mMaxSamples; i++)
		{
			if (frames == NULL) // write next numFrames frames after current
			{
				mSampledFrames.pushBack(mCurrentFrame + i + 1);
			}
			else
			{
				mSampledFrames.pushBack(frames[i]);
			}
		}
	}
	void setFramePeriod(uint32_t period)
	{
		mFramePeriod = period;
	}
	void setCallPerFrameMaxCount(uint32_t cpfMaxCount)
	{
		mCallPerFrameMaxCount = cpfMaxCount;
	}
	void setWriteForNotSuccessfulKernel(bool flag)
	{
		mIsWriteForNonSuccessfulKernel = flag;
	}
/*	void setCallPerFrameSeries(uint32_t callsCount, const uint32_t* calls)
	{
		for(uint32_t i = 0; i < callsCount && mSampledCallsPerFrame.size() < mCallPerFrameMaxCount; i++)
		{
			mSampledCallsPerFrame.pushBack(calls[i]);
		}
	}*/
	bool runKernel(const char* path);
	
private:
	SceneIntl*	mApexScene;
	uint32_t	mCurrentFrame;
	uint32_t	mMaxSamples;
	uint32_t	mFramePeriod;
	uint32_t	mCallPerFrameMaxCount;
	bool	mIsWriteForNonSuccessfulKernel;
	ApexSimpleString					mPath;
	Array <uint32_t>						mSampledFrames;
	//Array <uint32_t>						mSampledCallsPerFrame;
	Array <KernelInfo>					mKernels;
	Array <ApexCudaTestKernelContext*>	mContexts;
};

}
} // namespace nvidia::apex

#endif // __APEX_CUDA_TEST__
