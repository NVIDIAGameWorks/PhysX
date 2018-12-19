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
#ifndef COMPOUNDBASE
#define COMPOUNDBASE

#define TECHNICAL_MODE 1

#include <foundation/PxVec3.h>
#include <foundation/PxPlane.h>
#include <foundation/PxBounds3.h>
#include <foundation/PxTransform.h>
#include <PsArray.h>
#include <PxRigidDynamic.h>
#include <PsUserAllocated.h>


class Shader;

namespace physx
{
namespace fracture
{
namespace base
{

class Convex;
class Mesh;
class SimScene;
class CompoundGeometry;

// -----------------------------------------------------------------------------------
class Compound : public ::physx::shdfnd::UserAllocated
{
	friend class SimScene;
	friend class Actor;
protected:
	Compound(SimScene* scene, PxReal contactOffset = 0.005f, PxReal restOffset = -0.001f);
public:
	virtual ~Compound();

	virtual bool createFromConvexes(Convex** convexes, int numConvexes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes = true, int matID = 0, int surfMatID = 0);
	bool createFromConvex(Convex* convex, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes = true, int matID = 0, int surfMatID = 0);
	bool createFromGeometry(const CompoundGeometry &geom, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, Shader* myShader, int matID = 0, int surfMatID = 0);
	bool createFromGeometry(const CompoundGeometry &geom, PxRigidDynamic* body, Shader* myShader, int matID = 0, int surfMatID = 0);
	void createFromMesh(const Mesh *mesh, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, int submeshNr = -1, const PxVec3& scale = PxVec3(1.f), int matID = 0, int surfMatID = 0);

	virtual void applyShapeTemplate(PxShape* /*shape*/) {}

	bool rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &convexNr, PxVec3 &normal);
	void setLifeFrames(int frames) { mLifeFrames = frames == 0 ? 1 : frames; }
	int  getLifeFrames() { return mLifeFrames; }


	virtual void convexAdded(Convex* /*c*/, Shader* shader = NULL) {}
	virtual void convexRemoved(Convex* /*c*/) {}

	void attach(const shdfnd::Array<PxBounds3> &bounds);
	void attachLocal(const shdfnd::Array<PxBounds3> &bounds);

	const shdfnd::Array<Convex*>& getConvexes() const { return mConvexes; }
	const shdfnd::Array<PxBounds3>& getAttachmentBounds() const { return mAttachmentBounds; }

	PxRigidDynamic* getPxActor() { return mPxActor; }
	void getWorldBounds(PxBounds3 &bounds) const;
	void getLocalBounds(PxBounds3 &bounds) const;
	void getRestBounds(PxBounds3 &bounds) const;

	//void setShader(Shader* shader, const ShaderMaterial &mat) { mShader = shader; mShaderMat = mat; }
	//Shader* getShader() const { return mShader; }
	//const ShaderMaterial& getShaderMat() { return mShaderMat; }

	void setKinematic(const PxVec3 &vel);
	void step(float dt);

	virtual void draw(bool /*useShader*/, bool /*debug*/ = false) {}

	virtual void clear();

	void setAdditionalImpactImpulse(float radial, float normal) { 
		mAdditionalImpactRadialImpulse = radial; mAdditionalImpactNormalImpulse = normal;
	}
	float getAdditionalImpactRadialImpulse() const { return mAdditionalImpactRadialImpulse; }
	float getAdditionalImpactNormalImpulse() const { return  mAdditionalImpactNormalImpulse; }

	virtual void copyShaders(Compound*) {}

protected:
	
	bool isAttached();

	bool createPxActor(shdfnd::Array<PxShape*> &shapes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega);

	static void appendUniformSamplesOfConvexPolygon(PxVec3* vertices, int numV, float area, shdfnd::Array<PxVec3>& samples, shdfnd::Array<PxVec3>* normals = NULL);

	virtual float getSleepingThresholdRB() {return 0.1f;}

	struct Edge {
		void init(int c0, int c1) {
			this->c0 = c0; this->c1 = c1;
			restLen = 0.0f;
			deleted = false;
		}
		int c0, c1;
		float restLen;
		bool deleted;
	};

	shdfnd::Array<Convex*> mConvexes;
	shdfnd::Array<Edge> mEdges;

	SimScene *mScene;
	Actor *mActor;
	PxRigidDynamic *mPxActor;
	PxVec3 mKinematicVel;
	shdfnd::Array<PxBounds3> mAttachmentBounds;

	//Shader *mShader;
	//ShaderMaterial mShaderMat;

	PxReal mContactOffset;
	PxReal mRestOffset;

	int mLifeFrames;

	float mAdditionalImpactNormalImpulse;
	float mAdditionalImpactRadialImpulse;

	PxU32 mDepth; // fracture depth
	PxVec3 mNormal; // normal use with mSheetFracture
};

}}}

#endif
#endif