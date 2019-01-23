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

#include "PxPhysicsAPI.h"

#include "characterkinematic/PxControllerManager.h"
#include "characterkinematic/PxBoxController.h"
#include "characterkinematic/PxCapsuleController.h"

#include "geometry/PxMeshQuery.h"
#include "geometry/PxTriangle.h"

#include "SampleCustomGravity.h"
#include "SampleCustomGravityCameraController.h"
#include "SampleCCTActor.h"
#include "SampleCustomGravityInputEventIds.h"
#include <SampleUserInputDefines.h>
#include <SamplePlatform.h>
#include <SampleUserInputIds.h>

#include "RenderBaseActor.h"
#include "RenderBoxActor.h"
#include "RenderCapsuleActor.h"



using namespace SampleRenderer;
using namespace SampleFramework;

#ifdef USE_BOX_CONTROLLER
PxBoxController* SampleCustomGravity::createCharacter(const PxExtendedVec3& position)
#else
PxCapsuleController* SampleCustomGravity::createCharacter(const PxExtendedVec3& position)
#endif
{
	const float height = 2.0f;		
//	const float height = 1e-6f;	// PT: TODO: make it work with 0?

#ifdef USE_BOX_CONTROLLER
	PxBoxControllerDesc cDesc;
	cDesc.halfHeight			= height;
	cDesc.halfSideExtent		= mControllerRadius;
	cDesc.halfForwardExtent		= mControllerRadius;
#else
	PxCapsuleControllerDesc cDesc;
	cDesc.height				= height;
	cDesc.radius				= mControllerRadius;
#endif
	cDesc.material				= &getDefaultMaterial();
	cDesc.position				= position;
	cDesc.slopeLimit			= SLOPE_LIMIT;
	cDesc.contactOffset			= CONTACT_OFFSET;
	cDesc.stepOffset			= STEP_OFFSET;
	cDesc.invisibleWallHeight	= INVISIBLE_WALLS_HEIGHT;
	cDesc.maxJumpHeight			= MAX_JUMP_HEIGHT;
	cDesc.reportCallback		= this;

	mControllerInitialPosition = cDesc.position;

#ifdef USE_BOX_CONTROLLER
	PxBoxController* ctrl = static_cast<PxBoxController*>(mControllerManager->createController(cDesc));
#else
	PxCapsuleController* ctrl = static_cast<PxCapsuleController*>(mControllerManager->createController(cDesc));
#endif
	// remove controller shape from scene query for standup overlap test
	PxRigidDynamic* actor = ctrl->getActor();
	if(actor)
	{
		if(actor->getNbShapes())
		{
			PxShape* ctrlShape;
			actor->getShapes(&ctrlShape,1);
			ctrlShape->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, false);

#ifdef USE_BOX_CONTROLLER
			const PxVec3 standingExtents(mControllerRadius, height, mControllerRadius);
 			mRenderActor = SAMPLE_NEW(RenderBoxActor)(*getRenderer(), standingExtents);
#else
 			mRenderActor = SAMPLE_NEW(RenderCapsuleActor)(*getRenderer(), mControllerRadius, height*0.5f);
#endif
			if(mRenderActor)
				mRenderActors.push_back(mRenderActor);
		}
		else
			fatalError("character actor has no shape");
	}
	else
		fatalError("character could not create actor");

	// uncomment the next line to render the character
	//createRenderObjectsFromActor(ctrl->getActor());

	return ctrl;
}

void SampleCustomGravity::onShapeHit(const PxControllerShapeHit& hit)
{
	PX_ASSERT(hit.shape);

	// PT: TODO: rewrite this
	if(hit.triangleIndex!=PxU32(-1))
	{
		PxTriangleMeshGeometry meshGeom;
		PxHeightFieldGeometry hfGeom;
		if(hit.shape->getTriangleMeshGeometry(meshGeom))
		{
			PxTriangle touchedTriangle;
			PxMeshQuery::getTriangle(meshGeom, PxShapeExt::getGlobalPose(*hit.shape, *hit.shape->getActor()), hit.triangleIndex, touchedTriangle, NULL/*, NULL, NULL*/);
			mValidTouchedTriangle = true;
			mTouchedTriangle[0] = touchedTriangle.verts[0];
			mTouchedTriangle[1] = touchedTriangle.verts[1];
			mTouchedTriangle[2] = touchedTriangle.verts[2];
		}
		else if(hit.shape->getHeightFieldGeometry(hfGeom))
		{
			PxTriangle touchedTriangle;
			PxMeshQuery::getTriangle(hfGeom, PxShapeExt::getGlobalPose(*hit.shape, *hit.shape->getActor()), hit.triangleIndex, touchedTriangle, NULL/*, NULL, NULL*/);
			mValidTouchedTriangle = true;
			mTouchedTriangle[0] = touchedTriangle.verts[0];
			mTouchedTriangle[1] = touchedTriangle.verts[1];
			mTouchedTriangle[2] = touchedTriangle.verts[2];
		}
	}

	defaultCCTInteraction(hit);
}

void SampleCustomGravity::resetScene()
{
#ifdef USE_BOX_CONTROLLER
	mBoxController->setPosition(mControllerInitialPosition);
#else
	mCapsuleController->setPosition(mControllerInitialPosition);
#endif
	mCCTCamera->setView(0,0);

/*	while(mPhysicsActors.size())
	{
		PxRigidActor* actor = mPhysicsActors.back();
		removeActor(actor);
	}*/
}

void SampleCustomGravity::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);

	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(DRAW_WALLS,				WKEY_B,				OSXKEY_B,			LINUXKEY_B			);
	DIGITAL_INPUT_EVENT_DEF(DEBUG_RENDER,			WKEY_J,				OSXKEY_J,			LINUXKEY_J			);
	DIGITAL_INPUT_EVENT_DEF(RELEASE_TOUCH_SHAPE,	WKEY_T,				OSXKEY_T,			LINUXKEY_T			);

	//digital mouse events
	DIGITAL_INPUT_EVENT_DEF(RAYCAST_HIT,			MOUSE_BUTTON_RIGHT,	MOUSE_BUTTON_RIGHT,	MOUSE_BUTTON_RIGHT	);
}

void SampleCustomGravity::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	if(val)
	{
		switch(ie.m_Id)
		{
		case RESET_SCENE:
			{
				resetScene();
			}
			break;
		case DRAW_WALLS:
			{
				mDrawInvisibleWalls = !mDrawInvisibleWalls;
			}
			break;
		case DEBUG_RENDER:
			{
				mEnableCCTDebugRender = !mEnableCCTDebugRender; 
			}
			break;
		case RELEASE_TOUCH_SHAPE:
			{
#ifndef USE_BOX_CONTROLLER
				PxControllerState state;
				mCapsuleController->getState(state);
				if (state.touchedShape && (!state.touchedShape->getActor()->is<PxRigidStatic>()))
				{
					PxRigidActor* actor = state.touchedShape->getActor();

					std::vector<PxRigidDynamic*>::iterator i;
					for(i=mDebugActors.begin(); i != mDebugActors.end(); i++)
					{
						if ((*i) == actor)
						{
							mDebugActors.erase(i);
							break;
						}
					}
					removeActor(actor);
				}
#endif
			}
			break;
		default:
			break;
		};
	}

	PhysXSample::onDigitalInputEvent(ie,val);
}
