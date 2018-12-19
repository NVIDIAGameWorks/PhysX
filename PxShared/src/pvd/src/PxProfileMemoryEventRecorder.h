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


#ifndef PXPVDSDK_PXPROFILEMEMORYEVENTRECORDER_H
#define PXPVDSDK_PXPROFILEMEMORYEVENTRECORDER_H


#include "PxProfileBase.h"
#include "PxProfileAllocatorWrapper.h"
#include "PxProfileMemoryEvents.h"
#include "PxProfileMemoryEventTypes.h"

#include "PsHashMap.h"
#include "PsUserAllocated.h"
#include "PsBroadcast.h"
#include "PxProfileMemory.h"

namespace physx { namespace profile {

	//Remember outstanding events.
	//Remembers allocations, forwards them to a listener if one is attached
	//and will forward all outstanding allocations to a listener when one is
	//attached.
	struct MemoryEventRecorder : public shdfnd::AllocationListener
	{
		typedef PxProfileWrapperReflectionAllocator<uint8_t> TAllocatorType;
		typedef shdfnd::HashMap<uint64_t,FullAllocationEvent,shdfnd::Hash<uint64_t>,TAllocatorType> THashMapType;

		PxProfileAllocatorWrapper		mWrapper;
		THashMapType			mOutstandingAllocations;
		AllocationListener*		mListener;

		MemoryEventRecorder( PxAllocatorCallback* inFoundation )
			: mWrapper( inFoundation )
			, mOutstandingAllocations( TAllocatorType( mWrapper ) )
			, mListener( NULL )
		{
		}

		static uint64_t ToU64( void* inData ) { return PX_PROFILE_POINTER_TO_U64( inData ); }
		static void* ToVoidPtr( uint64_t inData ) { return reinterpret_cast<void*>(size_t(inData)); }
		virtual void onAllocation( size_t size, const char* typeName, const char* filename, int line, void* allocatedMemory )
		{
			onAllocation( size, typeName, filename, uint32_t(line), ToU64( allocatedMemory ) );
		}
		
		void onAllocation( size_t size, const char* typeName, const char* filename, uint32_t line, uint64_t allocatedMemory )
		{
			if ( allocatedMemory == 0 )
				return;
			FullAllocationEvent theEvent;
			theEvent.init( size, typeName, filename, line, allocatedMemory );
			mOutstandingAllocations.insert( allocatedMemory, theEvent );
			if ( mListener != NULL ) mListener->onAllocation( size, typeName, filename, int(line), ToVoidPtr(allocatedMemory) );
		}
		
		virtual void onDeallocation( void* allocatedMemory )
		{
			onDeallocation( ToU64( allocatedMemory ) );
		}

		void onDeallocation( uint64_t allocatedMemory )
		{
			if ( allocatedMemory == 0 )
				return;
			mOutstandingAllocations.erase( allocatedMemory );
			if ( mListener != NULL ) mListener->onDeallocation( ToVoidPtr( allocatedMemory ) );
		}

		void flushProfileEvents() {}

		void setListener( AllocationListener* inListener )
		{
			mListener = inListener;
			if ( mListener )
			{	
				for ( THashMapType::Iterator iter = mOutstandingAllocations.getIterator();
					!iter.done();
					++iter )
				{
					const FullAllocationEvent& evt( iter->second );
					mListener->onAllocation( evt.mSize, evt.mType, evt.mFile, int(evt.mLine), ToVoidPtr( evt.mAddress ) );
				}
			}
		}
	};

	class PxProfileMemoryEventRecorderImpl : public shdfnd::UserAllocated
											, public physx::profile::PxProfileMemoryEventRecorder
	{
		MemoryEventRecorder mRecorder;
	public:
		PxProfileMemoryEventRecorderImpl( PxAllocatorCallback* inFnd )
			: mRecorder( inFnd )
		{
		}

		virtual void onAllocation( size_t size, const char* typeName, const char* filename, int line, void* allocatedMemory )
		{
			mRecorder.onAllocation( size, typeName, filename, line, allocatedMemory );
		}

		virtual void onDeallocation( void* allocatedMemory )
		{
			mRecorder.onDeallocation( allocatedMemory );
		}
		
		virtual void setListener( AllocationListener* inListener )
		{
			mRecorder.setListener( inListener );
		}

		virtual void release()
		{
			PX_PROFILE_DELETE( mRecorder.mWrapper.getAllocator(), this );
		}
	};

}}
#endif // PXPVDSDK_PXPROFILEMEMORYEVENTRECORDER_H
