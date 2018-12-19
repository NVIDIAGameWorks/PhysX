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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "SampleCharacterClothFlag.h"
#include "SampleCharacterCloth.h"

#include "TestClothHelpers.h"
#include "cloth/PxClothParticleData.h"

using namespace PxToolkit;

////////////////////////////////////////////////////////////////////////////////
SampleCharacterClothFlag::SampleCharacterClothFlag(SampleCharacterCloth& sample, const PxTransform &pose, PxU32 resX, PxU32 resY, PxReal sizeX, PxReal sizeY, PxReal height)
    :
	mRenderActor(NULL),
	mSample(sample),
    mWindDir(1.0f, 0.0f, 0.0f),
    mWindRange(1.0f),
	mWindStrength(1.0f),
	mTime(0.0f),
	mCapsuleActor(NULL)
{
	PxScene& scene = mSample.getActiveScene();
	PxPhysics& physics = mSample.getPhysics();

	// adjust placement position for the flag with ray casting so that we can put each flag properly on the terrain
	PxVec3 pos = pose.p;
	PxRaycastBuffer hit;
	scene.raycast(PxVec3(pos.x,100,pos.z), PxVec3(0,-1,0), 500.0f, hit, PxHitFlags(0xffff));
	bool didHit = hit.hasBlock;
	PX_ASSERT(didHit); PX_UNUSED(didHit);
	pos = hit.block.position + PxVec3(0,0.2f,0.0);

	// settings to adjust flag and pole position on the ground
    PxTransform clothPose(pos, pose.q);

	PxReal halfSizeX = sizeX * 0.5f;
    PxReal halfSizeY = sizeY * 0.5f;
    PxReal offset = sizeX * 0.1f;
	PxReal halfHeight = height * 0.5f;

    PxReal clothHeight = height - halfSizeY;
    clothPose.p.y += clothHeight;

	// create the cloth flag mesh
	SampleArray<PxVec4> vertices;
	SampleArray<PxU32> primitives;
	PxClothMeshDesc meshDesc = Test::ClothHelpers::createMeshGrid(
		PxVec3(sizeX,0,0), PxVec3(0,-sizeY,0), resX, resY, vertices, primitives, mUVs);

	// attach two corners on the left
	for (PxU32 i = 0; i < meshDesc.points.count; i++)
	{
		PxReal u = mUVs[i].x, v = mUVs[i].y;
		bool constrain = ( (u < 0.01) && (v < 0.01) ) || (( u < 0.01) && (v > 0.99));
		vertices[i].w = constrain ? 0.0f : 1.0f;
	}
	
	// create cloth fabric
	PxClothFabric* clothFabric = PxClothFabricCreate(physics, meshDesc, PxVec3(0,-1,0));
	PX_ASSERT(meshDesc.points.stride == sizeof(PxVec4));

	// create the cloth actor
	const PxClothParticle* particles = (const PxClothParticle*)meshDesc.points.data;
	PxCloth* cloth = physics.createCloth( clothPose, *clothFabric, particles, PxClothFlags());
	PX_ASSERT(cloth);	

	// add this cloth into the scene
	scene.addActor(*cloth);

	// create collision capsule for the pole
	SampleArray<PxClothCollisionSphere> spheres(2);
	spheres[0].pos = PxVec3(-halfSizeX - offset, -clothHeight, 0);
	spheres[1].pos = PxVec3(-halfSizeX - offset, halfSizeY, 0);
	spheres[0].radius = 0.1f;
	spheres[1].radius = 0.1f;

	SampleArray<PxU32> indexPairs;
	indexPairs.pushBack(0);
	indexPairs.pushBack(1);
	cloth->setCollisionSpheres(spheres.begin(), 2);
	cloth->addCollisionCapsule(indexPairs[0], indexPairs[1]);

	// create render material
	RenderMaterial* clothMaterial = mSample.mFlagMaterial;
	mRenderActor = SAMPLE_NEW (RenderClothActor)(*mSample.getRenderer(), *cloth, meshDesc,
		mUVs.begin(), 0.5f);
    mRenderActor->setRenderMaterial(clothMaterial);
	mSample.mRenderClothActors.push_back(mRenderActor);
	mSample.mRenderActors.push_back(mRenderActor);

	// set solver settings
	cloth->setSolverFrequency(120);
	cloth->setDampingCoefficient(PxVec3(0.0f));

	cloth->setStretchConfig(PxClothFabricPhaseType::eBENDING, PxClothStretchConfig(0.1f));
	cloth->setTetherConfig(PxClothTetherConfig(1.0f, 1.0f));

    mCloth = cloth;

	// we create an invisible kinematic capsule so that the CCT avoids colliding character against the pole of this flag
	// note that cloth collision capsule above has no effect on rigid body scenes.
	PxTransform capsulePose(pos, PxQuat(PxHalfPi, PxVec3(0,0,1)));
	capsulePose.p.x -= halfSizeX + offset;
	capsulePose.p.y += halfHeight;
	PxCapsuleGeometry geom(0.2f,halfHeight);

	mCapsuleActor = sample.getPhysics().createRigidDynamic(capsulePose);
	mCapsuleActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	PxRigidActorExt::createExclusiveShape(*mCapsuleActor, geom, sample.getDefaultMaterial());
	scene.addActor(*mCapsuleActor);
}

////////////////////////////////////////////////////////////////////////////////
void
SampleCharacterClothFlag::release()
{
	mSample.removeRenderClothActor(*mRenderActor);
	delete mRenderActor;

	mCloth->release();
	mCapsuleActor->release();

	delete this;
}

////////////////////////////////////////////////////////////////////////////////
void     
SampleCharacterClothFlag::setWind(const PxVec3 &dir, PxReal strength, const PxVec3& range)
{
    mWindStrength = strength;
    mWindDir = dir;
    mWindRange = range;
}

////////////////////////////////////////////////////////////////////////////////
void     
SampleCharacterClothFlag::update(PxReal dtime)
{
    PX_ASSERT(mCloth);
    
	// implementation of a simple wind force that varies its direction, strength over time
	mTime += dtime * PxToolkit::Rand(0.0f, 1.0f);

	float st = 1.0f + (float)sin(mTime); 
	float windStrength = PxToolkit::Rand(1.0f, st) * mWindStrength;	
	float windRangeStrength = PxToolkit::Rand(0.0f, 2.0f);

	PxVec3 offset( PxReal(PxToolkit::Rand(-1,1)), PxReal(PxToolkit::Rand(-1,1)), PxReal(PxToolkit::Rand(-1,1)));
	float ct = 1.0f + (float)cos(mTime + 0.1);
	offset *= ct;
	PxVec3 windAcceleration = windStrength * mWindDir + windRangeStrength * mWindRange.multiply(offset);

	mCloth->setExternalAcceleration(windAcceleration);

#if 0
	PxU32 nbParticles = mCloth->getNbParticles();
	SampleArray<PxVec4> accel(nbParticles);

	for (PxU32 i = 0; i < nbParticles; i++)
	{
		accel[i].x = PxToolkit::Rand(-1,1) * mWindRange.x;
		accel[i].y = PxToolkit::Rand(-1,1) * mWindRange.y;
		accel[i].z = PxToolkit::Rand(-1,1) * mWindRange.z;
	}
	mCloth->setParticleAccelerations(accel.begin());
#endif
}

#endif // PX_USE_CLOTH_API

 
