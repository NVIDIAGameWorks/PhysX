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

#include "SampleCharacterCloth.h"

#include "SampleCharacterClothCameraController.h"
#include "SampleCharacterClothFlag.h"

#include "TestClothHelpers.h"
#include "SampleCharacterHelpers.h"
#include "characterkinematic/PxCapsuleController.h"
#include "cloth/PxClothParticleData.h"
#include "extensions/PxClothMeshQuadifier.h"

using namespace PxToolkit;

#if PX_SUPPORT_GPU_PHYSX
PxClothFlag::Enum gGpuFlag = PxClothFlag::eCUDA; 
#elif PX_XBOXONE
PxClothFlag::Enum gGpuFlag = PxClothFlag::eDEFAULT;
#endif

// size of spheres for stickman (total 30 bones + 1 root)
static const PxReal gSphereRadius[] = { 
	2.0f,  // root

	// left lower limb
	1.5f,  // lhipjoint
	1.5f,  // lfemur (upper leg)
	1.0f,  // ltibia (lower leg)
	1.0f,  // lfoot
	1.0f,  // ltoes

	// right lower limb
	1.5f,  // rhipjoint
	1.5f,  // rfemur
	1.0f,  // rtibia
	1.0f,  // rfoot
	1.0f,  // rtoes

	// spine
	2.5f,  // lowerback
	2.5f,  // upperback
	2.0f,  // thorax
	1.0f,  // lowerneck
	1.0f,  // upperneck

	2.0f,  // head

	// left arm/hand
	1.5f,  // lclavicle (shoulder)
	1.5f,  // lhumerus (upper arm)
	1.3f,  // lradius (lower arm)
	1.5f,  // lwrist
	1.8f,  // lhand
	0.5f,  // lfingers
	0.5f,  // lthumb

	// right arm/hand
	1.5f,  // rclavicle
	1.5f,  // rhumerus
	1.3f,  // rradius
	1.5f,  // rwrist
	1.8f,  // rhand
	0.5f,  // rfingers
	0.5f,  // rthumb
};

static const PxReal gCharacterScale = 0.1;

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::createCharacter()
{
	const char* asfpath = getSampleMediaFilename("motion/stickman.asf");
	if (!mCharacter.create(asfpath, gCharacterScale))
	{
		fatalError("ERROR - Cannot load motion/stickman.asf\n");
	}

	// add motion clips to the character
	const char* amcpath = 0;

	amcpath = getSampleMediaFilename("motion/stickman_walking_01.amc");
	mMotionHandleWalk = mCharacter.addMotion(amcpath);
	if (mMotionHandleWalk < 0)
		fatalError("ERROR - Cannot load motion/stickman_walking_01.amc\n");

	amcpath = getSampleMediaFilename("motion/stickman_jump_01.amc");
	mMotionHandleJump = mCharacter.addMotion(amcpath, 100,200);
	if (mMotionHandleJump < 0)
		fatalError("ERROR - Cannot load motion/stickman_jump_01.amc\n");

	// initialize character position, scale, and orientation
	resetCharacter();
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::createCape()
{
	PxSceneWriteLock scopedLock(*mScene);

	// compute root transform and positions of all the bones
	PxTransform rootPose;
	SampleArray<PxVec3> positions;	
	SampleArray<PxU32> indexPairs;
	mCharacter.getFramePose(rootPose, positions, indexPairs);

	// convert bones to collision capsules
	SampleArray<PxClothCollisionSphere> spheres;	
	spheres.resize(positions.size());
	for (PxU32 i = 0; i < positions.size(); i++)
	{
		spheres[i].pos = positions[i];
		spheres[i].radius = gCharacterScale * gSphereRadius[i];
	}

	// create the cloth cape mesh from file
	SampleArray<PxVec4> vertices;
	SampleArray<PxU32> primitives;
	SampleArray<PxVec2> uvs;
	const char* capeFileName = getSampleMediaFilename("ctdm_cape_m.obj");
	PxClothMeshDesc meshDesc = Test::ClothHelpers::createMeshFromObj(capeFileName, 
		gCharacterScale * 0.3f, PxQuat(PxIdentity), PxVec3(0,-1.5,0), vertices, primitives, uvs);

	for (PxU32 i = 0; i < meshDesc.invMasses.count; i++)
	{
		((float*)meshDesc.invMasses.data)[i*4] = 0.5f;
	}

	if (!meshDesc.isValid()) fatalError("Could not load ctdm_cape_m.obj\n");

	for (PxU32 i = 0; i < meshDesc.points.count; i++)
	{
		if(uvs[i].y > 0.85f)
			vertices[i].w = 0.0f;
	}

	// create the cloth
	PxClothMeshQuadifier quadifier(meshDesc);
	mCape = createClothFromMeshDesc(quadifier.getDescriptor(), rootPose, PxVec3(0,-1,0),
		uvs.begin(), "dummy_cape_d.bmp", 0.8f);
	PxCloth& cloth = *mCape;

	cloth.setCollisionSpheres(spheres.begin(), static_cast<PxU32>(positions.size()));
	for(PxU32 i=0; i<indexPairs.size(); i+=2)
		cloth.addCollisionCapsule(indexPairs[i], indexPairs[i+1]);

	// compute initial skin binding to the character
	mSkin.bindToCharacter(mCharacter, vertices);

	// set solver settings
	cloth.setSolverFrequency(240);

	cloth.setStiffnessFrequency(10.0f);

	// damp global particle velocity to 90% every 0.1 seconds
	cloth.setDampingCoefficient(PxVec3(0.2f)); // damp local particle velocity
	cloth.setLinearDragCoefficient(PxVec3(0.2f)); // transfer frame velocity
	cloth.setAngularDragCoefficient(PxVec3(0.2f)); // transfer frame rotation

	// reduce impact of frame acceleration
	// x, z: cloth swings out less when walking in a circle
	// y: cloth responds less to jump acceleration
	cloth.setLinearInertiaScale(PxVec3(0.8f, 0.6f, 0.8f));

	// leave impact of frame torque at default
	cloth.setAngularInertiaScale(PxVec3(1.0f));

	// reduce centrifugal force of rotating frame
	cloth.setCentrifugalInertiaScale(PxVec3(0.3f));	
	
	const bool useVirtualParticles = true;
	const bool useSweptContact = true;
	const bool useCustomConfig = true;

	// virtual particles
	if (useVirtualParticles)
		Test::ClothHelpers::createVirtualParticles(cloth, meshDesc, 4);

	// ccd
	cloth.setClothFlag(PxClothFlag::eSWEPT_CONTACT, useSweptContact);
	
	// use GPU or not
#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	cloth.setClothFlag(gGpuFlag, mUseGPU);
#endif

	// custom fiber configuration
	if (useCustomConfig)
	{
		PxClothStretchConfig stretchConfig;
		stretchConfig.stiffness = 1.0f;

		cloth.setStretchConfig(PxClothFabricPhaseType::eVERTICAL, PxClothStretchConfig(1.0f));		
		cloth.setStretchConfig(PxClothFabricPhaseType::eVERTICAL, PxClothStretchConfig(1.0f));
		cloth.setStretchConfig(PxClothFabricPhaseType::eSHEARING, PxClothStretchConfig(0.75f));
		cloth.setStretchConfig(PxClothFabricPhaseType::eBENDING, PxClothStretchConfig(0.5f));
		cloth.setTetherConfig(PxClothTetherConfig(1.0f));
	}
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::createFlags()
{
	PxSceneWriteLock scopedLock(*mScene);

	PxQuat q = PxQuat(PxIdentity);

	PxU32 numFlagRow = 2;
	PxU32 resX = 20, resY = 10;
	if (mClothFlagCountIndex == 0) 
	{
		numFlagRow = 2;
		resX = 20;
		resY = 10;
	}
	else if (mClothFlagCountIndex == 1) 
	{
		numFlagRow = 4;
		resX = 30;
		resY = 15;
	}
	else if (mClothFlagCountIndex == 2) 
	{		
		numFlagRow = 8;
		resX = 50;
		resY = 25;
	}
	else if (mClothFlagCountIndex == 3) 
	{		
		numFlagRow = 15;
		resX = 50;
		resY = 25;
	}

	for (PxU32 i = 0; i < numFlagRow; i++)
	{
	    mFlags.pushBack(new SampleCharacterClothFlag(*this, PxTransform(PxVec3(-4,0,-10.0f -5.0f * i), q), resX, resY, 2.0f, 1.0f, 5.0f));
		mFlags.pushBack(new SampleCharacterClothFlag(*this, PxTransform(PxVec3(4,0,-10.0f -5.0f * i), q), resX, resY, 2.0f, 1.0f, 5.0f));
	}

#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
	for (PxU32 i = 0; i < mFlags.size(); ++i)
		mFlags[i]->getCloth()->setClothFlag(gGpuFlag, mUseGPU);
#endif
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::releaseFlags()
{
	PxSceneWriteLock scopedLock(*mScene);

	const size_t nbFlags = mFlags.size();
	for(PxU32 i = 0;i < nbFlags;i++)
		mFlags[i]->release();
	mFlags.clear();
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::resetCape()
{
	PxSceneWriteLock scopedLock(*mScene);

	PxCloth& cloth = *mCape;

	// compute cloth pose and collider positions
	PxTransform rootPose;
	SampleArray<PxVec3> positions;
	SampleArray<PxU32> indexPairs;
	mCharacter.getFramePose(rootPose, positions, indexPairs);

	// set cloth pose and zero velocity/acceleration
	cloth.setGlobalPose(rootPose);

	// set collision sphere positions
	SampleArray<PxClothCollisionSphere> spheres(positions.size());
	cloth.getCollisionData(spheres.begin(), 0, 0, 0, 0);
	for (PxU32 i = 0; i < positions.size(); i++)
		spheres[i].pos = positions[i];
	cloth.setCollisionSpheres(spheres.begin(), spheres.size());

	// set positions for constrained particles
	SampleArray<PxVec3> particlePositions;
	mSkin.computeNewPositions(mCharacter, particlePositions);
	Test::ClothHelpers::setParticlePositions(cloth, particlePositions, false, false);

	cloth.setExternalAcceleration(PxVec3(0.0f));

	// clear previous state of collision shapes etc.
	cloth.clearInterpolation();
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::resetCharacter()
{
	// compute global pose and local positions
	PxExtendedVec3 posExt = mController->getPosition();
	PxVec3 pos = PxVec3(PxReal(posExt.x), PxReal(posExt.y), PxReal(posExt.z)) - PxVec3(0.0f, 1.5, 0.0f);

	PxTransform pose;
	pose.p = pos;
	pose.q = PxQuat(PxIdentity);

	mCharacter.resetMotion(37.0f);
	mCharacter.setGlobalPose(pose);
	mCharacter.setTargetPosition(PxVec3(pos.x, pos.y, pos.z));
	mCharacter.setMotion(mMotionHandleWalk, true);
	mCharacter.move(1.0f, false, mCCTActive);
	mCharacter.faceToward(PxVec3(0,0,-1));

	mJump.reset();

	mCCTDisplacementPrevStep = PxVec3(0.0f);
}


///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::updateCape(float dtime)
{
	PxSceneWriteLock scopedLock(*mScene);

	PxCloth& cloth = *mCape;

	// compute cloth pose and collider positions
	PxTransform rootPose;
	SampleArray<PxVec3> spherePositions;
	SampleArray<PxU32> indexPairs;
	mCharacter.getFramePose(rootPose, spherePositions, indexPairs);

	// set cloth pose
	cloth.setTargetPose(rootPose);

	// set collision sphere positions
	SampleArray<PxClothCollisionSphere> spheres(spherePositions.size());
	cloth.getCollisionData(spheres.begin(), 0, 0, 0, 0);
	for (PxU32 i = 0; i < spherePositions.size(); i++)
		spheres[i].pos = spherePositions[i];
	cloth.setCollisionSpheres(spheres.begin(), spheres.size());

	// set positions for constrained particles
	SampleArray<PxVec3> particlePositions;
	mSkin.computeNewPositions(mCharacter, particlePositions);

	Test::ClothHelpers::setParticlePositions(cloth, particlePositions, true, true);

	// add wind while in freefall
	if (mJump.isInFreefall())
	{
		PxReal strength = 20.0f;
		PxVec3 offset( PxReal(PxToolkit::Rand(-1,1)), 1.0f + PxReal(PxToolkit::Rand(-1,1)), PxReal(PxToolkit::Rand(-1,1)));
		PxVec3 windAcceleration = strength * offset;
		cloth.setExternalAcceleration(windAcceleration);
	}
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::updateCharacter(float dtime)
{
	// compute global pose of the character from CCT
	PxExtendedVec3 posExt = mController->getPosition();
	PxVec3 offset(0.0f, gCharacterScale * 15.0f, 0.0f); // offset from controller center to the foot
	PxVec3 pos = PxVec3(PxReal(posExt.x), PxReal(posExt.y), PxReal(posExt.z)) - offset;
	
	// update character pose and motion 
	if (mJump.isJumping() == true)
	{	
		mCharacter.setTargetPosition(pos);
		mCharacter.setMotion(mMotionHandleJump);
		mCharacter.move(0.25f, true);
	}
	else 
	{
		bool cctActive = mCCTActive && (mJump.isInFreefall() == false);

		mCharacter.setTargetPosition(pos);
		mCharacter.setMotion(mMotionHandleWalk);
		mCharacter.move(1.0f, false, cctActive);
	}
}

///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::updateFlags(float dtime)
{
	PxSceneWriteLock scopedLock(*mScene);

	// apply wind force to flags
	for (PxU32 i = 0 ; i < mFlags.size(); i++)
	{
		mFlags[i]->setWind(PxVec3(1,0.1,0), 40.0f, PxVec3(0.0f, 10.0f, 10.0f));
		mFlags[i]->update(dtime);
	}
}

#if PX_SUPPORT_GPU_PHYSX || PX_XBOXONE
///////////////////////////////////////////////////////////////////////////////
void
SampleCharacterCloth::toggleGPU()
{
	PxSceneWriteLock scopedLock(*mScene);

	mUseGPU = !mUseGPU;

	if (mCape)
		mCape->setClothFlag(gGpuFlag, mUseGPU);

	for (PxU32 i = 0 ; i < mFlags.size(); i++)
	{
		mFlags[i]->getCloth()->setClothFlag(gGpuFlag, mUseGPU);
	}
}
#endif

#endif // PX_USE_CLOTH_API
