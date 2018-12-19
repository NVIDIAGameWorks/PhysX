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

#include "PtBodyTransformVault.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxMemory.h"
#include "PxvGeometry.h"
#include "PxvDynamics.h"
#include "PsHash.h"
#include "PsFoundation.h"

using namespace physx;
using namespace Pt;

BodyTransformVault::BodyTransformVault() : mBody2WorldPool("body2WorldPool", 256), mBodyCount(0)
{
	// Make sure the hash size is a power of 2
	PX_ASSERT((((PT_BODY_TRANSFORM_HASH_SIZE - 1) ^ PT_BODY_TRANSFORM_HASH_SIZE) + 1) ==
	          (2 * PT_BODY_TRANSFORM_HASH_SIZE));

	PxMemSet(mBody2WorldHash, 0, PT_BODY_TRANSFORM_HASH_SIZE * sizeof(Body2World*));
}

BodyTransformVault::~BodyTransformVault()
{
}

PX_FORCE_INLINE PxU32 BodyTransformVault::getHashIndex(const PxsBodyCore& body) const
{
	PxU32 index = Ps::hash(&body);
	return (index & (PT_BODY_TRANSFORM_HASH_SIZE - 1)); // Modulo hash size
}

void BodyTransformVault::addBody(const PxsBodyCore& body)
{
	Body2World* entry;
	Body2World* dummy;

	bool hasEntry = findEntry(body, entry, dummy);
	if(!hasEntry)
	{
		Body2World* newEntry;
		if(entry)
		{
			// No entry for the given body but the hash entry has other bodies
			// --> create new entry, link into list
			newEntry = createEntry(body);
			entry->next = newEntry;
		}
		else
		{
			// No entry for the given body and no hash entry --> create new entry
			PxU32 hashIndex = getHashIndex(body);
			newEntry = createEntry(body);
			mBody2WorldHash[hashIndex] = newEntry;
		}
		newEntry->refCount = 1;
		mBodyCount++;
	}
	else
	{
		entry->refCount++;
	}
}

void BodyTransformVault::removeBody(const PxsBodyCore& body)
{
	Body2World* entry;
	Body2World* prevEntry;

	bool hasEntry = findEntry(body, entry, prevEntry);
	PX_ASSERT(hasEntry);
	PX_UNUSED(hasEntry);

	if(entry->refCount == 1)
	{
		if(prevEntry)
		{
			prevEntry->next = entry->next;
		}
		else
		{
			// Shape entry was first in list
			PxU32 hashIndex = getHashIndex(body);

			mBody2WorldHash[hashIndex] = entry->next;
		}
		mBody2WorldPool.destroy(entry);
		PX_ASSERT(mBodyCount > 0);
		mBodyCount--;
	}
	else
	{
		entry->refCount--;
	}
}

void BodyTransformVault::teleportBody(const PxsBodyCore& body)
{
	Body2World* entry;
	Body2World* dummy;

	bool hasEntry = findEntry(body, entry, dummy);
	PX_ASSERT(hasEntry);
	PX_ASSERT(entry);
	PX_UNUSED(hasEntry);

	PX_CHECK_AND_RETURN(body.body2World.isValid(), "BodyTransformVault::teleportBody: body.body2World is not valid.");

	entry->b2w = body.body2World;
}

const PxTransform* BodyTransformVault::getTransform(const PxsBodyCore& body) const
{
	Body2World* entry;
	Body2World* dummy;

	bool hasEntry = findEntry(body, entry, dummy);
	// PX_ASSERT(hasEntry);
	// PX_UNUSED(hasEntry);
	// PX_ASSERT(entry);
	return hasEntry ? &entry->b2w : NULL;
}

void BodyTransformVault::update()
{
	if(mBodyCount)
	{
		for(PxU32 i = 0; i < PT_BODY_TRANSFORM_HASH_SIZE; i++)
		{
			Body2World* entry = mBody2WorldHash[i];

			while(entry)
			{
				PX_ASSERT(entry->body);
				entry->b2w = entry->body->body2World;
				entry = entry->next;
			}
		}
	}
}

BodyTransformVault::Body2World* BodyTransformVault::createEntry(const PxsBodyCore& body)
{
	Body2World* entry = mBody2WorldPool.construct();

	if(entry)
	{
		entry->b2w = body.body2World;
		entry->next = NULL;
		entry->body = &body;
	}

	return entry;
}

bool BodyTransformVault::isInVaultInternal(const PxsBodyCore& body) const
{
	PxU32 hashIndex = getHashIndex(body);

	if(mBody2WorldHash[hashIndex])
	{
		Body2World* curEntry = mBody2WorldHash[hashIndex];

		while(curEntry->next)
		{
			if(curEntry->body == &body)
				break;

			curEntry = curEntry->next;
		}

		if(curEntry->body == &body)
			return true;
	}

	return false;
}

bool BodyTransformVault::findEntry(const PxsBodyCore& body, Body2World*& entry, Body2World*& prevEntry) const
{
	PxU32 hashIndex = getHashIndex(body);

	prevEntry = NULL;
	bool hasEntry = false;
	if(mBody2WorldHash[hashIndex])
	{
		Body2World* curEntry = mBody2WorldHash[hashIndex];

		while(curEntry->next)
		{
			if(curEntry->body == &body)
				break;

			prevEntry = curEntry;
			curEntry = curEntry->next;
		}

		entry = curEntry;
		if(curEntry->body == &body)
		{
			// An entry already exists for the given body
			hasEntry = true;
		}
	}
	else
	{
		entry = NULL;
	}

	return hasEntry;
}

#endif // PX_USE_PARTICLE_SYSTEM_API
