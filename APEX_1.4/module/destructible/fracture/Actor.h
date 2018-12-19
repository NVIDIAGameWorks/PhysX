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
#ifndef RT_ACTOR_H
#define RT_ACTOR_H

#include "ActorBase.h"

namespace nvidia
{
namespace destructible
{
	class DestructibleActorImpl;
	struct DamageEvent;
	struct FractureEvent;
}

using namespace destructible;
namespace fracture
{

class Compound;
class FracturePattern;
class Renderable;

class Actor : public base::Actor
{
	friend class SimScene;
	friend class Renderable;
protected:
	Actor(base::SimScene* scene, DestructibleActorImpl* actor);
public:
	virtual ~Actor();

	Compound* createCompound();

	Compound* createCompoundFromChunk(const destructible::DestructibleActorImpl& destructibleActor, uint32_t partIndex);

	bool patternFracture(const PxVec3& hitLocation, const PxVec3& dir, float scale = 1.f, float vel = 0.f, float radius = 0.f);
	bool patternFracture(const DamageEvent& damageEvent);
	bool patternFracture(const FractureEvent& fractureEvent, bool fractureOnLoad = true);

	bool rayCast(const PxVec3& orig, const PxVec3& dir, float &dist) const;

	DestructibleActorImpl* getDestructibleActor() {return mActor;}

protected:
	void attachBasedOnFlags(base::Compound* c);

	FracturePattern* mDefaultFracturePattern;
	DestructibleActorImpl* mActor;
	bool mRenderResourcesDirty;

	float mMinRadius;
	float mRadiusMultiplier;
	float mImpulseScale;
	bool mSheetFracture;

	struct AttachmentFlags
	{
		AttachmentFlags() :
			posX(0), negX(0), posY(0), negY(0), posZ(0), negZ(0) {}

		uint32_t posX : 1;
		uint32_t negX : 1;
		uint32_t posY : 1;
		uint32_t negY : 1;
		uint32_t posZ : 1;
		uint32_t negZ : 1;
	}mAttachmentFlags;

	struct MyDamageEvent
	{
		PxVec3	position;
		PxVec3	direction;
		float	damage;
		float	radius;
	};
	MyDamageEvent mDamageEvent;

};

}
}

#endif
#endif