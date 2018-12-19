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


#ifndef PXPVDSDK_PXPROFILEMEMORYEVENTPARSER_H
#define PXPVDSDK_PXPROFILEMEMORYEVENTPARSER_H

#include "PxProfileMemoryEvents.h"
#include "PxProfileAllocatorWrapper.h"
#include "PxProfileEventSerialization.h"

#include "PsHashMap.h"
#include "PsString.h"

namespace physx { namespace profile {

	template<bool TSwapBytes, typename TParserType, typename THandlerType> 
	bool parseEventData( TParserType& inParser, const uint8_t* inData, uint32_t inLength, THandlerType* inHandler );

	template<bool TSwapBytes>
	struct MemoryEventParser
	{
		typedef PxProfileWrapperReflectionAllocator<uint8_t> TAllocatorType;
		typedef shdfnd::HashMap<uint32_t, char*, shdfnd::Hash<uint32_t>, TAllocatorType > THdlToStringMap;
		typedef EventDeserializer<TSwapBytes>	TDeserializerType;
		
		PxProfileAllocatorWrapper	mWrapper;
		THdlToStringMap		mHdlToStringMap;
		TDeserializerType	mDeserializer;

		MemoryEventParser( PxAllocatorCallback& inAllocator )
			: mWrapper( inAllocator )
			, mHdlToStringMap( TAllocatorType( mWrapper ) )
			, mDeserializer ( 0, 0 )
		{
		}

		~MemoryEventParser()
		{
			for ( THdlToStringMap::Iterator iter( mHdlToStringMap.getIterator() ); iter.done() == false; ++iter )
				mWrapper.getAllocator().deallocate( reinterpret_cast<void*>(iter->second) );
		}

		template<typename TOperator>
		void parse(const StringTableEvent&, const MemoryEventHeader& inHeader, TOperator& inOperator)
		{
			StringTableEvent evt;
			evt.init();
			evt.streamify( mDeserializer, inHeader );
			uint32_t len = static_cast<uint32_t>( strlen( evt.mString ) );
			char* newStr = static_cast<char*>( mWrapper.getAllocator().allocate( len + 1, "const char*", __FILE__, __LINE__ ) );
			shdfnd::strlcpy( newStr, len+1, evt.mString );
			mHdlToStringMap[evt.mHandle] = newStr;
			inOperator( inHeader, evt );
		}

		const char* getString( uint32_t inHdl )
		{
			const THdlToStringMap::Entry* entry = mHdlToStringMap.find( inHdl );
			if ( entry ) return entry->second;
			return "";
		}

		//Slow reverse lookup used only for testing.
		uint32_t getHandle( const char* inStr )
		{
			for ( THdlToStringMap::Iterator iter = mHdlToStringMap.getIterator();
				!iter.done();
				++iter )
			{
				if ( safeStrEq( iter->second, inStr ) )
					return iter->first;
			}
			return 0;
		}

		template<typename TOperator>
		void parse(const AllocationEvent&, const MemoryEventHeader& inHeader, TOperator& inOperator)
		{
			AllocationEvent evt;
			evt.streamify( mDeserializer, inHeader );
			inOperator( inHeader, evt );
		}

		template<typename TOperator>
		void parse(const DeallocationEvent&, const MemoryEventHeader& inHeader, TOperator& inOperator)
		{
			DeallocationEvent evt;
			evt.streamify( mDeserializer, inHeader );
			inOperator( inHeader, evt );
		}

		template<typename TOperator>
		void parse(const FullAllocationEvent&, const MemoryEventHeader&, TOperator& )
		{
			PX_ASSERT( false ); //will never happen.
		}

		template<typename THandlerType>
		void parseEventData( const uint8_t* inData, uint32_t inLength, THandlerType* inOperator )
		{
			physx::profile::parseEventData<TSwapBytes>( *this, inData, inLength, inOperator );
		}
	};
	

	template<typename THandlerType, bool TSwapBytes>
	struct MemoryEventParseOperator
	{
		MemoryEventParser<TSwapBytes>* mParser;
		THandlerType* mOperator;
		MemoryEventHeader* mHeader;
		MemoryEventParseOperator( MemoryEventParser<TSwapBytes>* inParser, THandlerType* inOperator, MemoryEventHeader* inHeader )
			: mParser( inParser )
			, mOperator( inOperator )
			, mHeader( inHeader )
		{
		}

		bool wasSuccessful() { return mParser->mDeserializer.mFail == false; }

		bool parseHeader()
		{
			mHeader->streamify( mParser->mDeserializer );
			return wasSuccessful();
		}

		template<typename TDataType>
		bool operator()( const TDataType& inType )
		{
			mParser->parse( inType, *mHeader, *mOperator );
			return wasSuccessful();
		}
		
		bool operator()( uint8_t ) { PX_ASSERT( false ); return false;}
	};

	template<bool TSwapBytes, typename TParserType, typename THandlerType> 
	inline bool parseEventData( TParserType& inParser, const uint8_t* inData, uint32_t inLength, THandlerType* inHandler )
	{
		inParser.mDeserializer = EventDeserializer<TSwapBytes>( inData, inLength );
		MemoryEvent::EventData crapData;
		uint32_t eventCount = 0;
		MemoryEventHeader theHeader;
		MemoryEventParseOperator<THandlerType, TSwapBytes> theOp( &inParser, inHandler, &theHeader );
		while( inParser.mDeserializer.mLength && inParser.mDeserializer.mFail == false)
		{
			if ( theOp.parseHeader() )
			{
				if( visit<bool>( theHeader.getType(), crapData, theOp ) == false )
					inParser.mDeserializer.mFail = true;
			}
			++eventCount;
		}
		return inParser.mDeserializer.mFail == false;
	}
}}

#endif // PXPVDSDK_PXPROFILEMEMORYEVENTPARSER_H
