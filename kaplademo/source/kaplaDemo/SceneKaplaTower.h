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

#ifndef SCENE_KAPLA_TOWER_H
#define SCENE_KAPLA_TOWER_H

#include "SceneKapla.h"

using namespace physx;

// ---------------------------------------------------------------------
class SceneKaplaTower : public SceneKapla
{
public:
	SceneKaplaTower(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
		Shader *defaultShader, const char *resourcePath, float slowMotionFactor);

	virtual ~SceneKaplaTower(){}

	virtual void handleKeyDown(unsigned char key, int x, int y);

	virtual void preSim(float dt);
	virtual void postSim(float dt);

	virtual void onInit(PxScene* pxScene);

private:

	

	void createGeometricTower(PxU32 height, PxU32 nbFacets, PxVec3 dims, PxVec3 startPos, PxReal startDensity, PxMaterial* material, ShaderMaterial& mat);
	void createCommunicationWire(PxVec3 startPos, PxVec3 endPos,PxReal connectRadius, PxReal connectHeight, PxReal density, PxReal offset, PxRigidDynamic* startBody, 
		PxRigidDynamic* endBody, PxMaterial* material, ShaderMaterial& mat, PxQuat& rot);
	void createTwistTower(PxU32 height, PxU32 nbBlocksPerLayer, PxVec3 dims, PxVec3 startPos, PxMaterial* material, ShaderMaterial& mat);
	void createRectangularTower(PxU32 nbX, PxU32 nbZ, PxU32 height, PxVec3 dims, PxVec3 centerPos, PxMaterial* material, ShaderMaterial& mat);
	//void createCylindricalTower(PxU32 nbRadialPoints, PxReal maxRadius, PxReal minRadius, PxU32 height, PxVec3 dims, PxVec3 centerPos, PxMaterial* material, ShaderMaterial& mat);

	PxReal mAccumulatedTime;

	bool hadInteraction;
	PxU32 nbProjectiles;

};
#endif  // SCENE_BOXES_H
