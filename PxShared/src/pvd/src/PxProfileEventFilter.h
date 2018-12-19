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

#ifndef PXPVDSDK_PXPROFILEEVENTFILTER_H
#define PXPVDSDK_PXPROFILEEVENTFILTER_H

#include "foundation/PxAssert.h"
#include "PxProfileBase.h"
#include "PxProfileEventId.h"

namespace physx { namespace profile {

	/**
	\brief Called upon every event to give a quick-out before adding the event
	to the event buffer.

	\note: not thread safe, can be called from different threads at the same time
	*/
	class PxProfileEventFilter
	{
	protected:
		virtual ~PxProfileEventFilter(){}
	public:
		/**
		\brief Disabled events will not go into the event buffer and will not be 
		transmitted to clients.
		\param inId Profile event id.
		\param isEnabled True if event should be enabled.
		*/
		virtual void setEventEnabled( const PxProfileEventId& inId, bool isEnabled ) = 0;

		/**
		\brief Returns the current state of the profile event.
		\return True if profile event is enabled.
		*/
		virtual bool isEventEnabled( const PxProfileEventId& inId ) const = 0;
	};

	/**
	\brief Forwards the filter requests to another event filter.
	*/
	template<typename TFilterType>
	struct PxProfileEventFilterForward
	{		
		/**
		\brief Default constructor.
		*/
		PxProfileEventFilterForward( TFilterType* inFilter ) : filter( inFilter ) {}

		/**
		\brief Disabled events will not go into the event buffer and will not be 
		transmitted to clients.
		\param inId Profile event id.
		\param isEnabled True if event should be enabled.
		*/
		void setEventEnabled( const PxProfileEventId& inId, bool isEnabled ) { filter->setEventEnabled( inId, isEnabled ); }

		/**
		\brief Returns the current state of the profile event.
		\return True if profile event is enabled.
		*/
		bool isEventEnabled( const PxProfileEventId& inId ) const { return filter->isEventEnabled( inId ); }

		TFilterType* filter;
	};

} }

#endif // PXPVDSDK_PXPROFILEEVENTFILTER_H
