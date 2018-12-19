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

#include "dgTypes.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"


static dgMatrix zeroMatrix (dgVector (float(0.0f), float(0.0f), float(0.0f), float(0.0f)),
							dgVector (float(0.0f), float(0.0f), float(0.0f), float(0.0f)),
							dgVector (float(0.0f), float(0.0f), float(0.0f), float(0.0f)),
							dgVector (float(0.0f), float(0.0f), float(0.0f), float(0.0f)));

static dgMatrix identityMatrix (dgVector (float(1.0f), float(0.0f), float(0.0f), float(0.0f)),
								dgVector (float(0.0f), float(1.0f), float(0.0f), float(0.0f)),
								dgVector (float(0.0f), float(0.0f), float(1.0f), float(0.0f)),
								dgVector (float(0.0f), float(0.0f), float(0.0f), float(1.0f)));

const dgMatrix& dgGetIdentityMatrix()
{
	return identityMatrix;
}

const dgMatrix& dgGetZeroMatrix ()
{
	return zeroMatrix;
}


dgMatrix::dgMatrix (const dgQuaternion &rotation, const dgVector &position)
{
	float x2 = float (2.0f) * rotation.m_q1 * rotation.m_q1;
	float y2 = float (2.0f) * rotation.m_q2 * rotation.m_q2;
	float z2 = float (2.0f) * rotation.m_q3 * rotation.m_q3;

	float xy = float (2.0f) * rotation.m_q1 * rotation.m_q2;
	float xz = float (2.0f) * rotation.m_q1 * rotation.m_q3;
	float xw = float (2.0f) * rotation.m_q1 * rotation.m_q0;
	float yz = float (2.0f) * rotation.m_q2 * rotation.m_q3;
	float yw = float (2.0f) * rotation.m_q2 * rotation.m_q0;
	float zw = float (2.0f) * rotation.m_q3 * rotation.m_q0;

	m_front = dgVector (float(1.0f) - y2 - z2, xy + zw,                   xz - yw				    , float(0.0f));
	m_up    = dgVector (xy - zw,                   float(1.0f) - x2 - z2, yz + xw					, float(0.0f));
	m_right = dgVector (xz + yw,                   yz - xw,                   float(1.0f) - x2 - y2 , float(0.0f));


	m_posit.m_x = position.m_x;
	m_posit.m_y = position.m_y;
	m_posit.m_z = position.m_z;
	m_posit.m_w = float(1.0f);
}

dgMatrix dgMatrix::operator* (const dgMatrix &B) const
{
	const dgMatrix& A = *this;
	return dgMatrix (dgVector (A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0],
							   A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1],
							   A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2],
							   A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3]),
					 dgVector (A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0],
							   A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1],
							   A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2],
							   A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3]),
					 dgVector (A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0],
							   A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1],
							   A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2],
							   A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3]),
					 dgVector (A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0],
							   A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1],
							   A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2],
							   A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3]));
}



void dgMatrix::TransformTriplex (float* const dst, int32_t dstStrideInBytes, const float* const src, int32_t srcStrideInBytes, int32_t count) const
{
	int32_t dstStride = dstStrideInBytes / (int32_t)sizeof (float);
	int32_t srcStride = srcStrideInBytes / (int32_t)sizeof (float);

	int32_t dstIndex = 0;
	int32_t srcIndex = 0;
	for (int32_t i = 0 ; i < count; i ++ ) {
		float x = src[srcIndex + 0];
		float y = src[srcIndex + 1];
		float z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}

void dgMatrix::TransformTriplex (double* const dst, int32_t dstStrideInBytes, const double* const src, int32_t srcStrideInBytes, int32_t count) const
{
	int32_t dstStride = dstStrideInBytes / (int32_t)sizeof (double);
	int32_t srcStride = srcStrideInBytes / (int32_t)sizeof (double);

	int32_t dstIndex = 0;
	int32_t srcIndex = 0;
	for (int32_t i = 0 ; i < count; i ++ ) {
		double x = src[srcIndex + 0];
		double y = src[srcIndex + 1];
		double z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}


void dgMatrix::TransformTriplex (double* const dst, int32_t dstStrideInBytes, const float* const src, int32_t srcStrideInBytes, int32_t count) const
{
	int32_t dstStride = dstStrideInBytes / (int32_t)sizeof (double);
	int32_t srcStride = srcStrideInBytes / (int32_t)sizeof (float);

	int32_t dstIndex = 0;
	int32_t srcIndex = 0;
	for (int32_t i = 0 ; i < count; i ++ ) {
		double x = src[srcIndex + 0];
		double y = src[srcIndex + 1];
		double z = src[srcIndex + 2];
		srcIndex += srcStride;
		dst[dstIndex + 0] = x * m_front.m_x + y * m_up.m_x + z * m_right.m_x + m_posit.m_x;
		dst[dstIndex + 1] = x * m_front.m_y + y * m_up.m_y + z * m_right.m_y + m_posit.m_y;
		dst[dstIndex + 2] = x * m_front.m_z + y * m_up.m_z + z * m_right.m_z + m_posit.m_z;
		dstIndex += dstStride;
	}
}


void dgMatrix::TransformBBox (const dgVector& p0local, const dgVector& p1local, dgVector& p0, dgVector& p1) const
{
	dgVector box[8];

	box[0][0] = p0local[0];
	box[0][1] = p0local[1];
	box[0][2] = p0local[2];
	box[0][3] = float(1.0f);

	box[1][0] = p0local[0];
	box[1][1] = p0local[1];
	box[1][2] = p1local[2];
	box[1][3] = float(1.0f);

	box[2][0] = p0local[0];
	box[2][1] = p1local[1];
	box[2][2] = p0local[2];
	box[2][3] = float(1.0f);

	box[3][0] = p0local[0];
	box[3][1] = p1local[1];
	box[3][2] = p1local[2];
	box[3][3] = float(1.0f);

	box[4][0] = p1local[0];
	box[4][1] = p0local[1];
	box[4][2] = p0local[2];
	box[4][3] = float(1.0f);

	box[5][0] = p1local[0];
	box[5][1] = p0local[1];
	box[5][2] = p1local[2];
	box[1][3] = float(1.0f);

	box[6][0] = p1local[0];
	box[6][1] = p1local[1];
	box[6][2] = p0local[2];
	box[6][3] = float(1.0f);

	box[7][0] = p1local[0];
	box[7][1] = p1local[1];
	box[7][2] = p1local[2];
	box[7][3] = float(1.0f);

	TransformTriplex (&box[0].m_x, sizeof (dgVector), &box[0].m_x, sizeof (dgVector), 8);

	p0 = box[0];
	p1 = box[0];
	for (int32_t i = 1; i < 8; i ++) {
		p0.m_x = GetMin (p0.m_x, box[i].m_x);
		p0.m_y = GetMin (p0.m_y, box[i].m_y);
		p0.m_z = GetMin (p0.m_z, box[i].m_z);

		p1.m_x = GetMax (p1.m_x, box[i].m_x);
		p1.m_y = GetMax (p1.m_y, box[i].m_y);
		p1.m_z = GetMax (p1.m_z, box[i].m_z);
	}
}



dgMatrix dgMatrix::Symetric3by3Inverse () const
{
	const dgMatrix& mat = *this;
	double det = mat[0][0] * mat[1][1] * mat[2][2] + 
			mat[0][1] * mat[1][2] * mat[0][2] * float (2.0f) -
			mat[0][2] * mat[1][1] * mat[0][2] -
			mat[0][1] * mat[0][1] * mat[2][2] -
			mat[0][0] * mat[1][2] * mat[1][2];

	det = float (1.0f) / det;

	float x11 = (float)(det * (mat[1][1] * mat[2][2] - mat[1][2] * mat[1][2]));  
	float x22 = (float)(det * (mat[0][0] * mat[2][2] - mat[0][2] * mat[0][2]));  
	float x33 = (float)(det * (mat[0][0] * mat[1][1] - mat[0][1] * mat[0][1]));  

	float x12 = (float)(det * (mat[1][2] * mat[2][0] - mat[1][0] * mat[2][2]));  
	float x13 = (float)(det * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0]));  
	float x23 = (float)(det * (mat[0][1] * mat[2][0] - mat[0][0] * mat[2][1]));  


#ifdef _DEBUG
	dgMatrix matInv (dgVector (x11, x12, x13, float(0.0f)),
				     dgVector (x12, x22, x23, float(0.0f)),
					 dgVector (x13, x23, x33, float(0.0f)),
					 dgVector (float(0.0f), float(0.0f), float(0.0f), float(1.0f)));

	dgMatrix test (matInv * mat);
	HACD_ASSERT (dgAbsf (test[0][0] - float(1.0f)) < float(0.01f));
	HACD_ASSERT (dgAbsf (test[1][1] - float(1.0f)) < float(0.01f));
	HACD_ASSERT (dgAbsf (test[2][2] - float(1.0f)) < float(0.01f));
#endif

	return dgMatrix (dgVector (x11, x12, x13, float(0.0f)),
					 dgVector (x12, x22, x23, float(0.0f)),
					 dgVector (x13, x23, x33, float(0.0f)),
					 dgVector (float(0.0f), float(0.0f), float(0.0f), float(1.0f)));
}





dgVector dgMatrix::CalcPitchYawRoll () const
{
	const float minSin = float(0.99995f);

	const dgMatrix& matrix = *this;

	float roll = float(0.0f);
	float pitch  = float(0.0f);
	float yaw = dgAsin (-ClampValue (matrix[0][2], float(-0.999999f), float(0.999999f)));

	HACD_ASSERT (HACD_ISFINITE (yaw));
	if (matrix[0][2] < minSin) {
		if (matrix[0][2] > (-minSin)) {
			roll = dgAtan2 (matrix[0][1], matrix[0][0]);
			pitch = dgAtan2 (matrix[1][2], matrix[2][2]);
		} else {
			pitch = dgAtan2 (matrix[1][0], matrix[1][1]);
		}
	} else {
		pitch = -dgAtan2 (matrix[1][0], matrix[1][1]);
	}

	return dgVector (pitch, yaw, roll, float(0.0f));
}


/*
static inline void ROT(dgMatrix &a, int32_t i, int32_t j, int32_t k, int32_t l, float s, float tau) 
{
	float g = a[i][j]; 
	float h = a[k][l]; 
	a[i][j] = g - s * (h + g * tau); 
	a[k][l] = h + s * (g - h * tau);
}

// from numerical recipes in c
// Jacobian method for computing the eigenvectors of a symmetric matrix
void dgMatrix::EigenVectors (dgVector &eigenValues, const dgMatrix& initialGuess)
{
	float b[3];
	float z[3];
	float d[3];

	//	dgMatrix eigenVectors (initialGuess.Transpose4X4());
	//	dgMatrix &mat = *this;

	dgMatrix& mat = *this;
	dgMatrix eigenVectors (initialGuess.Transpose4X4());
	mat = initialGuess * mat * eigenVectors;


	b[0] = mat[0][0]; 
	b[1] = mat[1][1];
	b[2] = mat[2][2];

	d[0] = mat[0][0]; 
	d[1] = mat[1][1]; 
	d[2] = mat[2][2]; 

	z[0] = float (0.0f);
	z[1] = float (0.0f);
	z[2] = float (0.0f);

	int32_t nrot = 0;
	for (int32_t i = 0; i < 50; i++) {
		float sm = dgAbsf(mat[0][1]) + dgAbsf(mat[0][2]) + dgAbsf(mat[1][2]);

		if (sm < float (1.0e-6f)) {
			HACD_ASSERT (dgAbsf((eigenVectors.m_front % eigenVectors.m_front) - float(1.0f)) < dgEPSILON);
			HACD_ASSERT (dgAbsf((eigenVectors.m_up % eigenVectors.m_up) - float(1.0f)) < dgEPSILON);
			HACD_ASSERT (dgAbsf((eigenVectors.m_right % eigenVectors.m_right) - float(1.0f)) < dgEPSILON);

			// order the eigenvalue vectors	
			dgVector tmp (eigenVectors.m_front * eigenVectors.m_up);
			if (tmp % eigenVectors.m_right < float(0.0f)) {
				eigenVectors.m_right = eigenVectors.m_right.Scale (-float(1.0f));
			}

			eigenValues = dgVector (d[0], d[1], d[2], float (0.0f));
			*this = eigenVectors.Inverse();
			return;
		}

		float thresh = float (0.0f);
		if (i < 3) {
			thresh = (float)(0.2f / 9.0f) * sm;
		}


		// First row
		float g = float (100.0f) * dgAbsf(mat[0][1]);
		if ((i > 3) && (dgAbsf(d[0]) + g == dgAbsf(d[0])) && (dgAbsf(d[1]) + g == dgAbsf(d[1]))) {
			mat[0][1] = float (0.0f);
		} else if (dgAbsf(mat[0][1]) > thresh) {
			float h = d[1] - d[0];
			float t;
			if (dgAbsf(h) + g == dgAbsf(h)) {
				t = mat[0][1] / h;
			} else {
				float theta = float (0.5f) * h / mat[0][1];
				t = float(1.0f) / (dgAbsf(theta) + dgSqrt(float(1.0f) + theta * theta));
				if (theta < float (0.0f)) {
					t = -t;
				}
			}
			float c = float(1.0f) / dgSqrt (float (1.0f) + t * t); 
			float s = t * c; 
			float tau = s / (float(1.0f) + c); 
			h = t * mat[0][1];
			z[0] -= h; 
			z[1] += h; 
			d[0] -= h; 
			d[1] += h;
			mat[0][1] = float(0.0f);
			ROT (mat, 0, 2, 1, 2, s, tau); 
			ROT (eigenVectors, 0, 0, 0, 1, s, tau); 
			ROT (eigenVectors, 1, 0, 1, 1, s, tau); 
			ROT (eigenVectors, 2, 0, 2, 1, s, tau); 

			nrot++;
		}


		// second row
		g = float (100.0f) * dgAbsf(mat[0][2]);
		if ((i > 3) && (dgAbsf(d[0]) + g == dgAbsf(d[0])) && (dgAbsf(d[2]) + g == dgAbsf(d[2]))) {
			mat[0][2] = float (0.0f);
		} else if (dgAbsf(mat[0][2]) > thresh) {
			float h = d[2] - d[0];
			float t;
			if (dgAbsf(h) + g == dgAbsf(h)) {
				t = (mat[0][2]) / h;
			}	else {
				float theta = float (0.5f) * h / mat[0][2];
				t = float(1.0f) / (dgAbsf(theta) + dgSqrt(float(1.0f) + theta * theta));
				if (theta < float (0.0f)) {
					t = -t;
				}
			}
			float c = float(1.0f) / dgSqrt(float (1.0f) + t * t); 
			float s = t * c; 
			float tau = s / (float(1.0f) + c); 
			h = t * mat[0][2];
			z[0] -= h; 
			z[2] += h; 
			d[0] -= h; 
			d[2] += h;
			mat[0][2]=float (0.0f);
			ROT (mat, 0, 1, 1, 2, s, tau); 
			ROT (eigenVectors, 0, 0, 0, 2, s, tau); 
			ROT (eigenVectors, 1, 0, 1, 2, s, tau); 
			ROT (eigenVectors, 2, 0, 2, 2, s, tau); 
		}

		// third row
		g = float (100.0f) * dgAbsf(mat[1][2]);
		if ((i > 3) && (dgAbsf(d[1]) + g == dgAbsf(d[1])) && (dgAbsf(d[2]) + g == dgAbsf(d[2]))) {
			mat[1][2] = float (0.0f);
		} else if (dgAbsf(mat[1][2]) > thresh) {
			float h = d[2] - d[1];
			float t;
			if (dgAbsf(h) + g == dgAbsf(h)) {
				t = mat[1][2] / h;
			}	else {
				float theta = float (0.5f) * h / mat[1][2];
				t = float(1.0f) / (dgAbsf(theta) + dgSqrt(float(1.0f) + theta * theta));
				if (theta < float (0.0f)) {
					t = -t;
				}
			}
			float c = float(1.0f) / dgSqrt(float (1.0f) + t*t); 
			float s = t * c; 
			float tau = s / (float(1.0f) + c); 

			h = t * mat[1][2];
			z[1] -= h; 
			z[2] += h; 
			d[1] -= h; 
			d[2] += h;
			mat[1][2] = float (0.0f);
			ROT (mat, 0, 1, 0, 2, s, tau); 
			ROT (eigenVectors, 0, 1, 0, 2, s, tau); 
			ROT (eigenVectors, 1, 1, 1, 2, s, tau); 
			ROT (eigenVectors, 2, 1, 2, 2, s, tau); 
			nrot++;
		}

		b[0] += z[0]; d[0] = b[0]; z[0] = float (0.0f);
		b[1] += z[1]; d[1] = b[1]; z[1] = float (0.0f);
		b[2] += z[2]; d[2] = b[2]; z[2] = float (0.0f);
	}

	eigenValues = dgVector (d[0], d[1], d[2], float (0.0f));
	*this = dgGetIdentityMatrix();
} 	
*/

void dgMatrix::EigenVectors (dgVector &eigenValues, const dgMatrix& initialGuess)
{
	float b[3];
	float z[3];
	float d[3];

	dgMatrix& mat = *this;
	dgMatrix eigenVectors (initialGuess.Transpose4X4());
	mat = initialGuess * mat * eigenVectors;

	b[0] = mat[0][0]; 
	b[1] = mat[1][1];
	b[2] = mat[2][2];

	d[0] = mat[0][0]; 
	d[1] = mat[1][1]; 
	d[2] = mat[2][2]; 

	z[0] = float (0.0f);
	z[1] = float (0.0f);
	z[2] = float (0.0f);

	for (int32_t i = 0; i < 50; i++) {
		float sm = dgAbsf(mat[0][1]) + dgAbsf(mat[0][2]) + dgAbsf(mat[1][2]);

		if (sm < float (1.0e-6f)) {
			HACD_ASSERT (dgAbsf((eigenVectors.m_front % eigenVectors.m_front) - float(1.0f)) < dgEPSILON);
			HACD_ASSERT (dgAbsf((eigenVectors.m_up % eigenVectors.m_up) - float(1.0f)) < dgEPSILON);
			HACD_ASSERT (dgAbsf((eigenVectors.m_right % eigenVectors.m_right) - float(1.0f)) < dgEPSILON);

			// order the eigenvalue vectors	
			dgVector tmp (eigenVectors.m_front * eigenVectors.m_up);
			if (tmp % eigenVectors.m_right < float(0.0f)) {
				eigenVectors.m_right = eigenVectors.m_right.Scale (-float(1.0f));
			}

			eigenValues = dgVector (d[0], d[1], d[2], float (0.0f));
			*this = eigenVectors.Inverse();
			return;
		}

		float thresh = float (0.0f);
		if (i < 3) {
			thresh = (float)(0.2f / 9.0f) * sm;
		}

		for (int32_t ip = 0; ip < 2; ip ++) {
			for (int32_t iq = ip + 1; iq < 3; iq ++) {
				float g = float (100.0f) * dgAbsf(mat[ip][iq]);
				//if ((i > 3) && (dgAbsf(d[0]) + g == dgAbsf(d[ip])) && (dgAbsf(d[1]) + g == dgAbsf(d[1]))) {
				if ((i > 3) && ((dgAbsf(d[ip]) + g) == dgAbsf(d[ip])) && ((dgAbsf(d[iq]) + g) == dgAbsf(d[iq]))) {
					mat[ip][iq] = float (0.0f);
				} else if (dgAbsf(mat[ip][iq]) > thresh) {

					float t;
					float h = d[iq] - d[ip];
					if (dgAbsf(h) + g == dgAbsf(h)) {
						t = mat[ip][iq] / h;
					} else {
						float theta = float (0.5f) * h / mat[ip][iq];
						t = float(1.0f) / (dgAbsf(theta) + dgSqrt(float(1.0f) + theta * theta));
						if (theta < float (0.0f)) {
							t = -t;
						}
					}
					float c = float(1.0f) / dgSqrt (float (1.0f) + t * t); 
					float s = t * c; 
					float tau = s / (float(1.0f) + c); 
					h = t * mat[ip][iq];
					z[ip] -= h; 
					z[iq] += h; 
					d[ip] -= h; 
					d[iq] += h;
					mat[ip][iq] = float(0.0f);

					for (int32_t j = 0; j <= ip - 1; j ++) {
						//ROT (mat, j, ip, j, iq, s, tau); 
						//ROT(dgMatrix &a, int32_t i, int32_t j, int32_t k, int32_t l, float s, float tau) 
						float g = mat[j][ip]; 
						float h = mat[j][iq]; 
						mat[j][ip] = g - s * (h + g * tau); 
						mat[j][iq] = h + s * (g - h * tau);

					}
					for (int32_t j = ip + 1; j <= iq - 1; j ++) {
						//ROT (mat, ip, j, j, iq, s, tau); 
						//ROT(dgMatrix &a, int32_t i, int32_t j, int32_t k, int32_t l, float s, float tau) 
						float g = mat[ip][j]; 
						float h = mat[j][iq]; 
						mat[ip][j] = g - s * (h + g * tau); 
						mat[j][iq] = h + s * (g - h * tau);
					}
					for (int32_t j = iq + 1; j < 3; j ++) {
						//ROT (mat, ip, j, iq, j, s, tau); 
						//ROT(dgMatrix &a, int32_t i, int32_t j, int32_t k, int32_t l, float s, float tau) 
						float g = mat[ip][j]; 
						float h = mat[iq][j]; 
						mat[ip][j] = g - s * (h + g * tau); 
						mat[iq][j] = h + s * (g - h * tau);
					}

					for (int32_t j = 0; j < 3; j ++) {
						//ROT (eigenVectors, j, ip, j, iq, s, tau); 
						//ROT(dgMatrix &a, int32_t i, int32_t j, int32_t k, int32_t l, float s, float tau) 
						float g = eigenVectors[j][ip]; 
						float h = eigenVectors[j][iq]; 
						eigenVectors[j][ip] = g - s * (h + g * tau); 
						eigenVectors[j][iq] = h + s * (g - h * tau);
					}
				}
			}
		}
		b[0] += z[0]; d[0] = b[0]; z[0] = float (0.0f);
		b[1] += z[1]; d[1] = b[1]; z[1] = float (0.0f);
		b[2] += z[2]; d[2] = b[2]; z[2] = float (0.0f);
	}

	eigenValues = dgVector (d[0], d[1], d[2], float (0.0f));
	*this = dgGetIdentityMatrix();
}

