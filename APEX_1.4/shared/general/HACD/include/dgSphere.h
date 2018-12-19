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

#ifndef __dgSphere__
#define __dgSphere__

#include "dgTypes.h"
#include "dgVector.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"


class dgPlane;

class dgSphere: public dgMatrix
{
	public:
	dgSphere ();

	dgSphere (const dgQuaternion &quat, const dgVector &position, const dgVector& dim = dgVector(0, 0, 0, 0));
	dgSphere (const dgMatrix &matrix, const dgVector& dim = dgVector(0, 0, 0, 0));

	dgSphere &operator= (const dgMatrix &arg);
	void Scale (float Ws, float Hs, float Bs) ;
	void SetDimensions (float W, float H, float B);
	void SetDimensions (const float vertex[], int32_t strideInBytes, int32_t vertexCount, const dgMatrix *basis = NULL);
	void SetDimensions (const float vertex[], int32_t strideInBytes, const int32_t triangles[], int32_t indexCount, const dgMatrix *basis);

	private:

	public:
	dgVector m_size;
}; 


const dgSphere& GetIdentitySphere();


inline dgSphere &dgSphere::operator= (const dgMatrix &arg)
{
	m_front = arg.m_front;
	m_up = arg.m_up;
	m_right = arg.m_right;
	m_posit = arg.m_posit;
	return *this;
}

inline void dgSphere::SetDimensions (float W, float H, float B)
{
	m_size = dgVector (dgAbsf(W), dgAbsf(H), dgAbsf(B), dgSqrt (W * W + H * H + B * B));
}

inline void dgSphere::Scale (float Ws, float Hs, float Bs) 
{
	SetDimensions (m_size.m_x * Ws, m_size.m_y * Hs, m_size.m_z * Bs);
}


#endif

