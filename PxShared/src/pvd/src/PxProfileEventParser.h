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


#ifndef PXPVDSDK_PXPROFILEEVENTPARSER_H
#define PXPVDSDK_PXPROFILEEVENTPARSER_H

#include "PxProfileEvents.h"
#include "PxProfileEventSerialization.h"

namespace physx { namespace profile {
	
	struct EventParserData
	{
		EventContextInformation		mContextInformation;
		uint64_t					mLastTimestamp;		

		EventParserData() : mLastTimestamp(0)
		{
		}
	};

	//This object will be copied a lot so all of its important
	//data must be pointers.
	template<typename THandlerType, bool TSwapBytes>
	struct EventParseOperator
	{
		typedef EventDeserializer<TSwapBytes>	TDeserializerType;

		EventParserData*						mData;
		TDeserializerType*						mDeserializer;
		EventHeader*							mHeader;
		THandlerType*							mHandler;

		EventParseOperator( EventParserData* inData, TDeserializerType* inDeserializer, EventHeader* inHeader, THandlerType* inHandler ) 
			: mData( inData )
			, mDeserializer( inDeserializer )
			, mHeader( inHeader )
			, mHandler( inHandler ) 
		{}
		
		template<typename TEventType>
		bool parse( TEventType& ioEvent )
		{
			ioEvent.streamify( *mDeserializer, *mHeader );
			bool success = mDeserializer->mFail == false;
			PX_ASSERT( success );
			return success;
		}

		bool parseHeader( EventHeader& ioEvent )
		{
			ioEvent.streamify( *mDeserializer );
			bool success = mDeserializer->mFail == false;
			PX_ASSERT( success );
			return success;
		}

		template<typename TEventType>
		bool handleProfileEvent( TEventType& evt )
		{
			bool retval = parse( evt );
			if ( retval )
			{
				mData->mContextInformation = evt.mContextInformation;
				handle( evt.getRelativeEvent(), evt.mContextInformation );
			}
			return retval;
		}
		
		template<typename TEventType>
		bool handleRelativeProfileEvent( TEventType& evt )
		{
			bool retval = parse( evt );
			if ( retval )
				handle( evt, mData->mContextInformation );
			return retval;
		}

		template<typename TRelativeEventType>
		void handle( const TRelativeEventType& evt, const EventContextInformation& inInfo )
		{	
			mData->mLastTimestamp = mHeader->uncompressTimestamp( mData->mLastTimestamp, evt.getTimestamp());
			const_cast<TRelativeEventType&>(evt).setTimestamp( mData->mLastTimestamp );
			evt.handle( mHandler, mHeader->mEventId
						, inInfo.mThreadId
						, inInfo.mContextId
						, inInfo.mCpuId
						, inInfo.mThreadPriority );
		}

		bool operator()( const StartEvent& )
		{
			StartEvent evt;
			return handleProfileEvent( evt );
		}
		bool operator()( const StopEvent& )
		{
			StopEvent evt;
			return handleProfileEvent( evt );
		}
		bool operator()( const RelativeStartEvent& )
		{
			RelativeStartEvent evt;
			return handleRelativeProfileEvent( evt );

		}
		bool operator()( const RelativeStopEvent& )
		{
			RelativeStopEvent evt;
			return handleRelativeProfileEvent( evt );
		}
		bool operator()( const EventValue& )
		{
			EventValue evt;
			bool retval = parse( evt );
			if ( retval )
			{
				evt.mValue = mHeader->uncompressTimestamp( 0, evt.mValue );
				evt.handle( mHandler, mHeader->mEventId );
			}
			return retval;
		}

		//obsolete, placeholder to skip data from PhysX SDKs < 3.4
		bool operator()( const CUDAProfileBuffer& )
		{
			CUDAProfileBuffer evt;
			bool retval = parse( evt );
			return retval;
		}

		//Unknown event type.
		bool operator()(uint8_t )
		{
			PX_ASSERT( false );
			return false;
		}
	};

	template<bool TSwapBytes, typename THandlerType> 
	inline bool parseEventData( const uint8_t* inData, uint32_t inLength, THandlerType* inHandler )
	{
		EventDeserializer<TSwapBytes> deserializer( inData, inLength );
		Event::EventData crapData;
		EventHeader theHeader;
		EventParserData theData;
		EventParseOperator<THandlerType,TSwapBytes> theOp( &theData, &deserializer, &theHeader, inHandler );
		while( deserializer.mLength && deserializer.mFail == false)
		{
			if ( theOp.parseHeader( theHeader ) )
				visit<bool>( static_cast<EventTypes::Enum>( theHeader.mEventType ), crapData, theOp );
		}
		return deserializer.mFail == false;
	}

	class PxProfileBulkEventHandler
	{
	protected:
		virtual ~PxProfileBulkEventHandler(){}
	public:
		virtual void handleEvents(const physx::profile::Event* inEvents, uint32_t inBufferSize) = 0;
		static void parseEventBuffer(const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileBulkEventHandler& inHandler, bool inSwapBytes);
	};
}}

#endif // PXPVDSDK_PXPROFILEEVENTPARSER_H
