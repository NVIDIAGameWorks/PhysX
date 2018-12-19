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
// Copyright (c) 2018 NVIDIA Corporation. All rights reserved.



#ifndef PVDBINDING_ERROR_STREAM_H
#define PVDBINDING_ERROR_STREAM_H

#include "PxErrorCallback.h"
#include "PxProfileBase.h"

#include <stdio.h>

namespace physx { namespace profile {

inline void printInfo(const char* format, ...)
{
	PX_UNUSED(format); 
#if PRINT_TEST_INFO
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va); 
#endif
}

class PVDBindingErrorStream : public PxErrorCallback
{
public:

	PVDBindingErrorStream() {}
	void reportError(PxErrorCode::Enum e, const char* message, const char* file, int line)
	{
		PX_UNUSED(line); 
		PX_UNUSED(file); 
		switch (e)
		{
		case PxErrorCode::eINVALID_PARAMETER:
			printf( "on invalid parameter: %s", message );
			break;
		case PxErrorCode::eINVALID_OPERATION:
			printf( "on invalid operation: %s", message );
			break;
		case PxErrorCode::eOUT_OF_MEMORY:
			printf( "on out of memory: %s", message );
			break;
		case PxErrorCode::eDEBUG_INFO:
			printf( "on debug info: %s", message );
			break;
		case PxErrorCode::eDEBUG_WARNING:
			printf( "on debug warning: %s", message );
			break;
		default:
			printf( "on unknown error: %s", message );
			break;
		}
	}
};

}}

#endif // PVDBINDING_ERROR_STREAM_H
