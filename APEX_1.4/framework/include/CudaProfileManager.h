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



#ifndef CUDA_PROFILE_MANAGER_H
#define CUDA_PROFILE_MANAGER_H

/*!
\file
\brief classes CudaProfileManager
*/

#include <ApexDefs.h>
#include <PxSimpleTypes.h>

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Interface for options of ApexCudaProfileManager
 */
class CudaProfileManager
{
public:
	/**
	 * Normalized time unit for profile data
	 */
	enum TimeFormat
	{
		MILLISECOND = 1,
		MICROSECOND = 1000,
		NANOSECOND = 1000000
	};

	 /**
	\brief Set path for writing results
	*/
	virtual void setPath(const char* path) = 0;
	 /**
	\brief Set kernel for profile
	*/
	virtual void setKernel(const char* functionName, const char* moduleName) = 0;
	/**
	\brief Set normailized time unit
	*/
	virtual void setTimeFormat(TimeFormat tf) = 0;
	/**
	\brief Set state (on/off) for profile manager
	*/
	virtual void enable(bool state) = 0;
	/**
	\brief Get state (on/off) of profile manager
	*/
	virtual bool isEnabled() const = 0;
};

PX_POP_PACK

}
}

#endif // CUDA_PROFILE_MANAGER_H

