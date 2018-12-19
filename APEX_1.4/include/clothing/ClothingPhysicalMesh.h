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



#ifndef CLOTHING_PHYSICAL_MESH_H
#define CLOTHING_PHYSICAL_MESH_H

#include "ApexInterface.h"
#include "IProgressListener.h"

namespace nvidia
{
namespace apex
{


/**
\brief Constrain coefficients of the physical mesh vertices
*/
struct ClothingConstrainCoefficients
{
	float maxDistance; ///< \brief Maximum distance a vertex is allowed to move
	float collisionSphereRadius; ///< \brief Backstop sphere radius
	float collisionSphereDistance; ///< \brief Backstop sphere distance
};


PX_PUSH_PACK_DEFAULT

/**
\brief Contains the data for handing out statistics about a ClothingPhysicalMesh
*/
struct ClothingPhysicalMeshStats
{
	/// the number of bytes allocated for the physical mesh
	uint32_t	totalBytes;
	/// the number of vertices
	uint32_t	numVertices;
	/// the number of indices
	uint32_t	numIndices;
};


/**
\brief Holder for a physical mesh, this can be generated through various ways (see single- and multi-layered clothing) or hand crafted
*/
class ClothingPhysicalMesh : public ApexInterface
{
public:
	/**
	\brief returns the number of vertices
	*/
	virtual uint32_t getNumVertices() const = 0;

	/**
	\brief returns the number of simulated vertices
	*/
	virtual uint32_t getNumSimulatedVertices() const = 0;

	/**
	\brief returns the number of vertices with zero max distance
	*/
	virtual uint32_t getNumMaxDistance0Vertices() const = 0;

	/**
	\brief returns the number of indices
	*/
	virtual uint32_t getNumIndices() const = 0;

	/**
	\brief returns the number of simulated indices
	*/
	virtual uint32_t getNumSimulatedIndices() const = 0;

	/**
	\brief writes the indices to a destination buffer

	\param [out] indexDestination	destination buffer where to write the indices
	\param [in] byteStride			stride of the destination buffer
	\param [in] numIndices			number of indices the buffer can hold. This can be smaller than getNumIndices()
	*/
	virtual void getIndices(void* indexDestination, uint32_t byteStride, uint32_t numIndices) const = 0;

	/**
	\brief returns whether the mesh is built out of tetrahedra or triangles
	*/
	virtual bool isTetrahedralMesh() const = 0;

	/**
	\brief This will simplify the current mesh.

	\param [in] subdivisions	used to derive the maximal length a new edge can get.<br>
								Divide the bounding box diagonal by this value to get the maximal edge length for newly created edges<br>
								Use 0 to not restrict the maximal edge length
	\param [in] maxSteps		The maximum number of edges to be considered for simplification.<br>
								Use 0 to turn off
	\param [in] maxError		The maximal quadric error an edge can cause to be considered simplifyable.<br>
								Use any value < 0 to turn off
	\param [in] progress		Callback class that will be fired every now and then to update a progress bar in the gui.
	\return The number of edges collapsed
	*/
	virtual void simplify(uint32_t subdivisions, int32_t maxSteps, float maxError, IProgressListener* progress) = 0;

	/**
	\brief Create a physical mesh from scratch

	Overwrites all vertices/indices, and invalidates all misc vertex buffers. vertices must be PxVec3 and indices uint32_t.
	If driveChannels is NULL, all vertices are assigned to all drive channels (initialized to 0xffffffff)
	*/
	virtual void setGeometry(bool tetraMesh, uint32_t numVertices, uint32_t vertexByteStride, const void* vertices,	const uint32_t* driveChannels, 
							uint32_t numIndices, uint32_t indexByteStride, const void* indices) = 0;

	// direct access to specific buffers
	/**
	\brief writes the indices into a user specified buffer.

	Returns false if the buffer doesn't exist.
	The buffer must be bigger than sizeof(uint32_t) * getNumIndices()
	*/
	virtual bool getIndices(uint32_t* indices, uint32_t byteStride) const = 0;

	/**
	\brief Writes the vertex positions into a user specified buffer.

	Returns false if the buffer doesn't exist.
	The buffer must be bigger than sizeof(PxVec3) * getNumVertices()
	*/
	virtual bool getVertices(PxVec3* vertices, uint32_t byteStride) const = 0;

	/**
	\brief Writes the normals into a user specified buffer.

	Returns false if the buffer doesn't exist.
	The buffer must be bigger than sizeof(PxVec3) * getNumVertices()
	*/
	virtual bool getNormals(PxVec3* normals, uint32_t byteStride) const = 0;

	/**
	\brief Returns the number of bones per vertex.
	*/
	virtual uint32_t getNumBonesPerVertex() const = 0;

	/**
	\brief Writes the bone indices into a user specified buffer.

	Returns false if the buffer doesn't exist.
	The buffer must be bigger than sizeof(uint16_t) * getNumVertices() * getNumBonesPerVertex().
	(numBonesPerVertex is the same as in the graphical mesh for LOD 0)

	The bytestride is applied only after writing numBonesPerVertex and thus must be >= sizoef(uint16_t) * numBonesPerVertex
	*/
	virtual bool getBoneIndices(uint16_t* boneIndices, uint32_t byteStride) const = 0;

	/**
	\brief Writes the bone weights into a user specified buffer.

	Returns false if the buffer doesn't exist.
	The buffer must be bigger than sizeof(float) * getNumVertices() * getNumBonesPerVertex().
	(numBonesPerVertex is the same as in the graphical mesh for LOD 0)
	The bytestride is applied only after writing numBonesPerVertex and thus must be >= sizoef(float) * numBonesPerVertex
	*/
	virtual bool getBoneWeights(float* boneWeights, uint32_t byteStride) const = 0;

	/**
	\brief Allocates and initializes the drive channels (master flags) of the physics mesh.

	This allows to set the drive channels directly on the physics mesh.
	*/
	virtual void allocateMasterFlagsBuffer() = 0;

	/**
	\brief Returns the masterFlag buffer pointer in order to edit the values.

	This allows to set values directly on the physics mesh.
	*/
	virtual uint32_t* getMasterFlagsBuffer() = 0;

	/**
	\brief Allocates and initializes the constrain coefficients of the physics mesh.

	This allows to set the constrain coefficients like maxDistance directly on the physics mesh.
	If this is not called by the authoring tool, the constrain coefficients are read from the
	graphical mesh.
	*/
	virtual void allocateConstrainCoefficientBuffer() = 0;

	/**
	\brief Returns the constrain coefficient buffer pointer in order to edit the values.

	This allows to set the constrain coefficients like maxDistance directly on the physics mesh.
	*/
	virtual ClothingConstrainCoefficients* getConstrainCoefficientBuffer() const = 0;

	/**
	\brief Writes the cloth constrain coefficients into a user specified buffer.

	Returns false if the buffer doesn't exist. The buffer must be bigger than sizeof(ClothingConstrainCoefficients) * getNumVertices().
	*/
	virtual bool getConstrainCoefficients(ClothingConstrainCoefficients* coeffs, uint32_t byteStride) const = 0;

	/**
	\brief Returns stats (sizes, counts) for the asset.  See ClothingPhysicalMeshStats.
	*/
	virtual void getStats(ClothingPhysicalMeshStats& stats) const = 0;

};

PX_POP_PACK

}
} // namespace nvidia

#endif // CLOTHING_PHYSICAL_MESH_H
