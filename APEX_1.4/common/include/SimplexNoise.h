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



#ifndef SIMPLEX_NOISE_H
#define SIMPLEX_NOISE_H

#include "PxVec4.h"

namespace nvidia
{
namespace apex
{

class SimplexNoise
{
	static const int X_NOISE_GEN = 1619;
	static const int Y_NOISE_GEN = 31337;
	static const int Z_NOISE_GEN = 6971;
	static const int W_NOISE_GEN = 1999;
	static const int SEED_NOISE_GEN = 1013;
	static const int SHIFT_NOISE_GEN = 8;

	PX_CUDA_CALLABLE static PX_INLINE int fastfloor(float x)
	{
		return (x >= 0) ? (int)x : (int)(x - 1);
	}

public:

	// 4D simplex noise
	// returns: (x,y,z) = noise grad, w = noise value
	PX_CUDA_CALLABLE static physx::PxVec4 eval4D(float x, float y, float z, float w, int seed)
	{
		// The skewing and unskewing factors are hairy again for the 4D case
		const float F4 = (physx::PxSqrt(5.0f) - 1.0f) / 4.0f;
		const float G4 = (5.0f - physx::PxSqrt(5.0f)) / 20.0f;
		// Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
		float s = (x + y + z + w) * F4; // Factor for 4D skewing
		int ix = fastfloor(x + s);
		int iy = fastfloor(y + s);
		int iz = fastfloor(z + s);
		int iw = fastfloor(w + s);
		float t = (ix + iy + iz + iw) * G4; // Factor for 4D unskewing
		// Unskew the cell origin back to (x,y,z,w) space
		float x0 = x - (ix - t); // The x,y,z,w distances from the cell origin
		float y0 = y - (iy - t);
		float z0 = z - (iz - t);
		float w0 = w - (iw - t);

		int c  = (x0 > y0) ? (1 << 0) : (1 << 2);
		c += (x0 > z0) ? (1 << 0) : (1 << 4);
		c += (x0 > w0) ? (1 << 0) : (1 << 6);
		c += (y0 > z0) ? (1 << 2) : (1 << 4);
		c += (y0 > w0) ? (1 << 2) : (1 << 6);
		c += (z0 > w0) ? (1 << 4) : (1 << 6);

		physx::PxVec4 res;
		res.setZero();

		// Calculate the contribution from the five corners
#ifdef __CUDACC__
#pragma unroll
#endif
		for (int p = 4; p >= 0; --p)
		{
			int ixp = ((c >> 0) & 3) >= p ? 1 : 0;
			int iyp = ((c >> 2) & 3) >= p ? 1 : 0;
			int izp = ((c >> 4) & 3) >= p ? 1 : 0;
			int iwp = ((c >> 6) & 3) >= p ? 1 : 0;

			float xp = x0 - ixp + (4 - p) * G4;
			float yp = y0 - iyp + (4 - p) * G4;
			float zp = z0 - izp + (4 - p) * G4;
			float wp = w0 - iwp + (4 - p) * G4;

			float t = 0.6f - xp * xp - yp * yp - zp * zp - wp * wp;
			if (t > 0)
			{
				//get index
				int gradIndex = int((
				                    X_NOISE_GEN    * (ix + ixp)
				                    + Y_NOISE_GEN    * (iy + iyp)
				                    + Z_NOISE_GEN    * (iz + izp)
				                    + W_NOISE_GEN    * (iw + iwp)
				                    + SEED_NOISE_GEN * seed)
				                & 0xffffffff);
				gradIndex ^= (gradIndex >> SHIFT_NOISE_GEN);
				gradIndex &= 31;

				physx::PxVec4 g;
				{
					const int h = gradIndex;
					const int hs = 2 - (h >> 4);
					const int h1 = (h >> 3);
					g.x = (h1 == 0) ? 0.0f : ((h & 4)       ? -1.0f : 1.0f);
					g.y = (h1 == 1) ? 0.0f : ((h & (hs << 1)) ? -1.0f : 1.0f);
					g.z = (h1 == 2) ? 0.0f : ((h & hs)      ? -1.0f : 1.0f);
					g.w = (h1 == 3) ? 0.0f : ((h &  1)      ? -1.0f : 1.0f);
				}
				float gdot = (g.x * xp + g.y * yp + g.z * zp + g.w * wp);

				float t2 = t * t;
				float t3 = t2 * t;
				float t4 = t3 * t;

				float dt4gdot = 8 * t3 * gdot;

				res.x += t4 * g.x - dt4gdot * xp;
				res.y += t4 * g.y - dt4gdot * yp;
				res.z += t4 * g.z - dt4gdot * zp;
				res.w += t4 * gdot;
			}
		}
		// scale the result to cover the range [-1,1]
		res *= 27;
		return res;
	}
};


}
} // namespace nvidia::apex

#endif
