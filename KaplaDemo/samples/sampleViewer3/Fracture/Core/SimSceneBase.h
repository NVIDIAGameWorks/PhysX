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
#ifndef SIM_SCENE_BASE
#define SIM_SCENE_BASE

#include "PxSimulationEventCallback.h"
#include <PsArray.h>
#include <PsUserAllocated.h>
#include "PsHashMap.h"

#include "RTdef.h"

namespace physx
{
class PxPhysics;
class PxCooking;
class PxScene;
class PxRigidDynamic;
class PxD6Joint;
namespace fracture
{
namespace base
{

class Actor;
class Compound;
class Convex;
class FracturePattern;
class CompoundCreator;
class Delaunay2d;
class Delaunay3d;
class PolygonTriangulator;

// -----------------------------------------------------------------------------------
class SimScene :  
	public PxSimulationEventCallback, public ::physx::shdfnd::UserAllocated
{
	friend class Actor;
public:
	static SimScene* createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
protected:
	SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath);
public:
	virtual ~SimScene();

	// Creates Scene Level Singletons
	virtual void createSingletons();
	// Access singletons
	CompoundCreator* getCompoundCreator() {return mCompoundCreator;}
	PolygonTriangulator* getPolygonTriangulator() {return mPolygonTriangulator;}

	// Create non-Singletons
	virtual Actor* createActor();
	virtual Convex* createConvex();
	virtual Compound* createCompound(const FracturePattern *pattern, const FracturePattern *secondaryPattern = NULL, PxReal contactOffset = 0.005f, PxReal restOffset = -0.001f);
	virtual void clear();
	void addCompound(Compound *m);
	void removeCompound(Compound *m);
	// perform deferred deletion
	void deleteCompounds();

	void setScene(PxScene* scene) { mScene = scene; }
	// 
	bool findCompound(const Compound* c, int& actorNr, int& compoundNr);

	void removeActor(Actor* a);

	// Profiler hooks
	virtual void profileBegin(const char* /*name*/) {}
	virtual void profileEnd(const char* /*name*/) {}

	virtual void playSound(const char * /*name*/, int /*nr*/ = -1) {}

	// accessors
	shdfnd::Array<Compound*> getCompounds(); //{ return mCompounds; }
	shdfnd::Array<Actor*> getActors() { return mActors; }
	PxPhysics* getPxPhysics() { return mPxPhysics; }
	PxCooking* getPxCooking() { return mPxCooking; }
	PxScene* getScene() { return mScene; }

	//ConvexRenderer &getConvexRenderer() { return mConvexRenderer; }

	void preSim(float dt);
	void postSim(float dt); //, RegularCell3D* fluidSim);

	void setPlaySounds(bool play) { mPlaySounds = play; }
	void setContactImpactRadius(float radius) { mContactImpactRadius = radius; }
	void setNumNoFractureFrames(int num) { mNumNoFractureFrames = num; }

	void setCamera(const PxVec3 &pos, const PxVec3 &dir, const PxVec3 &up, float fov ) {
		mCameraPos = pos; mCameraDir = dir; mCameraUp = up; mCameraFov = fov;
	}

	//void draw(bool useShader, Shader* particleShader = NULL) {}
	//void setShaderMaterial(Shader* shader, const ShaderMaterial& mat) {this->mShader = shader; this->mShaderMat = mat;}
	//void setFractureForceThreshold(float threshold) { mFractureForceThreshold = threshold; }

	PxMaterial *getPxDefaultMaterial() { return mPxDefaultMaterial; }

	void toggleDebugDrawing() { mDebugDraw = !mDebugDraw; }

	virtual bool pickStart(const PxVec3 &orig, const PxVec3 &dir);
	virtual void pickMove(const PxVec3 &orig, const PxVec3 &dir);
	virtual void pickRelease();
	PxRigidDynamic* getPickActor() { return mPickActor; }
	const PxVec3 &getPickPos() { return mPickPos; }
	const PxVec3 &getPickLocalPos() { return mPickLocalPos; }

	// callback interface

	void onContactNotify(unsigned int arraySizes, void ** shape0Array, void ** shape1Array, void ** actor0Array, void ** actor1Array, float * positionArray, float * normalArray);
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)	{ PX_UNUSED((constraints, count)); }
	void onWake(PxActor** actors, PxU32 count)							{ PX_UNUSED((actors, count)); }
	void onSleep(PxActor** actors, PxU32 count)							{ PX_UNUSED((actors, count)); }
	void onTrigger(PxTriggerPair* pairs, PxU32 count)					{ PX_UNUSED((pairs, count)); }
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);
	void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) { PX_UNUSED(bodyBuffer);  PX_UNUSED(poseBuffer);  PX_UNUSED(count); }

	void toggleRenderDebris() {mRenderDebris = !mRenderDebris;}
	bool getRenderDebrs() {return mRenderDebris;}
	//virtual void dumpSceneGeometry() {}
	shdfnd::Array<PxVec3>& getDebugPoints();
	//virtual void createRenderBuffers() {}
	//void loadAndCreateTextureArrays();

	shdfnd::Array<PxVec3>& getCrackNormals() {return crackNormals;}
	shdfnd::Array<PxVec3>& getTmpPoints() {return tmpPoints;}

	bool mapShapeToConvex(const PxShape& shape, Convex& convex);
	bool unmapShape(const PxShape& shape);
	Convex* findConvexForShape(const PxShape& shape);
	bool owns(const PxShape& shape) {return NULL != findConvexForShape(shape);}

protected:
	//virtual void create3dTexture() {}
	//virtual void updateConvexesTex() {}
	//void playShatterSound(float objectSize);

	void addActor(Actor* a);  // done internally upon creation

	PxPhysics *mPxPhysics;
	PxCooking *mPxCooking;
	PxScene *mScene;
	const char *mResourcePath;
	bool mPlaySounds;

	//shdfnd::Array<Compound*> mCompounds;
	shdfnd::Array<Actor*> mActors;

	float mFractureForceThreshold;
	float mContactImpactRadius;

	shdfnd::Array<PxContactPairPoint> mContactPoints;
	struct FractureEvent {
		void init() {
			compound = NULL; 
			pos = normal = PxVec3(0.0f, 0.0f, 0.0f); 
			additionalRadialImpulse = additionalNormalImpulse = 0.0f;
			withStatic = false;
		}
		Compound *compound;
		PxVec3 pos;
		PxVec3 normal;
		float additionalRadialImpulse;
		float additionalNormalImpulse;
		bool withStatic;
	};

	struct RenderBuffers {
		void init() {
			numVertices = 0; numIndices = 0;
			VBO = 0; IBO = 0; matTex = 0; volTex = 0;
			texSize = 0; numConvexes = -1;
		}
		shdfnd::Array<float> tmpVertices;
		shdfnd::Array<unsigned int> tmpIndices;
		shdfnd::Array<float> tmpTexCoords;
		int numVertices, numIndices;
		unsigned int VBO;
		unsigned int IBO;
		unsigned int volTex;
		unsigned int matTex;
		int texSize;
		int numConvexes;
	};
	RenderBuffers mRenderBuffers;
	unsigned int  mSceneVersion;		// changed on each update
	unsigned int  mRenderBufferVersion;	// to handle updates
	unsigned int  mOptixBufferVersion;	// to handle updates

	PxMaterial *mPxDefaultMaterial;

	//ConvexRenderer mConvexRenderer;

	int mNoFractureFrames;
	int mNoSoundFrames;
	int mFrameNr;
	bool mDebugDraw;

	float mPickDepth;
	PxRigidDynamic *mPickActor;
	PxD6Joint* mPickJoint;
	PxVec3 mPickPos;
	PxVec3 mPickLocalPos;

	float mMinConvexSize;
	int mNumNoFractureFrames; // > 1 to prevent a slow down by too many fracture events

	PxVec3 mCameraPos, mCameraDir, mCameraUp;
	float  mCameraFov;

	float bumpTextureUVScale;
	float extraNoiseScale;
	float roughnessScale;

	float particleBumpTextureUVScale;
	float particleRoughnessScale;
	float particleExtraNoiseScale;
	shdfnd::Array<PxVec3> debugPoints;
	bool mRenderDebris;

	PxSimulationEventCallback* mAppNotify;

	//GLuint diffuseTexArray, bumpTexArray, specularTexArray, emissiveReflectSpecPowerTexArray;

	//GLuint loadTextureArray(std::vector<std::string>& names);

	//Singletons
	CompoundCreator* mCompoundCreator;
	PolygonTriangulator* mPolygonTriangulator;

	//Array for use by Compound (effectively static)
	shdfnd::Array<PxVec3> crackNormals;
	shdfnd::Array<PxVec3> tmpPoints;

	// Deferred Deletion list
	shdfnd::Array<Compound*> delCompoundList;

	// Map used to determine SimScene ownership of shape
	shdfnd::HashMap<const PxShape*,Convex*> mShapeMap;
};

}
}
}

#endif
#endif
