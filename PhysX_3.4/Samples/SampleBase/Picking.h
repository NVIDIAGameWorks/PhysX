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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PICKING_H
#define PICKING_H

#include "foundation/PxSimpleTypes.h"
#include "foundation/PxVec3.h"
#include "extensions/PxD6Joint.h"
#include "extensions/PxSphericalJoint.h"

#include "PhysXSample.h"
#include "RendererProjection.h"

#define USE_D6_JOINT_FOR_MOUSE 1			// PT: please keep it 0 for interactive tests where one needs to rotate objects
#define USE_SPHERICAL_JOINT_FOR_MOUSE 0
//#define VISUALIZE_PICKING_RAYS

namespace physx
{
	class PxActor;
	class PxDistanceJoint;
	class PxRigidDynamic;
	class PxScene;
	class PxPhysics;
}

namespace physx {

	struct PickingCommands
	{
		enum Enum
		{
			PICK_START, //Bound to mouse 1 down
			PICK_STOP,  //bound to mouse 1 up
			SCREEN_MOTION_CURSOR, //See DefaultMovementStrategy
			SCREEN_MOTION_CURSOR_DEPTH, //See DefaultMovementStrategy
		};
	};
	
	class Picking 
	{
	public:
													Picking(PhysXSample& frame);
													~Picking();

		PX_FORCE_INLINE	void						lazyPick() { pick(mMouseScreenX, mMouseScreenY); }
						bool						isPicked() const;
						bool						pick(int x, int y);
						void						moveCursor(PxI32 x, PxI32 y);
//						void						moveCursor(PxReal deltaDepth);

						void						computeCameraRay(PxVec3& orig, PxVec3& dir, PxI32 x, PxI32 y)	const;

		// returns picked actor
						PxActor*					letGo();
						void						tick();
						void						project(const physx::PxVec3& v, int& xi, int& yi, float& depth)	const;

#ifdef VISUALIZE_PICKING_RAYS
		struct Ray
		{
			PxVec3	origin;
			PxVec3	dir;
		};
		PX_FORCE_INLINE	const std::vector<Ray>&		getRays()	const	{ return mRays;	}
#endif

	private:
						void						grabActor(const PxVec3& worldImpact, const PxVec3& rayOrigin);
						void						moveActor(int x, int y);
						PxVec3						unProject( int x, int y, float depth)	const;

						PxActor*					mSelectedActor;

#if USE_D6_JOINT_FOR_MOUSE
						PxD6Joint*					mMouseJoint; // was PxDistanceJoint, PxSphericalJoint, PxD6Joint
#elif USE_SPHERICAL_JOINT_FOR_MOUSE
						PxSphericalJoint*			mMouseJoint;
#else
						PxDistanceJoint*			mMouseJoint;
#endif
						PxRigidDynamic*				mMouseActor;
						PxRigidDynamic*				mMouseActorToDelete;
						PxReal						mDistanceToPicked;
						int							mMouseScreenX, mMouseScreenY;
						PhysXSample&				mFrame;
#ifdef VISUALIZE_PICKING_RAYS
						std::vector<Ray>			mRays;
#endif
	};
}

#endif
