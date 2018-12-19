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



#ifndef APEX_MATH_H
#define APEX_MATH_H

#include "PxMat44.h"
#include "PsMathUtils.h"

#include "PsVecMath.h"
namespace nvidia
{

#define APEX_ALIGN_UP(offset, alignment) (((offset) + (alignment)-1) & ~((alignment)-1))

/**
 * computes weight * _origin + (1.0f - weight) * _target
 */
PX_INLINE PxMat44 interpolateMatrix(float weight, const PxMat44& _origin, const PxMat44& _target)
{
	// target: normalize, save scale, transform to quat
	PxMat33 target(_target.column0.getXYZ(),
					_target.column1.getXYZ(),
					_target.column2.getXYZ());
	PxVec3 axis0 = target.column0;
	PxVec3 axis1 = target.column1;
	PxVec3 axis2 = target.column2;
	const PxVec4 targetScale(axis0.normalize(), axis1.normalize(), axis2.normalize(), 1.0f);
	target.column0 = axis0;
	target.column1 = axis1;
	target.column2 = axis2;
	const PxQuat targetQ = PxQuat(target);

	// origin: normalize, save scale, transform to quat
	PxMat33 origin(_origin.column0.getXYZ(),
					_origin.column1.getXYZ(),
					_origin.column2.getXYZ());
	const PxVec4 originScale(axis0.normalize(), axis1.normalize(), axis2.normalize(), 1.0f);
	origin.column0 = axis0;
	origin.column1 = axis1;
	origin.column2 = axis2;
	const PxQuat originQ = PxQuat(origin);

	// interpolate
	PxQuat relativeQ = physx::shdfnd::slerp(1.0f - weight, originQ, targetQ);
	PxMat44 relative(relativeQ);
	relative.setPosition(weight * _origin.getPosition() + (1.0f - weight) * _target.getPosition());

	PxMat44 _relative = relative;
	const PxVec4 scale = weight * originScale + (1.0f - weight) * targetScale;
	_relative.scale(scale);

	return _relative;
}

bool operator != (const PxMat44& a, const PxMat44& b);

}


#endif // APEX_MATH_H
