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



#ifndef APEX_COLLISION_H
#define APEX_COLLISION_H

#include "ApexDefs.h"

#include "ApexUsingNamespace.h"
#include "PxVec3.h"
#include "PxMat33.h"

namespace nvidia
{
namespace apex
{



struct Segment
{
	PxVec3 p0;
	PxVec3 p1;
};

struct Capsule : Segment
{
	float radius;
};

struct Triangle
{
	Triangle(const PxVec3& a, const PxVec3& b, const PxVec3& c) : v0(a), v1(b), v2(c) {}
	PxVec3 v0;
	PxVec3 v1;
	PxVec3 v2;
};

struct Box
{
	PxVec3	center;
	PxVec3	extents;
	PxMat33	rot;
};


bool capsuleCapsuleIntersection(const Capsule& worldCaps0, const Capsule& worldCaps1, float tolerance = 1.2);
bool boxBoxIntersection(const Box& worldBox0, const Box& worldBox1);

float APEX_pointTriangleSqrDst(const Triangle& triangle, const PxVec3& position);
float APEX_segmentSegmentSqrDist(const Segment& seg0, const Segment& seg1, float* s, float* t);
float APEX_pointSegmentSqrDist(const Segment& seg, const PxVec3& point, float* param = 0);
uint32_t APEX_RayCapsuleIntersect(const PxVec3& origin, const PxVec3& dir, const Capsule& capsule, float s[2]);

bool APEX_RayTriangleIntersect(const PxVec3& orig, const PxVec3& dir, const PxVec3& a, const PxVec3& b, const PxVec3& c, float& t, float& u, float& v);

} // namespace apex
} // namespace nvidia


#endif // APEX_COLLISION_H
