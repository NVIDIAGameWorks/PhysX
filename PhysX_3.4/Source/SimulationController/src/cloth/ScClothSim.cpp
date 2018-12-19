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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#include "ScClothSim.h"
#if PX_USE_CLOTH_API

#include "PxClothParticleData.h"
#include "PxConvexMesh.h"
#include "PxTriangleMesh.h"
#include "PxHeightField.h"
#include "PxHeightFieldSample.h"

#include "ScPhysics.h"
#include "ScScene.h"
#include "ScClothCore.h"
#include "ScShapeSim.h"

#include "CmScaling.h"

#include "Types.h"
#include "Range.h"
#include "Factory.h"
#include "Cloth.h"

#include "GuIntersectionTriangleBox.h"
#include "ScNPhaseCore.h"
#include "PsFoundation.h"

using namespace physx;

namespace 
{
	template <typename D, typename S>
	PX_FORCE_INLINE cloth::Range<D> createRange(S begin, PxU32 size)
	{
		D* start = reinterpret_cast<D*>(begin);
		D* end = start + size;

		return cloth::Range<D>(start, end);
	}
}

Sc::ClothSim::ClothSim(Scene& scene, ClothCore& core) : 
    ActorSim(scene, core),
    mClothShape(*this),
	mNumSpheres(0),
	mNumCapsules(0),
	mNumPlanes(0),
	mNumBoxes(0),
	mNumConvexes(0),
	mNumMeshes(0),
	mNumHeightfields(0),
	mNumConvexPlanes(0)
{
	startStep(); // sync lowlevel gravity to prevent waking up on simulate()
}


Sc::ClothSim::~ClothSim()
{
	getCore().setSim(NULL);
}


Sc::ClothCore& Sc::ClothSim::getCore() const
{
	return static_cast<ClothCore&>(getActorCore());
}

void Sc::ClothSim::updateBounds()
{
	mClothShape.updateBoundsInAABBMgr();
}

void Sc::ClothSim::startStep()
{
    updateRigidBodyPositions();

	// update total external acceleration in LL
	PxVec3 externalAcceleration = getCore().getExternalAcceleration();
	if ((getActorCore().getActorFlags() & PxActorFlag::eDISABLE_GRAVITY) == false)
		externalAcceleration += getScene().getGravity();
	getCore().getLowLevelCloth()->setGravity(externalAcceleration);
}


void Sc::ClothSim::reinsert()
{
	Sc::Scene& scene = getScene();
	Sc::ClothCore& core = getCore();

	scene.removeCloth(core);
	scene.addCloth(core);
}

bool Sc::ClothSim::addCollisionShape(const ShapeSim* shape)
{
	switch (shape->getGeometryType())
	{
	case PxGeometryType::eSPHERE:
		return addCollisionSphere(shape);
	case PxGeometryType::ePLANE:
		return addCollisionPlane(shape);
	case PxGeometryType::eCAPSULE:
		return addCollisionCapsule(shape);
	case PxGeometryType::eBOX:
		return addCollisionBox(shape);
	case PxGeometryType::eCONVEXMESH:
		return addCollisionConvex(shape);
	case PxGeometryType::eTRIANGLEMESH:
		return addCollisionMesh(shape);
	case PxGeometryType::eHEIGHTFIELD:
		return addCollisionHeightfield(shape);
	case PxGeometryType::eGEOMETRY_COUNT:
	case PxGeometryType::eINVALID:
		break;
	}
	return false;
}

void Sc::ClothSim::removeCollisionShape(const ShapeSim* shape)
{
	switch (shape->getGeometryType())
	{
	case PxGeometryType::eSPHERE:
		removeCollisionSphere(shape);
		break;
	case PxGeometryType::ePLANE:
		removeCollisionPlane(shape);
		break;
	case PxGeometryType::eCAPSULE:
		removeCollisionCapsule(shape);
		break;
	case PxGeometryType::eBOX:
		removeCollisionBox(shape);
		break;
	case PxGeometryType::eCONVEXMESH:
		removeCollisionConvex(shape);
		break;
	case PxGeometryType::eTRIANGLEMESH:
		removeCollisionMesh(shape);
		break;
	case PxGeometryType::eHEIGHTFIELD:
		removeCollisionHeightfield(shape);
		break;
	case PxGeometryType::eGEOMETRY_COUNT:
	case PxGeometryType::eINVALID:
		break;
	}
}

bool Sc::ClothSim::addCollisionSphere(const ShapeSim* shape)
{
	PxU32 startIndex = 0;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumSpheres; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	PxU32 sphereIndex = core.mNumUserSpheres + mNumSpheres;
	if(sphereIndex >= 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision sphere due to 32 sphere limit");
		return false;
	}

	// current position here is before simulation
	const PxSphereGeometry& geometry = static_cast<const PxSphereGeometry&>(shape->getCore().getGeometry());

	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);

	PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
	PxVec3 center = trafo.p;
	PxVec4 sphere(center, geometry.radius);

	core.getLowLevelCloth()->setSpheres(createRange<const PxVec4>(&sphere, 1), sphereIndex, sphereIndex);

	insertShapeSim(startIndex + mNumSpheres++, shape);
	return true;
}

void Sc::ClothSim::removeCollisionSphere(const ShapeSim* shape)
{
	ClothCore& core = getCore();

	PxU32 startIndex = 0;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumSpheres; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			--mNumSpheres;
			PxU32 sphereIndex = core.mNumUserSpheres + index;
			core.getLowLevelCloth()->setSpheres(cloth::Range<const PxVec4>(), sphereIndex, sphereIndex+1);
			break;
		}
	}
}

bool Sc::ClothSim::addCollisionCapsule( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumCapsules; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	PxU32 capsuleIndex = core.mNumUserCapsules + mNumCapsules;
	if(capsuleIndex >= 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision capsule due to 32 capsule limit");
		return false;
	}

	PxU32 sphereIndex = core.mNumUserSpheres + mNumSpheres + 2*mNumCapsules;
	if(sphereIndex >= 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision capsule due to 32 sphere limit");
		return false;
	}

	// current position here is before simulation
	const PxCapsuleGeometry& geometry = static_cast<const PxCapsuleGeometry&>(shape->getCore().getGeometry());

	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);

	PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
	PxVec3 center = trafo.p;
	PxVec3 direction = trafo.q.rotate(PxVec3(geometry.halfHeight, 0, 0));
	PxReal radius = geometry.radius;
	PxVec4 spheres[2] = { PxVec4(center-direction, radius), PxVec4(center+direction, radius) } ;

	core.getLowLevelCloth()->setSpheres(createRange<const PxVec4>(&spheres, 2), sphereIndex, sphereIndex);
	PxU32 indices[2] = { sphereIndex, sphereIndex+1 };
	core.getLowLevelCloth()->setCapsules(createRange<PxU32>(indices, 2), capsuleIndex, capsuleIndex);

	insertShapeSim(startIndex + mNumCapsules++, shape);
	return true;
}

void Sc::ClothSim::removeCollisionCapsule( const ShapeSim* shape )
{
	ClothCore& core = getCore();

	PxU32 startIndex = mNumSpheres;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumCapsules; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			--mNumCapsules;
			PxU32 sphereIndex = core.mNumUserSpheres + mNumSpheres + 2*index;
			core.getLowLevelCloth()->setSpheres(cloth::Range<const PxVec4>(), sphereIndex, sphereIndex+2);
			// capsule is being removed automatically
			break;
		}
	}
}

bool Sc::ClothSim::addCollisionPlane( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumPlanes; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	PxU32 planeIndex = core.mNumUserPlanes + mNumPlanes;
	if(planeIndex >= 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision plane due to 32 plane limit");
		return false;
	}

	// current position here is before simulation
	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);

	PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
	PxPlane plane = PxPlaneEquationFromTransform(trafo);

	const PxVec4* data = reinterpret_cast<const PxVec4*>(&plane);
	core.getLowLevelCloth()->setPlanes(createRange<const PxVec4>(data, 1), planeIndex, planeIndex);
	PxU32 convexIndex = core.mNumUserConvexes + mNumPlanes;
	PxU32 convexMask = PxU32(0x1 << planeIndex);
	core.getLowLevelCloth()->setConvexes(createRange<const PxU32>(&convexMask, 1), convexIndex, convexIndex);

	insertShapeSim(startIndex + mNumPlanes++, shape);
	return true;
}

void Sc::ClothSim::removeCollisionPlane( const ShapeSim* shape )
{
	ClothCore& core = getCore();

	PxU32 startIndex = mNumSpheres + mNumCapsules;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumPlanes; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			--mNumPlanes;
			PxU32 planeIndex = core.mNumUserPlanes + index;
			core.getLowLevelCloth()->setPlanes(cloth::Range<const PxVec4>(), planeIndex, planeIndex+1);
			// convex is being removed automatically
			break;
		}
	}
}

bool Sc::ClothSim::addCollisionBox( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumBoxes; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	PxU32 planeIndex = core.mNumUserPlanes + mNumPlanes + 6*mNumBoxes;
	if(planeIndex+6 > 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision box due to 32 plane limit");
		return false;
	}

	// current position here is before simulation
	const PxBoxGeometry& geometry = static_cast<const PxBoxGeometry&>(shape->getCore().getGeometry());
	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);
	PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
	PxVec3 halfExtents = geometry.halfExtents;
	PxPlane planes[6] = { 
		trafo.transform(PxPlane(PxVec3( 1.f,  0,  0), -halfExtents.x)),
		trafo.transform(PxPlane(PxVec3(-1.f,  0,  0), -halfExtents.x)),
		trafo.transform(PxPlane(PxVec3( 0,  1.f,  0), -halfExtents.y)),
		trafo.transform(PxPlane(PxVec3( 0, -1.f,  0), -halfExtents.y)),
		trafo.transform(PxPlane(PxVec3( 0,  0,  1.f), -halfExtents.z)),
		trafo.transform(PxPlane(PxVec3( 0,  0, -1.f), -halfExtents.z)),
	};

	core.getLowLevelCloth()->setPlanes(createRange<const PxVec4>(&planes, 6), planeIndex, planeIndex);
	PxU32 convexIndex = core.mNumUserConvexes + mNumPlanes + mNumBoxes;
	PxU32 convexMask = PxU32(0x3f << planeIndex);
	core.getLowLevelCloth()->setConvexes(createRange<const PxU32>(&convexMask, 1), convexIndex, convexIndex);

	insertShapeSim(startIndex + mNumBoxes++, shape);
	return true;
}

void Sc::ClothSim::removeCollisionBox( const ShapeSim* shape )
{
	ClothCore& core = getCore();

	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumBoxes; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			--mNumBoxes;
			PxU32 planeIndex = core.mNumUserPlanes + mNumPlanes + 6*index;
			core.getLowLevelCloth()->setPlanes(cloth::Range<const PxVec4>(), planeIndex, planeIndex+6);
			// convex is being removed automatically
			break;
		}
	}
}

bool Sc::ClothSim::addCollisionConvex( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumConvexes; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	const PxConvexMeshGeometry& geometry = static_cast<const PxConvexMeshGeometry&>(shape->getCore().getGeometry());
	PxU32 numPlanes = geometry.convexMesh->getNbPolygons();

	PxU32 planeIndex = core.mNumUserPlanes + mNumPlanes + 6*mNumBoxes + mNumConvexPlanes;
	if(planeIndex+numPlanes > 32)
	{
		Ps::getFoundation().error(PX_WARN, "Dropping collision convex due to 32 plane limit");
		return false;
	}

	// current position here is before simulation
	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);
	Cm::Matrix34 trafo = core.getGlobalPose().transformInv(globalPose) * geometry.scale;
	Ps::Array<PxPlane> planes;
	planes.reserve(numPlanes);

	for(PxU32 i=0; i<numPlanes; ++i)
	{
		PxHullPolygon polygon;
		geometry.convexMesh->getPolygonData(i, polygon);
		PxVec3 normal = trafo.rotate(reinterpret_cast<const PxVec3&>(polygon.mPlane));
		planes.pushBack(PxPlane(normal, polygon.mPlane[3] - trafo.p.dot(normal)));
	}

	core.getLowLevelCloth()->setPlanes(createRange<const PxVec4>(planes.begin(), numPlanes), planeIndex, planeIndex);
	PxU32 convexIndex = core.mNumUserConvexes + mNumPlanes + mNumBoxes + mNumConvexes;
	PxU32 convexMask = PxU32(((1 << numPlanes) - 1) << planeIndex);
	core.getLowLevelCloth()->setConvexes(createRange<const PxU32>(&convexMask, 1), convexIndex, convexIndex);

	mNumConvexPlanes += numPlanes;
	insertShapeSim(startIndex + mNumConvexes++, shape);
	return true;
}

void Sc::ClothSim::removeCollisionConvex( const ShapeSim* shape )
{
	ClothCore& core = getCore();

	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	PxU32 planeIndex = core.mNumUserPlanes + mNumPlanes + 6*mNumBoxes;
	for (PxU32 index = 0; index<mNumConvexes; ++index)
	{
		const PxConvexMeshGeometry& geometry = static_cast<const PxConvexMeshGeometry&>(shapeIt[index]->getCore().getGeometry());
		PxU32 numPlanes = geometry.convexMesh->getNbPolygons();
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			--mNumConvexes;
			core.getLowLevelCloth()->setPlanes(cloth::Range<const PxVec4>(), planeIndex, planeIndex+numPlanes);
			mNumConvexPlanes -= numPlanes;
			// convex is being removed automatically
			break;
		}
		planeIndex += numPlanes;
	}
}

namespace
{
	template <typename IndexT>
	void gatherMeshVertices(const PxTriangleMesh& mesh, Ps::Array<PxVec3>& vertices, bool flipNormal)
	{
		PxU32 numTriangles = mesh.getNbTriangles();
		const IndexT* indices = reinterpret_cast<const IndexT*>(mesh.getTriangles());
		const PxVec3* meshVertices = mesh.getVertices();
		for(PxU32 i=0; i<numTriangles; ++i)
		{
			const PxI32 winding = flipNormal ? 1 : 0;
			vertices.pushBack(meshVertices[indices[i*3+0]]);
			vertices.pushBack(meshVertices[indices[i*3+1 + winding]]);
			vertices.pushBack(meshVertices[indices[i*3+2 - winding]]);
		}
	}

	void offsetTriangles(PxVec3* it, PxVec3* end, PxReal offset)
	{
		for(; it < end; it += 3)
		{
			PxVec3 v0 = it[0];
			PxVec3 v1 = it[1];
			PxVec3 v2 = it[2];

			PxVec3 n = (v1-v0).cross(v2-v0).getNormalized() * offset;

			it[0] = v0 + n;
			it[1] = v1 + n;
			it[2] = v2 + n;
		}
	}

	void transform(const Cm::Matrix34& trafo, PxVec3* first, PxVec3* last)
	{
		for(; first < last; ++first)
			*first = trafo.transform(*first);
	}
}

bool Sc::ClothSim::addCollisionMesh( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes + mNumConvexes;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumMeshes; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	// current position here is before simulation
	const PxTriangleMeshGeometry& geometry = static_cast<const PxTriangleMeshGeometry&>(shape->getCore().getGeometry());
	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);
	Cm::Matrix34 trafo = core.getGlobalPose().transformInv(globalPose) * geometry.scale;
	insertShapeSim(startIndex + mNumMeshes++, shape);
	mStartShapeTrafos.pushBack(trafo);
	return true;
}

void Sc::ClothSim::removeCollisionMesh( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes + mNumConvexes;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumMeshes; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			mStartShapeTrafos.remove(index);
			--mNumMeshes;
			break;
		}
	}
}

namespace
{
	void gatherHeightfieldSamples(const PxHeightField& hf, Ps::Array<PxVec3>& vertices, Ps::Array<PxHeightFieldSample>& samples)
	{
		const PxU32 numCols = hf.getNbColumns();
		const PxU32 numRows = hf.getNbRows();
		const PxU32 numVertices = numRows * numCols;	

		samples.resize(numVertices);
		hf.saveCells(samples.begin(), numVertices * sizeof(PxHeightFieldSample));

		vertices.reserve(numVertices);
		for(PxU32 i = 0; i < numRows; ++i) 
		{
			for(PxU32 j = 0; j < numCols; ++j) 
			{
				vertices.pushBack(PxVec3(PxReal(i), PxReal(samples[j + (i*numCols)].height), PxReal(j)));
			}
		}
	}

	void tessellateHeightfield(const PxHeightField& hf, const PxVec3* vertices, 
		const PxHeightFieldSample* samples, Ps::Array<PxVec3>& triangles)
	{
		const PxU32 numCols = hf.getNbColumns();
		const PxU32 numRows = hf.getNbRows();
		const PxU32 numTriangles = (numRows-1) * (numCols-1) * 2;

		triangles.reserve(triangles.size() + numTriangles*3);
		for(PxU32 i = 0; i < (numCols - 1); ++i) 
		{
			for(PxU32 j = 0; j < (numRows - 1); ++j) 
			{
				PxU8 tessFlag = samples[i+j*numCols].tessFlag();

				// i2--i3
				// |    |
				// i0--i1
				PxU32 i0 =   i   * numRows + j;
				PxU32 i1 =   i   * numRows + j + 1;
				PxU32 i2 = (i+1) * numRows + j;
				PxU32 i3 = (i+1) * numRows + j+1;

				// this is really a corner vertex index, not triangle index
				PxU32 mat0 = hf.getTriangleMaterialIndex((j*numCols+i)*2);
				if(mat0 != PxHeightFieldMaterial::eHOLE)
				{
					triangles.pushBack(vertices[i2]);
					triangles.pushBack(vertices[i0]);
					triangles.pushBack(vertices[tessFlag ? i3 : i1]);
				}

				PxU32 mat1 = hf.getTriangleMaterialIndex((j*numCols+i)*2+1);
				if(mat1 != PxHeightFieldMaterial::eHOLE)
				{
					triangles.pushBack(vertices[i1]);
					triangles.pushBack(vertices[i3]);
					triangles.pushBack(vertices[tessFlag ? i0 : i2]);
				}

				// we don't handle holes yet (number of triangles < expected)
				PX_ASSERT(mat0 != PxHeightFieldMaterial::eHOLE);
				PX_ASSERT(mat1 != PxHeightFieldMaterial::eHOLE);
			}
		}
	}
}


bool Sc::ClothSim::addCollisionHeightfield( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes + mNumConvexes + mNumMeshes;

#if PX_DEBUG
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumHeightfields; ++index)
		PX_ASSERT(shapeIt[index] != shape);
#endif

	ClothCore& core = getCore();

	// current position here is before simulation
	const PxHeightFieldGeometry& geometry = static_cast<const PxHeightFieldGeometry&>(shape->getCore().getGeometry());
	PX_ALIGN(16, PxTransform globalPose);
	shape->getAbsPoseAligned(&globalPose);
	Cm::Matrix34 trafo(core.getGlobalPose().transformInv(globalPose));

	trafo.m.column0 *= geometry.rowScale;
	trafo.m.column1 *= geometry.heightScale;
	trafo.m.column2 *= geometry.columnScale;

	insertShapeSim(startIndex + mNumHeightfields++, shape);
	mStartShapeTrafos.pushBack(trafo);
	return true;
}

void Sc::ClothSim::removeCollisionHeightfield( const ShapeSim* shape )
{
	PxU32 startIndex = mNumSpheres + mNumCapsules + mNumPlanes + mNumBoxes + mNumConvexes + mNumMeshes;
	const ShapeSim* const* shapeIt = mShapeSims.begin() + startIndex;
	for (PxU32 index = 0; index<mNumHeightfields; ++index)
	{
		if (shapeIt[index] == shape)
		{
			mShapeSims.remove(startIndex + index);
			mStartShapeTrafos.remove(mNumMeshes + index);
			--mNumHeightfields;
			break;
		}
	}
}

void Sc::ClothSim::updateRigidBodyPositions()
{
	ClothCore& core = getCore();

	if(!(core.getClothFlags() & PxClothFlag::eSCENE_COLLISION))
	{
		PX_ASSERT(0 == mNumSpheres + mNumCapsules + mNumPlanes + 
			mNumBoxes + mNumConvexes + mNumMeshes + mNumHeightfields);
		return;
	}

	PxReal restOffset = core.getRestOffset();

	const ShapeSim* const* shapeIt = mShapeSims.begin();

	Ps::Array<PxVec4> spheres;
	for (PxU32 i=0; i<mNumSpheres; ++i, ++shapeIt)
	{
		// current position here is after simulation
		const ShapeSim* shape = *shapeIt;
		const PxSphereGeometry& geometry = static_cast<const PxSphereGeometry&>(shape->getCore().getGeometry());
		PX_ALIGN(16, PxTransform globalPose);
		shape->getAbsPoseAligned(&globalPose);
		PxVec3 p = core.getGlobalPose().transformInv(globalPose.p);
		spheres.pushBack(PxVec4(p, geometry.radius + restOffset));
	}

	for (PxU32 i=0; i<mNumCapsules; ++i, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		const PxCapsuleGeometry& geometry = static_cast<const PxCapsuleGeometry&>(shape->getCore().getGeometry());
		PX_ALIGN(16, PxTransform globalPose);
		shape->getAbsPoseAligned(&globalPose);
		PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
		PxVec3 center = trafo.p;
		PxVec3 direction = trafo.q.rotate(PxVec3(geometry.halfHeight, 0, 0));
		PxReal radius = geometry.radius + restOffset;
		spheres.pushBack(PxVec4(center-direction, radius));
		spheres.pushBack(PxVec4(center+direction, radius));
	}

	PxU32 sphereIndex = core.mNumUserSpheres, numSpheres = spheres.size();
	core.getLowLevelCloth()->setSpheres(createRange<const PxVec4>(spheres.begin(), numSpheres), sphereIndex, sphereIndex+numSpheres);

	Ps::Array<PxPlane> planes;
	for (PxU32 i = 0; i<mNumPlanes; ++i, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		PX_ALIGN(16, PxTransform globalPose);
		shape->getAbsPoseAligned(&globalPose);
		PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
		PxPlane plane = PxPlaneEquationFromTransform(trafo);
		plane.d -= restOffset;
		planes.pushBack(plane);
	}

	for (PxU32 i = 0; i<mNumBoxes; ++i, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		const PxBoxGeometry& geometry = static_cast<const PxBoxGeometry&>(shape->getCore().getGeometry());
		PX_ALIGN(16, PxTransform globalPose);
		shape->getAbsPoseAligned(&globalPose);
		PxTransform trafo = core.getGlobalPose().transformInv(globalPose);
		PxVec3 halfExtents = geometry.halfExtents + PxVec3(restOffset);
		planes.pushBack(trafo.transform(PxPlane(PxVec3( 1.f,  0,  0), -halfExtents.x)));
		planes.pushBack(trafo.transform(PxPlane(PxVec3(-1.f,  0,  0), -halfExtents.x)));
		planes.pushBack(trafo.transform(PxPlane(PxVec3( 0,  1.f,  0), -halfExtents.y)));
		planes.pushBack(trafo.transform(PxPlane(PxVec3( 0, -1.f,  0), -halfExtents.y)));
		planes.pushBack(trafo.transform(PxPlane(PxVec3( 0,  0,  1.f), -halfExtents.z)));
		planes.pushBack(trafo.transform(PxPlane(PxVec3( 0,  0, -1.f), -halfExtents.z)));
	}

	for (PxU32 j= 0; j<mNumConvexes; ++j, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		const PxConvexMeshGeometry& geometry = static_cast<const PxConvexMeshGeometry&>(shape->getCore().getGeometry());
		PX_ALIGN(16, PxTransform globalPose);
		shape->getAbsPoseAligned(&globalPose);
		Cm::Matrix34 trafo = core.getGlobalPose().transformInv(globalPose) * geometry.scale;
		PxU32 numPlanes = geometry.convexMesh->getNbPolygons();
		for(PxU32 i=0; i<numPlanes; ++i)
		{
			PxHullPolygon polygon;
			geometry.convexMesh->getPolygonData(i, polygon);
			PxVec3 normal = trafo.rotate(reinterpret_cast<const PxVec3&>(polygon.mPlane));
			planes.pushBack(PxPlane(normal, polygon.mPlane[3] - trafo.p.dot(normal) - restOffset));
		}
	}

	PxU32 planeIndex = core.mNumUserPlanes, numPlanes = planes.size();
	core.getLowLevelCloth()->setPlanes(createRange<const PxVec4>(planes.begin(), numPlanes), planeIndex, planeIndex+numPlanes);

	Ps::Array<PxVec3> curTriangles, prevTriangles;
	for (PxU32 i = 0; i<mNumMeshes; ++i, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		const PxTriangleMeshGeometry& geometry = static_cast<const PxTriangleMeshGeometry&>(shape->getCore().getGeometry());
		PxTransform clothPose = core.getGlobalPose();
		PX_ALIGN(16, PxTransform meshPose);
		shape->getAbsPoseAligned(&meshPose);
		Cm::Matrix34 trafo = clothPose.transformInv(meshPose) * geometry.scale;

		PxU32 start = curTriangles.size();
		if(geometry.triangleMesh->getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES)
			gatherMeshVertices<PxU16>(*geometry.triangleMesh, curTriangles, geometry.scale.hasNegativeDeterminant());
		else
			gatherMeshVertices<PxU32>(*geometry.triangleMesh, curTriangles, geometry.scale.hasNegativeDeterminant());

		Cm::Matrix34 startTrafo = mStartShapeTrafos[i];
		mStartShapeTrafos[i] = trafo;
		for(PxU32 j=start, n=curTriangles.size(); j<n; ++j)
			prevTriangles.pushBack(startTrafo.transform(curTriangles[j]));

		transform(trafo, curTriangles.begin() + start, curTriangles.end());
	}

	Ps::Array<PxVec3> vertices;
	Ps::Array<PxHeightFieldSample> samples;
	for (PxU32 i = 0; i<mNumHeightfields; ++i, ++shapeIt)
	{
		const ShapeSim* shape = *shapeIt;
		const PxHeightFieldGeometry& geometry = static_cast<const PxHeightFieldGeometry&>(shape->getCore().getGeometry());
		PxTransform clothPose = core.getGlobalPose();
		PX_ALIGN(16, PxTransform heightfieldPose);
		shape->getAbsPoseAligned(&heightfieldPose);
		Cm::Matrix34 trafo = Cm::Matrix34(clothPose.transformInv(heightfieldPose));

		trafo.m.column0 *= geometry.rowScale;
		trafo.m.column1 *= geometry.heightScale;
		trafo.m.column2 *= geometry.columnScale;
		
		gatherHeightfieldSamples(*geometry.heightField, vertices, samples);

		PxU32 start = curTriangles.size();
		tessellateHeightfield(*geometry.heightField, vertices.begin(), samples.begin(), curTriangles);

		Cm::Matrix34 startTrafo = mStartShapeTrafos[mNumMeshes + i];
		mStartShapeTrafos[mNumMeshes + i] = trafo;
		for(PxU32 j=start, n=curTriangles.size(); j<n; ++j)
			prevTriangles.pushBack(startTrafo.transform(curTriangles[j]));

		transform(trafo, curTriangles.begin() + start, curTriangles.end());

		vertices.resize(0);
		samples.resize(0);
	}

	// cull triangles that don't intersect the cloth bounding box
	PxVec3 bboxCenter = core.getLowLevelCloth()->getBoundingBoxCenter();
	PxVec3 bboxScale = core.getLowLevelCloth()->getBoundingBoxScale() + PxVec3(core.getContactOffset());
	PxU32 trianglesSize = curTriangles.size(), numVertices = 0;
	for(PxU32 i=0; i<trianglesSize; i+=3)
	{
		// PT: TODO: change the code so that we can safely call the (faster) intersectTriangleBox() function
		if (Gu::intersectTriangleBox_ReferenceCode(bboxCenter, bboxScale, curTriangles[i], curTriangles[i+1], curTriangles[i+2]) ||
			Gu::intersectTriangleBox_ReferenceCode(bboxCenter, bboxScale, prevTriangles[i], prevTriangles[i+1], prevTriangles[i+2]))
		{
			curTriangles[numVertices+0] = curTriangles[i+0];
			curTriangles[numVertices+1] = curTriangles[i+1];
			curTriangles[numVertices+2] = curTriangles[i+2];

			prevTriangles[numVertices+0] = prevTriangles[i+0];
			prevTriangles[numVertices+1] = prevTriangles[i+1];
			prevTriangles[numVertices+2] = prevTriangles[i+2];

			numVertices += 3;
		}
	}

	cloth::Range<PxVec3> prevRange(prevTriangles.begin(), prevTriangles.begin() + numVertices);
	cloth::Range<PxVec3> curRange(curTriangles.begin(), curTriangles.begin() + numVertices);

	offsetTriangles(prevRange.begin(), prevRange.end(), restOffset);
	offsetTriangles(curRange.begin(), curRange.end(), restOffset);

	core.getLowLevelCloth()->setTriangles(prevRange, curRange, core.mNumUserTriangles);

	PX_ASSERT(shapeIt == mShapeSims.end());
}

void Sc::ClothSim::clearCollisionShapes()
{
	ClothCore& core = getCore();
	cloth::Cloth* lowLevelCloth = core.getLowLevelCloth();

	lowLevelCloth->setSpheres(cloth::Range<const PxVec4>(), core.mNumUserSpheres, lowLevelCloth->getNumSpheres());
	lowLevelCloth->setPlanes(cloth::Range<const PxVec4>(), core.mNumUserPlanes, lowLevelCloth->getNumPlanes());
	lowLevelCloth->setTriangles(cloth::Range<const PxVec3>(), core.mNumUserTriangles, lowLevelCloth->getNumTriangles());

	mNumSpheres = 0;
	mNumCapsules = 0;
	mNumPlanes = 0;
	mNumBoxes = 0;
	mNumConvexes = 0;
	mNumMeshes = 0;
	mNumHeightfields = 0;
	mNumConvexPlanes = 0;

	NPhaseCore* narrowPhase = getScene().getNPhaseCore();
	for(PxU32 i=0, n=mShapeSims.size(); i<n; ++i)
		narrowPhase->removeClothOverlap(this, mShapeSims[i]);

	mShapeSims.resize(0);
}

void physx::Sc::ClothSim::insertShapeSim( PxU32 index, const ShapeSim* shapeSim)
{
	mShapeSims.pushBack(0);

	for(PxU32 i = mShapeSims.size(); --i > index; )
		mShapeSims[i] = mShapeSims[i-1];

	mShapeSims[index] = shapeSim;
}

#endif	// PX_USE_CLOTH_API
