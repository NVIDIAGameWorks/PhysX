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


#ifndef PXD_ARTICULATION_H
#define PXD_ARTICULATION_H

#include "foundation/PxVec3.h"
#include "foundation/PxQuat.h"
#include "foundation/PxTransform.h"
#include "PsVecMath.h"
#include "CmUtils.h"
#include "CmSpatialVector.h"
#include "PxArticulationJoint.h"

namespace physx
{

class PxcRigidBody;
struct PxsBodyCore;

namespace Sc
{
	class ArticulationSim;
}


#define DY_DEBUG_ARTICULATION 0


namespace Dy
{
static const size_t DY_ARTICULATION_MAX_SIZE = 64;
struct ArticulationJointTransforms;
struct FsInertia;
struct FsData;
struct ArticulationCore
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================

	PxU32		internalDriveIterations;
	PxU32		externalDriveIterations;
	PxU32		maxProjectionIterations;
	PxU16		solverIterationCounts; //KS - made a U16 so that it matches PxsRigidCore
	PxReal		separationTolerance;
	PxReal		sleepThreshold;
	PxReal		freezeThreshold;
	PxReal		wakeCounter;
};

struct ArticulationJointCore
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================

	// attachment points
	PxTransform		parentPose;
	PxTransform		childPose;

	// drive model
	PxQuat			targetPosition;
	PxVec3			targetVelocity; 

	PxReal			spring;
	PxReal			damping;

	PxReal			solverSpring;
	PxReal			solverDamping;

	PxReal			internalCompliance;
	PxReal			externalCompliance;

	// limit model

	PxReal			swingYLimit;
	PxReal			swingZLimit;
	PxReal			swingLimitContactDistance;
	bool			swingLimited;

	PxU8			driveType;

	PxReal			tangentialStiffness;
	PxReal			tangentialDamping;

	PxReal			twistLimitHigh;
	PxReal			twistLimitLow;
	PxReal			twistLimitContactDistance;
	bool			twistLimited;

	PxReal			tanQSwingY;
	PxReal			tanQSwingZ;
	PxReal			tanQSwingPad;
	PxReal			tanQTwistHigh;
	PxReal			tanQTwistLow;
	PxReal			tanQTwistPad;

	ArticulationJointCore()
	{
		Cm::markSerializedMem(this, sizeof(ArticulationJointCore));
		parentPose = PxTransform(PxIdentity);
		childPose = PxTransform(PxIdentity);
		internalCompliance = 0;
		externalCompliance = 0;
		swingLimitContactDistance = 0.05f;
		twistLimitContactDistance = 0.05f;
		driveType = PxArticulationJointDriveType::eTARGET;
	}
// PX_SERIALIZATION
	ArticulationJointCore(const PxEMPTY&)	{}
//~PX_SERIALIZATION

};

#define DY_ARTICULATION_LINK_NONE 0xffffffff

typedef PxU64 ArticulationBitField;

struct ArticulationLink
{
	ArticulationBitField			children;		// child bitmap
	ArticulationBitField			pathToRoot;		// path to root, including link and root
	PxcRigidBody*					body;
	PxsBodyCore*					bodyCore;
	const ArticulationJointCore*	inboundJoint;
	PxU32							parent;
};

typedef size_t ArticulationLinkHandle;

struct ArticulationSolverDesc
{
	FsData*						fsData;
	const ArticulationLink*		links;
	Cm::SpatialVectorV*			motionVelocity;	
	Cm::SpatialVector*			acceleration;
	PxTransform*				poses;
	physx::shdfnd::aos::Mat33V* externalLoads;
	physx::shdfnd::aos::Mat33V* internalLoads;
	const ArticulationCore*		core;
	char*						scratchMemory;
	PxU16						totalDataSize;
	PxU16						solverDataSize;
	PxU8						linkCount;
	PxU8						numInternalConstraints;
	PxU16						scratchMemorySize;
};

#if PX_VC 
    #pragma warning(push)   
	#pragma warning( disable : 4324 ) // Padding was added at the end of a structure because of a __declspec(align) value.
#endif
PX_ALIGN_PREFIX(64)
class Articulation
{
public:
	// public interface

							Articulation(Sc::ArticulationSim*);
							~Articulation();

	// solver methods
	PxU32					getLinkIndex(ArticulationLinkHandle handle)	const;
	PxU32					getBodyCount()									const	{ return mSolverDesc->linkCount;				}
	FsData*					getFsDataPtr()									const	{ return mSolverDesc->fsData;					}
	PxU32					getSolverDataSize()								const	{ return mSolverDesc->solverDataSize;			}
	PxU32					getTotalDataSize()								const	{ return mSolverDesc->totalDataSize;			}
	void					getSolverDesc(ArticulationSolverDesc& d)		const	{ d = *mSolverDesc;	}
	void					setSolverDesc(const ArticulationSolverDesc& d)			{ mSolverDesc = &d;	}

	const ArticulationSolverDesc* getSolverDescPtr()						const	{ return mSolverDesc;	}
	const ArticulationCore*	getCore()										const	{ return mSolverDesc->core;}	
	PxU16					getIterationCounts()							const	{ return mSolverDesc->core->solverIterationCounts; }

	Sc::ArticulationSim*	getArticulationSim()							const	{ return mArticulationSim; }


	// get data sizes for allocation at higher levels
	static void		getDataSizes(PxU32 linkCount, 
								 PxU32 &solverDataSize, 
								 PxU32& totalSize, 
								 PxU32& scratchSize);

private:

	const ArticulationSolverDesc*	mSolverDesc;
	Sc::ArticulationSim* mArticulationSim;
	
#if DY_DEBUG_ARTICULATION
	// debug quantities

	Cm::SpatialVector		computeMomentum(const FsInertia *inertia) const;
	void					computeResiduals(const Cm::SpatialVector *, 
											 const ArticulationJointTransforms* jointTransforms,
											 bool dump = false) const;
	void					checkLimits() const;
#endif

} PX_ALIGN_SUFFIX(64);

#if PX_VC 
     #pragma warning(pop) 
#endif

class PxvArticulationDriveCache
{
public:
	// drive cache stuff
	static void		initialize(FsData &cache,
							   PxU16 linkCount,
							   const ArticulationLink* links,
							   PxReal compliance,
							   PxU32 iterations,
							   char* scratchMemory,
							   PxU32 scratchMemorySize);

	static PxU32	getLinkCount(const FsData& cache);

	static void		applyImpulses(const FsData& cache,
								  Cm::SpatialVectorV* Z,
								  Cm::SpatialVectorV* V);

	static void		getImpulseResponse(const FsData& cache, 
									   PxU32 linkID, 
									   const Cm::SpatialVectorV& impulse,
									   Cm::SpatialVectorV& deltaV);
};

void PxvRegisterArticulations();


static const size_t DY_ARTICULATION_IDMASK = DY_ARTICULATION_MAX_SIZE-1;

PX_FORCE_INLINE Articulation* getArticulation(ArticulationLinkHandle handle)
{
	return reinterpret_cast<Articulation*>(handle & ~DY_ARTICULATION_IDMASK);
}

PX_FORCE_INLINE bool isArticulationRootLink(ArticulationLinkHandle handle)
{
	return !(handle & DY_ARTICULATION_IDMASK);
}


}

}

#endif
