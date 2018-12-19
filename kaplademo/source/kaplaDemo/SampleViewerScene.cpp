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

#include "SampleViewerScene.h"
#include "PxMaterial.h"
#include "ShaderShadow.h"

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>


#ifdef USE_OPTIX
	SampleViewerScene::RenderType SampleViewerScene::mRenderType = SampleViewerScene::rtOPTIX;
#else
	SampleViewerScene::RenderType SampleViewerScene::mRenderType = SampleViewerScene::rtOPENGL;
#endif	

OptixRenderer *SampleViewerScene::mOptixRenderer = NULL;

bool SampleViewerScene::mBenchmark = false;

//-----------------------------------------------------------------------------
SampleViewerScene::SampleViewerScene(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
		Shader *defaultShader, const char *resourcePath, float slowMotionFactor)
{
	mPxPhysics = pxPhysics;
	mPxCooking = pxCooking;
	mCameraPos = PxVec3(0.0f, 20.0f, 10.0f);
	mCameraDir = PxVec3(0.0f, 0.0f, 1.0f);
    mCameraUp = PxVec3(0.0f, 1.0f, 0.0f);
    mCameraFov = 40.f;
	mDefaultShader = defaultShader;
	//printf("hhh = %s\n", resourcePath);
	mResourcePath = resourcePath;
	mSlowMotionFactor = slowMotionFactor;
	mDefaultMaterial = pxPhysics->createMaterial(0.5f, 0.5f, 0.0f);
	mCameraDisable = false;
}

//-----------------------------------------------------------------------------
SampleViewerScene::~SampleViewerScene()
{
	for (int i = 0; i < (int)mShaders.size(); i++)
		delete mShaders[i];
	mShaders.clear();
}
	
// ----------------------------------------------------------------------------------------------
void SampleViewerScene::getMouseRay(int xi, int yi, PxVec3 &orig, PxVec3 &dir)
{
	GLint viewPort[4];
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	glGetIntegerv(GL_VIEWPORT, viewPort);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

	yi = viewPort[3] - yi - 1;
	GLdouble x,y,z;
	gluUnProject((GLdouble) xi, (GLdouble) yi, 0.0f,
		modelMatrix, projMatrix, viewPort, &x, &y, &z);
	orig = PxVec3((float)x, (float)y, (float)z);
	gluUnProject((GLdouble) xi, (GLdouble) yi, 1.0f,
		modelMatrix, projMatrix, viewPort, &x, &y, &z);
	dir = PxVec3((float)x, (float)y, (float)z);
	dir = dir - orig;
	dir.normalize();
	orig -= dir * 100.0f;	// to catch ray casts
}

// ----------------------------------------------------------------------------------------------
void SampleViewerScene::setMaterial(float restitution, float staticFriction, float dynamicFriction)
{
	mDefaultMaterial->setRestitution(restitution);
	mDefaultMaterial->setStaticFriction(staticFriction);
	mDefaultMaterial->setDynamicFriction(dynamicFriction);
}

// ----------------------------------------------------------------------------------------------
void SampleViewerScene::render(bool useShader)
{
#ifdef USE_OPTIX
	if (mRenderType == rtOPTIX && mOptixRenderer == NULL)
		mOptixRenderer = new OptixRenderer( getRendererOptions() );
#endif
}

// ----------------------------------------------------------------------------------------------
void SampleViewerScene::cleanupStaticResources()
{
#ifdef USE_OPTIX
	if( mOptixRenderer ) {
		OptixRenderer* ren = mOptixRenderer;
		mOptixRenderer = 0;
		delete ren;
	}
#endif
}

// ----------------------------------------------------------------------------------------------
std::string SampleViewerScene::mRendererOptions;

void SampleViewerScene::setRendererOptions( const char* text )
{
	if( text )  mRendererOptions = text;
	else        mRendererOptions.clear();
}
