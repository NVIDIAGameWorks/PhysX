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



#ifndef RENDER_MESH_ACTOR_H
#define RENDER_MESH_ACTOR_H

/*!
\file
\brief class RenderMeshActor
*/

#include "Actor.h"
#include "Renderable.h"
#include "UserRenderResourceManager.h"	// For RenderCullMode
#include "foundation/PxVec3.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class UserRenderInstanceBuffer;

/**
\brief Flags used for raycasting an RenderMeshActor
*/
struct RenderMeshActorRaycastFlags
{
	/**
	\brief Enum of flags used for raycasting an RenderMeshActor
	*/
	enum Enum
	{
		VISIBLE_PARTS =	(1 << 0),
		INVISIBLE_PARTS =	(1 << 1),

		ALL_PARTS =			VISIBLE_PARTS | INVISIBLE_PARTS
	};
};

/**
\brief Return data from raycasting an RenderMeshActor
*/
struct RenderMeshActorRaycastHitData
{
	/**
		The time to the hit point.  That is, the hit point is given by worldOrig + time*worldDisp.
	*/
	float		time;

	/**
		The part index containing the hit triangle.
	*/
	uint32_t	partIndex;

	/**
		The submesh index of the triangle hit.
	*/
	uint32_t	submeshIndex;

	/**
		The vertex indices of the triangle hit.
	*/
	uint32_t	vertexIndices[3];

	/**
		The lighting normal at the hit point, if the vertex format supports this.  Otherwise set to (0,0,0).
	*/
	PxVec3		normal;

	/**
		The lighting tangent at the hit point, if the vertex format supports this.  Otherwise set to (0,0,0).
	*/
	PxVec3		tangent;

	/**
		The lighting binormal at the hit point, if the vertex format supports this.  Otherwise set to (0,0,0).
	*/
	PxVec3		binormal;
};


/**
\brief Renderable mesh (dynamic data associated with RenderMeshAsset)
*/
class RenderMeshActor : public Actor, public Renderable
{
public:

	enum
	{
		InvalidInstanceID = 0xFFFFFFFF
	};

	/**
	\brief Returns the visibilities of all mesh parts in the given array.

	The user must supply the array size.  If the size is less than the
	part count, then the list will be truncated to fit the given array.

	Returns true if any of the visibility values in visibilityArray are changed.
	*/
	virtual bool					getVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const = 0;

	/**
	\brief Set the visibility of the indexed part.  Returns true iff the visibility for the part is changed by this operation.
	*/
	virtual bool					setVisibility(bool visible, uint16_t partIndex = 0) = 0;

	/**
	\brief Returns the visibility of the indexed part.
	*/
	virtual bool					isVisible(uint16_t partIndex = 0) const = 0;

	/**
	\brief Returns the number of visible parts.
	*/
	virtual uint32_t				visiblePartCount() const = 0;

	/**
	\brief Returns an array of visible part indices.

	The size of this array is given by visiblePartCount().  Note: the
	indices are in an arbitrary order.
	*/
	virtual const uint32_t*			getVisibleParts() const = 0;

	/**
	\brief Returns the number of bones used by this render mesh
	*/
	virtual uint32_t            	getBoneCount() const = 0;

	/**
	\brief Sets the local-to-world transform for the indexed bone.  The transform need not be orthonormal.
	*/
	virtual void					setTM(const PxMat44& tm, uint32_t boneIndex = 0) = 0;

	/**
	\brief Same as setTM(), but assumes tm is pure rotation.

	This can allow some optimization.  The user must supply scaling
	separately.  The scale vector is interpreted as the diagonal of a
	diagonal matrix, applied before the rotation component of tm.
	*/
	virtual void					setTM(const PxMat44& tm, const PxVec3& scale, uint32_t boneIndex = 0) = 0;

	/**
	\brief Update the axis-aligned bounding box which encloses all visible parts in their world-transformed poses.
	*/
	virtual void					updateBounds() = 0;

	/**
	\brief Returns the local-to-world transform for the indexed bone.
	*/
	virtual const PxMat44			getTM(uint32_t boneIndex = 0) const = 0;

	/**
	\brief If the number of visible parts becomes 0, or if instancing and the number of instances
	becomes 0, then release resources if this bool is true.
	*/
	virtual void					setReleaseResourcesIfNothingToRender(bool value) = 0;

	/**
	\brief If this set to true, render visibility will not be updated until the user calls syncVisibility().
	*/
	virtual void					setBufferVisibility(bool bufferVisibility) = 0;

	/**
	\brief Sets the override material for the submesh with the given index.
	*/
	virtual void					setOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName) = 0;

	/**
	\brief Sync render visibility with that set by the user.  Only
	needed if bufferVisibility(true) is called, or bufferVisibility = true
	in the actor's descriptor.

	If useLock == true, the RenderMeshActor's lock is used during the sync.
	*/
	virtual void					syncVisibility(bool useLock = true) = 0;

	/**
	\brief get the user-provided instance buffer.
	*/
	virtual UserRenderInstanceBuffer* getInstanceBuffer() const = 0;
	/**
	\brief applies the user-provided instance buffer to all submeshes.
	*/
	virtual void					setInstanceBuffer(UserRenderInstanceBuffer* instBuf) = 0;
	/**
	\brief allows the user to change the max instance count in the case that the instance buffer was changed
	*/
	virtual void					setMaxInstanceCount(uint32_t count) = 0;
	/**
	\brief sets the range for the instance buffer

	\param from the position in the buffer (measured in number of elements) to start reading from
	\param count number of instances to be rendered. Must not exceed maxInstances for this actor
	*/
	virtual void					setInstanceBufferRange(uint32_t from, uint32_t count) = 0;

	/**
		Returns true if and only if a part is hit matching various criteria given in the function
		parameters. If a part is hit, the hitData field contains information about the ray intersection.
		(hitData may be modified even if the function returns false.)
			hitData (output) = information about the mesh at the hitpoint.  See RenderMeshActorRaycastHitData.
			worldOrig = the origin of the ray in world space
			worldDisp = the displacement of the ray in world space (need not be normalized)
			flags = raycast control flags (see RenderMeshActorRaycastFlags)
			winding = winding filter for hit triangle.  If RenderCullMode::CLOCKWISE or
				RenderCullMode::COUNTER_CLOCKWISE, then triangles will be assumed to have
				that winding, and will only contribute to the raycast if front-facing.  If
				RenderCullMode::NONE, then all triangles will contribute.
			partIndex = If -1, then all mesh parts will be raycast, and the result returned for
				the earliest hit.  Otherwise only the part indexed by partIndex will be raycast.

		N.B. Currently only works for static (unskinned) and one transform per-part, single-weighted vertex skinning.
	*/
	virtual bool					rayCast(RenderMeshActorRaycastHitData& hitData,
	                                        const PxVec3& worldOrig, const PxVec3& worldDisp,
	                                        RenderMeshActorRaycastFlags::Enum flags = RenderMeshActorRaycastFlags::VISIBLE_PARTS,
	                                        RenderCullMode::Enum winding = RenderCullMode::CLOCKWISE,
	                                        int32_t partIndex = -1) const = 0;

protected:

	virtual							~RenderMeshActor() {}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RENDER_MESH_ACTOR_H
