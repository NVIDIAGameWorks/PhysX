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



#ifndef __EMITTER_GEOM_BASE_H__
#define __EMITTER_GEOM_BASE_H__

#include "Apex.h"
#include "EmitterGeoms.h"
#include "PsArray.h"
#include "PsUserAllocated.h"
#include <ApexUsingNamespace.h>
#include "ApexRand.h"

namespace nvidia
{
namespace apex
{
class RenderDebugInterface;
}
namespace emitter
{

/* Implementation base class for all EmitterGeom derivations */

class EmitterGeomBase : public UserAllocated
{
public:
	/* Asset callable functions */
	virtual EmitterGeom*				getEmitterGeom() = 0;
	virtual void						destroy() = 0;

	/* ApexEmitterActor runtime access methods */
	virtual float				computeEmitterVolume() const = 0;
	virtual void						computeFillPositions(physx::Array<PxVec3>& positions,
	        physx::Array<PxVec3>& velocities,
	        const PxTransform&,
			const PxVec3&,
	        float,
	        PxBounds3& outBounds,
	        QDSRand& rand) const = 0;

	virtual PxVec3				randomPosInFullVolume(const PxMat44&, QDSRand&) const = 0;

	/* AssetPreview methods */
	virtual void                        drawPreview(float scale, RenderDebugInterface* renderDebug) const = 0;

	/* Optional override functions */
	virtual void						visualize(const PxTransform&, RenderDebugInterface&) { }

	virtual float						computeNewlyCoveredVolume(const PxMat44&, const PxMat44&, float, QDSRand&) const;
	virtual PxVec3				randomPosInNewlyCoveredVolume(const PxMat44&, const PxMat44&, QDSRand&) const;

protected:
	virtual bool						isInEmitter(const PxVec3& pos, const PxMat44& pose) const = 0;
};

}
} // end namespace nvidia

#endif
