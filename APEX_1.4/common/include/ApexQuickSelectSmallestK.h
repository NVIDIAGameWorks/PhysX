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



#ifndef APEX_QUICK_SELECT_SMALLEST_K_H
#define APEX_QUICK_SELECT_SMALLEST_K_H

namespace nvidia
{
namespace apex
{
//A variant of quick sort to move the smallest k members of a sequence to its start.
//Does much less work than a full sort.

template<class Sortable, class Predicate>
PX_INLINE void ApexQuickSelectSmallestK(Sortable* start, Sortable* end, uint32_t k, const Predicate& p = Predicate())
{
	Sortable* origStart = start;
	Sortable* i;
	Sortable* j;
	Sortable m;

	for (;;)
	{
		i = start;
		j = end;
		m = *(i + ((j - i) >> 1));

		while (i <= j)
		{
			while (p(*i, m))
			{
				i++;
			}
			while (p(m, *j))
			{
				j--;
			}
			if (i <= j)
			{
				if (i != j)
				{
					nvidia::swap(*i, *j);
				}
				i++;
				j--;
			}
		}



		if (start < j
		        && k + origStart - 1 < j)	//we now have found the (j - start+1) smallest.  we need to continue sorting these only if k < (j - start+1)
			//if we sort this we definitely won't need to sort the right hand side.
		{
			end = j;
		}
		else if (i < end
		         && k + origStart > i)	//only continue sorting these if left side is not larger than k.
			//we do this instead of recursing
		{
			start = i;
		}
		else
		{
			return;
		}
	}
}

} // namespace apex
} // namespace nvidia

#endif // APEX_QUICK_SELECT_SMALLEST_K_H
