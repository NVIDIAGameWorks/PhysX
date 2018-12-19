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
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		IceRevisitedRadix.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERADIXSORT_BASE_H__
#define __ICERADIXSORT_BASE_H__

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"
#include <PsUserAllocated.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

	class RadixSort : public UserAllocated
	{
		public:
		// Constructor/Destructor
								RadixSort();
								virtual ~RadixSort();
		// Sorting methods
				RadixSort&		Sort(const uint32_t* input, uint32_t nb, bool signedvalues=true);
				RadixSort&		Sort(const float* input, uint32_t nb);

		//! Access to results. mRanks is a list of indices in sorted order, i.e. in the order you may further process your data
		inline	uint32_t*			GetRanks()			const	{ return mRanks;		}

		//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
		inline	uint32_t*			GetRecyclable()		const	{ return mRanks2;		}

		// Stats
				uint32_t			GetUsedRam()		const;
		//! Returns the total number of calls to the radix sorter.
		inline	uint32_t			GetNbTotalCalls()	const	{ return mTotalCalls;	}
		//! Returns the number of premature exits due to temporal coherence.
		inline uint32_t			GetNbHits()			const	{ return mNbHits;		}

		private:

				uint32_t			mHistogram[256*4];			//!< Counters for each byte
				uint32_t			mOffset[256];				//!< Offsets (nearly a cumulative distribution function)
				uint32_t			mCurrentSize;				//!< Current size of the indices list

				uint32_t           mRanksSize;
				uint32_t*			mRanks;				//!< Two lists, swapped each pass
				uint32_t*			mRanks2;
		// Stats
				uint32_t			mTotalCalls;
				uint32_t			mNbHits;
		// Internal methods
				void			CheckResize(uint32_t nb);
				bool			Resize(uint32_t nb);
	};

}
}
}

#endif // __ICERADIXSORT_H__
#endif
