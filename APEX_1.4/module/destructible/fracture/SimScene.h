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



#include "RTdef.h"
#if RT_COMPILE
#ifndef SIM_SCENE
#define SIM_SCENE

#include "SimSceneBase.h"

namespace nvidia
{
namespace destructible
{
	class DestructibleActorImpl;
}

namespace fracture
{

class FracturePattern;
class Actor;

class SimScene : public base::SimScene
{
public:
	static SimScene* createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
protected:
	SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, PxScene *scene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
public:
	virtual ~SimScene();

	virtual void createSingletons();

	virtual base::Actor* createActor(nvidia::destructible::DestructibleActorImpl* actor);
	virtual base::Convex* createConvex();
	virtual base::Compound* createCompound(const base::FracturePattern *pattern, const base::FracturePattern *secondaryPattern = NULL, float contactOffset = 0.005f, float restOffset = -0.001f);
	virtual base::FracturePattern* createFracturePattern();
	virtual void onWake(PxActor** actors, uint32_t count);
	virtual void onSleep(PxActor** actors, uint32_t count);	

	FracturePattern* getDefaultGlass() {return mDefaultGlass;}

protected:
	FracturePattern* mDefaultGlass;
};

}
}

#endif
#endif