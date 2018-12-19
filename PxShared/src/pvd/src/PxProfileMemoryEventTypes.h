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

#ifndef PXPVDSDK_PXPROFILEMEMORYEVENTTYPES_H
#define PXPVDSDK_PXPROFILEMEMORYEVENTTYPES_H

#include "PxProfileBase.h"
#include "PxProfileEventBufferClientManager.h"
#include "PxProfileEventSender.h"
#include "PsBroadcast.h"

namespace physx { namespace profile {

	struct PxProfileMemoryEventType
	{
		enum Enum
		{
			Unknown = 0,
			Allocation,
			Deallocation
		};
	};

	struct PxProfileBulkMemoryEvent
	{
		uint64_t mAddress;
		uint32_t mDatatype;
		uint32_t mFile;
		uint32_t mLine;
		uint32_t mSize;
		PxProfileMemoryEventType::Enum mType;

		PxProfileBulkMemoryEvent(){}

		PxProfileBulkMemoryEvent( uint32_t size, uint32_t type, uint32_t file, uint32_t line, uint64_t addr )
			: mAddress( addr )
			, mDatatype( type )
			, mFile( file )
			, mLine( line )
			, mSize( size )
			, mType( PxProfileMemoryEventType::Allocation )
		{
		}
		
		PxProfileBulkMemoryEvent( uint64_t addr )
			: mAddress( addr )
			, mDatatype( 0 )
			, mFile( 0 )
			, mLine( 0 )
			, mSize( 0 )
			, mType( PxProfileMemoryEventType::Deallocation )
		{
		}
	};
	
	class PxProfileBulkMemoryEventHandler
	{
	protected:
		virtual ~PxProfileBulkMemoryEventHandler(){}
	public:
		virtual void handleEvents( const PxProfileBulkMemoryEvent* inEvents, uint32_t inBufferSize ) = 0;
		static void parseEventBuffer( const uint8_t* inBuffer, uint32_t inBufferSize, PxProfileBulkMemoryEventHandler& inHandler, bool inSwapBytes, PxAllocatorCallback* inAlloc );
	};
} }

#endif // PXPVDSDK_PXPROFILEMEMORYEVENTTYPES_H
