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

#include "CmRenderOutput.h"
#include "CmFlushPool.h"
#include "BpBroadPhaseMBPCommon.h"
#include "BpSimpleAABBManager.h"
#include "BpBroadPhase.h"
#include "PsFoundation.h"
#include "PsHash.h"
#include "PsSort.h"
#include "PsHashSet.h"
#include "PsVecMath.h"
#include "GuInternal.h"
//#include <stdio.h>

using namespace physx;
using namespace Cm;
using namespace Ps::aos;


// PT: TODO: use PX_INTEL_FAMILY for overlap macros
// PT: TODO: port MBP_Pair optim to MBP
// PT: TODO: benchmark for aggregate creation/deletion
// PT: TODO: "mixed" benchmark where objects are added/removed/updated from aggregates (not just benchmarking one specific part)
// PT: TODO: cleanup code, add/fix comments, fix AABBManager UTs
// PT: TODO: revisit bool getStateChanged() const { return mStateChanged; }
// => it triggers various DMAs in the Cuda code, some of them not always necessary
// PT: TODO: PEEL
// PT: TODO: remove random lines and check that at least one UT fails => e.g. outputDeletedOverlaps
// PT: TODO: UTs for all combinations of add/remove aggregated/aggregate and dirty lists and stuff
// PT: TODO: origin shift
// PT: TODO: eventually revisit choices of data structures/etc (LLs, arrays, hashmaps, arays of pointers, arrays of objects...)
// PT: TODO: consider dropping actor-agg pairs and replace with actorS-agg pairs
// PT: TODO: different kinds of dirty?
// PT: TODO: changed/deleted/updated bitmaps are inconsistent: changed/deleted maps are owned by the manager, always as large as the max amount of objects,
// and use unbounded tests. Updated map is owned by the scene, only as large as the largest updated object, and uses bounded tests. What's the story here?
// PT: TODO: remove the sort of updated objects. Best way is to move the updated map inside the manager, and intercept the scene-calls accessing it. That
// way when an aggregated is marked dirty we can mark its aggregate dirty at the same time, and we can also reuse that map for dirty aggregates, removing
// the need for the dirty aggregates array, the dirty index, etc
// PT: TODO: the whole hashmap things may not be needed if the BP can provide a pair pointer or a pair handle....
// PT: TODO: multi-thread, MBP, micro optims
// PT: TODO: consider storing all pairs in the same hashmap
// PT: TODO: when creating an object with a new id, add asserts that no interaction exists with this id
// PT: TODO: investigate how it's done in Sc with interactions. It should be the same kind of problems. Could one part benefit from better ideas from the other part? could we share the code?
// PT: TODO: optimize redundant hash computations for pairs (inserted in hashmap & hashset)
// PT: TODO: static aggs => add a benchmark for that
// PT: TODO: revisit SIMD overlap (move first part out of the test)
// PT: TODO: revisit pair deltas (10% slower than trunk). Options are a PM instead of the two arrays+hashset, or try to see if we can output pairs in sorted order (in which case
// we can simply go over the previous and current arrays at the same time, no need for a hashset). Maybe the best option would be to reuse the MBP region code, because it would handle
// "sleeping" objects directly.... Or just use bitmaps?
// PT: TODO: idea: store the hash in the pair structure somehow, to avoid recomputing it each frame. Useful?
// PT: TODO: use MBP hash instead of AggPair hash, consider using a shared PM to get rid of the last hashset thing (last part of post-BP)
// PT: TODO: consider optims when objects are marked as updated but bounds didn't actually change
// PT: TODO: use shared MBP PM between aggs? ==> not sure it would work, would have to put back the bitmap test for updated objects in the create/delete final loop (same as in MBP)
// PT: TODO: optimize memory allocations
// PT: TODO: try to refactor/share pruning code => seems to have a significant impact on perf?
// PT: TODO: revisit forceinline for addPair. Seems to help but is it worth it?
// PT: TODO: handle pair deltas differently in each case? For example for actor-vs-aggregate a "bitmap" makes more sense => the bit can be hidden in the aggregated index, removing the need
// for extra memory and bitmap promotion => doesn't work easily since the aggregated array is shared between all actor-aggregate pairs. So the bits have to be in the persistent pairs structure,
// which creates an issue when we remove an object and the bitmap must be reorganized as well. Unless we leave a hole in it I guess. Or can we fix it in "computeBounds" or something?

static const bool gSingleThreaded = false;
static const bool gUseBruteForce = false;
//#define EXPERIMENT
#define USE_MBP_PAIR_MANAGER
#define STORE_SORTED_BOUNDS
#if PX_INTEL_FAMILY && !defined(PX_SIMD_DISABLED)
	#define USE_SIMD_BOUNDS
#endif

namespace physx
{

namespace Bp
{
static PX_FORCE_INLINE uint32_t hash(const Pair& p)
{
	return PxU32(Ps::hash( (p.mID0&0xffff)|(p.mID1<<16)) );
}

static PX_FORCE_INLINE uint32_t hash(const AggPair& p)
{
	return PxU32(Ps::hash( (p.mIndex0&0xffff)|(p.mIndex1<<16)) );
}

static PX_FORCE_INLINE bool shouldPairBeDeleted(const Ps::Array<Bp::FilterGroup::Enum, Ps::VirtualAllocator>& groups, ShapeHandle h0, ShapeHandle h1)
{
	PX_ASSERT(h0<groups.size());
	PX_ASSERT(h1<groups.size());
	return (groups[h0]==Bp::FilterGroup::eINVALID) || (groups[h1]==Bp::FilterGroup::eINVALID);
}

// PT: TODO: refactor with CCT version
template <class T> 
static void resetOrClear(T& a)
{
	const PxU32 c = a.capacity();
	const PxU32 s = a.size();
	if(s>=c/2)
		a.clear();
	else
		a.reset();
}

///

#ifdef USE_SIMD_BOUNDS
	typedef SIMD_AABB	InflatedAABB;
	typedef PxU32		InflatedType;
#else
	typedef PxBounds3	InflatedAABB;
	typedef float		InflatedType;
#endif

	// PT: TODO: revisit/optimize all that stuff once it works
	class Aggregate : public Ps::UserAllocated
	{
		public:
														Aggregate(BoundsIndex index, bool selfCollisions);
														~Aggregate();

						BoundsIndex						mIndex;
		private:
						Ps::Array<BoundsIndex>			mAggregated;	// PT: TODO: replace with linked list?
		public:
						PersistentSelfCollisionPairs*	mSelfCollisionPairs;
						PxU32							mDirtyIndex;	// PT: index in mDirtyAggregates
						InflatedAABB*					mInflatedBounds;
						PxU32							mAllocatedSize;

		PX_FORCE_INLINE	PxU32							getNbAggregated()		const	{ return mAggregated.size();					}
		PX_FORCE_INLINE	BoundsIndex						getAggregated(PxU32 i)	const	{ return mAggregated[i];						}
		PX_FORCE_INLINE	const BoundsIndex*				getIndices()			const	{ return mAggregated.begin();					}
		PX_FORCE_INLINE	void							addAggregated(BoundsIndex i)	{ mAggregated.pushBack(i);						}
		PX_FORCE_INLINE	bool							removeAggregated(BoundsIndex i)	{ return mAggregated.findAndReplaceWithLast(i);	}	// PT: TODO: optimize?

		PX_FORCE_INLINE	void							resetDirtyState()				{ mDirtyIndex = PX_INVALID_U32;				}
		PX_FORCE_INLINE	bool							isDirty()				const	{ return mDirtyIndex != PX_INVALID_U32;		}
		PX_FORCE_INLINE void							markAsDirty(Ps::Array<Aggregate*>& dirtyAggregates)
														{
															if(!isDirty())
															{
																mDirtyIndex = dirtyAggregates.size();
																dirtyAggregates.pushBack(this);
															}
														}

						void							allocateBounds();
						void							computeBounds(const BoundsArray& boundsArray, const float* contactDistances)	/*PX_RESTRICT*/;

#ifdef STORE_SORTED_BOUNDS
		PX_FORCE_INLINE	void							getSortedMinBounds()
														{
															if(mDirtySort)
																sortBounds();
														}
#else
		PX_FORCE_INLINE	const PxU32*					getSortedMinBounds()
														{
															if(mDirtySort)
																sortBounds();
															return mRS.GetRanks();
														}
#endif

						PxBounds3						mBounds;
		private:
#ifndef STORE_SORTED_BOUNDS
						Cm::RadixSortBuffered			mRS;
#endif
						bool							mDirtySort;

						void							sortBounds();
						PX_NOCOPY(Aggregate)
	};

///

#ifdef USE_MBP_PAIR_MANAGER
namespace
{
#define MBP_ALLOC(x)		PX_ALLOC(x, "MBP")
#define MBP_ALLOC_TMP(x)	PX_ALLOC_TEMP(x, "MBP_TMP")
#define MBP_FREE(x)			if(x)	PX_FREE_AND_RESET(x)
#define	INVALID_ID	0xffffffff
#define INVALID_USER_ID	0xffffffff

	// PT: TODO: consider sharing this with the MBP code but we cannot atm because of the extra "usrData" in the other structure
/*	struct MBP_Pair : public Ps::UserAllocated
	{
		PxU32		id0;
		PxU32		id1;
		// TODO: optimize memory here
		bool		isNew;
		bool		isUpdated;

		PX_FORCE_INLINE	PxU32	getId0()	const	{ return id0;		}
		PX_FORCE_INLINE	PxU32	getId1()	const	{ return id1;		}

		PX_FORCE_INLINE	bool	isNew()		const	{ return isNew;		}
		PX_FORCE_INLINE	bool	isUpdated()	const	{ return isUpdated;	}

		PX_FORCE_INLINE	void	setNewPair(PxU32 id0_, PxU32 id1_)
		{
			p->id0		= id0_;	// ### CMOVs would be nice here
			p->id1		= id1_;
			p->isNew	= true;
			p->isUpdated= false;
		}

		PX_FORCE_INLINE	void	setUpdated()		{ isUpdated = true;		}
		PX_FORCE_INLINE	void	clearUpdated()		{ isUpdated = false;	}
		PX_FORCE_INLINE	void	clearNew()			{ isNew = false;		}
	};
	static PX_FORCE_INLINE bool differentPair(const MBP_Pair& p, PxU32 id0, PxU32 id1)	{ return (id0!=p.id0) || (id1!=p.id1);	}*/

	struct MBP_Pair : public Ps::UserAllocated
	{
		PX_FORCE_INLINE	PxU32	getId0()	const	{ return id0_isNew & ~PX_SIGN_BITMASK;		}
		PX_FORCE_INLINE	PxU32	getId1()	const	{ return id1_isUpdated & ~PX_SIGN_BITMASK;	}

		PX_FORCE_INLINE	PxU32	isNew()		const	{ return id0_isNew & PX_SIGN_BITMASK;		}
		PX_FORCE_INLINE	PxU32	isUpdated()	const	{ return id1_isUpdated & PX_SIGN_BITMASK;	}

		PX_FORCE_INLINE	void	setNewPair(PxU32 id0, PxU32 id1)
		{
			PX_ASSERT(!(id0 & PX_SIGN_BITMASK));
			PX_ASSERT(!(id1 & PX_SIGN_BITMASK));
			id0_isNew = id0 | PX_SIGN_BITMASK;
			id1_isUpdated = id1;
		}
		PX_FORCE_INLINE	void	setUpdated()		{ id1_isUpdated |= PX_SIGN_BITMASK;		}
		PX_FORCE_INLINE	void	clearUpdated()		{ id1_isUpdated &= ~PX_SIGN_BITMASK;	}
		PX_FORCE_INLINE	void	clearNew()			{ id0_isNew &= ~PX_SIGN_BITMASK;		}

		protected:
		PxU32		id0_isNew;
		PxU32		id1_isUpdated;
	};
	static PX_FORCE_INLINE bool differentPair(const MBP_Pair& p, PxU32 id0, PxU32 id1)	{ return (id0!=p.getId0()) || (id1!=p.getId1());	}

	struct MBPEntry;
	struct RegionHandle;
	struct MBP_Object;

	class MBP_PairManager : public Ps::UserAllocated
	{
		public:
											MBP_PairManager();
											~MBP_PairManager();

						void				purge();
						void				shrinkMemory();

		PX_FORCE_INLINE	MBP_Pair*			addPair				(PxU32 id0, PxU32 id1/*, const BpHandle* PX_RESTRICT groups = NULL, const MBP_Object* objects = NULL*/);

						PxU32				mHashSize;
						PxU32				mMask;
						PxU32				mNbActivePairs;
						PxU32*				mHashTable;
						PxU32*				mNext;
						MBP_Pair*			mActivePairs;
						PxU32				mReservedMemory;

		PX_FORCE_INLINE	MBP_Pair*			findPair(PxU32 id0, PxU32 id1, PxU32 hashValue) const;
						void				removePair(PxU32 id0, PxU32 id1, PxU32 hashValue, PxU32 pairIndex);
						void				reallocPairs();
		PX_NOINLINE		PxU32				growPairs(PxU32 fullHashValue);
	};


static PX_FORCE_INLINE void sort(PxU32& id0, PxU32& id1)							{ if(id0>id1)	Ps::swap(id0, id1);		}

///////////////////////////////////////////////////////////////////////////////

MBP_PairManager::MBP_PairManager() :
	mHashSize		(0),
	mMask			(0),
	mNbActivePairs	(0),
	mHashTable		(NULL),
	mNext			(NULL),
	mActivePairs	(NULL),
	mReservedMemory (0)
{
}

///////////////////////////////////////////////////////////////////////////////

MBP_PairManager::~MBP_PairManager()
{
	purge();
}

///////////////////////////////////////////////////////////////////////////////

void MBP_PairManager::purge()
{
	MBP_FREE(mNext);
	MBP_FREE(mActivePairs);
	MBP_FREE(mHashTable);
	mHashSize		= 0;
	mMask			= 0;
	mNbActivePairs	= 0;
}

///////////////////////////////////////////////////////////////////////////////

	static PX_FORCE_INLINE PxU32 hash(PxU32 id0, PxU32 id1)
	{
		return PxU32(Ps::hash( (id0&0xffff)|(id1<<16)) );
	}

	static PX_FORCE_INLINE void storeDwords(PxU32* dest, PxU32 nb, PxU32 value)
	{
		while(nb--)
			*dest++ = value;
	}

///////////////////////////////////////////////////////////////////////////////

// Internal version saving hash computation
PX_FORCE_INLINE MBP_Pair* MBP_PairManager::findPair(PxU32 id0, PxU32 id1, PxU32 hashValue) const
{
	if(!mHashTable)
		return NULL;	// Nothing has been allocated yet

	MBP_Pair* PX_RESTRICT activePairs = mActivePairs;
	const PxU32* PX_RESTRICT next = mNext;

	// Look for it in the table
	PxU32 offset = mHashTable[hashValue];
	while(offset!=INVALID_ID && differentPair(activePairs[offset], id0, id1))
	{
		PX_ASSERT(activePairs[offset].getId0()!=INVALID_USER_ID);	// PT: TODO: this one is not possible with the new encoding of id0
		offset = next[offset];		// Better to have a separate array for this
	}
	if(offset==INVALID_ID)
		return NULL;
	PX_ASSERT(offset<mNbActivePairs);
	// Match mActivePairs[offset] => the pair is persistent
	return &activePairs[offset];
}

///////////////////////////////////////////////////////////////////////////////

PX_NOINLINE PxU32 MBP_PairManager::growPairs(PxU32 fullHashValue)
{
	// Get more entries
	mHashSize = Ps::nextPowerOfTwo(mNbActivePairs+1);
	mMask = mHashSize-1;

	reallocPairs();

	// Recompute hash value with new hash size
	return fullHashValue & mMask;
}

PX_FORCE_INLINE MBP_Pair* MBP_PairManager::addPair(PxU32 id0, PxU32 id1/*, const BpHandle* PX_RESTRICT groups, const MBP_Object* objects*/)
{
	PX_ASSERT(id0!=INVALID_ID);
	PX_ASSERT(id1!=INVALID_ID);

/*	if(groups)
	{
		const MBP_ObjectIndex index0 = decodeHandle_Index(id0);
		const MBP_ObjectIndex index1 = decodeHandle_Index(id1);

		const BpHandle object0 = objects[index0].mUserID;
		const BpHandle object1 = objects[index1].mUserID;

		if(groups[object0] == groups[object1])
			return NULL;
	}*/

	// Order the ids
	sort(id0, id1);

	const PxU32 fullHashValue = hash(id0, id1);
	PxU32 hashValue = fullHashValue & mMask;

	{
		MBP_Pair* PX_RESTRICT p = findPair(id0, id1, hashValue);
		if(p)
		{
			p->setUpdated();
			return p;	// Persistent pair
		}
	}

	// This is a new pair
	if(mNbActivePairs >= mHashSize)
		hashValue = growPairs(fullHashValue);

	const PxU32 index = mNbActivePairs++;
	MBP_Pair* PX_RESTRICT p = &mActivePairs[index];
	p->setNewPair(id0, id1);
	mNext[index] = mHashTable[hashValue];
	mHashTable[hashValue] = index;
	return p;
}

///////////////////////////////////////////////////////////////////////////////

void MBP_PairManager::removePair(PxU32 /*id0*/, PxU32 /*id1*/, PxU32 hashValue, PxU32 pairIndex)
{
	// Walk the hash table to fix mNext
	{
		PxU32 offset = mHashTable[hashValue];
		PX_ASSERT(offset!=INVALID_ID);

		PxU32 previous=INVALID_ID;
		while(offset!=pairIndex)
		{
			previous = offset;
			offset = mNext[offset];
		}

		// Let us go/jump us
		if(previous!=INVALID_ID)
		{
			PX_ASSERT(mNext[previous]==pairIndex);
			mNext[previous] = mNext[pairIndex];
		}
		// else we were the first
		else mHashTable[hashValue] = mNext[pairIndex];
		// we're now free to reuse mNext[pairIndex] without breaking the list
	}
#if PX_DEBUG
	mNext[pairIndex]=INVALID_ID;
#endif
	// Invalidate entry

	// Fill holes
	{
		// 1) Remove last pair
		const PxU32 lastPairIndex = mNbActivePairs-1;
		if(lastPairIndex==pairIndex)
		{
			mNbActivePairs--;
		}
		else
		{
			const MBP_Pair* last = &mActivePairs[lastPairIndex];
			const PxU32 lastHashValue = hash(last->getId0(), last->getId1()) & mMask;

			// Walk the hash table to fix mNext
			PxU32 offset = mHashTable[lastHashValue];
			PX_ASSERT(offset!=INVALID_ID);

			PxU32 previous=INVALID_ID;
			while(offset!=lastPairIndex)
			{
				previous = offset;
				offset = mNext[offset];
			}

			// Let us go/jump us
			if(previous!=INVALID_ID)
			{
				PX_ASSERT(mNext[previous]==lastPairIndex);
				mNext[previous] = mNext[lastPairIndex];
			}
			// else we were the first
			else mHashTable[lastHashValue] = mNext[lastPairIndex];
			// we're now free to reuse mNext[lastPairIndex] without breaking the list

#if PX_DEBUG
			mNext[lastPairIndex]=INVALID_ID;
#endif

			// Don't invalidate entry since we're going to shrink the array

			// 2) Re-insert in free slot
			mActivePairs[pairIndex] = mActivePairs[lastPairIndex];
#if PX_DEBUG
			PX_ASSERT(mNext[pairIndex]==INVALID_ID);
#endif
			mNext[pairIndex] = mHashTable[lastHashValue];
			mHashTable[lastHashValue] = pairIndex;

			mNbActivePairs--;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

void MBP_PairManager::shrinkMemory()
{
	// Check correct memory against actually used memory
	const PxU32 correctHashSize = Ps::nextPowerOfTwo(mNbActivePairs);
	if(mHashSize==correctHashSize)
		return;

	if(mReservedMemory && correctHashSize < mReservedMemory)
		return;

	// Reduce memory used
	mHashSize = correctHashSize;
	mMask = mHashSize-1;

	reallocPairs();
}

///////////////////////////////////////////////////////////////////////////////

void MBP_PairManager::reallocPairs()
{
	MBP_FREE(mHashTable);
	mHashTable = reinterpret_cast<PxU32*>(MBP_ALLOC(mHashSize*sizeof(PxU32)));
	storeDwords(mHashTable, mHashSize, INVALID_ID);

	// Get some bytes for new entries
	MBP_Pair* newPairs	= reinterpret_cast<MBP_Pair*>(MBP_ALLOC(mHashSize * sizeof(MBP_Pair)));	PX_ASSERT(newPairs);
	PxU32* newNext		= reinterpret_cast<PxU32*>(MBP_ALLOC(mHashSize * sizeof(PxU32)));		PX_ASSERT(newNext);

	// Copy old data if needed
	if(mNbActivePairs)
		PxMemCopy(newPairs, mActivePairs, mNbActivePairs*sizeof(MBP_Pair));
	// ### check it's actually needed... probably only for pairs whose hash value was cut by the and
	// yeah, since hash(id0, id1) is a constant
	// However it might not be needed to recompute them => only less efficient but still ok
	for(PxU32 i=0;i<mNbActivePairs;i++)
	{
		const PxU32 hashValue = hash(mActivePairs[i].getId0(), mActivePairs[i].getId1()) & mMask;	// New hash value with new mask
		newNext[i] = mHashTable[hashValue];
		mHashTable[hashValue] = i;
	}

	// Delete old data
	MBP_FREE(mNext);
	MBP_FREE(mActivePairs);

	// Assign new pointer
	mActivePairs = newPairs;
	mNext = newNext;
}

///////////////////////////////////////////////////////////////////////////////

}
#endif

#ifdef USE_MBP_PAIR_MANAGER
	typedef MBP_PairManager	PairArray;
#else
	typedef Ps::Array<Pair>	PairArray;
#endif

static PX_FORCE_INLINE void outputPair(PairArray& pairs, ShapeHandle index0, ShapeHandle index1)
{
#ifdef USE_MBP_PAIR_MANAGER
	pairs.addPair(index0, index1);
#else
	if(index1<index0)
		Ps::swap(index0, index1);
	pairs.pushBack(Pair(index0, index1));
#endif
}

///

class PersistentPairs : public Ps::UserAllocated
{
	public:
									PersistentPairs() : mTimestamp(PX_INVALID_U32),
#ifndef USE_MBP_PAIR_MANAGER
										mCurrentPairArray(0),
#endif
										mShouldBeDeleted(false)					{}
	virtual							~PersistentPairs()							{}

	virtual			bool			update(SimpleAABBManager& /*manager*/, BpCacheData* /*data*/ = NULL)		{ return false; }

	PX_FORCE_INLINE	void			updatePairs(PxU32 timestamp, const PxBounds3* bounds, const float* contactDistances, const Bp::FilterGroup::Enum* groups,
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
												const bool* lut,
#endif
												Ps::Array<VolumeData>& volumeData, Ps::Array<AABBOverlap>* createdOverlaps, Ps::Array<AABBOverlap>* destroyedOverlaps);
					void			outputDeletedOverlaps(Ps::Array<AABBOverlap>* overlaps, const Ps::Array<VolumeData>& volumeData);
	private:
	virtual			void			findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT bounds, const float* PX_RESTRICT contactDistances, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
		)	= 0;
	protected:
					PxU32			mTimestamp;
#ifdef USE_MBP_PAIR_MANAGER
					MBP_PairManager	mPM;
#else
					PxU32			mCurrentPairArray;
					Ps::Array<Pair>	mCurrentPairs[2];
#endif
	public:
					bool			mShouldBeDeleted;
};

/////
#ifdef USE_SIMD_BOUNDS
#define SIMD_OVERLAP_INIT(box)	\
	const __m128i b = _mm_shuffle_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(&box.mMinY)), 78);
#define SIMD_OVERLAP_TEST(box)	\
	const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&box.mMinY));	\
	const __m128i d = _mm_cmpgt_epi32(a, b);											\
	if(_mm_movemask_epi8(d)==0x0000ff00)

static PX_FORCE_INLINE int intersects2D(const InflatedAABB& box0, const InflatedAABB& box1)
{
#if PX_INTEL_FAMILY
	// PT: TODO: move this out of the test
	const __m128i b = _mm_shuffle_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(&box0.mMinY)), 78);

	const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&box1.mMinY));
	const __m128i d = _mm_cmpgt_epi32(a, b);
	if(_mm_movemask_epi8(d)==0x0000ff00)
		return 1;
	return 0;
#else
	if(	(box1.mMaxY < box0.mMinY) || (box0.mMaxY < box1.mMinY)
	||	(box1.mMaxZ < box0.mMinZ) || (box0.mMaxZ < box1.mMinZ))
		return 0;
	return 1;
#endif
}
#else
static PX_FORCE_INLINE int intersects2D(const PxBounds3& a, const PxBounds3& b)
{
	if(	(b.maximum.y < a.minimum.y) || (a.maximum.y < b.minimum.y)
	||	(b.maximum.z < a.minimum.z) || (a.maximum.z < b.minimum.z))
		return 0;
	return 1;
}
#endif

static PX_FORCE_INLINE InflatedType getMinX(const InflatedAABB& box)
{
#ifdef USE_SIMD_BOUNDS
	return box.mMinX;
#else
	return box.minimum.x;
#endif
}

static PX_FORCE_INLINE InflatedType getMaxX(const InflatedAABB& box)
{
#ifdef USE_SIMD_BOUNDS
	return box.mMaxX;
#else
	return box.maximum.x;
#endif
}

static PX_FORCE_INLINE void testPair(PairArray& pairs, const InflatedAABB* bounds0, const InflatedAABB* bounds1, const Bp::FilterGroup::Enum* PX_RESTRICT groups,
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	const bool* PX_RESTRICT lut,
#endif
	const PxU32 aggIndex0, const PxU32 aggIndex1, const PxU32 index0, const PxU32 index1)
{
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	if(groupFiltering(groups[aggIndex0], groups[aggIndex1], lut))
#else
	if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
#endif
		if(intersects2D(bounds0[index0], bounds1[index1]))
			outputPair(pairs, aggIndex0, aggIndex1);
}

#ifdef REMOVED_FAILED_EXPERIMENT
static PX_NOINLINE void processCandidates(PxU32 nbCandidates, const Pair* PX_RESTRICT candidates,
										PairArray& pairs, const InflatedAABB* bounds0, const InflatedAABB* bounds1, const Bp::FilterGroup::Enum* PX_RESTRICT groups, const PxU32* aggIndices0, const PxU32* aggIndices1)
{
	while(nbCandidates--)
	{
		const PxU32 index0 = candidates->mID0;
		const PxU32 index1 = candidates->mID1;
		candidates++;

		const PxU32 aggIndex0 = aggIndices0[index0];
		const PxU32 aggIndex1 = aggIndices1[index1];

		testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
	}
}

static /*PX_FORCE_INLINE*/ void boxPruning(PairArray& pairs, const InflatedAABB* bounds0, const InflatedAABB* bounds1, const Bp::FilterGroup::Enum* PX_RESTRICT groups, PxU32 size0, PxU32 size1, const PxU32* aggIndices0, const PxU32* aggIndices1)
{
//	PxU32 nbCandidates = 0;
//	Pair candidates[16384];

	PxU32 index0 = 0;
	PxU32 runningAddress1 = 0;
	while(runningAddress1<size1 && index0<size0)
	{
		const PxU32 aggIndex0 = aggIndices0[index0];
		const InflatedAABB& box0 = bounds0[index0];

		const InflatedType minLimit = getMinX(box0);
		while(getMinX(bounds1[runningAddress1])<minLimit)
			runningAddress1++;

		PxU32 index1 = runningAddress1;
		const InflatedType maxLimit = getMaxX(box0);
		while(getMinX(bounds1[index1])<=maxLimit)
		{
			const PxU32 aggIndex1 = aggIndices1[index1];
			testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
			index1++;
//			if(nbCandidates==1024)
//				processCandidates(nbCandidates, candidates, pairs, bounds0, bounds1, groups, aggIndices0, aggIndices1);
//			candidates[nbCandidates++] = Pair(index0, index1);
//			index1++;
		}
		index0++;
	}
//	processCandidates(nbCandidates, candidates, pairs, bounds0, bounds1, groups, aggIndices0, aggIndices1);
}
#endif

#ifdef REMOVED
static /*PX_FORCE_INLINE*/ void boxPruning(PairArray& pairs, const InflatedAABB* bounds0, const InflatedAABB* bounds1, const Bp::FilterGroup::Enum* PX_RESTRICT groups, PxU32 size0, PxU32 size1, const PxU32* aggIndices0, const PxU32* aggIndices1)
{
	PxU32 index0 = 0;
	PxU32 runningAddress1 = 0;
	while(runningAddress1<size1 && index0<size0)
	{
		const PxU32 aggIndex0 = aggIndices0[index0];
		const InflatedAABB& box0 = bounds0[index0];
		index0++;

		const InflatedType minLimit = getMinX(box0);
		while(getMinX(bounds1[runningAddress1])<minLimit)
			runningAddress1++;

		PxU32 index1 = runningAddress1;
		const InflatedType maxLimit = getMaxX(box0);
		while(getMinX(bounds1[index1])<=maxLimit)
		{
			const PxU32 aggIndex1 = aggIndices1[index1];
//			testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
				if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
					if(intersects2D(box0, bounds1[index1]))
						outputPair(pairs, aggIndex0, aggIndex1);

			index1++;
		}
//		index0++;
	}
}
#endif

#ifdef STORE_SORTED_BOUNDS
static void boxPruning(	PairArray& pairs, const InflatedAABB* PX_RESTRICT bounds0, const InflatedAABB* PX_RESTRICT bounds1, const Bp::FilterGroup::Enum* PX_RESTRICT groups,
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
						const bool* PX_RESTRICT lut,
#endif
						PxU32 size0, PxU32 size1, const BoundsIndex* PX_RESTRICT aggIndices0, const BoundsIndex* PX_RESTRICT aggIndices1)
{
	{
		PxU32 index0 = 0;
		PxU32 runningAddress1 = 0;
		while(runningAddress1<size1 && index0<size0)
		{
			const PxU32 aggIndex0 = aggIndices0[index0];
			const InflatedAABB& box0 = bounds0[index0];
			index0++;
#ifdef USE_SIMD_BOUNDS
			SIMD_OVERLAP_INIT(box0)
#endif
			const InflatedType minLimit = getMinX(box0);
			while(getMinX(bounds1[runningAddress1])<minLimit)
				runningAddress1++;

			PxU32 index1 = runningAddress1;
			const InflatedType maxLimit = getMaxX(box0);
			while(getMinX(bounds1[index1])<=maxLimit)
			{
				const PxU32 aggIndex1 = aggIndices1[index1];
//				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
					if(groupFiltering(groups[aggIndex0], groups[aggIndex1], lut))
#else
					if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
#endif
					{
#ifdef USE_SIMD_BOUNDS
						SIMD_OVERLAP_TEST(bounds1[index1])
#else
						if(intersects2D(box0, bounds1[index1]))
#endif
						{
							outputPair(pairs, aggIndex0, aggIndex1);
						}
					}
				index1++;
			}
//			index0++;
		}
//		boxPruning(pairs, bounds0, bounds1, groups, size0, size1, mAggregate0->mAggregated.begin(), mAggregate1->mAggregated.begin());
	}

	{
		PxU32 index0 = 0;
		PxU32 runningAddress0 = 0;
		while(runningAddress0<size0 && index0<size1)
		{
			const PxU32 aggIndex0 = aggIndices1[index0];
			const InflatedAABB& box1 = bounds1[index0];
			index0++;
#ifdef USE_SIMD_BOUNDS
			SIMD_OVERLAP_INIT(box1)
#endif
			const InflatedType minLimit = getMinX(box1);
			while(getMinX(bounds0[runningAddress0])<=minLimit)
				runningAddress0++;

			PxU32 index1 = runningAddress0;
			const InflatedType maxLimit = getMaxX(box1);
			while(getMinX(bounds0[index1])<=maxLimit)
			{
				const PxU32 aggIndex1 = aggIndices0[index1];
//				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index1, index0);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
					if(groupFiltering(groups[aggIndex0], groups[aggIndex1], lut))
#else
					if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
#endif
					{
#ifdef USE_SIMD_BOUNDS
						SIMD_OVERLAP_TEST(bounds0[index1])
#else
						if(intersects2D(bounds0[index1], box1))
#endif
						{
							outputPair(pairs, aggIndex0, aggIndex1);
						}
					}
				index1++;
			}
//			index0++;
		}
//		boxPruning(pairs, bounds1, bounds0, groups, size1, size0, mAggregate1->mAggregated.begin(), mAggregate0->mAggregated.begin());
	}
}
#endif

/////

class PersistentActorAggregatePair : public PersistentPairs
{
	public:
								PersistentActorAggregatePair(Aggregate* aggregate, ShapeHandle actorHandle);
	virtual						~PersistentActorAggregatePair()	{}

	virtual			void		findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT bounds, const float* PX_RESTRICT contactDistances, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
		);
	virtual			bool		update(SimpleAABBManager& manager, BpCacheData* data);

					ShapeHandle	mAggregateHandle;
					ShapeHandle	mActorHandle;
					Aggregate*	mAggregate;
};

PersistentActorAggregatePair::PersistentActorAggregatePair(Aggregate* aggregate, ShapeHandle actorHandle) :
	mAggregateHandle	(aggregate->mIndex),
	mActorHandle		(actorHandle),
	mAggregate			(aggregate)
{
}

void PersistentActorAggregatePair::findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT bounds, const float* PX_RESTRICT contactDistances, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
	)
{
	const ShapeHandle actorHandle = mActorHandle;

	// PT: TODO: don't recompute inflated bounds all the time - worth it?
	const PxBounds3& actorSrcBounds = bounds[actorHandle];
	const PxVec3 offset(contactDistances[actorHandle]);
	const PxBounds3 actorInflatedBounds(actorSrcBounds.minimum - offset, actorSrcBounds.maximum + offset);

	InflatedAABB actorBounds[2];
#ifdef USE_SIMD_BOUNDS
	actorBounds[0].initFrom2(actorInflatedBounds);
#else
	actorBounds[0] = actorInflatedBounds;
#endif

	const PxU32 size = mAggregate->getNbAggregated();
	const InflatedAABB* inflatedBounds = mAggregate->mInflatedBounds;
	if(gUseBruteForce)
	{
		const Bp::FilterGroup::Enum actorGroup = groups[actorHandle];
		for(PxU32 i=0;i<size;i++)
		{
			const BoundsIndex index = mAggregate->getAggregated(i);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
			if(groupFiltering(actorGroup, groups[index], lut))
#else
			if(groupFiltering(actorGroup, groups[index]))
#endif
			{
				if(actorBounds[0].intersects(inflatedBounds[i]))
					outputPair(pairs, index, actorHandle);
			}
		}
	}
	else
	{
#ifdef USE_SIMD_BOUNDS
		actorBounds[1].mMinX = 0xffffffff;
#else
		actorBounds[1].minimum.x = PX_MAX_F32;
#endif

#ifdef STORE_SORTED_BOUNDS
		const InflatedAABB* PX_RESTRICT bounds0 = actorBounds;
		const InflatedAABB* PX_RESTRICT bounds1 = inflatedBounds;
		mAggregate->getSortedMinBounds();
	#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
		boxPruning(pairs, bounds0, bounds1, groups, lut, 1, size, &actorHandle, mAggregate->getIndices());
	#else
		boxPruning(pairs, bounds0, bounds1, groups, 1, size, &actorHandle, mAggregate->getIndices());
	#endif
#else
		const InflatedAABB* PX_RESTRICT bounds0 = actorBounds;
		const InflatedAABB* PX_RESTRICT bounds1 = inflatedBounds;
		const PxU32 size0 = 1;
		const PxU32 size1 = size;

		const PxU32 sortedIndices[2] = { 0, 1 };
		const PxU32* sorted0 = sortedIndices;
		const PxU32* sorted1 = mAggregate->getSortedMinBounds();

		const PxU32* const lastSorted0 = &sorted0[size0];
		const PxU32* const lastSorted1 = &sorted1[size1];
		const PxU32* runningAddress0 = sorted0;
		const PxU32* runningAddress1 = sorted1;

		while(runningAddress1<lastSorted1 && sorted0<lastSorted0)
		{
			const PxU32 index0 = *sorted0++;
//			const PxU32 aggIndex0 = mAggregate0->getAggregated(index0);
			const PxU32 aggIndex0 = actorHandle;

			const InflatedType minLimit = getMinX(bounds0[index0]);
			while(getMinX(bounds1[*runningAddress1])<minLimit)
				runningAddress1++;

			const PxU32* runningAddress2_1 = runningAddress1;
			const InflatedType maxLimit = getMaxX(bounds0[index0]);
			PxU32 index1;
			while(getMinX(bounds1[index1 = *runningAddress2_1++])<=maxLimit)
			{
				const PxU32 aggIndex1 = mAggregate->getAggregated(index1);
				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
			}
		}

		////

		while(runningAddress0<lastSorted0 && sorted1<lastSorted1)
		{
			const PxU32 index0 = *sorted1++;
			const PxU32 aggIndex0 = mAggregate->getAggregated(index0);

			const InflatedType minLimit = getMinX(bounds1[index0]);
			while(getMinX(bounds0[*runningAddress0])<=minLimit)
				runningAddress0++;

			const PxU32* runningAddress2_0 = runningAddress0;
			const InflatedType maxLimit = getMaxX(bounds1[index0]);
			PxU32 index1;
			while(getMinX(bounds0[index1 = *runningAddress2_0++])<=maxLimit)
			{
//				const PxU32 aggIndex1 = mAggregate0->getAggregated(index1);
				const PxU32 aggIndex1 = actorHandle;
				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index1, index0);
			}
		}
#endif
	}
}

bool PersistentActorAggregatePair::update(SimpleAABBManager& manager, BpCacheData* data)
{
	if(mShouldBeDeleted || shouldPairBeDeleted(manager.mGroups, mAggregateHandle, mActorHandle))
		return true;

	if(!mAggregate->getNbAggregated())	// PT: needed with lazy empty actors
		return true;

	if(mAggregate->isDirty() || manager.mChangedHandleMap.boundedTest(mActorHandle))
		manager.updatePairs(*this, data);

	return false;
}

/////

class PersistentAggregateAggregatePair : public PersistentPairs
{
	public:
								PersistentAggregateAggregatePair(Aggregate* aggregate0, Aggregate* aggregate1);
	virtual						~PersistentAggregateAggregatePair()	{}

	virtual			void		findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT bounds, const float* PX_RESTRICT contactDistances, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
		);
	virtual			bool		update(SimpleAABBManager& manager, BpCacheData*);

					ShapeHandle	mAggregateHandle0;
					ShapeHandle	mAggregateHandle1;
					Aggregate*	mAggregate0;
					Aggregate*	mAggregate1;
};

PersistentAggregateAggregatePair::PersistentAggregateAggregatePair(Aggregate* aggregate0, Aggregate* aggregate1) :
	mAggregateHandle0	(aggregate0->mIndex),
	mAggregateHandle1	(aggregate1->mIndex),
	mAggregate0			(aggregate0),
	mAggregate1			(aggregate1)
{
}

void PersistentAggregateAggregatePair::findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT /*bounds*/, const float* PX_RESTRICT /*contactDistances*/, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
	)
{
	const PxU32 size0 = mAggregate0->getNbAggregated();
	const PxU32 size1 = mAggregate1->getNbAggregated();
	if(gUseBruteForce)
	{
		const InflatedAABB* inflated0 = mAggregate0->mInflatedBounds;
		const InflatedAABB* inflated1 = mAggregate1->mInflatedBounds;

		// Brute-force nb0*nb1 overlap tests
		for(PxU32 i=0;i<size0;i++)
		{
			const BoundsIndex index0 = mAggregate0->getAggregated(i);

			const InflatedAABB& inflatedBounds0 = inflated0[i];

			for(PxU32 j=0;j<size1;j++)
			{
				const BoundsIndex index1 = mAggregate1->getAggregated(j);

#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
				if(groupFiltering(groups[index0], groups[index1], lut))
#else
				if(groupFiltering(groups[index0], groups[index1]))
#endif
				{
					if(inflatedBounds0.intersects(inflated1[j]))
						outputPair(pairs, index0, index1);
				}
			}
		}
	}
	else
	{
#ifdef STORE_SORTED_BOUNDS
		mAggregate0->getSortedMinBounds();
		mAggregate1->getSortedMinBounds();
		const InflatedAABB* PX_RESTRICT bounds0 = mAggregate0->mInflatedBounds;
		const InflatedAABB* PX_RESTRICT bounds1 = mAggregate1->mInflatedBounds;
	#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
		boxPruning(pairs, bounds0, bounds1, groups, lut, size0, size1, mAggregate0->getIndices(), mAggregate1->getIndices());
	#else
		boxPruning(pairs, bounds0, bounds1, groups, size0, size1, mAggregate0->getIndices(), mAggregate1->getIndices());
	#endif
#else
		const InflatedAABB* PX_RESTRICT bounds0 = mAggregate0->mInflatedBounds;
		const InflatedAABB* PX_RESTRICT bounds1 = mAggregate1->mInflatedBounds;

		const PxU32* sorted0 = mAggregate0->getSortedMinBounds();
		const PxU32* sorted1 = mAggregate1->getSortedMinBounds();

		const PxU32* const lastSorted0 = &sorted0[size0];
		const PxU32* const lastSorted1 = &sorted1[size1];
		const PxU32* runningAddress0 = sorted0;
		const PxU32* runningAddress1 = sorted1;

		while(runningAddress1<lastSorted1 && sorted0<lastSorted0)
		{
			const PxU32 index0 = *sorted0++;
			const PxU32 aggIndex0 = mAggregate0->getAggregated(index0);

//			const __m128i b = _mm_shuffle_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(&bounds0[index0].mMinY)), 78);

			const InflatedType minLimit = getMinX(bounds0[index0]);
			while(getMinX(bounds1[*runningAddress1])<minLimit)
				runningAddress1++;

			const PxU32* runningAddress2_1 = runningAddress1;
			const InflatedType maxLimit = getMaxX(bounds0[index0]);
			PxU32 index1;
			while(getMinX(bounds1[index1 = *runningAddress2_1++])<=maxLimit)
			{
				const PxU32 aggIndex1 = mAggregate1->getAggregated(index1);
				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index0, index1);
/*					if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
					{
						const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&bounds1[index1].mMinY));
						const __m128i d = _mm_cmpgt_epi32(a, b);
						if(_mm_movemask_epi8(d)==0x0000ff00)
//						if(intersects2D(bounds0[index0], bounds1[index1]))
						{
							outputPair(pairs, aggIndex0, aggIndex1);
						}
					}*/
			}
		}

		////

		while(runningAddress0<lastSorted0 && sorted1<lastSorted1)
		{
			const PxU32 index0 = *sorted1++;
			const PxU32 aggIndex0 = mAggregate1->getAggregated(index0);

//			const __m128i b = _mm_shuffle_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(&bounds1[index0].mMinY)), 78);

			const InflatedType minLimit = getMinX(bounds1[index0]);
			while(getMinX(bounds0[*runningAddress0])<=minLimit)
				runningAddress0++;

			const PxU32* runningAddress2_0 = runningAddress0;
			const InflatedType maxLimit = getMaxX(bounds1[index0]);
			PxU32 index1;
			while(getMinX(bounds0[index1 = *runningAddress2_0++])<=maxLimit)
			{
				const PxU32 aggIndex1 = mAggregate0->getAggregated(index1);
				testPair(pairs, bounds0, bounds1, groups, aggIndex0, aggIndex1, index1, index0);
/*					if(groupFiltering(groups[aggIndex0], groups[aggIndex1]))
					{
						const __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&bounds0[index1].mMinY));
						const __m128i d = _mm_cmpgt_epi32(a, b);
						if(_mm_movemask_epi8(d)==0x0000ff00)
//						if(intersects2D(bounds0[index1], bounds1[index0]))
						{
							outputPair(pairs, aggIndex0, aggIndex1);
						}
					}*/
			}
		}
#endif
	}
}

bool PersistentAggregateAggregatePair::update(SimpleAABBManager& manager, BpCacheData* data)
{
	if(mShouldBeDeleted || shouldPairBeDeleted(manager.mGroups, mAggregateHandle0, mAggregateHandle1))
		return true;

	if(!mAggregate0->getNbAggregated() || !mAggregate1->getNbAggregated())	// PT: needed with lazy empty actors
		return true;

	if(mAggregate0->isDirty() || mAggregate1->isDirty())
		manager.updatePairs(*this, data);

	return false;
}

/////

class PersistentSelfCollisionPairs : public PersistentPairs
{
	public:
						PersistentSelfCollisionPairs(Aggregate* aggregate);
	virtual				~PersistentSelfCollisionPairs()	{}

	virtual	void		findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT bounds, const float* PX_RESTRICT contactDistances, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
		);

			Aggregate*	mAggregate;
};

PersistentSelfCollisionPairs::PersistentSelfCollisionPairs(Aggregate* aggregate) :
	mAggregate	(aggregate)
{
}

void PersistentSelfCollisionPairs::findOverlaps(PairArray& pairs, const PxBounds3* PX_RESTRICT/*bounds*/, const float* PX_RESTRICT/*contactDistances*/, const Bp::FilterGroup::Enum* PX_RESTRICT groups
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	, const bool* PX_RESTRICT lut
#endif
	)
{
	const PxU32 size0 = mAggregate->getNbAggregated();

	if(gUseBruteForce)
	{
		const InflatedAABB* inflatedBounds = mAggregate->mInflatedBounds;

		// Brute-force n(n-1)/2 overlap tests
		for(PxU32 i=0;i<size0;i++)
		{
			const BoundsIndex index0 = mAggregate->getAggregated(i);

			const InflatedAABB& inflatedBounds0 = inflatedBounds[i];

			for(PxU32 j=i+1;j<size0;j++)
			{
				const BoundsIndex index1 = mAggregate->getAggregated(j);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
				if(groupFiltering(groups[index0], groups[index1], lut))
#else
				if(groupFiltering(groups[index0], groups[index1]))
#endif
				{
					if(inflatedBounds0.intersects(inflatedBounds[j]))
						outputPair(pairs, index0, index1);
				}
			}
		}
	}
	else
	{
#ifdef STORE_SORTED_BOUNDS
		mAggregate->getSortedMinBounds();
		const InflatedAABB* PX_RESTRICT bounds = mAggregate->mInflatedBounds;

		PxU32 index0 = 0;
		PxU32 runningAddress = 0;
		while(runningAddress<size0 && index0<size0)
		{
			const PxU32 aggIndex0 = mAggregate->getAggregated(index0);
			const InflatedAABB& box0 = bounds[index0];

			const InflatedType minLimit = getMinX(box0);
			while(getMinX(bounds[runningAddress++])<minLimit);

			PxU32 index1 = runningAddress;
			const InflatedType maxLimit = getMaxX(box0);
			while(getMinX(bounds[index1])<=maxLimit)
			{
//				if(index0!=index1)
				{
					const PxU32 aggIndex1 = mAggregate->getAggregated(index1);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
					testPair(pairs, bounds, bounds, groups, lut, aggIndex0, aggIndex1, index0, index1);
#else
					testPair(pairs, bounds, bounds, groups, aggIndex0, aggIndex1, index0, index1);
#endif
				}
				index1++;
			}
			index0++;
		}
#else
		const InflatedAABB* bounds = mAggregate->mInflatedBounds;
		const PxU32* sorted = mAggregate->getSortedMinBounds();

		const PxU32* const lastSorted = &sorted[size0];
		const PxU32* runningAddress = sorted;
		while(runningAddress<lastSorted && sorted<lastSorted)
		{
			const PxU32 index0 = *sorted++;
			const PxU32 aggIndex0 = mAggregate->getAggregated(index0);

			const InflatedType minLimit = getMinX(bounds[index0]);
			while(getMinX(bounds[*runningAddress++])<minLimit);

			const PxU32* runningAddress2 = runningAddress;
			const InflatedType maxLimit = getMaxX(bounds[index0]);
			PxU32 index1;
			while(getMinX(bounds[index1 = *runningAddress2++])<=maxLimit)
			{
//				if(index0!=index1)
				{
					const PxU32 aggIndex1 = mAggregate->getAggregated(index1);
					testPair(pairs, bounds, bounds, groups, aggIndex0, aggIndex1, index0, index1);
				}
			}
		}
#endif
	}
}

/////

Aggregate::Aggregate(BoundsIndex index, bool selfCollisions) :
	mIndex			(index),
	mInflatedBounds	(NULL),
	mAllocatedSize	(0),
	mDirtySort		(false)
{
	resetDirtyState();
	mSelfCollisionPairs = selfCollisions ? PX_NEW(PersistentSelfCollisionPairs)(this) : NULL;
}

Aggregate::~Aggregate()
{
	PX_FREE_AND_RESET(mInflatedBounds);

	if(mSelfCollisionPairs)
		PX_DELETE_AND_RESET(mSelfCollisionPairs);
}

void Aggregate::sortBounds()
{
	mDirtySort = false;
	const PxU32 nbObjects = getNbAggregated();

//	if(nbObjects>128)
	{
		PX_ALLOCA(minPosBounds, InflatedType, nbObjects+1);
		bool alreadySorted =  true;
		InflatedType previousB = getMinX(mInflatedBounds[0]);
		minPosBounds[0] = previousB;
		for(PxU32 i=1;i<nbObjects;i++)
		{
			const InflatedType minB = getMinX(mInflatedBounds[i]);
			if(minB<previousB)
				alreadySorted = false;
			previousB = minB;
			minPosBounds[i] = minB;
		}
		if(alreadySorted)
			return;

		{
#ifdef STORE_SORTED_BOUNDS
		Cm::RadixSortBuffered mRS;
#endif

#ifdef USE_SIMD_BOUNDS
		minPosBounds[nbObjects] = 0xffffffff;
		mRS.Sort(minPosBounds, nbObjects+1, /*RadixHint::*/RADIX_UNSIGNED);
#else
		minPosBounds[nbObjects] = PX_MAX_F32;
		mRS.Sort(minPosBounds, nbObjects+1);
#endif

#ifdef STORE_SORTED_BOUNDS
		if(1)
		{
			Ps::Array<PxU32> copy = mAggregated;
			InflatedAABB* boundsCopy = reinterpret_cast<InflatedAABB*>(PX_ALLOC(sizeof(InflatedAABB)*(nbObjects+1), "mInflatedBounds"));
			PxMemCopy(boundsCopy, mInflatedBounds, (nbObjects+1)*sizeof(InflatedAABB));

			const PxU32* Sorted = mRS.GetRanks();
			for(PxU32 i=0;i<nbObjects;i++)
			{
				const PxU32 sortedIndex = Sorted[i];
				mAggregated[i] = copy[sortedIndex];
				mInflatedBounds[i] = boundsCopy[sortedIndex];
			}
			PX_FREE(boundsCopy);
		}
#endif
		}
	}
/*	else
	{
		struct Keys
		{
			InflatedType	mMinX;
			PxU32			mIndex;
			PX_FORCE_INLINE bool operator < (const Keys& data) const
			{
				return mMinX < data.mMinX;
			}		
		};

		PX_ALLOCA(minPosBounds, Keys, nbObjects+1);
		for(PxU32 i=0;i<nbObjects;i++)
		{
			minPosBounds[i].mMinX = getMinX(mInflatedBounds[i]);
			minPosBounds[i].mIndex = i;
		}

#ifdef USE_SIMD_BOUNDS
		minPosBounds[nbObjects].mMinX = 0xffffffff;
		minPosBounds[nbObjects].mIndex = nbObjects;
//		mRS.Sort(minPosBounds, nbObjects+1, Cm::RadixHint::RADIX_UNSIGNED);
		Ps::sort<Keys>(minPosBounds, nbObjects+1);
#else
		minPosBounds[nbObjects].mMinX = PX_MAX_F32;
		minPosBounds[nbObjects].mIndex = nbObjects;
		mRS.Sort(minPosBounds, nbObjects+1);
#endif
	}*/
}

#if PX_INTEL_FAMILY && !defined(PX_SIMD_DISABLED)
	#define SSE_CONST4(name, val) static const PX_ALIGN(16, PxU32 name[4]) = { (val), (val), (val), (val) } 
	#define SSE_CONST(name) *(const __m128i *)&name
	#define SSE_CONSTF(name) *(const __m128 *)&name

SSE_CONST4(gMaskAll, 0xffffffff);
SSE_CONST4(gMaskSign, PX_SIGN_BITMASK);

static PX_FORCE_INLINE __m128 encode(const Vec4V src)
{
	const __m128i MinV = _mm_castps_si128(src);
#if PX_PS4 || PX_OSX
	// PT: this doesn't compile on PC
	const __m128i BranchAValueV = _mm_andnot_si128(MinV, _mm_load_ps(reinterpret_cast<const float*>(&gMaskAll)));
	const __m128i BranchBValueV = _mm_or_si128(MinV, _mm_load_ps(reinterpret_cast<const float*>(&gMaskSign)));
#else
	// PT: this doesn't compile on PS4
	const __m128i BranchAValueV = _mm_andnot_si128(MinV, SSE_CONST(gMaskAll));
	const __m128i BranchBValueV = _mm_or_si128(MinV, SSE_CONST(gMaskSign));
#endif
	const __m128i SelMaskV = _mm_srai_epi32(MinV, 31);
	__m128i EncodedV = _mm_or_si128(_mm_andnot_si128(SelMaskV, BranchBValueV), _mm_and_si128(SelMaskV, BranchAValueV));
	EncodedV = _mm_srli_epi32(EncodedV, 1);
	return _mm_castsi128_ps(EncodedV);
}
#endif

#ifdef USE_SIMD_BOUNDS
static PX_FORCE_INLINE	void encodeBounds(SIMD_AABB* PX_RESTRICT dst, Vec4V minV, Vec4V maxV)
{
#if PX_INTEL_FAMILY
	const Vec4V EncodedMinV = encode(minV);
	PX_ALIGN(16, PxVec4) TmpMin4;
	V4StoreA(EncodedMinV, &TmpMin4.x);

	const Vec4V EncodedMaxV = encode(maxV);
	PX_ALIGN(16, PxVec4) TmpMax4;
	V4StoreA(EncodedMaxV, &TmpMax4.x);

	const PxU32* BinaryMin = reinterpret_cast<const PxU32*>(&TmpMin4.x);
	dst->mMinX = BinaryMin[0];
	dst->mMinY = BinaryMin[1];
	dst->mMinZ = BinaryMin[2];

	const PxU32* BinaryMax = reinterpret_cast<const PxU32*>(&TmpMax4.x);
	dst->mMaxX = BinaryMax[0];
	dst->mMaxY = BinaryMax[1];
	dst->mMaxZ = BinaryMax[2];
#else
	PxBounds3 tmp;
	StoreBounds(tmp, minV, maxV);
	dst->initFrom2(tmp);
#endif
}
#endif //USE_SIMD_BOUNDS

void Aggregate::allocateBounds()
{
	const PxU32 size = getNbAggregated();
	if(size!=mAllocatedSize)
	{
		mAllocatedSize = size;
		PX_FREE(mInflatedBounds);
		mInflatedBounds = reinterpret_cast<InflatedAABB*>(PX_ALLOC(sizeof(InflatedAABB)*(size+1), "mInflatedBounds"));
	}
}

void Aggregate::computeBounds(const BoundsArray& boundsArray, const float* contactDistances) /*PX_RESTRICT*/
{
//	PX_PROFILE_ZONE("Aggregate::computeBounds",0);

	const PxU32 size = getNbAggregated();
	PX_ASSERT(size);

	// PT: TODO: delay the conversion to integers until we sort (i.e. really need) the aggregated bounds?

	const PxU32 lookAhead = 4;
	const PxBounds3* PX_RESTRICT bounds = boundsArray.begin();
	Vec4V minimumV;
	Vec4V maximumV;
	{
		const BoundsIndex index0 = getAggregated(0);
		const PxU32 last = PxMin(lookAhead, size-1);
		for(PxU32 i=1;i<=last;i++)
		{
			const BoundsIndex index = getAggregated(i);
			Ps::prefetchLine(bounds + index, 0);
			Ps::prefetchLine(contactDistances + index, 0);
		}
		const PxBounds3& b = bounds[index0];
		const Vec4V offsetV = V4Load(contactDistances[index0]);
		minimumV = V4Sub(V4LoadU(&b.minimum.x), offsetV);
		maximumV = V4Add(V4LoadU(&b.maximum.x), offsetV);
#ifdef USE_SIMD_BOUNDS
		encodeBounds(&mInflatedBounds[0], minimumV, maximumV);
#else
		StoreBounds(mInflatedBounds[0], minimumV, maximumV);
#endif
	}

	for(PxU32 i=1;i<size;i++)
	{
		const BoundsIndex index = getAggregated(i);
		if(i+lookAhead<size)
		{
			const BoundsIndex nextIndex = getAggregated(i+lookAhead);
			Ps::prefetchLine(bounds + nextIndex, 0);
			Ps::prefetchLine(contactDistances + nextIndex, 0);
		}
		const PxBounds3& b = bounds[index];
		const Vec4V offsetV = V4Load(contactDistances[index]);
		const Vec4V aggregatedBoundsMinV = V4Sub(V4LoadU(&b.minimum.x), offsetV);
		const Vec4V aggregatedBoundsMaxV = V4Add(V4LoadU(&b.maximum.x), offsetV);
		minimumV = V4Min(minimumV, aggregatedBoundsMinV);
		maximumV = V4Max(maximumV, aggregatedBoundsMaxV);
#ifdef USE_SIMD_BOUNDS
		encodeBounds(&mInflatedBounds[i], aggregatedBoundsMinV, aggregatedBoundsMaxV);
#else
		StoreBounds(mInflatedBounds[i], aggregatedBoundsMinV, aggregatedBoundsMaxV);
#endif
	}

	StoreBounds(mBounds, minimumV, maximumV);
//	StoreBounds(boundsArray.begin()[mIndex], minimumV, maximumV);
//	boundsArray.setChangedState();

/*	if(0)
	{
		const PxBounds3& previousBounds = boundsArray.getBounds(mIndex);
		if(previousBounds.minimum==aggregateBounds.minimum && previousBounds.maximum==aggregateBounds.maximum)
		{
			// PT: same bounds as before
			printf("SAME BOUNDS\n");
		}
	}*/
#ifdef USE_SIMD_BOUNDS
	mInflatedBounds[size].mMinX = 0xffffffff;
#else
	mInflatedBounds[size].minimum.x = PX_MAX_F32;
#endif
	mDirtySort = true;
}

/////

void SimpleAABBManager::reserveShapeSpace(PxU32 nbTotalBounds)
{
	nbTotalBounds = Ps::nextPowerOfTwo(nbTotalBounds);
	mGroups.resize(nbTotalBounds, Bp::FilterGroup::eINVALID);
	mVolumeData.resize(nbTotalBounds);					//KS - must be initialized so that userData is NULL for SQ-only shapes
	mContactDistance.resizeUninitialized(nbTotalBounds);
	mAddedHandleMap.resize(nbTotalBounds);
	mRemovedHandleMap.resize(nbTotalBounds);
}

#if PX_VC
#pragma warning(disable: 4355 )	// "this" used in base member initializer list
#endif

SimpleAABBManager::SimpleAABBManager(	BroadPhase& bp, BoundsArray& boundsArray, Ps::Array<PxReal, Ps::VirtualAllocator>& contactDistance,
										PxU32 maxNbAggregates, PxU32 maxNbShapes, Ps::VirtualAllocator& allocator, PxU64 contextID,
										PxPairFilteringMode::Enum kineKineFilteringMode, PxPairFilteringMode::Enum staticKineFilteringMode) :
	mPostBroadPhase2			(contextID, *this),
	mPostBroadPhase3			(contextID, this, "SimpleAABBManager::postBroadPhaseStage3"),
	mFinalizeUpdateTask			(contextID),
	mChangedHandleMap			(allocator),
	mGroups						(allocator),
	mContactDistance			(contactDistance),
	mVolumeData					(PX_DEBUG_EXP("SimpleAABBManager::mVolumeData")),
	mAddedHandles				(allocator),
	mUpdatedHandles				(allocator),
	mRemovedHandles				(allocator),
	mBroadPhase					(bp),
	mBoundsArray				(boundsArray),
	mOutOfBoundsObjects		(PX_DEBUG_EXP("SimpleAABBManager::mOutOfBoundsObjects")),
	mOutOfBoundsAggregates	(PX_DEBUG_EXP("SimpleAABBManager::mOutOfBoundsAggregates")),
	//mCreatedOverlaps			{ Ps::Array<Bp::AABBOverlap>(PX_DEBUG_EXP("SimpleAABBManager::mCreatedOverlaps")) },
	//mDestroyedOverlaps			{ Ps::Array<Bp::AABBOverlap>(PX_DEBUG_EXP("SimpleAABBManager::mDestroyedOverlaps")) },
	mScratchAllocator			(NULL),
	mNarrowPhaseUnblockTask		(NULL),
	mUsedSize					(0),
	mOriginShifted				(false),
	mPersistentStateChanged		(true),
	mNbAggregates				(0),
	mFirstFreeAggregate			(PX_INVALID_U32),
	mTimestamp					(0),
#ifdef BP_USE_AGGREGATE_GROUP_TAIL
	mAggregateGroupTide			(PxU32(Bp::FilterGroup::eAGGREGATE_BASE)),
#endif
	mContextID					(contextID)
{
	PX_UNUSED(maxNbAggregates);	// PT: TODO: use it or remove it
	reserveShapeSpace(PxMax(maxNbShapes, 1u));

//	mCreatedOverlaps.reserve(16000);
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	{
		const bool discardKineKine = kineKineFilteringMode==PxPairFilteringMode::eKILL ? true : false;
		const bool discardStaticKine = staticKineFilteringMode==PxPairFilteringMode::eKILL ? true : false;

		for(int j=0;j<Bp::FilterType::COUNT;j++)
			for(int i=0;i<Bp::FilterType::COUNT;i++)
				mLUT[j][i] = false;
		mLUT[Bp::FilterType::STATIC][Bp::FilterType::DYNAMIC] = mLUT[Bp::FilterType::DYNAMIC][Bp::FilterType::STATIC] = true;
		mLUT[Bp::FilterType::STATIC][Bp::FilterType::KINEMATIC] = mLUT[Bp::FilterType::KINEMATIC][Bp::FilterType::STATIC] = !discardStaticKine;
		mLUT[Bp::FilterType::DYNAMIC][Bp::FilterType::KINEMATIC] = mLUT[Bp::FilterType::KINEMATIC][Bp::FilterType::DYNAMIC] = true;
		mLUT[Bp::FilterType::DYNAMIC][Bp::FilterType::DYNAMIC] = true;
		mLUT[Bp::FilterType::KINEMATIC][Bp::FilterType::KINEMATIC] = !discardKineKine;

		mLUT[Bp::FilterType::STATIC][Bp::FilterType::AGGREGATE] = mLUT[Bp::FilterType::AGGREGATE][Bp::FilterType::STATIC] = true;
		mLUT[Bp::FilterType::KINEMATIC][Bp::FilterType::AGGREGATE] = mLUT[Bp::FilterType::AGGREGATE][Bp::FilterType::KINEMATIC] = true;
		mLUT[Bp::FilterType::DYNAMIC][Bp::FilterType::AGGREGATE] = mLUT[Bp::FilterType::AGGREGATE][Bp::FilterType::DYNAMIC] = true;
		mLUT[Bp::FilterType::AGGREGATE][Bp::FilterType::AGGREGATE] = true;
	}
#else
	PX_UNUSED(kineKineFilteringMode);
	PX_UNUSED(staticKineFilteringMode);
#endif
}

static void releasePairs(AggPairMap& map)
{
	for(AggPairMap::Iterator iter = map.getIterator(); !iter.done(); ++iter)
		PX_DELETE(iter->second);
}

void SimpleAABBManager::destroy()
{
	releasePairs(mActorAggregatePairs);
	releasePairs(mAggregateAggregatePairs);

	const PxU32 nb = mAggregates.size();
	for(PxU32 i=0;i<nb;i++)
	{
		bool found = false;
		PxU32 currentFree = mFirstFreeAggregate;
		while(currentFree!=PX_INVALID_U32)
		{
			if(currentFree==i)
			{
				found = true;
				break;
			}
			currentFree = PxU32(reinterpret_cast<size_t>(mAggregates[currentFree]));
		}

		if(!found)
		{
			Aggregate* a = mAggregates[i];
			PX_DELETE(a);
		}
	}

	BpCacheData* entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
	while (entry)
	{
		entry->~BpCacheData();
		PX_FREE(entry);
		entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
	}

	PX_DELETE(this);
}

/*bool SimpleAABBManager::checkID(ShapeHandle id)
{
	for(AggPairMap::Iterator iter = mActorAggregatePairs.getIterator(); !iter.done(); ++iter)
	{
		PersistentActorAggregatePair* p = static_cast<PersistentActorAggregatePair*>(iter->second);
		if(p->mActorHandle==id || p->mAggregateHandle==id)
			return false;
	}

	for(AggPairMap::Iterator iter = mAggregateAggregatePairs.getIterator(); !iter.done(); ++iter)
	{
		PersistentAggregateAggregatePair* p = static_cast<PersistentAggregateAggregatePair*>(iter->second);
		if(p->mAggregateHandle0==id || p->mAggregateHandle1==id)
			return false;
	}
	return true;
}*/

static void removeAggregateFromDirtyArray(Aggregate* aggregate, Ps::Array<Aggregate*>& dirtyAggregates)
{
	// PT: TODO: do this lazily like for interactions?
	if(aggregate->isDirty())
	{
		const PxU32 dirtyIndex = aggregate->mDirtyIndex;
		PX_ASSERT(dirtyAggregates[dirtyIndex]==aggregate);
		dirtyAggregates.replaceWithLast(dirtyIndex);
		if(dirtyIndex<dirtyAggregates.size())
			dirtyAggregates[dirtyIndex]->mDirtyIndex = dirtyIndex;
		aggregate->resetDirtyState();
	}
	else
	{
		PX_ASSERT(!dirtyAggregates.findAndReplaceWithLast(aggregate));
	}
}

void SimpleAABBManager::reserveSpaceForBounds(BoundsIndex index)
{
	if ((index+1) >= mVolumeData.size())
		reserveShapeSpace(index+1);

	resetEntry(index); //KS - make sure this entry is flagged as invalid
}

// PT: TODO: what is the "userData" here?
bool SimpleAABBManager::addBounds(BoundsIndex index, PxReal contactDistance, Bp::FilterGroup::Enum group, void* userData, AggregateHandle aggregateHandle, PxU8 volumeType)
{
//	PX_ASSERT(checkID(index));

	initEntry(index, contactDistance, group, userData);
	mVolumeData[index].setVolumeType(volumeType);

	if(aggregateHandle==PX_INVALID_U32)
	{
		mVolumeData[index].setSingleActor();

		addBPEntry(index);

		mPersistentStateChanged = true;
	}
	else
	{
#if PX_CHECKED
		if(aggregateHandle>=mAggregates.size())
		{
			Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "AABBManager::addBounds - aggregateId out of bounds\n");
			return false;
		}

/*		{
			PxU32 firstFreeAggregate = mFirstFreeAggregate;
			while(firstFreeAggregate!=PX_INVALID_U32)
			{
				if(firstFreeAggregate==aggregateHandle)
				{
					Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "AABBManager::destroyAggregate - aggregate has already been removed\n");
					return BP_INVALID_BP_HANDLE;
				}
				firstFreeAggregate = PxU32(reinterpret_cast<size_t>(mAggregates[firstFreeAggregate]));
			}
		}*/
#endif
		mVolumeData[index].setAggregated(aggregateHandle);

		mPersistentStateChanged = true;		// PT: TODO: do we need this here?

		Aggregate* aggregate = getAggregateFromHandle(aggregateHandle);

		if(aggregate->getNbAggregated()<BP_MAX_AGGREGATE_BOUND_SIZE)
		{
			// PT: schedule the aggregate for BP insertion here, if we just added its first shape
			if(!aggregate->getNbAggregated())
				addBPEntry(aggregate->mIndex);

			aggregate->addAggregated(index);

			// PT: new actor added to aggregate => mark dirty to recompute bounds later
			aggregate->markAsDirty(mDirtyAggregates);
		}
#if PX_CHECKED
		else
		{
			// PT: TODO: change "AABBManager::createVolume", revisit limit & what happens to these shapes
			PX_CHECK_AND_RETURN_VAL(false, "AABBManager::createVolume - An aggregate has exceeded the limit of 128 PxShapes, not all shapes of the aggregate will be added to the broapdhase \n", true);
		}
#endif
	}

	// PT: TODO: remove or use this return value. Currently useless since always true. Gives birth to unreachable code in callers.
	return true;
}

void SimpleAABBManager::removeBounds(BoundsIndex index)
{
	// PT: TODO: shouldn't it be compared to mUsedSize?
	PX_ASSERT(index < mVolumeData.size());

	if(mVolumeData[index].isSingleActor())
	{
		removeBPEntry(index);

		mPersistentStateChanged = true;
	}
	else
	{
		PX_ASSERT(mVolumeData[index].isAggregated());

		const AggregateHandle aggregateHandle = mVolumeData[index].getAggregateOwner();
		Aggregate* aggregate = getAggregateFromHandle(aggregateHandle);
		bool status = aggregate->removeAggregated(index);
		(void)status;
//		PX_ASSERT(status);	// PT: can be false when >128 shapes

		// PT: remove empty aggregates, otherwise the BP will crash with empty bounds
		if(!aggregate->getNbAggregated())
		{
			removeBPEntry(aggregate->mIndex);
			removeAggregateFromDirtyArray(aggregate, mDirtyAggregates);
		}
		else
			aggregate->markAsDirty(mDirtyAggregates);	// PT: actor removed from aggregate => mark dirty to recompute bounds later

		mPersistentStateChanged = true;	// PT: TODO: do we need this here?
	}

	resetEntry(index);
}

// PT: TODO: the userData is actually a PxAggregate pointer. Maybe we could expose/use that.
AggregateHandle SimpleAABBManager::createAggregate(BoundsIndex index, Bp::FilterGroup::Enum group, void* userData, const bool selfCollisions)
{
//	PX_ASSERT(checkID(index));

	Aggregate* aggregate = PX_NEW(Aggregate)(index, selfCollisions);

	AggregateHandle handle;
	if(mFirstFreeAggregate==PX_INVALID_U32)
	{
		handle = mAggregates.size();
		mAggregates.pushBack(aggregate);
	}
	else
	{
		handle = mFirstFreeAggregate;
		mFirstFreeAggregate = PxU32(reinterpret_cast<size_t>(mAggregates[mFirstFreeAggregate]));
		mAggregates[handle] = aggregate;
	}

#ifdef BP_USE_AGGREGATE_GROUP_TAIL
/*	#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
		PxU32 id = index;
		id<<=2;
		id|=FilterType::AGGREGATE;
		initEntry(index, 0.0f, Bp::FilterGroup::Enum(id), userData);
	#endif*/
	initEntry(index, 0.0f, getAggregateGroup(), userData);
	PX_UNUSED(group);
#else
	initEntry(index, 0.0f, group, userData);
#endif

	mVolumeData[index].setAggregate(handle);

	mBoundsArray.setBounds(PxBounds3::empty(), index);	// PT: no need to set mPersistentStateChanged since "setBounds" already does something similar

	mNbAggregates++;

	// PT: we don't add empty aggregates to mAddedHandleMap yet, since they make the BP crash.
	return handle;
}

bool SimpleAABBManager::destroyAggregate(BoundsIndex& index_, Bp::FilterGroup::Enum& group_, AggregateHandle aggregateHandle)
{
#if PX_CHECKED
	if(aggregateHandle>=mAggregates.size())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "AABBManager::destroyAggregate - aggregateId out of bounds\n");
		return false;
	}

	{
		PxU32 firstFreeAggregate = mFirstFreeAggregate;
		while(firstFreeAggregate!=PX_INVALID_U32)
		{
			if(firstFreeAggregate==aggregateHandle)
			{
				Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "AABBManager::destroyAggregate - aggregate has already been removed\n");
				return false;
			}
			firstFreeAggregate = PxU32(reinterpret_cast<size_t>(mAggregates[firstFreeAggregate]));
		}
	}
#endif

	Aggregate* aggregate = getAggregateFromHandle(aggregateHandle);

#if PX_CHECKED
	if(aggregate->getNbAggregated())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, "AABBManager::destroyAggregate - aggregate still has bounds that needs removed\n");
		return false;
	}
#endif

	const BoundsIndex index = aggregate->mIndex;
	removeAggregateFromDirtyArray(aggregate, mDirtyAggregates);

	if(mAddedHandleMap.test(index))			// PT: if object had been added this frame...
		mAddedHandleMap.reset(index);		// PT: ...then simply revert the previous operation locally (it hasn't been passed to the BP yet).
	else if(aggregate->getNbAggregated())	// PT: else we need to remove it from the BP if it has been added there. If there's no aggregated
		mRemovedHandleMap.set(index);		// PT: shapes then the aggregate has never been added, or already removed.

	PX_DELETE(aggregate);
	mAggregates[aggregateHandle] = reinterpret_cast<Aggregate*>(size_t(mFirstFreeAggregate));
	mFirstFreeAggregate = aggregateHandle;

	// PT: TODO: shouldn't it be compared to mUsedSize?
	PX_ASSERT(index < mVolumeData.size());

	index_ = index;
	group_ = mGroups[index];

#ifdef BP_USE_AGGREGATE_GROUP_TAIL
	releaseAggregateGroup(mGroups[index]);
#endif
	resetEntry(index);

	mPersistentStateChanged = true;

	PX_ASSERT(mNbAggregates);
	mNbAggregates--;

	return true;
}

void SimpleAABBManager::handleOriginShift()
{
	mOriginShifted = false;
	mPersistentStateChanged = true;
	// PT: TODO: isn't the following loop potentially updating removed objects?
	// PT: TODO: check that aggregates code is correct here
	for(PxU32 i=0; i<mUsedSize; i++)
	{
		if(mGroups[i] == Bp::FilterGroup::eINVALID)
			continue;

		{
			if(mVolumeData[i].isSingleActor())
			{
				if(!mAddedHandleMap.test(i))
					mUpdatedHandles.pushBack(i);	// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
			}
			else if(mVolumeData[i].isAggregate())
			{
				const AggregateHandle aggregateHandle = mVolumeData[i].getAggregate();
				Aggregate* aggregate = getAggregateFromHandle(aggregateHandle);
				if(aggregate->getNbAggregated())
				{
					aggregate->markAsDirty(mDirtyAggregates);
					aggregate->allocateBounds();
					aggregate->computeBounds(mBoundsArray, mContactDistance.begin());
					mBoundsArray.begin()[aggregate->mIndex] = aggregate->mBounds;
					if(!mAddedHandleMap.test(i))
						mUpdatedHandles.pushBack(i);	// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
				}
			}
		}
	}
}

void AggregateBoundsComputationTask::runInternal()
{
	const BoundsArray& boundArray = mManager->getBoundsArray();
	const float* contactDistances = mManager->getContactDistances();

	PxU32 size = mNbToGo;
	Aggregate** currentAggregate = mAggregates + mStart;
	while(size--)
	{
		if(size)
		{
			Aggregate* nextAggregate = *(currentAggregate+1);
			Ps::prefetchLine(nextAggregate, 0);
			Ps::prefetchLine(nextAggregate, 64);
		}

		(*currentAggregate)->computeBounds(boundArray, contactDistances);
		currentAggregate++;
	}
}

void FinalizeUpdateTask::runInternal()
{
	mManager->finalizeUpdate(mNumCpuTasks, mScratchAllocator, getContinuation(), mNarrowPhaseUnlockTask);
}

void SimpleAABBManager::startAggregateBoundsComputationTasks(PxU32 nbToGo, PxU32 numCpuTasks, Cm::FlushPool& flushPool)
{
	const PxU32 nbAggregatesPerTask = nbToGo > numCpuTasks ? nbToGo / numCpuTasks : nbToGo;

	// PT: TODO: better load balancing

	PxU32 start = 0;
	while(nbToGo)
	{
		AggregateBoundsComputationTask* T = PX_PLACEMENT_NEW(flushPool.allocate(sizeof(AggregateBoundsComputationTask)), AggregateBoundsComputationTask(mContextID));

		const PxU32 nb = nbToGo < nbAggregatesPerTask ? nbToGo : nbAggregatesPerTask;
		T->Init(this, start, nb, mDirtyAggregates.begin());
		start += nb;
		nbToGo -= nb;

		T->setContinuation(&mFinalizeUpdateTask);
		T->removeReference();
	}
}

void SimpleAABBManager::updateAABBsAndBP(PxU32 numCpuTasks, Cm::FlushPool& flushPool, PxcScratchAllocator* scratchAllocator, bool hasContactDistanceUpdated, PxBaseTask* continuation, PxBaseTask* narrowPhaseUnlockTask)
{
	PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP", getContextId());

	mPersistentStateChanged = mPersistentStateChanged || hasContactDistanceUpdated;

	mScratchAllocator = scratchAllocator;
	mNarrowPhaseUnblockTask = narrowPhaseUnlockTask;

	const bool singleThreaded = gSingleThreaded || numCpuTasks<2;
	if(!singleThreaded)
	{
		PX_ASSERT(numCpuTasks);
		mFinalizeUpdateTask.Init(this, numCpuTasks, scratchAllocator, narrowPhaseUnlockTask);
		mFinalizeUpdateTask.setContinuation(continuation);
	}

	// Add
	{
		PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - add", getContextId());

		resetOrClear(mAddedHandles);

		const PxU32* bits = mAddedHandleMap.getWords();
		if(bits)
		{
			const PxU32 lastSetBit = mAddedHandleMap.findLast();
			for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
			{
				for(PxU32 b = bits[w]; b; b &= b-1)
				{
					const BoundsIndex handle = PxU32(w<<5|Ps::lowestSetBit(b));
					PX_ASSERT(!mVolumeData[handle].isAggregated());
					mAddedHandles.pushBack(handle);		// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
				}
			}
		}
	}

	// Update
	{
		PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - update", getContextId());

		resetOrClear(mUpdatedHandles);
		if(!mOriginShifted)
		{
			// PT: TODO:
			// - intercept calls marking aggregateD shapes dirty, in order to mark their aggregates dirty at the same time. That way we don't discover
			// them while parsing the map, i.e. the map is already fully complete when the parsing begins (no need to parse twice etc).
			// - once this is done, aggregateD shapes can be ignored during parsing (since we only needed them to go to their aggregates)
			// - we can then handle aggregates while parsing the map, i.e. there's no need for sorting anymore.
			// - there will be some thoughts to do about the dirty aggregates coming from the added map parsing: we still need to compute their bounds,
			// but we don't want to add these to mUpdatedHandles (since they're already in  mAddedHandles)
			// - we still need the set of dirty aggregates post broadphase, but we don't want to re-parse the full map for self-collisions. So we may still
			// need to put dirty aggregates in an array, but that might be simplified now
			// - the 'isDirty' checks to updatePairs can use the update map though - but the boundedTest is probably more expensive than the current test

			// PT: TODO: another idea: just output all aggregate handles by default then have a pass on mUpdatedHandles to remove them if that wasn't actually necessary
			// ...or just drop the artificial requirement for aggregates...

			{
				PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - update - bitmap iteration", getContextId());

				const PxU32* bits = mChangedHandleMap.getWords();
				if(bits)
				{
					const PxU32 lastSetBit = mChangedHandleMap.findLast();
					for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
					{
						for(PxU32 b = bits[w]; b; b &= b-1)
						{
							const BoundsIndex handle = PxU32(w<<5|Ps::lowestSetBit(b));
							PX_ASSERT(!mRemovedHandleMap.test(handle));		// a handle may only be updated and deleted if it was just added.
							PX_ASSERT(!mVolumeData[handle].isAggregate());	// PT: make sure changedShapes doesn't contain aggregates

							if(mAddedHandleMap.test(handle))					// just-inserted handles may also be marked updated, so skip them
								continue;

							if(mVolumeData[handle].isSingleActor())
							{
								PX_ASSERT(mGroups[handle] != Bp::FilterGroup::eINVALID);
								mUpdatedHandles.pushBack(handle);	// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
							}
							else
							{
								PX_ASSERT(mVolumeData[handle].isAggregated());
								const AggregateHandle aggregateHandle = mVolumeData[handle].getAggregateOwner();
								Aggregate* aggregate = getAggregateFromHandle(aggregateHandle);
								// PT: an actor from the aggregate has been updated => mark dirty to recompute bounds later
								// PT: we don't recompute the bounds right away since multiple actors from the same aggregate may have changed.
								aggregate->markAsDirty(mDirtyAggregates);
							}
						}
					}
				}
			}

			const PxU32 size = mDirtyAggregates.size();
			if(size)
			{
				PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - update - dirty iteration", getContextId());
				for(PxU32 i=0;i<size;i++)
				{
					Aggregate* aggregate = mDirtyAggregates[i];
					if(i!=size-1)
					{
						Aggregate* nextAggregate = mDirtyAggregates[i];
						Ps::prefetchLine(nextAggregate, 0);
					}

					aggregate->allocateBounds();
					if(singleThreaded)
					{
						aggregate->computeBounds(mBoundsArray, mContactDistance.begin());
						mBoundsArray.begin()[aggregate->mIndex] = aggregate->mBounds;
					}

					// PT: Can happen when an aggregate has been created and then its actors have been changed (with e.g. setLocalPose)
					// before a BP call.
					if(!mAddedHandleMap.test(aggregate->mIndex))
						mUpdatedHandles.pushBack(aggregate->mIndex);	// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
				}

				if(!singleThreaded)
					startAggregateBoundsComputationTasks(size, numCpuTasks, flushPool);

				// PT: we're already sorted if no dirty-aggregates are involved
				{
					PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - update - sort", getContextId());

					mPersistentStateChanged = true;	// PT: was previously set multiple times within 'computeBounds'
					// PT: TODO: remove this
					Ps::sort(mUpdatedHandles.begin(), mUpdatedHandles.size());
				}
			}
		}
		else
		{
			handleOriginShift();
		}
	}

	// Remove
	{
		PX_PROFILE_ZONE("SimpleAABBManager::updateAABBsAndBP - remove", getContextId());

		resetOrClear(mRemovedHandles);

		const PxU32* bits = mRemovedHandleMap.getWords();
		if(bits)
		{
			const PxU32 lastSetBit = mRemovedHandleMap.findLast();
			for(PxU32 w = 0; w <= lastSetBit >> 5; ++w)
			{
				for(PxU32 b = bits[w]; b; b &= b-1)
				{
					const BoundsIndex handle = PxU32(w<<5|Ps::lowestSetBit(b));
					PX_ASSERT(!mVolumeData[handle].isAggregated());
					mRemovedHandles.pushBack(handle);	// PT: TODO: BoundsIndex-to-ShapeHandle confusion here
				}
			}
		}
	}

	/////

	// PT: TODO: do we need to run these threads when we origin-shifted everything before?
	if(singleThreaded)
		finalizeUpdate(numCpuTasks, scratchAllocator, continuation, narrowPhaseUnlockTask);
	else
		mFinalizeUpdateTask.removeReference();
}

void SimpleAABBManager::finalizeUpdate(PxU32 numCpuTasks, PxcScratchAllocator* scratchAllocator, PxBaseTask* continuation, PxBaseTask* narrowPhaseUnlockTask)
{
	PX_PROFILE_ZONE("SimpleAABBManager::finalizeUpdate", getContextId());

	const bool singleThreaded = gSingleThreaded || numCpuTasks<2;
	if(!singleThreaded)
	{
		const PxU32 size = mDirtyAggregates.size();
		for(PxU32 i=0;i<size;i++)
		{
			Aggregate* aggregate = mDirtyAggregates[i];
			mBoundsArray.begin()[aggregate->mIndex] = aggregate->mBounds;
		}
	}

	const BroadPhaseUpdateData updateData(	mAddedHandles.begin(), mAddedHandles.size(),
											mUpdatedHandles.begin(), mUpdatedHandles.size(),
											mRemovedHandles.begin(), mRemovedHandles.size(),
											mBoundsArray.begin(), mGroups.begin(),
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
											&mLUT[0][0],
#endif
											mContactDistance.begin(), mBoundsArray.getCapacity(),
											mPersistentStateChanged || mBoundsArray.hasChanged());
	mPersistentStateChanged = false;

	PX_ASSERT(updateData.isValid());
	
	//KS - skip broad phase if there are no updated shapes.
	if (updateData.getNumCreatedHandles() != 0 || updateData.getNumRemovedHandles() != 0 || updateData.getNumUpdatedHandles() != 0)
		mBroadPhase.update(numCpuTasks, scratchAllocator, updateData, continuation, narrowPhaseUnlockTask);
	else
		narrowPhaseUnlockTask->removeReference();
}

static PX_FORCE_INLINE void createOverlap(Ps::Array<AABBOverlap>* overlaps, const Ps::Array<VolumeData>& volumeData, PxU32 id0, PxU32 id1/*, ActorHandle handle*/)
{
//	overlaps.pushBack(AABBOverlap(volumeData[id0].userData, volumeData[id1].userData, handle));
	const PxU8 volumeType = PxMax(volumeData[id0].getVolumeType(), volumeData[id1].getVolumeType());
	overlaps[volumeType].pushBack(AABBOverlap(reinterpret_cast<void*>(size_t(id0)), reinterpret_cast<void*>(size_t(id1))/*, handle*/));
}

static PX_FORCE_INLINE void deleteOverlap(Ps::Array<AABBOverlap>* overlaps, const Ps::Array<VolumeData>& volumeData, PxU32 id0, PxU32 id1/*, ActorHandle handle*/)
{
//	PX_ASSERT(volumeData[id0].userData);
//	PX_ASSERT(volumeData[id1].userData);
	if (volumeData[id0].getUserData() && volumeData[id1].getUserData())	// PT: TODO: no idea if this is the right thing to do or if it's normal to get null ptrs here
	{
		const PxU8 volumeType = PxMax(volumeData[id0].getVolumeType(), volumeData[id1].getVolumeType());
//		overlaps.pushBack(AABBOverlap(volumeData[id0].userData, volumeData[id1].userData, handle));
		overlaps[volumeType].pushBack(AABBOverlap(reinterpret_cast<void*>(size_t(id0)), reinterpret_cast<void*>(size_t(id1))/*, handle*/));
	}
}

void PersistentPairs::outputDeletedOverlaps(Ps::Array<AABBOverlap>* overlaps, const Ps::Array<VolumeData>& volumeData)
{
#ifdef USE_MBP_PAIR_MANAGER
	const PxU32 nbActivePairs = mPM.mNbActivePairs;
	for(PxU32 i=0;i<nbActivePairs;i++)
	{
		const MBP_Pair& p = mPM.mActivePairs[i];
		deleteOverlap(overlaps, volumeData, p.getId0(), p.getId1()/*, BP_INVALID_BP_HANDLE*/);
	}
#else
	const PxU32 size = mCurrentPairs[mCurrentPairArray].size();
	const Pair* pairs = mCurrentPairs[mCurrentPairArray].begin();
	for(PxU32 i=0;i<size;i++)
		deleteOverlap(overlaps, volumeData, pairs[i].mID0, pairs[i].mID1, BP_INVALID_BP_HANDLE);
#endif
}

#ifndef USE_MBP_PAIR_MANAGER
static void computePairDeltas(/*const*/ Ps::Array<Pair>& newPairs, const Ps::Array<Pair>& previousPairs,
							  Ps::Array<VolumeData>& volumeData, Ps::Array<AABBOverlap>& createdOverlaps, Ps::Array<AABBOverlap>& destroyedOverlaps)
{
	if(0)
	{
		Ps::sort(newPairs.begin(), newPairs.size());
		(void)previousPairs;
		(void)volumeData;
		(void)createdOverlaps;
		(void)destroyedOverlaps;
/*		Ps::sort(newPairs.begin(), newPairs.size());
		const Pair* prevPairs = previousPairs.begin();
		const Pair* currPairs = newPairs.begin();
		const Pair& p = *currPairs;
		const Pair& o = *prevPairs;
		if(p==o)
		{
		}*/

	}
	else
	{

	// PT: TODO: optimize
	const PxU32 nbPreviousPairs = previousPairs.size();
/*	if(!nbPreviousPairs)
	{
		// PT: "HashSet::erase" still computes the hash when the table is empty
		const PxU32 size = newPairs.size();
		for(PxU32 i=0;i<size;i++)
			createOverlap(createdOverlaps, volumeData, newPairs[i].mID0, newPairs[i].mID1, BP_INVALID_BP_HANDLE);	// pairs[i] is new => add to mCreatedOverlaps
	}
	else*/
	{
		Ps::HashSet<Pair> existingPairs;
		{
			for(PxU32 i=0;i<nbPreviousPairs;i++)
				existingPairs.insert(previousPairs[i]);
		}

		const PxU32 size = newPairs.size();
		for(PxU32 i=0;i<size;i++)
		{
			if(!existingPairs.erase(newPairs[i]))	// PT: pairs[i] existed before => persistent pair => nothing to do. Just remove from current pairs so that we don't delete it later
				createOverlap(createdOverlaps, volumeData, newPairs[i].mID0, newPairs[i].mID1, BP_INVALID_BP_HANDLE);	// pairs[i] is new => add to mCreatedOverlaps
		}

		// PT: pairs remaining in previousPairs did not appear in pairs => add to mDestroyedOverlaps
		for(Ps::HashSet<Pair>::Iterator iter = existingPairs.getIterator(); !iter.done(); ++iter)
			deleteOverlap(destroyedOverlaps, volumeData, iter->mID0, iter->mID1, BP_INVALID_BP_HANDLE);
	}

	}
}
#endif

PX_FORCE_INLINE void PersistentPairs::updatePairs(	PxU32 timestamp, const PxBounds3* bounds, const float* contactDistances, const Bp::FilterGroup::Enum* groups,
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	const bool* lut,
#endif
									Ps::Array<VolumeData>& volumeData, Ps::Array<AABBOverlap>* createdOverlaps, Ps::Array<AABBOverlap>* destroyedOverlaps)
{
	if(mTimestamp==timestamp)
		return;

	mTimestamp = timestamp;

#ifdef USE_MBP_PAIR_MANAGER
	#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
	findOverlaps(mPM, bounds, contactDistances, groups, lut);
	#else
	findOverlaps(mPM, bounds, contactDistances, groups);
	#endif

	PxU32 i=0;
	PxU32 nbActivePairs = mPM.mNbActivePairs;
	while(i<nbActivePairs)
	{
		MBP_Pair& p = mPM.mActivePairs[i];
		const PxU32 id0 = p.getId0();
		const PxU32 id1 = p.getId1();

		if(p.isNew())
		{
			createOverlap(createdOverlaps, volumeData, id0, id1/*, BP_INVALID_BP_HANDLE*/);

			p.clearNew();
			p.clearUpdated();

			i++;
		}
		else if(p.isUpdated())
		{
			p.clearUpdated();
			i++;
		}
		else
		{
			deleteOverlap(destroyedOverlaps, volumeData, id0, id1/*, BP_INVALID_BP_HANDLE*/);

			const PxU32 hashValue = hash(id0, id1) & mPM.mMask;
			mPM.removePair(id0, id1, hashValue, i);
			nbActivePairs--;
		}
	}
	mPM.shrinkMemory();
#else
	const PxU32 otherArray = 1 - mCurrentPairArray;
	Ps::Array<Pair>& mTempPairs = mCurrentPairs[otherArray];

//		mTempPairs.clear();
	mTempPairs.forceSize_Unsafe(0);

	findOverlaps(mTempPairs, bounds, contactDistances, groups);

	computePairDeltas(mTempPairs, mCurrentPairs[mCurrentPairArray], volumeData, createdOverlaps, destroyedOverlaps);
	mCurrentPairArray = otherArray;
#endif
}

PersistentActorAggregatePair* SimpleAABBManager::createPersistentActorAggregatePair(ShapeHandle volA, ShapeHandle volB)
{
	ShapeHandle	actorHandle;
	ShapeHandle	aggregateHandle;
	if(mVolumeData[volA].isAggregate())
	{
		aggregateHandle = volA;
		actorHandle = volB;
	}
	else
	{
		PX_ASSERT(mVolumeData[volB].isAggregate());
		aggregateHandle = volB;
		actorHandle = volA;
	}
	const AggregateHandle h = mVolumeData[aggregateHandle].getAggregate();
	Aggregate* aggregate = getAggregateFromHandle(h);
	PX_ASSERT(aggregate->mIndex==aggregateHandle);
	return PX_NEW(PersistentActorAggregatePair)(aggregate, actorHandle);	// PT: TODO: use a pool or something
}

PersistentAggregateAggregatePair* SimpleAABBManager::createPersistentAggregateAggregatePair(ShapeHandle volA, ShapeHandle volB)
{
	PX_ASSERT(mVolumeData[volA].isAggregate());
	PX_ASSERT(mVolumeData[volB].isAggregate());
	const AggregateHandle h0 = mVolumeData[volA].getAggregate();
	const AggregateHandle h1 = mVolumeData[volB].getAggregate();
	Aggregate* aggregate0 = getAggregateFromHandle(h0);
	Aggregate* aggregate1 = getAggregateFromHandle(h1);
	PX_ASSERT(aggregate0->mIndex==volA);
	PX_ASSERT(aggregate1->mIndex==volB);
	return PX_NEW(PersistentAggregateAggregatePair)(aggregate0, aggregate1);	// PT: TODO: use a pool or something
}

void SimpleAABBManager::updatePairs(PersistentPairs& p, BpCacheData* data)
{
	if (data)
	{
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
		p.updatePairs(mTimestamp, mBoundsArray.begin(), mContactDistance.begin(), mGroups.begin(), &mLUT[0][0], mVolumeData, data->mCreatedPairs, data->mDeletedPairs);
#else
		p.updatePairs(mTimestamp, mBoundsArray.begin(), mContactDistance.begin(), mGroups.begin(), mVolumeData, data->mCreatedPairs, data->mDeletedPairs);
#endif
	}
	else
	{
#ifdef BP_FILTERING_USES_TYPE_IN_GROUP
		p.updatePairs(mTimestamp, mBoundsArray.begin(), mContactDistance.begin(), mGroups.begin(), &mLUT[0][0], mVolumeData, mCreatedOverlaps, mDestroyedOverlaps);
#else
		p.updatePairs(mTimestamp, mBoundsArray.begin(), mContactDistance.begin(), mGroups.begin(), mVolumeData, mCreatedOverlaps, mDestroyedOverlaps);
#endif
	}
}

void SimpleAABBManager::processBPCreatedPair(const BroadPhasePair& pair)
{
	PX_ASSERT(!mVolumeData[pair.mVolA].isAggregated());
	PX_ASSERT(!mVolumeData[pair.mVolB].isAggregated());
	const bool isSingleActorA = mVolumeData[pair.mVolA].isSingleActor();
	const bool isSingleActorB = mVolumeData[pair.mVolB].isSingleActor();

	if(isSingleActorA && isSingleActorB)
	{
		createOverlap(mCreatedOverlaps, mVolumeData, pair.mVolA, pair.mVolB/*, pair.mHandle*/);	// PT: regular actor-actor pair
		return;
	}

	// PT: TODO: check if this is needed
	ShapeHandle volA = pair.mVolA;
	ShapeHandle volB = pair.mVolB;
	if(volB<volA)
		Ps::swap(volA, volB);

	PersistentPairs* newPair;
	AggPairMap* pairMap;
	if(!isSingleActorA && !isSingleActorB)
	{
		pairMap = &mAggregateAggregatePairs;	// PT: aggregate-aggregate pair
		newPair = createPersistentAggregateAggregatePair(volA, volB);
	}
	else
	{
		pairMap = &mActorAggregatePairs;	// PT: actor-aggregate pair
		newPair = createPersistentActorAggregatePair(volA, volB);
	}
	bool status = pairMap->insert(AggPair(volA, volB), newPair);
	PX_UNUSED(status);
	PX_ASSERT(status);
	updatePairs(*newPair);

#ifdef EXPERIMENT
	BroadPhasePair* bpPairs = mBroadPhase.getBroadPhasePairs();
	if(bpPairs)
		bpPairs[pair.mHandle].mUserData = newPair;
#endif
}

void SimpleAABBManager::processBPDeletedPair(const BroadPhasePair& pair)
{
	PX_ASSERT(!mVolumeData[pair.mVolA].isAggregated());
	PX_ASSERT(!mVolumeData[pair.mVolB].isAggregated());
	const bool isSingleActorA = mVolumeData[pair.mVolA].isSingleActor();
	const bool isSingleActorB = mVolumeData[pair.mVolB].isSingleActor();

	if(isSingleActorA && isSingleActorB)
	{
		deleteOverlap(mDestroyedOverlaps, mVolumeData, pair.mVolA, pair.mVolB/*, pair.mHandle*/);	// PT: regular actor-actor pair
		return;
	}

	// PT: TODO: check if this is needed
	ShapeHandle volA = pair.mVolA;
	ShapeHandle volB = pair.mVolB;
	if(volB<volA)
		Ps::swap(volA, volB);

	AggPairMap* pairMap;
	if(!isSingleActorA && !isSingleActorB)
		pairMap = &mAggregateAggregatePairs;	// PT: aggregate-aggregate pair
	else
		pairMap = &mActorAggregatePairs;		// PT: actor-aggregate pair

	PersistentPairs* p;
#ifdef EXPERIMENT
	BroadPhasePair* bpPairs = mBroadPhase.getBroadPhasePairs();
	if(bpPairs)
	{
		p = reinterpret_cast<PersistentPairs*>(bpPairs[pair.mHandle].mUserData);
	}
	else
#endif
	{
		const AggPairMap::Entry* e = pairMap->find(AggPair(volA, volB));
		PX_ASSERT(e);
		p = e->second;
	}

	p->outputDeletedOverlaps(mDestroyedOverlaps, mVolumeData);
	p->mShouldBeDeleted = true;
}

struct CreatedPairHandler
{
	static PX_FORCE_INLINE void processPair(SimpleAABBManager& manager, const BroadPhasePair& pair) { manager.processBPCreatedPair(pair); }
};

struct DeletedPairHandler
{
	static PX_FORCE_INLINE void processPair(SimpleAABBManager& manager, const BroadPhasePair& pair) { manager.processBPDeletedPair(pair); }
};

template<class FunctionT>
static void processBPPairs(PxU32 nbPairs, const BroadPhasePair* pairs, SimpleAABBManager& manager)
{
	// PT: TODO: figure out this ShapeHandle/BpHandle thing. Is it ok to use "BP_INVALID_BP_HANDLE" for a "ShapeHandle"?
	ShapeHandle previousA = BP_INVALID_BP_HANDLE;
	ShapeHandle previousB = BP_INVALID_BP_HANDLE;

	while(nbPairs--)
	{
		PX_ASSERT(pairs->mVolA!=BP_INVALID_BP_HANDLE);
		PX_ASSERT(pairs->mVolB!=BP_INVALID_BP_HANDLE);
		// PT: TODO: why is that test needed now? GPU broadphase?
		if(pairs->mVolA != previousA || pairs->mVolB != previousB)
		{
			previousA = pairs->mVolA;
			previousB = pairs->mVolB;
			FunctionT::processPair(manager, *pairs);
		}
		pairs++;
	}
}

static void processAggregatePairs(AggPairMap& map, SimpleAABBManager& manager)
{
	// PT: TODO: hmmm we have a list of dirty aggregates but we don't have a list of dirty pairs.
	// PT: not sure how the 3.4 trunk solves this but let's just iterate all pairs for now
	// PT: atm we reuse this loop to delete removed interactions
	// PT: TODO: in fact we could handle all the "lost pairs" stuff right there with extra aabb-abb tests

	// PT: TODO: replace with decent hash map - or remove the hashmap entirely and use a linear array
	Ps::Array<AggPair> removedEntries;
	for(AggPairMap::Iterator iter = map.getIterator(); !iter.done(); ++iter)
	{
		PersistentPairs* p = iter->second;
		if(p->update(manager))
		{
			removedEntries.pushBack(iter->first);
			PX_DELETE(p);
		}
	}
	for(PxU32 i=0;i<removedEntries.size();i++)
	{
		bool status = map.erase(removedEntries[i]);
		PX_ASSERT(status);
		PX_UNUSED(status);
	}
}

struct PairData
{
	Ps::Array<AABBOverlap>* mArray;
	PxU32 mStartIdx;
	PxU32 mCount;

	PairData() : mArray(NULL), mStartIdx(0), mCount(0)
	{
	}
};

class ProcessAggPairsBase : public Cm::Task
{
public:
	static const PxU32 MaxPairs = 16;

	PairData mCreatedPairs[2];
	PairData mDestroyedPairs[2];

	ProcessAggPairsBase(PxU64 contextID) : Cm::Task(contextID)
	{
	}

	void setCache(BpCacheData& data)
	{
		for (PxU32 i = 0; i < 2; ++i)
		{
			mCreatedPairs[i].mArray = &data.mCreatedPairs[i];
			mCreatedPairs[i].mStartIdx = data.mCreatedPairs[i].size();
			mDestroyedPairs[i].mArray = &data.mDeletedPairs[i];
			mDestroyedPairs[i].mStartIdx = data.mDeletedPairs[i].size();
		}
	}

	void updateCounters()
	{
		for (PxU32 i = 0; i < 2; ++i)
		{
			mCreatedPairs[i].mCount = mCreatedPairs[i].mArray->size() - mCreatedPairs[i].mStartIdx;
			mDestroyedPairs[i].mCount = mDestroyedPairs[i].mArray->size() - mDestroyedPairs[i].mStartIdx;
		}
	}
};


class ProcessAggPairsParallelTask : public ProcessAggPairsBase
{
public:
	PersistentPairs* mPersistentPairs[MaxPairs];
	Bp::AggPair mAggPairs[MaxPairs];
	PxU32 mNbPairs;
	Bp::SimpleAABBManager* mManager;
	AggPairMap* mMap;
	Ps::Mutex* mMutex;
	const char* mName;

	ProcessAggPairsParallelTask(PxU64 contextID, Ps::Mutex* mutex, Bp::SimpleAABBManager* manager, AggPairMap* map, const char* name) : ProcessAggPairsBase(contextID),
		mNbPairs(0), mManager(manager), mMap(map), mMutex(mutex), mName(name)
	{
	}


	void runInternal()
	{
		BpCacheData* data = mManager->getBpCacheData();

		setCache(*data);


		Ps::InlineArray<AggPair, MaxPairs> removedEntries;
		for (PxU32 i = 0; i < mNbPairs; ++i)
		{
			if (mPersistentPairs[i]->update(*mManager, data))
			{
				removedEntries.pushBack(mAggPairs[i]);
				PX_DELETE(mPersistentPairs[i]);
			}
		}

		updateCounters();

		mManager->putBpCacheData(data);


		if (removedEntries.size())
		{
			Ps::Mutex::ScopedLock lock(*mMutex);
			for (PxU32 i = 0; i < removedEntries.size(); i++)
			{
				bool status = mMap->erase(removedEntries[i]);
				PX_ASSERT(status);
				PX_UNUSED(status);
			}
		}

		
	}

	virtual const char* getName() const { return mName; }

};

class SortAggregateBoundsParallel : public Cm::Task
{
public:
	static const PxU32 MaxPairs = 16;
	Aggregate** mAggregates;
	PxU32 mNbAggs;
	Bp::SimpleAABBManager* mManager;

	SortAggregateBoundsParallel(PxU64 contextID, Aggregate** aggs, PxU32 nbAggs) : Cm::Task(contextID),
		mAggregates(aggs), mNbAggs(nbAggs)
	{
	}


	void runInternal()
	{
		PX_PROFILE_ZONE("SortBounds", mContextID);
		for (PxU32 i = 0; i < mNbAggs; i++)
		{
			Aggregate* aggregate = mAggregates[i];

			aggregate->getSortedMinBounds();
		}
	}

	virtual const char* getName() const { return "SortAggregateBoundsParallel"; }

};


class ProcessSelfCollisionPairsParallel : public ProcessAggPairsBase
{
public:
	Aggregate** mAggregates;
	PxU32 mNbAggs;
	Bp::SimpleAABBManager* mManager;

	ProcessSelfCollisionPairsParallel(PxU64 contextID, Aggregate** aggs, PxU32 nbAggs, SimpleAABBManager* manager) : ProcessAggPairsBase(contextID),
		mAggregates(aggs), mNbAggs(nbAggs), mManager(manager)
	{
	}


	void runInternal()
	{
		BpCacheData* data = mManager->getBpCacheData();
		setCache(*data);
		PX_PROFILE_ZONE("ProcessSelfCollisionPairs", mContextID);
		for (PxU32 i = 0; i < mNbAggs; i++)
		{
			Aggregate* aggregate = mAggregates[i];

			if (aggregate->mSelfCollisionPairs)
				mManager->updatePairs(*aggregate->mSelfCollisionPairs, data);
		}
		updateCounters();
		mManager->putBpCacheData(data);
	}

	virtual const char* getName() const { return "ProcessSelfCollisionPairsParallel"; }

};

static void processAggregatePairsParallel(AggPairMap& map, SimpleAABBManager& manager, Cm::FlushPool& flushPool,
	PxBaseTask* continuation, const char* taskName, Ps::Array<ProcessAggPairsBase*>& pairTasks)
{
	// PT: TODO: hmmm we have a list of dirty aggregates but we don't have a list of dirty pairs.
	// PT: not sure how the 3.4 trunk solves this but let's just iterate all pairs for now
	// PT: atm we reuse this loop to delete removed interactions
	// PT: TODO: in fact we could handle all the "lost pairs" stuff right there with extra aabb-abb tests

	// PT: TODO: replace with decent hash map - or remove the hashmap entirely and use a linear array

	manager.mMapLock.lock();

	ProcessAggPairsParallelTask* task = PX_PLACEMENT_NEW(flushPool.allocate(sizeof(ProcessAggPairsParallelTask)), ProcessAggPairsParallelTask)(0, &manager.mMapLock, &manager, &map, taskName);


	PxU32 startIdx = pairTasks.size();

	for (AggPairMap::Iterator iter = map.getIterator(); !iter.done(); ++iter)
	{
		task->mAggPairs[task->mNbPairs] = iter->first;
		task->mPersistentPairs[task->mNbPairs++] = iter->second;
		if (task->mNbPairs == ProcessAggPairsParallelTask::MaxPairs)
		{
			pairTasks.pushBack(task);
			task->setContinuation(continuation);
			//task->runInternal();
			task = PX_PLACEMENT_NEW(flushPool.allocate(sizeof(ProcessAggPairsParallelTask)), ProcessAggPairsParallelTask)(0, &manager.mMapLock, &manager, &map, taskName);
		}
	}

	manager.mMapLock.unlock();

	for (PxU32 i = startIdx; i < pairTasks.size(); ++i)
	{
		pairTasks[i]->removeReference();
	}


	if (task->mNbPairs)
	{
		pairTasks.pushBack(task);
		task->setContinuation(continuation);
		task->removeReference();
		//task->runInternal();
	}
}

void SimpleAABBManager::postBroadPhase(PxBaseTask* continuation, PxBaseTask* narrowPhaseUnlockTask, Cm::FlushPool& flushPool)
{
	PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase", getContextId());

	//KS - There is a continuation task for discrete broad phase, but not for CCD broad phase. PostBroadPhase for CCD broad phase runs in-line.
	//This probably should be revisited but it means that we can't use the parallel codepath within CCD.
	if (continuation)
	{
		mPostBroadPhase3.setContinuation(continuation);
		mPostBroadPhase2.setContinuation(&mPostBroadPhase3);
	}

	mTimestamp++;

	// PT: TODO: consider merging mCreatedOverlaps & mDestroyedOverlaps
	// PT: TODO: revisit memory management of mCreatedOverlaps & mDestroyedOverlaps

	//KS - if we ran broad phase, fetch the results now
	if (mAddedHandles.size() != 0 || mUpdatedHandles.size() != 0 || mRemovedHandles.size() != 0)
		mBroadPhase.fetchBroadPhaseResults(narrowPhaseUnlockTask);

	for (PxU32 i = 0; i < VolumeBuckets::eCOUNT; ++i)
	{
		resetOrClear(mCreatedOverlaps[i]);
		resetOrClear(mDestroyedOverlaps[i]);
	}

	{
		PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - process deleted pairs", getContextId());
//		processBPPairs<CreatedPairHandler>(mBroadPhase.getNbCreatedPairs(), mBroadPhase.getCreatedPairs(), *this);
		processBPPairs<DeletedPairHandler>(mBroadPhase.getNbDeletedPairs(), mBroadPhase.getDeletedPairs(), *this);
	}

	{
		//If there is a continuation task, then this is not part of CCD, so we can trigger bounds to be recomputed in parallel before pair generation runs during
		//stage 2.			
		if (continuation)
		{
			const PxU32 size = mDirtyAggregates.size();
			for (PxU32 i = 0; i < size; i += SortAggregateBoundsParallel::MaxPairs)
			{
				const PxU32 nbToProcess = PxMin(size - i, SortAggregateBoundsParallel::MaxPairs);

				SortAggregateBoundsParallel* task = PX_PLACEMENT_NEW(flushPool.allocate(sizeof(SortAggregateBoundsParallel)), SortAggregateBoundsParallel)
					(mContextID, &mDirtyAggregates[i], nbToProcess);

				task->setContinuation(&mPostBroadPhase2);
				task->removeReference();
			}
		}
	}

	

	if (continuation)
	{
		mPostBroadPhase2.setFlushPool(&flushPool);
		mPostBroadPhase3.removeReference();
		mPostBroadPhase2.removeReference();
	}
	else
	{
		postBpStage2(NULL, flushPool);
		postBpStage3(NULL);
	}

}

void PostBroadPhaseStage2Task::runInternal()
{
	mManager.postBpStage2(mCont, *mFlushPool);
}

void SimpleAABBManager::postBpStage2(PxBaseTask* continuation, Cm::FlushPool& flushPool)
{
	{
		{
			const PxU32 size = mDirtyAggregates.size();
			for (PxU32 i = 0; i < size; i += ProcessSelfCollisionPairsParallel::MaxPairs)
			{
				const PxU32 nbToProcess = PxMin(size - i, ProcessSelfCollisionPairsParallel::MaxPairs);

				ProcessSelfCollisionPairsParallel* task = PX_PLACEMENT_NEW(flushPool.allocate(sizeof(ProcessSelfCollisionPairsParallel)), ProcessSelfCollisionPairsParallel)
					(mContextID, &mDirtyAggregates[i], nbToProcess, this);
				if (continuation)
				{
					task->setContinuation(continuation);
					task->removeReference();
				}
				else
					task->runInternal();
				mAggPairTasks.pushBack(task);
			}
		}

		{
			if (continuation)
				processAggregatePairsParallel(mAggregateAggregatePairs, *this, flushPool, continuation, "AggAggPairs", mAggPairTasks);
			else
				processAggregatePairs(mAggregateAggregatePairs, *this);
		}



		{
			if (continuation)
				processAggregatePairsParallel(mActorAggregatePairs, *this, flushPool, continuation, "AggActorPairs", mAggPairTasks);
			else
				processAggregatePairs(mActorAggregatePairs, *this);
		}


	}
}

void SimpleAABBManager::postBpStage3(PxBaseTask*)
{

	{
		{
			PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - aggregate self-collisions", getContextId());
			const PxU32 size = mDirtyAggregates.size();
			for (PxU32 i = 0; i < size; i++)
			{
				Aggregate* aggregate = mDirtyAggregates[i];
				aggregate->resetDirtyState();

			}
			resetOrClear(mDirtyAggregates);
		}
		
		{
			PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - append pairs", getContextId());

			for (PxU32 a = 0; a < mAggPairTasks.size(); ++a)
			{
				ProcessAggPairsBase* task = mAggPairTasks[a];
				for (PxU32 t = 0; t < 2; t++)
				{
					for (PxU32 i = 0, startIdx = task->mCreatedPairs[t].mStartIdx; i < task->mCreatedPairs[t].mCount; ++i)
					{
						mCreatedOverlaps[t].pushBack((*task->mCreatedPairs[t].mArray)[i+ startIdx]);
					}
					for (PxU32 i = 0, startIdx = task->mDestroyedPairs[t].mStartIdx; i < task->mDestroyedPairs[t].mCount; ++i)
					{
						mDestroyedOverlaps[t].pushBack((*task->mDestroyedPairs[t].mArray)[i + startIdx]);
					}
				}
			}

			mAggPairTasks.forceSize_Unsafe(0);


			Ps::InlineArray<BpCacheData*, 16> bpCache;
			BpCacheData* entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());

			while (entry)
			{
				entry->reset();
				bpCache.pushBack(entry);
				entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
			}

			//Now reinsert back into queue...
			for (PxU32 i = 0; i < bpCache.size(); ++i)
			{
				mBpThreadContextPool.push(*bpCache[i]);
			}
		}
	}

	{
		PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - process created pairs", getContextId());
		processBPPairs<CreatedPairHandler>(mBroadPhase.getNbCreatedPairs(), mBroadPhase.getCreatedPairs(), *this);
//		processBPPairs<DeletedPairHandler>(mBroadPhase.getNbDeletedPairs(), mBroadPhase.getDeletedPairs(), *this);
	}

	// PT: TODO: revisit this
	// Filter out pairs in mDestroyedOverlaps that already exist in mCreatedOverlaps. This should be done better using bitmaps
	// and some imposed ordering on previous operations. Later.
	// We could also have a dedicated function "reinsertBroadPhase()", which would preserve the existing interactions at Sc-level.
	if(1)
	{
		PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - post-process", getContextId());

		PxU32 totalCreatedOverlaps = 0;
		for (PxU32 idx = 0; idx < VolumeBuckets::eCOUNT; ++idx)
			totalCreatedOverlaps += mCreatedOverlaps[idx].size();

		mCreatedPairs.clear();
		mCreatedPairs.reserve(totalCreatedOverlaps);
		
		for (PxU32 idx = 0; idx < VolumeBuckets::eCOUNT; ++idx)
		{
			const PxU32 nbDestroyedOverlaps = mDestroyedOverlaps[idx].size();
			{
				const PxU32 size = mCreatedOverlaps[idx].size();
				for (PxU32 i = 0; i < size; i++)
				{
					const PxU32 id0 = PxU32(size_t(mCreatedOverlaps[idx][i].mUserData0));
					const PxU32 id1 = PxU32(size_t(mCreatedOverlaps[idx][i].mUserData1));
					mCreatedOverlaps[idx][i].mUserData0 = mVolumeData[id0].getUserData();
					mCreatedOverlaps[idx][i].mUserData1 = mVolumeData[id1].getUserData();
					if (nbDestroyedOverlaps)
						mCreatedPairs.insert(Pair(id0, id1));
				}
			}
			PxU32 newSize = 0;
			for (PxU32 i = 0; i < nbDestroyedOverlaps; i++)
			{
				const PxU32 id0 = PxU32(size_t(mDestroyedOverlaps[idx][i].mUserData0));
				const PxU32 id1 = PxU32(size_t(mDestroyedOverlaps[idx][i].mUserData1));
				if (!mCreatedPairs.contains(Pair(id0, id1)))
				{
					mDestroyedOverlaps[idx][newSize].mUserData0 = mVolumeData[id0].getUserData();
					mDestroyedOverlaps[idx][newSize].mUserData1 = mVolumeData[id1].getUserData();
					newSize++;
				}
			}
			mDestroyedOverlaps[idx].forceSize_Unsafe(newSize);
		}
	}

	// Handle out-of-bounds objects
	{
		PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - out-of-bounds", getContextId());
		PxU32 nbObjects = mBroadPhase.getNbOutOfBoundsObjects();
		const PxU32* objects = mBroadPhase.getOutOfBoundsObjects();
		while(nbObjects--)
		{
			const PxU32 index = *objects++;
			if(!mRemovedHandleMap.test(index))
			{
				if(mVolumeData[index].isSingleActor())
					mOutOfBoundsObjects.pushBack(mVolumeData[index].getUserData());
				else
				{
					PX_ASSERT(mVolumeData[index].isAggregate());
					mOutOfBoundsAggregates.pushBack(mVolumeData[index].getUserData());
				}
			}
		}
	}

	{
		PX_PROFILE_ZONE("SimpleAABBManager::postBroadPhase - clear", getContextId());
		mAddedHandleMap.clear();
		mRemovedHandleMap.clear();
	}
}

void SimpleAABBManager::freeBuffers()
{
	// PT: TODO: investigate if we need more stuff here
	mBroadPhase.freeBuffers();
}

void SimpleAABBManager::shiftOrigin(const PxVec3& shift)
{
	mBroadPhase.shiftOrigin(shift);
	mOriginShifted = true;
}

BpCacheData* SimpleAABBManager::getBpCacheData()
{
	BpCacheData* rv = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
	if (rv == NULL)
	{
		rv = PX_PLACEMENT_NEW(PX_ALLOC(sizeof(BpCacheData), PX_DEBUG_EXP("BpCacheData")), BpCacheData)();
	}
	return rv;
}
void SimpleAABBManager::putBpCacheData(BpCacheData* data)
{
	mBpThreadContextPool.push(*data);
}
void SimpleAABBManager::resetBpCacheData()
{
	Ps::InlineArray<BpCacheData*, 16> bpCache;
	BpCacheData* entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
	while (entry)
	{
		entry->reset();
		bpCache.pushBack(entry);
		entry = static_cast<BpCacheData*>(mBpThreadContextPool.pop());
	}

	//Now reinsert back into queue...
	for (PxU32 i = 0; i < bpCache.size(); ++i)
	{
		mBpThreadContextPool.push(*bpCache[i]);
	}
}

// PT: disabled this by default, since it bypasses all per-shape/per-actor visualization flags
//static const bool gVisualizeAggregateElems = false;

void SimpleAABBManager::visualize(Cm::RenderOutput& out)
{
	const PxTransform idt = PxTransform(PxIdentity);
	out << idt;

	const PxU32 N = mAggregates.size();
	for(PxU32 i=0;i<N;i++)
	{
		Aggregate* aggregate = mAggregates[i];
		if(aggregate->getNbAggregated())
		{
			out << PxU32(PxDebugColor::eARGB_GREEN);
			const PxBounds3& b = mBoundsArray.getBounds(aggregate->mIndex);
			out << Cm::DebugBox(b, true);
		}
	}

/*	const PxU32 N = mAggregateManager.getAggregatesCapacity();
	for(PxU32 i=0;i<N;i++)
	{
		const Aggregate* aggregate = mAggregateManager.getAggregate(i);
		if(aggregate->nbElems)
		{
			if(!mAggregatesUpdated.isInList(BpHandle(i)))
				out << PxU32(PxDebugColor::eARGB_GREEN);
			else
				out << PxU32(PxDebugColor::eARGB_RED);

			PxBounds3 decoded;
			const IntegerAABB& iaabb = mBPElems.getAABB(aggregate->bpElemId);
			iaabb.decode(decoded);

			out << Cm::DebugBox(decoded, true);

			if(gVisualizeAggregateElems)
			{
				PxU32 elem = aggregate->elemHeadID;
				while(BP_INVALID_BP_HANDLE!=elem)
				{
					out << PxU32(PxDebugColor::eARGB_CYAN);
					const IntegerAABB elemBounds = mAggregateElems.getAABB(elem);
					elemBounds.decode(decoded);
					out << Cm::DebugBox(decoded, true);
					elem = mAggregateElems.getNextId(elem);
				}
			}
		}
	}*/
}

} //namespace Bp

} //namespace physx
