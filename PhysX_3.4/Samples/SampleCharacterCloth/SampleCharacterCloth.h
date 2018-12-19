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

#ifndef SAMPLE_CHARACTER_CLOTH_H
#define SAMPLE_CHARACTER_CLOTH_H

#include "PhysXSample.h"
#include "PxSimulationEventCallback.h"
#include "characterkinematic/PxController.h"
#include "characterkinematic/PxControllerBehavior.h"
#include "PxShape.h"

#include "SampleCharacterHelpers.h"
#include "SampleCharacterClothJump.h"
#include "SampleCharacterClothPlatform.h"

namespace physx
{
	class PxCapsuleController;
	class PxControllerManager;
}

	class SampleCharacterClothFlag;
	class SampleCharacterClothCameraController;
	class ControlledActor;

	class SampleCharacterCloth : public PhysXSample, public PxUserControllerHitReport, public PxControllerBehaviorCallback
	{
		public:
												SampleCharacterCloth(PhysXSampleApplication& app);
		virtual									~SampleCharacterCloth();

		///////////////////////////////////////////////////////////////////////////////

		// Implements SampleApplication
		virtual	void							onInit();
        virtual	void						    onInit(bool restart) { onInit(); }
		virtual	void							onShutdown();
		virtual void                            onSubstep(float dtime);
		virtual	void							onTickPreRender(float dtime);
		virtual	void							onTickPostRender(float dtime);
		virtual void							onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val);
		virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
		
		///////////////////////////////////////////////////////////////////////////////

		// Implements PhysXSampleApplication
		virtual	void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							customizeRender();				
		virtual	void							customizeSample(SampleSetup&);
		virtual	void							customizeSceneDesc(PxSceneDesc&);	
		virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);

		///////////////////////////////////////////////////////////////////////////////

		// Implements PxUserControllerHitReport
		virtual void							onShapeHit(const PxControllerShapeHit& hit);
		virtual void							onControllerHit(const PxControllersHit& hit);
		virtual void							onObstacleHit(const PxControllerObstacleHit& hit);

		// Implements PxControllerBehaviorCallback
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxShape&, const PxActor&);
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxController&);
		virtual PxControllerBehaviorFlags		getBehaviorFlags(const PxObstacle&);

		///////////////////////////////////////////////////////////////////////////////
	protected:
		// non-dynamic environment
				PxRigidStatic*					buildHeightField();
				void							createLandscape(PxReal* heightmap, PxU32 size, PxReal scale, PxReal* outVerts, PxReal* outNorms, PxU32* outTris);
				PxRigidStatic*					createHeightField(PxReal* heightmap, PxReal scale, PxU32 size);
				SampleCharacterClothPlatform*	createPlatform(const PxTransform &pose, const PxGeometry &geom, PxReal travelTime, const PxVec3 &offset, RenderMaterial *renderMaterial);
				void							createPlatforms();
				void                            updatePlatforms(float dtime);

		// cct control
				void                            bufferCCTMotion(const PxVec3& disp, PxReal dtime);
				void                            createCCTController();
				void                            updateCCT(float dtime);

		// cloth and character
				void                            createCharacter();
				void                            createCape();
				void                            createFlags();
				void							releaseFlags();

				void                            resetCape();
				void                            resetCharacter();
				void							resetScene();
				
				void                            updateCape(float dtime);
				void                            updateCharacter(float dtime);
				void                            updateFlags(float dtime);

				void						    createRenderMaterials();
#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
				void                            toggleGPU();
#endif

	private:
		///////////////////////////////////////////////////////////////////////////////
		// Landscape
				PxTransform						mHFPose;
				PxU32							mNbVerts;
				PxU32							mNbTris;
				PxReal*							mVerts;
				PxU32*							mTris;

		///////////////////////////////////////////////////////////////////////////////
		// Camera and Controller
				SampleCharacterClothCameraController*		mCCTCamera;
				PxCapsuleController*			mController;
				PxControllerManager*			mControllerManager;
				PxVec3							mControllerInitialPosition;
				bool                            mCCTActive;
				PxVec3                          mCCTDisplacement;
				PxVec3                          mCCTDisplacementPrevStep;
				PxReal                          mCCTTimeStep;
				SampleCharacterClothJump		mJump;
	
		///////////////////////////////////////////////////////////////////////////////
		// rendering
				RenderMaterial*					mSnowMaterial;
				RenderMaterial*					mPlatformMaterial;
				RenderMaterial*					mRockMaterial;
				RenderMaterial*					mWoodMaterial;
				RenderMaterial*					mFlagMaterial;

		///////////////////////////////////////////////////////////////////////////////
		// cloth
				PxCloth*                        mCape;
		SampleArray<SampleCharacterClothFlag*>  mFlags;
#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
				bool                            mUseGPU;
#endif
				PxU32							mClothFlagCountIndex;
				PxU32							mClothFlagCountIndexMax;

		///////////////////////////////////////////////////////////////////////////////
		// character motion
				Character						mCharacter;
				Skin							mSkin;
				int								mMotionHandleWalk;
				int								mMotionHandleRun;
				int								mMotionHandleJump;

		///////////////////////////////////////////////////////////////////////////////
		// simulation stats
				shdfnd::Time					mTimer;
				PxReal							mAccumulatedSimulationTime;
				PxReal							mAverageSimulationTime;
				PxU32							mFrameCount;								

		///////////////////////////////////////////////////////////////////////////////
		// platform levels
		SampleArray<SampleCharacterClothPlatform*>		mPlatforms;

		friend class SampleCharacterClothCameraController;
		friend class SampleCharacterClothFlag;
	};

#endif
