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

#ifndef BOUNDS3_H
#define BOUNDS3_H

#include "Vec3.h"

//	Singe / VecVecReal Precision Vec 3
//  Matthias Mueller
//  derived from Vec3.h

namespace M
{

class Bounds3
{
public:

	Vec3 minimum, maximum;

	Bounds3()
	{
		// Default to empty boxes for compatibility TODO: PT: remove this if useless
		setEmpty();
	}


	~Bounds3()
	{
		//nothing
	}


	void setEmpty()
	{
		// We know use this particular pattern for empty boxes
		set(MAX_VEC_REAL, MAX_VEC_REAL, MAX_VEC_REAL,
			MIN_VEC_REAL, MIN_VEC_REAL, MIN_VEC_REAL);
	}

	void setInfinite()
	{
		set(MIN_VEC_REAL, MIN_VEC_REAL, MIN_VEC_REAL,
			MAX_VEC_REAL, MAX_VEC_REAL, MAX_VEC_REAL);
	}

	void set(VecReal mi, VecReal miny, VecReal minz, VecReal maxx, VecReal maxy,VecReal maxz)
	{
		minimum.set(mi, miny, minz);
		maximum.set(maxx, maxy, maxz);
	}

	void set(const Vec3& _min, const Vec3& _max)
	{
		minimum = _min;
		maximum = _max;
	}

	void include(const Vec3& v)
	{
		maximum.max(v);
		minimum.min(v);
	}

	void combine(const Bounds3& b2)
	{
		// - if we're empty, min = MAX,MAX,MAX => min will be b2 in all cases => it will copy b2, ok
		// - if b2 is empty, the opposite happens => keep us unchanged => ok
		// => same behavior as before, automatically
		minimum.min(b2.minimum);
		maximum.max(b2.maximum);
	}

	//void boundsOfOBB(const Mat33& orientation, const Vec3& translation, const Vec3& halfDims)
	//{
	//VecReal dimx = halfDims[0];
	//VecReal dimy = halfDims[1];
	//VecReal dimz = halfDims[2];

	//VecReal x = Math::abs(orientation(0,0) * dimx) + Math::abs(orientation(0,1) * dimy) + Math::abs(orientation(0,2) * dimz);
	//VecReal y = Math::abs(orientation(1,0) * dimx) + Math::abs(orientation(1,1) * dimy) + Math::abs(orientation(1,2) * dimz);
	//VecReal z = Math::abs(orientation(2,0) * dimx) + Math::abs(orientation(2,1) * dimy) + Math::abs(orientation(2,2) * dimz);

	//set(-x + translation[0], -y + translation[1], -z + translation[2], x + translation[0], y + translation[1], z + translation[2]);
	//}

	//void transform(const Mat33& orientation, const Vec3& translation)
	//{
	//// convert to center and extents form
	//Vec3 center, extents;
	//getCenter(center);
	//getExtents(extents);

	//center = orientation * center + translation;
	//boundsOfOBB(orientation, center, extents);
	//}

	bool isEmpty() const
	{
		// Consistency condition for (Min, Max) boxes: min < max
		// TODO: PT: should we test against the explicit pattern ?
		if(minimum.x < maximum.x)	return false;
		if(minimum.y < maximum.y)	return false;
		if(minimum.z < maximum.z)	return false;
		return true;
	}

	bool intersects(const Bounds3& b) const
	{
		if ((b.minimum.x > maximum.x) || (minimum.x > b.maximum.x)) return false;
		if ((b.minimum.y > maximum.y) || (minimum.y > b.maximum.y)) return false;
		if ((b.minimum.z > maximum.z) || (minimum.z > b.maximum.z)) return false;
		return true;
	}

	bool intersects2D(const Bounds3& b, unsigned axis) const
	{
		// TODO: PT: could be static and like this:
		// static unsigned i[3] = { 1,2,0,1 };
		// const unsigned ii = i[axis];
		// const unsigned jj = i[axis+1];
		const unsigned i[3] = { 1,0,0 };
		const unsigned j[3] = { 2,2,1 };
		const unsigned ii = i[axis];
		const unsigned jj = j[axis];
		if ((b.minimum[ii] > maximum[ii]) || (minimum[ii] > b.maximum[ii])) return false;
		if ((b.minimum[jj] > maximum[jj]) || (minimum[jj] > b.maximum[jj])) return false;
		return true;
	}

	bool contain(const Vec3& v) const
	{
		if ((v.x < minimum.x) || (v.x > maximum.x)) return false;
		if ((v.y < minimum.y) || (v.y > maximum.y)) return false;
		if ((v.z < minimum.z) || (v.z > maximum.z)) return false;
		return true;
	}

	void getCenter(Vec3& center) const
	{
		center.add(minimum,maximum);
		center *= VecReal(0.5);
	}

	void getDimensions(Vec3& dims) const
	{
		dims.subtract(maximum,minimum);
	}

	void getExtents(Vec3& extents) const
	{
		extents.subtract(maximum,minimum);
		extents *= VecReal(0.5);
	}

	void setCenterExtents(const Vec3& c, const Vec3& e)
	{
		minimum = c - e;
		maximum = c + e;
	}

	void scale(VecReal _scale)
	{
		Vec3 center, extents;
		getCenter(center);
		getExtents(extents);
		setCenterExtents(center, extents * _scale);
	}

	void fatten(VecReal distance)
	{
		minimum.x -= distance;
		minimum.y -= distance;
		minimum.z -= distance;

		maximum.x += distance;
		maximum.y += distance;
		maximum.z += distance;
	}
};

}

#endif
