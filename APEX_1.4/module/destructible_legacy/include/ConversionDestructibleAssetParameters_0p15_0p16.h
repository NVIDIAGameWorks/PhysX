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



#ifndef MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P15_0P16H_H
#define MODULE_CONVERSIONDESTRUCTIBLEASSETPARAMETERS_0P15_0P16H_H

#include "NvParamConversionTemplate.h"
#include "DestructibleAssetParameters_0p15.h"
#include "DestructibleAssetParameters_0p16.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::DestructibleAssetParameters_0p15, 
						nvidia::parameterized::DestructibleAssetParameters_0p16, 
						nvidia::parameterized::DestructibleAssetParameters_0p15::ClassVersion, 
						nvidia::parameterized::DestructibleAssetParameters_0p16::ClassVersion>
						ConversionDestructibleAssetParameters_0p15_0p16Parent;

class ConversionDestructibleAssetParameters_0p15_0p16: public ConversionDestructibleAssetParameters_0p15_0p16Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionDestructibleAssetParameters_0p15_0p16));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionDestructibleAssetParameters_0p15_0p16)(t) : 0;
	}

protected:
	ConversionDestructibleAssetParameters_0p15_0p16(NvParameterized::Traits* t) : ConversionDestructibleAssetParameters_0p15_0p16Parent(t) {}

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
		// Convert default behavior group's damage spread function parameters
		mNewData->defaultBehaviorGroup.damageSpread.minimumRadius = mLegacyData->defaultBehaviorGroup.minimumDamageRadius;
		mNewData->defaultBehaviorGroup.damageSpread.radiusMultiplier = mLegacyData->defaultBehaviorGroup.damageRadiusMultiplier;
		mNewData->defaultBehaviorGroup.damageSpread.falloffExponent = mLegacyData->defaultBehaviorGroup.damageFalloffExponent;

		// Convert user-defined behavior group's damage spread function parameters
		const int32_t behaviorGroupCount = mLegacyData->behaviorGroups.arraySizes[0];
		PX_ASSERT(mNewData->behaviorGroups.arraySizes[0] == behaviorGroupCount);
		for (int32_t i = 0; i < physx::PxMin(behaviorGroupCount, mNewData->behaviorGroups.arraySizes[0]); ++i)
		{
			mNewData->behaviorGroups.buf[i].damageSpread.minimumRadius = mLegacyData->behaviorGroups.buf[i].minimumDamageRadius;
			mNewData->behaviorGroups.buf[i].damageSpread.radiusMultiplier = mLegacyData->behaviorGroups.buf[i].damageRadiusMultiplier;
			mNewData->behaviorGroups.buf[i].damageSpread.falloffExponent = mLegacyData->behaviorGroups.buf[i].damageFalloffExponent;
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
