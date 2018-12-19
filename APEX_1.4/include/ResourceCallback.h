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



#ifndef RESOURCE_CALLBACK_H
#define RESOURCE_CALLBACK_H

/*!
\file
\brief class ResourceCallback
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief User defined callback for resource management

The user may implement a subclass of this abstract class and provide an instance to the
ApexSDK descriptor.  These callbacks can only be triggered directly by ApexSDK API calls,
so they do not need to be re-entrant or thread safe.
*/
class ResourceCallback
{
public:
	virtual ~ResourceCallback() {}

	/**
	\brief Request a resource from the user

	Will be called by the ApexSDK if a named resource is required but has not yet been provided.
	The resource pointer is returned directly, ResourceProvider::setResource() should not be called.
	This function will be called at most once per named resource, unless an intermediate call to
	releaseResource() has been made.
	
	\note If this call results in the application calling ApexSDK::createAsset, the name given 
		  to the asset must match the input name parameter in this method.
	*/
	virtual void* requestResource(const char* nameSpace, const char* name) = 0;

	/**
	\brief Request the user to release a resource

	Will be called by the ApexSDK when all internal references to a named resource have been released.
	If this named resource is required again in the future, a new call to requestResource() will be made.
	*/
	virtual void  releaseResource(const char* nameSpace, const char* name, void* resource) = 0;
};

PX_POP_PACK

}
} // namespace nvidia::apex

#endif // RESOURCE_CALLBACK_H
