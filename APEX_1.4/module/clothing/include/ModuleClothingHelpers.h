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



#ifndef MODULE_CLOTHING_HELPERS_H
#define MODULE_CLOTHING_HELPERS_H

#include "RenderMeshAssetIntl.h"
#include "RenderDebugInterface.h"
#include "PsUserAllocated.h"
#include "ApexSharedUtils.h"

namespace nvidia
{
namespace clothing
{

class ClothingCookedParam;

// used for mesh skinning map creation
struct TriangleWithNormals
{
	void init()
	{
		// bestDot and worstDot store the difference of the vertex normals from the face normal
		PxVec3 faceNormal = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0]);
		faceNormal.normalize();
		float bestDot = -1.0f;
		float worstDot = 1.0f;
		for (uint32_t j = 0; j < 3; j++)
		{
			const float dot = faceNormal.dot(normals[j]);
			bestDot = PxMax(bestDot, dot);
			worstDot = PxMin(worstDot, dot);
		}

		doNotUse = worstDot < 0.0f || bestDot < 0.5f; // bestDot has more than 60 degrees difference from faceNormal

		// init the rest of the triangle members
		timestamp = -1;
		valid = 0;

		bounds.minimum = bounds.maximum = vertices[0];
		bounds.include(vertices[1]);
		bounds.include(vertices[2]);
		PX_ASSERT(!bounds.isEmpty());
	}

	PxVec3 vertices[3];
	PxVec3 normals[3];
	uint32_t faceIndex0;
	int32_t timestamp : 30;
	uint32_t valid : 2;

	PxBounds3 bounds;

	PxVec3 tempBaryVertex;
	PxVec3 tempBaryNormal;
	PxVec3 tempBaryTangent;

	uint32_t master;

	bool doNotUse;
};



class ModuleClothingHelpers
{
public:
	/**
	\brief A fast inverse square root. Mainly used to normalize vectors more quickly (and less accurately)
	*/
	inline static float invSqrt(float input)
	{
		const float halfInput = 0.5f * input;
		int32_t i	= *(int*)&input;

		i		= 0x5f375a86 - (i >> 1);
		input	= *(float*) & i;
		input	= input * (1.5f - halfInput * input * input);
		return input;
	}


	static float triangleArea(const PxVec3& v1, const PxVec3& v2, const PxVec3& v3)
	{
		const PxVec3 a	= v2 - v1;
		const PxVec3 b	= v3 - v1;
		const PxVec3 c	= a.cross(b);
		//PxVec3       normal = c;
		//normal.normalize();
		//const float area = normal.dot(c) * 0.5f;
		//return area;
		return c.magnitude() * 0.5f;
	}

#pragma warning(push)
#pragma warning(disable:4127) // (conditional expression is constant)
	template<bool method1>
	static PxVec3 computeTriBarycentricCoords(const PxVec3& a, const PxVec3& b, const PxVec3& c, const PxVec3& g)
	{
		PxVec3 bary(0, 0, 0);
		if (method1)
		{
			const float abc = triangleArea(a, b, c);

			// Convert our position from local space into barycentric coordinates + offset from surface.
			if (abc >  0.0f)
			{
				PxVec3 Edge0 = b - a;
				PxVec3 Edge1 = c - a;

				PxVec3 planeNormal = Edge0.cross(Edge1);
				planeNormal.normalize();

				// See comments in set() for computation of d
				float planeD = -a.dot(planeNormal);

				// Valid for plane equation a*x + b*y + c*z + d = 0
				const float distance = g.dot(planeNormal) + planeD;

				// project on plane
				const PxVec3 p = g - planeNormal * distance;

				bary.x = triangleArea(p, b, c) / abc;
				bary.y = triangleArea(p, c, a) / abc;
				bary.z = distance;
			}
		}
		else
		{
			// triangle's normal
			PxVec3 faceNormal = (b - a).cross(c - a);
			faceNormal.normalize();

			// distance to plane
			float h = (g - a).dot(faceNormal);

			// project g onto plane
			PxVec3 pg = g - (h * faceNormal);

			// compute barycentric coordinates of g in <a,b,c>
			float s(0), t(0);
			generateBarycentricCoordinatesTri(a, b, c, pg, s, t);

			// PH: s and t are not the barycentric coordinate of a and b, but of b and c
			bary.x = 1 - s - t;
			bary.y = s;
			bary.z = h;
		}
		return bary;
	}
#pragma warning(pop)

	static float TetraVolume(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& p3)
	{
		return (p1 - p0).cross(p2 - p0).dot(p3 - p0) * (1.0f / 6.0f);
	}



	static PxVec3 computeTetraBarycentricCoords(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& p3, const PxVec3& p)
	{
		float tetraVol = TetraVolume(p0, p1, p2, p3);
		PX_ASSERT(tetraVol != 0.0f);
		PxVec3 bary;
		bary.x = TetraVolume(p, p1, p2, p3) / tetraVol;
		bary.y = TetraVolume(p, p2, p0, p3) / tetraVol;
		bary.z = TetraVolume(p, p3, p0, p1) / tetraVol;
		/*
		bary.x = PxAbs(bary.x);
		bary.y = PxAbs(bary.y);
		bary.z = PxAbs(bary.z);
		*/

		return bary;
	}



	static float barycentricDist(const PxVec3& barycentricCoords)
	{
		float dist = 0.0f;
		if (-barycentricCoords.x > dist)
		{
			dist = -barycentricCoords.x;
		}
		if (-barycentricCoords.y > dist)
		{
			dist = -barycentricCoords.y;
		}
		if (-barycentricCoords.z > dist)
		{
			dist = -barycentricCoords.z;
		}
		float sum = barycentricCoords.x + barycentricCoords.y + barycentricCoords.z - 1.0f;
		if (sum > dist)
		{
			dist = sum;
		}
		return dist;
	}


	static bool pointInTetra(const PxVec3& p0, const PxVec3& p1, const PxVec3& p2, const PxVec3& p3, const PxVec3& p, const float epsilon)
	{
		PxVec3 n = (p1 - p0).cross(p2 - p0);
		float s0 = n.dot(p - p0);

		n = (p2 - p0).cross(p3 - p0);
		float s1 = n.dot(p - p0);

		n = (p3 - p0).cross(p1 - p0);
		float s2 = n.dot(p - p0);

		n = (p3 - p1).cross(p2 - p1);
		float s3 = n.dot(p - p1);

		return	((s0 >= -epsilon) && (s1 >= -epsilon) && (s2 >= -epsilon) && (s3 >= -epsilon)) ||
		        ((s0 <= epsilon) && (s1 <= epsilon) && (s2 <= epsilon) && (s3 <= epsilon));
	}



	struct Ellipsoid
	{
		PxVec3 center;
		PxVec3 xAxis;	// contains scale
		PxVec3 yAxis;	// contains scale
		PxVec3 zAxis;	// contains scale
	};



	float GetWeight(float value, float maxValue)
	{
		float x = PxAbs(value) / PxAbs(maxValue);
		if (value > 1.0)
		{
			value = 1.0;
		}
		if (value < 0.0)
		{
			value = 0.0;
		}
		//return cos(HalfPi*x)*cos(HalfPi*x);
		return PxExp(-x * x * 4);
	}



	// Returns a value 0 and 1 if inside, > 1 if outside
	float GetEllipsoidDistance(const Ellipsoid& e, const PxVec3& p)
	{
#define SQR(_x) ((_x)*(_x))

		PxVec3 cp = p - e.center;
		float xProj = cp.dot(e.xAxis);
		float yProj = cp.dot(e.yAxis);
		float zProj = cp.dot(e.zAxis);

		return sqrtf(SQR(xProj) + SQR(yProj) + SQR(zProj));
	}



	float GetEllipsoidWeight(const Ellipsoid& e, const PxVec3& p)
	{
		float d = GetEllipsoidDistance(e, p);
		return GetWeight(d, 1.0f);
	}



	static PxVec3 projectOnPlane(const PxVec3& planeNormal, float planeD, const PxVec3& point, const PxVec3& projectionDirection)
	{
		// from http://en.wikipedia.org/wiki/Line-plane_intersection

		//t={-d-\mathbf{l}_a\cdot\mathbf{n} \over (\mathbf{l}_b-\mathbf{l}_a)\cdot\mathbf{n}}.

		const float nominator = -planeD - point.dot(planeNormal);
		const float denominator = projectionDirection.dot(planeNormal);

		if (denominator != 0.0f)
		{
			const float t = nominator / denominator;

			return point + t * projectionDirection;
		}

		return point;
	}


	/**
	Given target point p and triangle with points p1, p2, p3 and normals n1, n2, n3.
	Find barycentric coordinates (x, y, z) and height h, such that z = 1-x-y and
	x*p1+y*p2+z*p3 + (x*n1+y*p2+y*p3)*h

	This means finding a point on the triangle where the interpolated normal goes through the target point.
	FinalBary will contain (x, y, h).
	*/
	static bool iterateOnTriangle(uint32_t maxIterations, float convergenceThreshold, const TriangleWithNormals& triangle, const PxVec3& position,
		PxVec3& finalBary)
	{
		uint32_t iteration = 0;

		PxVec3 planeNormal = (triangle.vertices[1] - triangle.vertices[0]).cross(triangle.vertices[2] - triangle.vertices[0]);
		planeNormal.normalize();
		const float planeD = -triangle.vertices[0].dot(planeNormal);

		// start with normal from the middle of the triangle
		PxVec3 trustedBary(1.0f / 3.0f);
		//PxVec3 tempNormal = planeNormal;
		PxVec3 tempNormal = trustedBary.x * triangle.normals[0] + trustedBary.y * triangle.normals[1] + trustedBary.z * triangle.normals[2];

		float baryConvergence = PX_MAX_F32;

		float trust = 1.0f;
		while (iteration++ < maxIterations)
		{
			PxVec3 tempPosOnTriangle = projectOnPlane(planeNormal, planeD, position, tempNormal);
			PxVec3 tempBary = ModuleClothingHelpers::computeTriBarycentricCoords<false>(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2], tempPosOnTriangle);

			// projectOnPlane should lead to position on plane
			if (PxAbs(tempBary.z) > 0.001f)
			{
				return false;
			}

			// compute real bary coordinate
			tempBary.z = 1.0f - tempBary.x - tempBary.y;

			const PxVec3 errorVec = tempBary - trustedBary;
			baryConvergence = errorVec.dot(errorVec);

			// average bary coordinate, graded sum of new values
			trustedBary *= (1.0f - trust);
			trustedBary += trust * tempBary;

			// do we still maintain the barycentric invariant?
			if (PxAbs(1.0f - (trustedBary.x + trustedBary.y + trustedBary.z)) > 0.001f)
			{
				return false;
			}

			// find new normal
			tempNormal = trustedBary.x * triangle.normals[0] + trustedBary.y * triangle.normals[1] + trustedBary.z * triangle.normals[2];

			if (baryConvergence < convergenceThreshold)
			{
				break;
			}

			trust *= 0.8f;
		}

		if (baryConvergence < convergenceThreshold)
		{
			const PxVec3 posOnTriangle = trustedBary.x * triangle.vertices[0] + trustedBary.y * triangle.vertices[1] + trustedBary.z * triangle.vertices[2];

			const float length = tempNormal.normalize();
			float avgHeight = tempNormal.dot(position - posOnTriangle) / length;

			finalBary = trustedBary;
			finalBary.z = avgHeight;

			return true;
		}

		return false;
	}



	static void computeTriangleBarys(TriangleWithNormals& triangle, const PxVec3& position, const PxVec3& normal, const PxVec3& tangent, float thickness, int32_t timeStamp, bool onlyTangent)
	{
		// todo PH: assert is finite!

		if (triangle.timestamp < timeStamp)
		{
			triangle.valid = 0;

			// This is the value that is used when nothing is hit
			triangle.tempBaryVertex = PxVec3(PX_MAX_F32);
			triangle.tempBaryNormal = PxVec3(PX_MAX_F32);
			triangle.tempBaryTangent = PxVec3(PX_MAX_F32);

			PxVec3 positionBary(PX_MAX_F32);
			PxVec3 normalBary(PX_MAX_F32);
			PxVec3 tangentBary(PX_MAX_F32);

			if (onlyTangent || iterateOnTriangle(100, 0.0001f, triangle, position, positionBary))
			{
				triangle.valid = 1;

				triangle.tempBaryVertex = positionBary;
				PX_ASSERT(triangle.tempBaryVertex.isFinite());

				if (!normal.isZero())
				{
					if (iterateOnTriangle(100, 0.0001f, triangle, normal, normalBary))
					{
						triangle.valid = 2;

						triangle.tempBaryNormal = normalBary;
						PX_ASSERT(triangle.tempBaryNormal.isFinite());
					}
					else
					{
						PxVec3 newNormal = normal;
						for (uint32_t iter = 0; iter < 10; iter++)
						{
							// changing normal size might help
							newNormal = (position + newNormal) / 2.0f;
							if (iterateOnTriangle(10, 0.0001f, triangle, newNormal, normalBary))
							{
								triangle.valid = 2;

								triangle.tempBaryNormal = normalBary;
								PX_ASSERT(triangle.tempBaryNormal.isFinite());

								break;
							}
						}
						if (triangle.valid != 2)
						{
							// now this is a really low approximation, but still better than nothing
							triangle.tempBaryNormal = triangle.tempBaryVertex;
							triangle.tempBaryNormal.z += thickness;
						}
					}
				}

				if (!tangent.isZero())
				{
					if(iterateOnTriangle(1, PX_MAX_F32, triangle, tangent, tangentBary))
					{
						triangle.tempBaryTangent = tangentBary;
						PX_ASSERT(triangle.tempBaryTangent.isFinite());
					}
				}
			}

			triangle.timestamp = timeStamp;
		}
	}
};

}
} // namespace nvidia

#endif // MODULE_CLOTHING_HELPERS_H
