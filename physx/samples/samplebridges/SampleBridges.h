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


#ifndef SAMPLE_BRIDGES_H
#define SAMPLE_BRIDGES_H

#include "PhysXSample.h"
#include "PxSimulationEventCallback.h"
#include "SampleBridgesSettings.h"
#include "KinematicPlatform.h"

#ifdef CCT_ON_BRIDGES
	#include "characterkinematic/PxController.h"
	#include "characterkinematic/PxControllerBehavior.h"
#endif
#ifdef PLATFORMS_AS_OBSTACLES
	#include "characterkinematic/PxControllerObstacles.h"
#endif

namespace physx
{
	class PxJoint;

#ifdef CCT_ON_BRIDGES
	class PxControllerManager;
#endif
}

#ifdef CCT_ON_BRIDGES
	class SampleCCTCameraController;
	class ControlledActor;
#endif

	class SampleBridges : public PhysXSample
#ifdef CCT_ON_BRIDGES
		, public PxUserControllerHitReport, public PxControllerBehaviorCallback, public PxQueryFilterCallback
#endif
	{
		public:
												SampleBridges(PhysXSampleApplication& app);
		virtual									~SampleBridges();

		///////////////////////////////////////////////////////////////////////////////

		// Implements RAWImportCallback
		virtual	void							newMesh(const RAWMesh&);
		virtual	void							newShape(const RAWShape&);

		///////////////////////////////////////////////////////////////////////////////

		// Implements SampleApplication
		virtual	void							onInit();
        virtual	void						    onInit(bool restart) { onInit(); }
		virtual	void							onShutdown();
		virtual	void							onTickPreRender(float dtime);
		virtual void							onTickPostRender(float dtime);
		virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);			

		///////////////////////////////////////////////////////////////////////////////

		// Implements PhysXSampleApplication
		virtual	void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							customizeSample(SampleSetup&);
		virtual	void							customizeSceneDesc(PxSceneDesc&);
		virtual	void							customizeRender();
		virtual	void							onSubstep(float dtime);		
		virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);
		virtual PxU32							getDebugObjectTypes() const;

		///////////////////////////////////////////////////////////////////////////////

#ifdef CCT_ON_BRIDGES
		// Implements PxUserControllerHitReport
		virtual void							onShapeHit(const PxControllerShapeHit& hit);
		virtual void							onControllerHit(const PxControllersHit& hit)		{}
		virtual void							onObstacleHit(const PxControllerObstacleHit& hit)	{}

		// Implements PxControllerBehaviorCallback
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxShape& shape, const PxActor& actor);
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxController& controller);
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxObstacle& obstacle);

		// Implements PxQueryFilterCallback
		virtual PxQueryHitType::Enum			preFilter(const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags);
		virtual	PxQueryHitType::Enum			postFilter(const PxFilterData& filterData, const PxQueryHit& hit);
#endif

		private:
#ifdef CCT_ON_BRIDGES
				SampleCCTCameraController*		mCCTCamera;
				ControlledActor*				mActor;
				PxControllerManager*			mControllerManager;
				PxExtendedVec3					mControllerInitialPosition;
#endif
#ifdef PLATFORMS_AS_OBSTACLES
				PxObstacleContext*				mObstacleContext;
				PxBoxObstacle*					mBoxObstacles;
#endif
				PxReal							mGlobalScale;
				std::vector<PxJoint*>			mJoints;
				KinematicPlatformManager		mPlatformManager;
				PxU32							mCurrentPlatform;
				PxF32							mElapsedRenderTime;

				RenderMaterial*					mRockMaterial;
				RenderMaterial*					mWoodMaterial;
				RenderMaterial*					mPlatformMaterial;

				PxRigidActor*					mBomb;
				PxReal							mBombTimer;

				PxReal							mWaterY;
				bool							mMustResync;
				bool							mEnableCCTDebugRender;

#ifdef PLATFORMS_AS_OBSTACLES
				void							updateRenderPlatforms(float dtime);
#endif
	};

#endif
