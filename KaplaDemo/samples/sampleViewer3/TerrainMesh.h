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

#pragma once

#include "BmpFile.h"
#include "glmesh.h"
#include "Shader.h"
#include "PxCooking.h"
#include <vector>
using namespace std;
class TerrainMesh{
public:
	// Able to render mesh
	// Able to generate physX 
	PxRigidStatic* hfActor;
	PxTransform mHFPose;
	GLuint hfTexture;
	GLMesh* mMesh;
	PxScene* myScene;
	Shader* mShader;
	ShaderMaterial mMaterial;

	~TerrainMesh();
	TerrainMesh(PxPhysics& physics, PxCooking& cooking, PxScene& scene, PxMaterial& material, PxVec3 midPoint, const char* heightMapName, const char* textureName, 
		float hfScale, float maxHeight, Shader* shader, bool invert = false);
	void draw(bool useShader);


	float xStart;
	float zStart;
	float dx, idx;
	vector<float> terrainHeights;
	vector<PxVec3> terrainNormals;

	int width;
	int height;
	float getHeight(float x, float z);
	float getHeightNormal(float x, float z, PxVec3& normal);
	float getHeightNormalCondition(float x, float y, float z, PxVec3& normal);
};

