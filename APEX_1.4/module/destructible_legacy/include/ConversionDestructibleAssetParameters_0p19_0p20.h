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



#ifndef MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P19_0P20H_H
#define MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P19_0P20H_H

#include "NvParamConversionTemplate.h"
#include "DestructibleAssetParameters_0p19.h"
#include "DestructibleAssetParameters_0p20.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::DestructibleAssetParameters_0p19, 
						nvidia::parameterized::DestructibleAssetParameters_0p20, 
						nvidia::parameterized::DestructibleAssetParameters_0p19::ClassVersion, 
						nvidia::parameterized::DestructibleAssetParameters_0p20::ClassVersion>
						ConversionDestructibleAssetParameters_0p19_0p20Parent;

class ConversionDestructibleAssetParameters_0p19_0p20: public ConversionDestructibleAssetParameters_0p19_0p20Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleAssetParameters_0p19_0p20));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleAssetParameters_0p19_0p20)(t) : 0;
	}

protected:
	ConversionDestructibleAssetParameters_0p19_0p20(NvParameterized::Traits* t) : ConversionDestructibleAssetParameters_0p19_0p20Parent(t) {}

	const NvParameterized::PrefVer* getPreferredVersions() const
	{
		static NvParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (uint32_t)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		// Convert to new behaviorGroup index convention

		const int32_t chunkCount = mNewData->chunks.arraySizes[0];
		for (int32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			parameterized::DestructibleAssetParameters_0p20NS::Chunk_Type& chunk = mNewData->chunks.buf[chunkIndex];
			if (chunk.parentIndex < chunkCount && chunk.behaviorGroupIndex < 0)
			{
				chunk.behaviorGroupIndex = mNewData->chunks.buf[chunk.parentIndex].behaviorGroupIndex;
			}
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
