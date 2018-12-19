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

#include "MathUtils.h"
#include "foundation/PxMat33.h"
#include "PhysXMacros.h"

//---------------------------------------------------------------------

void jacobiRotate(PxMat33 &A, PxMat33 &R, int p, int q)
{
	// rotates A through phi in pq-plane to set A(p,q) = 0
	// rotation stored in R whose columns are eigenvectors of A
	if (A(p,q) == 0.0f)
		return;

	float d = (A(p,p) - A(q,q))/(2.0f*A(p,q));
	float t = 1.0f / (PxAbs(d) + PxSqrt(d*d + 1.0f));
	if (d < 0.0f) t = -t;
	float c = 1.0f/PxSqrt(t*t + 1);
	float s = t*c;
	A(p,p) += t*A(p,q);
	A(q,q) -= t*A(p,q);
	A(p,q) = A(q,p) = 0.0f;
	// transform A
	int k;
	for (k = 0; k < 3; k++) {
		if (k != p && k != q) {
			float Akp = c*A(k,p) + s*A(k,q);
			float Akq =-s*A(k,p) + c*A(k,q);
			A(k,p) = A(p,k) = Akp;
			A(k,q) = A(q,k) = Akq;
		}
	}
	// store rotation in R
	for (k = 0; k < 3; k++) {
		float Rkp = c*R(k,p) + s*R(k,q);
		float Rkq =-s*R(k,p) + c*R(k,q);
		R(k,p) = Rkp;
		R(k,q) = Rkq;
	}
}

//---------------------------------------------------------------------

void eigenDecomposition(PxMat33 &A, PxMat33 &R)
{
	const int numJacobiIterations = 10;
	const float epsilon = 1e-15f;

	// only for symmetric matrices!
	R = PX_MAT33_ID;	// unit matrix
	int iter = 0;
	while (iter < numJacobiIterations) {	// 3 off diagonal elements
		// find off diagonal element with maximum modulus
		int p,q;
		float a,max;
		max = PxAbs(A(0,1));
		p = 0; q = 1;
		a   = PxAbs(A(0,2));
		if (a > max) { p = 0; q = 2; max = a; }
		a   = PxAbs(A(1,2));
		if (a > max) { p = 1; q = 2; max = a; }
		// all small enough -> done
		if (max < epsilon) break;
		// rotate matrix with respect to that element
		jacobiRotate(A, R, p,q);
		iter++;
	}
}

//---------------------------------------------------------------------
void polarDecomposition(const PxMat33 &A, PxMat33 &R)
{
	// A = SR, where S is symmetric and R is orthonormal
	// -> S = (A A^T)^(1/2)

	PxMat33 AAT;
	AAT(0,0) = A(0,0)*A(0,0) + A(0,1)*A(0,1) + A(0,2)*A(0,2);
	AAT(1,1) = A(1,0)*A(1,0) + A(1,1)*A(1,1) + A(1,2)*A(1,2);
	AAT(2,2) = A(2,0)*A(2,0) + A(2,1)*A(2,1) + A(2,2)*A(2,2);

	AAT(0,1) = A(0,0)*A(1,0) + A(0,1)*A(1,1) + A(0,2)*A(1,2);
	AAT(0,2) = A(0,0)*A(2,0) + A(0,1)*A(2,1) + A(0,2)*A(2,2);
	AAT(1,2) = A(1,0)*A(2,0) + A(1,1)*A(2,1) + A(1,2)*A(2,2);

	AAT(1,0) = AAT(0,1);
	AAT(2,0) = AAT(0,2);
	AAT(2,1) = AAT(1,2);

	PxMat33 U;
	R = PX_MAT33_ID;
	eigenDecomposition(AAT, U);

	const float eps = 1e-15f;

	float l0 = AAT(0,0); if (l0 <= eps) l0 = 0.0f; else l0 = 1.0f / sqrt(l0);
	float l1 = AAT(1,1); if (l1 <= eps) l1 = 0.0f; else l1 = 1.0f / sqrt(l1);
	float l2 = AAT(2,2); if (l2 <= eps) l2 = 0.0f; else l2 = 1.0f / sqrt(l2);

	PxMat33 S1;
	S1(0,0) = l0*U(0,0)*U(0,0) + l1*U(0,1)*U(0,1) + l2*U(0,2)*U(0,2);
	S1(1,1) = l0*U(1,0)*U(1,0) + l1*U(1,1)*U(1,1) + l2*U(1,2)*U(1,2);
	S1(2,2) = l0*U(2,0)*U(2,0) + l1*U(2,1)*U(2,1) + l2*U(2,2)*U(2,2);

	S1(0,1) = l0*U(0,0)*U(1,0) + l1*U(0,1)*U(1,1) + l2*U(0,2)*U(1,2);
	S1(0,2) = l0*U(0,0)*U(2,0) + l1*U(0,1)*U(2,1) + l2*U(0,2)*U(2,2);
	S1(1,2) = l0*U(1,0)*U(2,0) + l1*U(1,1)*U(2,1) + l2*U(1,2)*U(2,2);

	S1(1,0) = S1(0,1);
	S1(2,0) = S1(0,2);
	S1(2,1) = S1(1,2);

	R = S1 * A;

	// stabilize
	PxVec3 c0 = R.column0;
	PxVec3 c1 = R.column1;
	PxVec3 c2 = R.column2;

	if (c0.magnitudeSquared() < eps)
		c0 = c1.cross(c2);
	else if (c1.magnitudeSquared() < eps)
		c1 = c2.cross(c0);
	else 
		c2 = c0.cross(c1);
	R.column0 = c0;
	R.column1 = c1;
	R.column2 = c2;
}

//---------------------------------------------------------------------
PxVec3 perpVec3(const PxVec3 &v)
{
	PxVec3 n;
	if (fabs(v.x) < fabs(v.y) && fabs(v.x) < fabs(v.z))
		n = PxVec3(1.0f, 0.0f, 0.0f);
	else if (fabs(v.y) < fabs(v.z))
		n = PxVec3(0.0f, 1.0f, 0.0f);
	else
		n = PxVec3(0.0f, 0.0f, 1.0f);
	n = v.cross(n);
	n.normalize();
	return n;
}

//---------------------------------------------------------------------
void polarDecompositionStabilized(const PxMat33 &A, PxMat33 &R)
{
	PxMat33 ATA;
	ATA = A.getTranspose() * A;

	PxMat33 Q;
	eigenDecomposition(ATA, Q);

	int degenerated = 0;

	float l0 = ATA(0,0);
	if (fabs(l0) <= FLT_EPSILON) {
		l0 = 0;
		degenerated += 1;
	}
	else l0 = 1.0f / PxSqrt(l0);

	float l1 = ATA(1,1);
	if (fabs(l1) <= FLT_EPSILON) {
		l1 = 0;
		degenerated += 2;
	}
	else l1 = 1.0f / PxSqrt(l1);
 
	float l2 = ATA(2,2);
	if (fabs(l2) <= FLT_EPSILON) {
		l2 = 0;
		degenerated += 4;
	}
	else l2 = 1.0f / PxSqrt(l2);
 

	if (A.getDeterminant() < 0) //Invertierung nach Irving,Fedkiw
	{
		float *max = &l0;
		if (l1 > *max) {max = &l1;}
		if (l2 > *max) {max = &l2;}
		*max *= -1;
	}

	PxMat33 D = PX_MAT33_ZERO;
	D(0,0) = l0;
	D(1,1) = l1;
	D(2,2) = l2;
	R = A * Q * D;

	Q = Q.getTranspose();

	//handle singular cases

	PxVec3 r0 = R.column0;
	PxVec3 r1 = R.column1;
	PxVec3 r2 = R.column2;

	if (degenerated == 0) {			// 000
	}
	else if (degenerated == 1) {	// 100
		r0 = r1.cross(r2);
		R.column0 = r0;
	}
	else if (degenerated == 2) {	// 010
		r1 = r2.cross(r0);
		R.column1 = r1;
	}
	else if (degenerated == 4) {	// 001
		r2 = r0.cross(r1);
		R.column2 = r2;
	}
	else if (degenerated == 6) {	// 011
		r1 = perpVec3(r0);
		r2 = r0.cross(r1);
		R.column1 = r1;
		R.column2 = r2;
	}
	else if (degenerated == 5) {	// 101
		r2 = perpVec3(r1);
		r0 = r1.cross(r2);
		R.column2 = r2;
		R.column0 = r0;
	}
	else if (degenerated == 3) {	// 110
		r0 = perpVec3(r2);
		r1 = r2.cross(r0);
		R.column0 = r0;
		R.column1 = r1;
	}
	else							// 111
		R = PX_MAT33_ID;

	R = R*Q;
}

//---------------------------------------------------------------------
void eigenDecomposition22(const PxMat33 &A, PxMat33 &R, PxMat33 &D)
{
	// only for symmetric matrices 

	// decompose A such that
	// A = R D R^T, where D is diagonal and R orthonormal (a rotation)

	R = PX_MAT33_ID;
	D = PX_MAT33_ID;
	D(0,0) = A(0,0); D(0,1) = A(0,1);
	D(1,0) = A(1,0); D(1,1) = A(1,1);

	if (D(0,1) == 0.0f)
		return;

	float d = (D(0,0) - D(1,1))/(2.0f*D(0,1));
	float t = 1.0f / (PxAbs(d) + PxSqrt(d*d + 1.0f));
	if (d < 0.0f) t = -t;
	float c = 1.0f/sqrt(t*t + 1);
	float s = t*c;
	D(0,0) += t*D(0,1);
	D(1,1) -= t*D(0,1);
	D(0,1) = D(1,0) = 0.0f;
	// store rotation in R
	for (int k = 0; k < 2; k++) {
		float Rkp = c*R(k,0) + s*R(k,1);
		float Rkq =-s*R(k,0) + c*R(k,1);
		R(k,0) = Rkp;
		R(k,1) = Rkq;
	}
}

//---------------------------------------------------------------------
PxMat33 outerProduct(const PxVec3 &v0, const PxVec3 &v1)
{
	PxMat33 M;
	M.column0 = v0 * v1.x;
	M.column1 = v0 * v1.y;
	M.column2 = v0 * v1.z;
	return M;
}

// From geometrictools.com
PxQuat align (const PxVec3& v1, const PxVec3& v2) {
	// vector U = Cross(V1,V2)/Length(Cross(V1,V2)).  The angle of rotation,
	// A, is the angle between V1 and V2.  The quaternion for the rotation is
	// q = cos(A/2) + sin(A/2)*(ux*i+uy*j+uz*k) where U = (ux,uy,uz).
	//
	// (1) Rather than extract A = acos(Dot(V1,V2)), multiply by 1/2, then
	//     compute sin(A/2) and cos(A/2), we reduce the computational costs by
	//     computing the bisector B = (V1+V2)/Length(V1+V2), so cos(A/2) =
	//     Dot(V1,B).
	//
	// (2) The rotation axis is U = Cross(V1,B)/Length(Cross(V1,B)), but
	//     Length(Cross(V1,B)) = Length(V1)*Length(B)*sin(A/2) = sin(A/2), in
	//     which case sin(A/2)*(ux*i+uy*j+uz*k) = (cx*i+cy*j+cz*k) where
	//     C = Cross(V1,B).
	//
	// If V1 = V2, then B = V1, cos(A/2) = 1, and U = (0,0,0).  If V1 = -V2,
	// then B = 0.  This can happen even if V1 is approximately -V2 using
	// floating point arithmetic, since Vector3::Normalize checks for
	// closeness to zero and returns the zero vector accordingly.  The test
	// for exactly zero is usually not recommend for floating point
	// arithmetic, but the implementation of Vector3::Normalize guarantees
	// the comparison is robust.  In this case, the A = pi and any axis
	// perpendicular to V1 may be used as the rotation axis.

	PxVec3 bisector = v1 + v2;
	bisector.normalize();

	float cosHalfAngle = v1.dot(bisector);
	PxVec3 cross;

	float mTuple[4];
	mTuple[0] = cosHalfAngle;

	if (cosHalfAngle != (float)0)
	{
		cross = v1.cross(bisector);
		mTuple[1] = cross.x;
		mTuple[2] = cross.y;
		mTuple[3] = cross.z;
	}
	else
	{
		float invLength;
		if (fabs(v1[0]) >= fabs(v1[1]))
		{
			// V1.x or V1.z is the largest magnitude component.
			invLength = fastInvSqrt(v1[0]*v1[0] + v1[2]*v1[2]);
			mTuple[1] = -v1[2]*invLength;
			mTuple[2] = (float)0;
			mTuple[3] = +v1[0]*invLength;
		}
		else
		{
			// V1.y or V1.z is the largest magnitude component.
			invLength = fastInvSqrt(v1[1]*v1[1] + v1[2]*v1[2]);
			mTuple[1] = (float)0;
			mTuple[2] = +v1[2]*invLength;
			mTuple[3] = -v1[1]*invLength;
		}
	}

	PxQuat q(mTuple[1],mTuple[2],mTuple[3], mTuple[0]);
	return q;
}

void decomposeTwistTimesSwing (const PxQuat& q,  const PxVec3& v1,
												 PxQuat& twist, PxQuat& swing)
{

	PxVec3 v2 = v1;
	q.rotate(v2);

	swing = align(v1, v2);
	twist = q*swing.getConjugate();
}

void decomposeSwingTimesTwist (const PxQuat& q, const PxVec3& v1,
												 PxQuat& swing, PxQuat& twist)
{
	PxVec3 v2 = v1;
	q.rotate(v2);

	swing = align(v1, v2);
	twist = swing.getConjugate()*q;
}
