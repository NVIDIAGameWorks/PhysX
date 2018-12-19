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



#ifndef MODULE_EMITTER_H
#define MODULE_EMITTER_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{


PX_PUSH_PACK_DEFAULT

class EmitterAsset;
class EmitterAssetAuthoring;

class GroundEmitterAsset;
class GroundEmitterAssetAuthoring;

class ImpactEmitterAsset;
class ImpactEmitterAssetAuthoring;


/**
\brief An APEX Module that provides generic Emitter classes
*/
class ModuleEmitter : public Module
{
protected:
	virtual ~ModuleEmitter() {}

public:

	/// get rate scale. Rate parameter in all emitters will be multiplied by rate scale.
	virtual float 	getRateScale() const = 0;
	
	/// get density scale. Density parameter in all emitters except ground emitters will be multiplied by density scale.
	virtual float 	getDensityScale() const = 0;
	
	/// get ground density scale. Density parameter in all ground emitters will be multiplied by ground density scale.
	virtual float 	getGroundDensityScale() const = 0;

	/// set rate scale. Rate parameter in all module emitters will be multiplied by rate scale.
	virtual void 	setRateScale(float rateScale) = 0;

	/// set density scale. Density parameter in all emitters except ground emitters will be multiplied by density scale.
	virtual void 	setDensityScale(float densityScale) = 0;

	/// set ground density scale. Density parameter in all ground emitters will be multiplied by ground density scale.
	virtual void 	setGroundDensityScale(float groundDensityScale) = 0;
};



PX_POP_PACK

}
} // end namespace nvidia

#endif // MODULE_EMITTER_H
