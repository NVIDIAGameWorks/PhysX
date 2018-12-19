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


#ifndef FRACTURING_H

#define FRACTURING_H

#include "Apex.h"
#include "ApexUsingNamespace.h"
#include "PxPlane.h"
//#include "ApexSharedSerialization.h"
#include "FractureTools.h"
#include "ApexString.h"
#include "ExplicitHierarchicalMesh.h"
#include "authoring/ApexCSG.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace nvidia
{
namespace apex
{

using namespace FractureTools;


struct IntersectMesh
{
	enum GridPattern
	{
		None,	// An infinite plane
		Equilateral,
		Right
	};

	float getSide(const physx::PxVec3& v)
	{
		if (m_pattern == None)
		{
			return m_plane.distance(v);
		}
		physx::PxVec3 vLocal = m_tm.inverseRT().transform(v);
		float x = vLocal.x - m_cornerX;
		float y = vLocal.y - m_cornerY;
		if (y < 0)
		{
			return 0;
		}
		float scaledY = y / m_ySpacing;
		uint32_t gridY = (uint32_t)scaledY;
		if (gridY >= m_numY)
		{
			return 0;
		}
		scaledY -= (float)gridY;
		uint32_t yParity = gridY & 1;
		if (yParity != 0)
		{
			scaledY = 1.0f - scaledY;
		}
		if (m_pattern == Equilateral)
		{
			x += 0.5f * m_xSpacing * scaledY;
		}
		if (x < 0)
		{
			return 0;
		}
		float scaledX = x / m_xSpacing;
		uint32_t gridX = (uint32_t)scaledX;
		if (gridX >= m_numX)
		{
			return 0;
		}
		scaledX -= (float)gridX;
		uint32_t xParity = (uint32_t)(scaledX >= scaledY);
		uint32_t triangleNum = 2 * (gridY * m_numX + gridX) + xParity;
		PX_ASSERT(triangleNum < m_triangles.size());
		nvidia::ExplicitRenderTriangle& triangle = m_triangles[triangleNum];
		physx::PxVec3& v0 = triangle.vertices[0].position;
		physx::PxVec3& v1 = triangle.vertices[1].position;
		physx::PxVec3& v2 = triangle.vertices[2].position;
		return ((v1 - v0).cross(v2 - v0)).dot(v - v0);
	}

	void clear()
	{
		m_pattern = None;
		m_plane = physx::PxPlane(0, 0, 1, 0);
		m_vertices.reset();
		m_triangles.reset();
	}

	void build(const physx::PxPlane& plane)
	{
		clear();
		m_plane = plane;
	}

	void build(GridPattern pattern, const physx::PxPlane& plane,
	           float cornerX, float cornerY, float xSpacing, float ySpacing, uint32_t numX, uint32_t numY,
	           const PxMat44& tm, float noiseAmplitude, float relativeFrequency, float xPeriod, float yPeriod,
	           int noiseType, int noiseDir, uint32_t submeshIndex, uint32_t frameIndex, const TriangleFrame& triangleFrame, bool forceGrid);

	GridPattern							m_pattern;

	PxMat44								m_tm;
	physx::PxPlane						m_plane;
	physx::Array<nvidia::Vertex>		m_vertices;
	physx::Array<nvidia::ExplicitRenderTriangle> m_triangles;

	uint32_t							m_numX;
	float								m_cornerX;
	float								m_xSpacing;
	uint32_t							m_numY;
	float								m_cornerY;
	float								m_ySpacing;
};

struct DisplacementMapVolumeImpl : public DisplacementMapVolume
{
	DisplacementMapVolumeImpl();

	void init(const FractureSliceDesc& desc);

	void getData(uint32_t& width, uint32_t& height, uint32_t& depth, uint32_t& size, unsigned char const** ppData) const;

private:

	void buildData(const physx::PxVec3 scale = physx::PxVec3(1)) const;

	// Data creation is lazy, and does not effect externally visible state
	//    Note: At some point, we will want to switch to floating point displacements
	mutable physx::Array<unsigned char> data;

	uint32_t width;
	uint32_t height;
	uint32_t depth;

};

// CutoutSetImpl

struct PolyVert
{
	uint16_t index;
	uint16_t flags;
};

struct ConvexLoop
{
	physx::Array<PolyVert> polyVerts;
};

struct Cutout
{
	physx::Array<physx::PxVec3> vertices;
	physx::Array<ConvexLoop> convexLoops;
};

struct CutoutSetImpl : public CutoutSet
{
	CutoutSetImpl() : periodic(false), dimensions(0.0f)
	{
	}

	enum Version
	{
		First = 0,
		// New versions must be put here.  There is no need to explicitly number them.  The
		// numbers above were put there to conform to the old DestructionToolStreamVersion enum.

		Count,
		Current = Count - 1
	};

	uint32_t			getCutoutCount() const
	{
		return cutouts.size();
	}

	uint32_t			getCutoutVertexCount(uint32_t cutoutIndex) const
	{
		return cutouts[cutoutIndex].vertices.size();
	}
	uint32_t			getCutoutLoopCount(uint32_t cutoutIndex) const
	{
		return cutouts[cutoutIndex].convexLoops.size();
	}

	const physx::PxVec3&	getCutoutVertex(uint32_t cutoutIndex, uint32_t vertexIndex) const
	{
		return cutouts[cutoutIndex].vertices[vertexIndex];
	}

	uint32_t			getCutoutLoopSize(uint32_t cutoutIndex, uint32_t loopIndex) const
	{
		return cutouts[cutoutIndex].convexLoops[loopIndex].polyVerts.size();
	}

	uint32_t			getCutoutLoopVertexIndex(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexNum) const
	{
		return cutouts[cutoutIndex].convexLoops[loopIndex].polyVerts[vertexNum].index;
	}
	uint32_t			getCutoutLoopVertexFlags(uint32_t cutoutIndex, uint32_t loopIndex, uint32_t vertexNum) const
	{
		return cutouts[cutoutIndex].convexLoops[loopIndex].polyVerts[vertexNum].flags;
	}
	bool					isPeriodic() const
	{
		return periodic;
	}
	const physx::PxVec2&	getDimensions() const
	{
		return dimensions;
	}

	void					serialize(physx::PxFileBuf& stream) const;
	void					deserialize(physx::PxFileBuf& stream);

	void					release()
	{
		delete this;
	}

	physx::Array<Cutout>	cutouts;
	bool					periodic;
	physx::PxVec2			dimensions;
};

class PartConvexHullProxy : public ExplicitHierarchicalMesh::ConvexHull, public UserAllocated
{
public:
	ConvexHullImpl	impl;

	PartConvexHullProxy()
	{
		impl.init();
	}

	PartConvexHullProxy(const PartConvexHullProxy& hull)
	{
		*this = hull;
	}

	PartConvexHullProxy&			operator = (const PartConvexHullProxy& hull)
	{
		impl.init();
		if (hull.impl.mParams)
		{
			impl.mParams->copy(*hull.impl.mParams);
		}
		return *this;
	}

	virtual void					buildFromPoints(const void* points, uint32_t numPoints, uint32_t pointStrideBytes)
	{
		impl.buildFromPoints(points, numPoints, pointStrideBytes);
	}

	virtual const physx::PxBounds3&	getBounds() const
	{
		return impl.getBounds();
	}

	virtual float			getVolume() const
	{
		return impl.getVolume();
	}

	virtual uint32_t			getVertexCount() const
	{
		return impl.getVertexCount();
	}

	virtual physx::PxVec3			getVertex(uint32_t vertexIndex) const
	{
		if (vertexIndex < impl.getVertexCount())
		{
			return impl.getVertex(vertexIndex);
		}
		return physx::PxVec3(0.0f);
	}

	virtual uint32_t			getEdgeCount() const
	{
		return impl.getEdgeCount();
	}

	virtual physx::PxVec3			getEdgeEndpoint(uint32_t edgeIndex, uint32_t whichEndpoint) const
	{
		if (edgeIndex < impl.getEdgeCount())
		{
			return impl.getVertex(impl.getEdgeEndpointIndex(edgeIndex, whichEndpoint));
		}
		return physx::PxVec3(0.0f);
	}

	/**
		This is the number of planes which bound the convex hull.
	*/
	virtual uint32_t			getPlaneCount() const
	{
		return impl.getPlaneCount();
	}

	/**
		This is the plane indexed by planeIndex, which must in
		the range [0, getPlaneCount()-1].
	*/
	virtual physx::PxPlane			getPlane(uint32_t planeIndex) const
	{
		if (planeIndex < impl.getPlaneCount())
		{
			return impl.getPlane(planeIndex);
		}
		return physx::PxPlane(physx::PxVec3(0.0f), 0.0f);
	}

	virtual bool					rayCast(float& in, float& out, const physx::PxVec3& orig, const physx::PxVec3& dir,
	        const physx::PxTransform& localToWorldRT, const physx::PxVec3& scale, physx::PxVec3* normal = NULL) const
	{
		return impl.rayCast(in, out, orig, dir, localToWorldRT, scale, normal);
	}

	virtual bool					reduceHull(uint32_t maxVertexCount, uint32_t maxEdgeCount, uint32_t maxFaceCount, bool inflated)
	{
		return impl.reduceHull(maxVertexCount, maxEdgeCount, maxFaceCount, inflated);
	}

	virtual void					release()
	{
		delete this;
	}
};

PX_INLINE void	resizeCollision(physx::Array<PartConvexHullProxy*>& collision, uint32_t hullCount)
{
	const uint32_t oldHullCount = collision.size();
	for (uint32_t i = hullCount; i < oldHullCount; ++i)
	{
		collision[i]->release();
	}
	collision.resize(hullCount);
	for (uint32_t i = oldHullCount; i < hullCount; ++i)
	{
		collision[i] = PX_NEW(PartConvexHullProxy)();
	}
}

void buildCollisionGeometry(physx::Array<PartConvexHullProxy*>& volumes, const CollisionVolumeDesc& desc,
							const physx::PxVec3* vertices, uint32_t vertexCount, uint32_t vertexByteStride,
							const uint32_t* indices, uint32_t indexCount);


// ExplicitHierarchicalMeshImpl

static uint64_t	sNextChunkEUID = 0;	// Execution-unique identifier for chunks

class ExplicitHierarchicalMeshImpl : public ExplicitHierarchicalMesh, public UserAllocated
{
public:

	// This has been copied from DestructionToolStreamVersion, at ToolStreamVersion_RemovedExplicitHMesh_mMaxDepth.
	enum Version
	{
		First = 0,
		AddedMaterialFramesToHMesh_and_NoiseType_and_GridSize_to_Cleavage = 7,
		IncludingVertexFormatInSubmeshData = 12,
		AddedMaterialLibraryToMesh = 14,
		AddedCacheChunkSurfaceTracesAndInteriorSubmeshIndex = 32,
		RemovedExplicitHMesh_mMaxDepth = 38,
		UsingExplicitPartContainers,
		SerializingMeshBSP,
		SerializingMeshBounds,
		AddedFlagsFieldToPart,
		PerPartMeshBSPs,
		StoringRootSubmeshCount,
		MultipleConvexHullsPerChunk,
		InstancingData,
		UVInstancingData,
		DisplacementData,
		ChangedMaterialFrameToIncludeFracturingMethodContext,
		RemovedInteriorSubmeshIndex,
		AddedSliceDepthToMaterialFrame,
		RemovedNxChunkAuthoringFlag,
		ReaddedFlagsToPart,
		IntroducingChunkPrivateFlags,
		// New versions must be put here.  There is no need to explicitly number them.  The
		// numbers above were put there to conform to the old DestructionToolStreamVersion enum.

		Count,
		Current = Count - 1
	};

	struct Part : public UserAllocated
	{
		Part() : mMeshBSP(NULL), mSurfaceNormal(0.0f), mFlags(0)
		{
			mBounds.setEmpty();
		}

		~Part()
		{
			if (mMeshBSP != NULL)
			{
				mMeshBSP->release();
				mMeshBSP = NULL;
			}
			resizeCollision(mCollision, 0);
		}

		enum Flags
		{
			MeshOpen =	(1<<0),
		};

		physx::PxBounds3								mBounds;
		physx::Array<nvidia::ExplicitRenderTriangle>	mMesh;
		ApexCSG::IApexBSP*								mMeshBSP;
		physx::Array<PartConvexHullProxy*>		mCollision;
		physx::PxVec3									mSurfaceNormal;	// used to kick chunk out if desired
		uint32_t									mFlags;	// See Flags
	};

	struct Chunk : public UserAllocated
	{
		Chunk() : mParentIndex(-1), mFlags(0), mPartIndex(-1), mInstancedPositionOffset(physx::PxVec3(0.0f)), mInstancedUVOffset(physx::PxVec2(0.0f)), mPrivateFlags(0)
		{
			mEUID = sNextChunkEUID++;
		}

		enum Flags
		{
			Root		=	(1<<0),
			RootLeaf	=	(1<<1),
		};

		bool	isRootChunk() const
		{
			return (mPrivateFlags & Root) != 0;
		}

		bool	isRootLeafChunk() const	// This means that the chunk is a root chunk and has no children that are root chunks
		{
			return (mPrivateFlags & RootLeaf) != 0;
		}

		PX_INLINE uint64_t	getEUID() const
		{
			return mEUID;
		}

		int32_t							mParentIndex;
		uint32_t							mFlags;	// See DestructibleAsset::ChunkFlags
		int32_t							mPartIndex;
		physx::PxVec3							mInstancedPositionOffset;	// if instanced, the offsetPosition
		physx::PxVec2							mInstancedUVOffset;	// if instanced, the offset UV
		uint32_t							mPrivateFlags;	// Things that don't make it to the DestructibleAsset; authoring only.  See ExplicitHierarchicalMeshImpl::Chunk::Flags

	private:
		uint64_t							mEUID;	// A unique identifier during the application execution.  Not to be serialized.
	};

	physx::Array<Part*>					mParts;
	physx::Array<Chunk*>				mChunks;
	physx::Array<ExplicitSubmeshData>	mSubmeshData;
	physx::Array<nvidia::MaterialFrame>	mMaterialFrames;
	uint32_t						mRootSubmeshCount;	// How many submeshes came with the root mesh

	ApexCSG::IApexBSPMemCache*			mBSPMemCache;

	DisplacementMapVolumeImpl               mDisplacementMapVolume;

	ExplicitHierarchicalMeshImpl();
	~ExplicitHierarchicalMeshImpl();

	// Sorts chunks in parent-sorted order (stable)
	void sortChunks(physx::Array<uint32_t>* indexRemap = NULL);

	// Generate part surface normals, if possible
	void createPartSurfaceNormals();

	// ExplicitHierarchicalMesh implementation:

	uint32_t addPart();
	bool removePart(uint32_t index);
	uint32_t addChunk();
	bool removeChunk(uint32_t index);
	void serialize(physx::PxFileBuf& stream, Embedding& embedding) const;
	void deserialize(physx::PxFileBuf& stream, Embedding& embedding);
	int32_t maxDepth() const;
	uint32_t partCount() const;
	uint32_t chunkCount() const;
	uint32_t depth(uint32_t chunkIndex) const;
	int32_t* parentIndex(uint32_t chunkIndex);
	uint64_t chunkUniqueID(uint32_t chunkIndex);
	int32_t* partIndex(uint32_t chunkIndex);
	physx::PxVec3* instancedPositionOffset(uint32_t chunkIndex);
	physx::PxVec2* instancedUVOffset(uint32_t chunkIndex);
	uint32_t meshTriangleCount(uint32_t partIndex) const;
	nvidia::ExplicitRenderTriangle* meshTriangles(uint32_t partIndex);
	physx::PxBounds3 meshBounds(uint32_t partIndex) const;
	physx::PxBounds3 chunkBounds(uint32_t chunkIndex) const;
	uint32_t* chunkFlags(uint32_t chunkIndex) const;
	uint32_t convexHullCount(uint32_t partIndex) const;
	const ExplicitHierarchicalMesh::ConvexHull** convexHulls(uint32_t partIndex) const;
	physx::PxVec3* surfaceNormal(uint32_t partIndex);
	const DisplacementMapVolume& displacementMapVolume() const;
	uint32_t submeshCount() const;
	ExplicitSubmeshData* submeshData(uint32_t submeshIndex);
	uint32_t addSubmesh(const ExplicitSubmeshData& submeshData);
	uint32_t getMaterialFrameCount() const;
	nvidia::MaterialFrame getMaterialFrame(uint32_t index) const;
	void setMaterialFrame(uint32_t index, const nvidia::MaterialFrame& materialFrame);
	uint32_t addMaterialFrame();
	void clear(bool keepRoot = false);
	void set(const ExplicitHierarchicalMesh& mesh);
	bool calculatePartBSP(uint32_t partIndex, uint32_t randomSeed, uint32_t microgridSize, BSPOpenMode::Enum meshMode, IProgressListener* progressListener = NULL, volatile bool* cancel = NULL);
	void calculateMeshBSP(uint32_t randomSeed, IProgressListener* progressListener = NULL, const uint32_t* microgridSize = NULL, BSPOpenMode::Enum meshMode = BSPOpenMode::Automatic);
	void replaceInteriorSubmeshes(uint32_t partIndex, uint32_t frameCount, uint32_t* frameIndices, uint32_t submeshIndex);
	void visualize(RenderDebugInterface& debugRender, uint32_t flags, uint32_t index = 0) const;
	void release();
	void buildMeshBounds(uint32_t partIndex);
	void buildCollisionGeometryForPart(uint32_t partIndex, const CollisionVolumeDesc& desc);
	void buildCollisionGeometryForRootChunkParts(const CollisionDesc& desc, bool aggregateRootChunkParentCollision = true);
	void initializeDisplacementMapVolume(const nvidia::FractureSliceDesc& desc);
	void reduceHulls(const CollisionDesc& desc, bool inflated);
	void aggregateCollisionHullsFromRootChildren(uint32_t chunkIndex);
};

}
} // end namespace nvidia::apex

#endif

#endif
