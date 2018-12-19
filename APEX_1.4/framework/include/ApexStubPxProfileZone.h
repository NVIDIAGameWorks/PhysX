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



#ifndef APEX_STUB_PX_PROFILE_ZONE_H
#define APEX_STUB_PX_PROFILE_ZONE_H

#include "Px.h"
#include "PxProfileZone.h"
#include "PsUserAllocated.h"

namespace physx
{
	namespace profile
	{
		class PxUserCustomProfiler;
	}
}

namespace nvidia
{
using namespace physx::profile;

namespace apex
{

// This class provides a stub implementation of PhysX's PxProfileZone.
// It would be nice to not be forced to do this, but our scoped profile event macros
// cannot have an if(gProfileZone) because it would ruin the scope.  So here we just
// create a stub that will be called so that the user need not create a PxProfileZoneManager
// in debug mode (and suffer an assertion).

class ApexStubPxProfileZone : public PxProfileZone, public UserAllocated
{
public:	

	// PxProfileZone methods
	virtual const char* getName() { return 0; }
	virtual void release() { PX_DELETE(this); }

	virtual void setProfileZoneManager(PxProfileZoneManager* ) {}
	virtual profile::PxProfileZoneManager* getProfileZoneManager() { return 0; }

	virtual uint16_t getEventIdForName( const char*  ) { return 0; }

	virtual void flushEventIdNameMap() {}

	virtual uint16_t getEventIdsForNames( const char** , uint32_t  ) { return 0; }
	virtual void setUserCustomProfiler(PxUserCustomProfiler* ) {};

	// physx::PxProfileEventBufferClientManager methods
	virtual void addClient( PxProfileZoneClient&  ) {}
	virtual void removeClient( PxProfileZoneClient&  ) {}
	virtual bool hasClients() const { return false; }

	// physx::PxProfileNameProvider methods
	virtual PxProfileNames getProfileNames() const { return PxProfileNames(); }

	// profile::PxProfileEventSender methods
	virtual void startEvent( uint16_t , uint64_t ) {}
	virtual void stopEvent( uint16_t , uint64_t ) {}

	virtual void startEvent( uint16_t , uint64_t , uint32_t ) {}
	virtual void stopEvent( uint16_t , uint64_t , uint32_t  ) {}
	virtual void eventValue( uint16_t , uint64_t , int64_t  ) {}

	// physx::PxProfileEventFlusher methods
	virtual void flushProfileEvents() {}
};

}
} // end namespace nvidia::apex

#endif // APEX_STUB_PX_PROFILE_ZONE_H
