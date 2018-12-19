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



#include "Apex.h"
#include "ApexUsingNamespace.h"
#include "PsSort.h"
#include "PsMathUtils.h"
#include "PsHashSet.h"
#include "EmitterAsset.h"
#include "EmitterGeomExplicitImpl.h"
//#include "ApexSharedSerialization.h"
#include "RenderDebugInterface.h"
#include "RenderDebugInterface.h"
#include "ApexPreview.h"
#include "EmitterGeomExplicitParams.h"
#include "ParamArray.h"

namespace nvidia
{
namespace emitter
{

PX_INLINE static void GetNormals(const PxVec3& n, PxVec3& t1, PxVec3& t2)
{
	PxVec3 nabs(PxAbs(n.x), PxAbs(n.y), PxAbs(n.z));

	t1 = nabs.x <= nabs.y && nabs.x <= nabs.z
	     ? PxVec3(0, -n.z, n.y)
	     : nabs.y <= nabs.x && nabs.y <= nabs.z ? PxVec3(-n.z, 0, n.x)
	     : PxVec3(-n.y, n.x, 0);
	t1.normalize();

	t2 = n.cross(t1);
	t2.normalize();
}

EmitterGeomExplicitImpl::EmitterGeomExplicitImpl(NvParameterized::Interface* params)
{
	mGeomParams = (EmitterGeomExplicitParams*)params;

	mDistance = mGeomParams->distance;
	if (mDistance < PX_EPS_F32)
	{
		mInvDistance = PX_MAX_F32;
	}
	else
	{
		mInvDistance = 1 / mDistance;
	}

	// get points
	mPoints.resize((uint32_t)mGeomParams->points.positions.arraySizes[0]);
	if (!mPoints.empty())
	{
		PX_COMPILE_TIME_ASSERT(sizeof(PointParams) == sizeof(mGeomParams->points.positions.buf[0]));
		memcpy(&mPoints[0], mGeomParams->points.positions.buf, mPoints.size() * sizeof(PointParams));
	}

	// get point velocities
	for (uint32_t i = 0; i < (uint32_t)mGeomParams->points.velocities.arraySizes[0]; i++)
	{
		mVelocities.pushBack(mGeomParams->points.velocities.buf[i]);
	}

	// get spheres
	mSpheres.resize((uint32_t)mGeomParams->spheres.positions.arraySizes[0]);
	if (!mSpheres.empty())
	{
		PX_COMPILE_TIME_ASSERT(sizeof(SphereParams) == sizeof(mGeomParams->spheres.positions.buf[0]));
		memcpy(&mSpheres[0], mGeomParams->spheres.positions.buf, mSpheres.size() * sizeof(SphereParams));
	}

	// get sphere velocities
	for (int32_t i = 0; i < mGeomParams->spheres.velocities.arraySizes[0]; i++)
	{
		mSphereVelocities.pushBack(mGeomParams->spheres.velocities.buf[i]);
	}

	// get ellipsoids
	mEllipsoids.resize((uint32_t)mGeomParams->ellipsoids.positions.arraySizes[0]);
	if (!mEllipsoids.empty())
	{
		PX_COMPILE_TIME_ASSERT(sizeof(EllipsoidParams) == sizeof(mGeomParams->ellipsoids.positions.buf[0]));
		memcpy(&mEllipsoids[0], mGeomParams->ellipsoids.positions.buf, mEllipsoids.size() * sizeof(EllipsoidParams));
	}

	// get ellipsoid velocities
	for (int32_t i = 0; i < mGeomParams->ellipsoids.velocities.arraySizes[0]; i++)
	{
		mEllipsoidVelocities.pushBack(mGeomParams->ellipsoids.velocities.buf[i]);
	}

	updateCollisions();
}

EmitterGeomExplicitImpl::EmitterGeomExplicitImpl() :
	mDistance(0.0f),
	mInvDistance(PX_MAX_F32),
	mGeomParams(NULL)
{}

EmitterGeom* EmitterGeomExplicitImpl::getEmitterGeom()
{
	return this;
}

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomExplicitImpl::visualize(const PxTransform& , RenderDebugInterface&)
{
}
#else
void EmitterGeomExplicitImpl::visualize(const PxTransform& pose, RenderDebugInterface& renderDebug)
{
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

	RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::DarkGreen));

	RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose);

	PxVec3 pos;
	for (uint32_t i = 0; i < mPoints.size(); ++i)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(pos, 0.01f);
	}

	for (uint32_t i = 0; i < mSpheres.size(); ++i)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(mSpheres[i].center, mSpheres[i].radius);
	}

	for (uint32_t i = 0; i < mEllipsoids.size(); ++i)
	{
		PxVec3 n = mEllipsoids[i].normal, t1, t2;
		GetNormals(n, t1, t2);

		PxMat44 unitSphereToEllipsoid(
		    mEllipsoids[i].radius * t1,
		    mEllipsoids[i].radius * t2,
		    mEllipsoids[i].polarRadius * n,
		    mEllipsoids[i].center
		);

		RENDER_DEBUG_IFACE(&renderDebug)->setPose(pose * PxTransform(unitSphereToEllipsoid));

		RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(PxVec3(0.0f), 1.0f);
	}

	RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
}
#endif

#ifdef WITHOUT_DEBUG_VISUALIZE
void EmitterGeomExplicitImpl::drawPreview(float , RenderDebugInterface*) const
{
}
#else
void EmitterGeomExplicitImpl::drawPreview(float scale, RenderDebugInterface* renderDebug) const
{
	using RENDER_DEBUG::DebugColors;
	RENDER_DEBUG_IFACE(renderDebug)->pushRenderState();
	RENDER_DEBUG_IFACE(renderDebug)->setCurrentColor(RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen),
	                             RENDER_DEBUG_IFACE(renderDebug)->getDebugColor(DebugColors::DarkGreen));

	PxVec3 pos;
	for (uint32_t i = 0 ; i < mPoints.size() ; i++)
	{
		pos = mPoints[i].position;
		RENDER_DEBUG_IFACE(renderDebug)->debugPoint(pos, scale);
	}

	for (uint32_t i = 0; i < mSpheres.size(); ++i)
	{
		RENDER_DEBUG_IFACE(renderDebug)->debugSphere(mSpheres[i].center, mSpheres[i].radius * scale);
	}

	for (uint32_t i = 0; i < mEllipsoids.size(); ++i)
	{
		PxVec3 n = mEllipsoids[i].normal, t1, t2;
		GetNormals(n, t1, t2);

		PxMat44 unitSphereToEllipsoid(
		    mEllipsoids[i].radius * t1,
		    mEllipsoids[i].radius * t2,
		    mEllipsoids[i].polarRadius * n,
		    mEllipsoids[i].center
		);

		RENDER_DEBUG_IFACE(renderDebug)->setPose(unitSphereToEllipsoid);

		RENDER_DEBUG_IFACE(renderDebug)->debugSphere(PxVec3(0.0f), scale);
	}

	RENDER_DEBUG_IFACE(renderDebug)->popRenderState();
}
#endif

/* ApexEmitterActor callable methods */

void EmitterGeomExplicitImpl::updateAssetPoints()
{
	if (!mGeomParams)
	{
		return;
	}

	// just copy the position and velocity lists to mGeomParams

	NvParameterized::Handle h(*mGeomParams);
	NvParameterized::ErrorType pError;

	// Update positions

	pError = mGeomParams->getParameterHandle("points.positions", h);
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	pError = h.resizeArray((int32_t)mPoints.size());
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	PX_COMPILE_TIME_ASSERT(sizeof(PointParams) == sizeof(mGeomParams->points.positions.buf[0]));
	memcpy(&mGeomParams->points.positions.buf[0], &mPoints[0], mPoints.size());

	// velocities may or may not exist, handle this
	pError = mGeomParams->getParameterHandle("velocities", h);
	if (pError == NvParameterized::ERROR_NONE)
	{
		pError = mGeomParams->resizeArray(h, (int32_t)mVelocities.size());
		PX_ASSERT(pError == NvParameterized::ERROR_NONE);

		pError = h.setParamVec3Array(&mVelocities[0], (int32_t)mVelocities.size());
		PX_ASSERT(pError == NvParameterized::ERROR_NONE);
	}
}

void EmitterGeomExplicitImpl::addParticleList(
    uint32_t count,
    const PointParams* params,
    const PxVec3* velocities)
{
	if (velocities)
	{
		mVelocities.resize(mPoints.size(), PxVec3(0.0f));
	}

	for (uint32_t i = 0 ; i < count ; i++)
	{
		mPoints.pushBack(*params++);

		if (velocities)
		{
			mVelocities.pushBack(*velocities++);
		}
	}

	updateCollisions();
	updateAssetPoints();
}

void EmitterGeomExplicitImpl::addParticleList(uint32_t count,
        const PxVec3* positions,
        const PxVec3* velocities)
{
	if (velocities)
	{
		mVelocities.resize(mPoints.size(), PxVec3(0.0f));
	}

	for (uint32_t i = 0 ; i < count ; i++)
	{
		PointParams params = { positions[i], false };
		mPoints.pushBack(params);

		if (velocities)
		{
			mVelocities.pushBack(velocities[i]);
		}
	}

	updateCollisions();
	updateAssetPoints();
}

void EmitterGeomExplicitImpl::addParticleList(uint32_t count,
		const PointListData& data)
{
	if (data.velocityStart != NULL)
	{
		mVelocities.resize(mPoints.size(), PxVec3(0.0f));
	}
	if (data.userDataStart != NULL)
	{
		mPointsUserData.resize(mPoints.size(), 0);
	}

	const uint8_t* positionCurr = static_cast<const uint8_t*>(data.positionStart);
	const uint8_t* velocityCurr = static_cast<const uint8_t*>(data.velocityStart);
	const uint8_t* userDataCurr = static_cast<const uint8_t*>(data.userDataStart);

	for (uint32_t i = 0 ; i < count ; i++)
	{
		const PxVec3& position = *reinterpret_cast<const PxVec3*>(positionCurr);
		positionCurr += data.positionStrideBytes;

		PointParams params = { position, false };
		mPoints.pushBack(params);

		if (velocityCurr != NULL)
		{
			const PxVec3& velocity = *reinterpret_cast<const PxVec3*>(velocityCurr);
			velocityCurr += data.velocityStrideBytes;

			mVelocities.pushBack(velocity);
		}
		if (userDataCurr != NULL)
		{
			const uint32_t userData = *reinterpret_cast<const uint32_t*>(userDataCurr);
			userDataCurr += data.userDataStrideBytes;

			mPointsUserData.pushBack(userData);
		}
	}

	updateCollisions();
	updateAssetPoints();
}

void EmitterGeomExplicitImpl::addSphereList(
    uint32_t count,
    const SphereParams*  params,
    const PxVec3* velocities)
{
	if (velocities)
	{
		mSphereVelocities.resize(mSpheres.size(), PxVec3(0.0f));
	}

	for (uint32_t i = 0 ; i < count ; i++)
	{
		mSpheres.pushBack(*params++);

		if (velocities)
		{
			mSphereVelocities.pushBack(*velocities++);
		}
	}

	updateCollisions();

	// just copy the position and velocity lists to mGeomParams
	if (!mGeomParams)
	{
		return;
	}

	NvParameterized::Handle h(*mGeomParams);
	NvParameterized::ErrorType pError;

	// Update positions

	pError = mGeomParams->getParameterHandle("spheres.positions", h);
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	pError = h.resizeArray((int32_t)mSpheres.size());
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	PX_COMPILE_TIME_ASSERT(sizeof(SphereParams) == sizeof(mGeomParams->spheres.positions.buf[0]));
	memcpy(mGeomParams->spheres.positions.buf, &mSpheres[0], mSpheres.size() * sizeof(SphereParams));

	// velocities may or may not exist, handle this
	pError = mGeomParams->getParameterHandle("spheres.velocities", h);
	if (pError == NvParameterized::ERROR_NONE)
	{
		pError = mGeomParams->resizeArray(h, (int32_t)mSphereVelocities.size());
		PX_ASSERT(pError == NvParameterized::ERROR_NONE);

		h.setParamVec3Array(&mSphereVelocities[0], (int32_t)mSphereVelocities.size());
	}
}

void EmitterGeomExplicitImpl::addEllipsoidList(
    uint32_t count,
    const EllipsoidParams*  params,
    const PxVec3* velocities)
{
	if (velocities)
	{
		mEllipsoidVelocities.resize(mEllipsoids.size(), PxVec3(0.0f));
	}

	for (uint32_t i = 0 ; i < count ; i++)
	{
		mEllipsoids.pushBack(*params++);

		if (velocities)
		{
			mEllipsoidVelocities.pushBack(*velocities++);
		}
	}

	updateCollisions();

	// just copy the position and velocity lists to mGeomParams
	if (!mGeomParams)
	{
		return;
	}

	NvParameterized::Handle h(*mGeomParams);
	NvParameterized::ErrorType pError;

	// Update positions

	pError = mGeomParams->getParameterHandle("ellipsoids.positions", h);
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	pError = h.resizeArray((int32_t)mEllipsoids.size());
	PX_ASSERT(pError == NvParameterized::ERROR_NONE);

	PX_COMPILE_TIME_ASSERT(sizeof(EllipsoidParams) == sizeof(mGeomParams->ellipsoids.positions.buf[0]));
	memcpy(mGeomParams->ellipsoids.positions.buf, &mEllipsoids[0], mEllipsoids.size() * sizeof(EllipsoidParams));

	// velocities may or may not exist, handle this
	pError = mGeomParams->getParameterHandle("ellipsoids.velocities", h);
	if (pError == NvParameterized::ERROR_NONE)
	{
		pError = mGeomParams->resizeArray(h, (int32_t)mEllipsoidVelocities.size());
		PX_ASSERT(pError == NvParameterized::ERROR_NONE);

		h.setParamVec3Array(&mEllipsoidVelocities[0], (int32_t)mEllipsoidVelocities.size());
	}
}

void EmitterGeomExplicitImpl::getParticleList(
    const PointParams* &params,
    uint32_t& numPoints,
    const PxVec3* &velocities,
    uint32_t& numVelocities) const
{
	numPoints = mPoints.size();
	params = numPoints ? &mPoints[0] : 0;

	numVelocities = mVelocities.size();
	velocities = numVelocities ? &mVelocities[0] : 0;
}

void EmitterGeomExplicitImpl::getSphereList(
    const SphereParams*  &params,
    uint32_t& numSpheres,
    const PxVec3* &velocities,
    uint32_t& numVelocities) const
{
	numSpheres = mSpheres.size();
	params = numSpheres ? &mSpheres[0] : 0;

	numVelocities = mSphereVelocities.size();
	velocities = numVelocities ? &mSphereVelocities[0] : 0;
}

void EmitterGeomExplicitImpl::getEllipsoidList(
    const EllipsoidParams*  &params,
    uint32_t& numEllipsoids,
    const PxVec3* &velocities,
    uint32_t& numVelocities) const
{
	numEllipsoids = mEllipsoids.size();
	params = numEllipsoids ? &mEllipsoids[0] : 0;

	numVelocities = mSphereVelocities.size();
	velocities = numVelocities ? &mSphereVelocities[0] : 0;
}

#define EPS 0.001f

bool EmitterGeomExplicitImpl::isInside(const PxVec3& x, uint32_t shapeIdx) const
{
	if (!mBboxes[shapeIdx].contains(x))
	{
		return false;
	}

	uint32_t nspheres = mSpheres.size(),
	             nellipsoids = mEllipsoids.size();

	if (shapeIdx < nspheres)  // Sphere
	{
		const SphereParams& sphere = mSpheres[shapeIdx];
		return PxAbs((x - sphere.center).magnitudeSquared() - sphere.radius * sphere.radius) < EPS;
	}
	else if (shapeIdx < nspheres + nellipsoids)  // Ellipsoid
	{
		const EllipsoidParams& ellipsoid = mEllipsoids[shapeIdx - nspheres];

		PxVec3 y(x - ellipsoid.center);

		PxVec3 n(ellipsoid.normal);
		n.normalize();

		float normalProj = y.dot(n);

		PxVec3 planeProj = y - normalProj * n;
		float planeProjMag2 = planeProj.magnitudeSquared();

		float R2 = ellipsoid.radius * ellipsoid.radius,
		             Rp2 = ellipsoid.polarRadius * ellipsoid.polarRadius;

		if (planeProjMag2 > R2 + EPS)
		{
			return false;
		}

		return normalProj * normalProj < EPS + Rp2 * (1.0f - 1.0f / R2 * planeProjMag2);
	}
	else // Point
	{
		return (x - mPoints[shapeIdx - nspheres - nellipsoids].position).magnitudeSquared() < EPS;
	}
}

void EmitterGeomExplicitImpl::AddParticle(
    physx::Array<PxVec3>& positions, physx::Array<PxVec3>& velocities,
    physx::Array<uint32_t>* userDataArrayPtr,
    const PxTransform& pose, float cutoff, PxBounds3& outBounds,
	const PxVec3& pos, const PxVec3& vel, uint32_t userData,
    uint32_t srcShapeIdx,
    QDSRand& rand) const
{
	float r = rand.getScaled(0, 1);
	if (r > cutoff)
	{
		return;
	}

	PxVec3 randPos = pos + PxVec3(
	                            rand.getScaled(-mDistance, mDistance),
	                            rand.getScaled(-mDistance, mDistance),
	                            rand.getScaled(-mDistance, mDistance));

	// Check collisions
	for (uint32_t i = 0; i < mCollisions[srcShapeIdx].next; ++i)
	{
		uint32_t collisionShapeIdx = mCollisions[srcShapeIdx].shapeIndices[i];

		// If colliding body was already processed and point lies in this body
		if (collisionShapeIdx < srcShapeIdx && isInside(pos, collisionShapeIdx))
		{
			return;
		}
	}

	/* if absolute positions are submitted, pose will be at origin */
	if (pose == PxTransform(PxIdentity))
	{
		positions.pushBack(randPos);
		velocities.pushBack(vel);
	}
	else
	{
		positions.pushBack(pose.transform(randPos));
		velocities.pushBack(pose.rotate(vel));
	}

	if (userDataArrayPtr)
	{
		userDataArrayPtr->pushBack(userData);
	}

	outBounds.include(positions.back());
}

void EmitterGeomExplicitImpl::computeFillPositions(
    physx::Array<PxVec3>& positions,
    physx::Array<PxVec3>& velocities,
	physx::Array<uint32_t>* userDataArrayPtr,
    const PxTransform& pose,
	const PxVec3& scale,
    float density,
    PxBounds3& outBounds,
    QDSRand& rand) const
{
	PX_UNUSED(scale);
	uint32_t shapeIdx = 0; // Global shape index

	for (uint32_t i = 0; i < mPoints.size(); ++i, ++shapeIdx)
	{
		const uint32_t pointUserData = i < mPointsUserData.size() ? mPointsUserData[i] : 0;

		AddParticle(
		    positions, velocities, userDataArrayPtr,
		    pose, density, outBounds,
		    mPoints[i].position, i >= mVelocities.size() ? PxVec3(0.0f) : mVelocities[i], pointUserData,
		    shapeIdx,
		    rand
		);
	}

	for (uint32_t i = 0; i < mSpheres.size(); ++i, ++shapeIdx)
	{
		PxVec3 c = mSpheres[i].center;
		float R = mSpheres[i].radius;

		const uint32_t sphereUserData = 0;

		float RR = R * R;

		int32_t nx = (int32_t)(R * mInvDistance);

		for (int32_t j = -nx; j < nx; ++j)
		{
			float x = j * mDistance,
			             xx = x * x;

			float yext = PxSqrt(PxMax(0.0f, RR - xx));
			int32_t ny = (int32_t)(yext * mInvDistance);

			for (int32_t k = -ny; k < ny; ++k)
			{
				float y = k * mDistance;

				float zext = PxSqrt(PxMax(0.0f, RR - xx - y * y));
				int32_t nz = (int32_t)(zext * mInvDistance);

				for (int32_t l = -nz; l < nz; ++l)
				{
					float z = l * mDistance;

					AddParticle(
					    positions, velocities, userDataArrayPtr,
					    pose, density, outBounds,
					    c + PxVec3(x, y, z),
					    i >= mSphereVelocities.size() ? PxVec3(0.0f) : mSphereVelocities[i], sphereUserData,
					    shapeIdx,
					    rand
					);
				} //l
			} //k
		} //j
	} //i

	for (uint32_t i = 0; i < mEllipsoids.size(); ++i, ++shapeIdx)
	{
		PxVec3 c = mEllipsoids[i].center,
		              n = mEllipsoids[i].normal;

		const uint32_t ellipsoidUserData = 0;

		// TODO: precompute this stuff?

		float R = mEllipsoids[i].radius,
		             Rp = mEllipsoids[i].polarRadius;

		PxVec3 t1, t2;
		GetNormals(n, t1, t2);

		float RR = R * R,
		             Rp_R = PxAbs(Rp / R);

		int32_t nx = (int32_t)(R * mInvDistance);
		for (int32_t j = -nx; j <= nx ; ++j)
		{
			float x = j * mDistance,
			             xx = x * x;

			float yext = PxSqrt(PxMax(0.0f, RR - xx));
			int32_t ny = (int32_t)(yext * mInvDistance);

			for (int32_t k = -ny; k <= ny; ++k)
			{
				float y = k * mDistance;

				float zext = Rp_R * PxSqrt(PxMax(0.0f, RR - xx - y * y));
				int32_t nz = (int32_t)(zext * mInvDistance);

				int32_t lmax = Rp < 0 ? 0 : nz; // Check for half-ellipsoid
				for (int32_t l = -nz; l <= lmax; ++l)
				{
					float z = l * mDistance;

					AddParticle(
					    positions, velocities, userDataArrayPtr,
					    pose, density, outBounds,
					    c + x * t1 + y * t2 + z * n,
					    i >= mEllipsoidVelocities.size() ? PxVec3(0.0f) : mEllipsoidVelocities[i], ellipsoidUserData,
					    shapeIdx,
					    rand
					);
				} //l
			} //k
		} //j
	} //i
}

struct EndPoint
{
	float	val;

	uint32_t	boxIdx;
	bool			isLeftPoint;

	EndPoint(float v, uint32_t idx, bool left)
		: val(v), boxIdx(idx), isLeftPoint(left) {}

	PX_INLINE static bool isLessThan(const EndPoint& p1, const EndPoint& p2)
	{
		if (p1.val < p2.val)
		{
			return true;
		}

		// Adjacent shapes do intersect
		if (p1.val == p2.val)
		{
			return p1.isLeftPoint && !p2.isLeftPoint;
		}

		return false;
	}
};

void EmitterGeomExplicitImpl::updateCollisions()
{
	uint32_t npoints = mPoints.size(),
	             nspheres = mSpheres.size(),
	             nellipsoids = mEllipsoids.size();

	mCollisions.reset();

	uint32_t ntotal = npoints + nspheres + nellipsoids;
	if (!ntotal)
	{
		return;
	}

	mBboxes.resize(ntotal);
	mCollisions.resize(ntotal);

	physx::Array<bool> doDetectOverlaps(ntotal);

	// TODO: use custom axis instead of Ox
	// E.g. (sx, sy, sz) or simply choose the best of Ox, Oy, Oz

	for (uint32_t i = 0; i < nspheres; ++i)
	{
		PxVec3 r3(mSpheres[i].radius);

		mBboxes[i] = PxBounds3(mSpheres[i].center - r3, mSpheres[i].center + r3);
		doDetectOverlaps[i] = mSpheres[i].doDetectOverlaps;
	}

	for (uint32_t i = 0; i < nellipsoids; ++i)
	{
		PxVec3 r3(mEllipsoids[i].radius, mEllipsoids[i].radius, mEllipsoids[i].polarRadius);
		PxBounds3 localBox(-r3, r3);

		PxVec3 n = mEllipsoids[i].normal;
		n.normalize();

		PxMat33 T = physx::shdfnd::rotFrom2Vectors(PxVec3(0, 0, 1), n);

		if(!localBox.isValid())
		{
			localBox.setEmpty();
		}

		localBox = PxBounds3::transformFast(T, localBox);
		localBox.minimum += mEllipsoids[i].center;
		localBox.maximum += mEllipsoids[i].center;

		mBboxes[nspheres + i] = localBox;
		doDetectOverlaps[nspheres + i] = mEllipsoids[i].doDetectOverlaps;
	}

	for (uint32_t i = 0; i < npoints; ++i)
	{
		mBboxes[nspheres + nellipsoids + i] = PxBounds3(mPoints[i].position, mPoints[i].position);
		doDetectOverlaps[nspheres + nellipsoids + i] = mPoints[i].doDetectOverlaps;
	}

	physx::Array<EndPoint> endPoints;
	endPoints.reserve(2 * ntotal);
	for (uint32_t i = 0; i < ntotal; ++i)
	{
		endPoints.pushBack(EndPoint(mBboxes[i].minimum.x, i, true));
		endPoints.pushBack(EndPoint(mBboxes[i].maximum.x, i, false));
	}

	nvidia::sort(&endPoints[0], endPoints.size(), EndPoint::isLessThan);

	nvidia::HashSet<uint32_t> openedBoxes;
	for (uint32_t i = 0; i < endPoints.size(); ++i)
	{
		uint32_t boxIdx = endPoints[i].boxIdx;

		if (endPoints[i].isLeftPoint)
		{
			for (nvidia::HashSet<uint32_t>::Iterator j = openedBoxes.getIterator(); !j.done(); ++j)
			{
				if (!doDetectOverlaps[boxIdx] || !doDetectOverlaps[*j])
				{
					break;
				}

				if (!mBboxes[*j].intersects(mBboxes[boxIdx]))
				{
					continue;
				}

				mCollisions[boxIdx].pushBack(*j);
				mCollisions[*j].pushBack(boxIdx);
			} //j

			openedBoxes.insert(boxIdx);
		}
		else
		{
			PX_ASSERT(openedBoxes.contains(boxIdx));
			openedBoxes.erase(boxIdx);
		}
	} //i
}

}
} // namespace nvidia::apex
