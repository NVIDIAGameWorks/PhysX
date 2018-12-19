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



#ifndef PAIR_FILTER_H
#define PAIR_FILTER_H

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"
#include "PsHashSet.h"

#ifndef NULL
#define NULL 0
#endif

namespace nvidia
{

using namespace shdfnd;

class userPair;

class HashKey
{
public:
	HashKey(void)
	{

	}
	// Must always be created in sorted order!
	HashKey(uint64_t _id0,uint64_t _id1)
	{
		if ( _id0 < _id1 )
		{
			id0 = _id0;
			id1 = _id1;
		}
		else
		{
			id0 = _id1;
			id1 = _id0;
		}
	}

	uint32_t hash(void) const
	{
		const uint32_t *h1 = (const uint32_t *)&id0;
		const uint32_t *h2 = (const uint32_t *)&id1;
		uint32_t ret = h1[0];
		ret^=h1[1];
		ret^=h2[0];
		ret^=h2[1];
		return ret;
	}

	bool keyEquals(const HashKey &k) const
	{
		return id0 == k.id0 && id1 == k.id1;
	}

	uint64_t	id0;
	uint64_t	id1;
};

struct HashKeyHash
{
	PX_FORCE_INLINE uint32_t operator()(const HashKey& k) const { return k.hash(); }
	PX_FORCE_INLINE bool equal(const HashKey& k0, const HashKey& k1) const { return k0.keyEquals(k1); }
};

class PairFilter : public UserAllocated
{
public:

	typedef HashSet< HashKey, HashKeyHash > PairMap;

	PairFilter(void)
	{

	}

	~PairFilter(void)
	{

	}

	void	purge(void)
	{
		mPairMap.clear();
	}

	void	addPair(uint64_t id0, uint64_t id1)
	{
		HashKey k(id0,id1);
		mPairMap.insert(k);
	}

	bool	removePair(uint64_t id0, uint64_t id1)
	{
		bool ret = false;
		PX_UNUSED(id0);
		PX_UNUSED(id1);
		HashKey k(id0,id1);
		if ( mPairMap.contains(k) )
		{
			ret = true;
			mPairMap.erase(k);
		}
		return ret;
	}

	bool	findPair(uint64_t id0,uint64_t id1)	const
	{
		HashKey k(id0,id1);
		return mPairMap.contains(k);
	}


private:
	PairMap		mPairMap;
};



};// end of namespace

#endif
