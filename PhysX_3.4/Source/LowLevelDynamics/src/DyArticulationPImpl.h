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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  



#ifndef DY_ARTICULATION_INTERFACE_H
#define DY_ARTICULATION_INTERFACE_H

#include "DyArticulationUtils.h"

namespace physx
{

class PxcConstraintBlockStream;
class PxcScratchAllocator;
class PxsConstraintBlockManager;
struct PxSolverConstraintDesc;

namespace Dy
{
	
	struct ArticulationSolverDesc;


class ArticulationPImpl
{
public:

	typedef PxU32 (*ComputeUnconstrainedVelocitiesFn)(const ArticulationSolverDesc& desc,
													 PxReal dt,
													 PxcConstraintBlockStream& stream,
													 PxSolverConstraintDesc* constraintDesc,
													 PxU32& acCount,
													 PxsConstraintBlockManager& constraintBlockManager,
													 const PxVec3& gravity, PxU64 contextID);

	typedef void (*UpdateBodiesFn)(const ArticulationSolverDesc& desc,
								   PxReal dt);

	typedef void (*SaveVelocityFn)(const ArticulationSolverDesc &m);

	static ComputeUnconstrainedVelocitiesFn sComputeUnconstrainedVelocities;
	static UpdateBodiesFn sUpdateBodies;
	static SaveVelocityFn sSaveVelocity;

	static PxU32 computeUnconstrainedVelocities(const ArticulationSolverDesc& desc,
										   PxReal dt,
										   PxcConstraintBlockStream& stream,
										   PxSolverConstraintDesc* constraintDesc,
										   PxU32& acCount,
										   PxcScratchAllocator&,
										   PxsConstraintBlockManager& constraintBlockManager,
										   const PxVec3& gravity, PxU64 contextID)
	{
		PX_ASSERT(sComputeUnconstrainedVelocities);
		if(sComputeUnconstrainedVelocities)
			return (sComputeUnconstrainedVelocities)(desc, dt, stream, constraintDesc, acCount, constraintBlockManager, gravity, contextID);
		else
			return 0;
	}

	static void	updateBodies(const ArticulationSolverDesc& desc,
						 PxReal dt)
	{
		PX_ASSERT(sUpdateBodies);
		if(sUpdateBodies)
			(*sUpdateBodies)(desc, dt);
	}

	static void	saveVelocity(const ArticulationSolverDesc& desc)
	{
		PX_ASSERT(sSaveVelocity);
		if(sSaveVelocity)
			(*sSaveVelocity)(desc);
	}
};


}
}
#endif //DY_ARTICULATION_INTERFACE_H

