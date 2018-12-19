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



#ifndef EXPLICIT_HIERARCHICAL_MESH_H
#define EXPLICIT_HIERARCHICAL_MESH_H

#include "foundation/Px.h"
#include "IProgressListener.h"
#include "RenderMeshAsset.h"
#include "ConvexHullMethod.h"
#include "foundation/PxPlane.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
	ExplicitVertexFormat

	This is used when authoring an VertexBuffer, to define which data channels exist.
 */
struct ExplicitVertexFormat
{
	/** This value defines which vertex winding orders will be rendered.  See RenderCullMode. */
	uint32_t			mWinding;

	/** Whether or not the accompanying vertex data has defined static vertex positions. */
	bool				mHasStaticPositions;

	/** Whether or not the accompanying vertex data has defined static vertex normals. */
	bool				mHasStaticNormals;

	/** Whether or not the accompanying vertex data has defined static vertex tangents. */
	bool				mHasStaticTangents;

	/** Whether or not the accompanying vertex data has defined static vertex binormals. */
	bool				mHasStaticBinormals;

	/** Whether or not the accompanying vertex data has defined static vertex colors. */
	bool				mHasStaticColors;

	/** Whether or not to create separate render resource for a static bone index buffer. */
	bool				mHasStaticSeparateBoneBuffer;

	/** Whether or not the accompanying vertex data has defined dynamic displacement coordinates */
	bool				mHasStaticDisplacements;

	/** Whether or not the accompanying vertex data has defined dynamic vertex positions. */
	bool				mHasDynamicPositions;

	/** Whether or not the accompanying vertex data has defined dynamic vertex normals. */
	bool				mHasDynamicNormals;

	/** Whether or not the accompanying vertex data has defined dynamic vertex tangents. */
	bool				mHasDynamicTangents;

	/** Whether or not the accompanying vertex data has defined dynamic vertex binormals. */
	bool				mHasDynamicBinormals;

	/** Whether or not the accompanying vertex data has defined dynamic vertex colors. */
	bool				mHasDynamicColors;

	/** Whether or not to create separate render resource for a dynamic bone index buffer. */
	bool				mHasDynamicSeparateBoneBuffer;

	/** Whether or not the accompanying vertex data has defined dynamic displacement coordinates */
	bool				mHasDynamicDisplacements;

	/** How many UV coordinate channels there are (per vertex) */
	uint32_t			mUVCount;

	/** How many bones may influence a vertex */
	uint32_t			mBonesPerVertex;

	/** Constructor, calls clear() to set formats to default settings */
	ExplicitVertexFormat()
	{
		clear();
	}

	/**
		Set formats to default settings:

		mWinding = RenderCullMode::CLOCKWISE;
		mHasStaticPositions = false;
		mHasStaticNormals = false;
		mHasStaticTangents = false;
		mHasStaticBinormals = false;
		mHasStaticColors = false;
		mHasStaticSeparateBoneBuffer = false;
		mHasStaticDisplacements = false;
		mHasDynamicPositions = false;
		mHasDynamicNormals = false;
		mHasDynamicTangents = false;
		mHasDynamicBinormals = false;
		mHasDynamicColors = false;
		mHasDynamicSeparateBoneBuffer = false;
		mHasDynamicDisplacements = false;
		mUVCount = 0;
		mBonesPerVertex = 0;
	*/
	void	clear()
	{
		mWinding = RenderCullMode::CLOCKWISE;
		mHasStaticPositions = false;
		mHasStaticNormals = false;
		mHasStaticTangents = false;
		mHasStaticBinormals = false;
		mHasStaticColors = false;
		mHasStaticSeparateBoneBuffer = false;
		mHasStaticDisplacements = false;
		mHasDynamicPositions = false;
		mHasDynamicNormals = false;
		mHasDynamicTangents = false;
		mHasDynamicBinormals = false;
		mHasDynamicColors = false;
		mHasDynamicSeparateBoneBuffer = false;
		mHasDynamicDisplacements = false;
		mUVCount = 0;
		mBonesPerVertex = 0;
	}

	/**
		Equality operator.  All values are tested for equality except mBonesPerVertex.
	*/
	bool	operator == (const ExplicitVertexFormat& data) const
	{
		if (mWinding != data.mWinding)
		{
			return false;
		}
		if (mHasStaticPositions != data.mHasStaticPositions ||
		        mHasStaticNormals != data.mHasStaticNormals ||
		        mHasStaticTangents != data.mHasStaticTangents ||
		        mHasStaticBinormals != data.mHasStaticBinormals ||
		        mHasStaticColors != data.mHasStaticColors ||
		        mHasStaticSeparateBoneBuffer != data.mHasStaticSeparateBoneBuffer ||
		        mHasStaticDisplacements != data.mHasStaticDisplacements)
		{
			return false;
		}
		if (mHasDynamicPositions != data.mHasDynamicPositions ||
		        mHasDynamicNormals != data.mHasDynamicNormals ||
		        mHasDynamicTangents != data.mHasDynamicTangents ||
		        mHasDynamicBinormals != data.mHasDynamicBinormals ||
		        mHasDynamicColors != data.mHasDynamicColors ||
		        mHasDynamicSeparateBoneBuffer != data.mHasDynamicSeparateBoneBuffer ||
		        mHasDynamicDisplacements != data.mHasDynamicDisplacements)
		{
			return false;
		}
		if (mUVCount != data.mUVCount)
		{
			return false;
		}
		return true;
	}

	/**
		Returns the logical complement of the == operator.
	*/
	bool	operator != (const ExplicitVertexFormat& data) const
	{
		return !(*this == data);
	}

	/**
		Creates a render-ready VertexFormat corresponding to this structure's member values.
	*/
	void	copyToVertexFormat(VertexFormat* format) const
	{
		format->reset();
		uint32_t bi;
		if (mHasStaticPositions)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::POSITION));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			format->setBufferAccess(bi, mHasDynamicPositions ? RenderDataAccess::DYNAMIC :  RenderDataAccess::STATIC);
		}
		if (mHasStaticNormals)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::NORMAL));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			format->setBufferAccess(bi, mHasDynamicNormals ? RenderDataAccess::DYNAMIC :  RenderDataAccess::STATIC);
		}
		if (mHasStaticTangents)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::TANGENT));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			format->setBufferAccess(bi, mHasDynamicTangents ? RenderDataAccess::DYNAMIC :  RenderDataAccess::STATIC);
		}
		if (mHasStaticBinormals)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BINORMAL));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			format->setBufferAccess(bi, mHasDynamicBinormals ? RenderDataAccess::DYNAMIC :  RenderDataAccess::STATIC);
		}
		if (mHasStaticDisplacements)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::DISPLACEMENT_TEXCOORD));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			format->setBufferAccess(bi, mHasDynamicDisplacements ? RenderDataAccess::DYNAMIC : RenderDataAccess::STATIC);
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::DISPLACEMENT_FLAGS));
			format->setBufferFormat(bi, RenderDataFormat::UINT1);
			format->setBufferAccess(bi, mHasDynamicDisplacements ? RenderDataAccess::DYNAMIC : RenderDataAccess::STATIC);
		}
		if (mUVCount > 0)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::TEXCOORD0));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT2);
		}
		if (mUVCount > 1)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::TEXCOORD1));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT2);
		}
		if (mUVCount > 2)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::TEXCOORD2));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT2);
		}
		if (mUVCount > 3)
		{
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::TEXCOORD3));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT2);
		}
		switch (mBonesPerVertex)
		{
		case 1:
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_INDEX));
			format->setBufferFormat(bi, RenderDataFormat::USHORT1);
			break;
		case 2:
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_INDEX));
			format->setBufferFormat(bi, RenderDataFormat::USHORT2);
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_WEIGHT));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT2);
			break;
		case 3:
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_INDEX));
			format->setBufferFormat(bi, RenderDataFormat::USHORT3);
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_WEIGHT));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT3);
			break;
		case 4:
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_INDEX));
			format->setBufferFormat(bi, RenderDataFormat::USHORT4);
			bi = (uint32_t)format->addBuffer(format->getSemanticName(RenderVertexSemantic::BONE_WEIGHT));
			format->setBufferFormat(bi, RenderDataFormat::FLOAT4);
			break;
		}

		format->setHasSeparateBoneBuffer(mHasStaticSeparateBoneBuffer);
		format->setWinding((RenderCullMode::Enum)mWinding);
	}
};


/**
	ExplicitSubmeshData

	This is used when authoring an RenderMeshAsset.

	This is the per-submesh data: the material name, and vertex format.
 */
struct ExplicitSubmeshData
{
	/**
		Enum for storing material name buffer size
	*/
	enum
	{
		MaterialNameBufferSize = 1024
	};

	/**
		Material name char buffer
	*/
	char					mMaterialName[MaterialNameBufferSize];
	
	/**
		Explicit vertex format
		\see ExplicitVertexFormat
	*/
	ExplicitVertexFormat	mVertexFormat; 

	/**
		Equal operator for ExplicitSubmeshData
	*/
	bool	operator == (const ExplicitSubmeshData& data) const
	{
		return !::strcmp(mMaterialName, data.mMaterialName) && mVertexFormat == data.mVertexFormat;
	}

	/**
		Not equal operator for ExplicitSubmeshData
	*/
	bool	operator != (const ExplicitSubmeshData& data) const
	{
		return !(*this == data);
	}
};


/**
	Collision volume descriptor for a chunk
*/
struct CollisionVolumeDesc
{
	CollisionVolumeDesc()
	{
		setToDefault();
	}

	/**
		Set CollisionVolumeDesc to default values
	*/
	void setToDefault()
	{
		mHullMethod = ConvexHullMethod::CONVEX_DECOMPOSITION;
		mConcavityPercent = 4.0f;
		mMergeThreshold = 4.0f;
		mRecursionDepth = 0;
		mMaxVertexCount = 0;
		mMaxEdgeCount = 0;
		mMaxFaceCount = 0;
	}

	/**
		How to generate convex hulls for a chunk.  See ConvexHullMethod::Enum.
		Default = CONVEX_DECOMPOSITION.
	*/
	ConvexHullMethod::Enum	mHullMethod;

	/**
		Allowed concavity if mHullMethod = ConvexHullMethod::CONVEX_DECOMPOSITION.
		Default = 4.0.
	*/
	float					mConcavityPercent;

	/**
		Merge threshold if mHullMethod = ConvexHullMethod::CONVEX_DECOMPOSITION.
		Default = 4.0.
	*/
	float					mMergeThreshold;
	
	/**
		Recursion depth if mHullMethod = ConvexHullMethod::CONVEX_DECOMPOSITION.
		Depth = 0 generates a single convex hull.  Higher recursion depths may generate
		more convex hulls to fit the mesh.
		Default = 0.
	*/
	uint32_t				mRecursionDepth;

	/**
		The maximum number of vertices each hull may have.  If 0, there is no limit.
		Default = 0.
	*/
	uint32_t				mMaxVertexCount;

	/**
		The maximum number of edges each hull may have.  If 0, there is no limit.
		Default = 0.
	*/
	uint32_t				mMaxEdgeCount;

	/**
		The maximum number of faces each hull may have.  If 0, there is no limit.
		Default = 0.
	*/
	uint32_t				mMaxFaceCount;
};


/**
	Collision descriptor
*/
struct CollisionDesc
{
	/**
		How many collision volume descriptors are in the mVolumeDescs array.

		This count need not match the depth count of the destructible to be created.  If it is greater than the depth count of the destructible,
		the extra volume descriptors will be ignored.  If it is less than the depth count of the destructible, then mVolumeDescs[depthCount-1] will
		be used unless depthCount is zero.  In that case, the defeult CollisionVolumeDesc() will be used.

		This may be zero, in which case all volume descriptors will be the default values.  If it is not zero, mVolumeDescs must be a valid pointer.
	*/
	unsigned				mDepthCount;

	/**
		Array of volume descriptors of length depthCount (may be NULL if depthCount is zero).
	*/
	CollisionVolumeDesc*	mVolumeDescs;

	/**
		The maximum amount to trim overlapping collision hulls (as a percentage of the hulls' widths)

		Default = 0.2f
	*/
	float					mMaximumTrimming;

	/** Constructor sets default values. */
	CollisionDesc()
	{
		setToDefault();
	}

	/**
		Set CollisionDesc to default values
	*/
	void setToDefault()
	{
		mDepthCount = 0;
		mVolumeDescs = NULL;
		mMaximumTrimming = 0.2f;
	}
};


/**
	Enumeration of current fracture methods.  Used when an authored mesh needs to know how it was created, for
	example if we need to re-apply UV mapping information.
*/
struct FractureMethod
{
	/**
		Enum of fracture methods
	*/
	enum Enum
	{
		Unknown,
		Slice,
		Cutout,
		Voronoi,

		FractureMethodCount
	};
};


/**
	FractureMaterialDesc

	Descriptor for materials applied to interior faces.
*/
struct FractureMaterialDesc
{
	/** The UV scale (geometric distance/unit texture distance) for interior materials.
	Default = (1.0f,1.0f).
	*/
	PxVec2	uvScale;

	/** A UV origin offset for interior materials.
	Default = (0.0f,0.0f).
	*/
	PxVec2	uvOffset;

	/** World space vector specifying surface tangent direction.  If this vector
	is (0.0f,0.0f,0.0f), then an arbitrary direction will be chosen.
	Default = (0.0f,0.0f,0.0f).
	*/
	PxVec3	tangent;

	/** Angle from tangent direction for the u coordinate axis.
	Default = 0.0f.
	*/
	float	uAngle;

	/**
		The submesh index to use for the newly-created triangles.
		If an invalid index is given, 0 will be used.
	*/
	uint32_t interiorSubmeshIndex;

	/** Constructor sets defaults */
	FractureMaterialDesc()
	{
		setToDefault();
	}

	/**
		Set default values:
		uvScale = PxVec2(1.0f);
		uvOffset = PxVec2(0.0f);
		tangent = PxVec3(0.0f);
		uAngle = 0.0f;
		interiorSubmeshIndex = 0;
	*/
	void	setToDefault()
	{
		uvScale = PxVec2(1.0f);
		uvOffset = PxVec2(0.0f);
		tangent = PxVec3(0.0f);
		uAngle = 0.0f;
		interiorSubmeshIndex = 0;
	}
};


/**
	A reference frame for applying UV mapping to triangles.  Also includes the fracturing method and an index
	which is used internally for such operations as re-applying UV mapping information.
*/
struct MaterialFrame
{
	MaterialFrame() :
		mCoordinateSystem(PxVec4(1.0f)),
		mUVPlane(PxVec3(0.0f, 0.0f, 1.0f), 0.0f),
		mUVScale(1.0f),
		mUVOffset(0.0f),
		mFractureMethod(FractureMethod::Unknown),
		mFractureIndex(-1),
		mSliceDepth(0)
	{		
	}

	/**
		Builds coordinate system from material desc
	*/
	void	buildCoordinateSystemFromMaterialDesc(const nvidia::FractureMaterialDesc& materialDesc, const PxPlane& plane)
			{
				PxVec3 zAxis = plane.n;
				zAxis.normalize();
				PxVec3 xAxis = materialDesc.tangent;
				PxVec3 yAxis = zAxis.cross(xAxis);
				const float l2 = yAxis.magnitudeSquared();
				if (l2 > PX_EPS_F32*PX_EPS_F32)
				{
					yAxis *= PxRecipSqrt(l2);
				}
				else
				{
					uint32_t maxDir =  PxAbs(plane.n.x) > PxAbs(plane.n.y) ?
						(PxAbs(plane.n.x) > PxAbs(plane.n.z) ? 0u : 2u) :
						(PxAbs(plane.n.y) > PxAbs(plane.n.z) ? 1u : 2u);
					xAxis = PxMat33(PxIdentity)[(maxDir + 1) % 3];
					yAxis = zAxis.cross(xAxis);
					yAxis.normalize();
				}
				xAxis = yAxis.cross(zAxis);

				const float c = PxCos(materialDesc.uAngle);
				const float s = PxSin(materialDesc.uAngle);

				mCoordinateSystem.column0 = PxVec4(c*xAxis + s*yAxis, 0.0f);
				mCoordinateSystem.column1 = PxVec4(c*yAxis - s*xAxis, 0.0f);
				mCoordinateSystem.column2 = PxVec4(zAxis, 0.0f);
				mCoordinateSystem.setPosition(plane.project(PxVec3(0.0f)));

				mUVPlane = plane;
				mUVScale = materialDesc.uvScale;
				mUVOffset = materialDesc.uvOffset;
			}

	PxMat44	mCoordinateSystem; ///< Coordinate system
	PxPlane	mUVPlane; ///< UV plane
	PxVec2	mUVScale; ///< UV scale
	PxVec2	mUVOffset; ///< UV offset
	uint32_t	mFractureMethod; ///< Fracture method
	int32_t	mFractureIndex; ///< Fracture index
	uint32_t	mSliceDepth;	///< The depth being created when this split is done.  mSliceDepth = 0 means "unknown"
};


/**
	Interface to a "displacement map volume," used with tessellated fracturing.
	A displacement map volume captures how to displace a particular point in 3D space
	along the x, y and z axes.  The data is stored as a 3D texture volume, with
	corresponding displacement coordinates acting as a look-up into this volume.
	X, Y and Z offsets correspond to R, G, and B color channels

	Various approaches can be used to generate the 3D noise field, in this case
	Perlin noise is used, with appropriate settings specified by the FractureSliceDesc.
*/
class DisplacementMapVolume
{
public:
	/** 
		Fills the specified array and parameters with texture-compatible information. 

		The corresponding texture aligns with the displacement UVs generated as fracturing occurs
		when displacement maps are enabled, with RGB data corresponding to XYZ offsets, respectively.
	*/
	virtual void getData(uint32_t& width, uint32_t& height, uint32_t& depth, uint32_t& size, unsigned char const** ppData) const = 0;

	virtual ~DisplacementMapVolume() { }
};


/**
	Handling of open meshes.

	BSPMode::Automatic causes the mesh to be analyzed to determine if it's open or closed
	BSPOpenMode::Closed means the mesh should be closed and interior faces are created when the mesh is split
	BSPOpenMode::Open means the mesh should be open and no interior faces are created when it is split
*/
struct BSPOpenMode
{
	/**
		Enum of BSP open modes
	*/
	enum Enum
	{
		Automatic,
		Closed,
		Open,

		BSPOpenModeCount
	};
};


/**
	ExplicitHierarchicalMesh

	An interface to a representation of a render mesh that is used for authoring.

	The "hierarchical" nature of this mesh is represented by extra parent/child relations
	among the parts that will become the parts of an RenderMeshAsset.
 */
class ExplicitHierarchicalMesh
{
public:
	/** Used in the visualize method to determine what to draw. */
	enum Enum
	{
		/**
			Draws the convex hulls associated with the BSP regions marked "outside," if a BSP has
			been created for this object (see calculateMeshBSP()).
		*/
		VisualizeMeshBSPOutsideRegions	= (1 << 0),

		/**
			Draws the convex hulls associated with the BSP regions marked "inside," if a BSP has
			been created for this object (see calculateMeshBSP()).
		*/
		VisualizeMeshBSPInsideRegions	= (1 << 1),

		/**
			Draws the convex hull associated with a single BSP regions, if a BSP has
			been created for this object (see calculateMeshBSP()).  The region index must
			be passed into the visualize function if this flag is set.
		*/
		VisualizeMeshBSPSingleRegion	= (1 << 8),

		/** Currently unused */
		VisualizeSliceBSPOutsideRegions	= (1 << 16),

		/** Currently unused */
		VisualizeSliceBSPInsideRegions	= (1 << 17),

		/** Currently unused */
		VisualizeSliceBSPSingleRegion	= (1 << 24),

		VisualizeMeshBSPAllRegions		= VisualizeMeshBSPOutsideRegions | VisualizeMeshBSPInsideRegions
	};

	/**
		Used in the serialize and deserialize methods, to embed extra data.
		The user must implement this class to serialize and deserialize
		the enumerated data type given.
	*/
	class Embedding
	{
	public:
		/**
			Enum of data types
		*/
		enum DataType
		{
			MaterialLibrary
		};
		
		/**
			Serialize the enumerated data type
		*/
		virtual void	serialize(PxFileBuf& stream, Embedding::DataType type) const = 0;
		
		/**
			Deserialize the enumerated data type
		*/
		virtual void	deserialize(PxFileBuf& stream, Embedding::DataType type, uint32_t version) = 0;
	};

	/**
		Used to access the collision data for each mesh part
	*/
	class ConvexHull
	{
	protected:
		ConvexHull()
		{
		}

		virtual ~ConvexHull()
		{
		}

	public:
		/**
			Builds the convex hull of the points given.
		*/
		virtual void					buildFromPoints(const void* points, uint32_t numPoints, uint32_t pointStrideBytes) = 0;

		/**
			The hull's axis aligned bounding box.
		*/
		virtual const PxBounds3&		getBounds() const = 0;

		/**
			The hull's volume.
		*/
		virtual float					getVolume() const = 0;

		/**
			This is the number of vertices in the convex hull.
		*/
		virtual uint32_t				getVertexCount() const = 0;

		/**
			This is the vertex indexed by vertexIndex, which must in
			the range [0, getVertexCount()-1].
		*/
		virtual PxVec3					getVertex(uint32_t vertexIndex) const = 0;

		/**
			This is the number of edges in the convex hull.
		*/
		virtual uint32_t				getEdgeCount() const = 0;

		/**
			This is an edge endpoint indexed by edgeIndex, which must in
			the range [0, getEdgeCount()-1], and
			whichEndpoint, which must be 0 or 1.
		*/
		virtual PxVec3					getEdgeEndpoint(uint32_t edgeIndex, uint32_t whichEndpoint) const = 0;

		/**
			This is the number of planes which bound the convex hull.
		*/
		virtual uint32_t				getPlaneCount() const = 0;

		/**
			This is the plane indexed by planeIndex, which must in
			the range [0, getPlaneCount()-1].
		*/
		virtual PxPlane					getPlane(uint32_t planeIndex) const = 0;

		/**
			Perform a ray cast against the convex hull.

			\param in				this MUST be set to the minimum 'time' that you wish to have reported for intersection.
										you may consider this an origin offset for the ray.
										On exit, if the hull is intersected, this value will contain the time of intersection,
										or its original value, which ever is larger.

			\param out				this MUST be set to the maximum 'time' that you wish to have reported for intersection.
										you may consider this the endpoint of a line segment intersection.
										On exit, if the hull is intersected, this value will contain the time that the ray
										exits the hull, or its original value, which ever is smaller.

			\param orig				describe the ray to intersect with the convex hull.
			\param dir				describe the ray to intersect with the convex hull.

			\param localToWorldRT	the rotation applied to the convex hull.

			\param scale			the scale applied to the convex hull.

			\param normal			if not NULL, *normal will contain the surface normal of the convex hull at the
										point of intersection (at the 'in' time).  If the point on the ray at the 'in' time lies
										within the volume of the convex hull, then *normal will be set to (0,0,0).

			\return					returns true if the line segment described by the user's supplied 'in' and 'out'
										parameters along the ray intersects the convex hull, false otherwise.
		*/
		virtual bool					rayCast(float& in, float& out, const PxVec3& orig, const PxVec3& dir,
		                                        const PxTransform& localToWorldRT, const PxVec3& scale, PxVec3* normal = NULL) const = 0;
		/**
			Removes vertices from the hull until the bounds given in the function's parameters are met.
			If inflated = true, then the maximum counts given are compared with the cooked hull, which may have higher counts due to beveling.
			
			\note		a value of zero indicates no limit, effectively infinite.
			
			\return		true if successful, i.e. the limits were met.  False otherwise.
		*/
		virtual bool					reduceHull(uint32_t maxVertexCount, uint32_t maxEdgeCount, uint32_t maxFaceCount, bool inflated) = 0;

		/**
			Releases all memory associated with this object and deletes itself.
		*/
		virtual void					release() = 0;
	};

	/**
		"Resets" this object to its initial state, freeing all internal data.
		If keepRoot is true, then parts up to the root depth will not be cleared.
		(In this case, not all of the submesh data, etc. will be deleted.)
		The root depth is set when the ExplicitHierarchicalMesh is first created.
		Fracturing methods create pieces beyond the root depth.
	*/
	virtual void						clear(bool keepRoot = false) = 0;

	/**
		The maximum child depth in the hierarchy.  Depth 0 is the base, depth 1 parts are children of depth 0, etc.
		If there are no parts, this function returns -1.
	*/
	virtual int32_t						maxDepth() const = 0;

	/**
		The number of parts in this mesh.
	*/
	virtual uint32_t					partCount() const = 0;

	/**
		The number of chunks in this mesh.
	*/
	virtual uint32_t					chunkCount() const = 0;

	/**
		The parent index of the chunk indexed by chunkIndex.
		Depth 0 parts have no parents, and for those parts this function returns -1.
	*/
	virtual int32_t*					parentIndex(uint32_t chunkIndex) = 0;

	/**
		A runtime unique identifier for a chunk.  During one execution of an application which
		contains the fracture tools, this chunk ID will be unique for the chunk.
	*/
	virtual uint64_t					chunkUniqueID(uint32_t chunkIndex) = 0;

	/**
		The geometric part index this chunk references
	*/
	virtual int32_t*					partIndex(uint32_t chunkIndex) = 0;

	/**
		If instanced, the part instance offset (translation).
	*/
	virtual PxVec3*						instancedPositionOffset(uint32_t chunkIndex) = 0;

	/**
		If instanced, the part instance offset (UV).
	*/
	virtual PxVec2*						instancedUVOffset(uint32_t chunkIndex) = 0;

	/**
		The number of triangles in the part indexed by partIndex.
		This includes all submeshes.
	*/
	virtual uint32_t					meshTriangleCount(uint32_t partIndex) const = 0;

	/**
		A pointer into the array of ExplicitRenderTriangles which form the mesh
		of the part indexed by partIndex.
	*/
	virtual ExplicitRenderTriangle*		meshTriangles(uint32_t partIndex) = 0;

	/**
		The axis aligned bounding box of the triangles for the part index by partIndex.
	*/
	virtual PxBounds3					meshBounds(uint32_t partIndex) const = 0;

	/**
		The axis aligned bounding box of the triangles for the chunk index by chunkIndex.
	*/
	virtual PxBounds3					chunkBounds(uint32_t chunkIndex) const = 0;

	/**
		Flags describing attributes of the part indexed by partIndex.
		See DestructibleAsset::ChunkFlags
	*/
	virtual uint32_t*					chunkFlags(uint32_t chunkIndex) const = 0;

	/**
		Build collision volumes for the part indexed by partIndex, using (See CollisionVolumeDesc.)
	*/
	virtual void						buildCollisionGeometryForPart(uint32_t partIndex, const CollisionVolumeDesc& desc) = 0;

	/**
		Build collision volumes for all parts referenced by chunks at the root depth.

		If aggregateRootChunkParentCollision, then every chunk which is the parent of root chunks
		gets all of the collision hulls of its children.  Otherwise, all root chunks have their
		collision volumes separately calculated.
	*/
	virtual void						buildCollisionGeometryForRootChunkParts(const CollisionDesc& desc, bool aggregateRootChunkParentCollision = true) = 0;

	/**
		Calls IConvexMesh::reduceHull on all part convex hulls.  See IConvexMesh::reduceHull.
	*/
	virtual void						reduceHulls(const CollisionDesc& desc, bool inflated) = 0;

	/**
		The number of convex hulls for the given part.
	*/
	virtual uint32_t					convexHullCount(uint32_t partIndex) const = 0;

	/**
		The convex hulls for the given part.
	*/
	virtual const ConvexHull**			convexHulls(uint32_t partIndex) const = 0;

	/**
		The outward surface normal associated with the chunk mesh geometry.
	*/
	virtual PxVec3*						surfaceNormal(uint32_t partIndex) = 0;

	/**
		The displacement map volume for the mesh.
	*/
	virtual const DisplacementMapVolume&	displacementMapVolume() const = 0;

	/**
		The number of submeshes.  The explicit mesh representation is just a list
		of ExplicitRenderTriangles for each part, and each ExplicitRenderTriangle has
		a submesh index.  These indices will lie in a contiguous range from 0 to submeshCount()-1.
	*/
	virtual uint32_t					submeshCount() const = 0;

	/**
		The submeshData indexed by submeshIndex.  See ExplicitSubmeshData.
	*/
	virtual ExplicitSubmeshData*		submeshData(uint32_t submeshIndex) = 0;

	/**
		Creates a submesh and adds it to the list of submeshes, and returns the index of
		the newly created submesh.
	*/
	virtual uint32_t					addSubmesh(const ExplicitSubmeshData& submeshData) = 0;

	/**
		If there are interior submeshes, then triangles belonging
		to those submesh will have materials applied to them with a using a coordinate frame.
		In the event that materials need to be re-applied (possibly at a different scale), it
		is convenient to store the material frames used.  This function returns the array of
		material frames.  The index extraDataIndex stored in each ExplicitRenderTriangle
		references this array.
	*/
	virtual uint32_t					getMaterialFrameCount() const = 0;
	virtual nvidia::MaterialFrame		getMaterialFrame(uint32_t index) const = 0; ///< \see ConvexHull::getMaterialFrameCount
	virtual void						setMaterialFrame(uint32_t index, const nvidia::MaterialFrame& materialFrame) = 0; ///< \see ConvexHull::getMaterialFrameCount
	virtual uint32_t					addMaterialFrame() = 0; ///< \see ConvexHull::getMaterialFrameCount

	/**
		Serialization.  The user must instantiate Embedding in order to successfully
		serialize any embedded data.
	*/
	virtual void						serialize(PxFileBuf& stream, Embedding& embedding) const = 0;
	
	/**
		Serialization.  The user must instantiate Embedding in order to successfully
		serialize any embedded data.
	*/
	virtual void						deserialize(PxFileBuf& stream, Embedding& embedding) = 0;

	/**
		Copies the input mesh in to this object.
	*/
	virtual void						set(const ExplicitHierarchicalMesh& mesh) = 0;

	/**
		Creates an internal BSP representation of the mesh parts up to the root depth.
		This is used by authoring tools to perform CSG operations.  If the user instantiates
		IProgressListener, they may pass it in to report progress of this operation.
		If microgridSize is not NULL, *microgridSize is used in the BSP calculation.  Otherwise the
		default parameters are used.
		meshMode is used to determine if the mesh is open or closed.  See NxMeshProcessingParameters::MeshMode
	*/
	virtual void						calculateMeshBSP(uint32_t randomSeed, IProgressListener* progressListener = NULL, const uint32_t* microgridSize = NULL, BSPOpenMode::Enum meshMode = BSPOpenMode::Automatic) = 0;

	/**
		Utility to replace the submesh on a set of interior triangles.
	*/
	virtual void						replaceInteriorSubmeshes(uint32_t partIndex, uint32_t frameCount, uint32_t* frameIndices, uint32_t submeshIndex) = 0;

	/**
		Draws various components of this object to the debugRenderer, as
		defined by the flags (see the visualization Enum above).  Some
		of the flags require an index be passed in as well.
	*/
	virtual void						visualize(RenderDebugInterface& debugRender, uint32_t flags, uint32_t index = 0) const = 0;

	/**
		Releases all memory associated with this object and deletes itself.
	*/
	virtual void						release() = 0;

protected:
	/**
		Constructor and destructor are not public
		Use createExplicitHierarchicalMesh() to instantiate an ExplicitHierarchicalMesh and
		ExplicitHierarchicalMesh::release() to destroy it.
	*/
	ExplicitHierarchicalMesh() {}
	virtual								~ExplicitHierarchicalMesh() {}

private:
	/** The assignment operator is disabled, use set() instead. */
	ExplicitHierarchicalMesh&			operator = (const ExplicitHierarchicalMesh&)
	{
		return *this;
	}
};


PX_POP_PACK

}
} // end namespace nvidia


#endif // EXPLICIT_HIERARCHICAL_MESH_H
