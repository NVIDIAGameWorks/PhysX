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

#include "foundation/PxVec3.h"
#include "foundation/PxMath.h"
#include "foundation/PxMemory.h"
#include "foundation/PxProfiler.h"

#include "PsUtilities.h"
#include "CmSpatialVector.h"
#include "DyArticulationHelper.h"
#include "DyArticulationReference.h"
#include "DyArticulationFnsSimd.h"
#include "DyArticulationFnsScalar.h"
#include "DyArticulationFnsDebug.h"
#include "DySolverConstraintDesc.h"
#include "PxvDynamics.h"
#include "DyArticulation.h"
#include "PxcRigidBody.h"
#include "CmConeLimitHelper.h"
#include "DySolverConstraint1D.h"
#include "PxcConstraintBlockStream.h"
#include "DySolverConstraint1D.h"
#include "DyArticulationPImpl.h"
#include "PsFoundation.h"

namespace physx
{

namespace Dy
{

void PxcFsFlushVelocity(FsData& matrix);

// we pass this around by value so that when we return from a function the size is unaltered. That means we don't preserve state
// across functions - even though that could be handy to preserve baseInertia and jointTransforms across the solver so that if we 
// need to run position projection  positions they don't get recomputed.

struct PxcFsScratchAllocator
{
	char*   base;
	size_t	size;
	size_t	taken;
	PxcFsScratchAllocator(char* p, size_t s): base(p), size(s), taken(0) {}

	template<typename T>
	static size_t sizeof16()
	{
		return (sizeof(T)+15)&~15;
	}

	template<class T> T* alloc(PxU32 count)
	{
		size_t s = sizeof16<T>();
		PX_ASSERT(taken+s*count <= size);
		T* result = reinterpret_cast<T*>(base+taken);
		taken+=s*count;
		return result;
	}
};

void PxcLtbFactor(FsData& m)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;
	LtbRow* rows = getLtbRows(m);

	for(PxU32 i=m.linkCount; --i>0;)
	{
		LtbRow& b = rows[i];
		PxU32 p = m.parent[i];
		const FsInertia inertia = Fns::invertInertia(b.inertia);
		const Mat33V jResponse = Fns::invertSym33(M33Neg(Fns::computeSIS(inertia, b.j1, b.j1)));
		b.inertia = inertia;
		rows[p].inertia = Fns::multiplySubtract(rows[p].inertia, jResponse, b.j0, b.j0);
		b.jResponse = jResponse;

	}
	rows[0].inertia = Fns::invertInertia(rows[0].inertia);
}

void PxcLtbSolve(const FsData& m, 
				 Vec3V* b,					// rhs error to solve for
				 Cm::SpatialVectorV* y)		// velocity delta output
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	const LtbRow* rows = getLtbRows(m);
	PxMemZero(y, m.linkCount*sizeof(Cm::SpatialVectorV));

	for(PxU32 i=m.linkCount;i-->1;)
	{
		const LtbRow& r = rows[i];
		const PxU32 p = m.parent[i];

		const Vec3V t = V3Sub(b[i], Fns::axisDot(r.j1, y[i]));
		b[i] = t;
		y[p] = Fns::subtract(y[p], Fns::axisMultiply(r.j0, t));
	}

	y[0] = Fns::multiply(rows[0].inertia, y[0]);

	for(PxU32 i=1; i<m.linkCount; i++)
	{
		const LtbRow& r = rows[i];
		const PxU32 p = m.parent[i];

		const Vec3V t = V3Sub(M33MulV3(r.jResponse, b[i]), Fns::axisDot(r.j0, y[p]));
		y[i] = Fns::subtract(Fns::multiply(r.inertia, y[i]), Fns::axisMultiply(r.j1, t));
	}
}

void PxcLtbProject(const FsData& m,
				   Cm::SpatialVectorV* velocity,
				   Vec3V* b)
{
	PX_ASSERT(m.linkCount<=DY_ARTICULATION_MAX_SIZE);
	Cm::SpatialVectorV y[DY_ARTICULATION_MAX_SIZE];

	PxcLtbSolve(m, b, y);

	for(PxU32 i=0;i<m.linkCount;i++)
		velocity[i] -= y[i];
}

void PxcFsPropagateDrivenInertiaSimd(FsData& matrix,
									 const FsInertia* baseInertia,
									 const PxReal* isf,
									 const Mat33V* load,
									 PxcFsScratchAllocator allocator)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	Cm::SpatialVectorV IS[3];

	FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	FsInertia* inertia = allocator.alloc<FsInertia>(matrix.linkCount);
	PxMemCopy(inertia, baseInertia, matrix.linkCount*sizeof(FsInertia));

	for(PxU32 i=matrix.linkCount; --i>0;)
	{
		FsRow& r = rows[i];
		const FsRowAux& a = aux[i];
		const FsJointVectors& jv = jointVectors[i];

		const Mat33V m = Fns::computeSIS(inertia[i], a.S, IS);
		const FloatV f = FLoad(isf[i]);

		const Mat33V D = Fns::invertSym33(Mat33V(V3ScaleAdd(load[i].col0, f, m.col0),
										   V3ScaleAdd(load[i].col1, f, m.col1),
										   V3ScaleAdd(load[i].col2, f, m.col2)));
		r.D = D;

		inertia[matrix.parent[i]] = Fns::addInertia(inertia[matrix.parent[i]], 
													Fns::translateInertia(jv.parentOffset, Fns::multiplySubtract(inertia[i], D,  IS,  r.DSI)));
	}

	getRootInverseInertia(matrix) = Fns::invertInertia(inertia[0]);
}

PX_FORCE_INLINE Cm::SpatialVectorV propagateDrivenImpulse(const FsRow& row, 
														  const FsJointVectors& jv,
														  Vec3V& SZMinusQ, 
														  const Cm::SpatialVectorV& Z,
														  const Vec3V& Q)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	SZMinusQ = V3Sub(V3Add(Z.angular, V3Cross(Z.linear,jv.jointOffset)), Q);
	Cm::SpatialVectorV result = Fns::translateForce(jv.parentOffset, Z - Fns::axisMultiply(row.DSI, SZMinusQ));

	return result;
}

void PxcFsApplyJointDrives(FsData& matrix,
						   const Vec3V* Q)
{				
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);

	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	Cm::SpatialVectorV Z[DY_ARTICULATION_MAX_SIZE];
	Cm::SpatialVectorV dV[DY_ARTICULATION_MAX_SIZE];
	Vec3V SZminusQ[DY_ARTICULATION_MAX_SIZE];

	PxMemZero(Z, matrix.linkCount*sizeof(Cm::SpatialVectorV));

	for(PxU32 i=matrix.linkCount;i-->1;)
		Z[matrix.parent[i]] += propagateDrivenImpulse(rows[i], jointVectors[i], SZminusQ[i], Z[i], Q[i]);

	
	dV[0] = Fns::multiply(getRootInverseInertia(matrix), -Z[0]);

	for(PxU32 i=1;i<matrix.linkCount;i++)
		dV[i] = Fns::propagateVelocity(rows[i], jointVectors[i], SZminusQ[i], dV[matrix.parent[i]], aux[i]);

	Cm::SpatialVectorV* V = getVelocity(matrix);
	for(PxU32 i=0;i<matrix.linkCount;i++)
		V[i] += dV[i];
}

void ArticulationHelper::applyImpulses(	const FsData& matrix,
										Cm::SpatialVectorV* Z,
										Cm::SpatialVectorV* V)
{	
	// note: Z is the negated impulse


	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);
	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	Cm::SpatialVectorV dV[DY_ARTICULATION_MAX_SIZE];
	Vec3V SZ[DY_ARTICULATION_MAX_SIZE];

	for(PxU32 i=matrix.linkCount;i-->1;)
		Z[matrix.parent[i]] += Fns::propagateImpulse(rows[i], jointVectors[i], SZ[i], Z[i], aux[i]);

	dV[0] = Fns::multiply(getRootInverseInertia(matrix), -Z[0]);

	for(PxU32 i=1;i<matrix.linkCount;i++)
		dV[i] = Fns::propagateVelocity(rows[i], jointVectors[i], SZ[i], dV[matrix.parent[i]], aux[i]);

	for(PxU32 i=0;i<matrix.linkCount;i++)
		V[i] += dV[i];
}

void getImpulseResponseSlow(const FsData& matrix, 
							PxU32 linkID0, 
							const Cm::SpatialVectorV& impulse0,
							Cm::SpatialVectorV& deltaV0,
							PxU32 linkID1,
							const Cm::SpatialVectorV& impulse1,
							Cm::SpatialVectorV& deltaV1)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);
	PxU32 stack[DY_ARTICULATION_MAX_SIZE];
	Vec3V SZ[DY_ARTICULATION_MAX_SIZE];

	PxU32 i0, i1, ic;
	
	for(i0 = linkID0, i1 = linkID1; i0!=i1;)	// find common path
	{
		if(i0<i1)
			i1 = matrix.parent[i1];
		else
			i0 = matrix.parent[i0];
	}

	PxU32 common = i0;

	Cm::SpatialVectorV Z0 = -impulse0, Z1 = -impulse1;
	for(i0 = 0; linkID0!=common; linkID0 = matrix.parent[linkID0])
	{
		Z0 = Fns::propagateImpulse(rows[linkID0], jointVectors[linkID0], SZ[linkID0], Z0, aux[linkID0]);
		stack[i0++] = linkID0;
	}

	for(i1 = i0; linkID1!=common; linkID1 = matrix.parent[linkID1])
	{
		Z1 = Fns::propagateImpulse(rows[linkID1], jointVectors[linkID1], SZ[linkID1], Z1, aux[linkID1]);
		stack[i1++] = linkID1;
	}

	Cm::SpatialVectorV Z = Z0 + Z1;
	for(ic = i1; common; common = matrix.parent[common])
	{
		Z = Fns::propagateImpulse(rows[common], jointVectors[common], SZ[common], Z, aux[common]);
		stack[ic++] = common;
	}

	Cm::SpatialVectorV v = Fns::multiply(getRootInverseInertia(matrix), -Z);

	for(PxU32 index = ic; index-->i1 ;)
		v = Fns::propagateVelocity(rows[stack[index]], jointVectors[stack[index]], SZ[stack[index]], v, aux[stack[index]]);

	deltaV1 = v;
	for(PxU32 index = i1; index-->i0 ;)
		deltaV1 = Fns::propagateVelocity(rows[stack[index]], jointVectors[stack[index]], SZ[stack[index]], deltaV1, aux[stack[index]]);

	deltaV0 = v;
	for(PxU32 index = i0; index-->0;)
		deltaV0 = Fns::propagateVelocity(rows[stack[index]], jointVectors[stack[index]], SZ[stack[index]], deltaV0, aux[stack[index]]);
}

void PxcFsGetImpulseResponse(const FsData& matrix,
							 PxU32 linkID,
							 const Cm::SpatialVectorV& impulse,
							 Cm::SpatialVectorV& deltaV)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);
	Vec3V SZ[DY_ARTICULATION_MAX_SIZE];

	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	Cm::SpatialVectorV Z = -impulse;
	
	for(PxU32 i = linkID; i; i = matrix.parent[i])
		Z = Fns::propagateImpulse(rows[i], jointVectors[i], SZ[i], Z, aux[i]);

	deltaV = Fns::multiply(getRootInverseInertia(matrix), -Z);

	PX_ASSERT(rows[linkID].pathToRoot&1);

	for(ArticulationBitField i=rows[linkID].pathToRoot-1; i; i &= (i-1))
	{
		const PxU32 index = ArticulationLowestSetBit(i);
		deltaV = Fns::propagateVelocity(rows[index], jointVectors[index], SZ[index], deltaV, aux[index]);
	}
}

void PxcFsGetImpulseSelfResponse(const FsData& matrix, 
								 PxU32 linkID0, 
								 const Cm::SpatialVectorV& impulse0,
								 Cm::SpatialVectorV& deltaV0,
								 PxU32 linkID1,
								 const Cm::SpatialVectorV& impulse1,
								 Cm::SpatialVectorV& deltaV1)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	PX_ASSERT(linkID0 != linkID1);

	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	// standard case: parent-child limit
	if(matrix.parent[linkID1] == linkID0)
	{
		Vec3V SZ;
		const Cm::SpatialVectorV Z = impulse0 - Fns::propagateImpulse(rows[linkID1], jointVectors[linkID1], SZ, -impulse1, aux[linkID1]);
		PxcFsGetImpulseResponse(matrix, linkID0, Z, deltaV0);
		deltaV1 = Fns::propagateVelocity(rows[linkID1], jointVectors[linkID1], SZ, deltaV0, aux[linkID1]);
	}
	else
		getImpulseResponseSlow(matrix, linkID0, impulse0, deltaV0, linkID1, impulse1, deltaV1);

#if DY_ARTICULATION_DEBUG_VERIFY
	Cm::SpatialVector V[DY_ARTICULATION_MAX_SIZE];
	for(PxU32 i=0;i<matrix.linkCount;i++) V[i] = Cm::SpatialVector::zero();
	ArticulationRef::applyImpulse(matrix,V,linkID0, reinterpret_cast<const Cm::SpatialVector&>(impulse0));
	ArticulationRef::applyImpulse(matrix,V,linkID1, reinterpret_cast<const Cm::SpatialVector&>(impulse1));

	Cm::SpatialVector refV0 = V[linkID0];
	Cm::SpatialVector refV1 = V[linkID1];
#endif
}

namespace
{

	PX_FORCE_INLINE Cm::SpatialVectorV getImpulseResponseSimd(const FsData& matrix, PxU32 linkID, Vec3V lZ, Vec3V aZ)
	{
		PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);
		Vec3V SZ[DY_ARTICULATION_MAX_SIZE];
		PxU32 indices[DY_ARTICULATION_MAX_SIZE], iCount = 0;

		const FsRow*PX_RESTRICT rows = getFsRows(matrix);
		const FsRowAux*PX_RESTRICT aux = getAux(matrix);
		const FsJointVectors* jointVectors = getJointVectors(matrix);

		PX_UNUSED(aux);
		PX_ASSERT(rows[linkID].pathToRoot&1);

		lZ = V3Neg(lZ);
		aZ = V3Neg(aZ);

		for(PxU32 i = linkID; i; i = matrix.parent[i])
		{
			const FsRow& r = rows[i];
			const FsJointVectors& j = jointVectors[i];

			Vec3V sz = V3Add(aZ, V3Cross(lZ, j.jointOffset));
			SZ[iCount] = sz;
			
			lZ = V3NegScaleSub(r.DSI[0].linear, V3GetX(sz), V3NegScaleSub(r.DSI[1].linear, V3GetY(sz), V3NegScaleSub(r.DSI[2].linear, V3GetZ(sz), lZ)));
			aZ = V3NegScaleSub(r.DSI[0].angular, V3GetX(sz), V3NegScaleSub(r.DSI[1].angular, V3GetY(sz), V3NegScaleSub(r.DSI[2].angular, V3GetZ(sz), aZ)));

			aZ = V3Add(aZ, V3Cross(j.parentOffset, lZ));
			indices[iCount++] = i;
		}

		const FsInertia& I = getRootInverseInertia(matrix);

		Vec3V lV = V3Neg(V3Add(M33MulV3(I.ll, lZ), M33MulV3(I.la, aZ)));
		Vec3V aV = V3Neg(V3Add(M33TrnspsMulV3(I.la, lZ), M33MulV3(I.aa, aZ)));

		while(iCount)
		{
			PxU32 i = indices[--iCount];
			const FsRow& r = rows[i];
			const FsJointVectors& j = jointVectors[i];

			lV = V3Sub(lV, V3Cross(j.parentOffset, aV));

			Vec3V n = V3Add(V3Merge(V3Dot(r.DSI[0].linear, lV),  V3Dot(r.DSI[1].linear, lV),  V3Dot(r.DSI[2].linear, lV)),
							V3Merge(V3Dot(r.DSI[0].angular, aV), V3Dot(r.DSI[1].angular, aV), V3Dot(r.DSI[2].angular, aV)));

			n = V3Add(n, M33MulV3(r.D, SZ[iCount]));
			lV = V3Sub(lV, V3Cross(j.jointOffset, n));
			aV = V3Sub(aV, n);
		}

		return Cm::SpatialVectorV(lV, aV);
	}
}
					
void ArticulationHelper::getImpulseResponse(const FsData& matrix,
											PxU32 linkID,
											const Cm::SpatialVectorV& impulse,
											Cm::SpatialVectorV& deltaV)
{
	PX_ASSERT(matrix.linkCount<=DY_ARTICULATION_MAX_SIZE);

	deltaV = getImpulseResponseSimd(matrix, linkID, impulse.linear, impulse.angular);

#if DY_ARTICULATION_DEBUG_VERIFY
	Cm::SpatialVectorV deltaV_;
	PxcFsGetImpulseResponse(matrix, linkID, impulse, deltaV_);
	PX_ASSERT(almostEqual(deltaV_, deltaV,1e-3f));
#endif
}

void ArticulationHelper::getImpulseSelfResponse(const FsData& matrix,
												PxU32 linkID0,
												const Cm::SpatialVectorV& impulse0,
												Cm::SpatialVectorV& deltaV0,
												PxU32 linkID1,
												const Cm::SpatialVectorV& impulse1,
												Cm::SpatialVectorV& deltaV1)
{
	PX_ASSERT(linkID0 != linkID1);

	const FsRow* rows = getFsRows(matrix);
	const FsRowAux* aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);

	PX_UNUSED(aux);

	Cm::SpatialVectorV& dV0 = deltaV0, 
				  & dV1 = deltaV1;

	// standard case: parent-child limit
	if(matrix.parent[linkID1] == linkID0)
	{
		const FsRow& r = rows[linkID1];
		const FsJointVectors& j = jointVectors[linkID1];

		Vec3V lZ = V3Neg(impulse1.linear),
			  aZ = V3Neg(impulse1.angular);

		Vec3V sz = V3Add(aZ, V3Cross(lZ, j.jointOffset));
		
		lZ = V3Sub(lZ, V3ScaleAdd(r.DSI[0].linear, V3GetX(sz), V3ScaleAdd(r.DSI[1].linear, V3GetY(sz), V3Scale(r.DSI[2].linear, V3GetZ(sz)))));
		aZ = V3Sub(aZ, V3ScaleAdd(r.DSI[0].angular, V3GetX(sz), V3ScaleAdd(r.DSI[1].angular, V3GetY(sz), V3Scale(r.DSI[2].angular, V3GetZ(sz)))));

		aZ = V3Add(aZ, V3Cross(j.parentOffset, lZ));

		lZ = V3Sub(impulse0.linear, lZ);
		aZ = V3Sub(impulse0.angular, aZ);

		dV0 = getImpulseResponseSimd(matrix, linkID0, lZ, aZ);

		Vec3V aV = dV0.angular;
		Vec3V lV = V3Sub(dV0.linear, V3Cross(j.parentOffset, aV));

		Vec3V n = V3Add(V3Merge(V3Dot(r.DSI[0].linear, lV),  V3Dot(r.DSI[1].linear, lV),  V3Dot(r.DSI[2].linear, lV)),
						V3Merge(V3Dot(r.DSI[0].angular, aV), V3Dot(r.DSI[1].angular, aV), V3Dot(r.DSI[2].angular, aV)));

		n = V3Add(n, M33MulV3(r.D, sz));
		lV = V3Sub(lV, V3Cross(j.jointOffset, n));
		aV = V3Sub(aV, n);

		dV1 = Cm::SpatialVectorV(lV, aV);
	}
	else
		getImpulseResponseSlow(matrix, linkID0, impulse0, deltaV0, linkID1, impulse1, deltaV1);

#if DY_ARTICULATION_DEBUG_VERIFY
	Cm::SpatialVectorV dV0_, dV1_;
	PxcFsGetImpulseSelfResponse(matrix, linkID0, impulse0, dV0_, linkID1, impulse1, dV1_);

	PX_ASSERT(almostEqual(dV0_, dV0, 1e-3f));
	PX_ASSERT(almostEqual(dV1_, dV1, 1e-3f));
#endif
}

void PxcLtbComputeJv(Vec3V* jv, const FsData& m, const Cm::SpatialVectorV* velocity)
{
	const LtbRow* rows = getLtbRows(m);
	const FsRow* fsRows = getFsRows(m);
	const FsJointVectors* jointVectors = getJointVectors(m);

	PX_UNUSED(rows);
	PX_UNUSED(fsRows);

	for(PxU32 i=1;i<m.linkCount;i++)
	{
		Cm::SpatialVectorV pv = velocity[m.parent[i]], v = velocity[i];

		Vec3V parentOffset = V3Add(jointVectors[i].jointOffset, jointVectors[i].parentOffset);

		Vec3V k0v = V3Add(pv.linear, V3Cross(pv.angular, parentOffset)),
			  k1v = V3Add(v.linear,  V3Cross(v.angular,jointVectors[i].jointOffset));
		jv[i] = V3Sub(k0v, k1v);
	}
}

void ArticulationHelper::saveVelocity(const ArticulationSolverDesc& d)
{
	Vec3V b[DY_ARTICULATION_MAX_SIZE];
	FsData& m = *d.fsData;

	Cm::SpatialVectorV* velocity = getVelocity(m);
	PxcFsFlushVelocity(m);

	// save off the motion velocity

	for(PxU32 i=0;i<m.linkCount;i++)
	{
		d.motionVelocity[i] = velocity[i];
		PX_ASSERT(isFiniteVec3V(velocity[i].linear));
		PX_ASSERT(isFiniteVec3V(velocity[i].angular));
	}

	// and now re-solve to use the unbiased velocities

	PxcLtbComputeJv(b, m, velocity);
	PxcLtbProject(m, velocity, b);

#if DY_ARTICULATION_DEBUG_VERIFY
	for(PxU32 i=0;i<m.linkCount;i++)
		getRefVelocity(m)[i] = velocity[i];
#endif
}

void PxcFsComputeJointLoadsSimd(const FsData& matrix,
								const FsInertia*PX_RESTRICT baseInertia,
								Mat33V*PX_RESTRICT load,
								const PxReal*PX_RESTRICT isf_,
								PxU32 linkCount,
								PxU32 maxIterations,
								PxcFsScratchAllocator allocator)
{
	// dsequeira: this is really difficult to optimize on XBox: not inlining generates lots of LHSs, 
	// inlining generates lots of cache misses because the fn is so huge (almost 2000 instrs.) 
	// Timing says even for 1 iteration the cache misses are slighly preferable for a
	// 20-bone articulation, for more iters it's *much* better to take the cache misses.
	//
	// about 400 instructions come from unnecessary and inexplicable branch checks

	if(!maxIterations)
		return;

	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	FloatV isf[DY_ARTICULATION_MAX_SIZE];

	for(PxU32 i=1;i<linkCount;i++)
		isf[i] = FLoad(isf_[i]);

	FsInertia*PX_RESTRICT inertia = allocator.alloc<FsInertia>(linkCount);
	FsInertia*PX_RESTRICT contribToParent = allocator.alloc<FsInertia>(linkCount);

	const FsRow*PX_RESTRICT row = getFsRows(matrix); 
	const FsRowAux*PX_RESTRICT aux = getAux(matrix);
	const FsJointVectors* jointVectors = getJointVectors(matrix);
	
	PX_UNUSED(row);

	// gets rid of about 200 LHSs, need to change the matrix format to make this part of it
	PxU64 parent[DY_ARTICULATION_MAX_SIZE];
	for(PxU32 i=0;i<linkCount;i++)
		parent[i] = matrix.parent[i];

	while(maxIterations--)
	{
		PxMemCopy(inertia, baseInertia, sizeof(FsInertia)*linkCount);
			
		for(PxU32 i=linkCount;i-->1;)
		{
			const Cm::SpatialVectorV*PX_RESTRICT S = aux[i].S;

			Ps::prefetch(&load[i-1]);
			Ps::prefetch(&jointVectors[i-1]);
			const FsInertia tmp = Fns::propagate(inertia[i], S, load[i], isf[i]);
			inertia[parent[i]] = Fns::addInertia(inertia[parent[i]], Fns::translateInertia(jointVectors[i].parentOffset, tmp));
			contribToParent[i] = tmp;
		}

		for(PxU32 i=1;i<linkCount;i++)
		{
			const Cm::SpatialVectorV*PX_RESTRICT S = aux[i].S;

			const FsInertia rootwardInertia = Fns::subtractInertia(Fns::translateInertia(V3Neg(jointVectors[i].parentOffset), inertia[parent[i]]), contribToParent[i]);
			const FsInertia tmp = Fns::propagate(rootwardInertia, S, load[i], isf[i]);
			load[i] = Fns::computeDriveInertia(inertia[i], rootwardInertia, S);
			inertia[i] = Fns::addInertia(inertia[i], tmp);
		}
	}
}

PxU32 ArticulationHelper::getFsDataSize(PxU32 linkCount)
{
	return sizeof(FsInertia) + sizeof(FsRow) * linkCount;
}

PxU32 ArticulationHelper::getLtbDataSize(PxU32 linkCount)
{
	return sizeof(LtbRow) * linkCount;
}

void ArticulationHelper::prepareDataBlock(	FsData& fsData,
										    const ArticulationLink* links, 
											PxU16 linkCount,
											PxTransform* poses,
										 	FsInertia* baseInertia,
											ArticulationJointTransforms* jointTransforms,
											PxU32 expectedSize)
{
	PxU32 stateSize = sizeof(FsData)
					+ sizeof(Cm::SpatialVectorV) * linkCount
					+ sizeof(Cm::SpatialVectorV) * linkCount
					+ sizeof(Vec3V)			 * linkCount
					+ sizeof(PxReal)		 * ((linkCount + 15) & 0xfffffff0);

	PxU32 jointVectorSize = sizeof(FsJointVectors) * linkCount;

	PxU32 fsDataSize  = getFsDataSize(linkCount);
	PxU32 ltbDataSize = getLtbDataSize(linkCount);

	PxU32 totalSize	= stateSize 
					+ jointVectorSize
					+ fsDataSize 
					+ ltbDataSize
					+ sizeof(Cm::SpatialVectorV) * linkCount
					+ sizeof(FsRowAux)    * linkCount;

	PX_UNUSED(totalSize);
	PX_UNUSED(expectedSize);
	PX_ASSERT(expectedSize == 0 || totalSize == expectedSize);

	PxMemZero(&fsData, stateSize);
	fsData.jointVectorOffset	= PxU16(stateSize);
	fsData.fsDataOffset			= PxU16(stateSize+jointVectorSize);
	fsData.ltbDataOffset		= PxU16(stateSize+jointVectorSize+fsDataSize);
	fsData.linkCount			= linkCount;

	for(PxU32 i=1;i<linkCount;i++)
		fsData.parent[i] = PxU8(links[i].parent);
	fsData.deferredZ = Cm::SpatialVectorV(PxZero);

	Cm::SpatialVector* velocity = reinterpret_cast<Cm::SpatialVector*>(getVelocity(fsData));

	PxMemZero(baseInertia, sizeof(FsInertia)*linkCount);

	PxReal* maxPenBias = getMaxPenBias(fsData);

	for(PxU32 i=0;i<linkCount;i++)
	{
		if((i+2)<linkCount)
		{
			Ps::prefetch(links[i+2].bodyCore);
			Ps::prefetch(links[i+2].inboundJoint);
		}
		PxsBodyCore& core = *links[i].bodyCore;
		poses[i] = core.body2World;
		velocity[i] = Cm::SpatialVector(core.linearVelocity, core.angularVelocity);
		setInertia(baseInertia[i], core, core.body2World);
		maxPenBias[i] = core.maxPenBias;

		if(i)
			setJointTransforms(jointTransforms[i], poses[links[i].parent], core.body2World, *links[i].inboundJoint);
	}

	FsJointVectors* jointVectors = getJointVectors(fsData);
	for(PxU32 i=1;i<linkCount;i++)
	{
		PX_ALIGN(16, PxVec3) parentOffset = poses[i].p - poses[fsData.parent[i]].p;
		PX_ALIGN(16, PxVec3) jointOffset = jointTransforms[i].cB2w.p - poses[i].p;
		jointVectors[i].parentOffset = V3LoadA(parentOffset);
		jointVectors[i].jointOffset = V3LoadA(jointOffset);
	}
}

PxU32 ArticulationHelper::computeUnconstrainedVelocities(	const ArticulationSolverDesc& desc,
															PxReal dt,
															PxcConstraintBlockStream& stream,
															PxSolverConstraintDesc* constraintDesc,
															PxU32& acCount,
															PxsConstraintBlockManager& constraintBlockManager,
															const PxVec3& gravity, PxU64 contextID)
{
	PX_UNUSED(contextID);
	const ArticulationLink* links = desc.links;
	PxU16 linkCount = desc.linkCount;
	FsData& fsData = *desc.fsData;
	PxTransform* poses = desc.poses;

	PxcFsScratchAllocator allocator(desc.scratchMemory, desc.scratchMemorySize);
	FsInertia*						PX_RESTRICT baseInertia = allocator.alloc<FsInertia>(desc.linkCount);
	ArticulationJointTransforms*	PX_RESTRICT jointTransforms = allocator.alloc<ArticulationJointTransforms>(desc.linkCount);

	{
		PX_PROFILE_ZONE("Articulations.prepareDataBlock", contextID);
		prepareDataBlock(fsData, links, linkCount, poses, baseInertia, jointTransforms, desc.totalDataSize);
	}

	const PxReal recipDt = 1.0f/dt;

	Cm::SpatialVectorV* velocity = getVelocity(fsData);

	{

		PX_PROFILE_ZONE("Articulations.setupProject", contextID);

		PxMemZero(getLtbRows(fsData), getLtbDataSize(linkCount));
		prepareLtbMatrix(fsData, baseInertia, poses, jointTransforms, recipDt);

		PxcLtbFactor(fsData);
	
		Vec3V b[DY_ARTICULATION_MAX_SIZE];
		PxcLtbComputeJv(b, fsData, velocity);

		LtbRow* rows = getLtbRows(fsData);
		for(PxU32 i=1;i<linkCount;i++)
			b[i] = V3Add(b[i], rows[i].jC);

		PxcLtbProject(fsData, velocity, b);
	}

	{
		PX_PROFILE_ZONE("Articulations.prepareFsData", contextID);
		PxMemZero(addAddr<void*>(&fsData,fsData.fsDataOffset), getFsDataSize(linkCount));
		prepareFsData(fsData, links);
	}

	{
		PX_PROFILE_ZONE("Articulations.setupDrives", contextID);
	
		if(!(desc.core->externalDriveIterations & 0x80000000))
			PxMemZero(desc.externalLoads, sizeof(Mat33V) * linkCount);

		if(!(desc.core->internalDriveIterations & 0x80000000))
			PxMemZero(desc.internalLoads, sizeof(Mat33V) * linkCount);

		PxReal				isf[DY_ARTICULATION_MAX_SIZE], esf[DY_ARTICULATION_MAX_SIZE];			// spring factors
		Vec3V				drive[DY_ARTICULATION_MAX_SIZE];

		bool externalEqualsInternalCompliance = (desc.core->internalDriveIterations&0xffff) == (desc.core->externalDriveIterations&0xffff);
		for(PxU32 i=1;i<linkCount;i++)
		{
			const ArticulationJointCore& j = *links[i].inboundJoint;
			isf[i] = (1 + j.damping * dt + j.spring * dt * dt) * getResistance(j.internalCompliance);
			esf[i] = (1 + j.damping * dt + j.spring * dt * dt) * getResistance(j.externalCompliance);

			externalEqualsInternalCompliance = externalEqualsInternalCompliance && j.internalCompliance == j.externalCompliance;
		}

		{
			PX_PROFILE_ZONE("Articulations.jointInternalLoads", contextID);
			PxcFsComputeJointLoadsSimd(fsData, baseInertia, desc.internalLoads, isf, linkCount, desc.core->internalDriveIterations&0xffff, allocator);
			
		}

		{
			PX_PROFILE_ZONE("Articulations.propagateDrivenInertia", contextID);
			PxcFsPropagateDrivenInertiaSimd(fsData, baseInertia, isf, desc.internalLoads, allocator);
		}

		{
			PX_PROFILE_ZONE("Articulations.computeJointDrives", contextID);
			computeJointDrives(fsData, drive, links, poses, jointTransforms, desc.internalLoads, dt);
		}

		{
			PX_PROFILE_ZONE("Articulations.applyJointDrives", contextID);
			PxcFsApplyJointDrives(fsData, drive);
		}

		if(!externalEqualsInternalCompliance)
		{
			{
				PX_PROFILE_ZONE("Articulations.jointExternalLoads", contextID);
				PxcFsComputeJointLoadsSimd(fsData, baseInertia, desc.externalLoads, esf, linkCount, desc.core->externalDriveIterations&0xffff, allocator);
			}

			{
				PX_PROFILE_ZONE("Articulations.propagateDrivenInertia", contextID);
				PxcFsPropagateDrivenInertiaSimd(fsData, baseInertia, esf, desc.externalLoads, allocator);
			}
		}
	}

	{
		PX_PROFILE_ZONE("Articulations.applyExternalImpulses", contextID);
		Cm::SpatialVectorV	Z[DY_ARTICULATION_MAX_SIZE];

		FloatV h = FLoad(dt);

		Cm::SpatialVector* acceleration = desc.acceleration;

		const Vec3V vGravity = V3LoadU(gravity);

		for(PxU32 i=0;i<linkCount;i++)
		{
			Vec3V linearAccel = V3LoadA(acceleration[i].linear);

			if (!(desc.links[i].body->mInternalFlags & PxcRigidBody::eDISABLE_GRAVITY))
				linearAccel = V3Add(linearAccel, vGravity);
			Cm::SpatialVectorV a(linearAccel, V3LoadA(acceleration[i].angular));
			Z[i] = -ArticulationFnsSimd<ArticulationFnsSimdBase>::multiply(baseInertia[i], a) * h;
			//KS - zero accelerations to ensure they don't get re-applied next frame if nothing touches them again.
			acceleration[i].linear = PxVec3(0.f); acceleration[i].angular = PxVec3(0.f);
		}

		applyImpulses(fsData, Z, getVelocity(fsData));
	}

	// save off the motion velocity in case there are no constraints with the articulation

	PxMemCopy(desc.motionVelocity, velocity, linkCount*sizeof(Cm::SpatialVectorV));

	// set up for deferred-update solve
	
	fsData.dirty = 0;

	// solver progress counters
	fsData.maxSolverNormalProgress		= 0;
	fsData.maxSolverFrictionProgress	= 0;
	fsData.solverProgress				= 0;


#if DY_ARTICULATION_DEBUG_VERIFY
	for(PxU32 i=0;i<linkCount;i++)
		getRefVelocity(fsData)[i] = getVelocity(fsData)[i];
#endif

	{
		PX_PROFILE_ZONE("Articulations.setupConstraints", contextID);
		return setupSolverConstraints(fsData, desc.solverDataSize, stream, constraintDesc, links, jointTransforms, dt, acCount, constraintBlockManager);
	}
}

void ArticulationHelper::initializeDriveCache(	FsData& fsData,
												PxU16 linkCount,
												const ArticulationLink* links,
												PxReal compliance,
												PxU32 iterations,
												char* scratchMemory,
												PxU32 scratchMemorySize)
{
	PxcFsScratchAllocator allocator(scratchMemory, scratchMemorySize);
	FsInertia*						PX_RESTRICT baseInertia = allocator.alloc<FsInertia>(linkCount);
	ArticulationJointTransforms*	PX_RESTRICT jointTransforms = allocator.alloc<ArticulationJointTransforms>(linkCount);
	PxTransform*					PX_RESTRICT poses = allocator.alloc<PxTransform>(linkCount);
	Mat33V*							PX_RESTRICT jointLoads = allocator.alloc<Mat33V>(linkCount);

	PxReal								springFactor[DY_ARTICULATION_MAX_SIZE];			// spring factors

	prepareDataBlock(fsData, links, linkCount, poses, baseInertia, jointTransforms, 0);

	PxMemZero(addAddr<void*>(&fsData,fsData.fsDataOffset), getFsDataSize(linkCount));
	prepareFsData(fsData, links);

	springFactor[0] = 0.0f;
	for(PxU32 i=1;i<linkCount;i++)
		springFactor[i] = getResistance(compliance);

	PxMemZero(jointLoads, sizeof(Mat33V)*linkCount);
	PxcFsComputeJointLoadsSimd(fsData, baseInertia, jointLoads, springFactor, linkCount, iterations&0xffff, allocator);
	PxcFsPropagateDrivenInertiaSimd(fsData, baseInertia, springFactor, jointLoads, allocator);
}

void ArticulationHelper::updateBodies(const ArticulationSolverDesc& desc, PxReal dt)
{
	FsData& fsData = *desc.fsData;
	const ArticulationCore& core = *desc.core;
	const ArticulationLink* links = desc.links;
	PxTransform* poses = desc.poses;
	Cm::SpatialVectorV* motionVelocity = desc.motionVelocity;

	Vec3V b[DY_ARTICULATION_MAX_SIZE];
	
	PxU32 linkCount = fsData.linkCount;

	PxcFsFlushVelocity(fsData);
	PxcLtbComputeJv(b, fsData, getVelocity(fsData));
	PxcLtbProject(fsData, getVelocity(fsData), b);

	// update positions
	PxcFsScratchAllocator allocator(desc.scratchMemory, desc.scratchMemorySize);
	PxTransform*		PX_RESTRICT oldPose = allocator.alloc<PxTransform>(desc.linkCount);

	for(PxU32 i=0;i<linkCount;i++)
	{
		const PxVec3& lv = reinterpret_cast<PxVec3&>(motionVelocity[i].linear);
		const PxVec3& av = reinterpret_cast<PxVec3&>(motionVelocity[i].angular);
		oldPose[i] = poses[i];
		poses[i] = PxTransform(poses[i].p + lv * dt, Ps::exp(av*dt) * poses[i].q);
	}

	bool projected = false;
	const PxReal recipDt = 1.0f/dt;

	FsInertia*						PX_RESTRICT baseInertia = allocator.alloc<FsInertia>(desc.linkCount);
	ArticulationJointTransforms*	PX_RESTRICT jointTransforms = allocator.alloc<ArticulationJointTransforms>(desc.linkCount);

	for(PxU32 iterations = 0; iterations < core.maxProjectionIterations; iterations++)
	{
		PxReal maxSeparation = -PX_MAX_F32;
		for(PxU32 i=1;i<linkCount;i++)
		{
			const ArticulationJointCore& j = *links[i].inboundJoint;
			maxSeparation = PxMax(maxSeparation,
								  (poses[links[i].parent].transform(j.parentPose).p -
								   poses[i].transform(j.childPose).p).magnitude());
		}

		if(maxSeparation<=core.separationTolerance)
			break;

		projected = true;

		// we go around again, finding velocities which pull us back together - this
		// form of projection is momentum-preserving but slow compared to hierarchical
		// projection

		PxMemZero(baseInertia, sizeof(FsInertia)*linkCount);

		ArticulationHelper::setInertia(baseInertia[0], *links[0].bodyCore, poses[0]);
		for(PxU32 i=1;i<linkCount;i++)
		{
			ArticulationHelper::setInertia(baseInertia[i], *links[i].bodyCore, poses[i]);
			ArticulationHelper::setJointTransforms(jointTransforms[i], poses[links[i].parent], poses[i], *links[i].inboundJoint);
		}

		ArticulationHelper::prepareLtbMatrix(fsData, baseInertia, poses, jointTransforms, recipDt);
		PxcLtbFactor(fsData);

		LtbRow* rows = getLtbRows(fsData);

		for(PxU32 i=1;i<linkCount;i++)
			b[i] = rows[i].jC;

		PxMemZero(motionVelocity, linkCount*sizeof(Cm::SpatialVectorV));

		PxcLtbProject(fsData, motionVelocity, b);

		for(PxU32 i=0;i<linkCount;i++)
		{
			const PxVec3& lv = reinterpret_cast<PxVec3&>(motionVelocity[i].linear);
			const PxVec3& av = reinterpret_cast<PxVec3&>(motionVelocity[i].angular);
			poses[i] = PxTransform(poses[i].p + lv * dt,  Ps::exp(av*dt) * poses[i].q);
		}
	}

	if(projected)
	{
		// recompute motion velocities.
		for(PxU32 i=0;i<linkCount;i++)
		{
			motionVelocity[i].linear = V3LoadU((poses[i].p - oldPose[i].p) * recipDt);
			motionVelocity[i].angular = V3LoadU(Ps::log(poses[i].q * oldPose[i].q.getConjugate()) * recipDt);
		}
	}

	Cm::SpatialVectorV* velocity = getVelocity(fsData);
	for(PxU32 i=0;i<linkCount;i++)
	{
		links[i].bodyCore->body2World = poses[i];

		V3StoreA(velocity[i].linear,  links[i].bodyCore->linearVelocity);
		V3StoreA(velocity[i].angular, links[i].bodyCore->angularVelocity);
	}
}

void ArticulationHelper::setInertia(FsInertia& inertia,
									const PxsBodyCore& body,
									const PxTransform& pose)
{
	// assumes that elements that are supposed to be zero (i.e. la matrix and off diagonal elements of ll) are zero

	const PxMat33 R(pose.q);
	const PxVec3& v = body.inverseInertia;
	const PxReal m = 1.0f/body.inverseMass;
	V3WriteX(inertia.ll.col0, m);
	V3WriteY(inertia.ll.col1, m);
	V3WriteZ(inertia.ll.col2, m);

	PX_ALIGN_PREFIX(16) PxMat33 PX_ALIGN_SUFFIX(16) alignedInertia = R * PxMat33::createDiagonal(PxVec3(1.0f/v.x, 1.0f/v.y, 1.0f/v.z)) * R.getTranspose();
	alignedInertia = (alignedInertia + alignedInertia.getTranspose())*0.5f;
	inertia.aa = Mat33V_From_PxMat33(alignedInertia);
}

void ArticulationHelper::setJointTransforms(ArticulationJointTransforms& transforms,
											const PxTransform& parentPose,
											const PxTransform& childPose,
											const ArticulationJointCore& joint)
{
	transforms.cA2w = parentPose.transform(joint.parentPose);
	transforms.cB2w = childPose.transform(joint.childPose);
	transforms.cB2cA = transforms.cA2w.transformInv(transforms.cB2w);
	if(transforms.cB2cA.q.w<0)	// the relative quat must be the short way round for limits to work...
	{
		transforms.cB2cA.q	= -transforms.cB2cA.q;
		transforms.cB2w.q	= -transforms.cB2w.q;
	}
}

void ArticulationHelper::prepareLtbMatrix(	FsData& fsData,
											const FsInertia* baseInertia,
											const PxTransform* poses,
											const ArticulationJointTransforms* jointTransforms,
											PxReal recipDt)
{
	PxU32 linkCount = fsData.linkCount;
	LtbRow* rows = getLtbRows(fsData);

	rows[0].inertia = baseInertia[0];

	const PxVec3 axis[3] = { PxVec3(1.0f,0.0f,0.0f), PxVec3(0.0f,1.0f,0.0f), PxVec3(0.0f,0.0f,1.0f) };
	for(PxU32 i=1;i<linkCount;i++)
	{
		rows[i].inertia = baseInertia[i];
		const ArticulationJointTransforms& s = jointTransforms[i];

		const PxU32 p = fsData.parent[i];

		// we put the action point of the constraint at the root of the child

		const PxVec3 ra = s.cB2w.p - poses[p].p;
		const PxVec3 rb = s.cB2w.p - poses[i].p;

		// A bit different from the 1D solver, 
		// there we use a formulation	j0.v0 - j1.v1 + c = 0
		// here we use the homogeneous	j0.v0 + j1.v1 + c = 0
		
		const PxVec3 error = (s.cA2w.p - s.cB2w.p) * 0.99f;

		Cm::SpatialVectorV* j0 = rows[i].j0;
		Cm::SpatialVectorV* j1 = rows[i].j1;

		for(PxU32 j=0;j<3;j++)
		{
			PxVec3 n = axis[j];
			j0[j] = Cm::SpatialVector(n, ra.cross(n));
			j1[j] = Cm::SpatialVector(-n, -rb.cross(n));
		}

		rows[i].jC = V3LoadU(error*recipDt);
	}
}

void ArticulationHelper::prepareFsData(FsData& fsData, const ArticulationLink* links)
{
	typedef ArticulationFnsSimd<ArticulationFnsSimdBase> Fns;

	PxU32 linkCount = fsData.linkCount;
	FsRow* rows = getFsRows(fsData);
	FsRowAux* aux = getAux(fsData);
	const FsJointVectors* jointVectors = getJointVectors(fsData);

	rows[0].children = links[0].children;
	rows[0].pathToRoot = 1;

	PX_ALIGN_PREFIX(16) PxVec4 v[] PX_ALIGN_SUFFIX(16) = { PxVec4(1.f,0,0,0), PxVec4(0,1.f,0,0), PxVec4(0,0,1.f,0) } ;
	const Vec3V* axes = reinterpret_cast<const Vec3V*>(v);

	for(PxU32 i=1;i<linkCount;i++)
	{
		PxU32 p = links[i].parent;
		FsRow& r = rows[i];
		FsRowAux& a = aux[i];

		PX_UNUSED(p);

		r.children = links[i].children;
		r.pathToRoot = links[i].pathToRoot;
		
		const Vec3V jointOffset =	jointVectors[i].jointOffset;
		
		// the joint coords are world oriented, located at the joint.
		a.S[0] = Fns::translateMotion(jointOffset, Cm::SpatialVectorV(V3Zero(), axes[0]));
		a.S[1] = Fns::translateMotion(jointOffset, Cm::SpatialVectorV(V3Zero(), axes[1]));
		a.S[2] = Fns::translateMotion(jointOffset, Cm::SpatialVectorV(V3Zero(), axes[2]));
	}
}

PX_FORCE_INLINE PxReal ArticulationHelper::getResistance(PxReal compliance)
{
	PX_ASSERT(compliance>0);
	return 1.0f/compliance;
}

void ArticulationHelper::createHardLimit(	const FsData& fsData,
											const ArticulationLink* links,
											PxU32 linkIndex,
											SolverConstraint1DExt& s, 
											const PxVec3& axis, 
											PxReal err,
											PxReal recipDt)
{
	init(s, PxVec3(0), PxVec3(0), axis, axis, 0, PX_MAX_F32);

	ArticulationHelper::getImpulseSelfResponse(fsData, 
												  links[linkIndex].parent,Cm::SpatialVector(PxVec3(0), axis), s.deltaVA,
												  linkIndex, Cm::SpatialVector(PxVec3(0), -axis), s.deltaVB);

	const PxReal unitResponse = axis.dot(reinterpret_cast<PxVec3&>(s.deltaVA.angular)) - axis.dot(reinterpret_cast<PxVec3&>(s.deltaVB.angular));
	if(unitResponse<0.0f)
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Warning: articulation ill-conditioned or under severe stress, joint limit ignored");

	const PxReal recipResponse = unitResponse>0.0f ? 1.0f/unitResponse : 0.0f;

	s.constant = recipResponse * -err * recipDt;
	s.unbiasedConstant = err>0.0f ? s.constant : 0.0f;
	s.velMultiplier = -recipResponse;
	s.impulseMultiplier = 1.0f;
}

void ArticulationHelper::createTangentialSpring(const FsData& fsData,
												const ArticulationLink* links,
												PxU32 linkIndex,
												SolverConstraint1DExt& s, 
												const PxVec3& axis, 
												PxReal stiffness,
												PxReal damping,
												PxReal dt)
{
	init(s, PxVec3(0), PxVec3(0), axis, axis, -PX_MAX_F32, PX_MAX_F32);

	Cm::SpatialVector axis6(PxVec3(0), axis);
	PxU32 parent = links[linkIndex].parent;
	getImpulseSelfResponse(fsData, parent, axis6, s.deltaVA, linkIndex, -axis6, s.deltaVB);

	const PxReal unitResponse = axis.dot(reinterpret_cast<PxVec3&>(s.deltaVA.angular)) - axis.dot(reinterpret_cast<PxVec3&>(s.deltaVB.angular));
	if(unitResponse<0.0f)
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Warning: articulation ill-conditioned or under severe stress, tangential spring ignored");
	const PxReal recipResponse = unitResponse>0.0F ? 1.0f/unitResponse : 0.0f;

	// this is a specialization of the spring code in setSolverConstants() for acceleration springs.
	// general case is  b = dt * (c.mods.spring.damping * c.velocityTarget - c.mods.spring.stiffness * geomError);
    // but geomError and velocityTarget are both zero

	const PxReal a = dt * dt * stiffness + dt * damping;
    const PxReal x = 1.0f/(1.0f+a);
    s.constant = s.unbiasedConstant = 0.0f;
    s.velMultiplier = -x * recipResponse * a;
    s.impulseMultiplier = 1.0f - x;
}

PxU32 ArticulationHelper::setupSolverConstraints(	FsData& fsData, PxU32 solverDataSize,
													PxcConstraintBlockStream& stream,
													PxSolverConstraintDesc* constraintDesc,
													const ArticulationLink* links,
													const ArticulationJointTransforms* jointTransforms,
													PxReal dt,
													PxU32& acCount,
													PxsConstraintBlockManager& constraintBlockManager)
{
	acCount = 0;

	const PxU16 linkCount = fsData.linkCount;
	PxU32 descCount = 0;
	const PxReal recipDt = 1.0f/dt;

	const PxConstraintInvMassScale ims(1.0f, 1.0f, 1.0f, 1.0f);

	for(PxU16 i=1;i<linkCount;i++)
	{
		const ArticulationJointCore& j = *links[i].inboundJoint;

		if(i+1<linkCount)
		{
			Ps::prefetch(links[i+1].inboundJoint, sizeof (ArticulationJointCore));
			Ps::prefetch(&jointTransforms[i+1], sizeof(ArticulationJointTransforms));
		}
		
		if(!(j.twistLimited || j.swingLimited))
			continue;

		PxQuat swing, twist;
		Ps::separateSwingTwist(jointTransforms[i].cB2cA.q, swing, twist);
	
		Cm::ConeLimitHelper eh(j.tanQSwingY, j.tanQSwingZ, j.tanQSwingPad);
		PxVec3 swingLimitAxis;
		PxReal swingLimitError = 0.0f;

		const bool swingLimited = j.swingLimited && eh.getLimit(swing, swingLimitAxis, swingLimitError);
		const bool tangentialStiffness = swingLimited && (j.tangentialStiffness>0 || j.tangentialDamping>0);

		const PxVec3 twistAxis = jointTransforms[i].cB2w.rotate(PxVec3(1.0f,0,0));
		const PxReal tqTwistAngle = Ps::tanHalf(twist.x, twist.w);

		const bool twistLowerLimited = j.twistLimited && tqTwistAngle < Cm::tanAdd(j.tanQTwistLow, j.tanQTwistPad);
		const bool twistUpperLimited = j.twistLimited && tqTwistAngle > Cm::tanAdd(j.tanQTwistHigh, -j.tanQTwistPad);
	
		const PxU8 constraintCount = PxU8(swingLimited + tangentialStiffness + twistUpperLimited + twistLowerLimited);
		if(!constraintCount)
			continue;

		PxSolverConstraintDesc& desc = constraintDesc[descCount++];

		desc.articulationA = &fsData;
		desc.linkIndexA = Ps::to16(links[i].parent);
		desc.articulationALength = Ps::to16(solverDataSize);

		desc.articulationB = &fsData;
		desc.linkIndexB = i;
		desc.articulationBLength = Ps::to16(solverDataSize);

		const PxU32 constraintLength = sizeof(SolverConstraint1DHeader) + 
								 sizeof(SolverConstraint1DExt) * constraintCount;

		PX_ASSERT(0==(constraintLength & 0x0f));
		desc.constraintLengthOver16 = Ps::to16(constraintLength/16);
		
		desc.constraint = stream.reserve(constraintLength + 16u, constraintBlockManager);

		desc.writeBack = NULL;
		
		SolverConstraint1DHeader* header = reinterpret_cast<SolverConstraint1DHeader*>(desc.constraint);
		SolverConstraint1DExt* constraints = reinterpret_cast<SolverConstraint1DExt*>(desc.constraint + sizeof(SolverConstraint1DHeader));

		init(*header, constraintCount, true, ims);

		PxU32 cIndex = 0;

		if(swingLimited)
		{
			const PxVec3 normal = jointTransforms[i].cA2w.rotate(swingLimitAxis);
			createHardLimit(fsData, links, i, constraints[cIndex++], normal, swingLimitError, recipDt);
			if(tangentialStiffness)
			{
				const PxVec3 tangent = twistAxis.cross(normal).getNormalized();
				createTangentialSpring(fsData, links, i, constraints[cIndex++], tangent, j.tangentialStiffness, j.tangentialDamping, dt);
			}
		}

		if(twistUpperLimited)
			createHardLimit(fsData, links, i, constraints[cIndex++], twistAxis, (j.tanQTwistHigh - tqTwistAngle)*4, recipDt);

		if(twistLowerLimited)
			createHardLimit(fsData, links, i, constraints[cIndex++], -twistAxis, -(j.tanQTwistLow - tqTwistAngle)*4, recipDt);

		*(desc.constraint + getConstraintLength(desc)) = 0;

		PX_ASSERT(cIndex == constraintCount);
		acCount += constraintCount;
	}

	return descCount;
}

void ArticulationHelper::computeJointDrives(FsData& fsData,
											Vec3V* drives, 
											const ArticulationLink* links,
											const PxTransform* poses, 
											const ArticulationJointTransforms* transforms, 
											const Mat33V* loads, 
											PxReal dt)
{
	typedef ArticulationFnsScalar Fns;

	const PxU32 linkCount = fsData.linkCount;
	const Cm::SpatialVector* velocity = reinterpret_cast<const Cm::SpatialVector*>(getVelocity(fsData));

	for(PxU32 i=1; i<linkCount;i++)
	{
		PxU32 parent = links[i].parent;
		const ArticulationJointTransforms& b = transforms[i];
		const ArticulationJointCore& j = *links[i].inboundJoint;

		const Cm::SpatialVector currentVel = Fns::translateMotion(poses[i].p - b.cA2w.p, velocity[i])
											 - Fns::translateMotion(poses[parent].p - b.cA2w.p, velocity[parent]);

		// we want the quat such that q * cB2cA = targetPosition
		PxVec3 rotVec;
		if(j.driveType == PxU8(PxArticulationJointDriveType::eTARGET))
			rotVec = Ps::log(j.targetPosition * b.cB2cA.q.getConjugate()); // as a rotation vector
		else
			rotVec = j.targetPosition.getImaginaryPart();

		// NM's Tests indicate behavior is better without the term commented out below, even though
		// an implicit spring derivation suggests it should be there.

		const PxVec3 posError = b.cA2w.rotate(rotVec); // - currentVel.angular * 0.5f * dt
		const PxVec3 velError = b.cA2w.rotate(j.targetVelocity) - currentVel.angular;

		drives[i] = M33MulV3(loads[i], V3LoadU((j.spring * posError + j.damping * velError) * dt * getResistance(j.internalCompliance)));
	}
}

ArticulationPImpl::ComputeUnconstrainedVelocitiesFn ArticulationPImpl::sComputeUnconstrainedVelocities = NULL;
ArticulationPImpl::UpdateBodiesFn ArticulationPImpl::sUpdateBodies = NULL;
ArticulationPImpl::SaveVelocityFn ArticulationPImpl::sSaveVelocity = NULL;

}
}
