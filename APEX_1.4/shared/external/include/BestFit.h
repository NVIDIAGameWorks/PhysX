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



#ifndef BEST_FIT_H

#define BEST_FIT_H

// A code snippet to compute the best fit AAB, OBB, plane, capsule and sphere
// Quaternions are assumed a float X,Y,Z,W
// Matrices are assumed 4x4 D3DX style format passed as a float pointer
// The orientation of a capsule is assumed that height is along the Y axis, the same format as the PhysX SDK uses
// The best fit plane routine is derived from code previously published by David Eberly on his Magic Software site.
// The best fit OBB is computed by first approximating the best fit plane, and then brute force rotating the points
// around a single axis to derive the closest fit.  If you set 'bruteforce' to false, it will just use the orientation
// derived from the best fit plane, which is close enough in most cases, but not all.
// Each routine allows you to pass the point stride between position elements in your input vertex stream.
// These routines should all be thread safe as they make no use of any global variables.



namespace SharedTools
{

bool  computeBestFitPlane(size_t vcount,	// number of input data points
                          const float* points,					// starting address of points array.
                          size_t vstride,							// stride between input points.
                          const float* weights,					// *optional point weighting values.
                          size_t wstride,							// weight stride for each vertex.
                          float plane[4]);

float  computeBestFitAABB(size_t vcount, const float* points, size_t pstride, float bmin[3], float bmax[3]); // returns the diagonal distance
float  computeBestFitSphere(size_t vcount, const float* points, size_t pstride, float center[3]);
void   computeBestFitOBB(size_t vcount, const float* points, size_t pstride, float* sides, float matrix[16], bool bruteForce);
void   computeBestFitOBB(size_t vcount, const float* points, size_t pstride, float* sides, float pos[3], float quat[4], bool bruteForce);
void   computeBestFitCapsule(size_t vcount, const float* points, size_t pstride, float& radius, float& height, float matrix[16], bool bruteForce);
void   computeBestFitCapsule(size_t vcount, const float* points, size_t pstride, float& radius, float& height, float pos[3], float quat[4], bool bruteForce);

};

#endif
