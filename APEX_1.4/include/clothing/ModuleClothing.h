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



#ifndef MODULE_CLOTHING_H
#define MODULE_CLOTHING_H

#include "Module.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class ClothingAsset;
class ClothingAssetAuthoring;
class ClothingPhysicalMesh;

class IProgressListener;
class RenderMeshAssetAuthoring;

/**
\mainpage APEX Clothing API Documentation

\section overview Overview

This document contains a full API documentation for all public classes.
*/



/**
\brief APEX Clothing Module

Used to generate simulated clothing on (mostly humanoid) characters.
*/
class ModuleClothing : public Module
{
public:
	/**
	\brief creates an empty physical mesh. A custom mesh can be assigned to it.
	*/
	virtual ClothingPhysicalMesh* createEmptyPhysicalMesh() = 0;

	/**
	\brief creates a physical mesh based on a render mesh asset. This will be a 1 to 1 copy of the render mesh

	\param [in] asset			The render mesh that is used as source for the physical mesh
	\param [in] subdivision		Modify the physical mesh such that all edges that are longer than (bounding box diagonal / subdivision) are split up. Must be <= 200
	\param [in] mergeVertices	All vertices with the same position will be welded together.
	\param [in] closeHoles		Close any hole found in the mesh.
	\param [in] progress		An optional callback for progress display.
	*/
	virtual ClothingPhysicalMesh* createSingleLayeredMesh(RenderMeshAssetAuthoring* asset, uint32_t subdivision, bool mergeVertices, bool closeHoles, IProgressListener* progress) = 0;

protected:
	virtual ~ModuleClothing() {}
};

#if !defined(_USRDLL)
/**
* If this module is distributed as a static library, the user must call this
* function before calling ApexSDK::createModule("Clothing")
*/
void instantiateModuleClothing();
#endif


PX_POP_PACK

}
} // namespace nvidia

#endif // MODULE_CLOTHING_H
