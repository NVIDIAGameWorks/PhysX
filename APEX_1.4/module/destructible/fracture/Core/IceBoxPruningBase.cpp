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



#include "RTdef.h"
#if RT_COMPILE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains code for box pruning.
 *	\file		IceBoxPruning.cpp
 *	\author		Pierre Terdiman
 *	\date		January, 29, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	You could use a complex sweep-and-prune as implemented in I-Collide.
	You could use a complex hashing scheme as implemented in V-Clip or recently in ODE it seems.
	You could use a "Recursive Dimensional Clustering" algorithm as implemented in GPG2.

	Or you could use this.
	Faster ? I don't know. Probably not. It would be a shame. But who knows ?
	Easier ? Definitely. Enjoy the sheer simplicity.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "IceBoxPruningBase.h"

namespace nvidia
{
namespace fracture
{
namespace base
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Bipartite box pruning. Returns a list of overlapping pairs of boxes, each box of the pair belongs to a different set.
 *	\param		nb0		[in] number of boxes in the first set
 *	\param		bounds0	[in] list of boxes for the first set
 *	\param		nb1		[in] number of boxes in the second set
 *	\param		bounds1	[in] list of boxes for the second set
 *	\param		pairs	[out] list of overlapping pairs
 *	\param		axes	[in] projection order (0,2,1 is often best)
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoxPruning::bipartiteBoxPruning(const nvidia::Array<PxBounds3> &bounds0, const nvidia::Array<PxBounds3> &bounds1, nvidia::Array<uint32_t>& pairs, const Axes& axes)
{
	uint32_t nb0 = bounds0.size();
	uint32_t nb1 = bounds1.size();
	pairs.clear();
	// Checkings
	if(nb0 == 0 || nb1 == 0)	return false;

	// Catch axes
	uint32_t Axis0 = axes.Axis0;
	//uint32_t Axis1 = axes.Axis1;
	//uint32_t Axis2 = axes.Axis2;

	// Allocate some temporary data
	if (mMinPosBounds0.size() < nb0)
		mMinPosBounds0.resize(nb0);
	if (mMinPosBounds1.size() < nb1)
		mMinPosBounds1.resize(nb1);

	// 1) Build main lists using the primary axis
	for(uint32_t i=0;i<nb0;i++)	mMinPosBounds0[i] = bounds0[i].minimum[Axis0];
	for(uint32_t i=0;i<nb1;i++)	mMinPosBounds1[i] = bounds1[i].minimum[Axis0];

	// 2) Sort the lists
	uint32_t* Sorted0 = mRS0.Sort(&mMinPosBounds0[0], nb0).GetRanks();
	uint32_t* Sorted1 = mRS1.Sort(&mMinPosBounds1[0], nb1).GetRanks();

	// 3) Prune the lists
	uint32_t Index0, Index1;

	const uint32_t* const LastSorted0 = &Sorted0[nb0];
	const uint32_t* const LastSorted1 = &Sorted1[nb1];
	const uint32_t* RunningAddress0 = Sorted0;
	const uint32_t* RunningAddress1 = Sorted1;

	while(RunningAddress1<LastSorted1 && Sorted0<LastSorted0)
	{
		Index0 = *Sorted0++;

		while(RunningAddress1<LastSorted1 && mMinPosBounds1[*RunningAddress1]<mMinPosBounds0[Index0])	RunningAddress1++;

		const uint32_t* RunningAddress2_1 = RunningAddress1;

		while(RunningAddress2_1<LastSorted1 && mMinPosBounds1[Index1 = *RunningAddress2_1++]<=bounds0[Index0].maximum[Axis0])
		{
			if(bounds0[Index0].intersects(bounds1[Index1]))
			{
				pairs.pushBack(Index0);
				pairs.pushBack(Index1);
			}
		}
	}

	////

	while(RunningAddress0<LastSorted0 && Sorted1<LastSorted1)
	{
		Index0 = *Sorted1++;

		while(RunningAddress0<LastSorted0 && mMinPosBounds0[*RunningAddress0]<=mMinPosBounds1[Index0])	RunningAddress0++;

		const uint32_t* RunningAddress2_0 = RunningAddress0;

		while(RunningAddress2_0<LastSorted0 && mMinPosBounds0[Index1 = *RunningAddress2_0++]<=bounds1[Index0].maximum[Axis0])
		{
			if(bounds0[Index1].intersects(bounds1[Index0]))
			{
				pairs.pushBack(Index1);
				pairs.pushBack(Index0);
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Complete box pruning. Returns a list of overlapping pairs of boxes, each box of the pair belongs to the same set.
 *	\param		nb		[in] number of boxes
 *	\param		list	[in] list of boxes
 *	\param		pairs	[out] list of overlapping pairs
 *	\param		axes	[in] projection order (0,2,1 is often best)
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoxPruning::completeBoxPruning(const nvidia::Array<PxBounds3> &bounds, nvidia::Array<uint32_t> &pairs, const Axes& axes)
{
	uint32_t nb = bounds.size();
	pairs.clear();

	// Checkings
	if(!nb)	return false;

	// Catch axes
	uint32_t Axis0 = axes.Axis0;
	//uint32_t Axis1 = axes.Axis1;
	//uint32_t Axis2 = axes.Axis2;

	// Allocate some temporary data
	if (mPosList.size() < nb)
		mPosList.resize(nb);

	// 1) Build main list using the primary axis
	for(uint32_t i=0;i<nb;i++)	mPosList[i] = bounds[i].minimum[Axis0];

	// 2) Sort the list
	uint32_t* Sorted = mRS.Sort(&mPosList[0], nb).GetRanks();

	// 3) Prune the list
	const uint32_t* const LastSorted = &Sorted[nb];
	const uint32_t* RunningAddress = Sorted;
	uint32_t Index0, Index1;
	while(RunningAddress<LastSorted && Sorted<LastSorted)
	{
		Index0 = *Sorted++;

		while(RunningAddress<LastSorted && mPosList[*RunningAddress++]<mPosList[Index0]);

		const uint32_t* RunningAddress2 = RunningAddress;

		while(RunningAddress2<LastSorted && mPosList[Index1 = *RunningAddress2++]<=bounds[Index0].maximum[Axis0])
		{
			if(Index0!=Index1)
			{
				if(bounds[Index0].intersects(bounds[Index1]))
				{
					pairs.pushBack(Index0);
					pairs.pushBack(Index1);
				}
			}
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Brute-force versions are kept:
// - to check the optimized versions return the correct list of intersections
// - to check the speed of the optimized code against the brute-force one
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Brute-force bipartite box pruning. Returns a list of overlapping pairs of boxes, each box of the pair belongs to a different set.
 *	\param		nb0		[in] number of boxes in the first set
 *	\param		bounds0	[in] list of boxes for the first set
 *	\param		nb1		[in] number of boxes in the second set
 *	\param		bounds1	[in] list of boxes for the second set
 *	\param		pairs	[out] list of overlapping pairs
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoxPruning::bruteForceBipartiteBoxTest(const nvidia::Array<PxBounds3> &bounds0, const nvidia::Array<PxBounds3> &bounds1, nvidia::Array<uint32_t>& pairs, const Axes& /*axes*/)
{
	uint32_t nb0 = bounds0.size();
	uint32_t nb1 = bounds1.size();
	pairs.clear();

	// Checkings
	if(!nb0 || !nb1)	return false;

	// Brute-force nb0*nb1 overlap tests
	for(uint32_t i=0;i<nb0;i++)
	{
		for(uint32_t j=0;j<nb1;j++)
		{
			if(bounds0[i].intersects(bounds1[j])) {
				pairs.pushBack(i);
				pairs.pushBack(j);
			}
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Complete box pruning. Returns a list of overlapping pairs of boxes, each box of the pair belongs to the same set.
 *	\param		nb		[in] number of boxes
 *	\param		list	[in] list of boxes
 *	\param		pairs	[out] list of overlapping pairs
 *	\return		true if success.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool BoxPruning::bruteForceCompleteBoxTest(const nvidia::Array<PxBounds3> &bounds, nvidia::Array<uint32_t> &pairs, const Axes& /*axes*/)
{
	uint32_t nb = bounds.size();
	pairs.clear();

	// Checkings
	if(!nb)	return false;

	// Brute-force n(n-1)/2 overlap tests
	for(uint32_t i=0;i<nb;i++)
	{
		for(uint32_t j=i+1;j<nb;j++)
		{
			if(bounds[i].intersects(bounds[j]))	
			{
				pairs.pushBack(i);
				pairs.pushBack(j);
			}
		}
	}
	return true;
}

}
}
}
#endif