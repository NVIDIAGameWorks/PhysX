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



#ifndef MODULE_CONVERSIONCONVEXHULLPARAMETERS_0P0_0P1H_H
#define MODULE_CONVERSIONCONVEXHULLPARAMETERS_0P0_0P1H_H

#include "NvParamConversionTemplate.h"
#include "ConvexHullParameters_0p0.h"
#include "ConvexHullParameters_0p1.h"

#include "ApexSharedUtils.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::ConvexHullParameters_0p0, 
						nvidia::parameterized::ConvexHullParameters_0p1, 
						nvidia::parameterized::ConvexHullParameters_0p0::ClassVersion, 
						nvidia::parameterized::ConvexHullParameters_0p1::ClassVersion>
						ConversionConvexHullParameters_0p0_0p1Parent;

class ConversionConvexHullParameters_0p0_0p1: public ConversionConvexHullParameters_0p0_0p1Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionConvexHullParameters_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionConvexHullParameters_0p0_0p1)(t) : 0;
	}

protected:
	ConversionConvexHullParameters_0p0_0p1(NvParameterized::Traits* t) : ConversionConvexHullParameters_0p0_0p1Parent(t) {}

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
		// The 0.0 format does not have the adjacentFaces array.  We will simply rebuild the convex hull from the
		// old version's vertices.

		nvidia::ConvexHullImpl hull;
		hull.init();

		hull.buildFromPoints(mLegacyData->vertices.buf, (uint32_t)mLegacyData->vertices.arraySizes[0], (uint32_t)mLegacyData->vertices.elementSize);
		mNewData->copy(*hull.mParams);

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
