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


#ifndef FRACTURE_TOOLS_STRUCTS_H
#define FRACTURE_TOOLS_STRUCTS_H

#include "foundation/Px.h"
#include "ExplicitHierarchicalMesh.h"

PX_PUSH_PACK_DEFAULT

namespace FractureTools
{

/**
	These parameters are passed into the fracturing functions to guide mesh processing.
*/
struct MeshProcessingParameters
{
	/**
		If this is true, separate mesh islands will be turned into separate chunks.
		Default = false.
	*/
	bool islandGeneration;

	/**
		If this is true, all T-junctions will be removed from the mesh.
		Default = false.
	*/
	bool removeTJunctions;

	/**
		The mesh is initially scaled to fit in a unit cube, then (if gridSize is not
		zero), the vertices of the scaled mesh are snapped to a grid of size 1/gridSize.
		A power of two is recommended.
		Default = 65536.
	*/
	unsigned microgridSize;

	/**
	Open mesh handling.  Modes: Automatic, Closed, Open (see BSPOpenMode)
	Closed mode assumes the mesh is closed and attempts to insert interior faces.
	Open mode assumes the mesh is open and does not insert interior faces.
	Automatic mode attempts to determine if the mesh is open or closed, and act accordingly.

	Default is Automatic mode.
	*/
	nvidia::BSPOpenMode::Enum meshMode;

	/**
		Debug output verbosity level.  The higher the number, the more messages are output.
		Default = 0.
	*/
	int verbosity;

	/** Constructor sets defaults */
	MeshProcessingParameters()
	{
		setToDefault();
	}

	/** Set default values */
	void	setToDefault()
	{
		islandGeneration = false;
		removeTJunctions = false;
		microgridSize = 65536;
		meshMode = nvidia::BSPOpenMode::Automatic;
		verbosity = 0;
	}
};


/**
	Interface to a "cutout set," used with chippable fracturing.  A cutout set is created from a bitmap.  The
	result is turned into cutouts which are applied to the mesh.  For example, a bitmap which looks like a brick
	pattern will generate a cutout for each "brick," forming the cutout set.

	Each cutout is a 2D entity, meant to be projected onto various faces of a mesh.  They are represented
	by a set of 2D vertices, which form closed loops.  More than one loop may represent a single cutout, if
	the loops are forced to be convex.  Otherwise, a cutout is represented by a single loop.
*/
class CutoutSet
{
public:
	/** Returns the number of cutouts in the set. */
	virtual uint32_t				getCutoutCount() const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the number of vertices in the cutout.
	*/
	virtual uint32_t				getCutoutVertexCount(uint32_t cutoutIndex) const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the number of loops in this cutout.
	*/
	virtual uint32_t				getCutoutLoopCount(uint32_t cutoutIndex) const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the vertex indexed by vertexIndex.  (Only the X and Y coordinates are used.)
	*/
	virtual const physx::PxVec3&	getCutoutVertex(uint32_t cutoutIndex, uint32_t vertexIndex) const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the number of vertices in the loop indexed by loopIndex.
	*/
	virtual uint32_t				getCutoutLoopSize(uint32_t coutoutIndex, uint32_t loopIndex) const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the vertex index of the vertex indexed by vertexNum, in the loop
		indexed by loopIndex.
	*/
	virtual uint32_t				getCutoutLoopVertexIndex(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexNum) const = 0;

	/**
		Applies to the cutout indexed by cutoutIndex:
		Returns the flags of the vertex indexed by vertexNum, in the loop
		indexed by loopIndex.
	*/
	virtual uint32_t				getCutoutLoopVertexFlags(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexNum) const = 0;

	/**
		Whether or not this cutout set is to be tiled.
	*/
	virtual bool					isPeriodic() const = 0;

	/**
		The dimensions of the fracture map used to create the cutout set.
	*/
	virtual const physx::PxVec2&	getDimensions() const = 0;

	/** Serialization */
	//virtual void			serialize(physx::PxFileBuf& stream) const = 0;
	//virtual void			deserialize(physx::PxFileBuf& stream) = 0;

	/** Releases all memory and deletes itself. */
	virtual void					release() = 0;

protected:
	/** Protected destructor.  Use the release() method. */
	virtual							~CutoutSet() {}
};


/**
	NoiseParameters
	These parameters are used to build a splitting surface.
*/
struct NoiseParameters
{
	/**
		Size of the fluctuations, relative to mesh size
	*/
	float	amplitude;

	/**
		Noise frequencey relative to 1/(grid spacing).  On scales much smaller than this, the function is smooth.
		On scales much large, the function looks uncorrelated
	*/
	float	frequency;

	/**
		Suggested number of grid elements across the mesh.  The actual number may vary depending
		on the mesh's proportions.
	*/
	int		gridSize;

	/**
		Noise function to use.  This parameter is currently unused.
		Noise is generated by superposition of many fourier modes in random directions,
		with frequencies randomly chosen in a band around the input frequency,
	*/
	int		type;

	/** Constructor sets defaults */
	NoiseParameters()
	{
		setToDefault();
	}

	/**
		Set default values:

		amplitude = 0.0f;
		frequency = 0.25f;
		gridSize = 10;
		type = 0;
	*/
	void	setToDefault()
	{
		amplitude = 0.0f;
		frequency = 0.25f;
		gridSize = 10;
		type = 0;
	}
};


/**
	SliceParameters

	The slicing parameters for X, Y, and Z slicing of a mesh.
*/
struct SliceParameters
{
	/**
		Which axis order to slice the mesh.
		This only matters if there is randomness in the slice surface.
	*/
	enum Order
	{
		XYZ,
		YZX,
		ZXY,
		ZYX,
		YXZ,
		XZY,
		Through
	};

	/** The slicing order (see the Order enum) */
	unsigned order;

	/** How many times to slice along each axis */
	unsigned splitsPerPass[3];

	/**
		Variation in slice position along each axis.  This is a relative quantity.
		linearVariation[axis] = 0 means the slicing offsets are evenly spaced across the mesh along the axis.
		linearVariation[axis] = 1 means the slicing offsets are randomly chosen in a range of width 1/(splitsPerPass[axis]+1)
								  times the width of the mesh along the axis.
	*/
	float linearVariation[3];

	/**
		Variation in the slice surface angle along each axis.
		0 variation means the slice surfaces are axis-aligned.  Otherwise, the surface normal will be varied randomly,
		with angle to the axis somewhere within the given variation (in radians).
	*/
	float angularVariation[3];

	/** The noise for each slicing direction */
	NoiseParameters	noise[3];

	/** Constructor sets defaults */
	SliceParameters()
	{
		setToDefault();
	}

	/** Sets all NoiseParameters to their defaults:
		order = XYZ;
		splitsPerPass[0] = splitsPerPass[1] = splitsPerPass[2] = 1;
		linearVariation[0] = linearVariation[1] = linearVariation[2] = 0.1f;
		angularVariation[0] = angularVariation[1] = angularVariation[2] = 20.0f*3.1415927f/180.0f;
		noise[0].setToDefault();
		noise[1].setToDefault();
		noise[2].setToDefault();
	*/
	void	setToDefault()
	{
		order = XYZ;
		splitsPerPass[0] = splitsPerPass[1] = splitsPerPass[2] = 1;
		linearVariation[0] = linearVariation[1] = linearVariation[2] = 0.1f;
		angularVariation[0] = angularVariation[1] = angularVariation[2] = 20.0f * 3.1415927f / 180.0f;
		noise[0].setToDefault();
		noise[1].setToDefault();
		noise[2].setToDefault();
	}
};


/**
	FractureSliceDesc

	Descriptor for slice-mode fracturing.
*/
struct FractureSliceDesc
{
	/** How many times to recurse the slicing process */
	unsigned						maxDepth;

	/** Array of slice parameters; must be of length maxDepth */
	SliceParameters*				sliceParameters;

	/**
		If this is true, the targetProportions (see below) will be used.
	*/
	bool							useTargetProportions;

	/**
		If useTargetProportions is true, the splitsPerPass values will not necessarily be used.
		Instead, the closest values will be chosen at each recursion of slicing, in order to make
		the pieces match the target proportions as closely as possible.

		Note: the more noise there is in the slicing surfaces, the less accurate these proportions will be.
	*/
	float							targetProportions[3];

	/**
		Smallest size allowed for chunks, as measured by their bounding box widths along the x, y, and z axes.
		The sizes are interpreted as fractions of the unfractured mesh's bounding box width.
		Chunks created by fracturing which are smaller than this limit will be removed.  Note - this will leave holes at the given depth.
		Default = (0,0,0), which disables this feature.
	*/
	float							minimumChunkSize[3];

	/**
		Material application descriptor used for each slice axis.
	*/
	nvidia::FractureMaterialDesc	materialDesc[3];

	/**
		If instanceChunks is true, corresponding chunks in different destructible actors will be instanced.
	*/
	bool							instanceChunks;

	/**
		If true, slice geometry and noise will be stored in a separate displacement offset buffer.
	*/
	bool							useDisplacementMaps;

	/** Enum describing creation of displacement maps. */
	enum NoiseMode
	{
		NoiseWavePlane = 0,
		NoisePerlin2D,
		NoisePerlin3D,

		NoiseModeCount
	};

	/**
		The noise mode.  If displacement maps are enabled, NoisePerlin3D will be used.
	*/
	unsigned			noiseMode;

	/** Constructor sets defaults */
	FractureSliceDesc()
	{
		setToDefault();
	}

	/**
		Sets the default values:

		maxDepth = 0;
		sliceParameters = NULL;
		useTargetProportions = false;
		targetProportions[0..2] = 1.0f;
		minimumChunkSize[0..2] = 0.0f;
		materialDesc[0..2].setToDefault();
		instanceChunks = false;
		useDisplacementMaps = false;
		noiseMode = NoiseWavePlane;
	*/
	void	setToDefault()
	{
		maxDepth             = 0;
		sliceParameters      = NULL;
		useTargetProportions = false;
		for (int i = 0; i < 3; ++i)
		{
			targetProportions[i] = 1.0f;
			minimumChunkSize[i] = 0.0f;
			materialDesc[i].setToDefault();
		}
		instanceChunks       = false;
		useDisplacementMaps  = false;
		noiseMode            = NoiseWavePlane;
	}
};


/**
	CutoutParameters

	Parameters for a single cutout direction.
*/
struct CutoutParameters
{
	/**
		The depth to apply cutout fracturing.
		0 has a special value; it means cut all the way through
	*/
	float depth;

	/**
		Material application descriptor used for the cutout direction.
		Note: The backface slice will use the U-direction and UV offset specified in each descriptor, however the cutout slices (surrounding
		each cutout chunk) will ignore these fields (only using the UV scale).
	*/
	nvidia::FractureMaterialDesc materialDesc;

	/**
		Describes the characteristics of the backface cutting surface (along the various cutout directions).
		If the noise is 0, the cutting surface will be a plane.  Otherwise, there will be some variation,
		or roughness, to the surface.
	*/
	NoiseParameters	backfaceNoise;

	/**
		Describes the characteristics of the perimeter cutting surfaces (for the various cutout directions).
		If the noise is 0, the cutting surface will smooth.  Otherwise, there will be some variation,
		or roughness, to the surface.

		Note: this noise is applied only to the graphics of the cutout chunks.  The chunks' collision volumes AND the chunks'
		children (if fractured further) will NOT be affected by this noise.
	*/
	NoiseParameters edgeNoise;

	/** Constructor sets defaults */
	CutoutParameters()
	{
		setToDefault();
	}

	/**
		Set default values:

		depth = 1.0f;
		backfaceNoise.setToDefault();
		edgeNoise.setToDefault();
		materialDesc.setToDefault();
	*/
	void	setToDefault()
	{
		depth = 1.0f;
		backfaceNoise.setToDefault();
		materialDesc.setToDefault();
		edgeNoise.setToDefault();
	}
};


/**
	FractureCutoutDesc

	Descriptor for cutout-mode (chippable) fracturing.
*/
struct FractureCutoutDesc
{
	/** Enum describing the directions to apply cutout fracturing. */
	enum Directions
	{
		UserDefined = 0,	// If no flags are set, the cutout direction is taken from userDefinedDirection

		NegativeX =	1 << 0,
		PositiveX =	1 << 1,
		NegativeY =	1 << 2,
		PositiveY =	1 << 3,
		NegativeZ =	1 << 4,
		PositiveZ =	1 << 5,

		DirectionCount = 6
	};

	/** The directions to apply cutout fracturing.  (See the Directions enum.) */
	unsigned directions;

	/**
		The order in which to apply each cutout direction.
		The direction in directionOrder[0] is applied first, in directionOrder[1], second, and so on.
	*/
	unsigned directionOrder[DirectionCount];

	/** Cutout parameters used for the various pre-defined cutout directions. */
	CutoutParameters cutoutParameters[DirectionCount];

	/**
		The cutout direction if directions = 0.  When this is used, it must be have non-zero length (it will be
		normalized internally), and userUVMapping must be set (this may be done with AssetAuthoring::calculateCutoutUVMapping).
	*/
	physx::PxVec3 userDefinedDirection;

	/** The UV mapping used if directons = 0 (along with userDefinedDirection). */
	physx::PxMat33 userUVMapping;

	/** Cutout parameters used if user-defined (UV-based) cutout fracturing is selected by setting directions = 0 */
	CutoutParameters userDefinedCutoutParameters;

	/** Enum describing the instancing mode. */
	enum InstancingMode
	{
		DoNotInstance,
		InstanceCongruentChunks,
		InstanceAllChunks,

		InstanceModeCount
	};

	/**
		The instancing mode.
	*/
	unsigned instancingMode;

	/**
		If tileFractureMap is true, the map will be tiled across the destructible.
	*/
	bool tileFractureMap;

	/**
		The U,V width of the fracture map when instancing chunks
	*/
	physx::PxVec2 uvTileSize;

	/**
		If true, non-convex cutouts will be split into convex ones.
	*/
	bool splitNonconvexRegions;

	/**
		Fracturing to apply to cutout chunks (if any), to break them down further.
		Current options include none, slice fracturing, and voronoi fracturing.
	*/
	enum CutoutChunkFracturingMethod
	{
		DoNotFractureCutoutChunks,
		SliceFractureCutoutChunks,
		VoronoiFractureCutoutChunks,

		CutoutChunkFracturingMethodCount
	};

	/**
		If true, slice-mode fracturing will be applied to each cutout piece.
		The cutout function must be provided with a FractureSliceDesc as well to describe
		the slice parameters.  These parameters, however, must be interpreted from the
		point of view of the cutout direction.  That is, X and Y slice parameters will be
		used to slice along the cutout faces.  The Z slice parameters will be used to slice
		into the cutout faces.
	*/
	unsigned chunkFracturingMethod;

	/**
		If true, the backface and cutouts will be trimmed if (a) backface noise is non-zero or
		(b) the collision hull is something other than the mesh convex hull ("Wrap Graphics Mesh").
		Trimming is done by intersecting the face slice plane (without added noise) with the backface
		and cutouts.
		Default is true.
	*/
	bool trimFaceCollisionHulls;

	/** Scale to apply to the X coordinates of the cutout set (along the various cutout directions). */
	float cutoutWidthScale[DirectionCount];

	/** Scale to apply to the Y coordinates of the cutout set (along the various cutout directions). */
	float cutoutHeightScale[DirectionCount];

	/** Offset to apply to the X coordinates of the cutout set (along the various cutout directions). */
	float cutoutWidthOffset[DirectionCount];

	/** Offset to apply to the Y coordinates of the cutout set (along the various cutout directions). */
	float cutoutHeightOffset[DirectionCount];

	/** If true, the cutout map will be flipped in the X direction (along the various cutout directions). */
	bool cutoutWidthInvert[DirectionCount];

	/** If true, the cutout map will be flipped in the Y direction (along the various cutout directions). */
	bool cutoutHeightInvert[DirectionCount];

	/** The interpreted size of the cutout map in the X direction */
	float cutoutSizeX;

	/** The interpreted size of the cutout map in the Y direction */
	float cutoutSizeY;

	/**
		Threshold angle to merge (smoothe) vertex normals around cutout, in degrees.
		If the exterior angle between two facets of a cutout region no more than this, the vertex normals and tangents will be
		averaged at the facet interface.  A value of 0 effectively disables smoothing.
		Default value = 60 degrees.
	*/
	float facetNormalMergeThresholdAngle;

	/** Constructor sets defaults */
	FractureCutoutDesc()
	{
		setToDefault();
	}

	/**
		Set default values:

		directions = 0;
		directionOrder[0..5] = {NegativeX, .., PositiveZ};
		cutoutParameters[0..5].setToDefault();
		userDefinedDirection= physx::PxVec3(0.0f);
		userUVMapping		= physx::PxMat33(physx::PxIdentity);
		userDefinedCutoutParameters.setToDefault();
		instancingMode = DoNotInstance;
		tileFractureMap = false;
		uvTileSize = (0.0f,0.0f)
		cutoutParameters[0..5].setToDefault();
		cutoutWidthScale[0..5] = 1.0f;
		cutoutHeightScale[0..5] = 1.0f;
		cutoutWidthOffset[0..5] = 0.0f;
		cutoutHeightOffset[0..5] = 0.0f;
		cutoutWidthInvert[0..5] = false;
		cutoutHeightInvert[0..5] = false;
		cutoutSizeX = 1.0f;
		cutoutSizeY = 1.0f;
		facetNormalMergeThresholdAngle = 60.0f;
		splitNonconvexRegions = false;
		chunkFracturingMethod = DoNotFractureCutoutChunks;
		trimFaceCollisionHulls = true;
	*/
	void	setToDefault()
	{
		directions          = 0;
		userDefinedDirection= physx::PxVec3(0.0f);
		userUVMapping		= physx::PxMat33(physx::PxIdentity);
		userDefinedCutoutParameters.setToDefault();
		instancingMode      = DoNotInstance;
		tileFractureMap     = false;
		uvTileSize          = physx::PxVec2(0.0f);
		for (uint32_t i = 0; i < DirectionCount; ++i)
		{
			directionOrder[i]     = 1u << i;
			cutoutParameters[i].setToDefault();
			cutoutWidthScale[i]   = 1.0f;
			cutoutHeightScale[i]  = 1.0f;
			cutoutWidthOffset[i]  = 0.0f;
			cutoutHeightOffset[i] = 0.0f;
			cutoutWidthInvert[i]  = false;
			cutoutHeightInvert[i] = false;
		}
		cutoutSizeX                    = 1.0f;
		cutoutSizeY                    = 1.0f;
		facetNormalMergeThresholdAngle = 60.0f;
		splitNonconvexRegions          = false;
		chunkFracturingMethod          = DoNotFractureCutoutChunks;
		trimFaceCollisionHulls         = true;
	}
};


/**
	FractureVoronoiDesc

	Descriptor for Voronoi decomposition fracturing.
*/
struct FractureVoronoiDesc
{
	/**
		Number of cell sites in the sites array.  Must be positive.
	*/
	unsigned				siteCount;

	/**
		Array of cell sites.  The length of this array is given by siteCount.
	*/
	const physx::PxVec3*	sites;

	/**
		Array of chunk indices to associate with each site.  If this pointer is NULL, then all sites will be used to split
		all chunks relevant to the splitting operation.  Otherwise, the chunkIndices array must be of length siteCount.
	*/
	const uint32_t*			chunkIndices;

	/**
		Describes the characteristics of the interior surfaces.
		If the noise is 0, the cutting surface will smooth.  Otherwise, there will be some variation, or roughness, to the surface.

		Note: this noise is applied only to the graphics of the chunks.  The chunks' collision volumes AND the chunks'
		children (if fractured further) will NOT be affected by this noise.
	*/
	NoiseParameters			faceNoise;

	/**
		If instanceChunks is true, corresponding chunks in different destructible actors will be instanced.
	*/
	bool					instanceChunks;

	/**
		If true, slice geometry and noise will be stored in a separate displacement offset buffer.
	*/
	bool					useDisplacementMaps;

	/** Enum describing creation of displacement maps. */
	enum NoiseMode
	{
		NoiseWavePlane = 0,
		NoisePerlin2D,
		NoisePerlin3D,

		NoiseModeCount
	};

	/**
		The noise mode.  If displacement maps are enabled, NoisePerlin3D will be used.
	*/
	unsigned				noiseMode;

	/**
		Smallest size allowed for chunks, as measured by their bounding sphere diameters.
		The sizes are interpreted as fractions of the unfractured mesh's bounding sphere diameter.
		Chunks created by fracturing which are smaller than this limit will be removed.  Note - this will leave holes at the given depth.
		Default = 0, which disables this feature.
	*/
	float					minimumChunkSize;

	/**
		Material application descriptor used for each slice.
		Note: the U-direction and UV offset in the descriptor will be ignored - UV mapping is done in arbitrary orientation and translation on each chunk face.
	*/
	nvidia::FractureMaterialDesc	materialDesc;


	/** Constructor sets defaults */
	FractureVoronoiDesc()
	{
		setToDefault();
	}

	/**
		Sets the default values:

		siteCount = 0;
		sites = NULL;
		chunkIndices = NULL;
		faceNoise.setToDefault();
		instanceChunks = false;
		useDisplacementMaps = false;
		noiseMode = NoiseWavePlane;
		minimumChunkSize = 0.0f;
		materialDesc.setToDefault();
	*/
	void	setToDefault()
	{
		siteCount = 0;
		sites = NULL;
		chunkIndices = NULL;
		faceNoise.setToDefault();
		instanceChunks = false;
		useDisplacementMaps = false;
		noiseMode = NoiseWavePlane;
		minimumChunkSize = 0.0f;
		materialDesc.setToDefault();
	}
};


} // namespace FractureTools


PX_POP_PACK

#endif // FRACTURE_TOOLS_H
