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



#ifndef CLOTHING_ASSET_H
#define CLOTHING_ASSET_H

#include "Asset.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class ClothingActor;
class ClothingPreview;

#define CLOTHING_AUTHORING_TYPE_NAME "ClothingAsset"


/**
\brief Describes the solver that is used to simulate the clothing actor.
*/
struct ClothSolverMode
{
	/** \brief the enum type */
	enum Enum
	{
		/**
		\brief The actor is simulated using the 3.x cloth solver.
		*/
		v3,
	};
};


/**
\brief Maps from a triangle of the simulated mesh to a vertex of the rendered mesh.
The barycentric coordinates describe the position of a point in space relative to a simulated
triangle. The actual z-coordinate is computed by bary.z = 1 - bary.x - bary.y.
The z part of the vector contains the height of the skinned vertex along the normal of the
simulated triangle.

So, the following computes the new skinned position:

uint32_t i0 = map[mapId].vertexIndex0;
uint32_t i1 = map[mapId].vertexIndex1;
uint32_t i2 = map[mapId].vertexIndex2;

PxVec3 bary = map[mapId].positionBary;
float heightOffset = bary.z * actorScale;
bary.z = (1 - bary.x - bary.y);

PxVec3 posOnTriangle =	physPositions[i0] * bary.x +
						physPositions[i1] * bary.y +
						physPositions[i2] * bary.z;

offsetFromTriangle =	(physNormals[i0] * bary.x +
						physNormals[i1] * bary.y +
						physNormals[i2] * bary.z)
						* heightOffset;

skinnedPos = posOnTriangle + offsetFromTriangle;
*/
struct ClothingMeshSkinningMap
{
	/**
	\brief Barycentric coordinates of the graphical vertex relative to the simulated triangle.
	\note PX_MAX_F32 values represent invalid coordinates.
	*/
	PxVec3 positionBary;

	/**
	\brief First vertex of the simulated triangle.
	*/
	uint32_t vertexIndex0;

	/**
	\brief Barycentric coordinates of a point that represents the normal.
	The actual normal can be extracted by subtracting the skinned position from this point.
	\note PX_MAX_F32 values represent invalid coordinates.
	*/
	PxVec3 normalBary;

	/**
	\brief Second vertex of the simulated triangle.
	*/
	uint32_t vertexIndex1;

	/**
	\brief Barycentric coordinates of a point that represents the tangent.
	The actual tangent can be extracted by subtracting the skinned position from this point.
	\note PX_MAX_F32 values represent invalid coordinates.
	*/
	PxVec3 tangentBary;

	/**
	\brief Third vertex of the simulated triangle.
	*/
	uint32_t vertexIndex2;
};

/**
\brief A clothing asset. It contains all the static and shared data for a given piece of clothing.
*/
class ClothingAsset : public Asset
{
protected:
	// do not manually delete this object
	virtual ~ClothingAsset() {}
public:
	/**
	\brief Returns the number of ClothingActors this asset has created.
	*/
	virtual uint32_t getNumActors() const = 0;

	/**
	\brief Returns the index'th ClothingActor
	*/
	virtual ClothingActor* getActor(uint32_t index) = 0;

	/**
	\brief Returns the bounding box for the asset.
	*/
	virtual PxBounds3 getBoundingBox() const = 0;

	/**
	\brief returns the number of LOD levels present in this asset
	*/
	virtual uint32_t getNumGraphicalLodLevels() const = 0;

	/**
	\brief returns the actual LOD value of any particular level, normally this is just the identity map
	*/
	virtual uint32_t getGraphicalLodValue(uint32_t lodLevel) const = 0;

	/**
	\brief returns the biggest max distance of any vertex in any physical mesh
	*/
	virtual float getBiggestMaxDistance() const = 0;

	/**
	\brief returns for which solver the asset has been cooked for.
	*/
	virtual ClothSolverMode::Enum getClothSolverMode() const = 0;

	/**
	\brief remaps bone with given name to a new index for updateState calls

	This is needed when the order of bones differs from authoring tool to runtime

	\note must be called after asset is deserialized. Changing this any later, especially after the first ClothingActor::updateState call
	will lead to ill-defined behavior.
	*/
	virtual bool remapBoneIndex(const char* name, uint32_t newIndex) = 0;

	/**
	\brief Returns the number of bones
	*/
	virtual uint32_t getNumBones() const = 0;

	/**
	\brief Returns the number of bones that are actually used. They are the first ones internally.
	*/
	virtual uint32_t getNumUsedBones() const = 0;

	/**
	\brief Returns the name of the bone at the given internal index.
	*/
	virtual const char* getBoneName(uint32_t internalIndex) const = 0;

	/**
	\brief Returns the bind pose transform for this bone.
	*/
	virtual bool getBoneBasePose(uint32_t internalIndex, PxMat44& result) const = 0;

	/**
	\brief returns the mapping from internal to external indices for a given asset.

	Use this map to transform all the boneIndices returned from the ClothingPhysicalMesh into the order you specified initially
	*/
	virtual void getBoneMapping(uint32_t* internal2externalMap) const = 0;

	/**
	\brief Gets the RenderMeshAsset associated with this asset.

	\param [in] lodLevel	The LoD level of the render mesh asset
	\return NULL if lodLevel is not valid
	\see ClothingAsset::getNumGraphicalLodLevels()
	*/
	virtual const RenderMeshAsset*	getRenderMeshAsset(uint32_t lodLevel) const = 0;

	/**
	\brief Prepares the asset for use with morph target.

	When setting morph target displacements, the asset needs to create a mapping of external to internal vertex order.
	These positions will be matched with the internal mesh positions with a smallest distance criterion.

	\param [in] originalPositions	Array of positions before the morphing is applied.
	\param [in] numPositions		Length of the Array
	\param [in] epsilon				Difference two vertices can have before being reported
	\return							Number of vertices for which the distance to the next originalPosition was larger than epsilon
	*/
	virtual uint32_t prepareMorphTargetMapping(const PxVec3* originalPositions, uint32_t numPositions, float epsilon) = 0;
	
	/**
	\brief Returns the size of the mesh skinning map.
	\note This call has the side effect of merging the immediate map into the mesh skinning map. This might affect skinning performance done by APEX.

	\return							Number of entries in the map
	*/
	virtual uint32_t getMeshSkinningMapSize(uint32_t lod) = 0;

	/**
	\brief Provides the mesh to mesh skinning map.

	This map stores for each graphics mesh vertex the corresponding triangle indices of the physics mesh and barycentric coordinates to
	skin the vertex position, normal and tangent.
	\note This call has the side effect of merging the immediate map into the mesh skinning map. This might affect skinning performance done by APEX.

	\param [in]		lod					lod for which the map is requested
	\param [out]	map					Skinning map
	*/
	virtual void getMeshSkinningMap(uint32_t lod, ClothingMeshSkinningMap* map) = 0;

	/**
	\brief Releases all data needed to compute render data in apex clothing.
	*/
	virtual bool releaseGraphicalData() = 0;
};

PX_POP_PACK

} // namespace nvidia
} // namespace nvidia

#endif // CLOTHING_ASSET_H
