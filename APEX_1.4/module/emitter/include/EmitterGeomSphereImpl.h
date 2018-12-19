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



#ifndef __EMITTER_GEOM_SPHERE_IMPL_H__
#define __EMITTER_GEOM_SPHERE_IMPL_H__

#include "EmitterGeomBase.h"
#include "PsUserAllocated.h"
#include "EmitterGeomSphereParams.h"

namespace NvParameterized
{
class Interface;
};

namespace nvidia
{
namespace emitter
{

class EmitterGeomSphereImpl : public EmitterGeomSphere, public EmitterGeomBase
{
public:
	EmitterGeomSphereImpl(NvParameterized::Interface* params);

	/* Asset callable methods */
	EmitterGeom*				getEmitterGeom();
	const EmitterGeomSphere* 	isSphereGeom() const
	{
		return this;
	}
	EmitterType::Enum		getEmitterType() const
	{
		return mType;
	}
	void						setEmitterType(EmitterType::Enum t);
	void	                    setRadius(float radius)
	{
		*mRadius = radius;
	}
	float	                    getRadius() const
	{
		return *mRadius;
	}
	void	                    setHemisphere(float hemisphere)
	{
		*mHemisphere = hemisphere;
	}
	float	                    getHemisphere() const
	{
		return *mHemisphere;
	}
	void						destroy()
	{
		delete this;
	}

	/* AssetPreview methods */
	void                        drawPreview(float scale, RenderDebugInterface* renderDebug) const;

	/* Actor callable methods */
	void						visualize(const PxTransform& pose, RenderDebugInterface& renderDebug);

	void						computeFillPositions(physx::Array<PxVec3>& positions,
	        physx::Array<PxVec3>& velocities,
	        const PxTransform&,
			const PxVec3&,
	        float,
	        PxBounds3& outBounds,
	        QDSRand& rand) const;

	float				computeEmitterVolume() const;
	PxVec3				randomPosInFullVolume(const PxMat44& pose, QDSRand& rand) const;
	bool						isInEmitter(const PxVec3& pos, const PxMat44& pose) const;

protected:
	EmitterType::Enum		mType;
	float*				mRadius;
	float*				mHemisphere;
	EmitterGeomSphereParams*	mGeomParams;
};

}
} // end namespace nvidia

#endif
