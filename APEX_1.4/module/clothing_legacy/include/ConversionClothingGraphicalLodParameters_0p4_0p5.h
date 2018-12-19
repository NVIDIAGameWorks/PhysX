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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P4_0P5H_H
#define CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P4_0P5H_H

#include "NvParamConversionTemplate.h"
#include "ClothingGraphicalLodParameters_0p4.h"
#include "ClothingGraphicalLodParameters_0p5.h"

namespace nvidia
{
namespace apex
{
namespace legacy 
{

typedef NvParameterized::ParamConversionTemplate<parameterized::ClothingGraphicalLodParameters_0p4, 
						parameterized::ClothingGraphicalLodParameters_0p5, 
						nvidia::parameterized::ClothingGraphicalLodParameters_0p4::ClassVersion, 
						nvidia::parameterized::ClothingGraphicalLodParameters_0p5::ClassVersion>
						ConversionClothingGraphicalLodParameters_0p4_0p5Parent;

class ConversionClothingGraphicalLodParameters_0p4_0p5: public ConversionClothingGraphicalLodParameters_0p4_0p5Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingGraphicalLodParameters_0p4_0p5));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingGraphicalLodParameters_0p4_0p5)(t) : 0;
	}

protected:
	ConversionClothingGraphicalLodParameters_0p4_0p5(NvParameterized::Traits* t) : ConversionClothingGraphicalLodParameters_0p4_0p5Parent(t) {}

	const NvParameterized::PrefVer* getPreferredVersions() const
	{
		static NvParameterized::PrefVer prefVers[] =
		{
			//TODO:
			//	Add your preferred versions for included references here.
			//	Entry format is
			//		{ (const char*)longName, (PxU32)preferredVersion }

			{ 0, 0 } // Terminator (do not remove!)
		};

		return prefVers;
	}

	bool convert()
	{
		int32_t partNum = mLegacyData->physicsSubmeshPartitioning.arraySizes[0];

		NvParameterized::Handle physicsMeshPartitioningHandle(*mNewData, "physicsMeshPartitioning");
		PX_ASSERT(physicsMeshPartitioningHandle.isValid());
		physicsMeshPartitioningHandle.resizeArray((int32_t)(partNum));

		for (int32_t p = 0; p < partNum; p++)
		{
			mNewData->physicsMeshPartitioning.buf[p].graphicalSubmesh = mLegacyData->physicsSubmeshPartitioning.buf[p].graphicalSubmesh;
			mNewData->physicsMeshPartitioning.buf[p].numSimulatedIndices = mLegacyData->physicsSubmeshPartitioning.buf[p].numSimulatedIndices;
			mNewData->physicsMeshPartitioning.buf[p].numSimulatedVertices = mLegacyData->physicsSubmeshPartitioning.buf[p].numSimulatedVertices;
			mNewData->physicsMeshPartitioning.buf[p].numSimulatedVerticesAdditional = mLegacyData->physicsSubmeshPartitioning.buf[p].numSimulatedVerticesAdditional;
		}

		return true;
	}
};

}
}
} //nvidia::apex::legacy

#endif
