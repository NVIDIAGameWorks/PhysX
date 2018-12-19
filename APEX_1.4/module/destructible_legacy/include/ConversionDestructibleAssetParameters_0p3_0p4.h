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



#ifndef MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P3_0P4H_H
#define MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P3_0P4H_H

#include "NvParamConversionTemplate.h"
#include "DestructibleAssetParameters_0p3.h"
#include "DestructibleAssetParameters_0p4.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::DestructibleAssetParameters_0p3, 
						nvidia::parameterized::DestructibleAssetParameters_0p4, 
						nvidia::parameterized::DestructibleAssetParameters_0p3::ClassVersion, 
						nvidia::parameterized::DestructibleAssetParameters_0p4::ClassVersion>
						ConversionDestructibleAssetParameters_0p3_0p4Parent;

class ConversionDestructibleAssetParameters_0p3_0p4: public ConversionDestructibleAssetParameters_0p3_0p4Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleAssetParameters_0p3_0p4));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleAssetParameters_0p3_0p4)(t) : 0;
	}

protected:
	ConversionDestructibleAssetParameters_0p3_0p4(NvParameterized::Traits* t) : ConversionDestructibleAssetParameters_0p3_0p4Parent(t) {}

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
		NvParameterized::Handle handle(*mNewData);
		mNewData->getParameterHandle("chunkConvexHullStartIndices", handle);
		mNewData->resizeArray(handle, mNewData->chunks.arraySizes[0] + 1);
		for (int32_t i = 0; i <= mNewData->chunks.arraySizes[0]; ++i)
		{
			mNewData->chunkConvexHullStartIndices.buf[i] = (uint32_t)i;
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
