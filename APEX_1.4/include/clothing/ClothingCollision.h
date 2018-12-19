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



#ifndef CLOTHING_COLLISION_H
#define CLOTHING_COLLISION_H

/*!
\file
\brief classes ClothingCollision, ClothingPlane, ClothingConvex, ClothingSphere, ClothingCapsule, ClothingTriangleMesh
*/

#include "ApexInterface.h"

namespace nvidia
{

namespace apex
{
PX_PUSH_PACK_DEFAULT


/**
\brief Type of the ClothingCollision
*/
struct ClothingCollisionType
{
	/**
	\brief Enum.
	*/
	enum Enum
	{
		Plane,
		Convex,
		Sphere,
		Capsule,
		TriangleMesh
	};
};



class ClothingPlane;
class ClothingConvex;
class ClothingSphere;
class ClothingCapsule;
class ClothingTriangleMesh;

/**
\brief Base class of all clothing collision types.
*/
class ClothingCollision : public ApexInterface
{
public:
	/**
	\brief Returns the Type of this collision object.
	*/
	virtual ClothingCollisionType::Enum getType() const = 0;

	/**
	\brief Returns the pointer to this object if it is a plane, NULL otherwise.
	*/
	virtual ClothingPlane* isPlane() = 0;

	/**
	\brief Returns the pointer to this object if it is a convex, NULL otherwise.
	*/
	virtual ClothingConvex* isConvex() = 0;

	/**
	\brief Returns the pointer to this object if it is a sphere, NULL otherwise.
	*/
	virtual ClothingSphere* isSphere() = 0;

	/**
	\brief Returns the pointer to this object if it is a capsule, NULL otherwise.
	*/
	virtual ClothingCapsule* isCapsule() = 0;

	/**
	\brief Returns the pointer to this object if it is a triangle mesh, NULL otherwise.
	*/
	virtual ClothingTriangleMesh* isTriangleMesh() = 0;
};



/**
\brief Plane collision of a clothing actor.
*/
class ClothingPlane : public ClothingCollision
{
public:
	/**
	\brief Sets the plane equation of the plane.
	*/
	virtual void setPlane(const PxPlane& plane) = 0;

	/**
	\brief Returns the plane equation of the plane.
	*/
	virtual const PxPlane& getPlane() = 0;

	/**
	\brief Returns the number of convexes that reference this plane.
	*/
	virtual uint32_t getRefCount() const = 0;
};



/**
\brief Convex collision of a clothing actor.

A convex is represented by a list of ClothingPlanes.
*/
class ClothingConvex : public ClothingCollision
{
public:
	/**
	\brief Returns the number of planes that define this convex.
	*/
	virtual uint32_t getNumPlanes() = 0;

	/**
	\brief Returns pointer to the planes that define this convex.
	*/
	virtual ClothingPlane** getPlanes() = 0;

	/**
	\brief Releases this convex and all its planes.

	\note Only use this if the planes are used exclusively by this convex. Planes referenced by other convexes will not be released.
	*/
	virtual void releaseWithPlanes() = 0;
};



/**
\brief Sphere collision of a clothing actor.
*/
class ClothingSphere : public ClothingCollision
{
public:
	/**
	\brief Sets the position of this sphere.
	*/
	virtual void setPosition(const PxVec3& position) = 0;

	/**
	\brief Returns the position of this sphere.
	*/
	virtual const PxVec3& getPosition() const = 0;

	/**
	\brief Sets the radius of this sphere.
	*/

	/**
	\brief Sets the Radius of this sphere.
	*/
	virtual void setRadius(float radius) = 0;

	/**
	\brief Returns the Radius of this sphere.
	*/
	virtual float getRadius() const = 0;

	/**
	\brief Returns the number of capsules that reference this plane.
	*/
	virtual uint32_t getRefCount() const = 0;
};



/**
\brief Capsule collision of a clothing actor.

A capsule is represented by two ClothingSpheres.
*/
class ClothingCapsule : public ClothingCollision
{
public:
	/**
	\brief Returns the pointers to the two spheres that represent this capsule.
	*/
	virtual ClothingSphere** getSpheres() = 0;

	/**
	\brief Releases this sphere and its two spheres.

	\note Only use releaseWithSpheres if the 2 spheres are used exclusively by this capsule. Spheres referenced by other convexes will not be released.
	*/
	virtual void releaseWithSpheres() = 0;
};



/**
\brief Triangle mesh collision of a clothing actor.
*/
class ClothingTriangleMesh : public ClothingCollision
{
public:
	/**
	\brief Read access to the current triangles of the mesh.
	unlockTriangles needs to be called when done reading.
	\param[out]	ids			Pointer to the triangle ids. Not written if NULL is provided.
	\param[out]	triangles	Pointer to the triangles. Contains (3*number of triangles) vertex positions. Not written if NULL is provided.
	\return					Current number of triangles
	*/
	virtual uint32_t lockTriangles(const uint32_t** ids, const PxVec3** triangles) = 0;

	/**
	\brief Write access to the current triangles of the mesh. Changing Ids is not allowed.
	unlockTriangles needs to be called when done editing.
	\param[out]	ids			pointer to the triangle ids. not written if NULL is provided.
	\param[out]	triangles	pointer to the triangles. Contains (3*number of triangles) vertex positions. Not written if NULL is provided.
	\return					Current number of triangles
	*/
	virtual uint32_t lockTrianglesWrite(const uint32_t** ids, PxVec3** triangles) = 0;

	/**
	\brief Unlock the mesh data after editing or reading.
	*/
	virtual void unlockTriangles() = 0;

	/**
	\brief Adds a triangle to the mesh. Clockwise winding.
	If a triangle with a given id already exists, it will update that triangle with the new values.
	Ids for triangle removal and adding are accumulated and processed during simulate. That means a triangle
	will not be added if the same id is removed in the same frame, independent of the order of the remove and add calls.
	\param[in]	id					User provided triangle id that allows identifing and removing the triangle.
	\param[in]	v0					First vertex of triangle
	\param[in]	v1					Second vertex of triangle
	\param[in]	v2					Third vertex of triangle
	*/
	virtual void addTriangle(uint32_t id, const PxVec3& v0, const PxVec3& v1, const PxVec3& v2) = 0;

	/**
	\brief Adds a list of triangles to the mesh. Clockwise winding.
	If a triangle with a given id already exists, it will update that triangle with the new values.
	Ids for triangle removal and adding are accumulated and processed during simulate. That means a triangle
	will not be added if the same id is removed in the same frame, independent of the order of the remove and add calls.
	\param[in]	ids					User provided triangle indices that allows identifing and removing the triangle.
									Needs to contain numTriangles indices.
	\param[in]	triangleVertices	Triangle vertices to add. Needs to contain 3*numTriangles vertex positions
	\param[in]	numTriangles		Number of triangles to add
	*/
	virtual void addTriangles(const uint32_t* ids, const PxVec3* triangleVertices, uint32_t numTriangles) = 0;

	/**
	\brief Removes triangle with given id.
	Ids for triangle removal and adding are accumulated and processed during simulate. That means a triangle
	will not be added if the same id is removed in the same frame, independent of the order of the remove and add calls.
	\param[in]	id		id of the triangle to be removed
	*/
	virtual void removeTriangle(uint32_t id) = 0;

	/**
	\brief Removes triangles.
	Ids for triangle removal and adding are accumulated and processed during simulate. That means a triangle
	will not be added if the same id is removed in the same frame, independent of the order of the remove and add calls.
	\param[in]	ids					ids of the triangles to be removed. Needs to contain numTriangle indices.
	\param[in]	numTriangles		Number of triangles to remove
	*/
	virtual void removeTriangles(const uint32_t* ids, uint32_t numTriangles) = 0;

	/**
	\brief Clear all triangles to start with an empty mesh.
	*/
	virtual void clearTriangles() = 0;

	/**
	\brief Sets the global pose of the mesh.
	*/
	virtual void setPose(PxMat44 pose) = 0;

	/**
	\brief Returns the global pose of the mesh.
	*/
	virtual const PxMat44& getPose() const = 0;
};



PX_POP_PACK
} // namespace nvidia
} // namespace nvidia


#endif // CLOTHING_COLLISION_H