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



#ifndef APEX_SDKVERSION_STRING_H
#define APEX_SDKVERSION_STRING_H

/*!
\file
\brief APEX SDK versioning definitions
*/

#include "foundation/PxPreprocessor.h"
#include "ApexDefs.h"

//! \brief physx namespace
namespace nvidia
{
//! \brief apex namespace
namespace apex
{

/**
\brief These values are used to select version string GetApexSDKVersionString function should return
*/
enum ApexSDKVersionString
{
	/**
	\brief APEX version
	*/
	VERSION = 0,
	/**
	\brief APEX build changelist
	*/
	CHANGELIST = 1,
	/**
	\brief APEX tools build changelist
	*/
	TOOLS_CHANGELIST = 2,
	/**
	\brief APEX branch used to create build 
	*/
	BRANCH = 3,
	/**
	\brief Time at which the build was created
	*/
	BUILD_TIME = 4,
	/**
	\brief Author of the build
	*/
	AUTHOR = 5,
	/**
	\brief Reason to prepare the build
	*/
	REASON = 6
};

/**
\def APEX_API
\brief Export the function declaration from its DLL
*/

/**
\def CALL_CONV
\brief Use C calling convention, required for exported functions
*/

#ifdef CALL_CONV
#undef CALL_CONV
#endif

#if PX_WINDOWS_FAMILY
#define APEX_API extern "C" __declspec(dllexport)
#define CALL_CONV __cdecl
#else
#define APEX_API extern "C"
#define CALL_CONV /* void */
#endif

/**
\brief Returns version strings
*/
APEX_API const char*	CALL_CONV GetApexSDKVersionString(ApexSDKVersionString versionString);

}
} // end namespace nvidia::apex

#endif // APEX_SDKVERSION_STRING_H
