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



#ifndef MODULE_CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P5_0P6H_H
#define MODULE_CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P5_0P6H_H

#include "NvParamConversionTemplate.h"
#include "ClothingPhysicalMeshParameters_0p5.h"
#include "ClothingPhysicalMeshParameters_0p6.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ClothingPhysicalMeshParameters_0p5, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p6, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p5::ClassVersion, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p6::ClassVersion>
						ConversionClothingPhysicalMeshParameters_0p5_0p6Parent;

class ConversionClothingPhysicalMeshParameters_0p5_0p6: public ConversionClothingPhysicalMeshParameters_0p5_0p6Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingPhysicalMeshParameters_0p5_0p6));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingPhysicalMeshParameters_0p5_0p6)(t) : 0;
	}

protected:
	ConversionClothingPhysicalMeshParameters_0p5_0p6(NvParameterized::Traits* t) : ConversionClothingPhysicalMeshParameters_0p5_0p6Parent(t) {}

	const NvParameterized::PrefVer* getPreferredVersions() const
	{
		static NvParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char *)longName, (uint32_t)preferredVersion }

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
		//	For more info see the versioning wiki

		const int32_t numVertices = mLegacyData->physicalMesh.vertices.arraySizes[0];
		const uint32_t numBonesPerVertex = mLegacyData->physicalMesh.numBonesPerVertex;

		const float* boneWeights = mNewData->physicalMesh.boneWeights.buf;

		uint32_t numVerticesPerCacheBlock = 8;// 128 / sizeof boneWeights per vertex (4 float), this is the biggest per vertex data
		uint32_t allocNumVertices = ((uint32_t)ceil((float)numVertices / numVerticesPerCacheBlock)) * numVerticesPerCacheBlock; // allocate more to have a multiple of numVerticesPerCachBlock

		NvParameterized::Handle optimizationDataHandle(*mNewData, "physicalMesh.optimizationData");
		PX_ASSERT(optimizationDataHandle.isValid());
		optimizationDataHandle.resizeArray((int32_t)(allocNumVertices + 1) / 2);
		uint8_t* optimizationData = mNewData->physicalMesh.optimizationData.buf;
		memset(optimizationData, 0, (allocNumVertices + 1) / 2);

		const parameterized::ClothingPhysicalMeshParameters_0p6NS::ConstrainCoefficient_Type* constrainCoeffs = mNewData->physicalMesh.constrainCoefficients.buf;

		for (int32_t i = 0; i < numVertices; ++i)
		{
			uint8_t numBones = 0;
			while (numBones < numBonesPerVertex)
			{
				if (boneWeights[numBonesPerVertex * i + numBones] == 0.0f)
				{
					break;
				}

				++numBones;
			}


			uint8_t& data = optimizationData[i / 2];
			PX_ASSERT(numBones < 8);
			uint8_t bitShift = 0;
			if (i % 2 == 0)
			{
				data = 0;
			}
			else
			{
				bitShift = 4;
			}
			data |= numBones << bitShift;

			// store for each vertex if collisionSphereDistance is < 0
			if (constrainCoeffs[i].collisionSphereDistance < 0.0f)
			{
				data |= 8 << bitShift;
				mNewData->physicalMesh.hasNegativeBackstop = true;
			}
			else
			{
				data &= ~(8 << bitShift);
			}
		}

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
