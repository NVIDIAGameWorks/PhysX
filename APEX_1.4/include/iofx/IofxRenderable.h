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



#ifndef IOFX_RENDERABLE_H
#define IOFX_RENDERABLE_H

#include "foundation/Px.h"
#include "ApexInterface.h"
#include "Renderable.h"
#include "IofxRenderCallback.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief IofxSharedRenderData stores common render data shared by several IOFX Renderables.
*/
struct IofxSharedRenderData
{
	uint32_t	maxObjectCount; //!< maximum number of particles
	uint32_t	activeObjectCount; //!< currently active number of particles
};

/**
\brief IofxSpriteSharedRenderData stores sprite render data shared by several IOFX Renderables.
*/
struct IofxSpriteSharedRenderData : IofxSharedRenderData
{
	IofxSpriteRenderLayout	spriteRenderLayout; //!< sprite render layout
	UserRenderBuffer*			spriteRenderBuffer; //!< sprite render buffer
	UserRenderSurface*		spriteRenderSurfaces[IofxSpriteRenderLayout::MAX_SURFACE_COUNT]; //!< sprite render surfaces
};

/**
\brief IofxMeshSharedRenderData stores mesh render data shared by several IOFX Renderables.
*/
struct IofxMeshSharedRenderData : IofxSharedRenderData
{
	IofxMeshRenderLayout		meshRenderLayout; //!< mesh render layout
	UserRenderBuffer*			meshRenderBuffer; //!< mesh render buffer
};

/**
\brief IofxCommonRenderData stores common render data for one IOFX Renderable.
*/
struct IofxCommonRenderData
{
	uint32_t		startIndex; //!< index of the first particle in render buffer
	uint32_t		objectCount; //!< number of particles in render buffer

	const char*		renderResourceNameSpace; //!< render resource name space
	const char*		renderResourceName; //!< render resource name
	void*			renderResource; //!< render resource
};

/**
\brief IofxSpriteRenderData stores sprite render data for one IOFX Renderable.
*/
struct IofxSpriteRenderData : IofxCommonRenderData
{
	uint32_t		visibleCount; //!< number of visible particles (if sprite depth sorting is enabled, includes only particles in front of the view frustrum)

	const IofxSpriteSharedRenderData* sharedRenderData; //!< reference to Sprite shared render data
};

/**
\brief IofxMeshRenderData stores mesh render data for one IOFX Renderable.
*/
struct IofxMeshRenderData : IofxCommonRenderData
{
	const IofxMeshSharedRenderData* sharedRenderData; //!< reference to Mesh shared render data
};

class IofxSpriteRenderable;
class IofxMeshRenderable;

/**
 \brief The IOFX renderable represents a unit of rendering.
		It contains complete information to render a batch of particles with the same material/mesh in the same render volume.
*/
class IofxRenderable : public ApexInterface
{
public:
	/**
	\brief Type of IOFX renderable
	*/
	enum Type
	{
		SPRITE, //!< Sprite particles type
		MESH //!< Mesh particles type
	};

	/**
	\brief Return Type of this renderable.
	*/
	virtual Type						getType() const = 0;

	/**
	\brief Return Sprite render data for Sprite renderable and NULL for other types.
	*/
	virtual const IofxSpriteRenderData*	getSpriteRenderData() const = 0;

	/**
	\brief Return Mesh render data for Mesh renderable and NULL for other types.
	*/
	virtual const IofxMeshRenderData*	getMeshRenderData() const = 0;

	/**
	\brief Return AABB of this renderable.
	*/
	virtual const physx::PxBounds3&		getBounds() const = 0;

protected:
	virtual	~IofxRenderable() {}

};

PX_POP_PACK

}
} // end namespace nvidia

#endif // IOFX_RENDERABLE_H
