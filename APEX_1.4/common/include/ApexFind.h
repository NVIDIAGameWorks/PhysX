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



#ifndef APEX_FIND_H
#define APEX_FIND_H

namespace nvidia
{
namespace apex
{
	// binary search
	template<class Sortable>
	int32_t ApexFind(const Sortable* buffer, uint32_t numEntries, const Sortable& element, int (*compare)(const void*, const void*))
	{

#if PX_CHECKED
		if (numEntries > 0)
		{
			for (uint32_t i = 1; i < numEntries; ++i)
			{
				PX_ASSERT(compare(buffer + i - 1, buffer + i) <= 0);
			}
		}
#endif

		int32_t curMin = 0;
		int32_t curMax = (int32_t)numEntries;
		int32_t testIndex = 0;

		while (curMin < curMax)
		{
			testIndex = (curMin + curMax) / 2;
			int32_t compResult = compare(&element, buffer+testIndex);
			if (compResult < 0)
			{
				curMax = testIndex;
			}
			else if (compResult > 0)
			{
				curMin = testIndex;
			}
			else
			{
				return testIndex;
			}

		}

		return -1;
	}

} // namespace apex
} // namespace nvidia

#endif // APEX_FIND_H
