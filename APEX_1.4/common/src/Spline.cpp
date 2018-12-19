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



#include "Spline.h"
#include <assert.h>

using namespace nvidia;

static float f(float x)
{
	return( x*x*x - x);
}


// build spline.
// copied from Sedgwick's Algorithm's in C, page 548.
void Spline::ComputeSpline(void)
{
	uint32_t i;
	uint32_t n = mNodes.size();

	float *d = new float[n];
	float *w = new float[n];

	for (i=1; i<(n-1); i++)
	{
		d[i] = 2.0f * (mNodes[i+1].x - mNodes[i-1].x);
	}

	for (i=0; i<(n-1); i++) 
	{
		mNodes[i].u = mNodes[i+1].x-mNodes[i].x;}

	for (i=1; i<(n-1); i++)
	{
		w[i] = 6.0f*((mNodes[i+1].y - mNodes[i].y)   / mNodes[i].u - (mNodes[i].y   - mNodes[i-1].y) / mNodes[i-1].u);
	}

	mNodes[0].p   = 0.0f;
	mNodes[n-1].p = 0.0f;

	for (i=1; i<(n-2); i++)
	{
		w[i+1] = w[i+1] - w[i]*mNodes[i].u /d[i];
		d[i+1] = d[i+1] - mNodes[i].u * mNodes[i].u / d[i];
	}

	for (i=n-2; i>0; i--)
	{
		mNodes[i].p = (w[i] - mNodes[i].u * mNodes[i+1].p ) / d[i];
	}

	delete[]d;
	delete[]w;
}

float Spline::Evaluate(float v,uint32_t &index,float &fraction) const
{
	float t;
	uint32_t i=0;
	uint32_t n = mNodes.size();

	while ( i < (n-1) && v > mNodes[i+1].x  ) 
	{
		i++;
	}
	if ( i == (n-1) )
	{
		i = n-2;
	}

	index = (uint32_t)i;

	t = (v - mNodes[i].x ) / mNodes[i].u;

	fraction = t;

	return( t*mNodes[i+1].y + (1-t)*mNodes[i].y + mNodes[i].u * mNodes[i].u * (f(t)*mNodes[i+1].p + f(1-t)*mNodes[i].p )/6.0f );
}


void Spline::AddNode(float x,float y)
{
	SplineNode s;
	s.x = x;
	s.y = y;
	mNodes.pushBack(s);
}

float SplineCurve::AddControlPoint(const PxVec3& p)  // add control point.
{
	int32_t size = mXaxis.GetSize();

	if ( size )
	{
		size--;
		PxVec3 last;
		last.x = mXaxis.GetEntry(size);
		last.y = mYaxis.GetEntry(size);
		last.z = mZaxis.GetEntry(size);
		PxVec3 diff = last-p;
		float dist = diff.magnitude();
		mTime+=dist;
	}
	else
	{
		mTime = 0;
	}

	mXaxis.AddNode(mTime,p.x);
	mYaxis.AddNode(mTime,p.y);
	mZaxis.AddNode(mTime,p.z);
	return mTime;
}

void SplineCurve::AddControlPoint(const PxVec3& p,float t)  // add control point.
{
	int32_t size = mXaxis.GetSize();
	if ( size )
	{
		size--;
		PxVec3 last;
		last.x = mXaxis.GetEntry(size);
		last.y = mYaxis.GetEntry(size);
		last.z = mZaxis.GetEntry(size);
	}

	mTime = t;

	mXaxis.AddNode(mTime,p.x);
	mYaxis.AddNode(mTime,p.y);
	mZaxis.AddNode(mTime,p.z);
}

void SplineCurve::ComputeSpline(void)  // compute spline.
{
	mXaxis.ComputeSpline();
	mYaxis.ComputeSpline();
	mZaxis.ComputeSpline();
}

PxVec3 SplineCurve::Evaluate(float dist,uint32_t &index,float &fraction)
{
	PxVec3 p;

	uint32_t index1,index2,index3;
	float t1,t2,t3;
	p.x = mXaxis.Evaluate(dist,index1,t1);
	p.y = mYaxis.Evaluate(dist,index2,t2);
	p.z = mZaxis.Evaluate(dist,index3,t3);
	assert( index1 == index2 && index1 == index3 && index2 == index3 );
	index = index1;
	fraction = t1;

	return p;
}

PxVec3 SplineCurve::GetEntry(int32_t i)
{
	PxVec3 p;
	if ( i >= 0 && i < GetSize() )
	{
		p.x = mXaxis.GetEntry(i);
		p.y = mYaxis.GetEntry(i);
		p.z = mZaxis.GetEntry(i);
	}
	return p;
}
