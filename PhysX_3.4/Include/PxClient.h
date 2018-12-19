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


#ifndef PX_PHYSICS_NX_CLIENT
#define PX_PHYSICS_NX_CLIENT

#include "foundation/PxFlags.h"

#if !PX_DOXYGEN
namespace physx
{
#endif

/**
\brief An ID to identify different clients for multiclient support.

@see PxScene::createClient() 
*/
typedef PxU8 PxClientID;

/**
\brief The predefined default PxClientID value.

@see PxClientID PxScene::createClient() 
*/
static const PxClientID PX_DEFAULT_CLIENT = 0;

/**
\brief The maximum number of clients we support.

@see PxClientID PxScene::createClient() 
*/
static const PxClientID PX_MAX_CLIENTS = 128;

/**
\brief Behavior bit flags for simulation clients.

\deprecated PxClientBehaviorFlag feature has been deprecated in PhysX version 3.4

@see PxClientBehaviorFlags PxScene::setClientBehaviorFlags() 
*/
struct PX_DEPRECATED PxClientBehaviorFlag
{ 
	enum Enum 
	{
		/**
		\brief Report actors belonging to other clients to the trigger callback of this client.

		@see PxSimulationEventCallback::onTrigger()
		*/
		eREPORT_FOREIGN_OBJECTS_TO_TRIGGER_NOTIFY			= (1<<0),
		/**
		\brief Report actors belonging to other clients to the contact callback of this client.

		@see PxSimulationEventCallback::onContact()
		*/
		eREPORT_FOREIGN_OBJECTS_TO_CONTACT_NOTIFY			= (1<<1),
		/**
		\brief Report actors belonging to other clients to the constraint break callback of this client.

		@see PxSimulationEventCallback::onConstraintBreak()
		*/
		eREPORT_FOREIGN_OBJECTS_TO_CONSTRAINT_BREAK_NOTIFY	= (1<<2),
		/**
		\brief Report actors belonging to other clients to scene queries of this client.
		*/
		eREPORT_FOREIGN_OBJECTS_TO_SCENE_QUERY				= (1<<3)
	};
};

/**
\brief Bitfield that contains a set of raised flags defined in PxClientBehaviorFlag.

\deprecated PxActorClientBehaviorFlag feature has been deprecated in PhysX version 3.4

@see PxClientBehaviorFlag PxScene::setClientBehaviorFlags() 
*/
typedef PX_DEPRECATED PxFlags<PxClientBehaviorFlag::Enum, PxU8> PxClientBehaviorFlags;
PX_FLAGS_OPERATORS(PxClientBehaviorFlag::Enum, PxU8)


/**
\brief Multiclient behavior bit flags for actors.

\deprecated PxActorClientBehaviorFlag feature has been deprecated in PhysX version 3.4

@see PxActorClientBehaviorFlags PxActor::setClientBehaviorFlags()
*/
struct PX_DEPRECATED PxActorClientBehaviorFlag
{ 
	enum Enum
	{
		/**
		\brief Report this actor to trigger callbacks of other clients.

		@see PxSimulationEventCallback::onTrigger()
		*/
		eREPORT_TO_FOREIGN_CLIENTS_TRIGGER_NOTIFY			= (1<<0),
		/**
		\brief Report this actor to contact callbacks of other clients.

		@see PxSimulationEventCallback::onContact()
		*/
		eREPORT_TO_FOREIGN_CLIENTS_CONTACT_NOTIFY			= (1<<1),
		/**
		\brief Report this actor to constraint break callbacks of other clients.

		@see PxSimulationEventCallback::onConstraintBreak()
		*/
		eREPORT_TO_FOREIGN_CLIENTS_CONSTRAINT_BREAK_NOTIFY	= (1<<2),
		/**
		\brief Report this actor to scene queries of other clients.
		*/
		eREPORT_TO_FOREIGN_CLIENTS_SCENE_QUERY				= (1<<3)
	};
};

/**
\brief Bitfield that contains a set of raised flags defined in PxActorClientBehaviorFlag.

\deprecated PxActorClientBehaviorFlag feature has been deprecated in PhysX version 3.4

@see PxActorClientBehaviorFlag PxActor::setClientBehaviorFlags()
*/
typedef PX_DEPRECATED PxFlags<PxActorClientBehaviorFlag::Enum, PxU8> PxActorClientBehaviorFlags;
PX_FLAGS_OPERATORS(PxActorClientBehaviorFlag::Enum, PxU8)

#if !PX_DOXYGEN
} // namespace physx
#endif

#endif
