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


#ifndef APEX_NAME_SPACE_H
#define APEX_NAME_SPACE_H

#include "ApexUsingNamespace.h"

/*!
\file
\brief Defines APEX namespace strings

These are predefined framework namespaces in the named resource provider
*/

/*!
\brief A namespace for names of collision groups (CollisionGroup).

Each name in this namespace must map to a 5-bit integer in the range [0..31] (stored in a void*).
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_NAME_SPACE				"NSCollisionGroup"

/*!
\brief A namespace for names of GroupsMasks

Each name in this namespace must map to a pointer to a persistent 128-bit GroupsMask type.
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_128_NAME_SPACE			"NSCollisionGroup128"

/*!
\brief A namespace for names of GroupsMasks64

Each name in this namespace must map to a pointer to a persistent 64-bit GroupsMask64 type.
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_64_NAME_SPACE			"NSCollisionGroup64"

/*!
\brief A namespace for names of collision group masks

Each name in this namespace must map to a 32-bit integer (stored in a void*), wherein each
bit represents a collision group (CollisionGroup). The NRP will not automatically generate
release requests for names in this namespace.
*/
#define APEX_COLLISION_GROUP_MASK_NAME_SPACE		"NSCollisionGroupMask"

/*!
\brief Internal namespace for authorable asset types

For examples, there are entries in this namespace for "ApexRenderMesh", "NxClothingAsset",
"DestructibleAsset", etc...
The values stored in this namespace are the namespace IDs of the authorable asset types.
So if your module needs to get a pointer to a FooAsset created by module Foo, you can ask
the ApexSDK for that asset's namespace ID.
*/
#define APEX_AUTHORABLE_ASSETS_TYPES_NAME_SPACE		"AuthorableAssetTypes"

/*!
\brief Internal namespace for parameterized authorable assets
*/
#define APEX_NV_PARAM_AUTH_ASSETS_TYPES_NAME_SPACE	"NvParamAuthorableAssetTypes"

/*!
\brief A namespace for graphical material names

Each name in this namespace maps to a pointer to a game-engine defined graphical material data
structure. APEX does not interpret or dereference this pointer in any way. APEX provides this
pointer to the UserRenderResource::setMaterial(void *material) callback to the rendering engine.
This mapping allows APEX assets to refer to game engine materials (e.g.: texture maps and shader
programs) without imposing any limitations on what a game engine graphical material can contain.
The NRP will not automatically generate release requests for names in this namespace.
*/
#define APEX_MATERIALS_NAME_SPACE					"ApexMaterials"

/*!
\brief A namespace for volumetric rendering material names

*/
#define APEX_VOLUME_RENDER_MATERIALS_NAME_SPACE		"ApexVolumeRenderMaterials"


/*!
\brief A namespace for physical material names

Each name in this namespace maps to MaterialID, which is a data type defined
by the PhysX SDK. The NRP will not automatically generate release requests for
names in this namespace.
*/
#define APEX_PHYSICS_MATERIAL_NAME_SPACE			"NSPhysicalMaterial"

/*!
\brief A namespace for custom vertex buffer semantics names

Each name in this namespace maps to a pointer to a game-engine defined data structure identifying
a custom vertex buffer semantic. APEX does not interpret or dereference this pointer in any way.
APEX provides an array of these pointers in UserRenderVertexBufferDesc::customBuffersIdents,
which is passed the rendering engine when requesting allocation of vertex buffers.
*/
#define APEX_CUSTOM_VB_NAME_SPACE                   "NSCustomVBNames"

#endif // APEX_NAME_SPACE_H
