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

#ifndef SAMPLE_VIEWER_SCENE_H
#define SAMPLE_VIEWER_SCENE_H

#include "foundation/PxVec3.h"
#include "foundation/PxTransform.h"
#include "PxPhysics.h"
#include "PxCooking.h"

using namespace physx;

#include <vector>
#include <string>

class OptixRenderer;

#ifdef USE_OPTIX
#include "OptixRenderer.h"
#endif

class Shader;
class ShaderShadow;

// ---------------------------------------------------------------------
class SampleViewerScene
{
public:
	SampleViewerScene(PxPhysics* pxPhysics, PxCooking *pxCooking, bool isGrb,
		Shader *defaultShader, const char *resourcePath, float slowMotionFactor);
	virtual ~SampleViewerScene();

	// virtual interface

	virtual void preSim(float dt) {};
	virtual void postSim(float dt) {};
	virtual void duringSim(float dt) {};
	virtual void syncAsynchronousWork() {};

	virtual void handleMouseButton(int button, int state, int x, int y) {};
	virtual void handleMouseMotion(int x, int y) {};

	virtual void handleKeyDown(unsigned char key, int x, int y) {};
	virtual void handleKeyUp(unsigned char key, int x, int y) {};
	virtual void handleSpecialKey(unsigned char key, int x, int y) {};

	virtual void handleGamepadButton(int button, bool state) {};
	virtual void handleGamepadAxis(int axis, float x) {};
	virtual void handleGamepadTrigger(int trigger, float x) {};

	virtual void render(bool useShader);
	virtual void printPerfInfo() {}

	virtual void setSlowMotionFactor(float factor) { mSlowMotionFactor = factor; }
	virtual std::string getWeaponName() { return ""; };
	virtual void customizeSceneDesc(PxSceneDesc& desc) {}

	virtual void onInit(PxScene* pxScene){ mPxScene = pxScene; }

	virtual void setScene(PxScene* pxScene) { mPxScene = pxScene; }

	// common code

	void setCamera(const PxVec3 &pos, const PxVec3 &dir, const PxVec3 &up, float fov ) {
		mCameraPos = pos; mCameraDir = dir; mCameraUp = up; mCameraFov = fov;
	}

	virtual void getCamera(PxVec3& pos, PxVec3& dir){}
	virtual bool isCameraDisable() { return mCameraDisable; }

	virtual void getInitialCamera(PxVec3& pos, PxVec3& dir) { pos = PxVec3(0.f, 25.f, 30.f); dir = PxVec3(0.f, -0.3f, -1.f).getNormalized(); }

	void getMouseRay(int xi, int yi, PxVec3 &orig, PxVec3 &dir);
	std::vector<ShaderShadow*>& getShaders() { return mShaders; }

	enum RenderType {
		rtOPENGL,
		rtOPTIX,
		rtNUM,
	};
	static void setRenderType(RenderType renderType) { mRenderType = renderType; }
	static void toggleRenderType() { setRenderType((RenderType)(((int)mRenderType+1)%rtNUM)); }
	static RenderType getRenderType() { return mRenderType; }
	static OptixRenderer* getOptixRenderer() { return mOptixRenderer; }
	
	// Set option string for renderers.
	static void setRendererOptions( const char* );
	static const char* getRendererOptions() {return mRendererOptions.c_str();}

	// Call this before exit().
	static void cleanupStaticResources();
	static void setBenchmark( bool on ) { mBenchmark = on; }
	static bool isBenchmark() { return mBenchmark; }

	virtual bool isSceneKapla() {return false;}


	PxPhysics& getPhysics() { return *mPxPhysics; }
	PxCooking& getCooking() { return *mPxCooking; }
	PxScene& getScene() { return *mPxScene; }


protected:
	void setMaterial(float restitution = 0.2f, float staticFriction = 0.2f, float dynamicFriction = 0.2f);

	PxPhysics *mPxPhysics;
	PxCooking *mPxCooking;
	PxScene *mPxScene;

	PxMaterial *mDefaultMaterial;

	PxVec3 mCameraPos, mCameraDir, mCameraUp;
	float  mCameraFov;
	std::vector<ShaderShadow*> mShaders;
	Shader *mDefaultShader;
	const char *mResourcePath;
	float mSlowMotionFactor;

	static RenderType mRenderType;
	static OptixRenderer *mOptixRenderer;
	static bool mBenchmark;
	static std::string	mRendererOptions;

	bool mCameraDisable;
};

#endif  // SAMPLE_VIEWER_SCENE_H
