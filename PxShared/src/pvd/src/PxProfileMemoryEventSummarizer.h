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


#ifndef PXPVDSDK_PXPROFILEMEMORYEVENTSUMMARIZER_H
#define PXPVDSDK_PXPROFILEMEMORYEVENTSUMMARIZER_H

#include "PxProfileBase.h"
#include "PxProfileAllocatorWrapper.h"
#include "PxProfileMemoryEvents.h"
#include "PxProfileMemoryEventRecorder.h"
#include "PxProfileMemoryEventParser.h"

#include "PsHashMap.h"

namespace physx { namespace profile {

	struct MemoryEventSummarizerEntry
	{
		uint32_t		mType;
		uint32_t		mFile;
		uint32_t		mLine;

		MemoryEventSummarizerEntry( const AllocationEvent& evt )
			: mType( evt.mType )
			, mFile( evt.mFile )
			, mLine( evt.mLine )
		{
		}

		MemoryEventSummarizerEntry( uint32_t tp, uint32_t f, uint32_t line )
			: mType( tp )
			, mFile( f )
			, mLine( line )
		{
		}
	};
}}


namespace physx { namespace shdfnd {

	template <>
	struct Hash<physx::profile::MemoryEventSummarizerEntry>
	{
	public:
		uint32_t operator()(const physx::profile::MemoryEventSummarizerEntry& entry) const
		{
			//Combine hash values in a semi-reasonable way.
			return Hash<uint32_t>()( entry.mType )
					^ Hash<uint32_t>()( entry.mFile )
					^ Hash<uint32_t>()( entry.mLine );
		}

		bool operator()(const physx::profile::MemoryEventSummarizerEntry& lhs, const physx::profile::MemoryEventSummarizerEntry& rhs) const
		{
			return lhs.mType == rhs.mType
				&& lhs.mFile == rhs.mFile
				&& lhs.mLine == rhs.mLine;
		}

		bool equal(const physx::profile::MemoryEventSummarizerEntry& lhs, const physx::profile::MemoryEventSummarizerEntry& rhs) const
		{
			return lhs.mType == rhs.mType
				&& lhs.mFile == rhs.mFile
				&& lhs.mLine == rhs.mLine;
		}
	};
}}

namespace physx { namespace profile {

	struct MemoryEventSummarizerAllocatedValue
	{
		MemoryEventSummarizerEntry	mEntry;
		uint32_t						mSize;
		MemoryEventSummarizerAllocatedValue( MemoryEventSummarizerEntry en, uint32_t sz )
			: mEntry( en )
			, mSize( sz )
		{
		}
	};

	template<typename TSummarizerType>
	struct SummarizerParseHandler
	{
		TSummarizerType* mSummarizer;
		SummarizerParseHandler( TSummarizerType* inType )
			: mSummarizer( inType )
		{
		}
		template<typename TDataType>
		void operator()( const MemoryEventHeader& inHeader, const TDataType& inType )
		{
			mSummarizer->handleParsedData( inHeader, inType );
		}
	};

	template<typename TForwardType>
	struct MemoryEventForward
	{
		TForwardType* mForward;
		MemoryEventForward( TForwardType& inForward )
			: mForward( &inForward )
		{
		}
		template<typename TDataType>
		void operator()( const MemoryEventHeader& inHeader, const TDataType& inType )
		{
			TForwardType& theForward( *mForward );
			theForward( inHeader, inType );
		}
	};

	struct NullMemoryEventHandler
	{
		template<typename TDataType>
		void operator()( const MemoryEventHeader&, const TDataType&)
		{
		}
	};

	template<typename TForwardType>
	struct NewEntryOperatorForward
	{
		TForwardType* mForward;
		NewEntryOperatorForward( TForwardType& inForward )
			: mForward( &inForward )
		{
		}
		void operator()( const MemoryEventSummarizerEntry& inEntry, const char* inTypeStr, const char* inFileStr, uint32_t inTotalsArrayIndex )
		{
			TForwardType& theType( *mForward );
			theType( inEntry, inTypeStr, inFileStr, inTotalsArrayIndex );
		}
	};

	struct NullNewEntryOperator
	{
		void operator()( const MemoryEventSummarizerEntry&, const char*, const char*, uint32_t)
		{
		}
	};

	//Very specialized class meant to take a stream of memory events
	//endian-convert it.
	//Produce a new stream
	//And keep track of the events in a meaningful way.
	//It collapses the allocations into groupings keyed
	//by file, line, and type.
	template<bool TSwapBytes
			, typename TNewEntryOperator
			, typename MemoryEventHandler>
	struct MemoryEventSummarizer : public PxProfileEventBufferClient
	{
		typedef MemoryEventSummarizer< TSwapBytes, TNewEntryOperator, MemoryEventHandler > TThisType;
		typedef PxProfileWrapperReflectionAllocator<MemoryEventSummarizerEntry> TAllocatorType;
		typedef shdfnd::HashMap<MemoryEventSummarizerEntry, uint32_t, shdfnd::Hash<MemoryEventSummarizerEntry>, TAllocatorType> TSummarizeEntryToU32Hash;
		typedef shdfnd::HashMap<uint64_t, MemoryEventSummarizerAllocatedValue, shdfnd::Hash<uint64_t>, TAllocatorType> TU64ToSummarizerValueHash;
		PxProfileAllocatorWrapper mWrapper;
		TSummarizeEntryToU32Hash		mEntryIndexHash;
		PxProfileArray<int32_t>				mTotalsArray;
		MemoryEventParser<TSwapBytes>	mParser;
		TU64ToSummarizerValueHash		mOutstandingAllocations;
		TNewEntryOperator				mNewEntryOperator;
		MemoryEventHandler				mEventHandler;

		
		MemoryEventSummarizer( PxAllocatorCallback& inAllocator
								, TNewEntryOperator inNewEntryOperator
								, MemoryEventHandler inEventHandler)

			: mWrapper( inAllocator )
			, mEntryIndexHash( TAllocatorType( mWrapper ) )
			, mTotalsArray( mWrapper )
			, mParser( inAllocator )
			, mOutstandingAllocations( mWrapper )
			, mNewEntryOperator( inNewEntryOperator )
			, mEventHandler( inEventHandler )
		{
		}
		virtual ~MemoryEventSummarizer(){}

		//parse this data block.  This will endian-convert the data if necessary
		//and then 
		void handleData( const uint8_t* inData, uint32_t inLen )
		{
			SummarizerParseHandler<TThisType> theHandler( this );
			parseEventData<TSwapBytes>( mParser, inData, inLen, &theHandler );
		}

		template<typename TDataType>
		void handleParsedData( const MemoryEventHeader& inHeader, const TDataType& inData )
		{
			//forward it to someone who might care
			mEventHandler( inHeader, inData );
			//handle the parsed data.
			doHandleParsedData( inData );
		}

		template<typename TDataType>
		void doHandleParsedData( const TDataType& ) {}
		
		void doHandleParsedData( const AllocationEvent& inEvt ) 
		{
			onAllocation( inEvt.mSize, inEvt.mType, inEvt.mFile, inEvt.mLine, inEvt.mAddress );
		}
		
		void doHandleParsedData( const DeallocationEvent& inEvt ) 
		{
			onDeallocation( inEvt.mAddress );
		}

		uint32_t getOrCreateEntryIndex( const MemoryEventSummarizerEntry& inEvent )
		{
			uint32_t index = 0;
			const TSummarizeEntryToU32Hash::Entry* entry( mEntryIndexHash.find(inEvent ) );
			if ( !entry )
			{
				index = mTotalsArray.size();
				mTotalsArray.pushBack( 0 );
				mEntryIndexHash.insert( inEvent, index );

				//Force a string lookup and such here.
				mNewEntryOperator( inEvent, mParser.getString( inEvent.mType), mParser.getString( inEvent.mFile ), index );
			}
			else
				index = entry->second;
			return index;
		}

		//Keep a running total of what is going on, letting a listener know when new events happen.
		void onMemoryEvent( const MemoryEventSummarizerEntry& inEvent, int32_t inSize )
		{
			MemoryEventSummarizerEntry theEntry( inEvent );
			uint32_t index = getOrCreateEntryIndex( theEntry );
			mTotalsArray[index] += inSize;
		}

		void onAllocation( uint32_t inSize, uint32_t inType, uint32_t inFile, uint32_t inLine, uint64_t inAddress )
		{
			MemoryEventSummarizerEntry theEntry( inType, inFile, inLine );
			onMemoryEvent( theEntry, static_cast<int32_t>( inSize ) );
			mOutstandingAllocations.insert( inAddress, MemoryEventSummarizerAllocatedValue( theEntry, inSize ) );
		}

		void onDeallocation( uint64_t inAddress )
		{
			const TU64ToSummarizerValueHash::Entry* existing( mOutstandingAllocations.find( inAddress ) );
			if ( existing )
			{
				const MemoryEventSummarizerAllocatedValue& data( existing->second );
				onMemoryEvent( data.mEntry, -1 * static_cast<int32_t>( data.mSize ) );
				mOutstandingAllocations.erase( inAddress );
			}
			//Not much we can do with an deallocation when we didn't track the allocation.
		}

		int32_t getTypeTotal( const char* inTypeName, const char* inFilename, uint32_t inLine )
		{
			uint32_t theType( mParser.getHandle( inTypeName ) );
			uint32_t theFile( mParser.getHandle( inFilename ) );
			uint32_t theLine = inLine; //all test lines are 50...
			uint32_t index = getOrCreateEntryIndex( MemoryEventSummarizerEntry( theType, theFile, theLine ) );
			return mTotalsArray[index];
		}

		virtual void handleBufferFlush( const uint8_t* inData, uint32_t inLength )
		{
			handleData( inData, inLength );
		}
		
		virtual void handleClientRemoved() {}
	};

}}

#endif // PXPVDSDK_PXPROFILEMEMORYEVENTSUMMARIZER_H
