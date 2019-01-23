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


#ifndef SAMPLE_NORTHPOLE_H
#define SAMPLE_NORTHPOLE_H

#include "PhysXSample.h"
#include "PxSimulationEventCallback.h"
#include "characterkinematic/PxController.h" // for PxUserControllerHitReport
#include "characterkinematic/PxControllerBehavior.h"
#include "PxShape.h"

namespace physx
{
	class PxCapsuleController;
	class PxControllerManager;
}

class SampleNorthPoleCameraController;

#define NUM_SNOWMEN 10
#define NUM_BALLS 50

struct SnowMan
{
	PxShape* eyeL;
	PxShape* eyeR;

	void changeMood()
	{
		PxTransform tl = eyeL->getLocalPose();
		PxTransform tr = eyeR->getLocalPose();

		eyeL->setLocalPose(PxTransform(tl.p,tr.q));
		eyeR->setLocalPose(PxTransform(tr.p,tl.q));
	}
};

class SampleNorthPole : public PhysXSample, 
						public PxSimulationEventCallback, 
						public PxUserControllerHitReport,
						public PxControllerBehaviorCallback
	{
		public:
												SampleNorthPole(PhysXSampleApplication& app);
		virtual									~SampleNorthPole();

		///////////////////////////////////////////////////////////////////////////////

		// Implements SampleApplication
		virtual	void							onInit();
        virtual	void						    onInit(bool restart) { onInit(); }
		virtual	void							onShutdown();
		virtual	void							onTickPreRender(PxReal dtime);
		virtual void							onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val);
		virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);		
		virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);
		
		///////////////////////////////////////////////////////////////////////////////

		// Implements PhysXSampleApplication
		virtual void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							customizeSample(SampleSetup&);
		virtual	void							customizeSceneDesc(PxSceneDesc&);
		virtual	void							onSubstep(float dtime);

		///////////////////////////////////////////////////////////////////////////////

		// Implements PxSimulationEventCallback
		virtual	void							onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);
		virtual	void							onTrigger(PxTriggerPair*, PxU32) {}
		virtual void							onConstraintBreak(PxConstraintInfo*, PxU32) {}
		virtual void							onWake(PxActor** , PxU32 ) {}
		virtual void							onSleep(PxActor** , PxU32 ){}
		virtual void							onAdvance(const PxRigidBody*const*, const PxTransform*, const PxU32) {}

		///////////////////////////////////////////////////////////////////////////////

		// Implements PxUserControllerHitReport
		virtual void							onShapeHit(const PxControllerShapeHit& hit);
		virtual void							onControllerHit(const PxControllersHit& hit)		{}
		virtual void							onObstacleHit(const PxControllerObstacleHit& hit)	{}

		// Implements PxControllerBehaviorCallback
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxShape&, const PxActor&)	{ return PxControllerBehaviorFlags(0);	}
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxController&)				{ return PxControllerBehaviorFlags(0);	}
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxObstacle&)					{ return PxControllerBehaviorFlags(0);	}

		///////////////////////////////////////////////////////////////////////////////

		// Implements PxSimulationFilterShader
		static PxFilterFlags					filter(	PxFilterObjectAttributes attributes0, 
														PxFilterData filterData0, 
														PxFilterObjectAttributes attributes1, 
														PxFilterData filterData1,
														PxPairFlags& pairFlags,
														const void* constantBlock,
														PxU32 constantBlockSize);
		///////////////////////////////////////////////////////////////////////////////

		// dynamics
				enum
				{
												CCD_FLAG = 1<<29,
												SNOWBALL_FLAG = 1<<30,
												DETACHABLE_FLAG = 1<<31
				};

				void							cookCarrotConvexMesh();
				PxConvexMesh*					mCarrotConvex;

				PxRigidDynamic*					throwBall();
				PxRigidDynamic*					mSnowBalls[NUM_BALLS];
				RenderBaseActor*				mSnowBallsRenderActors[NUM_BALLS];

				void							createSnowMen();
				PxRigidDynamic*					createSnowMan(const PxTransform& pos, const PxU32 mode, const PxU32 index);
				SnowMan							mSnowman[NUM_SNOWMEN+1]; // 1 extra so that NUM_SNOWMEN can be set to 0 for debugging

				void							resetScene();

		static	void							setSnowball(PxShape& shape);
		static	void							setDetachable(PxShape& shape);
		static	bool							isDetachable(PxFilterData& filterData);
		static	bool							needsContactReport(const PxFilterData& filterData0, const PxFilterData& filterData1);
		static	void							setCCDActive(PxShape& shape, PxRigidBody* rigidBody);
		static	bool							isCCDActive(PxFilterData& filterData);
				void							detach();
				std::vector<PxShape*>			mDetaching;

		///////////////////////////////////////////////////////////////////////////////

		// static landscape
				std::vector<PxVec3>				mVerts;
				std::vector<PxVec3>				mNorms;
				std::vector<PxU32>				mTris;

				PxRigidStatic*					buildIglooTriMesh();
				void							addBlock(PxVec3 blockVerts[8]);

				PxRigidStatic*					buildHeightField();
				void							createLandscape(PxReal* heightmap, PxU32 size, PxReal scale, PxReal* outVerts, PxReal* outNorms, PxU32* outTris);
				PxRigidStatic*					createHeightField(PxReal* heightmap, PxReal scale, PxU32 size);

		///////////////////////////////////////////////////////////////////////////////

		// Camera and Controller
		SampleNorthPoleCameraController*		mNorthPoleCamera;
				PxCapsuleController*			mController;
				PxControllerManager*			mControllerManager;
				PxExtendedVec3					mControllerInitialPosition;
				bool							mDoStandup;
				PxReal							mControllerRadius;
				PxReal							mStandingSize;
				PxReal							mCrouchingSize;
				PxCapsuleController*			createCharacter(const PxExtendedVec3& position);
				void							tryStandup();
				void							resizeController(PxReal height);

		///////////////////////////////////////////////////////////////////////////////
		
		// rendering
				RenderMaterial*					mSnowMaterial;
				RenderMaterial*					mCarrotMaterial;
				RenderMaterial*					mButtonMaterial;
	};

#endif
