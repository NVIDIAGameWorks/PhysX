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



#ifndef MESHIMPORT_H
#define MESHIMPORT_H

#include <stdio.h>
#include <string.h>
#if defined(__APPLE__)
#include <string>
#endif
#include <float.h>
#if !defined(__CELLOS_LV2__) && !defined(__APPLE__) && !defined(__PS4__)
#include <malloc.h>
#endif
#include <math.h>

#include "MeshImportTypes.h"

#pragma warning(push)
#pragma warning(disable:4996)


namespace mimp
{

inline void  fmi_multiplyTransform(const MiF32 *pA,const MiF32 *pB,MiF32 *pM)
{
	MiF32 a = pA[0*4+0] * pB[0*4+0] + pA[0*4+1] * pB[1*4+0] + pA[0*4+2] * pB[2*4+0] + pA[0*4+3] * pB[3*4+0];
	MiF32 b = pA[0*4+0] * pB[0*4+1] + pA[0*4+1] * pB[1*4+1] + pA[0*4+2] * pB[2*4+1] + pA[0*4+3] * pB[3*4+1];
	MiF32 c = pA[0*4+0] * pB[0*4+2] + pA[0*4+1] * pB[1*4+2] + pA[0*4+2] * pB[2*4+2] + pA[0*4+3] * pB[3*4+2];
	MiF32 d = pA[0*4+0] * pB[0*4+3] + pA[0*4+1] * pB[1*4+3] + pA[0*4+2] * pB[2*4+3] + pA[0*4+3] * pB[3*4+3];

	MiF32 e = pA[1*4+0] * pB[0*4+0] + pA[1*4+1] * pB[1*4+0] + pA[1*4+2] * pB[2*4+0] + pA[1*4+3] * pB[3*4+0];
	MiF32 f = pA[1*4+0] * pB[0*4+1] + pA[1*4+1] * pB[1*4+1] + pA[1*4+2] * pB[2*4+1] + pA[1*4+3] * pB[3*4+1];
	MiF32 g = pA[1*4+0] * pB[0*4+2] + pA[1*4+1] * pB[1*4+2] + pA[1*4+2] * pB[2*4+2] + pA[1*4+3] * pB[3*4+2];
	MiF32 h = pA[1*4+0] * pB[0*4+3] + pA[1*4+1] * pB[1*4+3] + pA[1*4+2] * pB[2*4+3] + pA[1*4+3] * pB[3*4+3];

	MiF32 i = pA[2*4+0] * pB[0*4+0] + pA[2*4+1] * pB[1*4+0] + pA[2*4+2] * pB[2*4+0] + pA[2*4+3] * pB[3*4+0];
	MiF32 j = pA[2*4+0] * pB[0*4+1] + pA[2*4+1] * pB[1*4+1] + pA[2*4+2] * pB[2*4+1] + pA[2*4+3] * pB[3*4+1];
	MiF32 k = pA[2*4+0] * pB[0*4+2] + pA[2*4+1] * pB[1*4+2] + pA[2*4+2] * pB[2*4+2] + pA[2*4+3] * pB[3*4+2];
	MiF32 l = pA[2*4+0] * pB[0*4+3] + pA[2*4+1] * pB[1*4+3] + pA[2*4+2] * pB[2*4+3] + pA[2*4+3] * pB[3*4+3];

	MiF32 m = pA[3*4+0] * pB[0*4+0] + pA[3*4+1] * pB[1*4+0] + pA[3*4+2] * pB[2*4+0] + pA[3*4+3] * pB[3*4+0];
	MiF32 n = pA[3*4+0] * pB[0*4+1] + pA[3*4+1] * pB[1*4+1] + pA[3*4+2] * pB[2*4+1] + pA[3*4+3] * pB[3*4+1];
	MiF32 o = pA[3*4+0] * pB[0*4+2] + pA[3*4+1] * pB[1*4+2] + pA[3*4+2] * pB[2*4+2] + pA[3*4+3] * pB[3*4+2];
	MiF32 p = pA[3*4+0] * pB[0*4+3] + pA[3*4+1] * pB[1*4+3] + pA[3*4+2] * pB[2*4+3] + pA[3*4+3] * pB[3*4+3];

	pM[0] = a;  pM[1] = b;  pM[2] = c;  pM[3] = d;

	pM[4] = e;  pM[5] = f;  pM[6] = g;  pM[7] = h;

	pM[8] = i;  pM[9] = j;  pM[10] = k;  pM[11] = l;

	pM[12] = m;  pM[13] = n;  pM[14] = o;  pM[15] = p;
}


inline MiF32 fmi_computePlane(const MiF32 *A,const MiF32 *B,const MiF32 *C,MiF32 *n) // returns D
{
	MiF32 vx = (B[0] - C[0]);
	MiF32 vy = (B[1] - C[1]);
	MiF32 vz = (B[2] - C[2]);

	MiF32 wx = (A[0] - B[0]);
	MiF32 wy = (A[1] - B[1]);
	MiF32 wz = (A[2] - B[2]);

	MiF32 vw_x = vy * wz - vz * wy;
	MiF32 vw_y = vz * wx - vx * wz;
	MiF32 vw_z = vx * wy - vy * wx;

	MiF32 mag = static_cast<MiF32>(sqrt((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z)));

	if ( mag < 0.000001f )
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f/mag;
	}

	MiF32 x = vw_x * mag;
	MiF32 y = vw_y * mag;
	MiF32 z = vw_z * mag;


	MiF32 D = 0.0f - ((x*A[0])+(y*A[1])+(z*A[2]));

	n[0] = x;
	n[1] = y;
	n[2] = z;

	return D;
}

inline void  fmi_transform(const MiF32 matrix[16],const MiF32 v[3],MiF32 t[3]) // rotate and translate this point
{
	if ( matrix )
	{
		MiF32 tx = (matrix[0*4+0] * v[0]) +  (matrix[1*4+0] * v[1]) + (matrix[2*4+0] * v[2]) + matrix[3*4+0];
		MiF32 ty = (matrix[0*4+1] * v[0]) +  (matrix[1*4+1] * v[1]) + (matrix[2*4+1] * v[2]) + matrix[3*4+1];
		MiF32 tz = (matrix[0*4+2] * v[0]) +  (matrix[1*4+2] * v[1]) + (matrix[2*4+2] * v[2]) + matrix[3*4+2];
		t[0] = tx;
		t[1] = ty;
		t[2] = tz;
	}
	else
	{
		t[0] = v[0];
		t[1] = v[1];
		t[2] = v[2];
	}
}

inline void  fmi_transformRotate(const MiF32 matrix[16],const MiF32 v[3],MiF32 t[3]) // rotate only
{
	if ( matrix )
	{
		MiF32 tx = (matrix[0*4+0] * v[0]) +  (matrix[1*4+0] * v[1]) + (matrix[2*4+0] * v[2]);
		MiF32 ty = (matrix[0*4+1] * v[0]) +  (matrix[1*4+1] * v[1]) + (matrix[2*4+1] * v[2]);
		MiF32 tz = (matrix[0*4+2] * v[0]) +  (matrix[1*4+2] * v[1]) + (matrix[2*4+2] * v[2]);
		t[0] = tx;
		t[1] = ty;
		t[2] = tz;
	}
	else
	{
		t[0] = v[0];
		t[1] = v[1];
		t[2] = v[2];
	}
}

inline MiF32 fmi_normalize(MiF32 *n) // normalize this vector
{
	MiF32 dist = (MiF32)sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
	if ( dist > 0.0000001f )
	{
		MiF32 mag = 1.0f / dist;
		n[0]*=mag;
		n[1]*=mag;
		n[2]*=mag;
	}
	else
	{
		n[0] = 1;
		n[1] = 0;
		n[2] = 0;
	}

	return dist;
}


inline void fmi_quatToMatrix(const MiF32 *quat,MiF32 *matrix) // convert quaterinion rotation to matrix, zeros out the translation component.
{

	MiF32 xx = quat[0]*quat[0];
	MiF32 yy = quat[1]*quat[1];
	MiF32 zz = quat[2]*quat[2];
	MiF32 xy = quat[0]*quat[1];
	MiF32 xz = quat[0]*quat[2];
	MiF32 yz = quat[1]*quat[2];
	MiF32 wx = quat[3]*quat[0];
	MiF32 wy = quat[3]*quat[1];
	MiF32 wz = quat[3]*quat[2];

	matrix[0*4+0] = 1 - 2 * ( yy + zz );
	matrix[1*4+0] =     2 * ( xy - wz );
	matrix[2*4+0] =     2 * ( xz + wy );

	matrix[0*4+1] =     2 * ( xy + wz );
	matrix[1*4+1] = 1 - 2 * ( xx + zz );
	matrix[2*4+1] =     2 * ( yz - wx );

	matrix[0*4+2] =     2 * ( xz - wy );
	matrix[1*4+2] =     2 * ( yz + wx );
	matrix[2*4+2] = 1 - 2 * ( xx + yy );

	matrix[3*4+0] = matrix[3*4+1] = matrix[3*4+2] = (MiF32) 0.0f;
	matrix[0*4+3] = matrix[1*4+3] = matrix[2*4+3] = (MiF32) 0.0f;
	matrix[3*4+3] =(MiF32) 1.0f;

}



// minimal support math routines
// *** Support math routines
inline void fmi_getAngleAxis(MiF32 &angle,MiF32 *axis,const MiF32 *quat)
{
	//return axis and angle of rotation of quaternion
	MiF32 x = quat[0];
	MiF32 y = quat[1];
	MiF32 z = quat[2];
	MiF32 w = quat[3];

	angle = acosf(w) * 2.0f;        //this is getAngle()
	MiF32 sa = sqrtf(1.0f - w*w);
	if (sa)
	{
		axis[0] = x/sa;
		axis[1] = y/sa;
		axis[2] = z/sa;
	}
	else
	{
		axis[0] = 1;
		axis[1] = 0;
		axis[2] = 0;
	}
}

inline void fmi_setOrientationFromAxisAngle(const MiF32 axis[3],MiF32 angle,MiF32 *quat)
{
	MiF32 x,y,z,w;

	x = axis[0];
	y = axis[1];
	z = axis[2];

	// required: Normalize the axis

	const MiF32 i_length =  MiF32(1.0f) / sqrtf( x*x + y*y + z*z );

	x = x * i_length;
	y = y * i_length;
	z = z * i_length;

	// now make a clQuaternionernion out of it
	MiF32 Half = angle * MiF32(0.5);

	w = cosf(Half);//this used to be w/o deg to rad.
	const MiF32 sin_theta_over_two = sinf(Half);

	x = x * sin_theta_over_two;
	y = y * sin_theta_over_two;
	z = z * sin_theta_over_two;

	quat[0] = x;
	quat[1] = y;
	quat[2] = z;
	quat[3] = w;
}


inline void fmi_identity(MiF32 *matrix)
{
	matrix[0*4+0] = 1;    matrix[1*4+1] = 1;    matrix[2*4+2] = 1;    matrix[3*4+3] = 1;
	matrix[1*4+0] = 0;    matrix[2*4+0] = 0;    matrix[3*4+0] = 0;
	matrix[0*4+1] = 0;    matrix[2*4+1] = 0;    matrix[3*4+1] = 0;
	matrix[0*4+2] = 0;    matrix[1*4+2] = 0;    matrix[3*4+2] = 0;
	matrix[0*4+3] = 0;    matrix[1*4+3] = 0;    matrix[2*4+3] = 0;
}


inline void fmi_fromQuat(MiF32 *matrix,const MiF32 quat[4])
{
	fmi_identity(matrix);

	MiF32 xx = quat[0]*quat[0];
	MiF32 yy = quat[1]*quat[1];
	MiF32 zz = quat[2]*quat[2];
	MiF32 xy = quat[0]*quat[1];
	MiF32 xz = quat[0]*quat[2];
	MiF32 yz = quat[1]*quat[2];
	MiF32 wx = quat[3]*quat[0];
	MiF32 wy = quat[3]*quat[1];
	MiF32 wz = quat[3]*quat[2];

	matrix[0*4+0] = 1 - 2 * ( yy + zz );
	matrix[1*4+0] =     2 * ( xy - wz );
	matrix[2*4+0] =     2 * ( xz + wy );

	matrix[0*4+1] =     2 * ( xy + wz );
	matrix[1*4+1] = 1 - 2 * ( xx + zz );
	matrix[2*4+1] =     2 * ( yz - wx );

	matrix[0*4+2] =     2 * ( xz - wy );
	matrix[1*4+2] =     2 * ( yz + wx );
	matrix[2*4+2] = 1 - 2 * ( xx + yy );

	matrix[3*4+0] = matrix[3*4+1] = matrix[3*4+2] = (MiF32) 0.0f;
	matrix[0*4+3] = matrix[1*4+3] = matrix[2*4+3] = (MiF32) 0.0f;
	matrix[3*4+3] =(MiF32) 1.0f;
}

inline void fmi_matrixToQuat(const MiF32 *matrix,MiF32 *quat) // convert the 3x3 portion of a 4x4 matrix into a quaterion as x,y,z,w
{

	MiF32 tr = matrix[0*4+0] + matrix[1*4+1] + matrix[2*4+2];

	// check the diagonal

	if (tr > 0.0f )
	{
		MiF32 s = sqrtf((tr + 1.0f) );
		quat[3] = s * 0.5f;
		s = 0.5f / s;
		quat[0] = (matrix[1*4+2] - matrix[2*4+1]) * s;
		quat[1] = (matrix[2*4+0] - matrix[0*4+2]) * s;
		quat[2] = (matrix[0*4+1] - matrix[1*4+0]) * s;

	}
	else
	{
		// diagonal is negative
		MiI32 nxt[3] = {1, 2, 0};
		MiF32  qa[4];

		MiI32 i = 0;

		if (matrix[1*4+1] > matrix[0*4+0]) i = 1;
		if (matrix[2*4+2] > matrix[i*4+i]) i = 2;

		MiI32 j = nxt[i];
		MiI32 k = nxt[j];

		MiF32 s = sqrtf( ((matrix[i*4+i] - (matrix[j*4+j] + matrix[k*4+k])) + 1.0f) );

		qa[i] = s * 0.5f;

		if (s != 0.0f ) s = 0.5f / s;

		qa[3] = (matrix[j*4+k] - matrix[k*4+j]) * s;
		qa[j] = (matrix[i*4+j] + matrix[j*4+i]) * s;
		qa[k] = (matrix[i*4+k] + matrix[k*4+i]) * s;

		quat[0] = qa[0];
		quat[1] = qa[1];
		quat[2] = qa[2];
		quat[3] = qa[3];
	}
}


inline MiF32 fmi_squared(MiF32 x) { return x*x; };

inline void fmi_decomposeTransform(const MiF32 local_transform[16],MiF32 trans[3],MiF32 rot[4],MiF32 scale[3])
{

	trans[0] = local_transform[12];
	trans[1] = local_transform[13];
	trans[2] = local_transform[14];

	scale[0] = sqrtf(fmi_squared(local_transform[0*4+0]) + fmi_squared(local_transform[0*4+1]) + fmi_squared(local_transform[0*4+2]));
	scale[1] = sqrtf(fmi_squared(local_transform[1*4+0]) + fmi_squared(local_transform[1*4+1]) + fmi_squared(local_transform[1*4+2]));
	scale[2] = sqrtf(fmi_squared(local_transform[2*4+0]) + fmi_squared(local_transform[2*4+1]) + fmi_squared(local_transform[2*4+2]));

	MiF32 m[16];
	memcpy(m,local_transform,sizeof(MiF32)*16);

	MiF32 sx = 1.0f / scale[0];
	MiF32 sy = 1.0f / scale[1];
	MiF32 sz = 1.0f / scale[2];

	m[0*4+0]*=sx;
	m[0*4+1]*=sx;
	m[0*4+2]*=sx;

	m[1*4+0]*=sy;
	m[1*4+1]*=sy;
	m[1*4+2]*=sy;

	m[2*4+0]*=sz;
	m[2*4+1]*=sz;
	m[2*4+2]*=sz;

	fmi_matrixToQuat(m,rot);
}

inline void fmi_fromScale(MiF32 *matrix,const MiF32 scale[3])
{
	fmi_identity(matrix);
	matrix[0*4+0] = scale[0];
	matrix[1*4+1] = scale[1];
	matrix[2*4+2] = scale[2];
}

inline void  fmi_multiply(const MiF32 *pA,const MiF32 *pB,MiF32 *pM)
{

	MiF32 a = pA[0*4+0] * pB[0*4+0] + pA[0*4+1] * pB[1*4+0] + pA[0*4+2] * pB[2*4+0] + pA[0*4+3] * pB[3*4+0];
	MiF32 b = pA[0*4+0] * pB[0*4+1] + pA[0*4+1] * pB[1*4+1] + pA[0*4+2] * pB[2*4+1] + pA[0*4+3] * pB[3*4+1];
	MiF32 c = pA[0*4+0] * pB[0*4+2] + pA[0*4+1] * pB[1*4+2] + pA[0*4+2] * pB[2*4+2] + pA[0*4+3] * pB[3*4+2];
	MiF32 d = pA[0*4+0] * pB[0*4+3] + pA[0*4+1] * pB[1*4+3] + pA[0*4+2] * pB[2*4+3] + pA[0*4+3] * pB[3*4+3];

	MiF32 e = pA[1*4+0] * pB[0*4+0] + pA[1*4+1] * pB[1*4+0] + pA[1*4+2] * pB[2*4+0] + pA[1*4+3] * pB[3*4+0];
	MiF32 f = pA[1*4+0] * pB[0*4+1] + pA[1*4+1] * pB[1*4+1] + pA[1*4+2] * pB[2*4+1] + pA[1*4+3] * pB[3*4+1];
	MiF32 g = pA[1*4+0] * pB[0*4+2] + pA[1*4+1] * pB[1*4+2] + pA[1*4+2] * pB[2*4+2] + pA[1*4+3] * pB[3*4+2];
	MiF32 h = pA[1*4+0] * pB[0*4+3] + pA[1*4+1] * pB[1*4+3] + pA[1*4+2] * pB[2*4+3] + pA[1*4+3] * pB[3*4+3];

	MiF32 i = pA[2*4+0] * pB[0*4+0] + pA[2*4+1] * pB[1*4+0] + pA[2*4+2] * pB[2*4+0] + pA[2*4+3] * pB[3*4+0];
	MiF32 j = pA[2*4+0] * pB[0*4+1] + pA[2*4+1] * pB[1*4+1] + pA[2*4+2] * pB[2*4+1] + pA[2*4+3] * pB[3*4+1];
	MiF32 k = pA[2*4+0] * pB[0*4+2] + pA[2*4+1] * pB[1*4+2] + pA[2*4+2] * pB[2*4+2] + pA[2*4+3] * pB[3*4+2];
	MiF32 l = pA[2*4+0] * pB[0*4+3] + pA[2*4+1] * pB[1*4+3] + pA[2*4+2] * pB[2*4+3] + pA[2*4+3] * pB[3*4+3];

	MiF32 m = pA[3*4+0] * pB[0*4+0] + pA[3*4+1] * pB[1*4+0] + pA[3*4+2] * pB[2*4+0] + pA[3*4+3] * pB[3*4+0];
	MiF32 n = pA[3*4+0] * pB[0*4+1] + pA[3*4+1] * pB[1*4+1] + pA[3*4+2] * pB[2*4+1] + pA[3*4+3] * pB[3*4+1];
	MiF32 o = pA[3*4+0] * pB[0*4+2] + pA[3*4+1] * pB[1*4+2] + pA[3*4+2] * pB[2*4+2] + pA[3*4+3] * pB[3*4+2];
	MiF32 p = pA[3*4+0] * pB[0*4+3] + pA[3*4+1] * pB[1*4+3] + pA[3*4+2] * pB[2*4+3] + pA[3*4+3] * pB[3*4+3];

	pM[0] = a;  pM[1] = b;  pM[2] = c;  pM[3] = d;

	pM[4] = e;  pM[5] = f;  pM[6] = g;  pM[7] = h;

	pM[8] = i;  pM[9] = j;  pM[10] = k;  pM[11] = l;

	pM[12] = m;  pM[13] = n;  pM[14] = o;  pM[15] = p;
}


inline void fmi_setTranslation(MiF32 *matrix,const MiF32 pos[3])
{
	matrix[12] = pos[0];  matrix[13] = pos[1];  matrix[14] = pos[2];
}


// compose this transform
inline void fmi_composeTransform(const MiF32 pos[3],const MiF32 quat[4],const MiF32 scale[3],MiF32 matrix[16])
{
	MiF32 mscale[16];
	MiF32 mrot[16];
	fmi_fromQuat(mrot,quat);
	fmi_fromScale(mscale,scale);
	fmi_multiply(mscale,mrot,matrix);
	fmi_setTranslation(matrix,pos);
}

inline MiF32 fmi_dot(const MiF32 *p1,const MiF32 *p2)
{
	return p1[0]*p2[0]+p1[1]*p2[1]+p1[2]*p2[2];
}

inline void fmi_cross(MiF32 *cross,const MiF32 *a,const MiF32 *b)
{
	cross[0] = a[1]*b[2] - a[2]*b[1];
	cross[1] = a[2]*b[0] - a[0]*b[2];
	cross[2] = a[0]*b[1] - a[1]*b[0];
}


inline MiF32 fmi_getDeterminant(const MiF32 matrix[16])
{
	MiF32 tempv[3];
	MiF32 p0[3];
	MiF32 p1[3];
	MiF32 p2[3];

	p0[0] = matrix[0*4+0];
	p0[1] = matrix[0*4+1];
	p0[2] = matrix[0*4+2];

	p1[0] = matrix[1*4+0];
	p1[1] = matrix[1*4+1];
	p1[2] = matrix[1*4+2];

	p2[0] = matrix[2*4+0];
	p2[1] = matrix[2*4+1];
	p2[2] = matrix[2*4+2];

	fmi_cross(tempv,p1,p2);

	return fmi_dot(p0,tempv);

}

inline void fmi_getSubMatrix(MiI32 ki,MiI32 kj,MiF32 pDst[16],const MiF32 matrix[16])
{
	MiI32 row, col;
	MiI32 dstCol = 0, dstRow = 0;

	for ( col = 0; col < 4; col++ )
	{
		if ( col == kj )
		{
			continue;
		}
		for ( dstRow = 0, row = 0; row < 4; row++ )
		{
			if ( row == ki )
			{
				continue;
			}
			pDst[dstCol*4+dstRow] = matrix[col*4+row];
			dstRow++;
		}
		dstCol++;
	}
}

inline void fmi_inverseTransform(const MiF32 matrix[16],MiF32 inverse_matrix[16])
{
	MiF32 determinant = fmi_getDeterminant(matrix);
	determinant = 1.0f / determinant;
	for (MiI32 i = 0; i < 4; i++ )
	{
		for (MiI32 j = 0; j < 4; j++ )
		{
			MiI32 sign = 1 - ( ( i + j ) % 2 ) * 2;
			MiF32 subMat[16];
			fmi_identity(subMat);
			fmi_getSubMatrix( i, j, subMat, matrix );
			MiF32 subDeterminant = fmi_getDeterminant(subMat);
			inverse_matrix[i*4+j] = ( subDeterminant * sign ) * determinant;
		}
	}
}

static char* skipSoft(char* scan, char& quote)
{
	quote = 0;

	while (*scan)
	{
		if (*scan == 34)
		{
			quote = 1;
			scan++;
			break;
		}
		else if (*scan == '(')
		{
			quote = 2;
			scan++;
			break;
		}
		else if (*scan > 32 && *scan < 127) // a valid character to process
		{
			break;
		}
		scan++;
	}

	return scan;
}

static bool endQuote(char c, char quote)
{
	bool ret = false;

	if (c == 34 && quote == 1)
	{
		ret = true;
	}
	else if (c == ')' && quote == 2)
	{
		ret = true;
	}

	return ret;
}

class KeyValue
{
	enum
	{
		MAXKEYVALUE = 256,
		MAXKEYVALUEBUFFER = 2048
	};
public:

	const char** getKeyValues(const char* userProperties, unsigned int& count)
	{
		const char** ret = 0;
		count = 0;

		if (userProperties)
		{
			strncpy(keyBuffer, userProperties, MAXKEYVALUEBUFFER);
			char* scan = keyBuffer;
			char quote;

			while (*scan) // while there are characters to process..
			{

				scan = skipSoft(scan, quote); // skip leading spaces if any, note if it begins with a quote

				if (*scan)  // if still data to process, copy the key pointer and search for the termination character.
				{
					keyValue[count++] = scan; // store the key pointer and advance the counter.
					while (*scan)   // search until we hit an 'equal' sign.
					{
						if (endQuote(*scan, quote)) // if we hit a quote mark, treat that as an end of string termination, but still look for the equal sign.
						{
							*scan = 0;
						}
						else if (*scan == '=') // if we hit the equal sign, terminate the key string by stomping on the equal and continue to value processing
						{
							*scan = 0; // stomp and EOS marker.
							scan++;    // advance to the next location.
							break;     // break out of the loop
						}
						scan++;
					}
					if (*scan) // if there is still data to process.
					{
						scan = skipSoft(scan, quote); // skip leading spaces, etc. note if there is a key.
						keyValue[count++] = scan;  // assign the value.
						while (*scan)
						{
							if (quote)  // if we began with a quote, then only a quote can terminate
							{
								if (endQuote(*scan, quote)) // if it is a quote, then terminate the string and continue to the next key
								{
									*scan = 0;
									scan++;
									scan++;
									break;
								}
							}
							else
							{
								// acceptable seperators are a space, a comma, or a semi-colon
								if (*scan == ';' || *scan == ',' || *scan == '=' || *scan == 32)
								{
									*scan = 0;
									scan++;
									break;
								}
							}
							scan++;
						}
					}
				}
			}
		}

		count = count / 2;

		if (count)
		{
			ret = (const char**)keyValue;
		}

		return ret;
	}


	const char* getKeyValue(const char* userProperties, const char* _key, bool caseSensitive = false)
	{
		const char* ret = 0;

		unsigned int count = 0;
		const char** slist = getKeyValues(userProperties, count);
		if (count)
		{
			for (unsigned int i = 0; i < count; ++i)
			{
				const char* key = slist[i * 2];
				const char* value = slist[i * 2 + 1];
				if (caseSensitive && ::strcmp(key, _key) == 0)
				{
					ret = value;
					break;
				}
#if PX_ANDROID || PX_PS4 || PX_LINUX_FAMILY || PX_SWITCH
				else if (strcasecmp(key, _key) == 0)
#else
				else if (::stricmp(key, _key) == 0)
#endif
				{
					ret = value;
					break;
				}
			}
		}
		return ret;
	}

private:
	char* keyValue[MAXKEYVALUE * 2];
	char  keyBuffer[MAXKEYVALUEBUFFER];
};


enum MeshVertexFlag
{
	MIVF_POSITION       = (1<<0),
	MIVF_NORMAL         = (1<<1),
	MIVF_COLOR          = (1<<2),
	MIVF_TEXEL1         = (1<<3),
	MIVF_TEXEL2         = (1<<4),
	MIVF_TEXEL3         = (1<<5),
	MIVF_TEXEL4         = (1<<6),
	MIVF_TANGENT        = (1<<7),
	MIVF_BINORMAL       = (1<<8),
	MIVF_BONE_WEIGHTING = (1<<9),
	MIVF_RADIUS         = (1<<10),
	MIVF_INTERP1        = (1<<11),
	MIVF_INTERP2        = (1<<12),
	MIVF_INTERP3        = (1<<13),
	MIVF_INTERP4        = (1<<14),
	MIVF_INTERP5        = (1<<15),
	MIVF_INTERP6        = (1<<16),
	MIVF_INTERP7        = (1<<17),
	MIVF_INTERP8        = (1<<18),
	MIVF_ALL = (MIVF_POSITION | MIVF_NORMAL | MIVF_COLOR | MIVF_TEXEL1 | MIVF_TEXEL2 | MIVF_TEXEL3 | MIVF_TEXEL4 | MIVF_TANGENT | MIVF_BINORMAL | MIVF_BONE_WEIGHTING )
};

class MeshVertex 
{
public:
	MeshVertex(void)
	{
		mPos[0] = mPos[1] = mPos[2] = 0;
		mNormal[0] = 0; mNormal[1] = 1; mNormal[2] = 0;
		mColor = 0xFFFFFFFF;
		mTexel1[0] = mTexel1[1] = 0;
		mTexel2[0] = mTexel2[1] = 0;
		mTexel3[0] = mTexel3[1] = 0;
		mTexel4[0] = mTexel4[1] = 0;

		mInterp1[0] = mInterp1[1] = mInterp1[2] = mInterp1[3] =0;
		mInterp2[0] = mInterp2[1] = mInterp2[2] = mInterp2[3] =0;
		mInterp3[0] = mInterp3[1] = mInterp3[2] = mInterp3[3] =0;
		mInterp4[0] = mInterp4[1] = mInterp4[2] = mInterp4[3] =0;
		mInterp5[0] = mInterp5[1] = mInterp5[2] = mInterp5[3] =0;
		mInterp6[0] = mInterp6[1] = mInterp6[2] = mInterp6[3] =0;
		mInterp7[0] = mInterp7[1] = mInterp7[2] = mInterp7[3] =0;
		mInterp8[0] = mInterp8[1] = mInterp8[2] = mInterp8[3] =0;

		mTangent[0] = mTangent[1] = mTangent[2] = 0;
		mBiNormal[0] = mBiNormal[1] = mBiNormal[2] = 0;
		mWeight[0] = 1; mWeight[1] = 0; mWeight[2] = 0; mWeight[3] = 0;
		mBone[0] = mBone[1] = mBone[2] = mBone[3] = 0;
		mRadius = 0; // use for cloth simulations
	}

	bool  operator==(const MeshVertex &v) const
	{
		bool ret = false;

		if ( memcmp(this,&v,sizeof(MeshVertex)) == 0 ) ret = true;

		return ret;
	}

	MiF32          mPos[3];
	MiF32          mNormal[3];
	MiU32            mColor;
	MiF32          mTexel1[2];
	MiF32          mTexel2[2];
	MiF32          mTexel3[2];
	MiF32          mTexel4[2];
	MiF32          mInterp1[4];
	MiF32          mInterp2[4];
	MiF32          mInterp3[4];
	MiF32          mInterp4[4];
	MiF32          mInterp5[4];
	MiF32          mInterp6[4];
	MiF32          mInterp7[4];
	MiF32          mInterp8[4];
	MiF32          mTangent[3];
	MiF32          mBiNormal[3];
	MiF32          mWeight[4];
	unsigned short mBone[4];
	MiF32          mRadius;
};

class MeshBone
{
public:
	MeshBone(void)
	{
		mParentIndex = -1;
		mName = "";
		Identity();
	}

	void Set(const char *name,MiI32 parentIndex,const MiF32 pos[3],const MiF32 rot[4],const MiF32 scale[3])
	{
		mName = name;
		mParentIndex = parentIndex;
		mPosition[0] = pos[0];
		mPosition[1] = pos[1];
		mPosition[2] = pos[2];
		mOrientation[0] = rot[0];
		mOrientation[1] = rot[1];
		mOrientation[2] = rot[2];
		mOrientation[3] = rot[3];
		mScale[0] = scale[0];
		mScale[1] = scale[1];
		mScale[2] = scale[2];
	}

	void Identity(void)
	{
		mPosition[0] = 0;
		mPosition[1] = 0;
		mPosition[2] = 0;

		mOrientation[0] = 0;
		mOrientation[1] = 0;
		mOrientation[2] = 0;
		mOrientation[3] = 1;

		mScale[0] = 1;
		mScale[1] = 1;
		mScale[2] = 1;

	}

	void SetName(const char *name)
	{
		mName = name;
	}

	const char * GetName(void) const { return mName; };

	MiI32 GetParentIndex(void) const { return mParentIndex; };

	const MiF32 * GetPosition(void) const { return mPosition; };
	const MiF32 * GetOrientation(void) const { return mOrientation; };
	const MiF32 * GetScale(void) const { return mScale; };

	void getAngleAxis(MiF32 &angle,MiF32 *axis) const
	{
		fmi_getAngleAxis(angle,axis,mOrientation);
	}

	void setOrientationFromAxisAngle(const MiF32 axis[3],MiF32 angle)
	{
		fmi_setOrientationFromAxisAngle(axis,angle,mOrientation);
	}

	const char   *mName;
	MiI32           mParentIndex;          // array index of parent bone
	MiF32         mPosition[3];
	MiF32         mOrientation[4];
	MiF32         mScale[3];
};

class MeshEntry
{
public:
	MeshEntry(void)
	{
		mName = "";
		mBone = 0;
	}
	const char *mName;
	MiI32         mBone;         // bone this mesh is associcated
};

class MeshSkeleton
{
public:
	MeshSkeleton(void)
	{
		mName = "";
		mBoneCount = 0;
		mBones = 0;
	}

	void SetName(const char *name)
	{
		mName = name;
	}

	void SetBones(MiI32 bcount,MeshBone *bones) // memory ownership changes hands here!!!!!!!!!!
	{
		mBoneCount = bcount;
		mBones     = bones;
	}

	MiI32 GetBoneCount(void) const { return mBoneCount; };

	const MeshBone& GetBone(MiI32 index) const { return mBones[index]; };

	MeshBone * GetBonePtr(MiI32 index) const { return &mBones[index]; };

	void SetBone(MiI32 index,const MeshBone &b) { mBones[index] = b; };

	const char * GetName(void) const { return mName; };

	const char     *mName;
	MiI32             mBoneCount;
	MeshBone       *mBones;
};


class MeshAnimPose
{
public:
	MeshAnimPose(void)
	{
		mPos[0] = 0;
		mPos[1] = 0;
		mPos[2] = 0;
		mQuat[0] = 0;
		mQuat[1] = 0;
		mQuat[2] = 0;
		mQuat[3] = 0;
		mScale[0] = 1;
		mScale[1] = 1;
		mScale[2] = 1;
	}

	void SetPose(const MiF32 *pos,const MiF32 *quat,const MiF32 *scale)
	{
		mPos[0] = pos[0];
		mPos[1] = pos[1];
		mPos[2] = pos[2];
		mQuat[0] = quat[0];
		mQuat[1] = quat[1];
		mQuat[2] = quat[2];
		mQuat[3] = quat[3];
		mScale[0] = scale[0];
		mScale[1] = scale[1];
		mScale[2] = scale[2];
	};

	void Sample(MiF32 *pos,MiF32 *quat,MiF32 *scale) const
	{
		pos[0] = mPos[0];
		pos[1] = mPos[1];
		pos[2] = mPos[2];
		quat[0] = mQuat[0];
		quat[1] = mQuat[1];
		quat[2] = mQuat[2];
		quat[3] = mQuat[3];
		scale[0] = mScale[0];
		scale[1] = mScale[1];
		scale[2] = mScale[2];
	}

	void getAngleAxis(MiF32 &angle,MiF32 *axis) const
	{
		fmi_getAngleAxis(angle,axis,mQuat);
	}

	MiF32 mPos[3];
	MiF32 mQuat[4];
	MiF32 mScale[3];
};

class MeshAnimTrack
{
public:
	MeshAnimTrack(void)
	{
		mName = "";
		mFrameCount = 0;
		mDuration = 0;
		mDtime = 0;
		mPose = 0;
	}

	void SetName(const char *name)
	{
		mName = name;
	}

	void SetPose(MiI32 frame,const MiF32 *pos,const MiF32 *quat,const MiF32 *scale)
	{
		if ( frame >= 0 && frame < mFrameCount )
			mPose[frame].SetPose(pos,quat,scale);
	}

	const char * GetName(void) const { return mName; };

	void SampleAnimation(MiI32 frame,MiF32 *pos,MiF32 *quat,MiF32 *scale) const
	{
		mPose[frame].Sample(pos,quat,scale);
	}

	MiI32 GetFrameCount(void) const { return mFrameCount; };

	MeshAnimPose * GetPose(MiI32 index) { return &mPose[index]; };

	const char *mName;
	MiI32       mFrameCount;
	MiF32     mDuration;
	MiF32     mDtime;
	MeshAnimPose *mPose;
};

class MeshAnimation
{
public:
	MeshAnimation(void)
	{
		mName = "";
		mTrackCount = 0;
		mFrameCount = 0;
		mDuration = 0;
		mDtime = 0;
		mTracks = 0;
	}


	void SetName(const char *name)
	{
		mName = name;
	}

	void SetTrackName(MiI32 track,const char *name)
	{
		mTracks[track]->SetName(name);
	}

	void SetTrackPose(MiI32 track,MiI32 frame,const MiF32 *pos,const MiF32 *quat,const MiF32 *scale)
	{
		mTracks[track]->SetPose(frame,pos,quat,scale);
	}

	const char * GetName(void) const { return mName; };

	const MeshAnimTrack * LocateTrack(const char *name) const
	{
		const MeshAnimTrack *ret = 0;
		for (MiI32 i=0; i<mTrackCount; i++)
		{
			const MeshAnimTrack *t = mTracks[i];
			if (::strcmp(t->GetName(),name) == 0)
			{
				ret = t;
				break;
			}
		}
		return ret;
	}

	MiI32 GetFrameIndex(MiF32 t) const
	{
		t = fmodf( t, mDuration );
		MiI32 index = MiI32(t / mDtime);
		return index;
	}

	MiI32 GetTrackCount(void) const { return mTrackCount; };
	MiF32 GetDuration(void) const { return mDuration; };

	MeshAnimTrack * GetTrack(MiI32 index)
	{
		MeshAnimTrack *ret = 0;
		if ( index >= 0 && index < mTrackCount )
		{
			ret = mTracks[index];
		}
		return ret;
	};

	MiI32 GetFrameCount(void) const { return mFrameCount; };
	MiF32 GetDtime(void) const { return mDtime; };

	const char*		mName;
	MiI32			mTrackCount;
	MiI32			mFrameCount;
	MiF32			mDuration;
	MiF32			mDtime;
	MeshAnimTrack**	mTracks;
};


class MeshMaterial
{
public:
	MeshMaterial(void)
	{
		mName = 0;
		mMetaData = 0;
	}
	const char* mName;
	const char* mMetaData;
};

class MeshAABB
{
public:
	MeshAABB(void)
	{
		mMin[0] = FLT_MAX;
		mMin[1] = FLT_MAX;
		mMin[2] = FLT_MAX;
		mMax[0] = FLT_MIN;
		mMax[1] = FLT_MIN;
		mMax[2] = FLT_MIN;
	}

	void include(const MiF32 pos[3])
	{
		if ( pos[0] < mMin[0] ) mMin[0] = pos[0];
		if ( pos[1] < mMin[1] ) mMin[1] = pos[1];
		if ( pos[2] < mMin[2] ) mMin[2] = pos[2];
		if ( pos[0] > mMax[0] ) mMax[0] = pos[0];
		if ( pos[1] > mMax[1] ) mMax[1] = pos[1];
		if ( pos[2] > mMax[2] ) mMax[2] = pos[2];
	}
	MiF32 mMin[3];
	MiF32 mMax[3];
};

class SubMesh
{
public:
	SubMesh(void)
	{
		mMaterialName	= 0;
		mMaterial		= 0;
		mVertexFlags	= 0;
		mTriCount		= 0;
		mIndices		= 0;
	}

	const char*			mMaterialName;
	MeshMaterial*		mMaterial;
	MeshAABB			mAABB;
	MiU32				mVertexFlags; // defines which vertex components are active.
	MiU32				mTriCount;    // number of triangles.
	MiU32*				mIndices;     // indexed triange list
};

class Mesh
{
public:
	Mesh(void)
	{
		mName			= 0;
		mSkeletonName	= 0;
		mSkeleton		= 0;
		mSubMeshCount	= 0;
		mSubMeshes		= 0;
		mVertexFlags	= 0;
		mVertexCount	= 0;
		mVertices		= 0;
	}
	const char*			mName;
	const char*			mSkeletonName;
	MeshSkeleton*		mSkeleton; // the skeleton used by this mesh system.
	MeshAABB			mAABB;
	MiU32				mSubMeshCount;
	SubMesh**			mSubMeshes;

	MiU32				mVertexFlags;  // combined vertex usage flags for all sub-meshes
	MiU32				mVertexCount;
	MeshVertex*			mVertices;

};

class MeshRawTexture
{
public:
	MeshRawTexture(void)
	{
		mName		= 0;
		mData		= 0;
		mWidth		= 0;
		mHeight		= 0;
		mBPP		= 0;
	}
	const char*		mName;
	MiU8*			mData;
	MiU32			mWidth;
	MiU32			mHeight;
	MiU32			mBPP;
};

class MeshInstance
{
public:
	MeshInstance(void)
	{
		mMeshName = 0;
		mMesh     = 0;
		mPosition[0] = mPosition[1] = mPosition[2] = 0;
		mRotation[0] = mRotation[1] = mRotation[2] = mRotation[3] = 0;
		mScale[0] = mScale[1] = mScale[2] = 0;
	}
	const char  *mMeshName;
	Mesh        *mMesh;
	MiF32        mPosition[3];
	MiF32        mRotation[4]; //quaternion XYZW
	MiF32        mScale[3];
};

class MeshUserData
{
public:
	MeshUserData(void)
	{
		mUserKey = 0;
		mUserValue = 0;
	}
	const char *mUserKey;
	const char *mUserValue;
};

class MeshUserBinaryData
{
public:
	MeshUserBinaryData(void)
	{
		mName     = 0;
		mUserData = 0;
		mUserLen  = 0;
	}
	const char    *mName;
	MiU32   mUserLen;
	MiU8 *mUserData;
};

class MeshTetra
{
public:
	MeshTetra(void)
	{
		mTetraName  = 0;
		mMeshName   = 0;  // mesh the tetraheadral mesh is associated with.
		mMesh       = 0;
		mTetraCount = 0;
		mTetraData  = 0;
	}

	const char*		mTetraName;
	const char*		mMeshName;
	MeshAABB		mAABB;
	Mesh*			mMesh;
	MiU32			mTetraCount; // number of tetrahedrons
	MiF32*			mTetraData;
};

#define MESH_SYSTEM_VERSION 1 // version number of this data structure, used for binary serialization

enum MeshCollisionType
{
	MCT_BOX,
	MCT_SPHERE,
	MCT_CAPSULE,
	MCT_CONVEX,
	MCT_LAST
};

class MeshCollision
{
public:
	MeshCollision(void)
	{
		mType = MCT_LAST;
		mName = NULL;
		mLocalPosition[0] = 0;
		mLocalPosition[1] = 0;
		mLocalPosition[2] = 0;
		mLocalOrientation[0] = 0;
		mLocalOrientation[1] = 0;
		mLocalOrientation[2] = 0;
		mLocalOrientation[3] = 1;
	}

	MeshCollisionType getType(void) const { return mType; };

	MeshCollisionType		mType;
	const char*				mName;  // the bone this collision geometry is associated with.
	MiF32					mLocalPosition[3];
	MiF32					mLocalOrientation[4];
};

class MeshCollisionBox : public MeshCollision 
{
public:
	MeshCollisionBox(void)
	{
		mType = MCT_BOX;
		mSides[0] = mSides[1] = mSides[2] = 1;
	}
	MiF32 mSides[3];
};

class MeshCollisionSphere : public MeshCollision
{
public:
	MeshCollisionSphere(void)
	{
		mType = MCT_SPHERE;
		mRadius = 1;
	}
	MiF32 mRadius;
};

class MeshCollisionCapsule : public MeshCollision
{
public:
	MeshCollisionCapsule(void)
	{
		mType = MCT_CAPSULE;
		mRadius = 1;
		mHeight = 1;
	}
	MiF32  mRadius;
	MiF32  mHeight;
};

class MeshConvex
{
public:
	MeshConvex(void)
	{
		mVertexCount	= 0;
		mVertices		= 0;
		mTriCount		= 0;
		mIndices		= 0;
	}

	MiU32				mVertexCount;
	MiF32*				mVertices;
	MiU32				mTriCount;
	MiU32*				mIndices;
};

class MeshCollisionConvex : public MeshCollision, public MeshConvex
{
public:
	MeshCollisionConvex(void)
	{
		mType = MCT_CONVEX;
	}


};

class MeshCollisionRepresentation
{
public:
	MeshCollisionRepresentation(void)
	{
		mName = NULL;
		mInfo = NULL;
		mCollisionCount = 0;
		mCollisionGeometry = NULL;
		mPosition[0] = 0;
		mPosition[1] = 0;
		mPosition[2] = 0;
		mOrientation[0] = 0;
		mOrientation[1] = 0;
		mOrientation[2] = 0;
		mOrientation[3] = 1;
		mSolverCount = 4;
		mAwake = true;
	}

	const char*		mName;
	const char*		mInfo;
	MiU32			mCollisionCount;
	MeshCollision**	mCollisionGeometry;
	MiF32			mPosition[3];
	MiF32			mOrientation[4];
	MiU32			mSolverCount;
	bool			mAwake;
};

class MeshSimpleJoint
{
public:
	MeshSimpleJoint(void)
	{
		mName		= "";
		mParent		= "";
		mTwistLow	= 0;
		mTwistHigh	= 0;
		mSwing1		= 0;
		mSwing2		= 0;
		mOffset[0]	= 0;
		mOffset[1]	= 0;
		mOffset[2]	= 0;
		mXaxis[0]	= 1;
		mXaxis[1]	= 0;
		mXaxis[2]	= 0;
		mZaxis[0]	= 0;
		mZaxis[1]	= 0;
		mZaxis[2]	= 1;
	}

	const char*		mName;            // name of joint; defines which mesh collision representation it is connected to.
	const char*		mParent;        // name of parent, defines which mesh collision representation this joint connects to.
	MiF32			mOffset[3];        // The offset of the joint from the root bone.
	MiF32			mXaxis[3];        // The axis of the joint from the root bone
	MiF32			mZaxis[3];        // The normal of the joint relative to the root bone.
	MiF32			mTwistLow;        // The low twist limit in radians
	MiF32			mTwistHigh;        // The high twist limit in radians
	MiF32			mSwing1;        // The swing1 limit
	MiF32			mSwing2;        // The swing2 limit
};

class MeshPairCollision
{
public:
	const char*		mBody1;
	const char*		mBody2;
};

class MeshSystem
{
public:
	MeshSystem(void)
	{
		mAssetName				= 0;
		mAssetInfo				= 0;
		mTextureCount			= 0;
		mTextures				= 0;
		mSkeletonCount			= 0;
		mSkeletons				= 0;
		mAnimationCount			= 0;
		mAnimations				= 0;
		mMaterialCount			= 0;
		mMaterials				= 0;
		mMeshCount				= 0;
		mMeshes					= 0;
		mMeshInstanceCount		= 0;
		mMeshInstances			= 0;
		mUserDataCount			= 0;
		mUserData				= 0;
		mUserBinaryDataCount	= 0;
		mUserBinaryData			= 0;
		mTetraMeshCount			= 0;
		mTetraMeshes			= 0;
		mMeshSystemVersion		= MESH_SYSTEM_VERSION;
		mAssetVersion			= 0;
		mMeshCollisionCount		= 0;
		mMeshCollisionRepresentations = 0;
		mMeshJointCount			= 0;
		mMeshJoints				= NULL;
		mMeshPairCollisionCount	= 0;
		mMeshPairCollisions		= NULL;

		mPlane[0]				= 1;
		mPlane[1]				= 0;
		mPlane[2]				= 0;
		mPlane[3]				= 0;
	}


	const char*					mAssetName;
	const char*					mAssetInfo;
	MiI32						mMeshSystemVersion;
	MiI32						mAssetVersion;
	MeshAABB					mAABB;
	MiU32						mTextureCount;          // Are textures necessary? [rgd].
	MeshRawTexture**			mTextures;              // Texture storage in mesh data is rare, and the name is simply an attribute of the material

	MiU32						mTetraMeshCount;        // number of tetrahedral meshes
	MeshTetra**					mTetraMeshes;           // tetraheadral meshes

	MiU32						mSkeletonCount;         // number of skeletons
	MeshSkeleton**				mSkeletons;             // the skeletons.

	MiU32						mAnimationCount;
	MeshAnimation**				mAnimations;

	MiU32						mMaterialCount;         // Materials are owned by this list, merely referenced later.
	MeshMaterial*				mMaterials;

	MiU32						mUserDataCount;
	MeshUserData**				mUserData;

	MiU32						mUserBinaryDataCount;
	MeshUserBinaryData**		mUserBinaryData;

	MiU32						mMeshCount;
	Mesh**						mMeshes;

	MiU32						mMeshInstanceCount;
	MeshInstance*				mMeshInstances;

	MiU32						mMeshCollisionCount;
	MeshCollisionRepresentation**	mMeshCollisionRepresentations;

	MiU32						mMeshJointCount;
	MeshSimpleJoint*			mMeshJoints;

	MiU32						mMeshPairCollisionCount;
	MeshPairCollision*			mMeshPairCollisions;

	MiF32						mPlane[4];

};


class MeshImportInterface
{
public:

	virtual void		importMaterial(const char* matName, const char* metaData) = 0;        // one material
	virtual void		importUserData(const char* userKey, const char* userValue) = 0;       // carry along raw user data as ASCII strings only..
	virtual void		importUserBinaryData(const char* name, MiU32 len, const MiU8* data) = 0;
	virtual void		importTetraMesh(const char* tetraName, const char* meshName, MiU32 tcount, const MiF32* tetraData) = 0;
	virtual void		importMeshPairCollision(MeshPairCollision& mpc) = 0;

	virtual void		importAssetName(const char* assetName, const char* info) = 0;         // name of the overall asset.
	virtual void		importMesh(const char* meshName, const char* skeletonName) = 0;       // name of a mesh and the skeleton it refers to.

	virtual void		importTriangle(const char *meshName,
		const char*	materialName,
		MiU32 vertexFlags,
		const MeshVertex &v1,
		const MeshVertex &v2,
		const MeshVertex &v3) = 0;

	virtual void		importIndexedTriangleList(const char *meshName,
		const char *materialName,
		MiU32 vertexFlags,
		MiU32 vcount,
		const MeshVertex *vertices,
		MiU32 tcount,
		const MiU32 *indices) = 0;

	virtual void		importAnimation(const MeshAnimation& animation) = 0;
	virtual void		importSkeleton(const MeshSkeleton& skeleton) = 0;
	virtual void		importRawTexture(const char* textureName, const MiU8* pixels, MiU32 wid, MiU32 hit) = 0;
	virtual void		importMeshInstance(const char* meshName, const MiF32 pos[3], const MiF32 rotation[4], const MiF32 scale[3]) = 0;

	virtual void		importCollisionRepresentation(const MeshCollisionRepresentation& mr) = 0; // the name of a new collision representation.

	virtual void		importSimpleJoint(const MeshSimpleJoint& joint) = 0;

	virtual MiI32		getSerializeFrame(void) = 0;

	virtual void		importPlane(const MiF32* p) = 0;
};

// allows the application to load external resources.
// For example, when loading wavefront OBJ files, the materials are saved in a seperate file.
// This interface allows the application to load the resource.
class MeshImportApplicationResource
{
public:
	virtual void*	getApplicationResource(const char *base_name,const char *resource_name,MiU32 &len) = 0;
	virtual void	releaseApplicationResource(void *mem) = 0;
};



class MeshImporter
{
public:
	virtual MiI32			getExtensionCount(void) { return 1; }; // most importers support just one file name extension.
	virtual const char*		getExtension(MiI32 index = 0) = 0; // report the default file name extension for this mesh type.
	virtual const char*		getDescription(MiI32 index = 0) = 0; // report the ascii description of the import type.

	virtual bool			importMesh(const char* meshName,const void* data, MiU32 dlen, MeshImportInterface* callback, const char* options, MeshImportApplicationResource* appResource) = 0;

	virtual const void*		saveMeshSystem(MeshSystem* /*ms*/,MiU32& /*dlen*/, bool /*binary*/) 
	{
		return NULL;
	}
	virtual void releaseSavedMeshSystem(const void* /*mem*/) 
	{

	}
};

enum MeshSerializeFormat
{
	MSF_EZMESH,		// save it back out into ez-mesh, lossless XML format.
	MSF_OGRE3D,		// save it back out into the Ogre3d XML format.
	MSF_WAVEFRONT,	// save as wavefront OBJ
	MSF_PSK,		// save it back out as a PSK format file.
	MSF_FBX,		// FBX import is supported by FBX output is not yet.
	MSF_MSH,        // custom binary .MSH file format used by the Planetside game engine
	MSF_SCARAB_3D,	// the scarab binary 3d file format.
	MSF_SCARAB_TRI,
	MSF_GR2,        // granny 2 file format.
	MSF_EZB,        // A binary representation of an ez-mesh; used for faster loading.  Should be endian aware/safe.  Avoids string parsing and tens of thousands of ascii to float conversions.
	MSF_POVRAY,		// export in POVRay format (suitable for raytracing)
	MSF_COLLADA, 
	MSF_BIN,        // legacy Rocket binary format.
	MSF_NXU,        // legacy PhysX 2.8.x NxuStream file format
	MSF_PSC,        // reads Legacy Rocket PSC files; can output to APEX Dynamic System module format
	MSF_DYNAMIC_SYSTEM, // APX file compatible with the dynamic system module for APEX
	MSF_APX,
	MSF_LAST
};


class MeshBoneInstance
{
public:
	MeshBoneInstance(void)
	{
		mBoneName = "";
	}

	void composeInverse(void)
	{
		fmi_inverseTransform(mTransform,mInverseTransform);
	}

	const char*		mBoneName;                     // the name of the bone
	MiI32			mParentIndex;                  // the parent index
	MiF32			mLocalTransform[16];
	MiF32			mTransform[16];                // the transform in world space
	MiF32			mAnimTransform[16];            // the sampled animation transform, multiplied times the inverse root transform.
	MiF32			mCompositeAnimTransform[16];   // the composite transform
	MiF32			mInverseTransform[16];         // the inverse transform
};

class MeshSkeletonInstance
{
public:
	MeshSkeletonInstance(void)
	{
		mName			= "";
		mBoneCount		= 0;
		mBones			= 0;
	}

	const char*			mName;
	MiI32				mBoneCount;
	MeshBoneInstance*	mBones;
};

class MeshSerialize
{
public:
	MeshSerialize(MeshSerializeFormat format)
	{
		mFormat			= format;
		mBaseData		= 0;
		mBaseLen		= 0;
		mExtendedData	= 0;
		mExtendedLen	= 0;
		mSaveFileName	= 0;
		fmi_identity(mExportTransform);
	}
	MeshSerializeFormat	mFormat;
	MiU8*				mBaseData;
	MiU32				mBaseLen;
	MiU8*				mExtendedData;
	MiU32				mExtendedLen;
	const char*			mSaveFileName; // need to know the name of the save file name for OBJ and Ogre3d format.
	MiF32				mExportTransform[16]; // matrix transform on export
};


class MeshSystemContainer;

class VertexIndex
{
public:
	virtual MiU32			getIndex(const MiF32 pos[3], bool& newPos) = 0;  // get welded index for this MiF32 vector[3]
	virtual const MiF32*	getVertices(void) const = 0;
	virtual const MiF32*	getVertex(MiU32 index) const = 0;
	virtual MiU32			getVcount(void) const = 0;
};

class MeshImport
{
public:
	virtual void			addImporter(MeshImporter* importer) = 0; // add an additional importer

	virtual bool			importMesh(const char* meshName, const void* data, MiU32 dlen, MeshImportInterface* callback, const char* options) = 0;

	virtual MeshSystemContainer*	createMeshSystemContainer(void) = 0;

	virtual MeshSystemContainer*	createMeshSystemContainer(const char* meshName,
		const void* data,
		MiU32 dlen,
		const char* options) = 0; // imports and converts to a single MeshSystem data structure

	virtual void			releaseMeshSystemContainer(MeshSystemContainer* mesh) = 0;

	virtual MeshSystem*		getMeshSystem(MeshSystemContainer* mb) = 0;

	virtual bool			serializeMeshSystem(MeshSystem* mesh, MeshSerialize& data) = 0;
	virtual void			releaseSerializeMemory(MeshSerialize& data) = 0;


	virtual MiI32			getImporterCount(void) = 0;
	virtual MeshImporter*	getImporter(MiI32 index) = 0;

	virtual MeshImporter*	locateMeshImporter(const char* fname) = 0; // based on this file name, find a matching mesh importer.

	virtual const char*		getFileRequestDialogString(void) = 0;

	virtual void			setMeshImportApplicationResource(MeshImportApplicationResource* resource) = 0;

	// convenience helper functions.
	virtual MeshSkeletonInstance*	createMeshSkeletonInstance(const MeshSkeleton& sk) = 0;
	virtual bool					sampleAnimationTrack(MiI32 trackIndex, const MeshSystem* msystem,MeshSkeletonInstance* skeleton) = 0;
	virtual void					releaseMeshSkeletonInstance(MeshSkeletonInstance* sk) = 0;

	// apply bone weighting transforms to this vertex buffer.
	virtual void transformVertices(MiU32 vcount,
		const MeshVertex* source_vertices,
		MeshVertex* dest_vertices,
		MeshSkeletonInstance* skeleton) = 0;

	virtual MeshImportInterface*	getMeshImportInterface(MeshSystemContainer *msc) = 0;

	virtual void gather(MeshSystemContainer* msc) = 0;

	virtual void scale(MeshSystemContainer* msc, MiF32 scale) = 0;
	virtual void rotate(MeshSystemContainer* msc, MiF32 rotX, MiF32 rotY, MiF32 rotZ) = 0; // rotate mesh system using these euler angles expressed as degrees.

	virtual VertexIndex*			createVertexIndex(MiF32 granularity) = 0;  // create an indexed vertext system for floats
	virtual void					releaseVertexIndex(VertexIndex* vindex) = 0;

protected:
	virtual ~MeshImport(void) {};
};


#define MESHIMPORT_VERSION 11  // version 0.01  increase this version number whenever an interface change occurs.


extern MeshImport* gMeshImport; // This is an optional global variable that can be used by the application.  If the application uses it, it should define it somewhere in its codespace.

MeshImport* loadMeshImporters(const char *directory); // loads the mesh import library (dll) and all available importers from the same directory.

}; // End of namespace for physx

#ifdef PLUGINS_EMBEDDED

extern "C"
{
    mimp::MeshImport   * getInterfaceMeshImport(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportAPX(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportEzm(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportFBX(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportObj(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportOgre(mimp::MiI32 version_number);
    mimp::MeshImporter * getInterfaceMeshImportPSK(mimp::MiI32 version_number);
};
#endif

#pragma warning(pop)

#endif
