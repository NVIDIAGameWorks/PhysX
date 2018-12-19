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



#ifndef FRACTURE_TOOLS_H
#define FRACTURE_TOOLS_H

#include "Px.h"
#include "ExplicitHierarchicalMesh.h"
#include "FractureToolsStructs.h"

PX_PUSH_PACK_DEFAULT

namespace nvidia
{
	namespace apex
	{
		struct IntersectMesh;
		class DestructibleAsset;
	}
}

namespace FractureTools
{

/**
	Tools for fracturing meshes.
*/


/** Instantiates a blank CutoutSet */
CutoutSet* createCutoutSet();

/**
	Builds a cutout set (which must have been initially created by createCutoutSet()).
	Uses a bitmap described by pixelBuffer, bufferWidth, and bufferHeight.  Each pixel is represented
	by one byte in the buffer.

	cutoutSet: the CutoutSet to build
	pixelBuffer: pointer to be beginning of the pixel buffer
	bufferWidth: the width of the buffer in pixels
	bufferHeight: the height of the buffer in pixels
	snapThreshold: the pixel distance at which neighboring cutout vertices and segments may be fudged into alignment.
	periodic: whether or not to use periodic boundary conditions when creating cutouts from the map
*/
void buildCutoutSet(CutoutSet& cutoutSet, const uint8_t* pixelBuffer, uint32_t bufferWidth, uint32_t bufferHeight, float snapThreshold, bool periodic);

/**
	Calculate the mapping between a cutout fracture map and a given triangle.
	The result is a 3 by 3 matrix M composed by an affine transformation and a rotation, we can get the 3-D projection for a texture coordinate pair (u,v) with such a formula:
	(x,y,z) = M*PxVec3(u,v,1)

	triangle: the target face's normal
	theMapping: resulted mapping, composed by an affine transformation and a rotation
*/
bool calculateCutoutUVMapping(const nvidia::ExplicitRenderTriangle& triangle, physx::PxMat33& theMapping);

/**
	Uses the passed-in target direction to find the best triangle in the root mesh with normal near the given targetDirection.  If triangles exist
	with normals within one degree of the given target direction, then one with the greatest area of such triangles is used.  Otherwise, the triangle
	with normal closest to the given target direction is used.  The resulting triangle is used to calculate a UV mapping as in the function
	calculateCutoutUVMapping (above).

	The assumption is that there exists a single mapping for all triangles on a specified face, for this feature to be useful. 

	hMesh: the explicit mesh with well rectangle-shaped faces
	targetDirection: the target face's normal
	theMapping: resulted mapping, composed by an affine transformation and a rotation
*/
bool calculateCutoutUVMapping(nvidia::ExplicitHierarchicalMesh& hMesh, const physx::PxVec3& targetDirection, physx::PxMat33& theMapping);

/**
	Splits the mesh in chunk[0], forming fractured pieces chunks[1...] using
	Voronoi decomposition fracturing.

	hMesh: the mesh to split
	iHMeshCore: if this mesh is not empty, chunk 0 will be used as an indestructible "core" of the fractured
	mesh.  That is, it will be subtracted from hMesh, and placed at level 1 of the hierarchy.  The remainder
	of hMesh will be split as usual, creating chunks at level 1 (and possibly deeper).
	exportCoreMesh: if true, a core mesh chunk will be created from iHMeshCore
	coreMeshImprintSubmeshIndex: if this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh
	meshProcessingParams: describes generic mesh processing directives
	desc: describes the voronoi splitting parameters surfaces (see FractureVoronoiDesc)
	collisionDesc: convex hulls will be generated for each chunk using the method.  See CollisionDesc.
	randomSeed: seed for the random number generator, to ensure reproducibility.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	cancel: if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

	returns true if successful.
*/
bool	createVoronoiSplitMesh
(
	nvidia::ExplicitHierarchicalMesh& hMesh,
	nvidia::ExplicitHierarchicalMesh& iHMeshCore,
	bool exportCoreMesh,
	int32_t coreMeshImprintSubmeshIndex,
	const MeshProcessingParameters& meshProcessingParams,
	const FractureVoronoiDesc& desc,
	const nvidia::CollisionDesc& collisionDesc,
	uint32_t randomSeed,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel = NULL
);

/**
	Generates a set of uniformly distributed points in the interior of the root mesh.

	hMesh: the mesh in which to distribute sites
	siteBuffer: an array of physx::PxVec3, at least the size of siteCount
	siteChunkIndices: if not NULL, then must be at least the size of siteCount.  siteCount indices will be written to this buffer, associating each site with a chunk that contains it.
	siteCount: the number of points to write into siteBuffer
	randomSeed: pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
	microgridSize: pointer to a grid size used for BSP creation.  If NULL, the default settings will be used.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	meshMode: Open mesh handling.  Modes: Automatic, Closed, Open (see BSPOpenMode)
	chunkIndex: If this is a valid index, the voronoi sites will only be created within the volume of the indexed chunk.  Otherwise,
		the sites will be created within each of the root-level chunks.  Default value is an invalid index.

	returns the number of sites actually created (written to siteBuffer and siteChunkIndices).  This may be less than the number of sites requested if site placement fails.
*/
uint32_t	createVoronoiSitesInsideMesh
(
	nvidia::ExplicitHierarchicalMesh& hMesh,
	physx::PxVec3* siteBuffer,
	uint32_t* siteChunkIndices,
	uint32_t siteCount,
	uint32_t* randomSeed,
	uint32_t* microgridSize,
	nvidia::BSPOpenMode::Enum meshMode,
	nvidia::IProgressListener& progressListener,
	uint32_t chunkIndex = 0xFFFFFFFF
);

/**
	Creates scatter mesh sites randomly distributed on the mesh.

	meshIndices: user-allocated array of size scatterMeshInstancesBufferSize which will be filled in by this function, giving the scatter mesh index used
	relativeTransforms: user-allocated array of size scatterMeshInstancesBufferSize which will be filled in by this function, giving the chunk-relative transform for each chunk instance
	chunkMeshStarts: user-allocated array which will be filled in with offsets into the meshIndices and relativeTransforms array.
		For a chunk indexed by i, the corresponding range [chunkMeshStart[i], chunkMeshStart[i+1]-1] in meshIndices and relativeTransforms is used.
		*NOTE*: chunkMeshStart array must be of at least size N+1, where N is the number of chunks in the base explicit hierarchical mesh.
	scatterMeshInstancesBufferSize: the size of meshIndices and relativeTransforms array.
	scatterMeshInstancesBufferSize: the size of meshIndices and relativeTransforms array.
	hMesh: the mesh in which to distribute sites
	targetChunkCount: how many chunks are in the array targetChunkIndices
	targetChunkIndices: an array of chunk indices which are candidates for scatter meshes.  The elements in the array chunkIndices will come from this array
	randomSeed: pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
	scatterMeshAssetCount: the number of different scatter meshes (not instances).  Should not exceed 255.  If scatterMeshAssetCount > 255, only the first 255 will be used.
	scatterMeshAssets: an array of size scatterMeshAssetCount, of the render mesh assets which will be used for the scatter meshes
	minCount: an array of size scatterMeshAssetCount, giving the minimum number of instances to place for each mesh
	maxCount: an array of size scatterMeshAssetCount, giving the maximum number of instances to place for each mesh
	minScales: an array of size scatterMeshAssetCount, giving the minimum scale to apply to each scatter mesh
	maxScales: an array of size scatterMeshAssetCount, giving the maximum scale to apply to each scatter mesh
	maxAngles: an array of size scatterMeshAssetCount, giving a maximum deviation angle (in degrees) from the surface normal to apply to each scatter mesh

	return value: the number of instances placed in indices and relativeTransforms (will not exceed scatterMeshInstancesBufferSize)
*/
uint32_t	createScatterMeshSites
(
	uint8_t*						meshIndices,
	physx::PxMat44*						relativeTransforms,
	uint32_t*						chunkMeshStarts,
	uint32_t						scatterMeshInstancesBufferSize,
	nvidia::ExplicitHierarchicalMesh&	hMesh,
	uint32_t						targetChunkCount,
	const uint16_t*					targetChunkIndices,
	uint32_t*						randomSeed,
	uint32_t						scatterMeshAssetCount,
	nvidia::RenderMeshAsset**			scatterMeshAssets,
	const uint32_t*					minCount,
	const uint32_t*					maxCount,
	const float*					minScales,
	const float*					maxScales,
	const float*					maxAngles
);

/**
	Utility to visualize Voronoi cells for a given set of sites.

	debugRender: rendering object which will receive the drawing primitives associated with this cell visualization
	sites: an array of Voronoi cell sites, of length siteCount
	siteCount: the number of Voronoi cell sites (length of sites array)
	cellColors: an optional array of colors (see ApexRenderDebug for format) for the cells.  If NULL, the white (0xFFFFFFFF) color will be used.
		If not NULL, this (of length cellColorCount) is used to color the cell graphics.  The number cellColorCount need not match siteCount.  If
		cellColorCount is less than siteCount, the cell colors will cycle.  That is, site N gets cellColor[N%cellColorCount].
	cellColorCount: the number of cell colors (the length of cellColors array)
	bounds: defines an axis-aligned bounding box which clips the visualization, since some cells extend to infinity
	cellIndex: if this is a valid index (cellIndex < siteCount), then only the cell corresponding to sites[cellIndex] will be drawn.  Otherwise, all cells will be drawn.
*/
void	visualizeVoronoiCells
(
	nvidia::RenderDebugInterface& debugRender,
	const physx::PxVec3* sites,
	uint32_t siteCount,
	const uint32_t* cellColors,
	uint32_t cellColorCount,
	const physx::PxBounds3& bounds,
	uint32_t cellIndex = 0xFFFFFFFF
);

/**
	Builds a new ExplicitHierarchicalMesh from an array of triangles.

	iHMesh: the ExplicitHierarchicalMesh to build
	meshTriangles: pointer to array of ExplicitRenderTriangles which make up the mesh
	meshTriangleCount the size of the meshTriangles array
	submeshData: pointer to array of ExplicitSubmeshData, describing the submeshes
	submeshCount: the size of the submeshData array
	meshPartition: if not NULL, an array of size meshPartitionCount, giving the end elements of contiguous subsets of meshTriangles.
		If meshPartition is NULL, one partition is assumed.
		When there is one partition, these triangles become the level 0 part.
		When there is more than one partition, these triangles become level 1 parts, the behavior is determined by firstPartitionIsDepthZero (see below).
	meshPartitionCount: if meshPartition is not NULL, this is the size of the meshPartition array.
	parentIndices: if not NULL, the parent indices for each chunk (corresponding to a partition in the mesh partition).
	parentIndexCount: the size of the parentIndices array.  This does not need to match meshPartitionCount.  If a mesh partition has an index beyond the end of parentIndices,
		then the parentIndex is considered to be 0.  Therefore, if parentIndexCount = 0, all parents are 0 and so all chunks created will be depth 1.  This will cause a
		depth 0 chunk to be created that is the aggregate of the depth 1 chunks.  If parentIndexCount > 0, then the depth-0 chunk must have a parentIndex of -1.
		To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = true, set parentIndices to the address of a int32_t containing the value -1, and set parentIndexCount = 1.
		To reproduce the effect of the old parameter 'firstPartitionIsDepthZero' = false, set parentIndexCount = 0.
		Note: if parent indices are given, the first one must be -1, and *only* that index may be negative.  That is, there may be only one depth-0 mesh and it must be the first mesh.
*/
bool buildExplicitHierarchicalMesh
(
    nvidia::ExplicitHierarchicalMesh& iHMesh,
    const nvidia::ExplicitRenderTriangle* meshTriangles,
    uint32_t meshTriangleCount,
    const nvidia::ExplicitSubmeshData* submeshData,
    uint32_t submeshCount,
    uint32_t* meshPartition = NULL,
    uint32_t meshPartitionCount = 0,
	int32_t* parentIndices = NULL,
	uint32_t parentIndexCount = 0
);

/**
	Set the tolerances used in CSG calculations with BSPs.

	linearTolerance: relative (to mesh size) tolerance used with angularTolerance to determine coplanarity.  Default = 1.0e-4.
	angularTolerance: used with linearTolerance to determine coplanarity.  Default = 1.0e-3
	baseTolerance: relative (to mesh size) tolerance used for spatial partitioning
	clipTolerance: relative (to mesh size) tolerance used when clipping triangles for CSG mesh building operations.  Default = 1.0e-4.
	cleaningTolerance: relative (to mesh size) tolerance used when cleaning the out put mesh generated from the toMesh() function.  Default = 1.0e-7.
*/
void setBSPTolerances
(
	float linearTolerance,
	float angularTolerance,
	float baseTolerance,
	float clipTolerance,
	float cleaningTolerance
);

/**
	Set the parameters used in BSP building operations.

	logAreaSigmaThreshold:	At each step in the tree building process, the surface with maximum triangle area is compared
		to the other surface triangle areas.  If the maximum area surface is far from the "typical" set of
		surface areas, then that surface is chosen as the next splitting plane.  Otherwise, a random
		test set is chosen and a winner determined based upon the weightings below.
		The value logAreaSigmaThreshold determines how "atypical" the maximum area surface must be to
		be chosen in this manner.
		Default value = 2.0.
	testSetSize: Larger values of testSetSize may find better BSP trees, but will take more time to create.
		testSetSize = 0 is treated as infinity (all surfaces will be tested for each branch).
		Default value = 10.
	splitWeight: How much to weigh the relative number of triangle splits when searching for a BSP surface.
		Default value = 0.5.
	imbalanceWeight: How much to weigh the relative triangle imbalance when searching for a BSP surface.
		Default value = 0.0.
*/
void setBSPBuildParameters
(
	float logAreaSigmaThreshold,
	uint32_t testSetSize,
	float splitWeight,
	float imbalanceWeight
);


/** 
	Builds the root ExplicitHierarchicalMesh from an RenderMeshAsset.
	Since an DestructibleAsset contains no hierarchy information, the input mesh must have only one part.

	iHMesh: the ExplicitHierarchicalMesh to build
	renderMeshAsset: Input RenderMesh asset
	maxRootDepth: cap the root depth at this value.  Re-fracturing of the mesh will occur at this depth.  Default = UINT32_MAX
*/
bool buildExplicitHierarchicalMeshFromRenderMeshAsset(nvidia::ExplicitHierarchicalMesh& iHMesh, const nvidia::RenderMeshAsset& renderMeshAsset, uint32_t maxRootDepth = UINT32_MAX);

/** 
	Builds the root ExplicitHierarchicalMesh from an DestructibleAsset.
	Since an DestructibleAsset contains hierarchy information, the explicit mesh formed
	will have this hierarchy structure.

	iHMesh: the ExplicitHierarchicalMesh to build
	destructibleAsset: Input Destructible asset
	maxRootDepth: cap the root depth at this value.  Re-fracturing of the mesh will occur at this depth.  Default = UINT32_MAX
*/
bool buildExplicitHierarchicalMeshFromDestructibleAsset(nvidia::ExplicitHierarchicalMesh& iHMesh, const nvidia::DestructibleAsset& destructibleAsset, uint32_t maxRootDepth = UINT32_MAX);

/**
	Partitions (and possibly re-orders) the mesh array if the triangles form disjoint islands.
	mesh: pointer to array of ExplicitRenderTriangles which make up the mesh
	meshTriangleCount: the size of the meshTriangles array
	meshPartition: user-allocated array for mesh partition, will be filled with the end elements of contiguous subsets of meshTriangles.
	meshPartitionMaxCount: size of user-allocated meshPartitionArray
	padding: distance (as a fraction of the mesh size) to consider vertices touching

	Returns the number of partitions.  The value may be larger than meshPartitionMaxCount.  In that case, the partitions beyond meshPartitionMaxCount are not recorded.
*/
uint32_t partitionMeshByIslands
(
	nvidia::ExplicitRenderTriangle* mesh,
	uint32_t meshTriangleCount,
    uint32_t* meshPartition,
    uint32_t meshPartitionMaxCount,
	float padding = 0.0001f
);

/**
	Splits the mesh in chunk[0], forming a hierarchy of fractured meshes in chunks[1...]

	hMesh: the mesh to split
	iHMeshCore: if this mesh is not empty, chunk 0 will be used as an indestructible "core" of the fractured
		mesh.  That is, it will be subtracted from hMesh, and placed at level 1 of the hierarchy.  The remainder
		of hMesh will be split as usual, creating chunks at level 1 (and possibly deeper).
	exportCoreMesh: if true, a core mesh chunk will be created from iHMeshCore
	coreMeshImprintSubmeshIndex: if this is < 0, use the core mesh materials (was applyCoreMeshMaterialToNeighborChunks).  Otherwise, use the given submesh
	meshProcessingParams: describes generic mesh processing directives
	desc: describes the slicing surfaces (see FractureSliceDesc)
	collisionDesc: convex hulls will be generated for each chunk using the method.  See CollisionDesc.
	randomSeed: seed for the random number generator, to ensure reproducibility.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	cancel: if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

	returns true if successful.
*/
bool createHierarchicallySplitMesh
(
    nvidia::ExplicitHierarchicalMesh& hMesh,
    nvidia::ExplicitHierarchicalMesh& iHMeshCore,
    bool exportCoreMesh,
	int32_t coreMeshImprintSubmeshIndex,
    const MeshProcessingParameters& meshProcessingParams,
    const FractureSliceDesc& desc,
	const nvidia::CollisionDesc& collisionDesc,
    uint32_t randomSeed,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel = NULL
);

/**
	Chips the mesh in chunk[0], forming a hierarchy of fractured meshes in chunks[1...]

	hMesh: the mesh to split
	meshProcessingParams: describes generic mesh processing directives
	desc: describes the slicing surfaces (see FractureCutoutDesc)
	iCutoutSet: the cutout set to use for fracturing (see CutoutSet)
	sliceDesc: used if desc.chunkFracturingMethod = SliceFractureCutoutChunks
	voronoiDesc: used if desc.chunkFracturingMethod = VoronoiFractureCutoutChunks
	collisionDesc: convex hulls will be generated for each chunk using the method.  See CollisionDesc.
	randomSeed: seed for the random number generator, to ensure reproducibility.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	cancel: if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

	returns true if successful.
*/
bool createChippedMesh
(
    nvidia::ExplicitHierarchicalMesh& hMesh,
    const MeshProcessingParameters& meshProcessingParams,
    const FractureCutoutDesc& desc,
    const CutoutSet& iCutoutSet,
    const FractureSliceDesc& sliceDesc,
	const FractureVoronoiDesc& voronoiDesc,
	const nvidia::CollisionDesc& collisionDesc,
    uint32_t randomSeed,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel = NULL
);

/**
	Splits the chunk in chunk[chunkIndex], forming a hierarchy of fractured chunks using
	slice-mode fracturing.  The chunks will be rearranged so that they are in breadth-first order.

	hMesh: the ExplicitHierarchicalMesh to act upon
	chunkIndex: index of chunk to be split
	meshProcessingParams: used to create a BSP for this chunk
	desc: describes the slicing surfaces (see FractureSliceDesc)
	collisionDesc: convex hulls will be generated for each chunk using the method.  See CollisionDesc.
	randomSeed: pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	cancel: if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

	returns true if successful.
*/
bool hierarchicallySplitChunk
(
	nvidia::ExplicitHierarchicalMesh& hMesh,
	uint32_t chunkIndex,
	const MeshProcessingParameters& meshProcessingParams,
	const FractureSliceDesc& desc,
	const nvidia::CollisionDesc& collisionDesc,
	uint32_t* randomSeed,
	nvidia::IProgressListener& progressListener,
	volatile bool* cancel = NULL
);

/**
	Splits the chunk in chunk[chunkIndex], forming fractured chunks using
	Voronoi decomposition fracturing.  The chunks will be rearranged so that they are in breadth-first order.

	hMesh: the ExplicitHierarchicalMesh to act upon
	chunkIndex: index of chunk to be split
	meshProcessingParams: describes generic mesh processing directives
	desc: describes the voronoi splitting parameters surfaces (see FractureVoronoiDesc)
	collisionDesc: convex hulls will be generated for each chunk using the method.  See CollisionDesc.
	randomSeed: pointer to a seed for the random number generator, to ensure reproducibility.  If NULL, the random number generator will not be re-seeded.
	progressListener: The user must instantiate an IProgressListener, so that this function may report progress of this operation
	cancel: if not NULL and *cancel is set to true, the root mesh will be restored to its original state, and the function will return at its earliest opportunity.  Meant to be set from another thread.

	returns true if successful.
*/
bool voronoiSplitChunk
(
	nvidia::ExplicitHierarchicalMesh& hMesh,
	uint32_t chunkIndex,
    const MeshProcessingParameters& meshProcessingParams,
	const FractureVoronoiDesc& desc,
	const nvidia::CollisionDesc& collisionDesc,
    uint32_t* randomSeed,
    nvidia::IProgressListener& progressListener,
    volatile bool* cancel = NULL
);

/**
	Builds a mesh used for slice fracturing, given the noise parameters and random seed.  This function is mostly intended
	for visualization - to give the user a "typical" slice surface used for fracturing.
*/
bool buildSliceMesh
(
	nvidia::IntersectMesh& intersectMesh,
	nvidia::ExplicitHierarchicalMesh& referenceMesh,
	const physx::PxPlane& slicePlane,
	const FractureTools::NoiseParameters& noiseParameters,
	uint32_t randomSeed
);

/** Instantiates an ExplicitHierarchicalMesh */
nvidia::ExplicitHierarchicalMesh*	createExplicitHierarchicalMesh();

/** Instantiates an ExplicitHierarchicalMesh::ConvexHull */
nvidia::ExplicitHierarchicalMesh::ConvexHull*	createExplicitHierarchicalMeshConvexHull();

PX_POP_PACK

} // namespace FractureTools

#endif // FRACTURE_TOOLS_H
