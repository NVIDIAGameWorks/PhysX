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

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Vec3.h"
#include "Quat.h"
#include "Plane.h"

//	Singe / VecVecReal Precision Vec 3
//  Matthias Mueller
//  derived from NxVec3.h

namespace M
{

class Transform
{
public:
	Quat q;
	Vec3 p;

	Transform() {};
	Transform(const Vec3& position): q(0, 0, 0, 1), p(position) {}
	Transform(const Quat& orientation): q(orientation), p(0, 0, 0) {}
	Transform(const Vec3& p0, const Quat& q0): q(q0), p(p0) {}

	Transform operator*(const Transform& x) const
	{
		return transform(x);
	}

	Transform getInverse() const
	{
		return Transform(q.rotateInv(-p),q.getConjugate());
	}


	Vec3 transform(const Vec3& input) const
	{
		return q.rotate(input) + p;
	}

	Vec3 transformInv(const Vec3& input) const
	{
		return q.rotateInv(input-p);
	}

	Vec3 rotate(const Vec3& input) const
	{
		return q.rotate(input);
	}

	Vec3 rotateInv(const Vec3& input) const
	{
		return q.rotateInv(input);
	}

	Transform transform(const Transform& src) const
	{
		return Transform(q.rotate(src.p) + p, q*src.q);
	}

	Transform transformInv(const Transform& src) const
	{
		Quat qinv = q.getConjugate();
		return Transform(qinv.rotate(src.p - p), qinv*src.q);
	}

	static  Transform createIdentity() 
	{ 
		return Transform(Vec3(0)); 
	}

	Plane transform(const Plane& plane) const
	{
		Vec3 transformedNormal = rotate(plane.n);
		return Plane(transformedNormal, plane.d - p.dot(transformedNormal));
	}

	Plane inverseTransform(const Plane& plane) const
	{
		Vec3 transformedNormal = rotateInv(plane.n);
		return Plane(transformedNormal, plane.d + p.dot(plane.n));
	}

	Transform getNormalized() const
	{
		return Transform(p, q.getNormalized());
	}

};

}

#endif 
