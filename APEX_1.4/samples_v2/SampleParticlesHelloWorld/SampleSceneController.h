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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.


#ifndef SCENE_CONTROLLER_H
#define SCENE_CONTROLLER_H

#include "ApexController.h"
#include "SampleManager.h"
#include "PxPhysicsAPI.h"

class CFirstPersonCamera;

using namespace physx;
using namespace nvidia;

class SampleSceneController : public ISampleController
{
public:
	SampleSceneController(CFirstPersonCamera* camera, ApexController& apex);
	virtual ~SampleSceneController();

	struct AssetDescription
	{
		const char* model;
		const char* uiName;
	};
	static AssetDescription ASSETS[];
	static int getAssetsCount();

	void setCurrentAsset(int);

	int getCurrentAsset()
	{
		return mCurrentAsset;
	}

	virtual void onSampleStart();
	virtual void Animate(double dt);

	void throwCube();

	enum TouchEvent
	{
		ePRESS,
		eDRAG,
		eRELEASE
	};

	void onTouchEvent(TouchEvent touchEvent, float mouseX, float mouseY);

private:
	SampleSceneController& operator= (SampleSceneController&);

	void loadEffectPackageDatabase();

	int mCurrentAsset;

	PxRigidDynamic* mDraggingActor;
	nvidia::PxVec3 mDraggingActorHookLocalPoint;
	nvidia::PxVec3 mDragAttractionPoint;
	float mDragDistance;

	ApexController& mApex;
	CFirstPersonCamera* mCamera;

	EffectPackageActor* mActor;

};

#endif