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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

// ****************************************************************************
// This snippet demonstrates the use of articulations.
// ****************************************************************************

#include <ctype.h>
#include <vector>

#include "PxPhysicsAPI.h"

#include "../snippetutils/SnippetUtils.h"
#include "../snippetcommon/SnippetPrint.h"
#include "../snippetcommon/SnippetPVD.h"

#define USE_REDUCED_COORDINATE_ARTICULATION 1
#define CREATE_SCISSOR_LIFT 1

#if CREATE_SCISSOR_LIFT
	bool gCreateLiftScene = true;
#else
	bool gCreateLiftScene = false;
#endif

using namespace physx;

PxDefaultAllocator		gAllocator;
PxDefaultErrorCallback	gErrorCallback;

PxFoundation*			gFoundation		= NULL;
PxPhysics*				gPhysics		= NULL;

PxDefaultCpuDispatcher*	gDispatcher		= NULL;
PxScene*				gScene			= NULL;

PxMaterial*				gMaterial		= NULL;

PxPvd*                  gPvd			= NULL;

#if USE_REDUCED_COORDINATE_ARTICULATION
	PxArticulationReducedCoordinate*		gArticulation = NULL;
	PxArticulationJointReducedCoordinate*	gDriveJoint = NULL;
#else
	PxArticulation*							gArticulation	= NULL;
#endif

#if USE_REDUCED_COORDINATE_ARTICULATION
PxFilterFlags scissorFilter(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
							PxFilterObjectAttributes attributes1, PxFilterData filterData1,
							PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED(attributes0);
	PX_UNUSED(attributes1);
	PX_UNUSED(constantBlock);
	PX_UNUSED(constantBlockSize);
	if (filterData0.word2 != 0 && filterData0.word2 == filterData1.word2)
		return PxFilterFlag::eKILL;
	pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
	return PxFilterFlag::eDEFAULT;
}

void createScissorLift()
{
	const PxReal runnerLength = 2.f;
	const PxReal placementDistance = 1.8f;

	const PxReal cosAng = (placementDistance) / (runnerLength);

	const PxReal angle = PxAcos(cosAng);

	const PxReal sinAng = PxSin(angle);

	const PxQuat leftRot(-angle, PxVec3(1.f, 0.f, 0.f));
	const PxQuat rightRot(angle, PxVec3(1.f, 0.f, 0.f));

	//(1) Create base...
	PxArticulationLink* base = gArticulation->createLink(NULL, PxTransform(PxVec3(0.f, 0.25f, 0.f)));
	PxRigidActorExt::createExclusiveShape(*base, PxBoxGeometry(0.5f, 0.25f, 1.5f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*base, 3.f);

	//Now create the slider and fixed joints...

	gArticulation->setSolverIterationCounts(32);

	PxArticulationLink* leftRoot = gArticulation->createLink(base, PxTransform(PxVec3(0.f, 0.55f, -0.9f)));
	PxRigidActorExt::createExclusiveShape(*leftRoot, PxBoxGeometry(0.5f, 0.05f, 0.05f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*leftRoot, 1.f);

	PxArticulationLink* rightRoot = gArticulation->createLink(base, PxTransform(PxVec3(0.f, 0.55f, 0.9f)));
	PxRigidActorExt::createExclusiveShape(*rightRoot, PxBoxGeometry(0.5f, 0.05f, 0.05f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*rightRoot, 1.f);

	PxArticulationJointReducedCoordinate* joint = static_cast<PxArticulationJointReducedCoordinate*>(leftRoot->getInboundJoint());
	joint->setJointType(PxArticulationJointType::eFIX);
	joint->setParentPose(PxTransform(PxVec3(0.f, 0.25f, -0.9f)));
	joint->setChildPose(PxTransform(PxVec3(0.f, -0.05f, 0.f)));

	//Set up the drive joint...	
	gDriveJoint = static_cast<PxArticulationJointReducedCoordinate*>(rightRoot->getInboundJoint());
	gDriveJoint->setJointType(PxArticulationJointType::ePRISMATIC);
	gDriveJoint->setMotion(PxArticulationAxis::eZ, PxArticulationMotion::eLIMITED);
	gDriveJoint->setLimit(PxArticulationAxis::eZ, -1.4f, 0.2f);
	gDriveJoint->setDrive(PxArticulationAxis::eZ, 100000.f, 0.f, PX_MAX_F32);

	gDriveJoint->setParentPose(PxTransform(PxVec3(0.f, 0.25f, 0.9f)));
	gDriveJoint->setChildPose(PxTransform(PxVec3(0.f, -0.05f, 0.f)));


	const PxU32 linkHeight = 3;
	PxArticulationLink* currLeft = leftRoot, *currRight = rightRoot;

	PxQuat rightParentRot(PxIdentity);
	PxQuat leftParentRot(PxIdentity);
	for (PxU32 i = 0; i < linkHeight; ++i)
	{
		const PxVec3 pos(0.5f, 0.55f + 0.1f*(1 + i), 0.f);
		PxArticulationLink* leftLink = gArticulation->createLink(currLeft, PxTransform(pos + PxVec3(0.f, sinAng*(2 * i + 1), 0.f), leftRot));
		PxRigidActorExt::createExclusiveShape(*leftLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *gMaterial);
		PxRigidBodyExt::updateMassAndInertia(*leftLink, 1.f);

		const PxVec3 leftAnchorLocation = pos + PxVec3(0.f, sinAng*(2 * i), -0.9f);

		joint = static_cast<PxArticulationJointReducedCoordinate*>(leftLink->getInboundJoint());
		joint->setParentPose(PxTransform(currLeft->getGlobalPose().transformInv(leftAnchorLocation), leftParentRot));
		joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, -1.f), rightRot));
		joint->setJointType(PxArticulationJointType::eREVOLUTE);

		leftParentRot = leftRot;

		joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
		joint->setLimit(PxArticulationAxis::eTWIST, -PxPi, angle);


		PxArticulationLink* rightLink = gArticulation->createLink(currRight, PxTransform(pos + PxVec3(0.f, sinAng*(2 * i + 1), 0.f), rightRot));
		PxRigidActorExt::createExclusiveShape(*rightLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *gMaterial);
		PxRigidBodyExt::updateMassAndInertia(*rightLink, 1.f);

		const PxVec3 rightAnchorLocation = pos + PxVec3(0.f, sinAng*(2 * i), 0.9f);

		joint = static_cast<PxArticulationJointReducedCoordinate*>(rightLink->getInboundJoint());
		joint->setJointType(PxArticulationJointType::eREVOLUTE);
		joint->setParentPose(PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation), rightParentRot));
		joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, 1.f), leftRot));
		joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
		joint->setLimit(PxArticulationAxis::eTWIST, -angle, PxPi);

		rightParentRot = rightRot;

		PxD6Joint* d6joint = PxD6JointCreate(*gPhysics, leftLink, PxTransform(PxIdentity), rightLink, PxTransform(PxIdentity));

		d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
		d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
		d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

		currLeft = rightLink;
		currRight = leftLink;
	}

	
	PxArticulationLink* leftTop = gArticulation->createLink(currLeft, currLeft->getGlobalPose().transform(PxTransform(PxVec3(-0.5f, 0.f, -1.0f), leftParentRot)));
	PxRigidActorExt::createExclusiveShape(*leftTop, PxBoxGeometry(0.5f, 0.05f, 0.05f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*leftTop, 1.f);

	PxArticulationLink* rightTop = gArticulation->createLink(currRight, currRight->getGlobalPose().transform(PxTransform(PxVec3(-0.5f, 0.f, 1.0f), rightParentRot)));
	PxRigidActorExt::createExclusiveShape(*rightTop, PxCapsuleGeometry(0.05f, 0.8f), *gMaterial);
	//PxRigidActorExt::createExclusiveShape(*rightTop, PxBoxGeometry(0.5f, 0.05f, 0.05f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*rightTop, 1.f);

	joint = static_cast<PxArticulationJointReducedCoordinate*>(leftTop->getInboundJoint());
	joint->setParentPose(PxTransform(PxVec3(0.f, 0.f, -1.f), currLeft->getGlobalPose().q.getConjugate()));
	joint->setChildPose(PxTransform(PxVec3(0.5f, 0.f, 0.f), leftTop->getGlobalPose().q.getConjugate()));
	joint->setJointType(PxArticulationJointType::eREVOLUTE);
	joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
	//joint->setDrive(PxArticulationAxis::eTWIST, 0.f, 10.f, PX_MAX_F32);

	joint = static_cast<PxArticulationJointReducedCoordinate*>(rightTop->getInboundJoint());
	joint->setParentPose(PxTransform(PxVec3(0.f, 0.f, 1.f), currRight->getGlobalPose().q.getConjugate()));
	joint->setChildPose(PxTransform(PxVec3(0.5f, 0.f, 0.f), rightTop->getGlobalPose().q.getConjugate()));
	joint->setJointType(PxArticulationJointType::eREVOLUTE);
	joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
	//joint->setDrive(PxArticulationAxis::eTWIST, 0.f, 10.f, PX_MAX_F32);


	currLeft = leftRoot;
	currRight = rightRoot;

	rightParentRot = PxQuat(PxIdentity);
	leftParentRot = PxQuat(PxIdentity);

	for (PxU32 i = 0; i < linkHeight; ++i)
	{
		const PxVec3 pos(-0.5f, 0.55f + 0.1f*(1 + i), 0.f);
		PxArticulationLink* leftLink = gArticulation->createLink(currLeft, PxTransform(pos + PxVec3(0.f, sinAng*(2 * i + 1), 0.f), leftRot));
		PxRigidActorExt::createExclusiveShape(*leftLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *gMaterial);
		PxRigidBodyExt::updateMassAndInertia(*leftLink, 1.f);

		const PxVec3 leftAnchorLocation = pos + PxVec3(0.f, sinAng*(2 * i), -0.9f);

		joint = static_cast<PxArticulationJointReducedCoordinate*>(leftLink->getInboundJoint());
		joint->setJointType(PxArticulationJointType::eREVOLUTE);
		joint->setParentPose(PxTransform(currLeft->getGlobalPose().transformInv(leftAnchorLocation), leftParentRot));
		joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, -1.f), rightRot));

		leftParentRot = leftRot;

		joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
		joint->setLimit(PxArticulationAxis::eTWIST, -PxPi, angle);

		PxArticulationLink* rightLink = gArticulation->createLink(currRight, PxTransform(pos + PxVec3(0.f, sinAng*(2 * i + 1), 0.f), rightRot));
		PxRigidActorExt::createExclusiveShape(*rightLink, PxBoxGeometry(0.05f, 0.05f, 1.f), *gMaterial);
		PxRigidBodyExt::updateMassAndInertia(*rightLink, 1.f);

		const PxVec3 rightAnchorLocation = pos + PxVec3(0.f, sinAng*(2 * i), 0.9f);

		/*joint = PxD6JointCreate(getPhysics(), currRight, PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation)),
		rightLink, PxTransform(PxVec3(0.f, 0.f, 1.f)));*/

		joint = static_cast<PxArticulationJointReducedCoordinate*>(rightLink->getInboundJoint());
		joint->setParentPose(PxTransform(currRight->getGlobalPose().transformInv(rightAnchorLocation), rightParentRot));
		joint->setJointType(PxArticulationJointType::eREVOLUTE);
		joint->setChildPose(PxTransform(PxVec3(0.f, 0.f, 1.f), leftRot));
		joint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eLIMITED);
		joint->setLimit(PxArticulationAxis::eTWIST, -angle, PxPi);

		rightParentRot = rightRot;

		PxD6Joint* d6joint = PxD6JointCreate(*gPhysics, leftLink, PxTransform(PxIdentity), rightLink, PxTransform(PxIdentity));

		d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
		d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
		d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

		currLeft = rightLink;
		currRight = leftLink;
	}

	PxD6Joint* d6joint = PxD6JointCreate(*gPhysics, currLeft, PxTransform(PxVec3(0.f, 0.f, -1.f)), leftTop, PxTransform(PxVec3(-0.5f, 0.f, 0.f)));

	d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);

	d6joint = PxD6JointCreate(*gPhysics, currRight, PxTransform(PxVec3(0.f, 0.f, 1.f)), rightTop, PxTransform(PxVec3(-0.5f, 0.f, 0.f)));

	d6joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	d6joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	d6joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);


	const PxTransform topPose(PxVec3(0.f, leftTop->getGlobalPose().p.y + 0.15f, 0.f));

	PxArticulationLink* top = gArticulation->createLink(leftTop, topPose);
	PxRigidActorExt::createExclusiveShape(*top, PxBoxGeometry(0.5f, 0.1f, 1.5f), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*top, 1.f);

	joint = static_cast<PxArticulationJointReducedCoordinate*>(top->getInboundJoint());
	joint->setJointType(PxArticulationJointType::eFIX);
	joint->setParentPose(PxTransform(PxVec3(0.f, 0.0f, 0.f)));
	joint->setChildPose(PxTransform(PxVec3(0.f, -0.15f, -0.9f)));

	gScene->addArticulation(*gArticulation);

	for (PxU32 i = 0; i < gArticulation->getNbLinks(); ++i)
	{
		PxArticulationLink* link;
		gArticulation->getLinks(&link, 1, i);

		link->setLinearDamping(0.2f);
		link->setAngularDamping(0.2f);

		link->setMaxAngularVelocity(20.f);
		link->setMaxLinearVelocity(100.f);

		if (link != top)
		{
			for (PxU32 b = 0; b < link->getNbShapes(); ++b)
			{
				PxShape* shape;
				link->getShapes(&shape, 1, b);

				shape->setSimulationFilterData(PxFilterData(0, 0, 1, 0));
			}
		}
	}

	const PxVec3 halfExt(0.25f);
	const PxReal density(0.5f);

	PxRigidDynamic* box0 = gPhysics->createRigidDynamic(PxTransform(PxVec3(-0.25f, 5.f, 0.5f)));
	PxRigidActorExt::createExclusiveShape(*box0, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box0, density);

	gScene->addActor(*box0);

	PxRigidDynamic* box1 = gPhysics->createRigidDynamic(PxTransform(PxVec3(0.25f, 5.f, 0.5f)));
	PxRigidActorExt::createExclusiveShape(*box1, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box1, density);

	gScene->addActor(*box1);

	PxRigidDynamic* box2 = gPhysics->createRigidDynamic(PxTransform(PxVec3(-0.25f, 4.5f, 0.5f)));
	PxRigidActorExt::createExclusiveShape(*box2, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box2, density);

	gScene->addActor(*box2);

	PxRigidDynamic* box3 = gPhysics->createRigidDynamic(PxTransform(PxVec3(0.25f, 4.5f, 0.5f)));
	PxRigidActorExt::createExclusiveShape(*box3, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box3, density);

	gScene->addActor(*box3);

	PxRigidDynamic* box4 = gPhysics->createRigidDynamic(PxTransform(PxVec3(-0.25f, 5.f, 0.f)));
	PxRigidActorExt::createExclusiveShape(*box4, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box4, density);

	gScene->addActor(*box4);

	PxRigidDynamic* box5 = gPhysics->createRigidDynamic(PxTransform(PxVec3(0.25f, 5.f, 0.f)));
	PxRigidActorExt::createExclusiveShape(*box5, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box5, density);

	gScene->addActor(*box5);

	PxRigidDynamic* box6 = gPhysics->createRigidDynamic(PxTransform(PxVec3(-0.25f, 4.5f, 0.f)));
	PxRigidActorExt::createExclusiveShape(*box6, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box6, density);

	gScene->addActor(*box6);

	PxRigidDynamic* box7 = gPhysics->createRigidDynamic(PxTransform(PxVec3(0.25f, 4.5f, 0.f)));
	PxRigidActorExt::createExclusiveShape(*box7, PxBoxGeometry(halfExt), *gMaterial);
	PxRigidBodyExt::updateMassAndInertia(*box7, density);

	gScene->addActor(*box7);
}

#endif

void createLongChain()
{
	const float scale = 0.25f;
	const float radius = 0.5f*scale;
	const float halfHeight = 1.0f*scale;
	const PxU32 nbCapsules = 40;
	const float capsuleMass = 1.0f;

	const PxVec3 initPos(0.0f, 24.0f, 0.0f);
	PxVec3 pos = initPos;
	PxShape* capsuleShape = gPhysics->createShape(PxCapsuleGeometry(radius, halfHeight), *gMaterial);
	PxArticulationLink* firstLink = NULL;
	PxArticulationLink* parent = NULL;

	const bool overlappingLinks = true;	// Change this for another kind of rope

	gArticulation->setSolverIterationCounts(16);

											// Create rope
	for (PxU32 i = 0; i<nbCapsules; i++)
	{
		PxArticulationLink* link = gArticulation->createLink(parent, PxTransform(pos));
		if (!firstLink)
			firstLink = link;

		link->attachShape(*capsuleShape);
		PxRigidBodyExt::setMassAndUpdateInertia(*link, capsuleMass);

		link->setLinearDamping(0.1f);
		link->setAngularDamping(0.1f);

		link->setMaxAngularVelocity(30.f);
		link->setMaxLinearVelocity(100.f);

		PxArticulationJointBase* joint = link->getInboundJoint();

		if (joint)	// Will be null for root link
		{
#if USE_REDUCED_COORDINATE_ARTICULATION
			PxArticulationJointReducedCoordinate* rcJoint = static_cast<PxArticulationJointReducedCoordinate*>(joint);
			rcJoint->setJointType(PxArticulationJointType::eSPHERICAL);
			rcJoint->setMotion(PxArticulationAxis::eSWING2, PxArticulationMotion::eFREE);
			rcJoint->setMotion(PxArticulationAxis::eSWING1, PxArticulationMotion::eFREE);
			rcJoint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
			rcJoint->setFrictionCoefficient(1.f);
			rcJoint->setMaxJointVelocity(1000000.f);
#endif
			if (overlappingLinks)
			{
				joint->setParentPose(PxTransform(PxVec3(halfHeight, 0.0f, 0.0f)));
				joint->setChildPose(PxTransform(PxVec3(-halfHeight, 0.0f, 0.0f)));
			}
			else
			{
				joint->setParentPose(PxTransform(PxVec3(radius + halfHeight, 0.0f, 0.0f)));
				joint->setChildPose(PxTransform(PxVec3(-radius - halfHeight, 0.0f, 0.0f)));
			}
		}

		if (overlappingLinks)
			pos.x += (radius + halfHeight*2.0f);
		else
			pos.x += (radius + halfHeight) * 2.0f;
		parent = link;
	}

	//Attach large & heavy box at the end of the rope
	{
		const float boxMass = 50.0f;
		const float boxSize = 1.0f;
		PxShape* boxShape = gPhysics->createShape(PxBoxGeometry(boxSize, boxSize, boxSize), *gMaterial);

		pos.x -= (radius + halfHeight) * 2.0f;
		pos.x += (radius + halfHeight) + boxSize;

		PxArticulationLink* link = gArticulation->createLink(parent, PxTransform(pos));

		link->setLinearDamping(0.1f);
		link->setAngularDamping(0.1f);
		link->setMaxAngularVelocity(30.f);
		link->setMaxLinearVelocity(100.f);

		link->attachShape(*boxShape);
		PxRigidBodyExt::setMassAndUpdateInertia(*link, boxMass);

		PxArticulationJointBase* joint = link->getInboundJoint();
#if USE_REDUCED_COORDINATE_ARTICULATION
		PxArticulationJointReducedCoordinate* rcJoint = static_cast<PxArticulationJointReducedCoordinate*>(joint);
		rcJoint->setJointType(PxArticulationJointType::eSPHERICAL);
		rcJoint->setMotion(PxArticulationAxis::eSWING2, PxArticulationMotion::eFREE);
		rcJoint->setMotion(PxArticulationAxis::eSWING1, PxArticulationMotion::eFREE);
		rcJoint->setMotion(PxArticulationAxis::eTWIST, PxArticulationMotion::eFREE);
		rcJoint->setFrictionCoefficient(1.f);
		rcJoint->setMaxJointVelocity(1000000.f);
#endif
		if (joint)	// Will be null for root link
		{
			joint->setParentPose(PxTransform(PxVec3(radius + halfHeight, 0.0f, 0.0f)));
			joint->setChildPose(PxTransform(PxVec3(-boxSize, 0.0f, 0.0f)));
		}
	}
	gScene->addArticulation(*gArticulation);

#if USE_REDUCED_COORDINATE_ARTICULATION
	gArticulation->setArticulationFlags(PxArticulationFlag::eFIX_BASE);
#else
	// Attach articulation to static world
	{
		PxShape* anchorShape = gPhysics->createShape(PxSphereGeometry(0.05f), *gMaterial);
		PxRigidStatic* anchor = PxCreateStatic(*gPhysics, PxTransform(initPos), *anchorShape);
		gScene->addActor(*anchor);
		PxSphericalJoint* j = PxSphericalJointCreate(*gPhysics, anchor, PxTransform(PxVec3(0.0f)), firstLink, PxTransform(PxVec3(0.0f)));
		PX_UNUSED(j);
	}
#endif

	// Create obstacle
	{
		PxShape* boxShape = gPhysics->createShape(PxBoxGeometry(1.0f, 0.1f, 2.0f), *gMaterial);
		PxRigidStatic* obstacle = PxCreateStatic(*gPhysics, PxTransform(initPos + PxVec3(10.0f, -3.0f, 0.0f)), *boxShape);
		gScene->addActor(*obstacle);
	}
}


void initPhysics(bool /*interactive*/)
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPvd = PxCreatePvd(*gFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
	PxInitExtensions(*gPhysics, gPvd);

	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	
	PxU32 numCores = SnippetUtils::getNbPhysicalCores();
	gDispatcher = PxDefaultCpuDispatcherCreate(numCores == 0 ? 0 : numCores - 1);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader	= PxDefaultSimulationFilterShader;

#if USE_REDUCED_COORDINATE_ARTICULATION
	sceneDesc.solverType = PxSolverType::eTGS;
#if CREATE_SCISSOR_LIFT
	sceneDesc.filterShader = scissorFilter;
#endif

#endif

	gScene = gPhysics->createScene(sceneDesc);
	PxPvdSceneClient* pvdClient = gScene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0,1,0,0), *gMaterial);
	gScene->addActor(*groundPlane);

#if USE_REDUCED_COORDINATE_ARTICULATION
	gArticulation = gPhysics->createArticulationReducedCoordinate();
#else
	gArticulation = gPhysics->createArticulation();

	// Stabilization can create artefacts on jointed objects so we just disable it
	gArticulation->setStabilizationThreshold(0.0f);

	gArticulation->setMaxProjectionIterations(16);
	gArticulation->setSeparationTolerance(0.001f);
#endif

#if USE_REDUCED_COORDINATE_ARTICULATION & CREATE_SCISSOR_LIFT
	createScissorLift();
#else
	createLongChain();
#endif
}

#if USE_REDUCED_COORDINATE_ARTICULATION
	static bool gClosing = true;
#endif

void stepPhysics(bool /*interactive*/)
{
	const PxReal dt = 1.0f / 60.f;
#if USE_REDUCED_COORDINATE_ARTICULATION & CREATE_SCISSOR_LIFT
	PxReal driveValue = gDriveJoint->getDriveTarget(PxArticulationAxis::eZ);

	if (gClosing && driveValue < -1.2f)
		gClosing = false;
	else if (!gClosing && driveValue > 0.f)
		gClosing = true;

	if (gClosing)
		driveValue -= dt*0.25f;
	else
		driveValue += dt*0.25f;
	gDriveJoint->setDriveTarget(PxArticulationAxis::eZ, driveValue);
#endif

	gScene->simulate(dt);
	gScene->fetchResults(true);
}
	
void cleanupPhysics(bool /*interactive*/)
{
	gArticulation->release();
	gScene->release();
	gDispatcher->release();
	gPhysics->release();	
	PxPvdTransport* transport = gPvd->getTransport();
	gPvd->release();
	transport->release();
	PxCloseExtensions();  
	gFoundation->release();

	printf("SnippetArticulation done.\n");
}

void keyPress(unsigned char /*key*/, const PxTransform& /*camera*/)
{
}

int snippetMain(int, const char*const*)
{
#ifdef RENDER_SNIPPET
	extern void renderLoop();
	renderLoop();
#else
	static const PxU32 frameCount = 100;
	initPhysics(false);
	for(PxU32 i=0; i<frameCount; i++)
		stepPhysics(false);
	cleanupPhysics(false);
#endif

	return 0;
}
