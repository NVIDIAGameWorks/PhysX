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

class CFirstPersonCamera;

class SampleSceneController : public ISampleController
{
public:
	SampleSceneController(CFirstPersonCamera* camera, ApexController& apex);
	virtual ~SampleSceneController();

	DestructibleActor* getCurrentActor() { return mActor; }

	virtual void onSampleStart();

	// commands
	void throwCube();

	// ui exposed settings:
	float& getCubeVelocity() { return mCubeVelocity; }
	float& getCubeMass() { return mCubeMass; }
	const float* getDamageThreshold() { return &mDamageThreshold; }
	const float getLastImpactDamage() { return mLastImpactDamage; }

private:
	SampleSceneController& operator= (SampleSceneController&);

	class SampleImpactDamageReport : public UserImpactDamageReport
	{
	public:
		SampleImpactDamageReport() : mScene(0) {}
		void setScene(SampleSceneController* scene)
		{
			mScene = scene;
		}
		void onImpactDamageNotify(const ImpactDamageEventData* buffer, uint32_t bufferSize);
	private:
		SampleImpactDamageReport& operator= (SampleImpactDamageReport&);
		SampleSceneController* mScene;
	};
	SampleImpactDamageReport mImpactDamageReport;

	ApexController& mApex;
	CFirstPersonCamera* mCamera;

	DestructibleActor* mActor;

	float mCubeVelocity;
	float mCubeMass;
	float mDamageThreshold;
	float mLastImpactDamage;
	char mLastCubeName[32];
	int mCubesCount;
};

PxFilterFlags DestructionImpactDamageFilterShader(
	PxFilterObjectAttributes attributes0,
	PxFilterData filterData0,
	PxFilterObjectAttributes attributes1,
	PxFilterData filterData1,
	PxPairFlags& pairFlags,
	const void* constantBlock,
	uint32_t constantBlockSize);

#endif