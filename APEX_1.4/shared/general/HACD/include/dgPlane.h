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

#ifndef __dgPlane__
#define __dgPlane__

#include "dgVector.h"

class dgPlane: public dgVector
{
	public:
	dgPlane ();
	dgPlane (float x, float y, float z, float w);
	dgPlane (const dgVector &normal, float distance); 
	dgPlane (const dgVector &P0, const dgVector &P1, const dgVector &P2);
	dgPlane Scale (float s) const;
	float Evalue (const float *point) const;
	float Evalue (const dgVector &point) const;
};

class dgBigPlane: public dgBigVector
{
	public:
	dgBigPlane ();
	dgBigPlane (double x, double y, double z, double w);
	dgBigPlane (const dgBigVector &normal, double distance); 
	dgBigPlane (const dgBigVector &P0, const dgBigVector &P1, const dgBigVector &P2);
	dgBigPlane Scale (double s) const;
	double Evalue (const float *point) const;
#ifndef __USE_DOUBLE_PRECISION__
	double Evalue (const double *point) const;
#endif
	double Evalue (const dgVector &point) const;
	double Evalue (const dgBigVector &point) const;
};




HACD_INLINE dgPlane::dgPlane () 
	:dgVector () 
{
}

HACD_INLINE dgPlane::dgPlane (float x, float y, float z, float w)
	:dgVector (x, y, z, w) 
{
}

HACD_INLINE dgPlane::dgPlane (const dgVector &normal, float distance) 
	:dgVector (normal)
{
	m_w = distance;
}

HACD_INLINE dgPlane::dgPlane (const dgVector &P0, const dgVector &P1, const dgVector &P2)
	:dgVector ((P1 - P0) * (P2 - P0)) 
{
	m_w = - (*this % P0);
}

HACD_INLINE dgPlane dgPlane::Scale (float s)	const
{
	return dgPlane (m_x * s, m_y * s, m_z * s, m_w * s);
}


HACD_INLINE float dgPlane::Evalue (const float *point) const
{
	return m_x * point[0] + m_y * point[1] + m_z * point[2] + m_w;
}

HACD_INLINE float dgPlane::Evalue (const dgVector &point) const
{
	return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z + m_w;
}



HACD_INLINE dgBigPlane::dgBigPlane () 
	:dgBigVector () 
{
}

HACD_INLINE dgBigPlane::dgBigPlane (double x, double y, double z, double w)
	:dgBigVector (x, y, z, w) 
{
}


HACD_INLINE dgBigPlane::dgBigPlane (const dgBigVector &normal, double distance) 
	:dgBigVector (normal)
{
	m_w = distance;
}

HACD_INLINE dgBigPlane::dgBigPlane (const dgBigVector &P0, const dgBigVector &P1, const dgBigVector &P2)
	:dgBigVector ((P1 - P0) * (P2 - P0)) 
{
	m_w = - (*this % P0);
}

HACD_INLINE dgBigPlane dgBigPlane::Scale (double s) const
{
	return dgBigPlane (m_x * s, m_y * s, m_z * s, m_w * s);
}

HACD_INLINE double dgBigPlane::Evalue (const float *point) const
{
	return m_x * point[0] + m_y * point[1] + m_z * point[2] + m_w;
}

#ifndef __USE_DOUBLE_PRECISION__
HACD_INLINE double dgBigPlane::Evalue (const double *point) const
{
	return m_x * point[0] + m_y * point[1] + m_z * point[2] + m_w;
}
#endif

HACD_INLINE double dgBigPlane::Evalue (const dgVector &point) const
{
	return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z + m_w;
}

HACD_INLINE double dgBigPlane::Evalue (const dgBigVector &point) const
{
	return m_x * point.m_x + m_y * point.m_y + m_z * point.m_z + m_w;
}


#endif


