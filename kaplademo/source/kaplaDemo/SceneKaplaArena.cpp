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

#include "SceneKaplaArena.h"
#include "Convex.h"
#include "PxSimpleFactory.h"
#include "PxRigidStatic.h"
#include "PxShape.h"
#include "foundation/PxMathUtils.h"
#include "foundation/PxMat44.h"

#include <stdio.h>
#include <GL/glut.h>

#include "SimScene.h"
#include "CompoundCreator.h"
#include "TerrainMesh.h"

#include "PhysXMacros.h"

#include "MathUtils.h"

#include "PxRigidBodyExt.h"


// ----------------------------------------------------------------------------------------------
SceneKaplaArena::SceneKaplaArena(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
	Shader *defaultShader, const char *resourcePath, float slowMotionFactor) :
	SceneKapla(pxPhysics, pxCooking, isGrb, defaultShader, resourcePath, slowMotionFactor)
{

	//onInit(pxPhysics);
}

void SceneKaplaArena::onInit(PxScene* pxScene)
{
	SceneKapla::onInit(pxScene);
	createGroundPlane();

	const PxVec3 dims(0.08f, 0.25f, 1.0f);
	PxMaterial* DefaultMaterial = mPxPhysics->createMaterial(0.4f, 0.15f, 0.1f);

	ShaderMaterial mat;
	mat.init();

	const PxU32 nbOuterRadialLayouts = 384;
	const PxReal outerRadius = 40;

	PxFilterData queryFilterData;
	PxFilterData simFilterData;

	createCylindricalTower(nbOuterRadialLayouts, outerRadius, outerRadius, 8, dims, PxVec3(0.f, 0.f, 0.f), DefaultMaterial, mat, simFilterData, queryFilterData);
	createCylindricalTower(nbOuterRadialLayouts, outerRadius - 2.f, outerRadius - 2.f, 8, dims, PxVec3(0.f, 0.f, 0.f), DefaultMaterial, mat, simFilterData, queryFilterData);
}