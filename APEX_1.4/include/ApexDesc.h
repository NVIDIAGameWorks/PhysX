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



#ifndef APEX_DESC_H
#define APEX_DESC_H

/*!
\file
\brief class ApexDesc
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Base class for all APEX Descriptor classes

A descriptor class of type NxXDesc is typically passed to a createX() function.  Descriptors have
several advantages over simply passing a number of explicit parameters to create():

- all parameters can have default values so the user only needs to know about the ones he needs to change
- new parameters can be added without changing the user code, along with defaults
- the user and the SDK can validate the parameter's correctness using isValid()
- if creation fails, the user can look at the code of isValid() to see what exactly is not being accepted by the SDK
- some object types can save out their state into descriptors again for serialization

Care should be taken that derived descriptor classes do not initialize their base class members multiple times,
once in the constructor, and once in setToDefault()!
*/
class ApexDesc
{
public:
	/**
	\brief for standard init of user data member
	*/
	void* userData;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ApexDesc()
	{
		setToDefault();
	}
	/**
	\brief (re)sets the structure to the default.
	*/
	PX_INLINE void setToDefault()
	{
		userData = 0;
	}
	/**
	\brief Returns true if the descriptor is valid.
	\return true if the current settings are valid.
	*/
	PX_INLINE bool isValid() const
	{
		return true;
	}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // APEX_DESC_H
