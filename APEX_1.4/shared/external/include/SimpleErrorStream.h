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


#include "PxErrorCallback.h"

#include <ApexDefs.h>

class SimplePxErrorStream : public physx::PxErrorCallback
{
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		const char* errorCode = NULL;
		switch (code)
		{
		case physx::PxErrorCode::eNO_ERROR :
			return;
		case physx::PxErrorCode::eINVALID_PARAMETER:
			errorCode = "Invalid Parameter";
			break;
		case physx::PxErrorCode::eINVALID_OPERATION:
			errorCode = "Invalid Operation";
			break;
		case physx::PxErrorCode::eOUT_OF_MEMORY:
			errorCode = "Out of Memory";
			break;
		case physx::PxErrorCode::eINTERNAL_ERROR :
			errorCode = "Internal Error";
			break;
//		case physx::PxErrorCode::eASSERTION:
//			errorCode = "Assertion";
//			break;
		case physx::PxErrorCode::eDEBUG_INFO:
			errorCode = "Debug Info";
			break;
		case physx::PxErrorCode::eDEBUG_WARNING:
			errorCode = "Debug Warning";
			break;
//		case physx::PxErrorCode::eSERIALIZATION_ERROR:
//			errorCode = "Serialization Error";
//			break;
		default:
			errorCode = "Unknown Error Code";
		}

		if (errorCode != NULL)
		{
			printf("PhysX error: %s %s:%d\n%s\n", errorCode, file, line, message);
		}
		else
		{
			printf("PhysX error: physx::PxErrorCode is %d in %s:%d\n%s\n", code, file, line, message);
		}
	}

	virtual void print(const char* message)
	{
		printf("%s", message);
	}
};
