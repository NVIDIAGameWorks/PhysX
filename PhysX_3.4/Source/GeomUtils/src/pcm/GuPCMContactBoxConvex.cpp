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

#include "GuGJKPenetration.h"
#include "GuEPA.h"
#include "GuVecBox.h"
#include "GuVecShrunkBox.h"
#include "GuVecShrunkConvexHull.h"
#include "GuVecShrunkConvexHullNoScale.h"
#include "GuVecConvexHull.h"
#include "GuVecConvexHullNoScale.h"
#include "GuGeometryUnion.h"

#include "GuContactMethodImpl.h"
#include "GuPCMContactGen.h"
#include "GuPCMShapeConvex.h"
#include "GuContactBuffer.h"


namespace physx
{

using namespace Ps::aos;

namespace Gu
{

static bool fullContactsGenerationBoxConvex(const PxVec3& halfExtents, const BoxV& box, ConvexHullV& convexHull, const PsTransformV& transf0, const PsTransformV& transf1, 
									PersistentContact* manifoldContacts, ContactBuffer& contactBuffer, Gu::PersistentContactManifold& manifold, Vec3VArg normal, 
									const Vec3VArg closestA, const Vec3VArg closestB, const FloatVArg contactDist, const bool idtScale, const bool doOverlapTest, Cm::RenderOutput* renderOutput,
									const PxReal toleranceScale)
{
	Gu::PolygonalData polyData0;
	PCMPolygonalBox polyBox0(halfExtents);
	polyBox0.getPolygonalData(&polyData0);
	polyData0.mPolygonVertexRefs = gPCMBoxPolygonData;
	
	Gu::PolygonalData polyData1;
	getPCMConvexData(convexHull, idtScale, polyData1);
	
	Mat33V identity =  M33Identity();
	SupportLocalImpl<BoxV> map0(box, transf0, identity, identity, true);

	PxU8 buff1[sizeof(SupportLocalImpl<ConvexHullV>)];

	SupportLocal* map1 = (idtScale ? static_cast<SupportLocal*>(PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullNoScaleV>)(static_cast<ConvexHullNoScaleV&>(convexHull), transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale)) : 
		static_cast<SupportLocal*>(PX_PLACEMENT_NEW(buff1, SupportLocalImpl<ConvexHullV>)(convexHull, transf1, convexHull.vertex2Shape, convexHull.shape2Vertex, idtScale)));

	PxU32 numContacts = 0;
	if(generateFullContactManifold(polyData0, polyData1, &map0, map1, manifoldContacts, numContacts, contactDist, normal, closestA, closestB, box.getMarginF(), convexHull.getMarginF(), 
		doOverlapTest, renderOutput, toleranceScale))
	{
		if (numContacts > 0)
		{
			//reduce contacts
			manifold.addBatchManifoldContacts(manifoldContacts, numContacts, toleranceScale);

#if	PCM_LOW_LEVEL_DEBUG
			manifold.drawManifold(*renderOutput, transf0, transf1);
#endif

			const Vec3V worldNormal = manifold.getWorldNormal(transf1);

			manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1, contactDist);
		}
		else
		{
			//if doOverlapTest is true, which means GJK/EPA degenerate so we won't have any contact in the manifoldContacts array
			if (!doOverlapTest)
			{
				const Vec3V worldNormal = manifold.getWorldNormal(transf1);

				manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1, contactDist);
			}
		}
		
		return true;
	}

	return false;

}

static bool addGJKEPAContacts(Gu::ShrunkConvexHullV& convexHull, Gu::BoxV& box, const PsMatTransformV& aToB, GjkStatus status,
	Gu::PersistentContact* manifoldContacts, const FloatV replaceBreakingThreshold, Vec3V& closestA, Vec3V& closestB, Vec3V& normal, FloatV& penDep,
	Gu::PersistentContactManifold& manifold)
{
	bool doOverlapTest = false;
	if (status == GJK_CONTACT)
	{
		const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
		const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

		//Add contact to contact stream
		manifoldContacts[0].mLocalPointA = localPointA;
		manifoldContacts[0].mLocalPointB = closestB;
		manifoldContacts[0].mLocalNormalPen = localNormalPen;

		//Add contact to manifold
		manifold.addManifoldPoint(aToB.transformInv(closestA), closestB, localNormalPen, replaceBreakingThreshold);

	}
	else
	{
		PX_ASSERT(status == EPA_CONTACT);

		RelativeConvex<BoxV> epaConvexA(box, aToB);
		LocalConvex<ConvexHullV> epaConvexB(convexHull);

		status = epaPenetration(epaConvexA, epaConvexB, manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints,
			closestA, closestB, normal, penDep);
		if (status == EPA_CONTACT)
		{

			const Vec3V localPointA = aToB.transformInv(closestA);//curRTrans.transformInv(closestA);
			const Vec4V localNormalPen = V4SetW(Vec4V_From_Vec3V(normal), penDep);

			//Add contact to contact stream
			manifoldContacts[0].mLocalPointA = localPointA;
			manifoldContacts[0].mLocalPointB = closestB;
			manifoldContacts[0].mLocalNormalPen = localNormalPen;

			//Add contact to manifold
			manifold.addManifoldPoint(localPointA, closestB, localNormalPen, replaceBreakingThreshold);
		}
		else
		{
			doOverlapTest = true;
		}
	}

	return doOverlapTest;
}

bool pcmContactBoxConvex(GU_CONTACT_METHOD_ARGS)
{
	using namespace Ps::aos;
	

	const PxConvexMeshGeometryLL& shapeConvex = shape1.get<const PxConvexMeshGeometryLL>();
	const PxBoxGeometry& shapeBox = shape0.get<const PxBoxGeometry>();
	
	Gu::PersistentContactManifold& manifold = cache.getManifold();
	Ps::prefetchLine(shapeConvex.hullData);
	
	PX_ASSERT(transform1.q.isSane());
	PX_ASSERT(transform0.q.isSane());

	const Vec3V zeroV = V3Zero();


	const FloatV contactDist = FLoad(params.mContactDistance);
	const Vec3V boxExtents = V3LoadU(shapeBox.halfExtents);

	const Vec3V vScale = V3LoadU_SafeReadW(shapeConvex.scale.scale);	// PT: safe because 'rotation' follows 'scale' in PxMeshScale
	
	//Transfer A into the local space of B
	const PsTransformV transf0 = loadTransformA(transform0);
	const PsTransformV transf1 = loadTransformA(transform1);
	const PsTransformV curRTrans(transf1.transformInv(transf0));
	const PsMatTransformV aToB(curRTrans);

	const PxReal tolerenceLength = params.mToleranceLength;
	const Gu::ConvexHullData* hullData = shapeConvex.hullData;
	const FloatV convexMargin = Gu::CalculatePCMConvexMargin(hullData, vScale, tolerenceLength);
	const FloatV boxMargin = Gu::CalculatePCMBoxMargin(boxExtents, tolerenceLength);

	const FloatV minMargin = FMin(convexMargin, boxMargin);//FMin(boxMargin, convexMargin);
	const FloatV projectBreakingThreshold = FMul(minMargin, FLoad(0.8f));
	const PxU32 initialContacts = manifold.mNumContacts;

	manifold.refreshContactPoints(aToB, projectBreakingThreshold, contactDist);  
	
	//After the refresh contact points, the numcontacts in the manifold will be changed

	const bool bLostContacts = (manifold.mNumContacts != initialContacts);

	PX_UNUSED(bLostContacts);

	if(bLostContacts || manifold.invalidate_BoxConvex(curRTrans, minMargin))	
	{
		
		GjkStatus status = manifold.mNumContacts > 0 ? GJK_UNDEFINED : GJK_NON_INTERSECT;

		Vec3V closestA(zeroV), closestB(zeroV), normal(zeroV); // from a to b
		FloatV penDep = FZero(); 
		
		const QuatV vQuat = QuatVLoadU(&shapeConvex.scale.rotation.x);
		const bool idtScale = shapeConvex.scale.isIdentity();
		Gu::ShrunkConvexHullV convexHull(hullData, V3LoadU(hullData->mCenterOfMass), vScale, vQuat, idtScale);
		convexHull.setMaxMargin(shapeConvex.maxMargin);
		Gu::BoxV box(zeroV, boxExtents);
		
		const RelativeConvex<BoxV> convexA(box, aToB);
		if(idtScale)
		{
			const LocalConvex<ShrunkConvexHullNoScaleV> convexB(*PX_SCONVEX_TO_NOSCALECONVEX(&convexHull));
			status = gjkPenetration<RelativeConvex<BoxV>, LocalConvex<ShrunkConvexHullNoScaleV> >(convexA, convexB, aToB.p, contactDist, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, false);
		}
		else
		{
			const LocalConvex<ShrunkConvexHullV> convexB(convexHull);
			status = gjkPenetration<RelativeConvex<BoxV>, LocalConvex<ShrunkConvexHullV> >(convexA, convexB, aToB.p, contactDist, closestA, closestB, normal, penDep,
				manifold.mAIndice, manifold.mBIndice, manifold.mNumWarmStartPoints, false);

		} 

		manifold.setRelativeTransform(curRTrans); 

		Gu::PersistentContact* manifoldContacts = PX_CP_TO_PCP(contactBuffer.contacts);
	
		if(status == GJK_DEGENERATE)
		{
			return fullContactsGenerationBoxConvex(shapeBox.halfExtents, box, convexHull, transf0, transf1, manifoldContacts, contactBuffer, 
				manifold, normal, closestA, closestB, contactDist, idtScale, true, renderOutput, params.mToleranceLength);
		}
		else if(status == GJK_NON_INTERSECT)
		{
			return false;
		}
		else
		{
			const Vec3V localNor = manifold.mNumContacts ? manifold.getLocalNormal() : V3Zero();

			const FloatV replaceBreakingThreshold = FMul(minMargin, FLoad(0.05f));
			
			//addGJKEPAContacts will increase the number of contacts in manifold. If status == EPA_CONTACT, we need to run epa algorithm and generate closest points, normal and
			//pentration. If epa doesn't degenerate, we will store the contacts information in the manifold. Otherwise, we will return true to do the fallback test
			const bool doOverlapTest = addGJKEPAContacts(convexHull, box, aToB, status, manifoldContacts, replaceBreakingThreshold, closestA, closestB, normal, penDep, manifold);

			//ML: after we refresh the contacts(newContacts) and generate a GJK/EPA contacts(we will store that in the manifold), if the number of contacts is still less than the original contacts,
			//which means we lose too mang contacts and we should regenerate all the contacts in the current configuration
			//Also, we need to look at the existing contacts, if the existing contacts has very different normal than the GJK/EPA contacts,
			//which means we should throw away the existing contacts and do full contact gen
			const bool fullContactGen = FAllGrtr(FLoad(0.707106781f), V3Dot(localNor, normal)) || (manifold.mNumContacts < initialContacts);

			if (fullContactGen || doOverlapTest)
			{
				return fullContactsGenerationBoxConvex(shapeBox.halfExtents, box, convexHull, transf0, transf1, manifoldContacts, contactBuffer, 
					manifold, normal, closestA, closestB, contactDist, idtScale, doOverlapTest, renderOutput, params.mToleranceLength);
			}
			else
			{
				const Vec3V newLocalNor = V3Add(localNor, normal);
				const Vec3V worldNormal = V3Normalize(transf1.rotate(newLocalNor));
				//const Vec3V worldNormal = transf1.rotate(normal);
				manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1, contactDist);
				return true;
			}
		} 
	}
	else if(manifold.getNumContacts()>0)
	{
		const Vec3V worldNormal =  manifold.getWorldNormal(transf1);
		manifold.addManifoldContactsToContactBuffer(contactBuffer, worldNormal, transf1, contactDist);
#if	PCM_LOW_LEVEL_DEBUG
		manifold.drawManifold(*renderOutput, transf0, transf1);
#endif
		return true;
	}

	return false;

}

}//Gu
}//physx
