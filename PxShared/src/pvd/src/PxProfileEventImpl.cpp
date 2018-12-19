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

#include "foundation/PxErrorCallback.h"
#include "foundation/PxAllocatorCallback.h"

#include "PxProfileEvents.h"
#include "PxProfileEventSerialization.h"
#include "PxProfileEventBuffer.h"
#include "PxProfileZoneImpl.h"
#include "PxProfileZoneManagerImpl.h"
#include "PxProfileEventParser.h"
#include "PxProfileEventHandler.h"
#include "PxProfileScopedMutexLock.h"
#include "PxProfileEventFilter.h"
#include "PxProfileContextProvider.h"
#include "PxProfileEventMutex.h"
#include "PxProfileMemoryEventTypes.h"
#include "PxProfileMemoryEventRecorder.h"
#include "PxProfileMemoryEventBuffer.h"
#include "PxProfileMemoryEventParser.h"
#include "PxProfileContextProviderImpl.h"

#include "PsUserAllocated.h"
#include "PsTime.h"

#include <stdio.h>

namespace physx { namespace profile {


	uint64_t PxProfileEventHandler::durationToNanoseconds(uint64_t duration)
	{
		return shdfnd::Time::getBootCounterFrequency().toTensOfNanos(duration) * 10;
	}

	void PxProfileEventHandler::parseEventBuffer( const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileEventHandler& inHandler, bool inSwapBytes )
	{
		if ( inSwapBytes == false )
			parseEventData<false>( inBuffer, inBufferSize, &inHandler );
		else
			parseEventData<true>( inBuffer, inBufferSize, &inHandler );
	}

	template<uint32_t TNumEvents>
	struct ProfileBulkEventHandlerBuffer
	{
		Event mEvents[TNumEvents];
		uint32_t mEventCount;
		PxProfileBulkEventHandler* mHandler;
		ProfileBulkEventHandlerBuffer( PxProfileBulkEventHandler* inHdl )
			: mEventCount( 0 )
			, mHandler( inHdl )
		{
		}
		void onEvent( const Event& inEvent )
		{
			mEvents[mEventCount] = inEvent;
			++mEventCount;
			if ( mEventCount == TNumEvents )
				flush();
		}
		void onEvent( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, uint8_t cpuId, uint8_t threadPriority, uint64_t timestamp, EventTypes::Enum inType )
		{
			StartEvent theEvent;
			theEvent.init( threadId, contextId, cpuId, static_cast<uint8_t>( threadPriority ), timestamp );
			onEvent( Event( EventHeader( static_cast<uint8_t>( inType ), inId.eventId ), theEvent ) );
		}
		void onStartEvent( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, uint8_t cpuId, uint8_t threadPriority, uint64_t timestamp )
		{
			onEvent( inId, threadId, contextId, cpuId, threadPriority, timestamp, EventTypes::StartEvent );
		}
		void onStopEvent( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, uint8_t cpuId, uint8_t threadPriority, uint64_t timestamp )
		{
			onEvent( inId, threadId, contextId, cpuId, threadPriority, timestamp, EventTypes::StopEvent );
		}
		void onEventValue( const PxProfileEventId& inId, uint32_t threadId, uint64_t contextId, int64_t value )
		{
			EventValue theEvent;
			theEvent.init( value, contextId, threadId );
			onEvent( Event( inId.eventId, theEvent ) );
		}
		void flush()
		{
			if ( mEventCount )
				mHandler->handleEvents( mEvents, mEventCount );
			mEventCount = 0;
		}
	};


	void PxProfileBulkEventHandler::parseEventBuffer( const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileBulkEventHandler& inHandler, bool inSwapBytes )
	{
		ProfileBulkEventHandlerBuffer<256> hdler( &inHandler );
		if ( inSwapBytes )
			parseEventData<true>( inBuffer, inBufferSize, &hdler );
		else
			parseEventData<false>( inBuffer, inBufferSize, &hdler );
		hdler.flush();
	}

	struct PxProfileNameProviderImpl
	{
		PxProfileNameProvider* mImpl;
		PxProfileNameProviderImpl( PxProfileNameProvider* inImpl )
			: mImpl( inImpl )
		{
		}
		PxProfileNames getProfileNames() const { return mImpl->getProfileNames(); }
	};

	
	struct PxProfileNameProviderForward
	{
		PxProfileNames mNames;
		PxProfileNameProviderForward( PxProfileNames inNames )
			: mNames( inNames )
		{
		}
		PxProfileNames getProfileNames() const { return mNames; }
	};

	
	PX_FOUNDATION_API PxProfileZone& PxProfileZone::createProfileZone( PxAllocatorCallback* inAllocator, const char* inSDKName, PxProfileNames inNames, uint32_t inEventBufferByteSize )
	{
		typedef ZoneImpl<PxProfileNameProviderForward> TSDKType;
		return *PX_PROFILE_NEW( inAllocator, TSDKType ) ( inAllocator, inSDKName, inEventBufferByteSize, PxProfileNameProviderForward( inNames ) );
	}
	
	PxProfileZoneManager& PxProfileZoneManager::createProfileZoneManager(PxAllocatorCallback* inAllocator )
	{
		return *PX_PROFILE_NEW( inAllocator, ZoneManagerImpl ) ( inAllocator );
	}

	PxProfileMemoryEventRecorder& PxProfileMemoryEventRecorder::createRecorder( PxAllocatorCallback* inAllocator )
	{
		return *PX_PROFILE_NEW( inAllocator, PxProfileMemoryEventRecorderImpl )( inAllocator );
	}
	
	PxProfileMemoryEventBuffer& PxProfileMemoryEventBuffer::createMemoryEventBuffer( PxAllocatorCallback& inAllocator, uint32_t inBufferSize )
	{
		return *PX_PROFILE_NEW( &inAllocator, PxProfileMemoryEventBufferImpl )( inAllocator, inBufferSize );
	}
	template<uint32_t TNumEvents>
	struct ProfileBulkMemoryEventHandlerBuffer
	{
		PxProfileBulkMemoryEvent mEvents[TNumEvents];
		uint32_t mEventCount;
		PxProfileBulkMemoryEventHandler* mHandler;
		ProfileBulkMemoryEventHandlerBuffer( PxProfileBulkMemoryEventHandler* inHdl )
			: mEventCount( 0 )
			, mHandler( inHdl )
		{
		}
		void onEvent( const PxProfileBulkMemoryEvent& evt )
		{
			mEvents[mEventCount] = evt;
			++mEventCount;
			if ( mEventCount == TNumEvents )
				flush();
		}

		template<typename TDataType>
		void operator()( const MemoryEventHeader&, const TDataType& ) {}

		void operator()( const MemoryEventHeader&, const AllocationEvent& evt )
		{
			onEvent( PxProfileBulkMemoryEvent( evt.mSize, evt.mType, evt.mFile, evt.mLine, evt.mAddress ) );
		}

		void operator()( const MemoryEventHeader&, const DeallocationEvent& evt )
		{
			onEvent( PxProfileBulkMemoryEvent( evt.mAddress ) );
		}

		void flush()
		{
			if ( mEventCount )
				mHandler->handleEvents( mEvents, mEventCount );
			mEventCount = 0;
		}
	};

	void PxProfileBulkMemoryEventHandler::parseEventBuffer( const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileBulkMemoryEventHandler& inHandler, bool inSwapBytes, PxAllocatorCallback* inAlloc )
	{
		PX_ASSERT(inAlloc);

		ProfileBulkMemoryEventHandlerBuffer<0x1000>* theBuffer = PX_PROFILE_NEW(inAlloc, ProfileBulkMemoryEventHandlerBuffer<0x1000>)(&inHandler);

		if ( inSwapBytes )
		{			
			MemoryEventParser<true> theParser( *inAlloc );
			theParser.parseEventData( inBuffer, inBufferSize, theBuffer );
		}
		else
		{
			MemoryEventParser<false> theParser( *inAlloc );
			theParser.parseEventData( inBuffer, inBufferSize, theBuffer );
		}
		theBuffer->flush();

		PX_PROFILE_DELETE(*inAlloc, theBuffer);
	}

} }

