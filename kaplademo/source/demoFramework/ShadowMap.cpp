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

#include "ShadowMap.h"
#include <GL/glew.h>

//----------------------------------------------------------------------------------
struct ShadowMap::Vec4 {
	Vec4() {}
	Vec4(const PxVec3 &v, float vw = 1.0f) { x = v.x; y = v.y; z = v.z; w = vw; }
	float x,y,z,w;
};

//----------------------------------------------------------------------------------
struct ShadowMap::Matrix44 {
	Vec4 operator*(const Vec4& v) const {
		Vec4 res;
		res.x = elems[0] * v.x + elems[4] * v.y + elems[8] * v.z + elems[12] * v.w;
		res.y = elems[1] * v.x + elems[5] * v.y + elems[9] * v.z + elems[13] * v.w;
		res.z = elems[2] * v.x + elems[6] * v.y + elems[10] * v.z + elems[14] * v.w;
		res.w = elems[3] * v.x + elems[7] * v.y + elems[11] * v.z + elems[15] * v.w;
		return res;
	}
	float &element(int i, int j) { return elems[i + 4*j]; }
	void zero() { for (int i = 0; i < 16; i++) elems[i] = 0.0f; }
	void id() { zero(); elems[0] = 1.0f; elems[5] = 1.0f; elems[10] = 1.0f; elems[15] = 1.0f; }

	float elems[16];
};

//----------------------------------------------------------------------------------
ShadowMap::ShadowMap( int w, int h, float fovi, int matOffseti, int resolution) 
{
	shadowOff = 1.0f;
	shadowOff2 = 2048.0f;
	fov = fovi;
	cur_num_splits = 1;
	//cur_num_splits = 3;

	width = w;
	height = h;
	depth_size = resolution;
	split_weight = 0.75;
	matOffset = matOffseti;

	minZAdd = 0;
	maxZAdd = 30.0f;

	init();
}

//----------------------------------------------------------------------------------
void ShadowMap::updateFrustumPoints(Frustum &f, const PxVec3 &center, const PxVec3 &view_dir)
{
	PxVec3 up(0.0, 1.0, 0.0);
	PxVec3 right = view_dir.cross(up);

	PxVec3 fc = center + view_dir*f.fard;
	PxVec3 nc = center + view_dir*f.neard;

	right.normalize();
	up = right.cross(view_dir);
	up.normalize();

	// these heights and widths are half the heights and widths of
	// the near and far plane rectangles
	float near_height = tanf(f.fov/2.0f) * f.neard;
	float near_width = near_height * f.ratio;
	float far_height = tanf(f.fov/2.0f) * f.fard;
	float far_width = far_height * f.ratio;

	f.point[0] = nc - up*near_height - right*near_width;
	f.point[1] = nc + up*near_height - right*near_width;
	f.point[2] = nc + up*near_height + right*near_width;
	f.point[3] = nc - up*near_height + right*near_width;

	f.point[4] = fc - up*far_height - right*far_width;
	f.point[5] = fc + up*far_height - right*far_width;
	f.point[6] = fc + up*far_height + right*far_width;
	f.point[7] = fc - up*far_height + right*far_width;
}

//----------------------------------------------------------------------------------
// updateSplitDist computes the near and far distances for every frustum slice
// in camera eye space - that is, at what distance does a slice start and end
void ShadowMap::updateSplitDist(Frustum f[MAX_SPLITS], float nd, float fd)
{
	float lambda = split_weight;
	float ratio = fd/nd;
	f[0].neard = nd;

	for(int i=1; i<cur_num_splits; i++)
	{
		float si = i / (float)cur_num_splits;

		f[i].neard = lambda*(nd*powf(ratio, si)) + (1-lambda)*(nd + (fd - nd)*si);
		f[i-1].fard = f[i].neard * 1.005f;
	}
	f[cur_num_splits-1].fard = fd;
}

//----------------------------------------------------------------------------------
// this function builds a projection matrix for rendering from the shadow's POV.
// First, it computes the appropriate z-range and sets an orthogonal projection.
// Then, it translates and scales it, so that it exactly captures the bounding box
// of the current frustum slice
float ShadowMap::applyCropMatrix(Frustum &f)
{
	float shad_proj[16];
	float maxX = -1000.0;
	float maxY = -1000.0;
	float maxZ;
	float minX =  1000.0;
	float minY =  1000.0;
	float minZ;

	Matrix44 nv_mvp;
	Vec4 transf;	
	
	// find the z-range of the current frustum as seen from the light
	// in order to increase precision
	glGetFloatv(GL_MODELVIEW_MATRIX, nv_mvp.elems);
	
	// note that only the z-component is need and thus
	// the multiplication can be simplified
	// transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] * f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
	transf = nv_mvp * Vec4(f.point[0]);
	minZ = transf.z;
	maxZ = transf.z;
	for(int i=1; i<8; i++)
	{
		transf = nv_mvp * Vec4(f.point[i]);
		if(transf.z > maxZ) maxZ = transf.z;
		if(transf.z < minZ) minZ = transf.z;
	}
	
	minZ += minZAdd;
	maxZ += maxZAdd;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set the projection matrix with the new z-bounds
	// note the inversion because the light looks at the neg. z axis
	// gluPerspective(LIGHT_FOV, 1.0, maxZ, minZ); // for point lights
	glOrtho(-1.0, 1.0, -1.0, 1.0, -maxZ, -minZ);
	glGetFloatv(GL_PROJECTION_MATRIX, shad_proj);
	glPushMatrix();
	glMultMatrixf(nv_mvp.elems);
	glGetFloatv(GL_PROJECTION_MATRIX, nv_mvp.elems);
	glPopMatrix();

	// find the extends of the frustum slice as projected in light's homogeneous coordinates
	for(int i=0; i<8; i++)
	{
		transf = nv_mvp * Vec4(f.point[i]);

		transf.x /= transf.w;
		transf.y /= transf.w;

		if(transf.x > maxX) maxX = transf.x;
		if(transf.x < minX) minX = transf.x;
		if(transf.y > maxY) maxY = transf.y;
		if(transf.y < minY) minY = transf.y;
	}

	float scaleX = 2.0f/(maxX - minX);
	float scaleY = 2.0f/(maxY - minY);
	float offsetX = -0.5f*(maxX + minX)*scaleX;
	float offsetY = -0.5f*(maxY + minY)*scaleY;

	// apply a crop matrix to modify the projection matrix we got from glOrtho.
	nv_mvp.id();
	nv_mvp.element(0,0) = scaleX;
	nv_mvp.element(1,1) = scaleY;
	nv_mvp.element(0,3) = offsetX;
	nv_mvp.element(1,3) = offsetY;
	glLoadMatrixf(nv_mvp.elems);
	glMultMatrixf(shad_proj);

	glMatrixMode(GL_MODELVIEW);

	return minZ;
}

//----------------------------------------------------------------------------------
// here all shadow map textures and their corresponding matrices are created
void ShadowMap::makeShadowMap(const PxVec3 &cameraPos, const PxVec3 &cameraDir, const PxVec3 &lightDir, float znear, float zfar, 
							  void (*renderShadowCasters)())
{
	float shad_modelview[16];
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(
		cameraPos.x, cameraPos.y, cameraPos.z, 
		cameraPos.x-lightDir.x, cameraPos.y-lightDir.y, cameraPos.z-lightDir.z,
		-1.0, 0.0, 0.0);

	glGetFloatv(GL_MODELVIEW_MATRIX, shad_modelview);

	// redirect rendering to the depth texture
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depth_fb);
	// store the screen viewport
	glPushAttrib(GL_VIEWPORT_BIT);
	// and render only to the shadowmap
	glViewport(0, 0, depth_size, depth_size);
	// offset the geometry slightly to prevent z-fighting
	// note that this introduces some light-leakage artifacts
	glPolygonOffset(shadowOff, shadowOff2);
//		cout<<"shadowOff = "<<shadowOff<<endl;
//		cout<<"shadowOff2 = "<<shadowOff2<<endl;
	glEnable(GL_POLYGON_OFFSET_FILL);

	// compute the z-distances for each split as seen in camera space
	updateSplitDist(f, znear, zfar);

	// for all shadow maps:
	for(int i=0; i<cur_num_splits; i++)
	{
		// compute the camera frustum slice boundary points in world space
		updateFrustumPoints(f[i], cameraPos, cameraDir);
		// adjust the view frustum of the light, so that it encloses the camera frustum slice fully.
		// note that this function sets the projection matrix as it sees best fit
		// minZ is just for optimization to cull trees that do not affect the shadows
		float minZ = applyCropMatrix(f[i]);
		// make the current depth map a rendering target
		glFramebufferTextureLayerEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, depth_tex_ar, 0, i);

		// clear the depth texture from last time
		glClear(GL_DEPTH_BUFFER_BIT);

		// draw the scene
		(*renderShadowCasters)();
		
		glMatrixMode(GL_PROJECTION);
		// store the product of all shadow matries for later
		glMultMatrixf(shad_modelview);
		glGetFloatv(GL_PROJECTION_MATRIX, shad_cpm[i]);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glPopAttrib(); 
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glEnable(GL_TEXTURE_2D);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);	
	glPopMatrix();
}

//----------------------------------------------------------------------------------
void ShadowMap::cameraInverse(float dst[16], float src[16])
{
	dst[0] = src[0];
	dst[1] = src[4];
	dst[2] = src[8];
	dst[3] = 0.0;
	dst[4] = src[1];
	dst[5] = src[5];
	dst[6]  = src[9];
	dst[7] = 0.0;
	dst[8] = src[2];
	dst[9] = src[6];
	dst[10] = src[10];
	dst[11] = 0.0;
	dst[12] = -(src[12] * src[0]) - (src[13] * src[1]) - (src[14] * src[2]);
	dst[13] = -(src[12] * src[4]) - (src[13] * src[5]) - (src[14] * src[6]);
	dst[14] = -(src[12] * src[8]) - (src[13] * src[9]) - (src[14] * src[10]);
	dst[15] = 1.0;
}

//----------------------------------------------------------------------------------
void ShadowMap::doneRender() 
{
	// Unbind texture
	for (int i = 0; i < 8; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
	}
	glActiveTexture(GL_TEXTURE0);
}

//----------------------------------------------------------------------------------
void ShadowMap::prepareForRender(float* cam_modelview, float* cam_proj)
{
	float cam_inverse_modelview[16];
	const float bias[16] = {	0.5, 0.0, 0.0, 0.0, 
								0.0, 0.5, 0.0, 0.0,
								0.0, 0.0, 0.5, 0.0,
								0.5, 0.5f, 0.5, 1.0	};

	// update the camera, so that the user can have a free look
	cameraInverse(cam_inverse_modelview, cam_modelview);

	glActiveTexture(GL_TEXTURE0);

	for(int i=cur_num_splits; i<MAX_SPLITS; i++)
		far_bound[i] = 0;

	// for every active split
	for(int i=0; i<cur_num_splits; i++)
	{
		// f[i].fard is originally in eye space - tell's us how far we can see.
		// Here we compute it in camera homogeneous coordinates. Basically, we calculate
		// cam_proj * (0, 0, f[i].fard, 1)^t and then normalize to [0; 1]
		far_bound[i] = 0.5f*(-f[i].fard*cam_proj[10]+cam_proj[14])/f[i].fard + 0.5f;

		// compute a matrix that transforms from camera eye space to light clip space
		// and pass it to the shader through the OpenGL texture matrices, since we
		// don't use them now
		glActiveTexture(GL_TEXTURE0 + (GLenum)i + matOffset);
		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(bias);
		glMultMatrixf(shad_cpm[i]);
		// multiply the light's (bias*crop*proj*modelview) by the inverse camera modelview
		// so that we can transform a pixel as seen from the camera
		glMultMatrixf(cam_inverse_modelview);
	}

	glActiveTexture(GL_TEXTURE0);
}

//----------------------------------------------------------------------------------
void ShadowMap::init()
{
	//glClearColor(0.8f, 0.8f , 0.9f, 1.0);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);


	glGenFramebuffersEXT(1, &depth_fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, depth_fb);
	glDrawBuffer(GL_NONE);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glGenTextures(1, &depth_tex_ar);

	glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, depth_tex_ar);
	glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_DEPTH_COMPONENT32, depth_size, depth_size, MAX_SPLITS, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	for(int i=0; i<MAX_SPLITS; i++)
	{
		// note that fov is in radians here and in OpenGL it is in degrees.
		// the 0.2f factor is important because we might get artifacts at
		// the screen borders.
		f[i].fov = fov/57.2957795f + 0.2f;
		f[i].ratio = (float)width/(float)height;
	}

}

