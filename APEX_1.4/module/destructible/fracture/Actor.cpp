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
#include <DestructibleActorImpl.h>
#include <RenderMeshAsset.h>
#include <DestructibleStructure.h>

#include "../fracture/Actor.h"

#include "FracturePattern.h"
#include "SimScene.h"
#include "Compound.h"
#include "Mesh.h"

namespace nvidia
{
namespace fracture
{

Actor::Actor(base::SimScene* scene, DestructibleActorImpl* actor):
	base::Actor(scene),
	mActor(actor),
	mRenderResourcesDirty(false),
	mMinRadius(0.f),
	mRadiusMultiplier(1.f),
	mImpulseScale(1.f),
	mSheetFracture(true)
{
	if(actor == NULL)
	{
		const uint32_t numSectors = 10;
		const float sectorRand = 0.3f;
		const float firstSegmentSize = 0.06f;
		const float segmentScale = 1.4f;
		const float segmentRand = 0.3f;
		const float size = 10.f;
		const float radius = size;
		const float thickness = size;
		mDefaultFracturePattern = (FracturePattern*)mScene->createFracturePattern();
		mDefaultFracturePattern->createGlass(radius,thickness,numSectors,sectorRand,firstSegmentSize,segmentScale,segmentRand);
	}
	else
	{
		// create fracture pattern
		const nvidia::destructible::DestructibleActorParamNS::RuntimeFracture_Type& params = actor->getParams()->destructibleParameters.runtimeFracture;
		const uint32_t numSectors = params.glass.numSectors;
		const float sectorRand = params.glass.sectorRand;
		const float firstSegmentSize = params.glass.firstSegmentSize;
		const float segmentScale = params.glass.segmentScale;
		const float segmentRand = params.glass.segmentRand;
		const float size = actor->getBounds().getDimensions().maxElement();
		const float radius = size;
		const float thickness = size;
		mDefaultFracturePattern = (FracturePattern*)mScene->createFracturePattern();
		mDefaultFracturePattern->createGlass(radius,thickness,(int32_t)numSectors,sectorRand,firstSegmentSize,segmentScale,segmentRand);
		// attachment
		mAttachmentFlags.posX = (uint32_t)params.attachment.posX;
		mAttachmentFlags.negX = (uint32_t)params.attachment.negX;
		mAttachmentFlags.posY = (uint32_t)params.attachment.posY;
		mAttachmentFlags.negY = (uint32_t)params.attachment.negY;
		mAttachmentFlags.posZ = (uint32_t)params.attachment.posZ;
		mAttachmentFlags.negZ = (uint32_t)params.attachment.negZ;
		// other
		mDepthLimit = params.depthLimit;
		mDestroyIfAtDepthLimit = params.destroyIfAtDepthLimit;
		mImpulseScale = params.impulseScale;
		mMinConvexSize = params.minConvexSize;
		mSheetFracture = params.sheetFracture;
	}
}

Actor::~Actor()
{
	PX_DELETE(mDefaultFracturePattern);
	mDefaultFracturePattern = NULL;
}

Compound* Actor::createCompound()
{
	base::Compound* c = mScene->createCompound((base::FracturePattern*)mDefaultFracturePattern);
	addCompound(c);
	return (Compound*)c;
}

Compound* Actor::createCompoundFromChunk(const destructible::DestructibleActorImpl& actor, uint32_t partIndex)
{
	Mesh RTmesh;
	RTmesh.loadFromRenderMesh(*actor.getDestructibleAsset()->getRenderMeshAsset(),partIndex);

	// Fix texture v's (different between right hand and left hand)
	RTmesh.flipV();
	
	PxTransform pose(actor.getChunkPose(partIndex));
	pose.p = actor.getStructure()->getChunkWorldCentroid(actor.getChunk(partIndex));

	Compound* c = createCompound();

	c->createFromMesh(&RTmesh,pose,actor.getChunkLinearVelocity(partIndex),actor.getChunkAngularVelocity(partIndex),-1,actor.getScale());

	const DestructibleActorParamNS::BehaviorGroup_Type& behaviorGroup = actor.getBehaviorGroup(partIndex);
	mMinRadius = behaviorGroup.damageSpread.minimumRadius;
	mRadiusMultiplier = behaviorGroup.damageSpread.radiusMultiplier;

	// attachment
	//const DestructibleActorParamNS::RuntimeFracture_Type& params = mActor->getParams()->destructibleParameters.runtimeFracture;
	DestructibleParameters& params = mActor->getDestructibleParameters();
	mAttachmentFlags.posX |= params.rtFractureParameters.attachment.posX;
	mAttachmentFlags.negX |= params.rtFractureParameters.attachment.negX;
	mAttachmentFlags.posY |= params.rtFractureParameters.attachment.posY;
	mAttachmentFlags.negY |= params.rtFractureParameters.attachment.negY;
	mAttachmentFlags.posZ |= params.rtFractureParameters.attachment.posZ;
	mAttachmentFlags.negZ |= params.rtFractureParameters.attachment.negZ;

	attachBasedOnFlags(c);

	mRenderResourcesDirty = true;

	return c;
}

bool Actor::patternFracture(const PxVec3& hitLocation, const PxVec3& dirIn, float scale, float vel, float radiusIn)
{
	int compoundNr = -1;
	int convexNr = -1;
	PxVec3 normal;
	float dist;
	bool ret = false;
	PxVec3 dir = dirIn;

	mScene->getScene()->lockWrite();

	bool hit = false;
	if (dir.magnitudeSquared() < 0.5f)
	{
		dir = PxVec3(1.f,0.f,0.f);
		hit = base::Actor::rayCast(hitLocation-dir,dir,dist,compoundNr,convexNr,normal);
		if(!hit)
		{
			dir = PxVec3(0.f,1.f,0.f);
			hit = base::Actor::rayCast(hitLocation-dir,dir,dist,compoundNr,convexNr,normal);
			if(!hit)
			{
				dir = PxVec3(0.f,0.f,1.f);
				hit = base::Actor::rayCast(hitLocation-dir,dir,dist,compoundNr,convexNr,normal);
			}
		}
	}
	else
	{
		hit = base::Actor::rayCast(hitLocation-dir,dir,dist,compoundNr,convexNr,normal);
	}
	if (hit)
	{
		float radius = mMinRadius + mRadiusMultiplier*radiusIn;
		float impulseMagn = scale*vel*mImpulseScale;

		if (mSheetFracture)
		{
			normal = ((Compound*)mCompounds[(uint32_t)compoundNr])->mNormal;
		}
		PxVec3 a(0.f,0.f,1.f);
		normal.normalize();
		a -= a.dot(normal)*normal;
		if( a.magnitudeSquared() < 0.1f )
		{
			a = PxVec3(0.f,1.f,0.f);
			a -= a.dot(normal)*normal;
		}
		a.normalize();
		PxVec3 b(normal.cross(a));
		PxMat33 trans(a,b,normal);
		ret = base::Actor::patternFracture(hitLocation,dir,compoundNr,trans,radius,impulseMagn,impulseMagn);
	}

	mScene->getScene()->unlockWrite();

	mRenderResourcesDirty = true;

	return ret;
}

bool Actor::patternFracture(const DamageEvent& damageEvent)
{
	mDamageEvent.position = damageEvent.position;
	mDamageEvent.direction = damageEvent.direction;
	mDamageEvent.damage = damageEvent.damage;
	mDamageEvent.radius = damageEvent.radius;
	return patternFracture(damageEvent.position, damageEvent.direction, 1.f, damageEvent.damage, damageEvent.radius);
}

bool Actor::patternFracture(const FractureEvent& fractureEvent, bool fractureOnLoad)
{
	createCompoundFromChunk(*mActor,fractureEvent.chunkIndexInAsset);
	if(fractureOnLoad)
	{
		patternFracture(mDamageEvent.position, mDamageEvent.direction, 1.f, mDamageEvent.damage, mDamageEvent.radius);
		//patternFracture(fractureEvent.position,fractureEvent.hitDirection,1.f,fractureEvent.impulse.magnitude()/mActor->getChunkMass(fractureEvent.chunkIndexInAsset),1);
	}
	return true;
}

bool Actor::rayCast(const PxVec3& orig, const PxVec3& dir, float &dist) const
{
	int compoundNr = 0;
	int convexNr = 0;
	PxVec3 n;
	bool ret = false;

	mScene->getScene()->lockRead();
	ret = base::Actor::rayCast(orig,dir,dist,compoundNr,convexNr,n);
	mScene->getScene()->unlockRead();

	return ret;
}

void Actor::attachBasedOnFlags(base::Compound* c)
{
	PxBounds3 b; 
	c->getLocalBounds(b);
	// Determine sheet face
	if (mSheetFracture)
	{
		PxVec3 dim = b.getDimensions();
		if ( dim.x < dim.y && dim.x < dim.z )
		{
			((Compound*)c)->mNormal = PxVec3(1.f,0.f,0.f);
		}
		else if ( dim.y < dim.x && dim.y < dim.z )
		{
			((Compound*)c)->mNormal = PxVec3(0.f,1.f,0.f);
		}
		else
		{
			((Compound*)c)->mNormal = PxVec3(0.f,0.f,1.f);
		}
	}
	// Attach
	const float w = 0.01f*b.getDimensions().maxElement();
	nvidia::Array<PxBounds3> bounds;
	bounds.reserve(6);
	if (mAttachmentFlags.posX)
	{
		PxBounds3 a(b);
		a.minimum.x = a.maximum.x-w;
		bounds.pushBack(a);
	}
	if (mAttachmentFlags.negX)
	{
		PxBounds3 a(b);
		a.maximum.x = a.minimum.x+w;
		bounds.pushBack(a);
	}
	if (mAttachmentFlags.posY)
	{
		PxBounds3 a(b);
		a.minimum.y = a.maximum.y-w;
		bounds.pushBack(a);
	}
	if (mAttachmentFlags.negY)
	{
		PxBounds3 a(b);
		a.maximum.y = a.minimum.y+w;
		bounds.pushBack(a);
	}
	if (mAttachmentFlags.posZ)
	{
		PxBounds3 a(b);
		a.minimum.z = a.maximum.z-w;
		bounds.pushBack(a);
	}
	if (mAttachmentFlags.negZ)
	{
		PxBounds3 a(b);
		a.maximum.z = a.minimum.z+w;
		bounds.pushBack(a);
	}
	c->attachLocal(bounds);
}

}
}
#endif
