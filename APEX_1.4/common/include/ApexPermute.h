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



#ifndef APEX_PERMUTE_H
#define APEX_PERMUTE_H

namespace nvidia
{
namespace apex
{

// permutationBuffer has to contain the indices that map from the new to the old index
template<class Sortable>
inline void ApexPermute(Sortable* sortBuffer, const uint32_t* permutationBuffer, uint32_t numElements, uint32_t numElementsPerPermutation = 1)
{
	nvidia::Array<Sortable> temp;
	temp.resize(numElementsPerPermutation);

	// TODO remove used buffer
	nvidia::Array<bool> used(numElements, false);

	for (uint32_t i = 0; i < numElements; i++)
	{
		//if (permutationBuffer[i] == (uint32_t)-1 || permutationBuffer[i] == i)
		if (used[i] || permutationBuffer[i] == i)
		{
			continue;
		}

		uint32_t dst = i;
		uint32_t src = permutationBuffer[i];
		for (uint32_t j = 0; j < numElementsPerPermutation; j++)
		{
			temp[j] = sortBuffer[numElementsPerPermutation * dst + j];
		}
		do
		{
			for (uint32_t j = 0; j < numElementsPerPermutation; j++)
			{
				sortBuffer[numElementsPerPermutation * dst + j] = sortBuffer[numElementsPerPermutation * src + j];
			}
			//permutationBuffer[dst] = (uint32_t)-1;
			used[dst] = true;
			dst = src;
			src = permutationBuffer[src];
			//} while (permutationBuffer[src] != (uint32_t)-1);
		}
		while (!used[src]);
		for (uint32_t j = 0; j < numElementsPerPermutation; j++)
		{
			sortBuffer[numElementsPerPermutation * dst + j] = temp[j];
		}
		//permutationBuffer[dst] = (uint32_t)-1;
		used[dst] = true;
	}
}

} // namespace apex
} // namespace nvidia

#endif // APEX_PERMUTE_H
