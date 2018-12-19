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

#include "PtCollisionMethods.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxMat33.h"
#include "foundation/PxVec3.h"
#include "foundation/PxPlane.h"
#include "GuConvexMeshData.h"
#include "CmPhysXCommon.h"
#include "PsVecMath.h"

#define FLCNVX_NO_DC (1 << 0)
#define FLCNVX_NO_PARALLEL_CC (1 << 1)
#define FLCNVX_NO_PROX (1 << 2)
#define FLCNVX_NO_CONTAINMENT (1 << 3)
#define PLCNVX_POTENTIAL_PROX (1 << 4)

using namespace physx::shdfnd::aos;
using namespace physx;
using namespace Pt;

namespace
{

void scalePlanes(PxPlane* scaledPlaneBuf, const Gu::ConvexHullData* convexHullData, const PxMat33& invScaling)
{
	PxU32 numPlanes = convexHullData->mNbPolygons;
	PxPlane* planeIt = scaledPlaneBuf;
	const Gu::HullPolygonData* polygonIt = convexHullData->mPolygons;
	for(; numPlanes > 0; --numPlanes, ++planeIt, ++polygonIt)
	{
		PxVec3 normal = polygonIt->mPlane.n;
		PxF32 d = polygonIt->mPlane.d;
		normal = invScaling.transformTranspose(normal);
		PxReal magnitude = normal.normalize();
		*planeIt = PxPlane(normal, d / magnitude);
	}
}

} // namespace

void physx::Pt::collideWithConvexPlanes(ParticleCollData& collData, const PxPlane* convexPlanes, PxU32 numPlanes,
                                        const PxReal proxRadius)
{
	PX_ASSERT(convexPlanes);

	// initializing these to 0 saves a test for accessing corresponding arrays
	PxU32 newPosPlaneIndex = 0;
	PxU32 oldPosPlaneIndex = 0;
	PxU32 rayPlaneIndex = 0;
	bool newPosOutMask = false;

	PxReal latestEntry = -FLT_MAX;
	PxReal soonestExit = FLT_MAX;
	PxReal newPosClosestDist = -FLT_MAX;
	PxReal oldPosClosestDist = -FLT_MAX;

	PxVec3 motion = collData.localNewPos - collData.localOldPos;

	const PxPlane* plane = convexPlanes;
	for(PxU32 k = 0; k < numPlanes; k++)
	{
		PxReal planeDistNewPos = plane[k].distance(collData.localNewPos);
		PxReal planeDistOldPos = plane[k].distance(collData.localOldPos);

		bool wasNewPosOutide = newPosClosestDist > 0.0f;

		// maximize distance to planes to find minimal distance to convex
		bool isOldPosFurther = planeDistOldPos > oldPosClosestDist;
		oldPosClosestDist = isOldPosFurther ? planeDistOldPos : oldPosClosestDist;
		oldPosPlaneIndex = isOldPosFurther ? k : oldPosPlaneIndex;

		bool isNewPosFurther = planeDistNewPos > newPosClosestDist;
		newPosClosestDist = isNewPosFurther ? planeDistNewPos : newPosClosestDist;
		newPosPlaneIndex = isNewPosFurther ? k : newPosPlaneIndex;

		bool isNewPosOutside = planeDistNewPos > 0.0f;

		// flagging cases where newPos it out multiple times
		newPosOutMask |= (wasNewPosOutide & isNewPosOutside);

		// continuous collision
		PxReal dot = motion.dot(plane[k].n);

		// div by zero shouldn't hurt, since dot == 0.0f case is masked out
		PxReal hitTime = -planeDistOldPos / dot;
		bool isEntry = (dot < 0.0f) & (hitTime > latestEntry);
		bool isExit = (dot > 0.0f) & (hitTime < soonestExit);

		latestEntry = isEntry ? hitTime : latestEntry;
		rayPlaneIndex = isEntry ? k : rayPlaneIndex;
		soonestExit = isExit ? hitTime : soonestExit;

		// mark parallel outside for no ccd in PxcFinalizeConvexCollision
		latestEntry = ((dot == 0.0f) & isNewPosOutside) ? FLT_MAX : latestEntry;
	}

	bool isContained = oldPosClosestDist <= 0.0f;
	bool isDc = newPosClosestDist <= collData.restOffset;
	bool isProximity = (newPosClosestDist > 0.0f) && (newPosClosestDist <= proxRadius) && !newPosOutMask;

	if(isContained)
	{
		// Treat the case where the old pos is inside the skeleton as
		// a continous collision with time 0

		collData.localFlags |= ParticleCollisionFlags::L_CC;
		collData.ccTime = 0.0f;
		collData.localSurfaceNormal = plane[oldPosPlaneIndex].n;

		// Push the particle to the surface (such that distance to surface is equal to the collision radius)
		collData.localSurfacePos =
		    collData.localOldPos + plane[oldPosPlaneIndex].n * (collData.restOffset - oldPosClosestDist);
	}
	else
	{
		// Check for continuous collision
		// only add a proximity/discrete case if there are no continous collisions
		// for this shape or any other shape before

		bool ccHappened = (0.0f <= latestEntry) && (latestEntry < collData.ccTime) && (latestEntry <= soonestExit);
		if(ccHappened)
		{
			collData.localSurfaceNormal = plane[rayPlaneIndex].n;
			// collData.localSurfacePos = collData.localOldPos + (motion * latestEntry) + (continuousNormal *
			// collData.restOffset);
			computeContinuousTargetPosition(collData.localSurfacePos, collData.localOldPos, motion * latestEntry,
			                                plane[rayPlaneIndex].n, collData.restOffset);
			collData.ccTime = latestEntry;
			collData.localFlags |= ParticleCollisionFlags::L_CC;
		}
		else if(!(collData.localFlags & ParticleCollisionFlags::CC))
		{
			// No other collision shape has caused a continuous collision so far
			if(isProximity) // proximity
			{
				collData.localSurfaceNormal = plane[newPosPlaneIndex].n;
				collData.localSurfacePos =
				    collData.localNewPos + plane[newPosPlaneIndex].n * (collData.restOffset - newPosClosestDist);
				collData.localFlags |= ParticleCollisionFlags::L_PROX;
			}
			if(isDc) // discrete collision
			{
				collData.localSurfaceNormal = plane[newPosPlaneIndex].n;
				collData.localSurfacePos =
				    collData.localNewPos + plane[newPosPlaneIndex].n * (collData.restOffset - newPosClosestDist);
				collData.localFlags |= ParticleCollisionFlags::L_DC;
			}
		}
	}
}

void physx::Pt::collideWithConvexPlanesSIMD(ParticleCollDataV4& collDataV4, const PxPlane* convexPlanes,
                                            PxU32 numPlanes, const PxReal proxRadius)
{
	PX_ASSERT(convexPlanes);
	Ps::prefetch(convexPlanes);

	Vec4V latestEntry = V4Load(-FLT_MAX);
	Vec4V soonestExit = V4Load(FLT_MAX);
	Vec4V newPosClosestDist = V4Load(-FLT_MAX);
	Vec4V oldPosClosestDist = V4Load(-FLT_MAX);
	Vec4V discreteNormal[4] = { V4Zero(), V4Zero(), V4Zero(), V4Zero() };
	Vec4V continuousNormal[4] = { V4Zero(), V4Zero(), V4Zero(), V4Zero() };
	Vec4V containmentNormal[4] = { V4Zero(), V4Zero(), V4Zero(), V4Zero() };

	Vec4V localNewPos0 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localNewPos[0]));
	Vec4V localOldPos0 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localOldPos[0]));

	Vec4V localNewPos1 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localNewPos[1]));
	Vec4V localOldPos1 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localOldPos[1]));

	Vec4V localNewPos2 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localNewPos[2]));
	Vec4V localOldPos2 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localOldPos[2]));

	Vec4V localNewPos3 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localNewPos[3]));
	Vec4V localOldPos3 = V4LoadA(reinterpret_cast<const PxF32*>(&collDataV4.localOldPos[3]));

	Vec4V motion[4];
	motion[0] = V4Sub(localNewPos0, localOldPos0);
	motion[1] = V4Sub(localNewPos1, localOldPos1);
	motion[2] = V4Sub(localNewPos2, localOldPos2);
	motion[3] = V4Sub(localNewPos3, localOldPos3);

	const Mat44V newPos44(localNewPos0, localNewPos1, localNewPos2, localNewPos3);
	const Mat44V oldPos44(localOldPos0, localOldPos1, localOldPos2, localOldPos3);
	const Mat44V motion44(motion[0], motion[1], motion[2], motion[3]);

	const Mat44V newPosTrans44 = M44Trnsps(newPos44);
	const Mat44V oldPosTrans44 = M44Trnsps(oldPos44);
	const Mat44V motionTrans44 = M44Trnsps(motion44);

	BoolV newPosOutMask = BLoad(false);

	const PxPlane* plane = convexPlanes;
	for(PxU32 k = 0; k < numPlanes; k++)
	{
		Vec4V planeNormal = Vec4V_From_Vec3V(V3LoadU(plane->n));
		Vec4V planeD = V4Load(plane->d);
		plane++;
		Ps::prefetch(plane);

		const FloatV normalX = V4GetX(planeNormal);
		const FloatV normalY = V4GetY(planeNormal);
		const FloatV normalZ = V4GetZ(planeNormal);

		Vec4V v1 = V4ScaleAdd(newPosTrans44.col0, normalX, planeD);
		Vec4V v2 = V4ScaleAdd(newPosTrans44.col1, normalY, v1);
		Vec4V planeDistNewPosV4 = V4ScaleAdd(newPosTrans44.col2, normalZ, v2);

		v1 = V4ScaleAdd(oldPosTrans44.col0, normalX, planeD);
		v2 = V4ScaleAdd(oldPosTrans44.col1, normalY, v1);
		Vec4V planeDistOldPosV4 = V4ScaleAdd(oldPosTrans44.col2, normalZ, v2);

		// containment: select the max distance plane
		BoolV mask = V4IsGrtr(planeDistOldPosV4, oldPosClosestDist);
		oldPosClosestDist = V4Sel(mask, planeDistOldPosV4, oldPosClosestDist);
		containmentNormal[0] = V4Sel(BSplatElement<0>(mask), planeNormal, containmentNormal[0]);
		containmentNormal[1] = V4Sel(BSplatElement<1>(mask), planeNormal, containmentNormal[1]);
		containmentNormal[2] = V4Sel(BSplatElement<2>(mask), planeNormal, containmentNormal[2]);
		containmentNormal[3] = V4Sel(BSplatElement<3>(mask), planeNormal, containmentNormal[3]);

		// proxmity and discrete: select the max distance planes
		BoolV wasNewPosOutide = V4IsGrtr(newPosClosestDist, V4Zero());
		BoolV isNewPosOutside = V4IsGrtr(planeDistNewPosV4, V4Zero());

		mask = V4IsGrtr(planeDistNewPosV4, newPosClosestDist);
		newPosClosestDist = V4Sel(mask, planeDistNewPosV4, newPosClosestDist);
		discreteNormal[0] = V4Sel(BSplatElement<0>(mask), planeNormal, discreteNormal[0]);
		discreteNormal[1] = V4Sel(BSplatElement<1>(mask), planeNormal, discreteNormal[1]);
		discreteNormal[2] = V4Sel(BSplatElement<2>(mask), planeNormal, discreteNormal[2]);
		discreteNormal[3] = V4Sel(BSplatElement<3>(mask), planeNormal, discreteNormal[3]);

		// flagging cases where newPos it out multiple times
		newPosOutMask = BOr(newPosOutMask, BAnd(wasNewPosOutide, isNewPosOutside));

		// Test continuous collision
		v1 = V4Scale(motionTrans44.col0, normalX);
		v2 = V4ScaleAdd(motionTrans44.col1, normalY, v1);
		Vec4V dotV4 = V4ScaleAdd(motionTrans44.col2, normalZ, v2);

		Vec4V hitTime = V4Neg(V4Div(planeDistOldPosV4, dotV4));

		BoolV exit = V4IsGrtr(dotV4, V4Zero());
		mask = BAnd(exit, V4IsGrtr(soonestExit, hitTime));
		soonestExit = V4Sel(mask, hitTime, soonestExit);

		BoolV entry = V4IsGrtr(V4Zero(), dotV4);
		mask = BAnd(entry, V4IsGrtr(hitTime, latestEntry));
		latestEntry = V4Sel(mask, hitTime, latestEntry);
		continuousNormal[0] = V4Sel(BSplatElement<0>(mask), planeNormal, continuousNormal[0]);
		continuousNormal[1] = V4Sel(BSplatElement<1>(mask), planeNormal, continuousNormal[1]);
		continuousNormal[2] = V4Sel(BSplatElement<2>(mask), planeNormal, continuousNormal[2]);
		continuousNormal[3] = V4Sel(BSplatElement<3>(mask), planeNormal, continuousNormal[3]);

		// mark parallel outside for no ccd in PxcFinalizeConvexCollision
		mask = BAnd(isNewPosOutside, V4IsEq(V4Zero(), dotV4));
		latestEntry = V4Sel(mask, V4One(), latestEntry);
	}

	VecU32V localFlags = U4LoadXYZW(collDataV4.localFlags[0], collDataV4.localFlags[1], collDataV4.localFlags[2],
	                                collDataV4.localFlags[3]);
	Vec4V proxRadiusV4 = V4Load(proxRadius);
	Vec4V restOffsetV4 = V4LoadA(collDataV4.restOffset);

	const VecU32V u4Zero = U4LoadXYZW(0, 0, 0, 0);
	const VecU32V flagCC = U4LoadXYZW(ParticleCollisionFlags::CC, ParticleCollisionFlags::CC,
	                                  ParticleCollisionFlags::CC, ParticleCollisionFlags::CC);
	const BoolV noFlagCC = V4IsEqU32(V4U32and(flagCC, localFlags), u4Zero);

	// proximity
	const VecU32V flagLPROX = U4LoadXYZW(ParticleCollisionFlags::L_PROX, ParticleCollisionFlags::L_PROX,
	                                     ParticleCollisionFlags::L_PROX, ParticleCollisionFlags::L_PROX);
	const BoolV proximityV =
	    BAnd(BAnd(BAnd(noFlagCC, V4IsGrtrOrEq(newPosClosestDist, V4Zero())), V4IsGrtr(proxRadiusV4, newPosClosestDist)),
	         BNot(newPosOutMask));
	VecU32V stateFlag = V4U32Sel(proximityV, flagLPROX, u4Zero);

	// discrete
	const VecU32V flagLDC = U4LoadXYZW(ParticleCollisionFlags::L_DC, ParticleCollisionFlags::L_DC,
	                                   ParticleCollisionFlags::L_DC, ParticleCollisionFlags::L_DC);
	const BoolV DCV =
	    BAnd(BAnd(noFlagCC, V4IsGrtrOrEq(newPosClosestDist, V4Zero())), V4IsGrtr(restOffsetV4, newPosClosestDist));
	stateFlag = V4U32or(stateFlag, V4U32Sel(DCV, flagLDC, u4Zero));

	// cc
	const VecU32V flagLCC = U4LoadXYZW(ParticleCollisionFlags::L_CC, ParticleCollisionFlags::L_CC,
	                                   ParticleCollisionFlags::L_CC, ParticleCollisionFlags::L_CC);

	Vec4V oldCCTimeV = V4LoadA(collDataV4.ccTime);
	const BoolV ccHappenedV = BAnd(BAnd(V4IsGrtrOrEq(latestEntry, V4Zero()), V4IsGrtr(oldCCTimeV, latestEntry)),
	                               V4IsGrtrOrEq(soonestExit, latestEntry));

	stateFlag = V4U32Sel(ccHappenedV, flagLCC, stateFlag);
	Vec4V localSurfaceNormal0 = V4Sel(BSplatElement<0>(ccHappenedV), continuousNormal[0], discreteNormal[0]);
	Vec4V localSurfaceNormal1 = V4Sel(BSplatElement<1>(ccHappenedV), continuousNormal[1], discreteNormal[1]);
	Vec4V localSurfaceNormal2 = V4Sel(BSplatElement<2>(ccHappenedV), continuousNormal[2], discreteNormal[2]);
	Vec4V localSurfaceNormal3 = V4Sel(BSplatElement<3>(ccHappenedV), continuousNormal[3], discreteNormal[3]);

	Vec4V ccTimeV = V4Sel(ccHappenedV, latestEntry, oldCCTimeV);
	Vec4V distV = newPosClosestDist;

#if PT_CCD_MEDTHOD == PT_CCD_PROJECT
	Vec4V projected0 = V4MulAdd(motion0, V4U32SplatElement<0>(latestEntry), localOldPos0);
	Vec4V projected1 = V4MulAdd(motion1, V4U32SplatElement<1>(latestEntry), localOldPos1);
	Vec4V projected2 = V4MulAdd(motion2, V4U32SplatElement<2>(latestEntry), localOldPos2);
	Vec4V projected2 = V4MulAdd(motion3, V4U32SplatElement<3>(latestEntry), localOldPos3);
	distV = V4Sel(ccHappenedV, V4Zero(), distV);

#elif PT_CCD_MEDTHOD == PT_CCD_STAY
	Vec4V projected0 = localOldPos0;
	Vec4V projected1 = localOldPos1;
	Vec4V projected2 = localOldPos2;
	Vec4V projected3 = localOldPos3;
	distV = V4Sel(ccHappenedV, restOffsetV4, distV);

#elif PT_CCD_MEDTHOD == PT_CCD_IMPACT
	Vec4V projected0 = V4MulAdd(motion0, V4U32SplatElement<0>(latestEntry), localOldPos0);
	Vec4V projected1 = V4MulAdd(motion1, V4U32SplatElement<1>(latestEntry), localOldPos1);
	Vec4V projected2 = V4MulAdd(motion2, V4U32SplatElement<2>(latestEntry), localOldPos2);
	Vec4V projected2 = V4MulAdd(motion3, V4U32SplatElement<3>(latestEntry), localOldPos3);
	distV = V4Sel(ccHappenedV, restOffsetV4, distV);
#else
	PX_ASSERT(0); // simd unspport yet
#endif

	Vec4V localSurfacePos0 = V4Sel(BSplatElement<0>(ccHappenedV), projected0, localNewPos0);
	Vec4V localSurfacePos1 = V4Sel(BSplatElement<1>(ccHappenedV), projected1, localNewPos1);
	Vec4V localSurfacePos2 = V4Sel(BSplatElement<2>(ccHappenedV), projected2, localNewPos2);
	Vec4V localSurfacePos3 = V4Sel(BSplatElement<3>(ccHappenedV), projected3, localNewPos3);

	// contain
	const BoolV containmentV = V4IsGrtrOrEq(V4Zero(), oldPosClosestDist);

	stateFlag = V4U32Sel(containmentV, flagLCC, stateFlag);

	localSurfaceNormal0 = V4Sel(BSplatElement<0>(containmentV), containmentNormal[0], localSurfaceNormal0);
	localSurfaceNormal1 = V4Sel(BSplatElement<1>(containmentV), containmentNormal[1], localSurfaceNormal1);
	localSurfaceNormal2 = V4Sel(BSplatElement<2>(containmentV), containmentNormal[2], localSurfaceNormal2);
	localSurfaceNormal3 = V4Sel(BSplatElement<3>(containmentV), containmentNormal[3], localSurfaceNormal3);

	localSurfacePos0 = V4Sel(BSplatElement<0>(containmentV), localOldPos0, localSurfacePos0);
	localSurfacePos1 = V4Sel(BSplatElement<1>(containmentV), localOldPos1, localSurfacePos1);
	localSurfacePos2 = V4Sel(BSplatElement<2>(containmentV), localOldPos2, localSurfacePos2);
	localSurfacePos3 = V4Sel(BSplatElement<3>(containmentV), localOldPos3, localSurfacePos3);

	distV = V4Sel(containmentV, oldPosClosestDist, distV);
	ccTimeV = V4Sel(containmentV, V4Zero(), ccTimeV);

	// localSurfacePos
	Vec4V reflectDistV = V4Sub(restOffsetV4, distV);
	localSurfacePos0 = V4MulAdd(localSurfaceNormal0, V4SplatElement<0>(reflectDistV), localSurfacePos0);
	localSurfacePos1 = V4MulAdd(localSurfaceNormal1, V4SplatElement<1>(reflectDistV), localSurfacePos1);
	localSurfacePos2 = V4MulAdd(localSurfaceNormal2, V4SplatElement<2>(reflectDistV), localSurfacePos2);
	localSurfacePos3 = V4MulAdd(localSurfaceNormal3, V4SplatElement<3>(reflectDistV), localSurfacePos3);

	// store
	V4StoreA(localSurfacePos0, reinterpret_cast<PxF32*>(&collDataV4.localSurfacePos[0]));
	V4StoreA(localSurfacePos1, reinterpret_cast<PxF32*>(&collDataV4.localSurfacePos[1]));
	V4StoreA(localSurfacePos2, reinterpret_cast<PxF32*>(&collDataV4.localSurfacePos[2]));
	V4StoreA(localSurfacePos3, reinterpret_cast<PxF32*>(&collDataV4.localSurfacePos[3]));

	V4StoreA(localSurfaceNormal0, reinterpret_cast<PxF32*>(&collDataV4.localSurfaceNormal[0]));
	V4StoreA(localSurfaceNormal1, reinterpret_cast<PxF32*>(&collDataV4.localSurfaceNormal[1]));
	V4StoreA(localSurfaceNormal2, reinterpret_cast<PxF32*>(&collDataV4.localSurfaceNormal[2]));
	V4StoreA(localSurfaceNormal3, reinterpret_cast<PxF32*>(&collDataV4.localSurfaceNormal[3]));

	V4StoreA(ccTimeV, collDataV4.ccTime);

	V4U32StoreAligned(stateFlag, reinterpret_cast<VecU32V*>(collDataV4.localFlags));
}

/**
input scaledPlaneBuf needs a capacity of the number of planes in convexShape
*/
void physx::Pt::collideWithConvex(PxPlane* scaledPlaneBuf, ParticleCollData* particleCollData, PxU32 numCollData,
                                  const Gu::GeometryUnion& convexShape, const PxReal proxRadius)
{
	PX_ASSERT(scaledPlaneBuf);
	PX_ASSERT(particleCollData);

	const PxConvexMeshGeometryLL& convexShapeData = convexShape.get<const PxConvexMeshGeometryLL>();
	const Gu::ConvexHullData* convexHullData = convexShapeData.hullData;
	PX_ASSERT(convexHullData);

	// convex bounds in local space
	PxMat33 scaling = convexShapeData.scale.toMat33(), invScaling;
	invScaling = scaling.getInverse();

	PX_ASSERT(!convexHullData->mAABB.isEmpty());
	PxBounds3 shapeBounds = convexHullData->mAABB.transformFast(scaling);
	PX_ASSERT(!shapeBounds.isEmpty());
	shapeBounds.fattenFast(proxRadius);
	bool scaledPlanes = false;

#if PT_USE_SIMD_CONVEX_COLLISION
	const Vec3V boundMin = V3LoadU(shapeBounds.minimum);
	const Vec3V boundMax = V3LoadU(shapeBounds.maximum);
	const Vec4V boundMinX = V4SplatElement<0>(Vec4V_From_Vec3V(boundMin));
	const Vec4V boundMinY = V4SplatElement<1>(Vec4V_From_Vec3V(boundMin));
	const Vec4V boundMinZ = V4SplatElement<2>(Vec4V_From_Vec3V(boundMin));
	const Vec4V boundMaxX = V4SplatElement<0>(Vec4V_From_Vec3V(boundMax));
	const Vec4V boundMaxY = V4SplatElement<1>(Vec4V_From_Vec3V(boundMax));
	const Vec4V boundMaxZ = V4SplatElement<2>(Vec4V_From_Vec3V(boundMax));

	ParticleCollDataV4 collDataV4;

	const VecU32V u4Zero = U4LoadXYZW(0, 0, 0, 0);
	const VecU32V u4One = U4LoadXYZW(1, 1, 1, 1);
	PX_ALIGN(16, ParticleCollData fakeCsd);
	fakeCsd.localOldPos = PxVec3(FLT_MAX, FLT_MAX, FLT_MAX);
	fakeCsd.localNewPos = PxVec3(FLT_MAX, FLT_MAX, FLT_MAX);
	PX_ALIGN(16, PxU32 overlapArray[128]);

	PxU32 start = 0;
	while(start < numCollData)
	{
		const PxU32 batchSize = PxMin(numCollData - start, PxU32(128));
		PxU32 v4Count = 0;
		ParticleCollData* particleCollDataIt = &particleCollData[start];
		for(PxU32 i = 0; i < batchSize; i += 4)
		{
			ParticleCollData* collData[4];
			collData[0] = particleCollDataIt++;
			collData[1] = (i + 1 < numCollData) ? particleCollDataIt++ : &fakeCsd;
			collData[2] = (i + 2 < numCollData) ? particleCollDataIt++ : &fakeCsd;
			collData[3] = (i + 3 < numCollData) ? particleCollDataIt++ : &fakeCsd;

			Vec4V oldPosV0 = V4LoadU(reinterpret_cast<PxF32*>(&collData[0]->localOldPos));
			Vec4V newPosV0 = V4LoadU(reinterpret_cast<PxF32*>(&collData[0]->localNewPos));
			Vec4V oldPosV1 = V4LoadU(reinterpret_cast<PxF32*>(&collData[1]->localOldPos));
			Vec4V newPosV1 = V4LoadU(reinterpret_cast<PxF32*>(&collData[1]->localNewPos));
			Vec4V oldPosV2 = V4LoadU(reinterpret_cast<PxF32*>(&collData[2]->localOldPos));
			Vec4V newPosV2 = V4LoadU(reinterpret_cast<PxF32*>(&collData[2]->localNewPos));
			Vec4V oldPosV3 = V4LoadU(reinterpret_cast<PxF32*>(&collData[3]->localOldPos));
			Vec4V newPosV3 = V4LoadU(reinterpret_cast<PxF32*>(&collData[3]->localNewPos));

			Vec4V particleMin0 = V4Min(oldPosV0, newPosV0);
			Vec4V particleMax0 = V4Max(oldPosV0, newPosV0);
			Vec4V particleMin1 = V4Min(oldPosV1, newPosV1);
			Vec4V particleMax1 = V4Max(oldPosV1, newPosV1);
			Vec4V particleMin2 = V4Min(oldPosV2, newPosV2);
			Vec4V particleMax2 = V4Max(oldPosV2, newPosV2);
			Vec4V particleMin3 = V4Min(oldPosV3, newPosV3);
			Vec4V particleMax3 = V4Max(oldPosV3, newPosV3);

			Mat44V particleMin44(particleMin0, particleMin1, particleMin2, particleMin3);
			const Mat44V particleMinTrans44 = M44Trnsps(particleMin44);
			Mat44V particleMax44(particleMax0, particleMax1, particleMax2, particleMax3);
			const Mat44V particleMaxTrans44 = M44Trnsps(particleMax44);

			BoolV mask = V4IsGrtr(boundMaxX, particleMinTrans44.col0);
			mask = BAnd(V4IsGrtr(boundMaxY, particleMinTrans44.col1), mask);
			mask = BAnd(V4IsGrtr(boundMaxZ, particleMinTrans44.col2), mask);
			mask = BAnd(V4IsGrtr(particleMaxTrans44.col0, boundMinX), mask);
			mask = BAnd(V4IsGrtr(particleMaxTrans44.col1, boundMinY), mask);
			mask = BAnd(V4IsGrtr(particleMaxTrans44.col2, boundMinZ), mask);

			VecU32V overlap4 = V4U32Sel(mask, u4One, u4Zero);
			V4U32StoreAligned(overlap4, reinterpret_cast<VecU32V*>(&overlapArray[i]));
		}

		particleCollDataIt = &particleCollData[start];
		for(PxU32 k = 0; k < batchSize; k++, ++particleCollDataIt)
		{
			if(overlapArray[k])
			{
				if(!scaledPlanes)
				{
					scalePlanes(scaledPlaneBuf, convexHullData, invScaling);
					scaledPlanes = true;
				}

				collDataV4.localOldPos[v4Count].v3 = particleCollDataIt->localOldPos;
				collDataV4.localNewPos[v4Count].v3 = particleCollDataIt->localNewPos;
				collDataV4.localFlags[v4Count] = particleCollDataIt->localFlags;
				collDataV4.restOffset[v4Count] = particleCollDataIt->restOffset;
				collDataV4.ccTime[v4Count] = particleCollDataIt->ccTime;
				collDataV4.collData[v4Count] = particleCollDataIt;
				v4Count++;
			}

			if(v4Count == 4 || (v4Count > 0 && (k == batchSize - 1)))
			{
				collideWithConvexPlanesSIMD(collDataV4, scaledPlaneBuf, convexHullData->mNbPolygons, proxRadius);

				for(PxU32 j = 0; j < v4Count; j++)
				{
					ParticleCollData* collData = collDataV4.collData[j];
					PxU32 stateFlag = collDataV4.localFlags[j];
					if(stateFlag)
					{
						collData->localFlags |= stateFlag;
						collData->ccTime = collDataV4.ccTime[j];
						collData->localSurfaceNormal = collDataV4.localSurfaceNormal[j].v3;
						collData->localSurfacePos = collDataV4.localSurfacePos[j].v3;
					}
				}
				v4Count = 0;
			}
		}
		start += batchSize;
	}
#else
	ParticleCollData* particleCollDataIt = particleCollData;
	for(PxU32 i = 0; i < numCollData; ++i, ++particleCollDataIt)
	{
		PxBounds3 particleBounds =
		    PxBounds3::boundsOfPoints(particleCollDataIt->localOldPos, particleCollDataIt->localNewPos);

		if(particleBounds.intersects(shapeBounds))
		{
			if(!scaledPlanes)
			{
				scalePlanes(scaledPlaneBuf, convexHullData, invScaling);
				scaledPlanes = true;
			}

			collideWithConvexPlanes(*particleCollDataIt, scaledPlaneBuf, convexHullData->mNbPolygons, proxRadius);
		}
	}
#endif
}

#endif // PX_USE_PARTICLE_SYSTEM_API
