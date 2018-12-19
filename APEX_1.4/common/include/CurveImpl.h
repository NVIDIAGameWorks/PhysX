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



#ifndef __CURVE_IMPL_H__
#define __CURVE_IMPL_H__

#include "Apex.h"
#include "PxAssert.h"
#include "nvparameterized/NvParameterized.h"
#include "PsArray.h"
#include "Curve.h"
#include <ApexUsingNamespace.h>

namespace nvidia
{
namespace apex
{

typedef physx::Array<Vec2R> Vec2RPointArray;

/**
	The CurveImpl is a class for storing control points on a curve and evaluating the results later.
*/
class CurveImpl : public Curve
{
public:
	CurveImpl()
	{}

	~CurveImpl()
	{}

	/**
		Retrieve the output Y for the specified input x, based on the properties of the stored curve described
		by mControlPoints.
	*/
	float evaluate(float x) const;

	/**
		Add a control point to the list of control points, returning the index of the new point.
	*/
	uint32_t addControlPoint(const Vec2R& controlPoint);

	/**
		Add a control points to the list of control points.  Assuming the
		hPoints points to a list of vec2s
	*/
	void addControlPoints(::NvParameterized::Interface* param, ::NvParameterized::Handle& hPoints);

	/**
		Locates the control points that contain x, placing the resulting control points in the two
		out parameters. Returns true if the points were found, false otherwise. If the points were not
		found, the output variables are untouched
	*/
	bool calculateControlPoints(float x, Vec2R& outXPoints, Vec2R& outYPoints) const;

	/**
		Locates the first control point with x larger than xValue or the nimber of control points if such point doesn't exist
	*/
	uint32_t calculateFollowingControlPoint(float xValue) const;

	///get the array of control points
	const Vec2R* getControlPoints(uint32_t& outCount) const;

private:
	// mControlPoints is a sorted list of control points for a curve. Currently, the curve is a lame
	// lirp'd curve. We could add support for other curvetypes in the future, either bezier curves,
	// splines, etc.
	Vec2RPointArray mControlPoints;
};

}
} // namespace apex

#endif /* __CURVE_IMPL_H__ */
