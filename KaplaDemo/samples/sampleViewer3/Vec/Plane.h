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

#ifndef PLANE_H
#define PLANE_H

#include "Vec3.H"

//	Singe / VecReal Precision Vec 3
//  Matthias Mueller
//  derived from Plane

namespace M
{

class Plane
{
public:
	Plane() {}
	Plane(VecReal nx, VecReal ny, VecReal nz, VecReal distance)
		: n(nx, ny, nz)
		, d(distance)
	{}

	Plane(const Vec3& normal, VecReal distance) 
		: n(normal)
		, d(distance)
	{}

	Plane(const Vec3& point, const Vec3& normal)
		: n(normal)		
		, d(-point.dot(n))		// p satisfies normal.dot(p) + d = 0
	{
	}

	Plane(const Vec3& p0, const Vec3& p1, const Vec3& p2)
	{
		n = (p1 - p0).cross(p2 - p0).getNormalized();
		d = -p0.dot(n);
	}

	VecReal distance(const Vec3& p) const
	{
		return p.dot(n) + d;
	}

	bool contains(const Vec3& p) const
	{
		return vecAbs(distance(p)) < (1.0e-7f);
	}

	Vec3 project(const Vec3 & p) const
	{
		return p - n * distance(p);
	}

	Vec3 pointInPlane() const
	{
		return -n*d;
	}

	void normalize()
	{
		VecReal denom = 1.0f / n.magnitude();
		n *= denom;
		d *= denom;
	}

	Vec3	n;			//!< The normal to the plane
	VecReal	d;			//!< The distance from the origin
};

}

#endif

