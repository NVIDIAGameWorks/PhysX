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



#ifndef MODULE_CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P9_0P10H_H
#define MODULE_CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P9_0P10H_H

#include "NvParamConversionTemplate.h"
#include "ClothingPhysicalMeshParameters_0p9.h"
#include "ClothingPhysicalMeshParameters_0p10.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ClothingPhysicalMeshParameters_0p9, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p10, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p9::ClassVersion, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p10::ClassVersion>
						ConversionClothingPhysicalMeshParameters_0p9_0p10Parent;

class ConversionClothingPhysicalMeshParameters_0p9_0p10: public ConversionClothingPhysicalMeshParameters_0p9_0p10Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingPhysicalMeshParameters_0p9_0p10));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingPhysicalMeshParameters_0p9_0p10)(t) : 0;
	}

protected:
	ConversionClothingPhysicalMeshParameters_0p9_0p10(NvParameterized::Traits* t) : ConversionClothingPhysicalMeshParameters_0p9_0p10Parent(t) {}

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

		float meshThickness = mLegacyData->transitionDownThickness;
		NvParameterized::Handle transitionDownHandle(*mNewData, "transitionDown");
		transitionDownHandle.resizeArray(mLegacyData->transitionDownC.arraySizes[0]);
		for (int32_t i = 0; i < mLegacyData->transitionDownC.arraySizes[0]; ++i)
		{
			const parameterized::ClothingPhysicalMeshParameters_0p9NS::SkinClothMapC_Type& mapC = mLegacyData->transitionDownC.buf[i];
			parameterized::ClothingPhysicalMeshParameters_0p10NS::SkinClothMapD_Type& mapD = mNewData->transitionDown.buf[i];

			mapD.vertexBary = mapC.vertexBary;
			mapD.vertexBary.z *= meshThickness;
			mapD.normalBary = mapC.normalBary;
			mapD.normalBary.z *= meshThickness;
			mapD.tangentBary = physx::PxVec3(PX_MAX_F32); // mark tangents as invalid
			mapD.vertexIndexPlusOffset = mapC.vertexIndexPlusOffset;

			// temporarily store face index to update in the ClothingAsset update
			mapD.vertexIndex0 = mapC.faceIndex0;
		}



		meshThickness = mLegacyData->transitionUpThickness;
		NvParameterized::Handle transitionUpHandle(*mNewData, "transitionUp");
		transitionUpHandle.resizeArray(mLegacyData->transitionUpC.arraySizes[0]);
		for (int32_t i = 0; i < mLegacyData->transitionUpC.arraySizes[0]; ++i)
		{
			const parameterized::ClothingPhysicalMeshParameters_0p9NS::SkinClothMapC_Type& mapC = mLegacyData->transitionUpC.buf[i];
			parameterized::ClothingPhysicalMeshParameters_0p10NS::SkinClothMapD_Type& mapD = mNewData->transitionUp.buf[i];

			mapD.vertexBary = mapC.vertexBary;
			mapD.vertexBary.z *= meshThickness;
			mapD.normalBary = mapC.normalBary;
			mapD.normalBary.z *= meshThickness;
			mapD.tangentBary = physx::PxVec3(PX_MAX_F32); // mark tangents as invalid
			mapD.vertexIndexPlusOffset = mapC.vertexIndexPlusOffset;

			// temporarily store face index to update in the ClothingAsset update
			mapD.vertexIndex0 = mapC.faceIndex0;
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
