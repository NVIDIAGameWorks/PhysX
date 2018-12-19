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

#ifndef PT_PARTICLE_SYSTEM_SIM_H
#define PT_PARTICLE_SYSTEM_SIM_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "CmPhysXCommon.h"
#include "PtParticleSystemCore.h"
#include "PtParticleShape.h"

namespace physx
{

class PxParticleDeviceExclusiveAccess;

namespace Pt
{

struct ParticleSystemSimDataDesc;
class ParticleSystemSim;

/*!
\file
ParticleSystemSim interface.
*/

/************************************************************************/
/* ParticleSystemsSim                                                      */
/************************************************************************/

/**
Descriptor for the batched shape update pipeline stage
*/
struct ParticleShapesUpdateInput
{
	ParticleShape** shapes;
	PxU32 shapeCount;
};

/**
Descriptor for the batched collision update pipeline stage
*/
struct ParticleCollisionUpdateInput
{
	PxU8* contactManagerStream;
};

/*!
Descriptor for updated particle packet shapes
*/
struct ParticleShapeUpdateResults
{
	ParticleShape* const* createdShapes;   //! Handles of newly created particle packet shapes
	PxU32 createdShapeCount;               //! Number of newly created particle packet shapes
	ParticleShape* const* destroyedShapes; //! Handles of particle packet shapes to delete
	PxU32 destroyedShapeCount;             //! Number of particle packet shapes to delete
};

class ParticleSystemSim
{
  public:
	virtual ParticleSystemState& getParticleStateV() = 0;
	virtual void getSimParticleDataV(ParticleSystemSimDataDesc& simParticleData, bool devicePtr) const = 0;
	virtual void getShapesUpdateV(ParticleShapeUpdateResults& updateResults) const = 0;

	virtual void setExternalAccelerationV(const PxVec3& v) = 0;
	virtual const PxVec3& getExternalAccelerationV() const = 0;

	virtual void setSimulationTimeStepV(PxReal value) = 0;
	virtual PxReal getSimulationTimeStepV() const = 0;

	virtual void setSimulatedV(bool) = 0;
	virtual Ps::IntBool isSimulatedV() const = 0;

	// gpuBuffer specifies that the interaction was created asynchronously to gpu execution (for rb ccd)
	virtual void addInteractionV(const ParticleShape& particleShape, ShapeHandle shape, BodyHandle body, bool isDynamic,
	                             bool gpuBuffer) = 0;

	// gpuBuffer specifies that the interaction was created asynchronously to gpu execution (for rb ccd)
	virtual void removeInteractionV(const ParticleShape& particleShape, ShapeHandle shape, BodyHandle body,
	                                bool isDynamic, bool isDyingRb, bool gpuBuffer) = 0;

	virtual void onRbShapeChangeV(const ParticleShape& particleShape, ShapeHandle shape) = 0;

	// applies the buffered interaction updates.
	virtual void flushBufferedInteractionUpdatesV() = 0;

	// passes the contact manager stream needed for collision - the callee is responsible for releasing it
	virtual void passCollisionInputV(ParticleCollisionUpdateInput input) = 0;

#if PX_SUPPORT_GPU_PHYSX
	virtual Ps::IntBool isGpuV() const = 0;
	virtual void enableDeviceExclusiveModeGpuV() = 0;
	virtual PxParticleDeviceExclusiveAccess* getDeviceExclusiveAccessGpuV() const = 0;
#endif

  protected:
	virtual ~ParticleSystemSim()
	{
	}
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_SYSTEM_SIM_H
