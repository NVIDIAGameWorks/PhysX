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



#ifndef APEX_RESOURCE_H
#define APEX_RESOURCE_H

#include "ApexUsingNamespace.h"
#include "PsUserAllocated.h"

namespace physx
{
	namespace pvdsdk
	{
		class PvdDataStream;
	}
}
namespace nvidia
{
namespace apex
{

/**
 *  Class defines semi-public interface to ApexResource objects
 *	Resource - gets added to a list, will be deleted when the list is deleted
 */
class ApexResourceInterface
{
public:
	virtual void    release() = 0;
	virtual void	setListIndex(class ResourceList& list, uint32_t index) = 0;
	virtual uint32_t	getListIndex() const = 0;
	virtual void	initPvdInstances(pvdsdk::PvdDataStream& /*pvdStream*/) {};
};

/**
Class that implements resource ID and bank
*/
class ApexResource : public UserAllocated
{
public:
	ApexResource() : m_listIndex(0xFFFFFFFF), m_list(NULL) {}
	void removeSelf();
	virtual ~ApexResource();

	uint32_t			m_listIndex;
	class ResourceList*	m_list;
};


/**
Initialized Template class.
*/
template <class DescType>class InitTemplate
{
	//gotta make a derived class cause of protected ctor
public:
	InitTemplate() : isSet(false) {}

	bool isSet;
	DescType data;


	void set(const DescType* desc)
	{
		if (desc)
		{
			isSet = true;
			//memcpy(this,desc, sizeof(DescType));
			data = *desc;
		}
		else
		{
			isSet = false;
		}
	}


	bool get(DescType& dest) const
	{
		if (isSet)
		{
			//memcpy(&dest,this, sizeof(DescType));
			dest = data;
			return true;
		}
		else
		{
			return false;
		}

	}
};

} // namespace apex
} // namespace nvidia

#endif // APEX_RESOURCE_H
