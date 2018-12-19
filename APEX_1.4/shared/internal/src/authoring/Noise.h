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



#ifndef NOISE_H
#define NOISE_H

#include "authoring/ApexCSGMath.h"

#include "NoiseUtils.h"

#ifndef WITHOUT_APEX_AUTHORING
 
namespace ApexCSG 
{

class UserRandom;

/**
	Provides Perlin noise sampling across multiple dimensions and for different data types
*/
template<typename T, int SampleSize = 1024, int D = 3, class VecType = Vec<T, D> >
class PerlinNoise
{
public:
	PerlinNoise(UserRandom& rnd, int octaves = 1, T frequency = 1., T amplitude = 1.)
	  : mRnd(rnd),
		mOctaves(octaves),
		mFrequency(frequency),
		mAmplitude(amplitude),
		mbInit(false)
	{

	}

	void reset(int octaves = 1, T frequency = (T)1., T amplitude = (T)1.)
	{
		mOctaves   = octaves;
		mFrequency = frequency;
		mAmplitude = amplitude;
		init();
	}

	T sample(const VecType& point) 
	{
		return perlinNoise(point);
	}

private:
	PerlinNoise& operator=(const PerlinNoise&);

	T perlinNoise(VecType point)
	{
		if (!mbInit)
			init();

		const int octaves  = mOctaves;
		const T frequency  = mFrequency;
		T amplitude        = mAmplitude;
		T result           = (T)0;

		point *= frequency;

		for (int i = 0; i < octaves; ++i)
		{
			result    += noiseSample<T, SampleSize>(point, p, g) * amplitude;
			point     *= (T)2.0;
			amplitude *= (T)0.5;
		}

		return result;
	}

	void init(void)
	{
		mbInit = true;

		unsigned  i, j;
		int k;

		for (i = 0 ; i < (unsigned)SampleSize; i++)
		{
			p[i]  = (int)i;
			for (j = 0; j < D; ++j)
				g[i][j] = (T)((mRnd.getInt() % (SampleSize + SampleSize)) - SampleSize) / SampleSize;
			g[i].normalize();
		}

		while (--i)
		{
			k    = p[i];
			j = (unsigned)mRnd.getInt() % SampleSize;
			p[i] = p[j];
			p[j] = k;
		}

		for (i = 0 ; i < SampleSize + 2; ++i)
		{
			p [(unsigned)SampleSize + i] =  p[i];
			for (j = 0; j < D; ++j)
				g[(unsigned)SampleSize + i][j] = g[i][j];
		}

	}

	UserRandom& mRnd;
	int   mOctaves;
	T     mFrequency;
	T     mAmplitude;

	// Permutation vector
	int p[(unsigned)(SampleSize + SampleSize + 2)];
	// Gradient vector
	VecType g[(unsigned)(SampleSize + SampleSize + 2)];

	bool  mbInit;
};

}

#endif /* #ifndef WITHOUT_APEX_AUTHORING */

#endif /* #ifndef NOISE_H */

