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

#ifndef PXPVDSDK_PXPROFILEMEMORYEVENTREFLEXIVEWRITER_H
#define PXPVDSDK_PXPROFILEMEMORYEVENTREFLEXIVEWRITER_H

#include "PxProfileMemoryBuffer.h"
#include "PxProfileFoundationWrapper.h"
#include "PxProfileMemoryEvents.h"

namespace physx { namespace profile {

	struct MemoryEventReflexiveWriter
	{
		typedef PxProfileWrapperReflectionAllocator<uint8_t>	TAllocatorType;
		typedef MemoryBuffer<TAllocatorType>		TMemoryBufferType;
		typedef EventSerializer<TMemoryBufferType>	TSerializerType;


		PxProfileAllocatorWrapper	mWrapper;
		TMemoryBufferType	mBuffer;
		TSerializerType		mSerializer;

		MemoryEventReflexiveWriter( PxAllocatorCallback* inFoundation )
			: mWrapper( inFoundation )
			, mBuffer( TAllocatorType( mWrapper ) )
			, mSerializer( &mBuffer )
		{
		}

		template<typename TDataType>
		void operator()( const MemoryEventHeader& inHeader, const TDataType& inType )
		{
			//copy to get rid of const.
			MemoryEventHeader theHeader( inHeader );
			TDataType theData( inType );

			//write them out.
			theHeader.streamify( mSerializer );
			theData.streamify( mSerializer, theHeader );
		}
	};
}}

#endif // PXPVDSDK_PXPROFILEMEMORYEVENTREFLEXIVEWRITER_H