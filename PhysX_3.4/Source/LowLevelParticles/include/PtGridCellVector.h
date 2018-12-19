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

#ifndef PT_GRID_CELL_VECTOR_H
#define PT_GRID_CELL_VECTOR_H

#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxVec3.h"
#include <string.h>
#include "PxvConfig.h"
#include "PsMathUtils.h"

namespace physx
{

namespace Pt
{

/*!
Simple integer vector in R3 with basic operations.

Used to define coordinates of a uniform grid cell.
*/
class GridCellVector
{
  public:
	PX_CUDA_CALLABLE PX_FORCE_INLINE GridCellVector()
	{
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE GridCellVector(const GridCellVector& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE GridCellVector(PxI16 _x, PxI16 _y, PxI16 _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE GridCellVector(const PxVec3& realVec, PxReal scale)
	{
		set(realVec, scale);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator==(const GridCellVector& v) const
	{
		return ((x == v.x) && (y == v.y) && (z == v.z));
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE bool operator!=(const GridCellVector& v) const
	{
		return ((x != v.x) || (y != v.y) || (z != v.z));
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector operator+(const GridCellVector& v)
	{
		return GridCellVector(x + v.x, y + v.y, z + v.z);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector& operator+=(const GridCellVector& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector operator-(const GridCellVector& v)
	{
		return GridCellVector(x - v.x, y - v.y, z - v.z);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector& operator-=(const GridCellVector& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector& operator=(const GridCellVector& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector operator<<(const PxU32 shift) const
	{
		return GridCellVector(x << shift, y << shift, z << shift);
	}

	//! Shift grid cell coordinates (can be used to retrieve coordinates of a coarser grid cell that contains the
	//! defined cell)
	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector operator>>(const PxU32 shift) const
	{
		return GridCellVector(x >> shift, y >> shift, z >> shift);
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector& operator<<=(const PxU32 shift)
	{
		x <<= shift;
		y <<= shift;
		z <<= shift;
		return *this;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE const GridCellVector& operator>>=(const PxU32 shift)
	{
		x >>= shift;
		y >>= shift;
		z >>= shift;
		return *this;
	}

	//! Set grid cell coordinates based on a point in space and a scaling factor
	PX_CUDA_CALLABLE PX_FORCE_INLINE void set(const PxVec3& realVec, PxReal scale)
	{
		set(realVec * scale);
	}

#ifdef __CUDACC__
	//! Set grid cell coordinates based on a point in space
	PX_CUDA_CALLABLE PX_FORCE_INLINE void set(const PxVec3& realVec)
	{
		x = static_cast<PxI16>(floorf(realVec.x));
		y = static_cast<PxI16>(floorf(realVec.y));
		z = static_cast<PxI16>(floorf(realVec.z));
	}
#else
	//! Set grid cell coordinates based on a point in space
	PX_FORCE_INLINE void set(const PxVec3& realVec)
	{
		x = static_cast<PxI16>(Ps::floor(realVec.x));
		y = static_cast<PxI16>(Ps::floor(realVec.y));
		z = static_cast<PxI16>(Ps::floor(realVec.z));
	}
#endif

	PX_CUDA_CALLABLE PX_FORCE_INLINE void set(PxI16 _x, PxI16 _y, PxI16 _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE void setZero()
	{
		x = 0;
		y = 0;
		z = 0;
	}

	PX_CUDA_CALLABLE PX_FORCE_INLINE bool isZero() const
	{
		return x == 0 && y == 0 && z == 0;
	}

  public:
	PxI16 x;
	PxI16 y;
	PxI16 z;
};

} // namespace Pt
} // namespace physx

#endif // PX_USE_PARTICLE_SYSTEM_API
#endif // PT_GRID_CELL_VECTOR_H
