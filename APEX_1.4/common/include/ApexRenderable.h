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



#ifndef APEX_RENDERABLE_H
#define APEX_RENDERABLE_H

#include "ApexUsingNamespace.h"
#include "PsMutex.h"

#include "PxBounds3.h"

namespace nvidia
{
namespace apex
{

/**
	Base class for implementations of Renderable classes
*/

class ApexRenderable
{
public:
	ApexRenderable()
	{
		mRenderBounds.setEmpty();
	}
	~ApexRenderable()
	{
		// the PS3 Mutex cannot be unlocked without first being locked, so grab the lock
		if (renderDataTryLock())
		{
			renderDataUnLock();
		}
		else
		{
			// someone is holding the lock and should not be, so assert
			PX_ALWAYS_ASSERT();
		}
	}
	void renderDataLock()
	{
		mRenderDataLock.lock();
	}
	void renderDataUnLock()
	{
		mRenderDataLock.unlock();
	}
	bool renderDataTryLock()
	{
		return mRenderDataLock.trylock();
	}
	const PxBounds3&	getBounds() const
	{
		return mRenderBounds;
	}

protected:
	//nvidia::Mutex		mRenderDataLock;
	// Converting to be a PS3 SPU-friendly lock
	// On PC this is the same as a Mutex, on PS3 it is a 128b (!) aligned U32. Subclasses might get bigger on PS3 and they
	// are most likely distributed over more than one cache line.
	AtomicLock	mRenderDataLock;

	PxBounds3	mRenderBounds;
};

}
} // end namespace nvidia::apex

#endif // APEX_RENDERABLE_H
