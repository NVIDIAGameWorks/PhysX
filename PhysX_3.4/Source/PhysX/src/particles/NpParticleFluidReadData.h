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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PX_PHYSICS_NP_PARTICLE_FLUID_READ_DATA
#define PX_PHYSICS_NP_PARTICLE_FLUID_READ_DATA

#include "PxParticleFluidReadData.h"
#include "PsFoundation.h"

namespace physx
{

class NpParticleFluidReadData : public PxParticleFluidReadData, public Ps::UserAllocated
{
public:

	NpParticleFluidReadData() :
		mIsLocked(false),
		mFlags(PxDataAccessFlag::eREADABLE)
	{
		strncpy(mLastLockedName, "UNDEFINED", sBufferLength);
	}

	virtual	~NpParticleFluidReadData()
	{}

	// implementation for PxLockedData
	virtual void setDataAccessFlags(PxDataAccessFlags flags) { mFlags = flags; }
	virtual PxDataAccessFlags getDataAccessFlags() { return mFlags; }

	virtual void unlock() { unlockFast(); }

	// internal methods
	void unlockFast() { mIsLocked = false; }

	void lock(const char* callerName)
	{
		if (mIsLocked)
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "PxParticleReadData access through %s while its still locked by last call of %s.", callerName, mLastLockedName);
			PX_ALWAYS_ASSERT_MESSAGE("PxParticleReadData access violation");
		}
		strncpy(mLastLockedName, callerName, sBufferLength); 
		mLastLockedName[sBufferLength-1]=0;
		mIsLocked = true;
	}

private:

	static const PxU32 sBufferLength = 128;
	bool mIsLocked;
	char mLastLockedName[sBufferLength];
	PxDataAccessFlags mFlags;
};

}

#endif
