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

#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"
#include "SampleSubmarine.h"
#include "Crab.h"
#include "RendererColor.h"
#include "RenderPhysX3Debug.h"

#include "PxTkStream.h"

#include "PhysXSample.h"
#include "PxTkFile.h"
using namespace PxToolkit;
// if enabled: runs the crab AI in sync, not as a parallel task to physx.
#define DEBUG_RENDERING 0

void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask);

// table with default times in seconds how the crab AI will try to stay in a state
static const PxReal gDefaultStateTime[CrabState::eNUM_STATES] = {5.0f, 10.0f, 10.0f, 10.0f, 10.0f, 6.0f};

Crab::Crab(SampleSubmarine& sample, const PxVec3& crabPos, RenderMaterial* material)
: ClassType(ClassType::eCRAB)
, mSampleSubmarine(&sample)
, mMaterial(material)
{
	initMembers();
	create(crabPos);
}

Crab::Crab(SampleSubmarine& sample, const char* filename, RenderMaterial* material)
: ClassType(ClassType::eCRAB)
, mSampleSubmarine(&sample)
, mMaterial(material)
{
	initMembers();
	load(filename);
}

void Crab::initMembers()
{
	mMemory = NULL;
	mCrabBody = NULL;
	mSqRayBuffer = NULL;
	mLegHeight = 0;
	mRespawnMe = false;
	mCrabState = CrabState::eWAITING;
	mStateTime = gDefaultStateTime[CrabState::eWAITING];
	mAccumTime = 0;
	mElapsedTime = 0;
	mRunning = 0;

	mAcceleration[0] = 0;
	mAcceleration[1] = 0;

	mAccelerationBuffer[0] = 0;
	mAccelerationBuffer[1] = 0;

	// setup buffer for 10 batched rays and 10 hits
	mSqRayBuffer = SAMPLE_NEW(SqRayBuffer)(*mSampleSubmarine, 10, 10);

	mSubmarinePos = PxVec3(0);

}
Crab::~Crab()
{
	// wait until background task is finished
	while(mRunning)
		;

	for(PxU32 i = 0; i < mJoints.size(); i++)
		mJoints[i]->release();
	mJoints.clear();

	for(PxU32 i = 0; i < mActors.size(); i++)
		mSampleSubmarine->removeActor(mActors[i]);
	mActors.clear();
	
	if(mMemory)
	{
		SAMPLE_FREE(mMemory);
	}

	delete mSqRayBuffer;
}

static void setShapeFlag(PxRigidActor* actor, PxShapeFlag::Enum flag, bool flagValue)
{
	const PxU32 numShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)SAMPLE_ALLOC(sizeof(PxShape*)*numShapes);
	actor->getShapes(shapes, numShapes);
	for(PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setFlag(flag, flagValue);
	}
	SAMPLE_FREE(shapes);
}

PxVec3 Crab::getPlaceOnFloor(PxVec3 start)
{
	PxRaycastBuffer rayHit;
	mSampleSubmarine->getActiveScene().raycast(start, PxVec3(0,-1,0), 1000.0f, rayHit);

	return rayHit.block.position + PxVec3(0,mLegHeight,0);
}

static const PxSerialObjectId mMaterial_id		= (PxSerialObjectId)0x01;
static const PxSerialObjectId mCrabBody_id		= (PxSerialObjectId)0x02;
static const PxSerialObjectId mMotorJoint0_id	= (PxSerialObjectId)0x03;
static const PxSerialObjectId mMotorJoint1_id	= (PxSerialObjectId)0x04; 

void Crab::save(const char* filename)
{
	PxPhysics& physics = mSampleSubmarine->getPhysics();
	PxCollection* thePxCollection = PxCreateCollection();
	PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(physics);	
	for(PxU32 i = 0; i < mActors.size(); ++i)
	{		
		thePxCollection->add(*mActors[i]);
	}
	
	for(PxU32 i = 0; i < mJoints.size(); ++i)
	{
		thePxCollection->add(*mJoints[i]);
	}
	
	thePxCollection->addId(*mCrabBody,		 mCrabBody_id);
	thePxCollection->addId(*mMotorJoint[0],  mMotorJoint0_id);
	thePxCollection->addId(*mMotorJoint[1],  mMotorJoint1_id);
	
	PxCollection* theExtRef = PxCreateCollection();
	theExtRef->add(*mSampleSubmarine->mMaterial, mMaterial_id);
	
	PxSerialization::complete(*thePxCollection, *sr, theExtRef);

	PxDefaultFileOutputStream s(filename);
	PxSerialization::serializeCollectionToBinary(s, *thePxCollection, *sr, theExtRef);
	
	theExtRef->release();
	thePxCollection->release();
	sr->release();
}

static PxU32 GetFileSize(const char* name)
{
	if(!name)	return 0;

#ifndef SEEK_END
#define SEEK_END 2
#endif

	SampleFramework::File* fp;
	if (PxToolkit::fopen_s(&fp, name, "rb"))
		return 0;
	fseek(fp, 0, SEEK_END);
	PxU32 eof_ftell = (PxU32)ftell(fp);
	fclose(fp);
	return eof_ftell;
}

void Crab::load(const char* filename)
{
	PxPhysics& thePhysics = mSampleSubmarine->getPhysics();

	SampleFramework::File* fp = NULL;
	if (!PxToolkit::fopen_s(&fp, filename, "rb"))
	{
		PxU32 theFileSize = GetFileSize(filename);
		
		if(!mMemory)
			mMemory = SAMPLE_ALLOC(theFileSize + PX_SERIAL_FILE_ALIGN);

		void* theMemory16 = (void*)((size_t(mMemory) + PX_SERIAL_FILE_ALIGN)&~(PX_SERIAL_FILE_ALIGN-1));
		const size_t theNumRead = fread(theMemory16, 1, theFileSize, fp);
		PX_ASSERT(PxU32(theNumRead) == theFileSize);
		PX_UNUSED(theNumRead);
		fclose(fp);

		PxCollection* theExtRef = PxCreateCollection();
		theExtRef->add(*mSampleSubmarine->mMaterial, mMaterial_id);
		PxSerializationRegistry* sr = PxSerialization::createSerializationRegistry(thePhysics);
		PxCollection* thePxCollection = PxSerialization::createCollectionFromBinary(theMemory16, *sr, theExtRef);
		PX_ASSERT(thePxCollection);

		mSampleSubmarine->getActiveScene().addCollection(*thePxCollection);
			
		mMotorJoint[0] = reinterpret_cast<PxRevoluteJoint*>( thePxCollection->find(mMotorJoint0_id));
		mMotorJoint[1] = reinterpret_cast<PxRevoluteJoint*>( thePxCollection->find(mMotorJoint1_id));
		mCrabBody = reinterpret_cast<PxRigidDynamic*>( thePxCollection->find(mCrabBody_id));
		PX_ASSERT(mMotorJoint[0] && mMotorJoint[1] && mCrabBody );

		PxU32 nbObjs = thePxCollection->getNbObjects();
		PX_ASSERT(nbObjs != 0);
		for(PxU32 i = 0; i < nbObjs; ++i)
		{
			PxBase* object = &thePxCollection->getObject(i);
			if(object)
			{
				const PxType serialType = object->getConcreteType();
				if(serialType == PxConcreteType::eRIGID_DYNAMIC)
				{
					PxRigidDynamic* actor = reinterpret_cast<PxRigidDynamic*>(object);

					mSampleSubmarine->createRenderObjectsFromActor(actor , mMaterial ); 
					mSampleSubmarine->addPhysicsActors( actor );
					mActors.push_back( actor );
				}
				else if(serialType == PxConcreteType::eCONSTRAINT)
				{
					PxU32 typeID = 0;
					PxConstraint* constraint = reinterpret_cast<PxConstraint*>(object);
					PxJoint* joint = reinterpret_cast<PxJoint*>(constraint->getExternalReference(typeID));
					mJoints.push_back( joint );
				}
				else if(serialType == PxConcreteType::eSHAPE)
				{
					//giving up application shape ownership early
					PxShape* shape = reinterpret_cast<PxShape*>(object);
					shape->release();
				}
			}
		}
		
		theExtRef->release();
		thePxCollection->release();
		sr->release();
	}

	if( !mCrabBody ) mSampleSubmarine->fatalError( "createBox failed!" );
	setupFiltering( mCrabBody, FilterGroup::eCRAB, FilterGroup::eHEIGHTFIELD );
}


void Crab::create(const PxVec3& _crabPos)
{
	static const PxReal scale = 0.8f;
	static const PxReal crabDepth = 2.0f;
	static const PxVec3 crabBodyDim = PxVec3(0.8f, 0.8f, crabDepth*0.5f)*scale;
	static const PxReal legMass = 0.03f;
	static const PxReal velocity = 0.0f;
	static const PxReal maxForce = 4000.0f;

	LegParameters params; // check edge ascii art in Crab.h
	params.a = 0.5f;
	params.b = 0.6f;
	params.c = 0.5f;
	params.d = 0.5f;
	params.e = 1.5f;
	params.m = 0.3f;
	params.n = 0.1f;

	mLegHeight = scale*2.0f*(params.d+params.c);
	mLegHeight += 0.5f;
	PxVec3 crabPos = getPlaceOnFloor(_crabPos); 
	mCrabBody = mSampleSubmarine->createBox(crabPos, crabBodyDim, NULL, mMaterial, 1.0f)->is<PxRigidDynamic>();
	if(!mCrabBody) mSampleSubmarine->fatalError("createBox failed!");
	PxShape* shape; mCrabBody->getShapes(&shape, 1);
	shape->setLocalPose(PxTransform(PxQuat(PxHalfPi*0.5f, PxVec3(0,0,1))));
	PxRigidBodyExt::setMassAndUpdateInertia(*mCrabBody, legMass*10.0f);
	PxTransform cmPose = mCrabBody->getCMassLocalPose();
	cmPose.p.y -= 0.8f;
	mCrabBody->setCMassLocalPose(cmPose);
	mCrabBody->setAngularDamping(100.0f);
	mCrabBody->userData = this;
	mActors.push_back(mCrabBody);

	// legs
	static const PxU32 numLegs = 4;
	PxReal recipNumLegs = 1.0f/PxReal(numLegs);
	PxReal recipNumLegsMinus1 = 1.0f/PxReal(numLegs-1);
	PX_COMPILE_TIME_ASSERT((numLegs&1) == 0);

	PxRigidDynamic* motor[2];
	{
		static const PxReal density = 1.0f;
		static const PxReal m = params.m * scale;
		static const PxReal n = params.n * scale;
		static const PxBoxGeometry boxGeomM = PxBoxGeometry(m, m, crabBodyDim.z * 0.5f);

		// create left and right motor
		PxVec3 motorPos = crabPos+PxVec3(0,n,0);
		for(PxU32 i = 0; i < 2; i++)
		{
			PxVec3 motorOfs = i==0 ? PxVec3(0,0, boxGeomM.halfExtents.z) : -PxVec3(0,0,boxGeomM.halfExtents.z);
			motor[i] = mSampleSubmarine->createBox(motorPos+motorOfs, boxGeomM.halfExtents, NULL, mMaterial, density)->is<PxRigidDynamic>();
			if(!motor[i]) mSampleSubmarine->fatalError("createBox failed!");
			
			PxRigidBodyExt::setMassAndUpdateInertia(*motor[i], legMass);
			motor[i]->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
			setShapeFlag(motor[i], PxShapeFlag::eSIMULATION_SHAPE, false);

			mMotorJoint[i] = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
				mCrabBody,		PxTransform(motorOfs,			PxQuat(-PxHalfPi, PxVec3(0,1,0))),
				motor[i],		PxTransform(PxVec3(0, 0, 0),	PxQuat(-PxHalfPi, PxVec3(0,1,0))));
			if(!mMotorJoint[i]) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");

			mMotorJoint[i]->setDriveVelocity(velocity);
			mMotorJoint[i]->setDriveForceLimit(maxForce);
			mMotorJoint[i]->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);
			mActors.push_back(motor[i]);
			mJoints.push_back(mMotorJoint[i]);
		}
		
		// create legs and attach to left and right motor
		PxReal legSpacing = crabDepth*recipNumLegsMinus1*scale;
		PxVec3 bodyToLegPos0 = PxVec3(0, 0, crabBodyDim.z);
		PxVec3 bodyToLegPos1 = PxVec3(0, 0, crabBodyDim.z-(numLegs/2)*legSpacing);
		PxVec3 motorToLegPos0 = PxVec3(0, 0, crabBodyDim.z*0.5f);
		PxVec3 motorToLegPos1 = PxVec3(0, 0, (crabBodyDim.z - legSpacing)*0.5f);
		for(PxU32 i = 0; i < numLegs/2; i++)
		{
			PxReal angle0 = -PxHalfPi + PxTwoPi*recipNumLegs*i;
			PxReal angle1 =  angle0 + PxPi;

			createLeg(mCrabBody, bodyToLegPos0, legMass, params, scale, motor[0],	motorToLegPos0 + m * PxVec3(PxCos(angle0), PxSin(angle0), 0));
			createLeg(mCrabBody, bodyToLegPos1, legMass, params, scale, motor[1],	motorToLegPos1 + m * PxVec3(PxCos(angle1), PxSin(angle1), 0));
			bodyToLegPos0.z -= legSpacing;
			bodyToLegPos1.z -= legSpacing;
			motorToLegPos0.z -= legSpacing;
			motorToLegPos1.z -= legSpacing;
		}
	}

	setupFiltering(mCrabBody, FilterGroup::eCRAB, FilterGroup::eHEIGHTFIELD);
}

void Crab::createLeg(PxRigidDynamic* mainBody, PxVec3 localPos, PxReal mass, const LegParameters& params, PxReal scale, PxRigidDynamic* motor, PxVec3 motorAttachmentPos)
{
	PxVec3 crabLegPos = mainBody->getGlobalPose().p + localPos;

	// params for Theo Jansen's machine
	// check edge ascii art in Crab.h
	static const PxReal stickExt = 0.125f * 0.5f * scale;
	const PxReal a = params.a * scale;
	const PxReal b = params.b * scale;
	const PxReal c = params.c * scale;
	const PxReal d = params.d * scale;
	const PxReal e = params.e * scale;
	const PxReal m = params.m * scale;
	const PxReal n = params.n * scale;

	const PxReal density = 1.0f;

	std::vector<PxTransform> poses;
	std::vector<const PxGeometry*> geometries;

	PxBoxGeometry boxGeomA = PxBoxGeometry(a, stickExt, stickExt);
	PxBoxGeometry boxGeomB = PxBoxGeometry(stickExt, b, stickExt);
	PxBoxGeometry boxGeomC = PxBoxGeometry(stickExt, c, stickExt);

	PxCapsuleGeometry capsGeomD = PxCapsuleGeometry(stickExt*2.0f, d);

	for(PxU32 leg = 0; leg < 2; leg++)
	{
		bool left = (leg==0);
		#define MIRROR(X) left ? -1.0f*(X) : (X)
		PxVec3 startPos = crabLegPos + PxVec3(MIRROR(e), 0, 0);

		// create upper triangle from boxes
		PxRigidDynamic* upperTriangle = NULL;
		{
			PxTransform poseA = PxTransform(PxVec3(MIRROR(a), 0, 0));
			PxTransform poseB = PxTransform(PxVec3(MIRROR(0), b, 0));
			poses.clear(); geometries.clear();
			poses.push_back(poseA); poses.push_back(poseB);
			geometries.push_back(&boxGeomA); geometries.push_back(&boxGeomB);
			upperTriangle = mSampleSubmarine->createCompound(startPos, poses, geometries, NULL, mMaterial, density)->is<PxRigidDynamic>();
			if(!upperTriangle) mSampleSubmarine->fatalError("createCompound failed!");
			mActors.push_back(upperTriangle);
		}

		// create lower triangle from boxes
		PxRigidDynamic* lowerTriangle = NULL;
		{
			PxTransform poseA = PxTransform(PxVec3(MIRROR(a), 0, 0));
			//PxTransform poseD = PxTransform(PxVec3(MIRROR(0), -d, 0));
			PxTransform poseD = PxTransform(PxVec3(MIRROR(0), -d, 0), PxQuat(PxHalfPi, PxVec3(0,0,1)));
			poses.clear(); geometries.clear();
			poses.push_back(poseA); poses.push_back(poseD);
			//geometries.push_back(&boxGeomA); geometries.push_back(&boxGeomD);
			geometries.push_back(&boxGeomA); geometries.push_back(&capsGeomD);
			lowerTriangle = mSampleSubmarine->createCompound(startPos + PxVec3(0, -2.0f*c, 0), poses, geometries, NULL, mMaterial, density)->is<PxRigidDynamic>();
			if(!lowerTriangle) mSampleSubmarine->fatalError("createCompound failed!");
			mActors.push_back(lowerTriangle);
		}

		// create vertical boxes to connect the triangles
		PxRigidDynamic* verticalBox0 = mSampleSubmarine->createBox(startPos + PxVec3(0, -c, 0), boxGeomC.halfExtents ,NULL, mMaterial, density)->is<PxRigidDynamic>();
		if(!verticalBox0) mSampleSubmarine->fatalError("createBox failed!");
		PxRigidDynamic* verticalBox1 = mSampleSubmarine->createBox(startPos + PxVec3(MIRROR(2.0f*a), -c, 0), boxGeomC.halfExtents ,NULL, mMaterial, density)->is<PxRigidDynamic>();
		if(!verticalBox1) mSampleSubmarine->fatalError("createBox failed!");
		mActors.push_back(verticalBox0);
		mActors.push_back(verticalBox1);

		// disable gravity
		upperTriangle->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		lowerTriangle->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		verticalBox0->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);
		verticalBox1->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, true);

		// set mass
		PxRigidBodyExt::setMassAndUpdateInertia(*upperTriangle, mass); 
		PxRigidBodyExt::setMassAndUpdateInertia(*lowerTriangle, mass); 
		PxRigidBodyExt::setMassAndUpdateInertia(*verticalBox0, mass); 
		PxRigidBodyExt::setMassAndUpdateInertia(*verticalBox1, mass); 

		// turn off collision upper triangle and vertical boxes
		setShapeFlag(upperTriangle, PxShapeFlag::eSIMULATION_SHAPE, false);
		setShapeFlag(verticalBox0, PxShapeFlag::eSIMULATION_SHAPE, false);
		setShapeFlag(verticalBox1, PxShapeFlag::eSIMULATION_SHAPE, false);

		// revolute joint in lower corner of upper triangle
		PxRevoluteJoint* joint;
		joint = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
			mainBody, PxTransform(PxVec3(MIRROR(e), 0, 0)+localPos, PxQuat(-PxHalfPi, PxVec3(0,1,0))),
			upperTriangle, PxTransform(PxVec3(0, 0, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))));
		if(!joint) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");
		mJoints.push_back(joint);

		// 4 revolute joints to connect triangles
		joint = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
			upperTriangle, PxTransform(PxVec3(0, 0, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))),
			verticalBox0, PxTransform(PxVec3(0, c, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))));
		if(!joint) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");
		mJoints.push_back(joint);

		joint = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
			upperTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0),PxQuat(-PxHalfPi, PxVec3(0,1,0))),
			verticalBox1, PxTransform(PxVec3(0, c, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))));
		if(!joint) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");
		mJoints.push_back(joint);

		joint = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
			lowerTriangle, PxTransform(PxVec3(0, 0, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))),
			verticalBox0, PxTransform(PxVec3(0, -c, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))));
		if(!joint) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");
		mJoints.push_back(joint);

		joint = PxRevoluteJointCreate(mSampleSubmarine->getPhysics(),
			lowerTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0),PxQuat(-PxHalfPi, PxVec3(0,1,0))),
			verticalBox1, PxTransform(PxVec3(0, -c, 0),				PxQuat(-PxHalfPi, PxVec3(0,1,0))));
		if(!joint) mSampleSubmarine->fatalError("PxRevoluteJointCreate failed!");
		mJoints.push_back(joint);

		// 2 distance constraints to connect motor with the triangles
		PxTransform motorTransform = PxTransform(motorAttachmentPos);
		PxReal dist0 = PxSqrt( (2.0f*b - n)*(2.0f*b - n) + (e-m)*(e-m));
		PxReal dist1 = PxSqrt( (2.0f*c + n)*(2.0f*c + n) + (e-m)*(e-m));
		
		PxDistanceJoint* distJoint0 = PxDistanceJointCreate(mSampleSubmarine->getPhysics(), upperTriangle, PxTransform(PxVec3(0, 2.0f*b, 0)), motor, motorTransform);
		if(!distJoint0) mSampleSubmarine->fatalError("PxDistanceJointCreate failed!");
		// set min & max distance to dist0
		distJoint0->setMaxDistance(dist0);
		distJoint0->setMinDistance(dist0);
		// setup damping & spring
		distJoint0->setDamping(0.1f);
		distJoint0->setStiffness(100.0f);
		distJoint0->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

		PxDistanceJoint* distJoint1 = PxDistanceJointCreate(mSampleSubmarine->getPhysics(), lowerTriangle, PxTransform(PxVec3(0, 0, 0)), motor, motorTransform);
		if(!distJoint1) mSampleSubmarine->fatalError("PxDistanceJointCreate failed!");
		// set min & max distance to dist0
		distJoint1->setMaxDistance(dist1);
		distJoint1->setMinDistance(dist1);
		// setup damping & spring
		distJoint1->setDamping(0.1f);
		distJoint1->setStiffness(100.0f);
		distJoint1->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

		// one distance joint to ensure that the vertical boxes do not get stuck if they cross the diagonal.
		PxReal halfDiagDist = PxSqrt(a*a + c*c);
		PxDistanceJoint* noFlip = PxDistanceJointCreate(mSampleSubmarine->getPhysics(), lowerTriangle, PxTransform(PxVec3(MIRROR(2.0f*a), 0, 0)), upperTriangle, PxTransform(PxVec3(0)));
		if(!noFlip) mSampleSubmarine->fatalError("PxDistanceJointCreate failed!");
		// set min & max distance to dist0
		noFlip->setMaxDistance(2.0f * (a+c));
		noFlip->setMinDistance(halfDiagDist);
		// setup damping & spring
		noFlip->setDamping(1.0f);
		noFlip->setStiffness(100.0f);
		noFlip->setDistanceJointFlags(PxDistanceJointFlag::eMAX_DISTANCE_ENABLED | PxDistanceJointFlag::eMIN_DISTANCE_ENABLED | PxDistanceJointFlag::eSPRING_ENABLED);

		mJoints.push_back(distJoint0);
		mJoints.push_back(distJoint1);
		mJoints.push_back(noFlip);
	}
}


void Crab::update(PxReal dt)
{
	PxSceneWriteLock scopedLock(mSampleSubmarine->getActiveScene());

	{
		// check if I have to be reset
		PxTransform pose = mCrabBody->getGlobalPose();
		PxVec3 upVect = PxVec3(0,1,0);
		PxVec3 crabUp = pose.rotate(upVect);
		PxReal angle = upVect.dot(crabUp);
		if(angle <= 0.1f)
		{
			mRespawnMe = true;
		}
	}

	PxReal maxVelocity = 16.0f;
	PxReal velDamping = 0.8f;

	if(mRunning == 0)
		flushAccelerationBuffer();

	for(PxU32 i = 0; i < 2; i++)
	{
		PxReal prevVelocity = mMotorJoint[i]->getDriveVelocity();
		PxReal velocityChange = mAcceleration[i] ? mAcceleration[i]*dt : -prevVelocity*velDamping*dt;
		PxReal newVelocity = PxClamp(prevVelocity + velocityChange, -maxVelocity, maxVelocity);
		mMotorJoint[i]->setDriveVelocity(newVelocity);

		if(mAcceleration[i] != 0.0f)
			mCrabBody->wakeUp();

		mAcceleration[i] = 0;
	}

	// add up elapsed time
	mAccumTime += dt;

	// submit accum time to AI time before starting the PxTask
	if(mRunning == 0)
	{
		mElapsedTime = mAccumTime;
		mAccumTime = 0;
		mSubmarinePos = mSampleSubmarine->mSubmarineActor ? mSampleSubmarine->mSubmarineActor->getGlobalPose().p : PxVec3(0);

#if DEBUG_RENDERING
		// run immediately
		scanForObstacles();
		updateState();
#endif
	}
}

void Crab::run()
{
#if !DEBUG_RENDERING
	mRunning = 1;

	// run as a separate task/thread
	scanForObstacles();
	updateState();

	mRunning = 0;
#endif
}


void Crab::setAcceleration(PxReal leftAcc, PxReal rightAcc)
{
	mAccelerationBuffer[0] = -leftAcc;
	mAccelerationBuffer[1] = -rightAcc;
}


void Crab::flushAccelerationBuffer()
{
	mAcceleration[0] =  mAccelerationBuffer[0];
	mAcceleration[1] =  mAccelerationBuffer[1];
}


void Crab::scanForObstacles()
{
	PxSceneReadLock scopedLock(mSampleSubmarine->getActiveScene());

	PxTransform crabPose = mCrabBody->getGlobalPose();
	PxVec3 rayStart[2] = {PxVec3(2.0f, 0.0f, 0.0f), PxVec3(-2.0f, 0.0f, 0.0f)} ;
	rayStart[0] = crabPose.transform(rayStart[0]);
	rayStart[1] = crabPose.transform(rayStart[1]);
	PxReal rayDist = 100.0f;
	
	// setup raycasts
	// 3 front & 3 back
	for(PxU32 j = 0; j < 2; j++)
	{
		PxVec3 rayDir = crabPose.rotate(PxVec3(j?-1.0f:1.0f,0,0));
		PxQuat rotY = PxQuat(0.4f, PxVec3(0,1,0));
		rayDir = rotY.rotateInv(rayDir);
		for(PxU32 i = 0; i < 3; i++)
		{
			mSqRayBuffer->mBatchQuery->raycast(rayStart[j], rayDir, rayDist);
			rayDir = rotY.rotate(rayDir);
		}
	}

	// add submarine visibility query
	if(mSampleSubmarine->mSubmarineActor)
	{
		PxVec3 rayStart = crabPose.transform(PxVec3(0,2,0));
		PxVec3 crabToSub = mSubmarinePos - rayStart;
		mSqRayBuffer->mBatchQuery->raycast(rayStart, crabToSub.getNormalized(), rayDist);
	}

	mSqRayBuffer->mBatchQuery->execute();

	for(PxU32 i = 0; i < mSqRayBuffer->mQueryResultSize; i++)
	{
		PxRaycastQueryResult& result = mSqRayBuffer->mRayCastResults[i];
		if(result.queryStatus == PxBatchQueryStatus::eSUCCESS && result.getNbAnyHits() == 1)
		{
			const PxRaycastHit& hit = result.getAnyHit(0);
			mDistances[i] = hit.distance;

			// don't see flat terrain as wall
			SampleRenderer::RendererColor rayColor(0,0,255);
			PxReal angle = hit.normal.dot(crabPose.q.rotate(PxVec3(0,1,0)));
			if(angle > 0.98f) // = 11.5 degree difference
			{
				mDistances[i] = rayDist;
				rayColor = SampleRenderer::RendererColor(0,255,0);
			}
#if DEBUG_RENDERING
			// debug rendering
			PxU8 blue = PxU8(mDistances[i] * (255.0f/rayDist));
			const SampleRenderer::RendererColor color(255, 0, blue);
			mSampleSubmarine->getDebugRenderer()->addLine(rayStart[i<3?0:1], hit.position, color);
			mSampleSubmarine->getDebugRenderer()->addLine(hit.position, hit.position + hit.normal*3.0f, rayColor);
#endif
		}
		else
			mDistances[i] = rayDist;
	}
}

void Crab::initState(CrabState::Enum state)
{
	mCrabState = state;
	mStateTime = gDefaultStateTime[mCrabState];
}

void Crab::updateState()
{
	// update remaining time in current state
	// transition if needed
	mStateTime -= mElapsedTime;
	mElapsedTime = 0;
	if(mStateTime <= 0.0f)
	{
		initState(CrabState::eMOVE_FWD);
	}

	PxTransform crabPose;
	{
		PxSceneReadLock scopedLock(mSampleSubmarine->getActiveScene());
		crabPose = mCrabBody->getGlobalPose();
	}

	// check if we should go into panic mode
	static const PxReal subMarinePanicDist = 50.0f;
	if(mSampleSubmarine->mSubmarineActor && mCrabState != CrabState::ePANIC)
	{
		PxRaycastQueryResult& rayResult = mSqRayBuffer->mRayCastResults[6];
		if(rayResult.queryStatus == PxBatchQueryStatus::eSUCCESS && rayResult.getNbAnyHits() == 1)
		{
			const PxRaycastHit& hit = rayResult.getAnyHit(0);
			PxVec3 subToCrab = crabPose.p - mSubmarinePos;
			PxReal distanceToSubMarine = subToCrab.magnitude();
			if(hit.actor == mSampleSubmarine->mSubmarineActor && distanceToSubMarine <= subMarinePanicDist)
			{
				initState(CrabState::ePANIC);
			}
		}
	}

	PxReal leftAcc = 0, rightAcc = 0;
	// compute fwd and bkwd distances
	static const PxReal minDist = 10.0f;
	static const PxReal fullSpeedDist = 50.0f;
	static const PxReal recipFullSpeedDist = 1.0f/fullSpeedDist;
	PxReal fDist = 0, bDist = 0;
	fDist = PxMin(mDistances[0], PxMin(mDistances[1], mDistances[2]));
	bDist = PxMin(mDistances[3], PxMin(mDistances[4], mDistances[5]));

	// handle states
	if(mCrabState == CrabState::eMOVE_FWD)
	{
		if(fDist < minDist)
		{
			initState(CrabState::eMOVE_BKWD);
		}
		else
		{
			leftAcc = PxMin(fullSpeedDist, mDistances[0])*recipFullSpeedDist*2.0f - 1.0f;
			rightAcc = PxMin(fullSpeedDist, mDistances[2])*recipFullSpeedDist*2.0f - 1.0f;
			leftAcc *= 3.0f;
			rightAcc *= 3.0f;
		}
	}
	else if (mCrabState == CrabState::eMOVE_BKWD)
	{
		if(bDist < minDist)
		{
			// find rotation dir, where we have some free space
			bool rotateLeft = mDistances[0] < mDistances[2];
			initState(rotateLeft ? CrabState::eROTATE_LEFT : CrabState::eROTATE_RIGHT);
		}
		else
		{
			leftAcc = -(PxMin(fullSpeedDist, mDistances[5])*recipFullSpeedDist*2.0f - 1.0f);
			rightAcc = -(PxMin(fullSpeedDist, mDistances[3])*recipFullSpeedDist*2.0f - 1.0f);
			leftAcc *= 3.0f;
			rightAcc *= 3.0f;
		}
	}
	else if (mCrabState == CrabState::eROTATE_LEFT)
	{
		leftAcc = -3.0f;
		rightAcc = 3.0f;
		if(fDist > minDist)
		{
			initState(CrabState::eMOVE_FWD);
		}
	}
	else if (mCrabState == CrabState::eROTATE_RIGHT)
	{
		leftAcc = 3.0f;
		rightAcc = -3.0f;
		if(fDist > minDist)
		{
			initState(CrabState::eMOVE_FWD);
		}
	}
	else if (mCrabState == CrabState::ePANIC)
	{
		if(mSampleSubmarine->mSubmarineActor)
		{
			PxVec3 subToCrab = crabPose.p - mSubmarinePos;
			PxReal distanceToSubMarine = subToCrab.magnitude();
			if(distanceToSubMarine <= subMarinePanicDist)
			{
				PxVec3 dir = crabPose.q.rotateInv(subToCrab);
				dir.y = 0;
				dir.normalize();
#if DEBUG_RENDERING
				PxVec3 startPos = crabPose.p + PxVec3(0,2,0);
				mSampleSubmarine->getDebugRenderer()->addLine(startPos, startPos + crabPose.q.rotate(dir)*2.0f, SampleRenderer::RendererColor(0,255,0));
#endif
				leftAcc =  (1.0f*dir.x + 0.2f*dir.z) * 6.0f;
				rightAcc = (1.0f*dir.x - 0.2f*dir.z) * 6.0f;
			}
		}
	}
	else if (mCrabState == CrabState::eWAITING)
	{
		// have a break
	}

	// change acceleration
	setAcceleration(leftAcc, rightAcc);

#if DEBUG_RENDERING
	PxVec3 startPosL = crabPose.transform(PxVec3(0,2,-1));
	PxVec3 startPosR =  crabPose.transform(PxVec3(0,2,1));
	mSampleSubmarine->getDebugRenderer()->addLine(startPosL, startPosL + crabPose.q.rotate(PxVec3(1,0,0))*leftAcc, SampleRenderer::RendererColor(255,255,0));
	mSampleSubmarine->getDebugRenderer()->addLine(startPosR, startPosR + crabPose.q.rotate(PxVec3(1,0,0))*rightAcc, SampleRenderer::RendererColor(0,255,0));
#endif
}


SqRayBuffer::SqRayBuffer(SampleSubmarine& sampleSubmarine, PxU32 numRays, PxU32 numHits)
: mSampleSubmarine(sampleSubmarine)
, mQueryResultSize(numRays)
, mHitSize(numHits) 
{
	mOrigAddresses[0] = malloc(mQueryResultSize*sizeof(PxRaycastQueryResult) + 15);
	mOrigAddresses[1] = malloc(mHitSize*sizeof(PxRaycastHit) + 15);

	mRayCastResults = reinterpret_cast<PxRaycastQueryResult*>((size_t(mOrigAddresses[0]) + 15) & ~15);
	mRayCastHits = reinterpret_cast<PxRaycastHit*>((size_t(mOrigAddresses[1]) + 15 )& ~15);

	PxBatchQueryDesc batchQueryDesc(mQueryResultSize, 0, 0);
	batchQueryDesc.queryMemory.userRaycastResultBuffer = mRayCastResults;
	batchQueryDesc.queryMemory.userRaycastTouchBuffer = mRayCastHits;
	batchQueryDesc.queryMemory.raycastTouchBufferSize = mHitSize;

	mBatchQuery = mSampleSubmarine.getActiveScene().createBatchQuery(batchQueryDesc);
	if(!mBatchQuery) mSampleSubmarine.fatalError("createBatchQuery failed!");
}

SqRayBuffer::~SqRayBuffer()
{
	mBatchQuery->release();
	free(mOrigAddresses[0]);
	free(mOrigAddresses[1]);
}
