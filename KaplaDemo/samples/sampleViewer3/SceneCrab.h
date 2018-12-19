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

#ifndef SCENE_CRAB_H
#define SCENE_CRAB_H

#include "SceneKapla.h"
#include "foundation/Px.h"
#include "foundation/PxSimpleTypes.h"
#include "common/PxPhysXCommonConfig.h"
#include "task/PxTask.h"
#include "CrabManager.h"
#include <vector>


namespace physx
{
	class PxRigidDynamic;
	class PxRevoluteJoint;
	class PxJoint;
}

class SceneCrab : public SceneKapla
{
public:
	SceneCrab(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb, Shader *defaultShader, const char *resourcePath, float slowMotionFactor);
	~SceneCrab();



	virtual void						onInit(PxScene* pxScene);

	virtual void						setScene(PxScene* scene);
	
	virtual void duringSim(float dt);
	virtual void syncAsynchronousWork();

	virtual void getInitialCamera(PxVec3& pos, PxVec3& dir) { pos = PxVec3(-70.f, -5.f, -70.f); dir = PxVec3(1.f, -0.3f, 1.f).getNormalized(); }

	virtual void handleKeyDown(unsigned char key, int x, int y);
	virtual void handleKeyUp(unsigned char key, int x, int y);

private:
	void	createCrabs();

private:

	CrabManager						mCrabManager;
	PxU32							mNbCrabsX;
	PxU32							mNbCrabsZ;
	PxU32							mNbSuperCrabs;
};

#endif
