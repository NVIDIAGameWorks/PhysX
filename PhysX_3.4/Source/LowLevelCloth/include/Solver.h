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

#pragma once

#include "Types.h"

namespace physx
{

class PxBaseTask;

namespace cloth
{

class Cloth;

// called during inter-collision, user0 and user1 are the user data from each cloth
typedef bool (*InterCollisionFilter)(void* user0, void* user1);

/// base class for solvers
class Solver
{
  protected:
	Solver(const Solver&);
	Solver& operator=(const Solver&);

  protected:
	Solver()
	{
	}

  public:
	virtual ~Solver()
	{
	}

	/// add cloth object, returns true if successful
	virtual void addCloth(Cloth*) = 0;

	/// remove cloth object
	virtual void removeCloth(Cloth*) = 0;

	/// simulate one time step
	virtual physx::PxBaseTask& simulate(float dt, physx::PxBaseTask&) = 0;

	// inter-collision parameters
	virtual void setInterCollisionDistance(float distance) = 0;
	virtual float getInterCollisionDistance() const = 0;
	virtual void setInterCollisionStiffness(float stiffness) = 0;
	virtual float getInterCollisionStiffness() const = 0;
	virtual void setInterCollisionNbIterations(uint32_t nbIterations) = 0;
	virtual uint32_t getInterCollisionNbIterations() const = 0;
	virtual void setInterCollisionFilter(InterCollisionFilter filter) = 0;

	/// returns true if an unrecoverable error has occurred
	virtual bool hasError() const = 0;
};

} // namespace cloth
} // namespace physx
