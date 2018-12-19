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



#ifndef EMITTER_LOD_PARAM_DESC_H
#define EMITTER_LOD_PARAM_DESC_H

#include "Apex.h"

namespace nvidia
{
namespace apex
{


PX_PUSH_PACK_DEFAULT

///LOD parameters fro emitters
class EmitterLodParamDesc : public ApexDesc
{
public:
	float  maxDistance;        ///< Objects greater than this distance from the player will be culled more aggressively
	float  distanceWeight;     ///< Weight given to distance parameter in LOD function
	float  speedWeight;        ///< Weight given to velocity parameter in LOD function
	float  lifeWeight;         ///< Weight given to life remain parameter in LOD function
	float  separationWeight;   ///< Weight given to separation parameter in LOD function
	float  bias;               ///< Bias given to objects spawned by this emitter, relative to other emitters in the same IOS

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE EmitterLodParamDesc() : ApexDesc()
	{
		init();
	}

	/**
	\brief sets members to default values.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();
		init();
	}

	/**
	\brief checks if this is a valid descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		if (distanceWeight < 0.0f || speedWeight < 0.0f || lifeWeight < 0.0f)
		{
			return false;
		}
		if (separationWeight < 0.0f || maxDistance < 0.0f || bias < 0.0f)
		{
			return false;
		}
		return ApexDesc::isValid();
	}

	/**
	\brief enum of manifest versions
	*/
	enum ManifestVersions
	{
		initial,

		count,
		current = count - 1
	};

private:

	PX_INLINE void init()
	{
		// These defaults give you pure distance based LOD weighting
		maxDistance = 0.0f;
		distanceWeight = 1.0f;
		speedWeight = 0.0f;
		lifeWeight = 0.0f;
		separationWeight = 0.0f;
		bias = 1.0f;
	}
};


PX_POP_PACK

}
} // end namespace nvidia

#endif // EMITTER_LOD_PARAM_DESC_H
