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

#ifndef PT_PARTICLE_SYSTEM_FLAGS_H
#define PT_PARTICLE_SYSTEM_FLAGS_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "CmPhysXCommon.h"
#include "particles/PxParticleBaseFlag.h"

namespace physx
{

namespace Pt
{

/*
ParticleSystems related constants
*/
// Maximum number of particles per particle system
#define PT_PARTICLE_SYSTEM_PARTICLE_LIMIT 0xfffffffe

/*!
PxParticleBaseFlag extension.
*/
struct InternalParticleSystemFlag
{
	enum Enum
	{
		// flags need to go into the unused bits of PxParticleBaseFlag
		eSPH                               = (1 << 16),
		eDISABLE_POSITION_UPDATE_ON_CREATE = (1 << 17),
		eDISABLE_POSITION_UPDATE_ON_SETPOS = (1 << 18)
	};
};

struct InternalParticleFlag
{
	enum Enum
	{
		// constraint info
		eCONSTRAINT_0_VALID          = (1 << 0),
		eCONSTRAINT_1_VALID          = (1 << 1),
		eANY_CONSTRAINT_VALID        = (eCONSTRAINT_0_VALID | eCONSTRAINT_1_VALID),
		eCONSTRAINT_0_DYNAMIC        = (1 << 2),
		eCONSTRAINT_1_DYNAMIC        = (1 << 3),
		eALL_CONSTRAINT_MASK         = (eANY_CONSTRAINT_VALID | eCONSTRAINT_0_DYNAMIC | eCONSTRAINT_1_DYNAMIC),

		// static geometry cache: 00 (cache invalid), 11 (cache valid and refreshed), 01 (cache valid, but aged by one
		// step).
		eGEOM_CACHE_BIT_0            = (1 << 4),
		eGEOM_CACHE_BIT_1            = (1 << 5),
		eGEOM_CACHE_MASK             = (eGEOM_CACHE_BIT_0 | eGEOM_CACHE_BIT_1),

		// cuda update info
		eCUDA_NOTIFY_CREATE          = (1 << 6),
		eCUDA_NOTIFY_SET_POSITION    = (1 << 7),
		eCUDA_NOTIFY_POSITION_CHANGE = (eCUDA_NOTIFY_CREATE | eCUDA_NOTIFY_SET_POSITION)
	};
};

struct ParticleFlags
{
	PxU16 api; // this is PxParticleFlag
	PxU16 low; // this is InternalParticleFlag
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_PARTICLE_SYSTEM_FLAGS_H
