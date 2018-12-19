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



#ifndef MODULE_CONVERSIONEMITTERGEOMEXPLICITPARAMS_0P0_0P1H_H
#define MODULE_CONVERSIONEMITTERGEOMEXPLICITPARAMS_0P0_0P1H_H

#include "NvParamConversionTemplate.h"
#include "EmitterGeomExplicitParams_0p0.h"
#include "EmitterGeomExplicitParams_0p1.h"

namespace nvidia {
namespace apex {
namespace legacy {


typedef NvParameterized::ParamConversionTemplate<nvidia::parameterized::EmitterGeomExplicitParams_0p0, 
						nvidia::parameterized::EmitterGeomExplicitParams_0p1, 
						nvidia::parameterized::EmitterGeomExplicitParams_0p0::ClassVersion, 
						nvidia::parameterized::EmitterGeomExplicitParams_0p1::ClassVersion>
						ConversionEmitterGeomExplicitParams_0p0_0p1Parent;

class ConversionEmitterGeomExplicitParams_0p0_0p1: public ConversionEmitterGeomExplicitParams_0p0_0p1Parent
{
public:
	static NvParameterized::Conversion* Create(NvParameterized::Traits* t)
	{
		void* buf = t->alloc(sizeof(ConversionEmitterGeomExplicitParams_0p0_0p1));
		return buf ? PX_PLACEMENT_NEW(buf, ConversionEmitterGeomExplicitParams_0p0_0p1)(t) : 0;
	}

protected:
	ConversionEmitterGeomExplicitParams_0p0_0p1(NvParameterized::Traits* t) : ConversionEmitterGeomExplicitParams_0p0_0p1Parent(t) {}

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


#	define NX_ERR_CHECK_RETURN(x) { if( NvParameterized::ERROR_NONE != (x) ) return false; }

	bool convert()
	{
		NvParameterized::Handle h(*mNewData);
		int32_t size;

		size = mLegacyData->positions.arraySizes[0];
		NX_ERR_CHECK_RETURN(mNewData->getParameterHandle("points.positions", h));
		NX_ERR_CHECK_RETURN(h.resizeArray(size));
		for (int32_t i = 0; i < size; ++i)
		{
			mNewData->points.positions.buf[i].position = mLegacyData->positions.buf[i];
			mNewData->points.positions.buf[i].doDetectOverlaps = false;
		}

		size = mLegacyData->velocities.arraySizes[0];
		NX_ERR_CHECK_RETURN(mNewData->getParameterHandle("points.velocities", h));
		NX_ERR_CHECK_RETURN(h.resizeArray(size));
		NX_ERR_CHECK_RETURN(h.setParamVec3Array(&mLegacyData->velocities.buf[0], size));

		return true;
	}
};


}
}
} //nvidia::apex::legacy

#endif
