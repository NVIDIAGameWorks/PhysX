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



#ifndef __EMITTER_GEOM_EXPLICIT_IMPL_H__
#define __EMITTER_GEOM_EXPLICIT_IMPL_H__

#include "EmitterGeomBase.h"
#include "PsUserAllocated.h"
#include "EmitterGeomExplicitParams.h"
#include "ApexRand.h"

namespace NvParameterized
{
class Interface;
};

namespace nvidia
{
namespace emitter
{


class EmitterGeomExplicitImpl : public EmitterGeomExplicit, public EmitterGeomBase
{
	void updateAssetPoints();

public:
	EmitterGeomExplicitImpl(NvParameterized::Interface* params);
	EmitterGeomExplicitImpl();

	virtual ~EmitterGeomExplicitImpl() {}

	/* Asset callable methods */
	EmitterGeom*				getEmitterGeom();
	const EmitterGeomExplicit* isExplicitGeom() const
	{
		return this;
	}
	EmitterType::Enum		getEmitterType() const
	{
		return EmitterType::ET_FILL;
	}
	void						setEmitterType(EmitterType::Enum) {}   // PX_ASSERT(t == EmitterType::ET_FILL);
	void						destroy()
	{
		delete this;
	}

	/* AssetPreview methods */
	void                        drawPreview(float scale, RenderDebugInterface* renderDebug) const;

	/* Actor callable methods */
	void						visualize(const PxTransform& pose, RenderDebugInterface& renderDebug);
	void						computeFillPositions(physx::Array<PxVec3>& positions,
	        physx::Array<PxVec3>& velocities,
	        const PxTransform& pose,
			const PxVec3& scale,
	        float density,
	        PxBounds3& outBounds,
	        QDSRand& rand) const
	{
		computeFillPositions(positions, velocities, NULL, pose, scale, density, outBounds, rand);
	}

	void						computeFillPositions(physx::Array<PxVec3>& positions,
	        physx::Array<PxVec3>& velocities,
			physx::Array<uint32_t>* userDataArrayPtr,
	        const PxTransform&,
			const PxVec3&,
	        float,
	        PxBounds3& outBounds,
	        QDSRand& rand) const;

	/* Stubs */
	float				computeNewlyCoveredVolume(const PxTransform&, const PxTransform&) const
	{
		return 0.0f;
	}
	float				computeEmitterVolume() const
	{
		return 0.0f;
	}
	PxVec3				randomPosInFullVolume(const PxMat44&, QDSRand&) const
	{
		return PxVec3(0.0f, 0.0f, 0.0f);
	}
	PxVec3				randomPosInNewlyCoveredVolume(const PxMat44& , const PxMat44&, QDSRand&) const
	{
		return PxVec3(0.0f, 0.0f, 0.0f);
	}
	bool						isInEmitter(const PxVec3&, const PxMat44&) const
	{
		return false;
	}

	void						resetParticleList()
	{
		mPoints.clear();
		mVelocities.clear();
		mPointsUserData.clear();

		mSpheres.clear();
		mSphereVelocities.clear();

		mEllipsoids.clear();
		mEllipsoidVelocities.clear();
	}

	void						addParticleList(uint32_t count,
	        const PointParams* params,
	        const PxVec3* velocities = 0);

	void						addParticleList(uint32_t count,
	        const PxVec3* positions,
	        const PxVec3* velocities = 0);

	void						addParticleList(uint32_t count,
			const PointListData& data);

	void						addSphereList(uint32_t count,
	        const SphereParams* params,
	        const PxVec3* velocities = 0);

	void						addEllipsoidList(uint32_t count,
	        const EllipsoidParams* params,
	        const PxVec3* velocities = 0);

	void						getParticleList(const PointParams* &params,
	        uint32_t& numPoints,
	        const PxVec3* &velocities,
	        uint32_t& numVelocities) const;

	void						getSphereList(const SphereParams* &params,
	        uint32_t& numSpheres,
	        const PxVec3* &velocities,
	        uint32_t& numVelocities) const;

	void						getEllipsoidList(const EllipsoidParams* &params,
	        uint32_t& numEllipsoids,
	        const PxVec3* &velocities,
	        uint32_t& numVelocities) const;

	uint32_t				getParticleCount() const
	{
		return mPoints.size();
	}
	PxVec3				getParticlePos(uint32_t index) const
	{
		return mPoints[ index ].position;
	}

	uint32_t				getSphereCount() const
	{
		return mSpheres.size();
	}
	PxVec3				getSphereCenter(uint32_t index) const
	{
		return mSpheres[ index ].center;
	}
	float				getSphereRadius(uint32_t index) const
	{
		return mSpheres[ index ].radius;
	}

	uint32_t				getEllipsoidCount() const
	{
		return mEllipsoids.size();
	}
	PxVec3				getEllipsoidCenter(uint32_t index) const
	{
		return mEllipsoids[ index ].center;
	}
	float				getEllipsoidRadius(uint32_t index) const
	{
		return mEllipsoids[ index ].radius;
	}
	PxVec3				getEllipsoidNormal(uint32_t index) const
	{
		return mEllipsoids[ index ].normal;
	}
	float				getEllipsoidPolarRadius(uint32_t index) const
	{
		return mEllipsoids[ index ].polarRadius;
	}

	float				getDistance() const
	{
		return mDistance;
	}

protected:
	mutable QDSRand					mRand;

#	define MAX_COLLISION_SHAPES 5

	struct CollisionList
	{
		uint32_t shapeIndices[MAX_COLLISION_SHAPES];
		uint32_t next;

		PX_INLINE CollisionList(): next(0) {}

		PX_INLINE void pushBack(uint32_t shapeIdx)
		{
			if (next >= MAX_COLLISION_SHAPES)
			{
				PX_ASSERT(0 && "Too many colliding shapes in explicit emitter");
				return;
			}

			shapeIndices[next++] = shapeIdx;
		}
	};

	// Collision table holds indices of shapes which collide with some shape
	// Shape indexing: firstly go spheres, then ellipsoids, then points
	physx::Array<CollisionList>		mCollisions;
	physx::Array<PxBounds3>	mBboxes;

	void							updateCollisions();
	bool							isInside(const PxVec3& x, uint32_t shapeIdx) const;

	void AddParticle(physx::Array<PxVec3>& positions, physx::Array<PxVec3>& velocities,
	                 physx::Array<uint32_t>* userDataArrayPtr,
					 const PxTransform& pose, float cutoff, PxBounds3& outBounds,
	                 const PxVec3& pos, const PxVec3& vel, uint32_t userData,
	                 uint32_t srcShapeIdx,
	                 QDSRand& rand) const;

	physx::Array<SphereParams>		mSpheres;
	physx::Array<PxVec3>		mSphereVelocities;

	physx::Array<EllipsoidParams>	mEllipsoids;
	physx::Array<PxVec3>		mEllipsoidVelocities;

	physx::Array<PointParams>		mPoints;
	physx::Array<PxVec3>		mVelocities;
	physx::Array<uint32_t>		mPointsUserData;

	float					mDistance, mInvDistance;

	EmitterGeomExplicitParams*		mGeomParams;
};

}
} // end namespace nvidia

#endif