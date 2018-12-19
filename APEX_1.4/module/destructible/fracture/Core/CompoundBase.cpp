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
#include "FracturePatternBase.h"
#include "MeshBase.h"
#include "CompoundGeometryBase.h"
#include "CompoundCreatorBase.h"
#include "PxConvexMeshGeometry.h"
#include "PxRigidBodyExt.h"
#include "PxMat44.h"
#include "PxScene.h"
#include "PxShape.h"

#include "SimSceneBase.h"
#include "CompoundBase.h"
#include "ActorBase.h"
#include "ConvexBase.h"
#include "PsMathUtils.h"

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace physx;

//vector<PxVec3> tmpPoints;
void Compound::appendUniformSamplesOfConvexPolygon(PxVec3* vertices, int numV, float area, nvidia::Array<PxVec3>& samples, nvidia::Array<PxVec3>* normals) {
	using namespace physx::shdfnd;
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
		int np = (int)physx::shdfnd::floor(aa);

		if (rand(0.0f,1.0f) <= aa-np) np++;

		for (int j = 0; j < np; j++) {
			float r1 = rand(0.0f,1.0f);
			float sr1 = PxSqrt(r1);
			float r2 = rand(0.0f, 1.0f);

			PxVec3 p = (1 - sr1) * p0 + (sr1 * (1 - r2)) * p1 + (sr1 * r2) * p2;
			samples.pushBack(p);
			if (normals) normals->pushBack(normal);
			
		}		
	}
}

// --------------------------------------------------------------------------------------------
Compound::Compound(SimScene *scene, const FracturePattern *pattern, const FracturePattern *secondaryPattern, float contactOffset, float restOffset)
{
	mScene = scene;
	mActor = NULL;
	mPxActor = NULL;
	mFracturePattern = pattern;
	mSecondardFracturePattern = secondaryPattern;
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
		const uint32_t shapeCount = mPxActor->getNbShapes();
		physx::PxShape* shapeBuffer[SHAPE_BUFFER_SIZE];
		for (uint32_t shapeStartIndex = 0; shapeStartIndex < shapeCount; shapeStartIndex += SHAPE_BUFFER_SIZE)
		{
			uint32_t shapesRead = mPxActor->getShapes(shapeBuffer, SHAPE_BUFFER_SIZE, shapeStartIndex);
			for (uint32_t shapeBufferIndex = 0; shapeBufferIndex < shapesRead; ++shapeBufferIndex)
			{
				mScene->unmapShape(*shapeBuffer[shapeBufferIndex]);
			}
		}
		// release the actor
		mPxActor->release();
		mPxActor = NULL;
	}
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
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
bool Compound::createFromConvex(Convex* convex, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes)
{
	return createFromConvexes(&convex, 1, pose, vel, omega, copyConvexes);
}

// --------------------------------------------------------------------------------------------
bool Compound::createFromConvexes(Convex** convexes, int numConvexes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, bool copyConvexes)
{
	if (numConvexes == 0)
		return false;

	clear();
	PxVec3 center(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numConvexes; i++) 
		center += convexes[i]->getCenter();
	center /= (float)numConvexes;

	nvidia::Array<PxShape*> shapes;

	for (int i = 0; i < numConvexes; i++) {
		Convex *c;
		if (copyConvexes) {
			c = mScene->createConvex();
			c->createFromConvex(convexes[i]);
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

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
		PxShape *shape = mScene->getPxPhysics()->createShape(
			PxConvexMeshGeometry(mesh),
			*mScene->getPxDefaultMaterial(),
			true,
			c->getLocalPose());
#else
		PxShape *shape = mScene->getPxPhysics()->createShape(
			PxConvexMeshGeometry(mesh),
			*mScene->getPxDefaultMaterial(),
			true
			);
		shape->setLocalPose(c->getLocalPose());
#endif

		shape->setContactOffset(mContactOffset);
		shape->setRestOffset(mRestOffset);

		mScene->mapShapeToConvex(*shape, *c);

		shapes.pushBack(shape);
	}

	if (shapes.empty())
	{
		return false;
	}

	createPxActor(shapes, pose, vel, omega);
	//if (!createPxActor(shapes, pose, vel, omega))
		/*int foo = 0*/;

	//createPointSamples(applyForcePointAreaPerSample);

	return true;
}

// --------------------------------------------------------------------------------------------
bool Compound::createFromGeometry(const CompoundGeometry &geom, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega)
{
	clear();

	nvidia::Array<PxShape*> shapes;

	for (int i = 0; i < (int)geom.convexes.size(); i++) {
		Convex *c = mScene->createConvex();
		c->createFromGeometry(geom, i);
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
			convexAdded(c); //mScene->getConvexRenderer().add(c);

#if (PX_PHYSICS_VERSION_MAJOR == 3) && (PX_PHYSICS_VERSION_MINOR < 3)
		PxShape *shape = mScene->getPxPhysics()->createShape(
			PxConvexMeshGeometry(mesh),
			*mScene->getPxDefaultMaterial(),
			true,
			c->getLocalPose());
#else
		PxShape *shape = mScene->getPxPhysics()->createShape(
			PxConvexMeshGeometry(mesh),
			*mScene->getPxDefaultMaterial(),
			true
			);
		shape->setLocalPose(c->getLocalPose());
#endif

		shape->setContactOffset(mContactOffset);
		shape->setRestOffset(mRestOffset);

		mScene->mapShapeToConvex(*shape, *c);

		shapes.pushBack(shape);
	}

	if (shapes.empty())
		return false;

	createPxActor(shapes, pose, vel, omega);
	return true;
}

// --------------------------------------------------------------------------------------------
void Compound::createFromMesh(const Mesh *mesh, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega, int submeshNr, const PxVec3& scale)
{
	const nvidia::Array<PxVec3> &verts = mesh->getVertices();
	const nvidia::Array<PxVec3> &normals = mesh->getNormals();
	const nvidia::Array<PxVec2> &texcoords = mesh->getTexCoords();
	const nvidia::Array<uint32_t> &indices = mesh->getIndices();

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
	createFromGeometry(mScene->getCompoundCreator()->getGeometry(), pose, vel, omega);
	PxTransform trans(-center);

	if (submeshNr < 0)
		mConvexes[0]->setExplicitVisMeshFromTriangles((int32_t)verts.size(), &verts[0], &normals[0], &texcoords[0], (int32_t)indices.size(), &indices[0], &trans, &scale);
	else {
		const Mesh::SubMesh &sm = mesh->getSubMeshes()[(uint32_t)submeshNr];
		mConvexes[0]->setExplicitVisMeshFromTriangles((int32_t)verts.size(), &verts[0], &normals[0], &texcoords[0], sm.numIndices, &indices[(uint32_t)sm.firstIndex], &trans, &scale);
	}
}

// --------------------------------------------------------------------------------------------
bool Compound::createPxActor(nvidia::Array<PxShape*> &shapes, const PxTransform &pose, const PxVec3 &vel, const PxVec3 &omega)
{
	if (shapes.empty())
		return false;

	for(uint32_t i = 0; i < shapes.size(); i++)
	{
		applyShapeTemplate(shapes[i]);
	}

	PxRigidDynamic* body = mScene->getPxPhysics()->createRigidDynamic(pose);
	if (body == NULL)
		return false;

	body->setSleepThreshold(getSleepingThresholdRB());

	physx::PxActorFlags actorFlags = body->getActorFlags();
	actorFlags |= physx::PxActorFlag::eSEND_SLEEP_NOTIFIES;
	body->setActorFlags(actorFlags);

	mScene->getScene()->addActor(*body);

	for (uint32_t i = 0; i < shapes.size(); i++)
		body->attachShape(*shapes[i]);

	//KS - we clamp the mass in the range [minMass, maxMass]. This helps to improve stability
	PxRigidBodyExt::updateMassAndInertia(*body, 1000.0f);
/*
	const float maxMass = 50.f;
	const float minMass = 1.f;
 
	float mass = PxMax(PxMin(maxMass, body->getMass()), minMass);
	PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);
*/
	

	body->setLinearVelocity(vel);
	body->setAngularVelocity(omega);

	mPxActor = body;
	for (uint32_t i = 0; i < mConvexes.size(); i++)
		mConvexes[i]->setPxActor(mPxActor);
	return true;
}

// --------------------------------------------------------------------------------------------
bool Compound::rayCast(const PxVec3 &orig, const PxVec3 &dir, float &dist, int &convexNr, PxVec3 &normal)
{
	dist = PX_MAX_F32;
	convexNr = -1;

	for (uint32_t i = 0; i < mConvexes.size(); i++) {
		float d;
		PxVec3 n;
		if (mConvexes[i]->rayCast(orig, dir, d, n)) {
			if (d < dist) {
				dist = d;
				convexNr = (int32_t)i;
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
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
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
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
		mConvexes[i]->getWorldBounds(bi);
		bounds.include(bi);
	}
}

// --------------------------------------------------------------------------------------------
void Compound::getLocalBounds(PxBounds3 &bounds) const
{
	bounds.setEmpty();
	PxBounds3 bi;
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
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
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
		Convex *c = mConvexes[i];
		b = c->getBounds();
		b.minimum += c->getMaterialOffset();
		b.maximum += c->getMaterialOffset();
		for (uint32_t j = 0; j < mAttachmentBounds.size(); j++) {
			if (b.intersects(mAttachmentBounds[j]))
				return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------------
bool Compound::patternFracture(const PxVec3 &pos, float minConvexSize, nvidia::Array<Compound*> &compounds, 
					const PxMat33 &patternTransform, nvidia::Array<PxVec3>& debugPoints, float impactRadius, 
					float radialImpulse, const PxVec3 &directedImpulse)
{
	if (mFracturePattern == NULL)
		return false;

	if (mDepth >= mActor->mDepthLimit)
	{
		if (mActor->mDestroyIfAtDepthLimit)
		{
			mActor->removeCompound(this);
		}
		else
		{
			PxTransform pose = mPxActor->getGlobalPose();
			// additional impulse
			PxVec3 imp(0.0f, 0.0f, 0.0f);
			PxVec3 n = pose.p - pos;
			n.normalize();
			imp += n * radialImpulse;
			imp += directedImpulse;
			// prevent small pieces from getting too fast
			const float minMass = 1.0f;
			float mass = getPxActor()->getMass();
			if (mass < minMass) 
				imp *= mass / minMass;
			PxRigidBodyExt::addForceAtPos(*getPxActor(), imp, pos, PxForceMode::eIMPULSE);
		}
		return false;
	}

	compounds.clear();

	//Profiler *p = Profiler::getInstance();

	PxVec3 hit = pos;
	PxTransform pose = mPxActor->getGlobalPose();
	PxVec3 localHit = pose.transformInv(hit);
	PxVec3 lv = mPxActor->getLinearVelocity();
	PxVec3 av = mPxActor->getAngularVelocity();

//	localHit = -pose.t;	// foo

	nvidia::Array<Convex*> newConvexes;
	nvidia::Array<int> compoundSizes;
	nvidia::Array<PxVec3>& crackNormals = mScene->getCrackNormals();
	crackNormals.clear();

	mScene->profileBegin("getCompoundIntersection"); //Profiler::getInstance()->begin("getCompoundIntersection");
	mFracturePattern->getCompoundIntersection(this, PxMat44(patternTransform, localHit), impactRadius, minConvexSize, compoundSizes, newConvexes);
	mScene->profileEnd("getCompoundIntersection"); //Profiler::getInstance()->end("getCompoundIntersection");

	if (compoundSizes.empty())
	{
		return false;
	}

	int debugPointsStart = (int32_t)debugPoints.size();

	uint32_t convexNr = 0;
	for (uint32_t i = 0; i < compoundSizes.size(); i++) {
		uint32_t num = (uint32_t)compoundSizes[i];

		PxVec3 off(0.0f, 0.0f, 0.0f);
		for (uint32_t j = 0; j < num; j++) {
			off += newConvexes[convexNr+j]->getCenter();

			//PxVec3 pp = pose.p + pose.rotate(newConvexes[convexNr+j]->getCenter());
			//debugPoints.push_back(pp);
		}
		off /= (float)num;

		addDust(newConvexes,debugPoints,(int32_t&)convexNr,(int32_t&)num);

		mScene->profileBegin("create new compounds"); //Profiler::getInstance()->begin("create new compounds");

		Compound *m = mScene->createCompound(mSecondardFracturePattern ? mSecondardFracturePattern : mFracturePattern);

		m->mActor = mActor;
		m->mDepth = mDepth+1;
		m->mNormal = mNormal;

		PxVec3 v = lv + av.cross(pose.rotate(off));
		PxTransform posei = pose;
		posei.p += pose.rotate(off);

		if (m->createFromConvexes(&newConvexes[convexNr], (int32_t)num, posei, v, av, false)) {
			m->mAttachmentBounds = mAttachmentBounds;
			if (m->isAttached())
			{
				m->getPxActor()->setActorFlag(physx::PxActorFlag::eSEND_SLEEP_NOTIFIES, false);
				m->getPxActor()->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
			}
			compounds.pushBack(m);
		}
		else
		{
			PX_DELETE(m);
			return false;
		}

		// decrement depth if outside fracture radius
		if (impactRadius > 0.f)
		{
			const nvidia::Array<Convex*>& convexes = m->getConvexes();
			for (uint32_t i = 0; i < convexes.size(); i++)
			{
				if (convexes[i]->getIsFarConvex())
				{
					m->mDepth--;
					break;
				}
			}
		}

		// additional impulse
		PxVec3 imp(0.0f, 0.0f, 0.0f);
		PxVec3 n = posei.p - hit;
		n.normalize();
		imp += n * radialImpulse;
		imp += directedImpulse;

		// prevent small pieces from getting too fast
		const float minMass = 1.0f;
		float mass = m->getPxActor()->getMass();
		if (mass < minMass) 
			imp *= mass / minMass;

		if (!m->isAttached())
			PxRigidBodyExt::addForceAtPos(*m->getPxActor(), imp, hit, PxForceMode::eIMPULSE);
//		m->getPxActor()->addForce(imp / mass, PxForceMode::PxForceMode::eIMPULSE);

		copyShaders(m);

		mScene->profileEnd("create new compounds"); //Profiler::getInstance()->end("create new compounds");

		convexNr += num;
	}

	spawnParticles(crackNormals,debugPoints,debugPointsStart,radialImpulse);

	return true;
}

// --------------------------------------------------------------------------------------------
void Compound::attach(const nvidia::Array<PxBounds3> &bounds)
{
	mAttachmentBounds.resize(bounds.size());

	PxTransform t = getPxActor()->getGlobalPose().getInverse();
	for (uint32_t i = 0; i < bounds.size(); i++) {
		PxVec3 a = t.transform(bounds[i].minimum);
		PxVec3 b = t.transform(bounds[i].maximum);
		mAttachmentBounds[i].minimum = PxVec3(PxMin(a.x,b.x),PxMin(a.y,b.y),PxMin(a.z,b.z));
		mAttachmentBounds[i].maximum = PxVec3(PxMax(a.x,b.x),PxMax(a.y,b.y),PxMax(a.z,b.z));
	}

	if (isAttached())
		mPxActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}

// --------------------------------------------------------------------------------------------
void Compound::attachLocal(const nvidia::Array<PxBounds3> &bounds)
{
	mAttachmentBounds.resize(bounds.size());

	for (uint32_t i = 0; i < bounds.size(); i++) {
		mAttachmentBounds[i] = bounds[i];
	}

	if (isAttached())
		mPxActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}

void Compound::createPointSamples(float area) {	

	pointSamples.clear();
	for (uint32_t i = 0; i < mConvexes.size(); i++) {
		Convex* c = mConvexes[i];
		const nvidia::Array<Convex::Face> &faces = c->getFaces();
		const nvidia::Array<int> &indices = c->getIndices();
		const nvidia::Array<PxVec3> &verts = c->getVertices();			


		
		for (uint32_t k = 0; k < faces.size(); k++) {
			const Convex::Face& f = faces[k];
			/*int nv = f.numIndices;*/

			/*
			PxVec3 avg(0.0f);
			for (int q = 0; q < f.numIndices; q++) {
				avg += verts[indices[f.firstIndex+q]];
			}
			avg /= f.numIndices;
			pointSamples.pushBack(avg);
			*/
			nvidia::Array<PxVec3>& tmpPoints = mScene->getTmpPoints();
			tmpPoints.clear();
			for (int q = 0; q < f.numIndices; q++) {
				tmpPoints.pushBack(verts[(uint32_t)indices[uint32_t(f.firstIndex+q)]]);
			}
			appendUniformSamplesOfConvexPolygon(&tmpPoints[0], (int32_t)tmpPoints.size(), area, pointSamples);

		}
		
	}
}


}
}
}
#endif