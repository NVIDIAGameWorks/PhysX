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



#ifndef MODULE_CONVERSIONGROUNDEMITTERASSETPARAMETERS_0P1_0P2H_H
#define MODULE_CONVERSIONGROUNDEMITTERASSETPARAMETERS_0P1_0P2H_H

#include "NvParamConversionTemplate.h"
#include "GroundEmitterAssetParameters_0p1.h"
#include "GroundEmitterAssetParameters_0p2.h"

#include "nvparameterized/NvParamUtils.h"

#define PARAM_RET(x) if( (x) != NvParameterized::ERROR_NONE ) \
						{ PX_ASSERT(0 && "INVALID Parameter"); return false; }

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::GroundEmitterAssetParameters_0p1, 
						nvidia::parameterized::GroundEmitterAssetParameters_0p2, 
						nvidia::parameterized::GroundEmitterAssetParameters_0p1::ClassVersion, 
						nvidia::parameterized::GroundEmitterAssetParameters_0p2::ClassVersion>
						ConversionGroundEmitterAssetParameters_0p1_0p2Parent;

class ConversionGroundEmitterAssetParameters_0p1_0p2: public ConversionGroundEmitterAssetParameters_0p1_0p2Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionGroundEmitterAssetParameters_0p1_0p2));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionGroundEmitterAssetParameters_0p1_0p2)(t) : 0;
	}

protected:
	ConversionGroundEmitterAssetParameters_0p1_0p2(NvParameterized::Traits* t) : ConversionGroundEmitterAssetParameters_0p1_0p2Parent(t) {}

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
		//TODO:
		//	Write custom conversion code here using mNewData and mLegacyData members.
		//
		//	Note that
		//		- mNewData has already been initialized with default values
		//		- same-named/same-typed members have already been copied
		//			from mLegacyData to mNewData
		//		- included references were moved to mNewData
		//			(and updated to preferred versions according to getPreferredVersions)
		//
		//	For more info see the versioning wiki.

		NvParameterized::Handle hOld(*mLegacyData, "materialFactoryMapList");
		PX_ASSERT(hOld.isValid());

		NvParameterized::Handle hNew(*mNewData, "materialFactoryMapList");
		PX_ASSERT(hNew.isValid());

		for (int32_t i = 0; i < mLegacyData->materialFactoryMapList.arraySizes[0]; i++)
		{
			hOld.set(i);
			hNew.set(i);

			NvParameterized::Handle hChild(mLegacyData);
			PARAM_RET(hOld.getChildHandle(mLegacyData, "iosAssetName", hChild));
			PX_ASSERT(hOld.isValid());

			NvParameterized::Interface* oldRef = NULL;

			PARAM_RET(hChild.getParamRef(oldRef));
			PARAM_RET(hChild.setParamRef(0));

			if (oldRef)
			{
				if (!physx::shdfnd::strcmp("NxBasicIosAsset", oldRef->className()))
				{
					oldRef->setClassName("BasicIosAsset");
				}
				PARAM_RET(hNew.getChildHandle(mNewData, "iosAssetName", hChild));
				PARAM_RET(hChild.setParamRef(oldRef));
			}
		
			hNew.popIndex();
			hOld.popIndex();
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
