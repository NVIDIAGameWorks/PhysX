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



#ifndef MODULE_CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P0_0P1H_H
#define MODULE_CONVERSIONCLOTHINGGRAPHICALLODPARAMETERS_0P0_0P1H_H

#include "NvParamConversionTemplate.h"
#include "ClothingGraphicalLodParameters_0p0.h"
#include "ClothingGraphicalLodParameters_0p1.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ClothingGraphicalLodParameters_0p0, 
						nvidia::parameterized::ClothingGraphicalLodParameters_0p1, 
						nvidia::parameterized::ClothingGraphicalLodParameters_0p0::ClassVersion, 
						nvidia::parameterized::ClothingGraphicalLodParameters_0p1::ClassVersion>
						ConversionClothingGraphicalLodParameters_0p0_0p1Parent;

class ConversionClothingGraphicalLodParameters_0p0_0p1: public ConversionClothingGraphicalLodParameters_0p0_0p1Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionClothingGraphicalLodParameters_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionClothingGraphicalLodParameters_0p0_0p1)(t) : 0;
	}

protected:
	ConversionClothingGraphicalLodParameters_0p0_0p1(NvParameterized::Traits* t) : ConversionClothingGraphicalLodParameters_0p0_0p1Parent(t) {}

	bool convert()
	{
		//TODO:
		//	write custom conversion code here using mNewData and mLegacyData members
		//	note that members with same names were already copied in parent converter
		// and mNewData was already initialized with default values

		mNewData->renderMeshAssetPointer = NULL;

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
