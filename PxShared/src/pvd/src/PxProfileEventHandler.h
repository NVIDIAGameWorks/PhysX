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

#ifndef PXPVDSDK_PXPROFILEEVENTHANDLER_H
#define PXPVDSDK_PXPROFILEEVENTHANDLER_H

#include "PxProfileBase.h"
#include "PxProfileEventId.h"

namespace physx { namespace profile {

	/**
	\brief A client of the event system can expect to find these events in the event buffer.
	*/
	class PxProfileEventHandler
	{
	protected:
		virtual ~PxProfileEventHandler(){}
	public:
		/**
		\brief Event start - onStartEvent.
			
		\param[in] inId Profile event id.
		\param[in] threadId Thread id.
		\param[in] contextId Context id.
		\param[in] cpuId CPU id.
		\param[in] threadPriority Thread priority.
		\param[in] timestamp Timestamp in cycles.		
		 */
		virtual void onStartEvent( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, uint8_t cpuId, uint8_t threadPriority, uint64_t timestamp ) = 0;

		/**
		\brief Event stop - onStopEvent.
			
		\param[in] inId Profile event id.
		\param[in] threadId Thread id.
		\param[in] contextId Context id.
		\param[in] cpuId CPU id.
		\param[in] threadPriority Thread priority.
		\param[in] timestamp Timestamp in cycles.		
		 */
		virtual void onStopEvent( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, uint8_t cpuId, uint8_t threadPriority, uint64_t timestamp ) = 0;

		/**
		\brief Event value - onEventValue.
			
		\param[in] inId Profile event id.
		\param[in] threadId Thread id.
		\param[in] contextId Context id.
		\param[in] inValue Value.
		 */
		virtual void onEventValue( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, int64_t inValue ) = 0;

		/**
		\brief Parse the flushed profile buffer which contains the profile events.
			
		\param[in] inBuffer The profile buffer with profile events.
		\param[in] inBufferSize Buffer size.
		\param[in] inHandler The profile event callback to receive the parsed events.
		\param[in] inSwapBytes Swap bytes possibility.		
		 */
		static void parseEventBuffer( const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileEventHandler& inHandler, bool inSwapBytes );

		/**
		\brief Translates event duration in timestamp (cycles) into nanoseconds.
			
		\param[in] duration Timestamp duration of the event.

		\return event duration in nanoseconds. 
		 */
		static uint64_t durationToNanoseconds(uint64_t duration);
	};
} }

#endif // PXPVDSDK_PXPROFILEEVENTHANDLER_H
