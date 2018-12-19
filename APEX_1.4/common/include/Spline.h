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



#ifndef SPLINE_H

#define SPLINE_H


/** @file spline.h
 *  @brief A utility class to manage 3d spline curves.
 *
 *  This is used heavily by the terrain terraforming tools for roads, lakes, and flatten operations.
 *
 *  @author John W. Ratcliff
*/

/** @file spline.cpp
 *  @brief A utility class to manage 3d spline curves.
 *
 *  This is used heavily by the terrain terraforming tools for roads, lakes, and flatten operations.
 *
 *  @author John W. Ratcliff
*/


#include "PsArray.h"
#include "PsUserAllocated.h"
#include "PxVec3.h"
#include "ApexUsingNamespace.h"

class SplineNode
{
public:
	float GetY(void) const
	{ 
		return y; 
	};
	float x;              // time/distance x-axis component.
	float y;              // y component.
	float u;
	float p;
};

typedef nvidia::Array< SplineNode > SplineNodeVector;
typedef nvidia::Array< physx::PxVec3 > PxVec3Vector;
typedef nvidia::Array< uint32_t > uint32_tVector;

class Spline : public nvidia::UserAllocated
{
public:
	void Reserve(int32_t size)
	{
		mNodes.reserve((uint32_t)size);
	};
	void AddNode(float x,float y);
	void ComputeSpline(void);
	float Evaluate(float x,uint32_t &index,float &fraction) const; // evaluate Y component based on X
	int32_t GetSize(void)  const
	{ 
		return (int32_t)mNodes.size(); 
	}
	float GetEntry(int32_t i) const 
	{ 
		return mNodes[(uint32_t)i].GetY(); 
	};
	void Clear(void)
	{
		mNodes.clear();
	};
private:
	SplineNodeVector mNodes; // nodes.
};

class SplineCurve : public nvidia::UserAllocated
{
public:
	void Reserve(int32_t size)
	{
		mXaxis.Reserve(size);
		mYaxis.Reserve(size);
		mZaxis.Reserve(size);
	};

	void setControlPoints(const PxVec3Vector &points)
	{
		Clear();
		Reserve( (int32_t)points.size() );
		for (uint32_t i=0; i<points.size(); i++)
		{
			AddControlPoint(points[i]);
		}
		ComputeSpline();
	}

	float AddControlPoint(const physx::PxVec3& p); // add control point, time is computed based on distance along the curve.
	void AddControlPoint(const physx::PxVec3& p,float t); // add control point.

	void GetPointOnSpline(float t,physx::PxVec3 &pos)
	{
		float d = t*mTime;
		uint32_t index;
		float fraction;
		pos = Evaluate(d,index,fraction);
	}

	physx::PxVec3 Evaluate(float dist,uint32_t &index,float &fraction);

	float GetLength(void) { return mTime; }; //total length of spline

	int32_t GetSize(void) { return mXaxis.GetSize(); };

	physx::PxVec3 GetEntry(int32_t i);

	void ComputeSpline(void); // compute spline.

	void Clear(void)
	{
		mXaxis.Clear();
		mYaxis.Clear();
		mZaxis.Clear();
		mTime = 0;
	};

	float Set(const PxVec3Vector &vlist)
	{
		Clear();
		int32_t count = (int32_t)vlist.size();
		Reserve(count);
		for (uint32_t i=0; i<vlist.size(); i++)
		{
			AddControlPoint( vlist[i] );
		}
		ComputeSpline();
		return mTime;
	};

	void ResampleControlPoints(const PxVec3Vector &inputpoints,
									  PxVec3Vector &outputpoints,
									  uint32_tVector &outputIndex,
 									float dtime)
	{
		float length = Set(inputpoints);
		for (float l=0; l<=length; l+=dtime)
		{
			uint32_t index;
			float fraction;
			physx::PxVec3 pos = Evaluate(l,index,fraction);
			outputpoints.pushBack(pos);
			outputIndex.pushBack(index);
		}
	};

private:
	float  mTime; // time/distance traveled.
	Spline mXaxis;
	Spline mYaxis;
	Spline mZaxis;
};

#endif
