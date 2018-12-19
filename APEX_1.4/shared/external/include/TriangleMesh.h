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


#ifndef TRIANGLE_MESH_H
#define TRIANGLE_MESH_H


#include <RenderMeshAsset.h>
#include <UserRenderResourceManager.h>

#include <vector>
#include <string>

#include "PsFastXml.h"
#include "PsAllocator.h"

#ifdef __ORBIS__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

namespace nvidia
{
namespace apex
{
class ClothingPhysicalMesh;
class RenderMeshAssetAuthoring;

class ResourceCallback;

class UserRenderer;
class UserRenderResourceManager;
class UserRenderResource;
class UserRenderVertexBuffer;
class UserRenderIndexBuffer;
class UserRenderBoneBuffer;
}
}

namespace mimp
{
class MeshSystemContainer;
};

namespace SampleRenderer
{
class Renderer;

class RendererVertexBuffer;
class RendererIndexBuffer;
class RendererMaterial;
class RendererMaterialInstance;
class RendererMesh;
class RendererMeshContext;
}

namespace SampleFramework
{
class SampleMaterialAsset;
}


namespace Samples
{

class SkeletalAnim;

enum PaintChannelType
{
	PC_MAX_DISTANCE,
	PC_COLLISION_DISTANCE,
	PC_LATCH_TO_NEAREST_SLAVE,
	PC_LATCH_TO_NEAREST_MASTER,
	PC_NUM_CHANNELS,
};

struct MaterialResource
{
	physx::PxVec3 color;
	uint32_t handle;
	bool hasAlpha;
	std::string name;
};

PX_INLINE void PxVec3FromArray(physx::PxVec3& out, const float arr[3])
{
	out = physx::PxVec3(arr[0], arr[1], arr[2]);
};

PX_INLINE void PxQuatFromArray(physx::PxQuat& out, const float arr[4])
{
	out.x = arr[0];
	out.y = arr[1];
	out.z = arr[2];
	out.w = arr[3];
};

//------------------------------------------------------------------------------------
struct TriangleSubMesh
{
	void init()
	{
		name = "";
		materialName = "";
		originalMaterialName = "";
		mRendererMesh = NULL;
		mRendererMeshContext = NULL;
		mSampleMaterial = NULL;
		mRendererMaterialReference = NULL;
		mRendererMaterialInstance = NULL;
		firstIndex = (uint32_t) - 1;
		numIndices = 0;
		color = 0xfefefeff;
		materialResource = NULL;
		maxBoneIndex = -1;
		maxBonesShader = 0;
		mRenderResource = NULL;
		cullMode = nvidia::RenderCullMode::NONE;
		show = true;
		selected = false;
		selectionActivated = false;
		usedForCollision = false;
		hasApexAsset = false;
		invisible = false;
		resourceNeedsUpdate = false;
	}

	void setMaterialReference(SampleRenderer::RendererMaterial* material, SampleRenderer::RendererMaterialInstance* materialInstance);
	std::string name;
	std::string materialName;
	std::string originalMaterialName;

	SampleRenderer::RendererMesh*				mRendererMesh;
	SampleRenderer::RendererMeshContext*		mRendererMeshContext;
	SampleFramework::SampleMaterialAsset*		mSampleMaterial;
	SampleRenderer::RendererMaterial*			mRendererMaterialReference; // reference from the mSampleMaterial
	SampleRenderer::RendererMaterialInstance*	mRendererMaterialInstance; // clone from the mSampleMaterial

	unsigned int firstIndex;
	unsigned int numIndices;
	unsigned int color;
	MaterialResource* materialResource;
	int maxBoneIndex;
	unsigned int maxBonesShader;
	nvidia::apex::UserRenderResource* mRenderResource;
	nvidia::apex::RenderCullMode::Enum cullMode;
	bool show;
	bool selected;
	bool selectionActivated;
	bool usedForCollision;
	bool hasApexAsset;
	bool invisible;
	bool resourceNeedsUpdate;
	bool operator<(const TriangleSubMesh& other) const
	{
		if (name == other.name)
		{
			return materialName < other.materialName;
		}
		return name < other.name;
	}
};

// ----------------------------------------------------------------------
struct float4
{
	float4() {}
	float4(float R, float G, float B, float A) : r(R), g(G), b(B), a(A) {}
	float r;
	float g;
	float b;
	float a;
};

//------------------------------------------------------------------------------------
struct ushort4
{
	unsigned short elem[4];
};

//------------------------------------------------------------------------------------
struct TriangleEdgeSplit		// for mesh subdivision
{
	int adjVertNr;
	int newVertNr;
	int next;
};

//------------------------------------------------------------------------------------
struct PaintedVertex
{
	explicit PaintedVertex()							: paintValueF32(0.0f), paintValueU32(0), color(0) {}
	explicit PaintedVertex(unsigned int u)				: paintValueF32(0.0f), paintValueU32(u), color(0) {}
	explicit PaintedVertex(float f)						: paintValueF32(f), paintValueU32(0), color(0) {}
	explicit PaintedVertex(unsigned int u, float f)		: paintValueF32(f), paintValueU32(u), color(0) {}

	void setColor(float r, float g, float b)
	{
		union
		{
			unsigned int ucolor;
			unsigned char ccolor[4];
		};
		ccolor[0] = (unsigned char)(r * 255);
		ccolor[1] = (unsigned char)(g * 255);
		ccolor[2] = (unsigned char)(b * 255);
		ccolor[3] = 0xff;
		color = ucolor;
	}
	float paintValueF32;
	unsigned int paintValueU32;
	unsigned int color;
};

//------------------------------------------------------------------------------------
class TriangleMesh : public physx::shdfnd::FastXml::Callback
{
public:
	TriangleMesh(uint32_t moduleIdentifier, SampleRenderer::Renderer* renderer = NULL);
	virtual ~TriangleMesh();

	void setRenderer(SampleRenderer::Renderer* renderer);

	void clear(nvidia::apex::UserRenderResourceManager* rrm, nvidia::apex::ResourceCallback* rcb);
	void loadMaterials(nvidia::apex::ResourceCallback* resourceCallback, nvidia::apex::UserRenderResourceManager* rrm,
	                   bool dummyMaterial = false, const char* materialPrefix = NULL, const char* materialSuffix = NULL,
	                   bool onlyVisibleMaterials = false);
	void moveAboveGround(float level);
	void initSingleMesh();

	void copyFrom(const TriangleMesh& mesh);
	void copyFromSubMesh(const TriangleMesh& mesh, int subMeshNr = -1, bool filterVertices = false);

	bool loadFromObjFile(const std::string& filename, bool useCustomChannels);
	bool saveToObjFile(const std::string& filename) const;
	bool loadFromXML(const std::string& filename, bool loadCustomChannels);
	bool saveToXML(const std::string& filename);
	bool loadFromParent(TriangleMesh* parent);
	bool loadFromMeshImport(mimp::MeshSystemContainer* msc, bool useCustomChannels);
	bool saveToMeshImport(mimp::MeshSystemContainer* msc);

	void initPlane(float length, float uvDist, const char* materialName);
	void initFrom(nvidia::apex::ClothingPhysicalMesh& mesh, bool initCustomChannels);
	void initFrom(nvidia::apex::RenderMeshAssetAuthoring& mesh, bool initCustomChannels);

	void applyMorphDisplacements(const std::vector<physx::PxVec3>& displacements);

	void drawPainting(PaintChannelType channelType, bool skinned, nvidia::apex::RenderDebugInterface* batcher);
	void drawVertices(PaintChannelType channelType, float maxDistanceScaling, float collisionDistanceScaling, float pointScaling,
	                  float vmin, float vmax, nvidia::apex::RenderDebugInterface* batcher);
	void drawVertices(size_t boneNr, float minWeight, float pointScaling, nvidia::apex::RenderDebugInterface* batcher) const;
	void drawNormals(float normalScale, bool activeVerticesOnly, nvidia::apex::RenderDebugInterface* batcher) const;
	void drawTangents(float tangentScale, nvidia::apex::RenderDebugInterface* batcher) const;
	void drawMaxDistancePartitions(float paintingScale, const float* partitions, size_t numPartitions, nvidia::apex::RenderDebugInterface* batcher);
	void drawTetrahedrons(bool wireframe, float scale, nvidia::apex::RenderDebugInterface* batcher);

	void updateRenderResources(bool rewriteBuffers, nvidia::apex::UserRenderResourceManager& rrm, void* userRenderData = 0);
	void updateRenderResourcesInternal(bool rewriteBuffers, nvidia::apex::UserRenderResourceManager& rrm, void* userRenderData, bool createResource);
	void dispatchRenderResources(nvidia::apex::UserRenderer& r, const physx::PxMat44& currentPose);

	void updateRenderer(bool rewriteBuffers, bool overrideMaterial, bool sharedOnly = false);
	void queueForRendering(const physx::PxMat44& currentPose, bool wireframe);

	void skin(const SkeletalAnim& anim, float scale = 1.0f);
	void unskin();

	enum
	{
		NUM_TEXCOORDS = 4,
	};

	// accessors
	size_t getNumSubmeshes() const
	{
		return mSubMeshes.size();
	}
	const TriangleSubMesh* getSubMesh(size_t i) const
	{
		if (i < mSubMeshes.size())
		{
			return &mSubMeshes[i];
		}
		return NULL;
	}

	void showSubmesh(size_t index, bool on);
	void selectSubMesh(size_t index, bool selected);
	void hideSubMesh(const char* submeshName, const char* materialName);
	size_t getNumTriangles(size_t subMeshNr) const;
	size_t getNumIndices() const
	{
		return mIndices.size();
	}
	size_t getNumVertices() const
	{
		return mVertices.size();
	}
	const std::vector<physx::PxVec3> &getVertices() const
	{
		return mVertices;
	}
	const std::vector<physx::PxVec3> &getNormals() const
	{
		return mNormals;
	}
	const std::vector<physx::PxVec3> &getTangents() const
	{
		return mTangents;
	}
	const std::vector<physx::PxVec3> &getBitangents() const
	{
		return mBitangents;
	}
	std::vector<PaintedVertex> &getPaintChannel(PaintChannelType channelType);
	const std::vector<PaintedVertex> &getPaintChannel(PaintChannelType channelType) const;
	float getMaximalMaxDistance() const;

	const std::vector<nvidia::VertexUV> &getTexCoords(int index) const
	{
		PX_ASSERT(index >= 0);
		PX_ASSERT(index < NUM_TEXCOORDS);
		return mTexCoords[index];
	}
	const std::vector<unsigned short>  &getBoneIndices() const
	{
		return mBoneIndicesExternal;
	}
	const std::vector<physx::PxVec4> &getBoneWeights() const
	{
		return mBoneWeights;
	}

	const std::vector<uint32_t> &getIndices() const
	{
		return mIndices;
	}

	const std::vector<bool>& getActiveSubmeshVertices() const
	{
		return mActiveSubmeshVertices;
	}

	void getBounds(physx::PxBounds3& bounds) const
	{
		bounds = mBounds;
	}

	int getBoneAssignments(uint32_t vertNr, const uint16_t* &bones, const float* &weights) const;
	bool hasBoneAssignments() const
	{
		return mBoneIndicesExternal.size() == mVertices.size() * 4;
	}

	int getMaxBoneIndex()
	{
		return mMaxBoneIndexExternal;
	}
	// manipulators

	void displaceAlongNormal(float displacement);
	bool generateTangentSpace();

	void updateBounds();

	void setSubMeshColor(size_t subMeshNr, uint32_t color);
	void setSubMeshMaterialName(size_t subMeshNr, const char* materialName, nvidia::ResourceCallback* resourceCallback);
	void setSubMeshUsedForCollision(size_t subMeshNr, bool enable);
	void setSubMeshHasPhysics(size_t subMeshNr, bool enable);
	void setAllColors(unsigned int color);

	void subdivideSubMesh(int subMeshNr, int subdivision, bool evenOutVertexDegrees);
	void evenOutVertexDegree(int subMeshNr, int numIters);

	void setCullMode(nvidia::RenderCullMode::Enum cullMode, int32_t submeshIndex);

	void updatePaintingColors(PaintChannelType channelType, float maxDistMin, float maxDistMax, unsigned int flag, nvidia::apex::RenderDebugInterface* batcher);

	virtual bool processElement(const char* elementName, const char* elementData, const physx::shdfnd::FastXml::AttributePairs& attr, int lineno);
	virtual bool processComment(const char* comment) // encountered a comment in the XML
	{
		PX_UNUSED(comment);
		return true;
	}

	virtual bool processClose(const char* element, uint32_t depth, bool& isError)	 // process the 'close' indicator for a previously encountered element
	{
		PX_UNUSED(element);
		PX_UNUSED(depth);
		isError = false;
		return true;
	}

	virtual void*   fastxml_malloc(uint32_t size)
	{
		return ::malloc(size);
	}
	virtual void	fastxml_free(void* mem)
	{
		::free(mem);
	}


	void setTextureUVOrigin(nvidia::TextureUVOrigin::Enum origin)
	{
		textureUvOriginChanged |= mTextureUVOrigin != origin;
		mTextureUVOrigin = origin;
	}
	nvidia::TextureUVOrigin::Enum getTextureUVOrigin() const
	{
		return mTextureUVOrigin;
	}

	bool hasSkinningVertices();
private:
	void operator=(const TriangleMesh& other); // empty

	void updateNormals(int subMeshNr);
	void updateTangents();
	void updateBoneWeights();
	void optimizeForRendering();
	void complete(bool useCustomChannels);
	void hasRandomColors(size_t howmany);

	void updateSubmeshInfo();

	enum ParserState
	{
		PS_Uninitialized,
		PS_Mesh,
		PS_Submeshes,
		PS_Skeleton,
	};
	ParserState mParserState;

	// vertices
	nvidia::UserRenderVertexBuffer* mDynamicVertexBuffer;
	nvidia::UserRenderVertexBuffer* mStaticVertexBuffer;
	nvidia::UserRenderIndexBuffer* mIndexBuffer;
	nvidia::UserRenderBoneBuffer* mBoneBuffer;

	std::vector<physx::PxVec3> mVertices;
	std::vector<physx::PxVec3> mNormals;
	std::vector<physx::PxVec3> mTangents;
	std::vector<physx::PxVec3> mBitangents;

	std::vector<PaintedVertex>	mPaintChannels[PC_NUM_CHANNELS];

	std::vector<nvidia::VertexUV> mTexCoords[NUM_TEXCOORDS];

	// triangles
	std::vector<TriangleSubMesh> mSubMeshes;
	std::vector<uint32_t> mIndices;

	// skeleton binding
	std::string mSkeletonFile;
	std::vector<unsigned short> mBoneIndicesExternal;
	std::vector<unsigned short> mBoneIndicesInternal;
	std::vector<physx::PxVec4> mBoneWeights;
	std::vector<uint32_t> mNumBoneWeights;

	std::vector<physx::PxVec3> mSkinnedVertices;
	std::vector<physx::PxVec3> mSkinnedNormals;

	std::vector<int> mBoneMappingInt2Ext;
	std::vector<physx::PxMat44> mSkinningMatrices; // PH: VERY CAREFUL WHEN CHANGING THIS!!! The bone buffer doesn't validate types in writeBuffer calls!

	// others
	std::string mName;
	physx::PxBounds3 mBounds;

	int mMaxBoneIndexInternal;
	int mMaxBoneIndexExternal;

	// submesh info for per-vertex drawing
	std::vector<bool> mActiveSubmeshVertices;

	// temporary, used in painting
	std::vector<int> mTriangleMarks;
	std::vector<int> mVertexMarks;
	int mNextMark;

	TriangleMesh* mParent;

	// temporary, subdivision data structures
	int addSplitVert(int vertNr0, int vertNr1);
	std::vector<int> mVertexFirstSplit;
	std::vector<TriangleEdgeSplit> mVertexSplits;

	bool vertexValuesChangedDynamic;
	bool vertexValuesChangedStatic;
	bool vertexCountChanged;
	bool indicesChanged;
	bool skinningMatricesChanged;
	bool oneCullModeChanged;
	bool textureUvOriginChanged;

	std::vector<uint32_t> mRandomColors;

	std::string mMaterialPrefix;
	std::string mMaterialSuffix;

	nvidia::apex::TextureUVOrigin::Enum mTextureUVOrigin;

	SampleRenderer::Renderer*				mRenderer;
	SampleRenderer::RendererVertexBuffer*	mRendererVertexBufferDynamic;
	SampleRenderer::RendererVertexBuffer*	mRendererVertexBufferShared;
	SampleRenderer::RendererIndexBuffer*	mRendererIndexBuffer;
	physx::PxMat44	mRendererTransform;

	SampleFramework::SampleMaterialAsset*	mOverrideMaterial;
	bool mUseGpuSkinning;
};

} // namespace Samples

#ifdef __ORBIS__
#pragma clang diagnostic pop
#endif

#endif
