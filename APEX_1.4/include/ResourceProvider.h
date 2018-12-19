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



#ifndef RESOURCE_PROVIDER_H
#define RESOURCE_PROVIDER_H

/*!
\file
\brief class ResourceProvider
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

class ResourceCallback;

PX_PUSH_PACK_DEFAULT

/*!
\brief A user provided class for mapping names to pointers or integers

Named resource provider - a name-to-pointer utility.  User must provide the pointed-to data.
				        - also supports name-to-integer
*/

class ResourceProvider
{
public:
	/**
	\brief Register a callback

	Register a callback function for unresolved named resources.  This function will be called by APEX when
	an unresolved named resource is requested.  The function will be called at most once for each named
	resource.  The function must directly return the pointer to the resource or NULL.
	*/
	virtual void 		registerCallback(ResourceCallback* impl) = 0;

	/**
	\brief Provide the pointer for the specified named resource
	*/
	virtual void 		setResource(const char* nameSpace, const char* name, void* resource, bool incRefCount = false) = 0;


	/**
	\brief Provide the unsigned integer for the specified named resource
	*/
	virtual void 		setResourceU32(const char* nameSpace, const char* name, uint32_t id, bool incRefCount = false) = 0;

	/**
	\brief Retrieve the pointer to the specified named resource.

	The game application (level loader) may use this to find assets by
	name, as a convenience.  If the named resource has not yet been
	loaded it will trigger a call to ResourceCallback::requestResource(),
	assuming an ResourceCallback instance was registered with the APEX SDK.
	If the named resource has already been loaded, getResource will not
	increment the reference count.
	*/
	virtual void* 		getResource(const char* nameSpace, const char* name) = 0;

	/**
	\brief Releases all resources in this namespace.
	\return the total number of resources released.
	*/
	virtual uint32_t  	releaseAllResourcesInNamespace(const char* nameSpace) = 0;

	/**
	\brief Releases a single resource.
	\return the outstanding referernce count after the release is performed.
	*/
	virtual uint32_t  	releaseResource(const char* nameSpace, const char* name) = 0;

	/**
	\brief Reports if a current resource exists and, if so, the reference count.
	*/
	virtual bool    	findRefCount(const char* nameSpace, const char* name, uint32_t& refCount) = 0;

	/**
	\brief Locates an existing resource

	This function will *not* call back to the application if the resource does not exist.
	Only reports currently set resources.
	*/
	virtual void* 		findResource(const char* nameSpace, const char* name) = 0;

	/**
	\brief Locates an existing integer resource.
	*/
	virtual uint32_t 	findResourceU32(const char* nameSpace, const char* name) = 0;

	/**
	\brief Returns a list of all resources in a particular namespace.
	*/
	virtual void** 		findAllResources(const char* nameSpace, uint32_t& count) = 0;

	/**
	\brief Returns a list of the names of all resources within a particular namespace
	*/
	virtual const char** findAllResourceNames(const char* nameSpace, uint32_t& count) = 0;

	/**
	\brief Returns a list of all registered namespaces.
	*/
	virtual const char** findNameSpaces(uint32_t& count) = 0;

	/**
	\brief Write contents of resource table to error stream.
	*/
	virtual void dumpResourceTable() = 0;

	/**
	\brief Returns if the resource provider is operating in a case sensitive mode.

	\note By default the resource provider is NOT case sensitive
	
	\note It's not possible to change the case sensitivity of the NRP 
		  once the APEX SDK is created, so the the switch is available in the 
		  ApexSDKDesc::resourceProviderIsCaseSensitive member variable.
	*/
	virtual bool isCaseSensitive() = 0;
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RESOURCE_PROVIDER_H
