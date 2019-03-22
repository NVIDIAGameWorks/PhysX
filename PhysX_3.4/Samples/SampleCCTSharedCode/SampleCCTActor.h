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

#ifndef SAMPLE_CCT_ACTOR_H
#define SAMPLE_CCT_ACTOR_H

#include "characterkinematic/PxExtended.h"
#include "characterkinematic/PxController.h"
#include "SamplePreprocessor.h"
#include "SampleCCTJump.h"

namespace physx
{
	class PxController;
	class PxUserControllerHitReport;
	class PxControllerBehaviorCallback;
	class PxControllerManager;
	class PxPhysics;
	class PxScene;
}

using namespace physx;

namespace SampleRenderer
{
	class Renderer;
}

class RenderBaseActor;
class PhysXSample;

	struct ControlledActorDesc
	{
										ControlledActorDesc();

		PxControllerShapeType::Enum		mType;
		PxExtendedVec3					mPosition;
		float							mSlopeLimit;
		float							mContactOffset;
		float							mStepOffset;
		float							mInvisibleWallHeight;
		float							mMaxJumpHeight;
		float							mRadius;
		float							mHeight;
		float							mCrouchHeight;
		float							mProxyDensity;
		float							mProxyScale;
		float							mVolumeGrowth;
		PxUserControllerHitReport*		mReportCallback;
		PxControllerBehaviorCallback*	mBehaviorCallback;
	};

	class ControlledActor : public SampleAllocateable
	{
		public:
													ControlledActor(PhysXSample& owner);
		virtual										~ControlledActor();

						PxController*				init(const ControlledActorDesc& desc, PxControllerManager* manager);
						PxExtendedVec3				getFootPosition()	const;
						void						reset();
						void						teleport(const PxVec3& pos);
						void						sync();
						void						tryStandup();
						void						resizeController(PxReal height);
						void						resizeStanding()			{ resizeController(mStandingSize);	}
						void						resizeCrouching()			{ resizeController(mCrouchingSize);	}
						void						jump(float force)			{ mJump.startJump(force);			}

		PX_FORCE_INLINE	RenderBaseActor*			getRenderActorStanding()	{ return mRenderActorStanding;		}
		PX_FORCE_INLINE	RenderBaseActor*			getRenderActorCrouching()	{ return mRenderActorCrouching;		}
		PX_FORCE_INLINE	PxController*				getController()				{ return mController;				}

						const Jump&                 getJump() const { return mJump; }

		protected:
						PhysXSample&				mOwner;
						PxControllerShapeType::Enum	mType;
						Jump						mJump;

						PxExtendedVec3				mInitialPosition;
						PxVec3						mDelta;
						bool						mTransferMomentum;

						PxController*				mController;
						RenderBaseActor*			mRenderActorStanding;
						RenderBaseActor*			mRenderActorCrouching;
						PxReal						mStandingSize;
						PxReal						mCrouchingSize;
						PxReal						mControllerRadius;
						bool						mDoStandup;
						bool						mIsCrouching;
		friend class SampleCCTCameraController;

	private:
		ControlledActor& operator=(const ControlledActor&);
	};

	void defaultCCTInteraction(const PxControllerShapeHit& hit);

#endif
