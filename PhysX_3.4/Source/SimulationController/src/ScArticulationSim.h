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


#ifndef PX_PHYSICS_ARTICULATION_SIM
#define PX_PHYSICS_ARTICULATION_SIM


#include "PsUserAllocated.h"
#include "CmPhysXCommon.h"
#include "DyArticulation.h"
#include "ScArticulationCore.h" 
#include "PxsSimpleIslandManager.h"

namespace physx
{

namespace Dy
{
	class Articulation;
}

class PxsTransformCache;
class PxsSimulationController;

namespace Cm
{
	class SpatialVector;
	template <class Allocator> class BitMapBase;
	typedef BitMapBase<Ps::NonTrackingAllocator> BitMap;
}

namespace Bp
{
	class BoundsArray;
}

namespace Sc
{

	class BodySim;
	class ArticulationJointSim;
	class ArticulationCore;
	class Scene;

	class ArticulationSim : public Ps::UserAllocated 
	{
	public:
											ArticulationSim(ArticulationCore& core, 
												Scene& scene,
												BodyCore& root);

											~ArticulationSim();

		PX_INLINE	Dy::Articulation*		getLowLevelArticulation() const { return mLLArticulation; }
		PX_INLINE	ArticulationCore&		getCore() const { return mCore; }

								void		addBody(BodySim& body, 
													BodySim* parent, 
													ArticulationJointSim* joint);
								void		removeBody(BodySim &sim);
								
								Dy::ArticulationLinkHandle	getLinkHandle(BodySim& body) const;
								
								void		checkResize() const;						// resize LL memory if necessary
								
								void		debugCheckWakeCounterOfLinks(PxReal wakeCounter) const;
								void		debugCheckSleepStateOfLinks(bool isSleeping) const;

								bool		isSleeping() const;
								void		internalWakeUp(PxReal wakeCounter);	// called when sim sets sleep timer
								void		sleepCheck(PxReal dt);
								PxU32		getCCDLinks(BodySim** sims);
								void		updateCached(Cm::BitMapPinned* shapehapeChangedMap);
								void		updateContactDistance(PxReal* contactDistance, const PxReal dt, Bp::BoundsArray& boundsArray);


	// temporary, to make sure link handles are set in island detection. This should be done on insertion,
	// but when links are inserted into the scene they don't know about the articulation so for the
	// moment we fix it up at sim start

								void		fixupHandles();

								void		setActive(const bool b, const PxU32 infoFlag=0);

								void		updateForces(PxReal dt, bool simUsesAdaptiveForce);
								void		saveLastCCDTransform();

	// drive cache implementation
	//
						ArticulationDriveCache*		
											createDriveCache(PxReal compliance,
															 PxU32 driveIterations) const;

						void				updateDriveCache(ArticulationDriveCache& cache,
															 PxReal compliance,
															 PxU32 driveIterations) const;

						void				releaseDriveCache(ArticulationDriveCache& cache) const;
						
						void				applyImpulse(BodyCore& link,
														 const ArticulationDriveCache& driveCache,
														 const PxVec3& force,
														 const PxVec3& torque);

						void				computeImpulseResponse(BodyCore& link,
																   PxVec3& linearResponse,
																   PxVec3& angularResponse,
																   const ArticulationDriveCache& driveCache,
																   const PxVec3& force,
																   const PxVec3& torque) const;

					PX_FORCE_INLINE IG::NodeIndex		getIslandNodeIndex() const { return mIslandNodeIndex; }


	private:
					ArticulationSim&		operator=(const ArticulationSim&);
								PxU32		findBodyIndex(BodySim &body) const;


					Dy::Articulation*					mLLArticulation;
					Scene&								mScene;
					ArticulationCore&					mCore;
					Ps::Array<Dy::ArticulationLink>		mLinks;
					Ps::Array<BodySim*>					mBodies;
					Ps::Array<ArticulationJointSim*>	mJoints;
					IG::NodeIndex						mIslandNodeIndex;
					

					// DS: looks slightly fishy, but reallocating/relocating the LL memory for the articulation
					// really is supposed to leave it behaviorally equivalent, and so we can reasonably make
					// all this stuff mutable and checkResize const

					mutable Dy::ArticulationSolverDesc	mSolverData;

					// persistent state of the articulation for warm-starting joint load computation
					mutable Ps::Array<Ps::aos::Mat33V>	mInternalLoads;
					mutable Ps::Array<Ps::aos::Mat33V>	mExternalLoads;

					// persistent data used during the solve that can be released at frame end

					mutable Ps::Array<PxTransform>		mPose;
					mutable Ps::Array<Cm::SpatialVectorV>	mMotionVelocity;		// saved here in solver
					mutable Ps::Array<char>				mFsDataBytes;			// drive cache creation (which is const) can force a resize
					mutable Ps::Array<char>				mScratchMemory;			// drive cache creation (which is const) can force a resize
					mutable Ps::Array<Cm::SpatialVector>		mAcceleration;

					mutable bool						mUpdateSolverData;
	};

} // namespace Sc

}

#endif
