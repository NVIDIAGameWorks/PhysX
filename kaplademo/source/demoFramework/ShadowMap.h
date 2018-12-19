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
// Copyright (c) 2007-2018 NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
// File:   ShadowMapping.cpp
// Original Author: Rouslan Dimitrov
// Modified by: Nuttapong Chentanez and Matthias Mueller-Fischer
//----------------------------------------------------------------------------------


#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "foundation/PxVec3.h"
#include <GL/glew.h>

using namespace physx;

#define FAR_DIST 200.0
#define MAX_SPLITS 1
#define LIGHT_FOV 45.0

//--------------------------------------------------------------------------
class ShadowMap
{
public:
	ShadowMap( int w, int h, float fovi, int matOffseti, int resolution = 4096);

	void makeShadowMap(const PxVec3 &cameraPos, const PxVec3 &cameraDir, const PxVec3 &lightDir, float znear, float zfar,
		void (*renderShadowCasters)());

	// call before the map is used for rendering
	void prepareForRender(float* cam_modelview, float* cam_proj);
	// call when rendering is done
	void doneRender();

	int    getTextureSize() { return depth_size; }
	GLuint getDepthTexArray() { return depth_tex_ar; }
	float  getFarBound(int i) { return far_bound[i]; }
	int    getNumSplits() { return cur_num_splits; }

private:
	struct Frustum {
		float neard;
		float fard;
		float fov;
		float ratio;
		PxVec3 point[8];
	};

	struct Vec4;
	struct Matrix44;

	void updateFrustumPoints(Frustum &f, const PxVec3 &center, const PxVec3 &view_dir);
	void updateSplitDist(Frustum f[MAX_SPLITS], float nd, float fd);
	float applyCropMatrix(Frustum &f);

	void cameraInverse(float dst[16], float src[16]);
	void init();

	float far_bound[MAX_SPLITS];

	float minZAdd;
	float maxZAdd;

	float shadowOff;
	float shadowOff2;
	float fov;

	int cur_num_splits;
	int width;
	int height;
	int depth_size ;

	GLuint depth_fb;
	GLuint depth_tex_ar;
	
	Frustum f[MAX_SPLITS];
	float shad_cpm[MAX_SPLITS][16];

	float split_weight;
	int matOffset;
};
#endif