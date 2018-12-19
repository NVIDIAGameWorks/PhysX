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



#ifndef MODULE_CONVERSIONAPEXEMITTERASSETPARAMETERS_0P6_0P7H_H
#define MODULE_CONVERSIONAPEXEMITTERASSETPARAMETERS_0P6_0P7H_H

#include "NvParamConversionTemplate.h"
#include "ApexEmitterAssetParameters_0p6.h"
#include "ApexEmitterAssetParameters_0p7.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ApexEmitterAssetParameters_0p6, 
						nvidia::parameterized::ApexEmitterAssetParameters_0p7, 
						nvidia::parameterized::ApexEmitterAssetParameters_0p6::ClassVersion, 
						nvidia::parameterized::ApexEmitterAssetParameters_0p7::ClassVersion>
						ConversionApexEmitterAssetParameters_0p6_0p7Parent;

class ConversionApexEmitterAssetParameters_0p6_0p7: public ConversionApexEmitterAssetParameters_0p6_0p7Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionApexEmitterAssetParameters_0p6_0p7));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionApexEmitterAssetParameters_0p6_0p7)(t) : 0;
	}

protected:
	ConversionApexEmitterAssetParameters_0p6_0p7(NvParameterized::Traits* t) : ConversionApexEmitterAssetParameters_0p6_0p7Parent(t) {}

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
		mNewData->density = mLegacyData->densityRange.max;
		mNewData->rate = mLegacyData->rateRange.max;
		mNewData->velocityLow = mLegacyData->velocityRange.min;
		mNewData->velocityHigh = mLegacyData->velocityRange.max;
		mNewData->lifetimeLow = mLegacyData->lifetimeRange.min;
		mNewData->lifetimeHigh = mLegacyData->lifetimeRange.max;

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
