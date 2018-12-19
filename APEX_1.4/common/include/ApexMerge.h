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



#ifndef APEX_MERGE_H
#define APEX_MERGE_H

namespace nvidia
{
namespace apex
{
	// merge 2 increasingly sorted arrays
	// it's ok if one of the input buffers is also the results array
	template<class Sortable>
	bool ApexMerge(Sortable* bufferA, uint32_t numEntriesA, Sortable* bufferB, uint32_t numEntriesB, Sortable* resultBuffer, uint32_t numEntriesResult, int (*compare)(const void*, const void*))
	{
		if (numEntriesResult != numEntriesA + numEntriesB)
			return false;

#if PX_CHECKED
		if (numEntriesA > 0)
		{
			for (uint32_t i = 1; i < numEntriesA; ++i)
			{
				PX_ASSERT(compare(bufferA + i - 1, bufferA + i) <= 0);
			}
		}

		if (numEntriesB > 0)
		{
			for (uint32_t i = 1; i < numEntriesB; ++i)
			{
				PX_ASSERT(compare(bufferB + i - 1, bufferB + i) <= 0);
			}
		}
#endif

		int32_t iA = (int32_t)numEntriesA-1;
		int32_t iB = (int32_t)numEntriesB-1;
		uint32_t iResult = numEntriesA + numEntriesB - 1;

		while (iA >= 0 && iB >= 0)
		{
			if (compare(&bufferA[iA], &bufferB[iB]) > 0)
			{
				resultBuffer[iResult] = bufferA[iA--];
			}
			else
			{
				resultBuffer[iResult] = bufferB[iB--];
			}

			--iResult;
		}

		if (iA < 0)
		{
			if (resultBuffer != bufferB)
			{
				memcpy(resultBuffer, bufferB, (iB + 1) * sizeof(Sortable));
			}
		}
		else
		{
			if (resultBuffer != bufferA)
			{
				memcpy(resultBuffer, bufferA, (iA + 1) * sizeof(Sortable));
			}
		}

		return true;
	}

} // namespace apex
} // namespace nvidia

#endif // APEX_MERGE_H
