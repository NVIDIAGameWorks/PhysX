/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __dgMatrix__
#define __dgMatrix__


#include "dgVector.h"
#include "dgPlane.h"
#include <math.h>

class dgMatrix;
class dgQuaternion;

const dgMatrix& dgGetZeroMatrix ();
const dgMatrix& dgGetIdentityMatrix();


class dgMatrix
{
	public:

	dgMatrix ();
	dgMatrix (const dgVector &front, const dgVector &up, const dgVector &right, const dgVector &posit);
	dgMatrix (const dgQuaternion &rotation, const dgVector &position);

	// create a orthonormal normal vector basis
	dgMatrix (const dgVector &front);


	

	dgVector& operator[] (int32_t i);
	const dgVector& operator[] (int32_t i) const;

	dgMatrix Inverse () const;
	dgMatrix Transpose () const;
	dgMatrix Transpose4X4 () const;
	dgMatrix Symetric3by3Inverse () const;
	dgVector RotateVector (const dgVector &v) const;
	dgVector UnrotateVector (const dgVector &v) const;
	dgVector TransformVector (const dgVector &v) const;
	dgVector UntransformVector (const dgVector &v) const;
	dgPlane TransformPlane (const dgPlane &localPlane) const;
	dgPlane UntransformPlane (const dgPlane &globalPlane) const;
	void TransformBBox (const dgVector& p0local, const dgVector& p1local, dgVector& p0, dgVector& p1) const; 

	dgVector CalcPitchYawRoll () const;
	void TransformTriplex (float* const dst, int32_t dstStrideInBytes,
						   const float* const src, int32_t srcStrideInBytes, int32_t count) const;

	void TransformTriplex (double* const dst, int32_t dstStrideInBytes,
						   const double* const src, int32_t srcStrideInBytes, int32_t count) const;

	void TransformTriplex (double* const dst, int32_t dstStrideInBytes,
						   const float* const src, int32_t srcStrideInBytes, int32_t count) const;


	dgMatrix operator* (const dgMatrix &B) const;
	

	// this function can not be a member of dgMatrix, because
	// dgMatrix a define to handle only orthogonal matrices
	// and this function take a parameter to a symmetric matrix
	void EigenVectors (dgVector &eigenValues, const dgMatrix& initialGuess = dgGetIdentityMatrix());
	void EigenVectors (const dgMatrix& initialGuess = dgGetIdentityMatrix());


	dgVector m_front;
	dgVector m_up;
	dgVector m_right;
	dgVector m_posit;
};





HACD_INLINE dgMatrix::dgMatrix ()
{
}

HACD_INLINE dgMatrix::dgMatrix (const dgVector &front, const dgVector &up, const dgVector &right, const dgVector &posit)
	:m_front (front), m_up(up), m_right(right), m_posit(posit)
{
}

HACD_INLINE dgMatrix::dgMatrix (const dgVector& front)
{
	m_front = front; 
	if (dgAbsf (front.m_z) > float (0.577f)) {
		m_right = front * dgVector (-front.m_y, front.m_z, float(0.0f), float(0.0f));
	} else {
	  	m_right = front * dgVector (-front.m_y, front.m_x, float(0.0f), float(0.0f));
	}
  	m_right = m_right.Scale (dgRsqrt (m_right % m_right));
  	m_up = m_right * m_front;

	m_front.m_w = float(0.0f);
	m_up.m_w = float(0.0f);
	m_right.m_w = float(0.0f);
	m_posit = dgVector (float(0.0f), float(0.0f), float(0.0f), float(1.0f));

	HACD_ASSERT ((dgAbsf (m_front % m_front) - float(1.0f)) < float(1.0e-5f)); 
	HACD_ASSERT ((dgAbsf (m_up % m_up) - float(1.0f)) < float(1.0e-5f)); 
	HACD_ASSERT ((dgAbsf (m_right % m_right) - float(1.0f)) < float(1.0e-5f)); 
	HACD_ASSERT ((dgAbsf (m_right % (m_front * m_up)) - float(1.0f)) < float(1.0e-5f)); 
}


HACD_INLINE dgVector& dgMatrix::operator[] (int32_t  i)
{
	HACD_ASSERT (i < 4);
	HACD_ASSERT (i >= 0);
	return (&m_front)[i];
}

HACD_INLINE const dgVector& dgMatrix::operator[] (int32_t  i) const
{
	HACD_ASSERT (i < 4);
	HACD_ASSERT (i >= 0);
	return (&m_front)[i];
}


HACD_INLINE dgMatrix dgMatrix::Transpose () const
{
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, float(0.0f)),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, float(0.0f)),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, float(0.0f)),
					 dgVector (float(0.0f), float(0.0f), float(0.0f), float(1.0f)));
}

HACD_INLINE dgMatrix dgMatrix::Transpose4X4 () const
{
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, m_posit.m_x),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, m_posit.m_y),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, m_posit.m_z),
					 dgVector (m_front.m_w, m_up.m_w, m_right.m_w, m_posit.m_w));
							
}

HACD_INLINE dgVector dgMatrix::RotateVector (const dgVector &v) const
{
	return dgVector (v.m_x * m_front.m_x + v.m_y * m_up.m_x + v.m_z * m_right.m_x,
					 v.m_x * m_front.m_y + v.m_y * m_up.m_y + v.m_z * m_right.m_y,
					 v.m_x * m_front.m_z + v.m_y * m_up.m_z + v.m_z * m_right.m_z, v.m_w);
}


HACD_INLINE dgVector dgMatrix::UnrotateVector (const dgVector &v) const
{
	return dgVector (v % m_front, v % m_up, v % m_right, v.m_w);
}


HACD_INLINE dgVector dgMatrix::TransformVector (const dgVector &v) const
{
//	return m_posit + RotateVector(v);
	return dgVector (v.m_x * m_front.m_x + v.m_y * m_up.m_x + v.m_z * m_right.m_x + m_posit.m_x,
					 v.m_x * m_front.m_y + v.m_y * m_up.m_y + v.m_z * m_right.m_y + m_posit.m_y,
					 v.m_x * m_front.m_z + v.m_y * m_up.m_z + v.m_z * m_right.m_z + m_posit.m_z, v.m_w);

}

HACD_INLINE dgVector dgMatrix::UntransformVector (const dgVector &v) const
{
	return UnrotateVector(v - m_posit);
}

HACD_INLINE dgPlane dgMatrix::TransformPlane (const dgPlane &localPlane) const
{
	return dgPlane (RotateVector (localPlane), localPlane.m_w - (localPlane % UnrotateVector (m_posit)));  
}

HACD_INLINE dgPlane dgMatrix::UntransformPlane (const dgPlane &globalPlane) const
{
	return dgPlane (UnrotateVector (globalPlane), globalPlane.Evalue(m_posit));
}

HACD_INLINE void dgMatrix::EigenVectors (const dgMatrix& initialGuess)
{
	dgVector eigenValues;
	EigenVectors (eigenValues, initialGuess);
}


HACD_INLINE dgMatrix dgPitchMatrix(float ang)
{
	float cosAng;
	float sinAng;
	sinAng = dgSin (ang);
	cosAng = dgCos (ang);
	return dgMatrix (dgVector (float(1.0f),  float(0.0f), float(0.0f), float(0.0f)), 
					 dgVector (float(0.0f),  cosAng,          sinAng,          float(0.0f)),
					 dgVector (float(0.0f), -sinAng,          cosAng,          float(0.0f)), 
					 dgVector (float(0.0f),  float(0.0f), float(0.0f), float(1.0f))); 

}

HACD_INLINE dgMatrix dgYawMatrix(float ang)
{
	float cosAng;
	float sinAng;
	sinAng = dgSin (ang);
	cosAng = dgCos (ang);
	return dgMatrix (dgVector (cosAng,          float(0.0f), -sinAng,          float(0.0f)), 
					 dgVector (float(0.0f), float(1.0f),  float(0.0f), float(0.0f)), 
					 dgVector (sinAng,          float(0.0f),  cosAng,          float(0.0f)), 
					 dgVector (float(0.0f), float(0.0f),  float(0.0f), float(1.0f))); 
}

HACD_INLINE dgMatrix dgRollMatrix(float ang)
{
	float cosAng;
	float sinAng;
	sinAng = dgSin (ang);
	cosAng = dgCos (ang);
	return dgMatrix (dgVector ( cosAng,          sinAng,          float(0.0f), float(0.0f)), 
					 dgVector (-sinAng,          cosAng,          float(0.0f), float(0.0f)),
					 dgVector ( float(0.0f), float(0.0f), float(1.0f), float(0.0f)), 
					 dgVector ( float(0.0f), float(0.0f), float(0.0f), float(1.0f))); 
}																		 


HACD_INLINE dgMatrix dgMatrix::Inverse () const
{
	return dgMatrix (dgVector (m_front.m_x, m_up.m_x, m_right.m_x, float(0.0f)),
					 dgVector (m_front.m_y, m_up.m_y, m_right.m_y, float(0.0f)),
					 dgVector (m_front.m_z, m_up.m_z, m_right.m_z, float(0.0f)),
					 dgVector (- (m_posit % m_front), - (m_posit % m_up), - (m_posit % m_right), float(1.0f)));
}

#endif

