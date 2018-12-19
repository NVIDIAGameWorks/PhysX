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

#ifndef CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P10_0P11H_H
#define CONVERSIONCLOTHINGPHYSICALMESHPARAMETERS_0P10_0P11H_H

#include "NvParamConversionTemplate.h"
#include "ClothingPhysicalMeshParameters_0p10.h"
#include "ClothingPhysicalMeshParameters_0p11.h"

namespace nvidia
{
namespace apex
{
namespace legacy 
{


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ClothingPhysicalMeshParameters_0p10, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p11, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p10::ClassVersion, 
						nvidia::parameterized::ClothingPhysicalMeshParameters_0p11::ClassVersion>
						ConversionClothingPhysicalMeshParameters_0p10_0p11Parent;

class ConversionClothingPhysicalMeshParameters_0p10_0p11: public ConversionClothingPhysicalMeshParameters_0p10_0p11Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingPhysicalMeshParameters_0p10_0p11));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingPhysicalMeshParameters_0p10_0p11)(t) : 0;
	}

protected:
	ConversionClothingPhysicalMeshParameters_0p10_0p11(NvParameterized::Traits* t) : ConversionClothingPhysicalMeshParameters_0p10_0p11Parent(t) {}

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
		if (mLegacyData->submeshes.buf != NULL)
		{
			mNewData->physicalMesh.numSimulatedVertices = mLegacyData->submeshes.buf[0].numVertices;
			mNewData->physicalMesh.numMaxDistance0Vertices = mLegacyData->submeshes.buf[0].numMaxDistance0Vertices;

			mNewData->physicalMesh.numSimulatedIndices = mLegacyData->submeshes.buf[0].numIndices;
		}
		else
		{
			mNewData->physicalMesh.numMaxDistance0Vertices = mLegacyData->physicalMesh.numVertices;
		}

		return true;
	}
};

}
}
} //nvidia::apex::legacy

#endif
