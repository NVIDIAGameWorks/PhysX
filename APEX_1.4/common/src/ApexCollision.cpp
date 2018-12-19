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



#include "ApexDefs.h"
#include "ApexCollision.h"

namespace nvidia
{
namespace apex
{

bool capsuleCapsuleIntersection(const Capsule& worldCaps0, const Capsule& worldCaps1, float tolerance)
{
	float s, t;
	float squareDist = APEX_segmentSegmentSqrDist(worldCaps0, worldCaps1, &s, &t);

	float totRad = (worldCaps0.radius * tolerance) + (worldCaps1.radius * tolerance);	//incl a bit of tolerance.
	return squareDist < totRad * totRad;
}

//----------------------------------------------------------------------------//

/// \todo replace this hack
bool boxBoxIntersection(const Box& worldBox0, const Box& worldBox1)
{
	Capsule worldCaps0, worldCaps1;
	worldCaps0.p0 = worldBox0.center - worldBox0.rot * PxVec3(0, worldBox0.extents.y, 0);
	worldCaps0.p1 = worldBox0.center + worldBox0.rot * PxVec3(0, worldBox0.extents.y, 0);
	worldCaps0.radius = worldBox0.extents.x / 0.7f;
	worldCaps1.p0 = worldBox1.center - worldBox1.rot * PxVec3(0, worldBox1.extents.y, 0);
	worldCaps1.p1 = worldBox1.center + worldBox1.rot * PxVec3(0, worldBox1.extents.y, 0);
	worldCaps1.radius = worldBox1.extents.x / 0.7f;

	float s, t;
	float squareDist = APEX_segmentSegmentSqrDist(worldCaps0, worldCaps1, &s, &t);

	float totRad = (worldCaps0.radius * 1.2f) + (worldCaps1.radius * 1.2f);	//incl a bit of tolerance.
	return squareDist < totRad * totRad;
}

//----------------------------------------------------------------------------//

float APEX_pointTriangleSqrDst(const Triangle& triangle, const PxVec3& position)
{
	PxVec3 d1 = triangle.v1 - triangle.v0;
	PxVec3 d2 = triangle.v2 - triangle.v0;
	PxVec3 pp1 = position - triangle.v0;
	float a = d1.dot(d1);
	float b = d2.dot(d1);
	float c = pp1.dot(d1);
	float d = b;
	float e = d2.dot(d2);
	float f = pp1.dot(d2);
	float det = a * e - b * d;
	if (det != 0.0f)
	{
		float s = (c * e - b * f) / det;
		float t = (a * f - c * d) / det;
		if (s > 0.0f && t > 0.0f && (s + t) < 1.0f)
		{
			PxVec3 q = triangle.v0 + d1 * s + d2 * t;
			return (q  - position).magnitudeSquared();
		}
	}
	Segment segment;
	segment.p0 = triangle.v0;
	segment.p1 = triangle.v1;
	float dist = APEX_pointSegmentSqrDist(segment	, position, NULL);
	segment.p0 = triangle.v1;
	segment.p1 = triangle.v2;
	dist = PxMin(dist, APEX_pointSegmentSqrDist(segment, position, NULL));
	segment.p0 = triangle.v2;
	segment.p1 = triangle.v0;
	dist = PxMin(dist, APEX_pointSegmentSqrDist(segment, position, NULL));
	return dist;

}

//----------------------------------------------------------------------------//

#define PARALLEL_TOLERANCE	1e-02f

float APEX_segmentSegmentSqrDist(const Segment& seg0, const Segment& seg1, float* s, float* t)
{
	PxVec3 rkSeg0Direction	= seg0.p1 - seg0.p0;
	PxVec3 rkSeg1Direction	= seg1.p1 - seg1.p0;

	PxVec3 kDiff	= seg0.p0 - seg1.p0;
	float fA00	= rkSeg0Direction.magnitudeSquared();
	float fA01	= -rkSeg0Direction.dot(rkSeg1Direction);
	float fA11	= rkSeg1Direction.magnitudeSquared();
	float fB0	= kDiff.dot(rkSeg0Direction);
	float fC	= kDiff.magnitudeSquared();
	float fDet	= PxAbs(fA00 * fA11 - fA01 * fA01);

	float fB1, fS, fT, fSqrDist, fTmp;

	if (fDet >= PARALLEL_TOLERANCE)
	{
		// line segments are not parallel
		fB1 = -kDiff.dot(rkSeg1Direction);
		fS = fA01 * fB1 - fA11 * fB0;
		fT = fA01 * fB0 - fA00 * fB1;

		if (fS >= 0.0f)
		{
			if (fS <= fDet)
			{
				if (fT >= 0.0f)
				{
					if (fT <= fDet) // region 0 (interior)
					{
						// minimum at two interior points of 3D lines
						float fInvDet = 1.0f / fDet;
						fS *= fInvDet;
						fT *= fInvDet;
						fSqrDist = fS * (fA00 * fS + fA01 * fT + 2.0f * fB0) +
						           fT * (fA01 * fS + fA11 * fT + 2.0f * fB1) + fC;
					}
					else  // region 3 (side)
					{
						fT = 1.0f;
						fTmp = fA01 + fB0;
						if (fTmp >= 0.0f)
						{
							fS = 0.0f;
							fSqrDist = fA11 + 2.0f * fB1 + fC;
						}
						else if (-fTmp >= fA00)
						{
							fS = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB1 + fTmp);
						}
						else
						{
							fS = -fTmp / fA00;
							fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
						}
					}
				}
				else  // region 7 (side)
				{
					fT = 0.0f;
					if (fB0 >= 0.0f)
					{
						fS = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB0 >= fA00)
					{
						fS = 1.0f;
						fSqrDist = fA00 + 2.0f * fB0 + fC;
					}
					else
					{
						fS = -fB0 / fA00;
						fSqrDist = fB0 * fS + fC;
					}
				}
			}
			else
			{
				if (fT >= 0.0)
				{
					if (fT <= fDet)    // region 1 (side)
					{
						fS = 1.0f;
						fTmp = fA01 + fB1;
						if (fTmp >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						}
						else if (-fTmp >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						}
						else
						{
							fT = -fTmp / fA11;
							fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
						}
					}
					else  // region 2 (corner)
					{
						fTmp = fA01 + fB0;
						if (-fTmp <= fA00)
						{
							fT = 1.0f;
							if (fTmp >= 0.0f)
							{
								fS = 0.0f;
								fSqrDist = fA11 + 2.0f * fB1 + fC;
							}
							else
							{
								fS = -fTmp / fA00;
								fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
							}
						}
						else
						{
							fS = 1.0f;
							fTmp = fA01 + fB1;
							if (fTmp >= 0.0f)
							{
								fT = 0.0f;
								fSqrDist = fA00 + 2.0f * fB0 + fC;
							}
							else if (-fTmp >= fA11)
							{
								fT = 1.0f;
								fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
							}
							else
							{
								fT = -fTmp / fA11;
								fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
							}
						}
					}
				}
				else  // region 8 (corner)
				{
					if (-fB0 < fA00)
					{
						fT = 0.0f;
						if (fB0 >= 0.0f)
						{
							fS = 0.0f;
							fSqrDist = fC;
						}
						else
						{
							fS = -fB0 / fA00;
							fSqrDist = fB0 * fS + fC;
						}
					}
					else
					{
						fS = 1.0f;
						fTmp = fA01 + fB1;
						if (fTmp >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fA00 + 2.0f * fB0 + fC;
						}
						else if (-fTmp >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB0 + fTmp);
						}
						else
						{
							fT = -fTmp / fA11;
							fSqrDist = fTmp * fT + fA00 + 2.0f * fB0 + fC;
						}
					}
				}
			}
		}
		else
		{
			if (fT >= 0.0f)
			{
				if (fT <= fDet)    // region 5 (side)
				{
					fS = 0.0f;
					if (fB1 >= 0.0f)
					{
						fT = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB1 >= fA11)
					{
						fT = 1.0f;
						fSqrDist = fA11 + 2.0f * fB1 + fC;
					}
					else
					{
						fT = -fB1 / fA11;
						fSqrDist = fB1 * fT + fC;
					}
				}
				else  // region 4 (corner)
				{
					fTmp = fA01 + fB0;
					if (fTmp < 0.0f)
					{
						fT = 1.0f;
						if (-fTmp >= fA00)
						{
							fS = 1.0f;
							fSqrDist = fA00 + fA11 + fC + 2.0f * (fB1 + fTmp);
						}
						else
						{
							fS = -fTmp / fA00;
							fSqrDist = fTmp * fS + fA11 + 2.0f * fB1 + fC;
						}
					}
					else
					{
						fS = 0.0f;
						if (fB1 >= 0.0f)
						{
							fT = 0.0f;
							fSqrDist = fC;
						}
						else if (-fB1 >= fA11)
						{
							fT = 1.0f;
							fSqrDist = fA11 + 2.0f * fB1 + fC;
						}
						else
						{
							fT = -fB1 / fA11;
							fSqrDist = fB1 * fT + fC;
						}
					}
				}
			}
			else   // region 6 (corner)
			{
				if (fB0 < 0.0f)
				{
					fT = 0.0f;
					if (-fB0 >= fA00)
					{
						fS = 1.0f;
						fSqrDist = fA00 + 2.0f * fB0 + fC;
					}
					else
					{
						fS = -fB0 / fA00;
						fSqrDist = fB0 * fS + fC;
					}
				}
				else
				{
					fS = 0.0f;
					if (fB1 >= 0.0f)
					{
						fT = 0.0f;
						fSqrDist = fC;
					}
					else if (-fB1 >= fA11)
					{
						fT = 1.0f;
						fSqrDist = fA11 + 2.0f * fB1 + fC;
					}
					else
					{
						fT = -fB1 / fA11;
						fSqrDist = fB1 * fT + fC;
					}
				}
			}
		}
	}
	else
	{
		// line segments are parallel
		if (fA01 > 0.0f)
		{
			// direction vectors form an obtuse angle
			if (fB0 >= 0.0f)
			{
				fS = 0.0f;
				fT = 0.0f;
				fSqrDist = fC;
			}
			else if (-fB0 <= fA00)
			{
				fS = -fB0 / fA00;
				fT = 0.0f;
				fSqrDist = fB0 * fS + fC;
			}
			else
			{
				fB1 = -kDiff.dot(rkSeg1Direction);
				fS = 1.0f;
				fTmp = fA00 + fB0;
				if (-fTmp >= fA01)
				{
					fT = 1.0f;
					fSqrDist = fA00 + fA11 + fC + 2.0f * (fA01 + fB0 + fB1);
				}
				else
				{
					fT = -fTmp / fA01;
					fSqrDist = fA00 + 2.0f * fB0 + fC + fT * (fA11 * fT + 2.0f * (fA01 + fB1));
				}
			}
		}
		else
		{
			// direction vectors form an acute angle
			if (-fB0 >= fA00)
			{
				fS = 1.0f;
				fT = 0.0f;
				fSqrDist = fA00 + 2.0f * fB0 + fC;
			}
			else if (fB0 <= 0.0f)
			{
				fS = -fB0 / fA00;
				fT = 0.0f;
				fSqrDist = fB0 * fS + fC;
			}
			else
			{
				fB1 = -kDiff.dot(rkSeg1Direction);
				fS = 0.0f;
				if (fB0 >= -fA01)
				{
					fT = 1.0f;
					fSqrDist = fA11 + 2.0f * fB1 + fC;
				}
				else
				{
					fT = -fB0 / fA01;
					fSqrDist = fC + fT * (2.0f * fB1 + fA11 * fT);
				}
			}
		}
	}

	if (s)
	{
		*s = fS;
	}
	if (t)
	{
		*t = fT;
	}

	return PxAbs(fSqrDist);

}

//----------------------------------------------------------------------------//

float APEX_pointSegmentSqrDist(const Segment& seg, const PxVec3& point, float* param)
{
	PxVec3 Diff = point - seg.p0;
	PxVec3 segExtent = seg.p1 - seg.p0;
	float fT = Diff.dot(segExtent);

	if (fT <= 0.0f)
	{
		fT = 0.0f;
	}
	else
	{
		float SqrLen = (seg.p1 - seg.p0).magnitudeSquared();
		if (fT >= SqrLen)
		{
			fT = 1.0f;
			Diff -= segExtent;
		}
		else
		{
			fT /= SqrLen;
			Diff -= fT * segExtent;
		}
	}

	if (param)
	{
		*param = fT;
	}

	return Diff.magnitudeSquared();
}

//----------------------------------------------------------------------------//

uint32_t APEX_RayCapsuleIntersect(const PxVec3& origin, const PxVec3& dir, const Capsule& capsule, float s[2])
{
	// set up quadratic Q(t) = a*t^2 + 2*b*t + c

	PxVec3 kU, kV, kW;
	const PxVec3 capsDir = capsule.p1 - capsule.p0;
	kW = capsDir;

	float fWLength = kW.normalize();

	// generate orthonormal basis

	float fInvLength;
	if (PxAbs(kW.x) >= PxAbs(kW.y))
	{
		// W.x or W.z is the largest magnitude component, swap them
		fInvLength = 1.0f / PxSqrt(kW.x * kW.x + kW.z * kW.z);
		kU.x = -kW.z * fInvLength;
		kU.y = 0.0f;
		kU.z = +kW.x * fInvLength;
	}
	else
	{
		// W.y or W.z is the largest magnitude component, swap them
		fInvLength = 1.0f / PxSqrt(kW.y * kW.y + kW.z * kW.z);
		kU.x = 0.0f;
		kU.y = +kW.z * fInvLength;
		kU.z = -kW.y * fInvLength;
	}
	kV = kW.cross(kU);
	kV.normalize();	// PT: fixed november, 24, 2004. This is a bug in Magic.

	// compute intersection

	PxVec3 kD(kU.dot(dir), kV.dot(dir), kW.dot(dir));
	float fDLength = kD.normalize();

	float fInvDLength = 1.0f / fDLength;
	PxVec3 kDiff = origin - capsule.p0;
	PxVec3 kP(kU.dot(kDiff), kV.dot(kDiff), kW.dot(kDiff));
	float fRadiusSqr = capsule.radius * capsule.radius;

	float fInv, fA, fB, fC, fDiscr, fRoot, fT, fTmp;

	// Is the velocity parallel to the capsule direction? (or zero)
	if (PxAbs(kD.z) >= 1.0f - PX_EPS_F32 || fDLength < PX_EPS_F32)
	{

		float fAxisDir = dir.dot(capsDir);

		fDiscr = fRadiusSqr - kP.x * kP.x - kP.y * kP.y;
		if (fAxisDir < 0 && fDiscr >= 0.0f)
		{
			// Velocity anti-parallel to the capsule direction
			fRoot = PxSqrt(fDiscr);
			s[0] = (kP.z + fRoot) * fInvDLength;
			s[1] = -(fWLength - kP.z + fRoot) * fInvDLength;
			return 2;
		}
		else if (fAxisDir > 0  && fDiscr >= 0.0f)
		{
			// Velocity parallel to the capsule direction
			fRoot = PxSqrt(fDiscr);
			s[0] = -(kP.z + fRoot) * fInvDLength;
			s[1] = (fWLength - kP.z + fRoot) * fInvDLength;
			return 2;
		}
		else
		{
			// sphere heading wrong direction, or no velocity at all
			return 0;
		}
	}

	// test intersection with infinite cylinder
	fA = kD.x * kD.x + kD.y * kD.y;
	fB = kP.x * kD.x + kP.y * kD.y;
	fC = kP.x * kP.x + kP.y * kP.y - fRadiusSqr;
	fDiscr = fB * fB - fA * fC;
	if (fDiscr < 0.0f)
	{
		// line does not intersect infinite cylinder
		return 0;
	}

	int iQuantity = 0;

	if (fDiscr > 0.0f)
	{
		// line intersects infinite cylinder in two places
		fRoot = PxSqrt(fDiscr);
		fInv = 1.0f / fA;
		fT = (-fB - fRoot) * fInv;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
		}

		fT = (-fB + fRoot) * fInv;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
		}

		if (iQuantity == 2)
		{
			// line intersects capsule wall in two places
			return 2;
		}
	}
	else
	{
		// line is tangent to infinite cylinder
		fT = -fB / fA;
		fTmp = kP.z + fT * kD.z;
		if (0.0f <= fTmp && fTmp <= fWLength)
		{
			s[0] = fT * fInvDLength;
			return 1;
		}
	}

	// test intersection with bottom hemisphere
	// fA = 1
	fB += kP.z * kD.z;
	fC += kP.z * kP.z;
	fDiscr = fB * fB - fC;
	if (fDiscr > 0.0f)
	{
		fRoot = PxSqrt(fDiscr);
		fT = -fB - fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}

		fT = -fB + fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}
	else if (fDiscr == 0.0f)
	{
		fT = -fB;
		fTmp = kP.z + fT * kD.z;
		if (fTmp <= 0.0f)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}

	// test intersection with top hemisphere
	// fA = 1
	fB -= kD.z * fWLength;
	fC += fWLength * (fWLength - 2.0f * kP.z);

	fDiscr = fB * fB - fC;
	if (fDiscr > 0.0f)
	{
		fRoot = PxSqrt(fDiscr);
		fT = -fB - fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}

		fT = -fB + fRoot;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}
	else if (fDiscr == 0.0f)
	{
		fT = -fB;
		fTmp = kP.z + fT * kD.z;
		if (fTmp >= fWLength)
		{
			s[iQuantity++] = fT * fInvDLength;
			if (iQuantity == 2)
			{
				return 2;
			}
		}
	}

	return (uint32_t)iQuantity;
}


//----------------------------------------------------------------------------//

bool APEX_RayTriangleIntersect(const PxVec3& orig, const PxVec3& dir, const PxVec3& a, const PxVec3& b, const PxVec3& c, float& t, float& u, float& v)
{
	PxVec3 edge1 = b - a;
	PxVec3 edge2 = c - a;
	PxVec3 pvec = dir.cross(edge2);

	// if determinant is near zero, ray lies in plane of triangle
	float det = edge1.dot(pvec);

	if (det == 0.0f)
	{
		return false;
	}

	float iPX_det = 1.0f / det;

	// calculate distance from vert0 to ray origin
	PxVec3 tvec = orig - a;

	// calculate U parameter and test bounds
	u = tvec.dot(pvec) * iPX_det;
	if (u < 0.0f || u > 1.0f)
	{
		return false;
	}

	// prepare to test V parameter
	PxVec3 qvec = tvec.cross(edge1);

	// calculate V parameter and test bounds
	v = dir.dot(qvec) * iPX_det;
	if (v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	// calculate t, ray intersects triangle
	t = edge2.dot(qvec) * iPX_det;

	return true;
}

} // namespace apex
} // namespace nvidia
