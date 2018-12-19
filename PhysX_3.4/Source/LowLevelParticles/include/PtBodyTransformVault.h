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

#ifndef PT_BODY_TRANSFORM_VAULT_H
#define PT_BODY_TRANSFORM_VAULT_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxTransform.h"
#include "PsPool.h"
#include "CmPhysXCommon.h"

namespace physx
{

struct PxsBodyCore;

namespace Pt
{

#define PT_BODY_TRANSFORM_HASH_SIZE 1024 // Size of hash table for last frame's body to world transforms
                                         // NOTE: Needs to be power of 2

/*!
Structure to store the current and the last frame's body to world transformations
for bodies that collide with particles.
*/
class BodyTransformVault : public Ps::UserAllocated
{
  public:
	BodyTransformVault();
	~BodyTransformVault();

	void addBody(const PxsBodyCore& body);
	void removeBody(const PxsBodyCore& body);
	void teleportBody(const PxsBodyCore& body);
	const PxTransform* getTransform(const PxsBodyCore& body) const;
	void update();
	PX_FORCE_INLINE bool isInVault(const PxsBodyCore& body) const;
	PX_FORCE_INLINE PxU32 getBodyCount() const
	{
		return mBodyCount;
	}

  private:
	struct Body2World
	{
		PxTransform b2w; // The old transform
		const PxsBodyCore* body;
		Body2World* next;
		PxU32 refCount;
	};

	PX_FORCE_INLINE PxU32 getHashIndex(const PxsBodyCore& body) const;
	PX_FORCE_INLINE Body2World* createEntry(const PxsBodyCore& body);
	PX_FORCE_INLINE bool findEntry(const PxsBodyCore& body, Body2World*& entry, Body2World*& prevEntry) const;

	void updateInternal();
	bool isInVaultInternal(const PxsBodyCore& body) const;

  private:
	Body2World* mBody2WorldHash[PT_BODY_TRANSFORM_HASH_SIZE]; // Hash table for last frames world to shape transforms.
	Ps::Pool<Body2World> mBody2WorldPool;                     // Pool of last frames body to world transforms.
	PxU32 mBodyCount;
};

bool BodyTransformVault::isInVault(const PxsBodyCore& body) const
{
	// if the vault is not even used this should be fast and inlined
	if(mBodyCount == 0)
		return false;

	return isInVaultInternal(body);
}

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_BODY_TRANSFORM_VAULT_H
