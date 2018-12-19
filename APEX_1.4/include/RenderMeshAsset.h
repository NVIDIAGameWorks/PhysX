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



#ifndef RENDER_MESH_ASSET_H
#define RENDER_MESH_ASSET_H

/*!
\file
\brief APEX RenderMesh Asset
*/

#include "ApexUsingNamespace.h"
#include "VertexFormat.h"
#include "Asset.h"
#include "RenderBufferData.h"
#include "RenderMesh.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

class RenderMeshActor;
class RenderMeshActorDesc;
class CustomBufferIterator;

//! \brief Name of RenderMesh authoring type namespace
#define RENDER_MESH_AUTHORING_TYPE_NAME "ApexRenderMesh"

/**
\brief Stats for an RenderMeshAsset: memory usage, counts, etc.
*/
struct RenderMeshAssetStats
{
	uint32_t	totalBytes;				//!< Total byte size of mesh
	uint32_t	submeshCount;			//!< Number of submeshes
	uint32_t	partCount;				//!< Number of mesh parts
	uint32_t	vertexCount;			//!< Number of vertices
	uint32_t	indexCount;				//!< Size (count) of index buffer
	uint32_t	vertexBufferBytes;		//!< Byte size of vertex buffer
	uint32_t	indexBufferBytes;		//!< Byte size of index buffer
};

/**
\brief Instance buffer data mode.  DEPRECATED, to be removed by APEX 1.0
*/
struct RenderMeshAssetInstanceMode
{
	/**
	\brief Enum of instance buffer data.
	*/
	enum Enum
	{
		POSE_SCALE = 0,
		POS_VEL_LIFE,

		NUM_MODES
	};
};


/**
\brief The full RGBA color of a vertex
*/
struct VertexColor
{
public:

	PX_INLINE					VertexColor()	{}

	/**
	\brief Constructor
	*/
	PX_INLINE					VertexColor(const ColorRGBA c)
	{
		const float recip255 = 1 / (float)255;
		set((float)c.r * recip255, (float)c.g * recip255, (float)c.b * recip255, (float)c.a * recip255);
	}

	/**
	\brief Copy assignment operator
	*/
	PX_INLINE	VertexColor&	operator = (const VertexColor& c)
	{
		r = c.r;
		g = c.g;
		b = c.b;
		a = c.a;
		return *this;
	}

	/// \brief set the color as RGBA floats
	PX_INLINE	void			set(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

	/// \brief return the color as a 32bit integer
	PX_INLINE	ColorRGBA		toColorRGBA() const
	{
		return ColorRGBA((uint8_t)(255 * r + (float)0.5),
		                   (uint8_t)(255 * g + (float)0.5),
		                   (uint8_t)(255 * b + (float)0.5),
		                   (uint8_t)(255 * a + (float)0.5));
	}

	float r;		//!< RED
	float g;		//!< GREEN
	float b;		//!< BLUE
	float a;		//!< ALPHA
};


/**
\brief a simple u, v coordinate struct
*/
struct VertexUV
{
	VertexUV() {}

	/**
	\brief Constructor
	*/
	VertexUV(float _u, float _v)
	{
		set(_u, _v);
	}

	/**
	\brief Constructor
	*/
	VertexUV(const float uv[])
	{
		set(uv);
	}

	/**
	\brief Set coordinates
	*/
	void			set(float _u, float _v)
	{
		u = _u;
		v = _v;
	}

	/**
	\brief Set coordinates
	*/
	void			set(const float uv[])
	{
		u = uv[0];
		v = uv[1];
	}

	/**
	\brief operator []
	*/
	float&			operator [](int i)
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	/**
	\brief const operator []
	*/
	const float&	operator [](int i) const
	{
		PX_ASSERT(i >= 0 && i <= 1);
		return (&u)[i];
	}

	/// coordinate
	float	u;
	/// coordinate
	float	v;
};


/**
\brief An inefficient vertex description used for authoring
*/
struct Vertex
{
	PxVec3		position;		//!< Vertex position
	PxVec3		normal;			//!< Surface normal at this position
	PxVec3		tangent;		//!< Surface tangent at this position
	PxVec3		binormal;		//!< Surface binormal at this position
	VertexUV	uv[VertexFormat::MAX_UV_COUNT]; //!< Texture UV coordinates
	VertexColor	color;			//!< Color ar this position
	uint16_t	boneIndices[VertexFormat::MAX_BONE_PER_VERTEX_COUNT]; //!< Bones which are attached to this vertex
	float		boneWeights[VertexFormat::MAX_BONE_PER_VERTEX_COUNT]; //!< Per bone wieght, 0.0 if no bone
	uint16_t	displacementFlags; //!< Flags for vertex displacement

	/**
	\brief Constructor which clears the entire structure
	*/
	Vertex()
	{
		memset(this, 0, sizeof(Vertex));
	}
};


/**
\brief A less inefficient triangle description used for authoring
*/
struct ExplicitRenderTriangle
{
	Vertex		vertices[3];		//!< The three verts that define the triangle
	int32_t		submeshIndex;		//!< The submesh to which this triangle belongs
	uint32_t	smoothingMask;		//!< Smoothing mask
	uint32_t	extraDataIndex;		//!< Index of extra data

	/**
	\brief Returns an unnormalized normal, in general
	*/
	PxVec3	calculateNormal() const
	{
		return (vertices[1].position - vertices[0].position).cross(vertices[2].position - vertices[0].position);
	}
};



/**
\brief Descriptor for creating a rendering mesh part
*/
struct RenderMeshPartData
{
	RenderMeshPartData() : triangleCount(0), userData(NULL) {}

	/**
	\brief Constructor
	*/
	RenderMeshPartData(uint32_t _triCount, void* _data) : triangleCount(_triCount), userData(_data) {}

	uint32_t		triangleCount;	//!< Number of triangles in this mesh part
	void*			userData;		//!< User definable pointer to part data, passed back to createTriangles
};


/**
\brief Authoring interface for an RenderMeshAsset
*/
class RenderMeshAssetAuthoring : public AssetAuthoring
{
public:
	/** \brief Vertex buffer class used for mesh creation */
	class VertexBuffer : public RenderBufferData<RenderVertexSemantic, RenderVertexSemantic::Enum> {};

	/** \brief How the geometry is stored.  Currently only supporting triangles. */
	struct Primitive
	{
		/**
		\brief Enum of geometry stored types.
		*/
		enum Enum
		{
			TRIANGLE_LIST,
			//		TRIANGLE_STRIP, // Not supported for now
			//		TRIANGLE_FAN,	// Not supported for now

			COUNT
		};
	};

	/** What kind of integer is used for indices. */
	struct IndexType
	{
		/**
		\brief Enum of integers types using for indices.
		*/
		enum Enum
		{
			UINT,
			USHORT,

			COUNT
		};
	};

	/** Description of one submesh, corresponding to one material.  The vertex buffer format contains bone indices, so these do not need
	    to be described here.  The submesh's partitioning into parts is described here. */
	class SubmeshDesc
	{
	public:
		/** Name of material associated with this geometry. */
		const char*				m_materialName;

		/** Vertex buffers for this submesh. One may pass in the same buffers for each submesh. */
		const VertexBuffer*		m_vertexBuffers;

		/** Number of vertex buffers in m_VertexBuffers array. */
		uint32_t				m_numVertexBuffers;

		/** Number of vertices.  Each vertex buffer in m_VertexBuffers must have this many vertices. */
		uint32_t				m_numVertices;

		/** How the geometry is represented.  See the Primitive enum. */
		Primitive::Enum			m_primitive;

		/** Type of the indices used in m_VertexIndices. See the IndexType enum. */
		IndexType::Enum			m_indexType;

		/** Buffer of vertex indices, stored as described by primitive and indexSize. If NULL, m_vertexIndices = {0,1,2,...} is implied. */
		const void*				m_vertexIndices;

		/** Size (in indices) of m_VertexIndices. */
		uint32_t				m_numIndices;

		/**
			Smoothing groups associated with each triangle.  The size of this array (if not NULL) must be appropriate for the m_primitive type.
			Since only triangle lists are currently supported, the size of this array (if not NULL) must currently be m_numIndices/3.
		*/
		uint32_t*				m_smoothingGroups;

		/** Vertex index offset. */
		uint32_t				m_firstVertex;

		/** If not NULL, an array (of m_IndexType-sized indices) into m_VertexIndices, at the start of each part. */
		const void*				m_partIndices;

		/** If m_PartIndices is not NULL, the number of parts. */
		uint32_t				m_numParts;

		/** Winding order of the submesh */
		RenderCullMode::Enum	m_cullMode;

		/** Constructor sets default values. */
		SubmeshDesc()
		{
			memset(this, 0, sizeof(SubmeshDesc));
		}

		/** Validity check, returns true if this descriptor contains valid fields. */
		bool	isValid() const
		{
			return	m_materialName != NULL &&
			        m_vertexBuffers != NULL &&	// BRG - todo: check the vertex buffers for validity
			        m_numVertexBuffers > 0 &&
			        m_numVertices > 0 &&
			        m_primitive >= (Primitive::Enum)0 && m_primitive < Primitive::COUNT &&
			        m_indexType >= (IndexType::Enum)0 && m_indexType < IndexType::COUNT &&
			        m_numIndices > 0 &&
			        (m_partIndices == NULL || m_numParts > 0) &&
			        (m_cullMode == RenderCullMode::CLOCKWISE || m_cullMode == RenderCullMode::COUNTER_CLOCKWISE || m_cullMode == RenderCullMode::NONE);
		}
	};

	/** Description of a mesh, used for authoring an Render mesh.  It contains a number of vertex buffers and submeshes. */
	class MeshDesc
	{
	public:
		/** Array of descriptors for the submeshes in this mesh. */
		const SubmeshDesc*		m_submeshes;

		/** The number of elements in m_submeshes. */
		uint32_t				m_numSubmeshes;

		/** Texture UV direction. */
		TextureUVOrigin::Enum	m_uvOrigin;



		/** Constructor sets default values. */
		MeshDesc() : m_submeshes(NULL), m_numSubmeshes(0), m_uvOrigin(TextureUVOrigin::ORIGIN_TOP_LEFT) {}

		/** Validity check, returns true if this descriptor contains valid fields. */
		bool	isValid() const
		{
			return	m_submeshes != NULL &&
			        m_numSubmeshes > 0;
		}
	};


	/**
	\brief Mesh-building function.
	\param [in] meshDesc					contains the setup for all vertex buffers
	\param [in] createMappingInformation	A vertex buffer with remapping indices will be generated. The name of the buffer is VERTEX_ORIGINAL_INDEX
	*/
	virtual void					createRenderMesh(const MeshDesc& meshDesc, bool createMappingInformation) = 0;


	/**
	Utility to reduce a vertex buffer of explicit vertices (Vertex).
	The parameters 'map' and 'vertices' must point to arrays of size vertexCount.
	The parameter 'smoothingGroups' must point to an array of size vertexCount, or be NULL.  If not NULL, only vertices with equal smoothing groups will be merged.
	Upon return, the map array will be filled in with remapped vertex positions for a new vertex buffer.
	The return value is the number of vertices in the reduced buffer.
	Note: this function does NOT actually create the new vertex buffer.
	*/
	virtual uint32_t				createReductionMap(uint32_t* map, const Vertex* vertices, const uint32_t* smoothingGroups, uint32_t vertexCount,
													   const PxVec3& positionTolerance, float normalTolerance, float UVTolerance) = 0;


	/**
	If set, static data buffers will be deleted after they are used in createRenderResources.
	*/
	virtual void					deleteStaticBuffersAfterUse(bool set)	= 0;

	/**
	Old mesh-building interface follows (DEPRECATED, to be removed by beta release):
	*/

	/* Public access to RenderMeshAsset get methods */

	/// \brief Return the number of submeshes
	virtual uint32_t				getSubmeshCount() const = 0;
	/// \brief Return the number of mesh parts
	virtual uint32_t				getPartCount() const = 0;
	/// \brief Return the name of a submesh
	virtual const char*				getMaterialName(uint32_t submeshIndex) const = 0;
	/// \brief Set the name of a submesh
	virtual void					setMaterialName(uint32_t submeshIndex, const char* name) = 0;
	/// \brief Set the winding order of a submesh
	virtual void					setWindingOrder(uint32_t submeshIndex, RenderCullMode::Enum winding) = 0;
	/// \brief Return the winding order of a submesh
	virtual RenderCullMode::Enum	getWindingOrder(uint32_t submeshIndex) const = 0;
	/// \brief Return a submesh
	virtual const RenderSubmesh&	getSubmesh(uint32_t submeshIndex) const = 0;
	/// \brief Return a mutable submesh
	virtual RenderSubmesh&			getSubmeshWritable(uint32_t submeshIndex) = 0;
	/// \brief Return the bounds of a mesh part
	virtual const PxBounds3&		getBounds(uint32_t partIndex = 0) const = 0;
	/// \brief Get the asset statistics
	virtual void					getStats(RenderMeshAssetStats& stats) const = 0;
};


/**
\brief Rendering mesh (data) class.

To render a mesh asset, you must create an instance
*/
class RenderMeshAsset : public Asset
{
public:

	/**
	\brief Instance this asset, return the created RenderMeshActor.

	See RenderMeshActor
	*/
	virtual RenderMeshActor*		createActor(const RenderMeshActorDesc& desc) = 0;

	/**
	\brief Releases an RenderMeshActor instanced by this asset.
	*/
	virtual void					releaseActor(RenderMeshActor&) = 0;

	/**
	\brief Number of submeshes.

	Each part effectively has the same number of submeshes, even if some are empty.
	*/
	virtual uint32_t				getSubmeshCount() const = 0;

	/**
	\brief Number of parts.

	These act as separate meshes, but they share submesh data (like materials).
	*/
	virtual uint32_t				getPartCount() const = 0;

	/**
	\brief Returns an array of length submeshCount()
	*/
	virtual const char*				getMaterialName(uint32_t submeshIndex) const = 0;

	/**
	\brief Returns an submesh

	A submesh contains all the triangles in all parts with the same material
	(indexed by submeshIndex)
	*/
	virtual const RenderSubmesh&	getSubmesh(uint32_t submeshIndex) const = 0;

	/**
	\brief Returns the axis-aligned bounding box of the vertices for the given part.

	Valid range of partIndex is {0..partCount()-1}
	*/
	virtual const PxBounds3&		getBounds(uint32_t partIndex = 0) const = 0;

	/**
	\brief Returns stats (sizes, counts) for the asset.

	See RenderMeshAssetStats.
	*/
	virtual void					getStats(RenderMeshAssetStats& stats) const = 0;

	/**
	\brief Returns opaque mesh resource (NULL when opaque mesh isn't specified)
	*/
	virtual UserOpaqueMesh*			getOpaqueMesh(void) const = 0;

protected:
	virtual							~RenderMeshAsset() {}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // RENDER_MESH_ASSET_H
