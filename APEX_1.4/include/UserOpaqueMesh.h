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



#ifndef USER_OPAQUE_MESH_H
#define USER_OPAQUE_MESH_H

/*!
\file
\brief class UserOpaqueMesh
*/

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

//! \brief Name of OpaqueMesh authoring type namespace
#define APEX_OPAQUE_MESH_NAME_SPACE "ApexOpaqueMesh"

/**
\brief Opaque mesh description
*/
class UserOpaqueMeshDesc
{
public:
	///the name of the opaque mesh
	const char* mMeshName;
};

/**
\brief An abstract interface to an opaque mesh
*
* An 'opaque' mesh is a binding between the 'name' of a mesh and some internal mesh representation used by the
* application.  This allows the application to refer to meshes by name without involving duplciation of index buffer and
* vertex buffer data declarations.
*/
class UserOpaqueMesh
{
public:
	virtual ~UserOpaqueMesh() {}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif
