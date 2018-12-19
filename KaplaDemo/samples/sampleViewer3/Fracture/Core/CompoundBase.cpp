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
#include "MeshBase.h"
#include "CompoundGeometryBase.h"
#include "CompoundCreatorBase.h"
#include "PxConvexMeshGeometry.h"
#include "PxRigidBodyExt.h"
#include "foundation/PxMat44.h"
#include "PxScene.h"
#include "PxShape.h"

#include "SimSceneBase.h"
#include "CompoundBase.h"
#include "ActorBase.h"
#include "ConvexBase.h"
#include "foundation/PxMathUtils.h"

#include <stdio.h>

#include "MathUtils.h"

namespace physx
{
namespace fracture
{
namespace base
{

//vector<PxVec3> tmpPoints;
void Compound::appendUniformSamplesOfConvexPolygon(PxVec3* vertices, int numV, float area, shdfnd::Array<PxVec3>& samples, shdfnd::Array<PxVec3>* normals) {
	PxVec3& p0 = vertices[0];
	PxVec3 normal;
	if (numV < 3) {
		normal = PxVec3(0.0f,1.0f,0.0f);
	} else {
		normal = (vertices[1]-vertices[0]).cross(vertices[2]-vertices[0]);
		normal.normalize();
	}
	for (int i = 1; i < numV-1; i++) {
		PxVec3& p1 = vertices[i];
		PxVec3& p2 = vertices[i+1];
		float tarea = 0.5f*((p1-p0).cross(p2-p0)).magnitude();
		float aa = tarea / area;
		int np = (int)floor(aa);

		if (randRange(0.0f,1.0f) <= aa-np) np++;

		for (int j = 0; j < np; j++) {
			float r1 = randRange(0.0f,1.0f);
			float sr1 = physx::PxSqrt(r1);
			float r2 = randRange(0.0f, 1.0f);

			PxVec3 p = (1 - sr1) * p0 + (sr1 * (1 - r2)) * p1 + (sr1 * r2) * p2;
			samples.pushBack(p);
			if (normals) normals->pushBack(normal);
			
		}		
	}
}

// --------------------------------------------------------------------------------------------
Compound::Compound(SimScene *scene, PxReal contactOffset, PxReal restOffset)
{
	mScene = scene;
	mActor = NULL;
	mPxActor = NULL;
	mContactOffset = contactOffset;
	mRestOffset = restOffset;
	mLifeFrames = -1;	// live forever
	mDepth = 0;
	clear();
}

// --------------------------------------------------------------------------------------------
Compound::~Compound()
{
	clear();
}

// --------------------------------------------------------------------------------------------
#define SHAPE_BUFFER_SIZE	100

void Compound::clear()
{
	if (mPxActor != NULL) {
		// Unmap all shapes for this actor
		const physx::PxU32 shapeCount = mPxActor->getNbShapes();
		physx::PxShape* shapeBuffer[SHAPE_BUFFER_SIZE];
		for (physx::PxU32 shapeStartIndex = 0; shapeStartIndex < shapeCount; shapeStartIndex += SHAPE_BUFFER_SIZE)
		{
			physx::PxU32 shapesRead = mPxActor->getShapes(shapeBuffer, SHAPE_BUFFER_SIZE, shapeStartIndex);
			for (physx::PxU32 shapeBufferIndex = 0; shapeBufferIndex < shapesRead; ++shapeBufferIndex)
			{
				mScene->unmapShape(*shapeBuffer[shapeBufferIndex]);
			}
		}
		// release the actor
		mPxActor->release();
		mPxActor = NULL;
	}
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		if (mConvexes[i]->decreaseRefCounter() <= 0) {
			convexRemoved(mConvexes[i]); //mScene->getConvexRenderer().remove(mConvexes[i]);
			PX_DELETE(mConvexes[i]);
		}
	}

	mConvexes.clear();
	mEdges.clear();

	//mShader = NULL;
	//mShaderMat.init();

	mKinematicVel = PxVec3(0.0f, 0.0f, 0.0f);
	mAttachmentBounds.clear();

	mAdditionalImpactNormalImpulse = 0.0f;
	mAdditionalImpactRadialImpulse = 0.0f;
}

// --------------------------------------------------------------------------------------------
void Compound::setKinematic(const PxVec3 &vel)
{
	if (mPxActor == NULL)
		return;

	mPxActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	mKinematicVel = vel;
}

// --------------------------------------------------------------------------------------------
void Compound::step(float dt)
{
	//if (mPxActor == NULL)
	//	return;

	if (!mKinematicVel.isZero()) {
		PxTransform pose = mPxActor->getGlobalPose();
		pose.p += mKinematicVel * dt;
 		mPxActor->setKinematicTarget(pose);	
	}

	if (mLifeFrames > 0) 
		mLifeFrames--;
}

// --------------------------------------------------------------------------------------------
bool Compound::createFromConvex(Convex* convex, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes, int matID, int surfMatID)
{
	return createFromConvexes(&convex, 1, pose, vel, omega, copyConvexes, matID, surfMatID);
}

// --------------------------------------------------------------------------------------------
bool Compound::createFromConvexes(Convex** convexes, int numConvexes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes, int matID, int surfMatID)
{
	if (numConvexes == 0)
		return false;

	clear();
	PxVec3 center(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numConvexes; i++) 
		center += convexes[i]->getCenter();
	center /= (float)numConvexes;

	shdfnd::Array<PxShape*> shapes;

	for (int i = 0; i < numConvexes; i++) {
		Convex *c;
		if (copyConvexes) {
			c = mScene->createConvex();
			c->createFromConvex(convexes[i], 0, matID, surfMatID);
		}
		else
			c = convexes[i];

		c->increaseRefCounter();
		mConvexes.pushBack(c);

		PxVec3 off = c->centerAtZero();
		c->setMaterialOffset(c->getMaterialOffset() + off);
		c->setLocalPose(PxTransform(off - center));

		if (convexes[i]->isGhostConvex())
			continue;

		bool reused = c->getPxConvexMesh() != NULL;

		mScene->profileBegin("cook convex meshes"); //Profiler::getInstance()->begin("cook convex meshes");
		PxConvexMesh* mesh = c->createPxConvexMesh(this, mScene->getPxPhysics(), mScene->getPxCooking());
		mScene->profileEnd("cook convex meshes"); //Profiler::getInstance()->end("cook convex meshes");

		if (mesh == NULL) {
			if (c->decreaseRefCounter() <= 0)
				PX_DELETE(c);
			mConvexes.popBack();
			continue;
		}

		if (!c->hasExplicitVisMesh())
			c->createVisMeshFromConvex();

		if (!reused)
			convexAdded(c); //mScene->getConvexRenderer().add(c);

		PxShape *shape = mScene->getPxPhysics()->createShape(
			PxConvexMeshGeometry(mesh),
			*mScene->getPxDefaultMaterial(),
			true
			);
		shape->setLocalPose(c->getLocalPose());

		//shape->setContactOffset(mContactOffset);
		//shape->setRestOffset(mRestOffset);

		if (mContactOffset < mRestOffset || mContactOffset < 0.0f)
		{
			printf("WRONG\n");
		}

		mScene->mapShapeToConvex(*shape, *c);

		shapes.pushBack(shape);
	}

	if (shapes.empty())
	{
		return false;
	}

	createPxActor(shapes, pose, vel, omega);


	return true;
}

// --------------------------------------------------------------------------------------------

bool Compound::createFromGeometry(const CompoundGeometry &geom, PxRigidDynamic* body, Shader* myShader, int matID, int surfMatID)
{
	clear();

	shdfnd::Array<PxShape*> shapes(body->getNbShapes());

	body->getShapes(shapes.begin(), body->getNbShapes());

	PX_ASSERT(geom.convexes.size() == body->getNbShapes());

	for (int i = 0; i < (int)geom.convexes.size(); i++) {
		Convex *c = mScene->createConvex();
		c->createFromGeometry(geom, i, 0, matID, surfMatID);
		c->increaseRefCounter();
		mConvexes.pushBack(c);

		PxVec3 off = c->centerAtZero();
		c->setMaterialOffset(c->getMaterialOffset() + off);
		c->createVisMeshFromConvex();
		c->setLocalPose(PxTransform(off));

		bool reused = c->getPxConvexMesh() != NULL;

		if (!reused)
			convexAdded(c, myShader); //mScene->getConvexRenderer().add(c);

		mScene->mapShapeToConvex(*shapes[i], *c);

	}

	if (shapes.empty())
		return false;

	mPxActor = body;
	for (int i = 0; i < (int)mConvexes.size(); i++)
		mConvexes[i]->setPxActor(mPxActor);

	//createPxActor(shapes, pose, vel, omega);
	return true;

}
bool Compound::createFromGeometry(const CompoundGeometry &geom, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, Shader* myShader, int matID, int surfMatID)
{
	clear();

	shdfnd::Array<PxShape*> shapes;

	for (int i = 0; i < (int)geom.convexes.size(); i++) {
		Convex *c = mScene->createConvex();
		c->createFromGeometry(geom, i, 0, matID, surfMatID);
		c->increaseRefCounter();
		mConvexes.pushBack(c);

		PxVec3 off = c->centerAtZero();
		c->setMaterialOffset(c->getMaterialOffset() + off);
		c->createVisMeshFromConvex();
		c->setLocalPose(PxTransform(off));

		bool reused = c->getPxConvexMesh() != NULL;

		PxConvexMesh* mesh = c->createPxConvexMesh(this, mScene->getPxPhysics(), mScene->getPxCooking());
		if (mesh == NULL) {
			if (c->decreaseRefCounter() <= 0)
				PX_DELETE(c);
			mConvexes.popBack();
			continue;
		}

		if (!reused)
			convexAdded(c, myShader); //mScene->getConvexRenderer().add(c);

		PxShape *shape;
		if (geom.convexes[i].isSphere)
		{
			shape = mScene->getPxPhysics()->createShape(
				PxSphereGeometry(geom.convexes[i].radius),
				*mScene->getPxDefaultMaterial(),
				true
				);
		}
		else
		{
			shape = mScene->getPxPhysics()->createShape(
				PxConvexMeshGeometry(mesh),
				*mScene->getPxDefaultMaterial(),
				true
				);
		}
		shape->setLocalPose(c->getLocalPose());

		//shape->setContactOffset(mContactOffset);
		//shape->setRestOffset(mRestOffset);

		mScene->mapShapeToConvex(*shape, *c);

		shapes.pushBack(shape);
	}

	if (shapes.empty())
		return false;

	createPxActor(shapes, pose, vel, omega);
	return true;
}

// --------------------------------------------------------------------------------------------
void Compound::createFromMesh(const Mesh *mesh, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, int submeshNr, const PxVec3& scale, int matID, int surfMatID)
{
	const shdfnd::Array<PxVec3> &verts = mesh->getVertices();
	const shdfnd::Array<PxVec3> &normals = mesh->getNormals();
	const shdfnd::Array<PxVec2> &texcoords = mesh->getTexCoords();
	const shdfnd::Array<PxU32> &indices = mesh->getIndices();

	if (verts.empty() || indices.empty()) 
		return;

	if (submeshNr >= 0 && submeshNr >= (int)mesh->getSubMeshes().size())
		return;

	PxBounds3 bounds;
	mesh->getBounds(bounds, submeshNr);
	PxMat33 scaleMat(PxMat33::createDiagonal(scale));
	bounds = PxBounds3::transformSafe(scaleMat, bounds);

	PxVec3 dims = bounds.getDimensions() * 1.01f;
	PxVec3 center = bounds.getCenter();

	mScene->getCompoundCreator()->createBox(dims);
	createFromGeometry(mScene->getCompoundCreator()->getGeometry(), pose, vel, omega, NULL, matID, surfMatID);
	PxTransform trans(-center);

	if (submeshNr < 0)
		mConvexes[0]->setExplicitVisMeshFromTriangles(verts.size(), &verts[0], &normals[0], &texcoords[0], indices.size(), &indices[0], &trans, &scale);
	else {
		const Mesh::SubMesh &sm = mesh->getSubMeshes()[submeshNr];
		mConvexes[0]->setExplicitVisMeshFromTriangles(verts.size(), &verts[0], &normals[0], &texcoords[0], sm.numIndices, &indices[sm.firstIndex], &trans, &scale);
	}
}

// --------------------------------------------------------------------------------------------
bool Compound::createPxActor(shdfnd::Array<PxShape*> &shapes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega)
{
	if (shapes.empty())
		return false;

	for(int i = 0; i < (int)shapes.size(); i++)
	{
		applyShapeTemplate(shapes[i]);
	}

	PxRigidDynamic* body = mScene->getPxPhysics()->createRigidDynamic(pose);
	if (body == NULL)
		return false;

	//body->setSleepThreshold(getSleepingThresholdRB());
#if 0
	body->setWakeCounter(100000000000.f);
#endif

	mScene->getScene()->addActor(*body);

	for (int i = 0; i < (int)shapes.size(); i++)
		body->attachShape(*shapes[i]);

	

	//KS - we clamp the mass in the range [minMass, maxMass]. This helps to improve stability
	PxRigidBodyExt::updateMassAndInertia(*body, 1.0f);

	/*const PxReal maxMass = 50.f;
	const PxReal minMass = 1.f;
 
	PxReal mass = PxMax(PxMin(maxMass, body->getMass()), minMass);
	PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);*/

	

	body->setLinearVelocity(vel);
	body->setAngularVelocity(omega);

	/*if (vel.isZero() && omega.isZero())
	{
		body->putToSleep();
	}*/

	mPxActor = body;
	for (int i = 0; i < (int)mConvexes.size(); i++)
		mConvexes[i]->setPxActor(mPxActor);
	return true;
}

// --------------------------------------------------------------------------------------------
bool Compound::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &convexNr, PxVec3 &normal)
{
	dist = PX_MAX_F32;
	convexNr = -1;

	for (int i = 0; i < (int)mConvexes.size(); i++) {
		float d;
		PxVec3 n;
		if (mConvexes[i]->rayCast(orig, dir, d, n)) {
			if (d < dist) {
				dist = d;
				convexNr = i;
				normal = n;
			}
		}
	}
	return convexNr >= 0;
}

// --------------------------------------------------------------------------------------------
void Compound::getRestBounds(PxBounds3 &bounds) const
{
	bounds.setEmpty();
	PxBounds3 bi;
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		Convex *c = mConvexes[i];
		PxBounds3 bi = c->getBounds();
		bi = PxBounds3::transformSafe(c->getLocalPose(), bi);
		bounds.include(bi);
	}
}

// --------------------------------------------------------------------------------------------
void Compound::getWorldBounds(PxBounds3 &bounds) const
{
	bounds.setEmpty();
	PxBounds3 bi;
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		mConvexes[i]->getWorldBounds(bi);
		bounds.include(bi);
	}
}

// --------------------------------------------------------------------------------------------
void Compound::getLocalBounds(PxBounds3 &bounds) const
{
	bounds.setEmpty();
	PxBounds3 bi;
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		mConvexes[i]->getLocalBounds(bi);
		bounds.include(bi);
	}
}

// --------------------------------------------------------------------------------------------
bool Compound::isAttached()
{
	if (mAttachmentBounds.empty())
		return false;

	PxBounds3 b;
	for (int i = 0; i < (int)mConvexes.size(); i++) {
		Convex *c = mConvexes[i];
		b = c->getBounds();
		b.minimum += c->getMaterialOffset();
		b.maximum += c->getMaterialOffset();
		for (int j = 0; j < (int)mAttachmentBounds.size(); j++) {
			if (b.intersects(mAttachmentBounds[j]))
				return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------------
void Compound::attach(const shdfnd::Array<PxBounds3> &bounds)
{
	mAttachmentBounds.resize(bounds.size());

	PxTransform t = getPxActor()->getGlobalPose().getInverse();
	for (int i = 0; i < (int)bounds.size(); i++) {
		PxVec3 a = t.transform(bounds[i].minimum);
		PxVec3 b = t.transform(bounds[i].maximum);
		mAttachmentBounds[i].minimum = PxVec3(PxMin(a.x,b.x),PxMin(a.y,b.y),PxMin(a.z,b.z));
		mAttachmentBounds[i].maximum = PxVec3(PxMax(a.x,b.x),PxMax(a.y,b.y),PxMax(a.z,b.z));
	}

	if (isAttached())
		mPxActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}

// --------------------------------------------------------------------------------------------
void Compound::attachLocal(const shdfnd::Array<PxBounds3> &bounds)
{
	mAttachmentBounds.resize(bounds.size());

	for (int i = 0; i < (int)bounds.size(); i++) {
		mAttachmentBounds[i] = bounds[i];
	}

	if (isAttached())
		mPxActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}
}
}
}
#endif