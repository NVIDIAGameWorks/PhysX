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



#ifndef COF44_H
#define COF44_H

#include "Apex.h"
#include "PxMat44.h"
#include "PxPlane.h"

namespace nvidia
{
namespace apex
{

/**
\brief Stores the info needed for the cofactor matrix of a 4x4 homogeneous transformation matrix (implicit last row = 0 0 0 1)
*/
class Cof44
{
public:
	/**
	\param [in] m can be an arbitrary homogeneoous transformation matrix
	*/
	Cof44(const PxMat44& m)
	{
		_block33(0, 0) = m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1);
		_block33(0, 1) = m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2);
		_block33(0, 2) = m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0);
		_block33(1, 0) = m(2, 1) * m(0, 2) - m(2, 2) * m(0, 1);
		_block33(1, 1) = m(2, 2) * m(0, 0) - m(2, 0) * m(0, 2);
		_block33(1, 2) = m(2, 0) * m(0, 1) - m(2, 1) * m(0, 0);
		_block33(2, 0) = m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1);
		_block33(2, 1) = m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2);
		_block33(2, 2) = m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
		_44 = _block33(0, 0) * m(0, 0) + _block33(0, 1) * m(0, 1) + _block33(0, 2) * m(0, 2);
		
		initCommon(m.getPosition());
	}

	/**
	\param [in] rt must be pure (proper) rotation and translation
	\param [in] s is any diagonal matrix (typically scale).
	\note The combined transform is assumed to be (rt)*s, i.e. s is applied first
	*/
	Cof44(const PxMat44& rt, const PxVec3 s)
	{
		const PxVec3 cofS(s.y * s.z, s.z * s.x, s.x * s.y);
		_block33(0, 0) = rt(0, 0) * cofS.x;
		_block33(0, 1) = rt(0, 1) * cofS.y;
		_block33(0, 2) = rt(0, 2) * cofS.z;
		_block33(1, 0) = rt(1, 0) * cofS.x;
		_block33(1, 1) = rt(1, 1) * cofS.y;
		_block33(1, 2) = rt(1, 2) * cofS.z;
		_block33(2, 0) = rt(2, 0) * cofS.x;
		_block33(2, 1) = rt(2, 1) * cofS.y;
		_block33(2, 2) = rt(2, 2) * cofS.z;
		_44 = cofS.x * s.x;

		initCommon(rt.getPosition());
	}

	Cof44(const PxTransform rt, const PxVec3 s)
	{
		_block33 = PxMat33(rt.q);
		const PxVec3 cofS(s.y * s.z, s.z * s.x, s.x * s.y);
		_block33.column0 *= cofS.x;
		_block33.column1 *= cofS.y;
		_block33.column2 *= cofS.z;
		_44 = cofS.x * s.x;

		initCommon(rt.p);
	}

	/**
	\brief Transforms a plane equation correctly even when the transformation is not a rotation
	\note If the transformation is not a rotation then the length of the plane's normal vector is not preserved in general.
	*/
	void transform(const PxPlane& src, PxPlane& dst) const
	{
		dst.n = _block33.transform(src.n);
		dst.d = (_block13.dot(src.n)) + _44 * src.d;
	}

	/**
	\brief Transforms a normal correctly even when the transformation is not a rotation
	\note If the transformation is not a rotation then the normal's length is not preserved in general.
	*/
	const PxMat33&	getBlock33() const
	{
		return _block33;
	}

	/**
	\brief The determinant of the original matrix.
	*/
	float			getDeterminant() const
	{
		return _44;
	}

private:

	void initCommon(const PxVec3& pos)
	{
		_block13 = _block33.transformTranspose(-pos);
		if (_44 < 0)
		{
			// det is < 0, we need to negate all values
			// The Cov Matrix divided by the determinant is the same as the inverse transposed of an affine transformation
			// For rotation normals, dividing by the determinant is useless as it gets renormalized afterwards again.
			// If the determinant is negative though, it is important that all values are negated to get the right results.
			_block33 *= -1;
			_block13 *= -1;
			_44 = -_44;
		}

	}
	PxMat33	_block33;
	PxVec3	_block13;
	float	_44;
};

}
} // end namespace nvidia::apex

#endif