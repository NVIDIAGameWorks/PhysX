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



#include "ApexDefs.h"

#include "Apex.h"
#include "ModuleDestructibleImpl.h"
#include "DestructibleActorJointImpl.h"
#include "DestructibleScene.h"
#include "DestructibleActorJoint.h"
#include "DestructibleActorProxy.h"

#include <PxScene.h>
#include <PxJoint.h>
#include <PxD6Joint.h>
#include <PxDistanceJoint.h>
#include <PxFixedJoint.h>
#include <PxPrismaticJoint.h>
#include <PxRevoluteJoint.h>
#include <PxSphericalJoint.h>

namespace nvidia
{
namespace destructible
{
using namespace physx;

DestructibleActorJointImpl::DestructibleActorJointImpl(const DestructibleActorJointDesc& desc, DestructibleScene& dscene) :
	joint(NULL)
{
	if (desc.destructible[0] == NULL && desc.destructible[1] == NULL)
	{
		APEX_DEBUG_WARNING("Both destructible actors in DestructibleActorJoint are NULL.");
		return;
	}

	PxRigidActor*	actor[2]		= {desc.actor[0], desc.actor[1]};
	PxVec3			localAxis[2]	= {desc.localAxis[0], desc.localAxis[1]};
	PxVec3			localAnchor[2]	= {desc.localAnchor[0], desc.localAnchor[1]};
	PxVec3			localNormal[2]	= {desc.localNormal[0], desc.localNormal[1]};

	PxTransform		localFrame[2];

	for (int i = 0; i < 2; ++i)
	{
		if (desc.destructible[i] == NULL)
		{
			structure[i] = NULL;
			attachmentChunkIndex[i] = ModuleDestructibleConst::INVALID_CHUNK_INDEX;

			if(NULL == actor[i])
			{
				// World constrained			
				PxMat33	rot(desc.globalAxis[i],desc.globalNormal[i],desc.globalAxis[i].cross(desc.globalNormal[i]));

				localFrame[i].p	= desc.globalAnchor[i];
				localFrame[i].q	= PxQuat(rot);
				localFrame[i].q.normalize();
			}
			else
			{
				// Constrained to physics object
				PxMat33	rot(localAxis[i], localNormal[i], localAxis[i].cross(localNormal[i]));

				localFrame[i].p	= localAnchor[i];
				localFrame[i].q	= PxQuat(rot);
				localFrame[i].q.normalize();
			}
			continue;
		}

		PxRigidDynamic* attachActor = NULL;
		DestructibleActorImpl& destructible = ((DestructibleActorProxy*)desc.destructible[i])->impl;
		structure[i] = destructible.getStructure();

		attachmentChunkIndex[i] = desc.attachmentChunkIndex[i];

		if (attachmentChunkIndex[i] >= 0 && attachmentChunkIndex[i] < (int32_t)destructible.getDestructibleAsset()->getChunkCount())
		{
			DestructibleStructure::Chunk& chunk = structure[i]->chunks[destructible.getFirstChunkIndex() + attachmentChunkIndex[i]];
			attachActor = structure[i]->dscene->chunkIntact(chunk);
		}

		SCOPED_PHYSX_LOCK_READ(dscene.getModulePhysXScene());

		if (attachActor == NULL)
		{
			float minDistance = PX_MAX_F32;
			for (uint32_t j = 0; j < destructible.getDestructibleAsset()->getChunkCount(); ++j)
			{
				DestructibleAssetParametersNS::Chunk_Type& source = destructible.getDestructibleAsset()->mParams->chunks.buf[j];
				const bool hasChildren = source.numChildren != 0;
				if (!hasChildren)	// Only attaching to lowest-level chunks, initially
				{
					DestructibleStructure::Chunk& chunk = structure[i]->chunks[destructible.getFirstChunkIndex() + j];
					PxRigidDynamic* actor = structure[i]->dscene->chunkIntact(chunk);
					if (actor)
					{
						const float distance = (actor->getGlobalPose().transform(chunk.localSphereCenter) - desc.globalAnchor[i]).magnitude();
						if (distance < minDistance)
						{
							attachActor = actor;
							attachmentChunkIndex[i] = (int32_t)(destructible.getFirstChunkIndex() + j);
							minDistance = distance;
						}
					}
				}
			}
		}

		if (attachActor == NULL)
		{
			APEX_DEBUG_WARNING("No physx actor could be found in destructible actor %p to attach the joint.", desc.destructible[i]);
			return;
		}

		actor[i] = (PxRigidActor*)attachActor;

		if (attachActor->getScene() != NULL && dscene.getModulePhysXScene() != attachActor->getScene())
		{
			APEX_DEBUG_WARNING("Trying to joint actors from a scene different from the joint scene.");
			return;
		}

		localAnchor[i] = attachActor->getGlobalPose().transformInv(desc.globalAnchor[i]);
		localAxis[i] = attachActor->getGlobalPose().rotateInv(desc.globalAxis[i]);
		localNormal[i] = attachActor->getGlobalPose().rotateInv(desc.globalNormal[i]);
		
		PxMat33	rot(localAxis[i], localNormal[i], localAxis[i].cross(localNormal[i]));

		localFrame[i].p	= localAnchor[i];
		localFrame[i].q	= PxQuat(rot);
		localFrame[i].q.normalize();
	}

	dscene.getModulePhysXScene()->lockRead();
	switch (desc.type)
	{
	case PxJointConcreteType::eD6:
		joint	= PxD6JointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	case PxJointConcreteType::eDISTANCE:
		joint	= PxDistanceJointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	case PxJointConcreteType::eFIXED:
		joint	= PxFixedJointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	case PxJointConcreteType::ePRISMATIC:
		joint	= PxPrismaticJointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	case PxJointConcreteType::eREVOLUTE:
		joint	= PxRevoluteJointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	case PxJointConcreteType::eSPHERICAL:
		joint	= PxSphericalJointCreate(dscene.getModulePhysXScene()->getPhysics(), actor[0], localFrame[0], actor[1], localFrame[1]);
		break;
	default:
		PX_ALWAYS_ASSERT();
		break;
	}
	dscene.getModulePhysXScene()->unlockRead();

	PX_ASSERT(joint != NULL);
}

DestructibleActorJointImpl::~DestructibleActorJointImpl()
{
	if (joint)
	{
		joint->release();
	}
}

bool DestructibleActorJointImpl::updateJoint()
{
	if (!joint)
	{
		return false;
	}

	PxRigidActor* actors[2];
	joint->getActors(actors[0], actors[1]);

	bool needsReattachment = false;
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (structure[i] == NULL)
		{
			continue;
		}
		if (attachmentChunkIndex[i] < 0 || attachmentChunkIndex[i] >= (int32_t)structure[i]->chunks.size())
		{
			return false;
		}
		DestructibleStructure::Chunk& chunk = structure[i]->chunks[(uint32_t)attachmentChunkIndex[i]];
		PxRigidDynamic* actor = structure[i]->dscene->chunkIntact(chunk);
		if (actor == NULL)
		{
			return false;
		}
		if ((PxRigidDynamic*)actors[i] != actor)
		{
			needsReattachment = true;
			actors[i] = (PxRigidDynamic*)actor;
		}

	}

	if (!needsReattachment)
	{
		return true;
	}

#define JOINT_SET_ACTORS_WORKAROUND	1
#if JOINT_SET_ACTORS_WORKAROUND
	PxScene* pxScenes[2] = { NULL, NULL };
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (actors[i] != NULL)
		{
			pxScenes[i] = actors[i]->getScene();
			if (actors[i]->getScene() != NULL)
			{
				pxScenes[i]->lockWrite();
				pxScenes[i]->removeActor(*actors[i], false);
				pxScenes[i]->unlockWrite();
			}
		}
	}
#endif
	joint->setActors(actors[0], actors[1]);
#if JOINT_SET_ACTORS_WORKAROUND
	for (uint32_t i = 0; i < 2; ++i)
	{
		if (actors[i] != NULL && pxScenes[i] != NULL)
		{
			pxScenes[i]->lockWrite();
			pxScenes[i]->addActor(*actors[i]);
			pxScenes[i]->unlockWrite();
		}
	}
#endif
	return joint != NULL;
}

}
} // end namespace nvidia


