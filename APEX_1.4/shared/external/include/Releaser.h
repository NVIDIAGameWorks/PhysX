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



#ifndef RELEASER_H
#define RELEASER_H

#include <stdio.h>

#include "ApexInterface.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"

#define RELEASE_AT_EXIT(obj, typeName) Releaser<typeName> obj ## Releaser(obj);

template<typename T>
class Releaser
{
	Releaser(Releaser<T>& rhs);
	Releaser<T>& operator =(Releaser<T>& rhs);

public:
	Releaser(T* obj, void* memory = NULL) : mObj(obj), mMemory(memory) 
	{
	}

	~Releaser() 
	{
		doRelease();
	}

	void doRelease();

	void reset(T* newObj = NULL, void* newMemory = NULL)
	{
		if (newObj != mObj)
		{
			doRelease(mObj);
		}
		mObj = newObj;
		mMemory = newMemory;
	}

private:
	T* mObj;

	void* mMemory;
};

template<> PX_INLINE void Releaser<NvParameterized::Interface>::doRelease()
{
	if (mObj != NULL)
	{
		mObj->destroy();
	}
}

template<> PX_INLINE void Releaser<NvParameterized::Traits>::doRelease()
{
	if (mMemory != NULL)
	{
		mObj->free(mMemory);
	}
}

template<> PX_INLINE void Releaser<nvidia::apex::ApexInterface>::doRelease()
{
	if (mObj != NULL)
	{
		mObj->release();
	}
}

template<> PX_INLINE void Releaser<physx::PxFileBuf>::doRelease()
{
	if (mObj != NULL)
	{
		mObj->release();
	}
}

template<> PX_INLINE void Releaser<FILE>::doRelease()
{
	if (mObj != NULL)
	{
		fclose(mObj);
	}
}

#endif	// RESOURCE_MANAGER_H
