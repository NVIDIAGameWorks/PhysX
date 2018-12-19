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

#ifndef PXPVDSDK_PXPROFILECOMPILETIMEEVENTFILTER_H
#define PXPVDSDK_PXPROFILECOMPILETIMEEVENTFILTER_H

#include "PxProfileBase.h"
#include "PxProfileEventId.h"

//Define before including header in order to enable a different
//compile time event profile threshold.
#ifndef PX_PROFILE_EVENT_PROFILE_THRESHOLD
#define PX_PROFILE_EVENT_PROFILE_THRESHOLD EventPriorities::Medium
#endif

namespace physx { namespace profile {

	/**
	\brief Profile event priorities. Used to filter out events.
	*/
	struct EventPriorities
	{
		enum Enum
		{
			None,		// the filter setting to kill all events
			Coarse,
			Medium,
			Detail,
			Never		// the priority to set for an event if it should never fire.
		};
	};

	/**
	\brief Gets the priority for a given event.
	Specialize this object in order to get the priorities setup correctly.
	*/
	template<uint16_t TEventId>
	struct EventPriority { static const uint32_t val = EventPriorities::Medium; };

	/**
	\brief 	Filter events by given event priority and set threshold.
	*/
	template<uint16_t TEventId>
	struct EventFilter
	{
		static const bool val = EventPriority<TEventId>::val <= PX_PROFILE_EVENT_PROFILE_THRESHOLD;
	};

}}

#endif // PXPVDSDK_PXPROFILECOMPILETIMEEVENTFILTER_H
