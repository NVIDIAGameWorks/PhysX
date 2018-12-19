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

#ifndef PT_PARTICLE_CONTACT_MANAGER_STREAM_H
#define PT_PARTICLE_CONTACT_MANAGER_STREAM_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

namespace physx
{

struct PxsRigidCore;

namespace Pt
{

class ParticleShape;

struct ParticleStreamContactManager
{
	const PxsRigidCore* rigidCore;
	const PxsShapeCore* shapeCore;
	const PxTransform* w2sOld;
	bool isDrain;
	bool isDynamic;
};

struct ParticleStreamShape
{
	const ParticleShape* particleShape;
	PxU32 numContactManagers;
	const ParticleStreamContactManager* contactManagers;
};

class ParticleContactManagerStreamWriter
{
  public:
	static PxU32 getStreamSize(PxU32 numParticleShapes, PxU32 numContactManagers)
	{
		return sizeof(PxU32) +                                                // particle shape count
		       sizeof(PxU32) +                                                // stream size
		       numParticleShapes * (sizeof(ParticleShape*) + sizeof(PxU32)) + // shape ll pointer + contact manager
		                                                                      // count per shape
		       numContactManagers * sizeof(ParticleStreamContactManager);     // contact manager data
	}

	PX_FORCE_INLINE ParticleContactManagerStreamWriter(PxU8* stream, PxU32 numParticleShapes, PxU32 numContactManagers)
	: mStream(stream), mNumContactsPtr(NULL)
	{
		PX_ASSERT(mStream);
		*reinterpret_cast<PxU32*>(mStream) = numParticleShapes;
		mStream += sizeof(PxU32);

		*reinterpret_cast<PxU32*>(mStream) = getStreamSize(numParticleShapes, numContactManagers);
		mStream += sizeof(PxU32);
	}

	PX_FORCE_INLINE void addParticleShape(const ParticleShape* particleShape)
	{
		*reinterpret_cast<const ParticleShape**>(mStream) = particleShape;
		mStream += sizeof(const ParticleShape*);

		mNumContactsPtr = reinterpret_cast<PxU32*>(mStream);
		mStream += sizeof(PxU32);

		*mNumContactsPtr = 0;
	}

	PX_FORCE_INLINE void addContactManager(const PxsRigidCore* rigidCore, const PxsShapeCore* shapeCore,
	                                       const PxTransform* w2sOld, bool isDrain, bool isDynamic)
	{
		ParticleStreamContactManager& cm = *reinterpret_cast<ParticleStreamContactManager*>(mStream);
		mStream += sizeof(ParticleStreamContactManager);

		cm.rigidCore = rigidCore;
		cm.shapeCore = shapeCore;
		cm.w2sOld = w2sOld;
		cm.isDrain = isDrain;
		cm.isDynamic = isDynamic;

		PX_ASSERT(mNumContactsPtr);
		(*mNumContactsPtr)++;
	}

  private:
	PxU8* mStream;
	PxU32* mNumContactsPtr;
};

class ParticleContactManagerStreamIterator
{
  public:
	PX_FORCE_INLINE ParticleContactManagerStreamIterator()
	{
	}
	PX_FORCE_INLINE ParticleContactManagerStreamIterator(const PxU8* stream) : mStream(stream)
	{
	}

	ParticleContactManagerStreamIterator getNext(ParticleStreamShape& next)
	{
		const ParticleShape* const* tmp0 = reinterpret_cast<const ParticleShape* const*>(mStream);
		mStream += sizeof(ParticleShape*);
		const PxU32* tmp1 = reinterpret_cast<const PxU32*>(mStream);
		next.particleShape = *tmp0;
		next.numContactManagers = *tmp1;
		mStream += sizeof(PxU32);

		next.contactManagers = reinterpret_cast<const ParticleStreamContactManager*>(mStream);
		mStream += next.numContactManagers * sizeof(ParticleStreamContactManager);

		return ParticleContactManagerStreamIterator(mStream);
	}

	PX_FORCE_INLINE ParticleContactManagerStreamIterator getNext()
	{
		mStream += sizeof(ParticleShape*);
		PxU32 numContactManagers = *reinterpret_cast<const PxU32*>(mStream);

		mStream += sizeof(PxU32);
		mStream += numContactManagers * sizeof(ParticleStreamContactManager);
		return ParticleContactManagerStreamIterator(mStream);
	}

	PX_FORCE_INLINE bool operator==(const ParticleContactManagerStreamIterator& it)
	{
		return mStream == it.mStream;
	}

	PX_FORCE_INLINE bool operator!=(const ParticleContactManagerStreamIterator& it)
	{
		return mStream != it.mStream;
	}

	PX_FORCE_INLINE const PxU8* getStream()
	{
		return mStream;
	}

  private:
	friend class ParticleContactManagerStreamReader;

  private:
	const PxU8* mStream;
};

class ParticleContactManagerStreamReader
{
  public:
	/*
	Reads header of stream consisting of shape count and stream end pointer
	*/
	PX_FORCE_INLINE ParticleContactManagerStreamReader(const PxU8* stream)
	{
		PX_ASSERT(stream);
		mStreamDataBegin = stream;
		mNumParticleShapes = *reinterpret_cast<const PxU32*>(mStreamDataBegin);
		mStreamDataBegin += sizeof(PxU32);
		PxU32 streamSize = *reinterpret_cast<const PxU32*>(mStreamDataBegin);
		mStreamDataBegin += sizeof(PxU32);
		mStreamDataEnd = stream + streamSize;
	}

	PX_FORCE_INLINE ParticleContactManagerStreamIterator getBegin() const
	{
		return ParticleContactManagerStreamIterator(mStreamDataBegin);
	}
	PX_FORCE_INLINE ParticleContactManagerStreamIterator getEnd() const
	{
		return ParticleContactManagerStreamIterator(mStreamDataEnd);
	}
	PX_FORCE_INLINE PxU32 getNumParticleShapes() const
	{
		return mNumParticleShapes;
	}

  private:
	PxU32 mNumParticleShapes;
	const PxU8* mStreamDataBegin;
	const PxU8* mStreamDataEnd;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_CONTACT_MANAGER_STREAM_H
