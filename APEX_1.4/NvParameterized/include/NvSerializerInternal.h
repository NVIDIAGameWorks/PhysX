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

#ifndef PX_SERIALIZER_INTERNAL_H
#define PX_SERIALIZER_INTERNAL_H

/*!
\file
\brief NvParameterized serializer factory
*/


#include "nvparameterized/NvSerializer.h"
#include "nvparameterized/NvParameterizedTraits.h"

namespace NvParameterized
{

PX_PUSH_PACK_DEFAULT

/**
\brief A factory function to create an instance of the Serializer class
\param [in] type serializer type (binary, xml, etc.)
\param [in] traits traits-object to do memory allocation, legacy version conversions, callback calls, etc.
*/
Serializer *internalCreateSerializer(Serializer::SerializeType type, Traits *traits);


// Query the current platform
const SerializePlatform &GetCurrentPlatform();
bool GetPlatform(const char *name, SerializePlatform &platform);
const char *GetPlatformName(const SerializePlatform &platform);


PX_POP_PACK

} // end of namespace

#endif
