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


#ifndef PX_PHYSICS_COMMON_VECTOR
#define PX_PHYSICS_COMMON_VECTOR

#include "foundation/PxVec3.h"
#include "CmPhysXCommon.h"
#include "PsVecMath.h"

/*!
Combination of two R3 vectors.
*/

namespace physx
{
namespace Cm
{
PX_ALIGN_PREFIX(16)
class SpatialVector
{
public:
	//! Default constructor
	PX_FORCE_INLINE SpatialVector()
	{}

	//! Construct from two PxcVectors
	PX_FORCE_INLINE SpatialVector(const PxVec3& lin, const PxVec3& ang)
		: linear(lin), pad0(0.0f), angular(ang), pad1(0.0f)
	{
	}

	PX_FORCE_INLINE ~SpatialVector()
	{}



	// PT: this one is very important. Without it, the Xbox compiler generates weird "float-to-int" and "int-to-float" LHS
	// each time we copy a SpatialVector (see for example PIX on "solveSimpleGroupA" without this operator).
	PX_FORCE_INLINE	void	operator = (const SpatialVector& v)
	{
		linear = v.linear;
		pad0 = 0.0f;
		angular = v.angular;
		pad1 = 0.0f;
	}


	static PX_FORCE_INLINE SpatialVector zero() {	return SpatialVector(PxVec3(0),PxVec3(0)); }

	PX_FORCE_INLINE SpatialVector operator+(const SpatialVector& v) const
	{
		return SpatialVector(linear+v.linear,angular+v.angular);
	}

	PX_FORCE_INLINE SpatialVector operator-(const SpatialVector& v) const
	{
		return SpatialVector(linear-v.linear,angular-v.angular);
	}

	PX_FORCE_INLINE SpatialVector operator-() const
	{
		return SpatialVector(-linear,-angular);
	}


	PX_FORCE_INLINE SpatialVector operator *(PxReal s) const
	{	
		return SpatialVector(linear*s,angular*s);	
	}
		
	PX_FORCE_INLINE void operator+=(const SpatialVector& v)
	{
		linear+=v.linear;
		angular+=v.angular;
	}

	PX_FORCE_INLINE void operator-=(const SpatialVector& v)
	{
		linear-=v.linear;
		angular-=v.angular;
	}

	PX_FORCE_INLINE PxReal magnitude()	const
	{
		return angular.magnitude() + linear.magnitude();
	}

	PX_FORCE_INLINE PxReal dot(const SpatialVector& v) const
	{
		return linear.dot(v.linear) + angular.dot(v.angular);
	}
		
	PX_FORCE_INLINE bool isFinite() const
	{
		return linear.isFinite() && angular.isFinite();
	}

	PX_FORCE_INLINE Cm::SpatialVector scale(PxReal l, PxReal a) const
	{
		return Cm::SpatialVector(linear*l, angular*a);
	}

	PxVec3 linear;
	PxReal pad0;
	PxVec3 angular;
	PxReal pad1;
}
PX_ALIGN_SUFFIX(16);


PX_ALIGN_PREFIX(16)
struct SpatialVectorV
{
	Ps::aos::Vec3V linear;
	Ps::aos::Vec3V angular;

	PX_FORCE_INLINE SpatialVectorV() {}
	PX_FORCE_INLINE SpatialVectorV(PxZERO): linear(Ps::aos::V3Zero()), angular(Ps::aos::V3Zero()) {}
	PX_FORCE_INLINE SpatialVectorV(const Cm::SpatialVector& v): linear(Ps::aos::V3LoadU(v.linear)), angular(Ps::aos::V3LoadU(v.angular)) {}
	PX_FORCE_INLINE SpatialVectorV(const Ps::aos::Vec3VArg l, const Ps::aos::Vec3VArg a): linear(l), angular(a) {}
	PX_FORCE_INLINE SpatialVectorV(const SpatialVectorV& other): linear(other.linear), angular(other.angular) {}
	PX_FORCE_INLINE SpatialVectorV& operator=(const SpatialVectorV& other) { linear = other.linear; angular = other.angular; return *this; }

	PX_FORCE_INLINE SpatialVectorV operator+(const SpatialVectorV& other) const { return SpatialVectorV(Ps::aos::V3Add(linear,other.linear),
																								  Ps::aos::V3Add(angular, other.angular)); }
	
	PX_FORCE_INLINE SpatialVectorV& operator+=(const SpatialVectorV& other) { linear = Ps::aos::V3Add(linear,other.linear); 
																			  angular = Ps::aos::V3Add(angular, other.angular);
																			  return *this;
																			}
																								    
	PX_FORCE_INLINE SpatialVectorV operator-(const SpatialVectorV& other) const { return SpatialVectorV(Ps::aos::V3Sub(linear,other.linear),
																								  Ps::aos::V3Sub(angular, other.angular)); }
	
	PX_FORCE_INLINE SpatialVectorV operator-() const { return SpatialVectorV(Ps::aos::V3Neg(linear), Ps::aos::V3Neg(angular)); }

	PX_FORCE_INLINE SpatialVectorV operator*(Ps::aos::FloatVArg r) const { return SpatialVectorV(Ps::aos::V3Scale(linear,r), Ps::aos::V3Scale(angular,r)); }

	PX_FORCE_INLINE SpatialVectorV& operator-=(const SpatialVectorV& other) { linear = Ps::aos::V3Sub(linear,other.linear); 
																			  angular = Ps::aos::V3Sub(angular, other.angular);
																			  return *this;
																			}

	PX_FORCE_INLINE Ps::aos::FloatV dot(const SpatialVectorV& other) const { return Ps::aos::FAdd(Ps::aos::V3Dot(linear, other.linear), Ps::aos::V3Dot(angular, other.angular)); }

	
}PX_ALIGN_SUFFIX(16);

} // namespace Cm

PX_COMPILE_TIME_ASSERT(sizeof(Cm::SpatialVector) == 32);
PX_COMPILE_TIME_ASSERT(sizeof(Cm::SpatialVectorV) == 32);

}

#endif
