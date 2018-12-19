/*!
**
** Copyright (c) 2015 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**



*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "ConvexDecomposition.h"
#include "ConvexHull.h"
#include "dgConvexHull3d.h"

const float FM_PI = 3.1415926535897932384626433832795028841971693993751f;
const float FM_DEG_TO_RAD = ((2.0f * FM_PI) / 360.0f);


typedef hacd::vector< uint32_t > UintVector;

namespace CONVEX_DECOMPOSITION
{
class ConvexDecompInterface
{
public:
		virtual void ConvexDecompResult(const ConvexResult &result) = 0;
};



class Cdesc
{
public:
  ConvexDecompInterface *mCallback;
  float			mMeshVolumePercent;
  float			mMasterVolume;
  uint32_t			mMaxDepth;
  float			mConcavePercent;
  uint32_t			mOutputCount;
  uint32_t			mOutputPow2;
  hacd::ICallback		*mICallback;
};

static float fm_computePlane(const float *A,const float *B,const float *C,float *n) // returns D
{
	float vx = (B[0] - C[0]);
	float vy = (B[1] - C[1]);
	float vz = (B[2] - C[2]);

	float wx = (A[0] - B[0]);
	float wy = (A[1] - B[1]);
	float wz = (A[2] - B[2]);

	float vw_x = vy * wz - vz * wy;
	float vw_y = vz * wx - vx * wz;
	float vw_z = vx * wy - vy * wx;

	float mag = ::sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

	if ( mag < 0.000001f )
	{
		mag = 0;
	}
	else
	{
		mag = 1.0f/mag;
	}

	float x = vw_x * mag;
	float y = vw_y * mag;
	float z = vw_z * mag;


	float D = 0.0f - ((x*A[0])+(y*A[1])+(z*A[2]));

	n[0] = x;
	n[1] = y;
	n[2] = z;

	return D;
}

static float fm_dot(const float *p1,const float *p2)
{
	return p1[0]*p2[0]+p1[1]*p2[1]+p1[2]*p2[2];
}



static bool fm_samePlane(const float p1[4],const float p2[4],float normalEpsilon,float dEpsilon,bool doubleSided)
{
	bool ret = false;

	float diff = (float) fabs(p1[3]-p2[3]);
	if ( diff < dEpsilon ) // if the plane -d  co-efficient is within our epsilon
	{
		float dot = fm_dot(p1,p2); // compute the dot-product of the vector normals.
		if ( doubleSided ) dot = (float)fabs(dot);
		float dmin = 1 - normalEpsilon;
		float dmax = 1 + normalEpsilon;
		if ( dot >= dmin && dot <= dmax )
		{
			ret = true; // then the plane equation is for practical purposes identical.
		}
	}

	return ret;
}

static bool    fm_isMeshCoplanar(uint32_t tcount,const uint32_t *indices,const float *vertices,bool doubleSided) // returns true if this collection of indexed triangles are co-planar!
{
	bool ret = true;

	if ( tcount > 0 )
	{
		uint32_t i1 = indices[0];
		uint32_t i2 = indices[1];
		uint32_t i3 = indices[2];
		const float *p1 = &vertices[i1*3];
		const float *p2 = &vertices[i2*3];
		const float *p3 = &vertices[i3*3];
		float plane[4];
		plane[3] = fm_computePlane(p1,p2,p3,plane);
		const uint32_t *scan = &indices[3];
		for (uint32_t i=1; i<tcount; i++)
		{
			i1 = *scan++;
			i2 = *scan++;
			i3 = *scan++;
			p1 = &vertices[i1*3];
			p2 = &vertices[i2*3];
			p3 = &vertices[i3*3];
			float _plane[4];
			_plane[3] = fm_computePlane(p1,p2,p3,_plane);
			if ( !fm_samePlane(plane,_plane,0.01f,0.001f,doubleSided) )
			{
				ret = false;
				break;
			}
		}
	}
	return ret;
}


static float fm_distToPlane(const float *plane,const float *p) // computes the distance of this point from the plane.
{
	return p[0]*plane[0]+p[1]*plane[1]+p[2]*plane[2]+plane[3];
}

static void fm_cross(float *cross,const float *a,const float *b)
{
	cross[0] = a[1]*b[2] - a[2]*b[1];
	cross[1] = a[2]*b[0] - a[0]*b[2];
	cross[2] = a[0]*b[1] - a[1]*b[0];
}

/* a = b - c */
#define vector(a,b,c) \
	(a)[0] = (b)[0] - (c)[0];	\
	(a)[1] = (b)[1] - (c)[1];	\
	(a)[2] = (b)[2] - (c)[2];



#define innerProduct(v,q) \
	((v)[0] * (q)[0] + \
	(v)[1] * (q)[1] + \
	(v)[2] * (q)[2])

#define crossProduct(a,b,c) \
	(a)[0] = (b)[1] * (c)[2] - (c)[1] * (b)[2]; \
	(a)[1] = (b)[2] * (c)[0] - (c)[2] * (b)[0]; \
	(a)[2] = (b)[0] * (c)[1] - (c)[0] * (b)[1];


inline float det(const float *p1,const float *p2,const float *p3)
{
	return  p1[0]*p2[1]*p3[2] + p2[0]*p3[1]*p1[2] + p3[0]*p1[1]*p2[2] -p1[0]*p3[1]*p2[2] - p2[0]*p1[1]*p3[2] - p3[0]*p2[1]*p1[2];
}


static float  fm_computeMeshVolume(const float *vertices,uint32_t tcount,const uint32_t *indices)
{
	float volume = 0;

	for (uint32_t i=0; i<tcount; i++,indices+=3)
	{
		const float *p1 = &vertices[ indices[0]*3 ];
		const float *p2 = &vertices[ indices[1]*3 ];
		const float *p3 = &vertices[ indices[2]*3 ];
		volume+=det(p1,p2,p3); // compute the volume of the tetrahedran relative to the origin.
	}

	volume*=(1.0f/6.0f);
	if ( volume < 0 )
		volume*=-1;
	return volume;
}



// assumes that the points are on opposite sides of the plane!
static void fm_intersectPointPlane(const float *p1,const float *p2,float *split,const float *plane)
{

	float dp1 = fm_distToPlane(plane,p1);

	float dir[3];

	dir[0] = p2[0] - p1[0];
	dir[1] = p2[1] - p1[1];
	dir[2] = p2[2] - p1[2];

	float dot1 = dir[0]*plane[0] + dir[1]*plane[1] + dir[2]*plane[2];
	float dot2 = dp1 - plane[3];

	float    t = -(plane[3] + dot2 ) / dot1;

	split[0] = (dir[0]*t)+p1[0];
	split[1] = (dir[1]*t)+p1[1];
	split[2] = (dir[2]*t)+p1[2];

}


static void  fm_transform(const float matrix[16],const float v[3],float t[3]) // rotate and translate this point
{
	if ( matrix )
	{
		float tx = (matrix[0*4+0] * v[0]) +  (matrix[1*4+0] * v[1]) + (matrix[2*4+0] * v[2]) + matrix[3*4+0];
		float ty = (matrix[0*4+1] * v[0]) +  (matrix[1*4+1] * v[1]) + (matrix[2*4+1] * v[2]) + matrix[3*4+1];
		float tz = (matrix[0*4+2] * v[0]) +  (matrix[1*4+2] * v[1]) + (matrix[2*4+2] * v[2]) + matrix[3*4+2];
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

static void  fm_rotate(const float matrix[16],const float v[3],float t[3]) // rotate and translate this point
{
	if ( matrix )
	{
		float tx = (matrix[0*4+0] * v[0]) +  (matrix[1*4+0] * v[1]) + (matrix[2*4+0] * v[2]);
		float ty = (matrix[0*4+1] * v[0]) +  (matrix[1*4+1] * v[1]) + (matrix[2*4+1] * v[2]);
		float tz = (matrix[0*4+2] * v[0]) +  (matrix[1*4+2] * v[1]) + (matrix[2*4+2] * v[2]);
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




static void fm_inverseRT(const float matrix[16],const float pos[3],float t[3]) // inverse rotate translate the point.
{

	float _x = pos[0] - matrix[3*4+0];
	float _y = pos[1] - matrix[3*4+1];
	float _z = pos[2] - matrix[3*4+2];

	// Multiply inverse-translated source vector by inverted rotation transform

	t[0] = (matrix[0*4+0] * _x) + (matrix[0*4+1] * _y) + (matrix[0*4+2] * _z);
	t[1] = (matrix[1*4+0] * _x) + (matrix[1*4+1] * _y) + (matrix[1*4+2] * _z);
	t[2] = (matrix[2*4+0] * _x) + (matrix[2*4+1] * _y) + (matrix[2*4+2] * _z);

}

template <class Type> class Eigen
{
public:


	void DecrSortEigenStuff(void)
	{
		Tridiagonal(); //diagonalize the matrix.
		QLAlgorithm(); //
		DecreasingSort();
		GuaranteeRotation();
	}

	void Tridiagonal(void)
	{
		Type fM00 = mElement[0][0];
		Type fM01 = mElement[0][1];
		Type fM02 = mElement[0][2];
		Type fM11 = mElement[1][1];
		Type fM12 = mElement[1][2];
		Type fM22 = mElement[2][2];

		m_afDiag[0] = fM00;
		m_afSubd[2] = 0;
		if (fM02 != (Type)0.0)
		{
			Type fLength = static_cast<Type>(::sqrt(fM01*fM01+fM02*fM02));
			Type fInvLength = ((Type)1.0)/fLength;
			fM01 *= fInvLength;
			fM02 *= fInvLength;
			Type fQ = ((Type)2.0)*fM01*fM12+fM02*(fM22-fM11);
			m_afDiag[1] = fM11+fM02*fQ;
			m_afDiag[2] = fM22-fM02*fQ;
			m_afSubd[0] = fLength;
			m_afSubd[1] = fM12-fM01*fQ;
			mElement[0][0] = (Type)1.0;
			mElement[0][1] = (Type)0.0;
			mElement[0][2] = (Type)0.0;
			mElement[1][0] = (Type)0.0;
			mElement[1][1] = fM01;
			mElement[1][2] = fM02;
			mElement[2][0] = (Type)0.0;
			mElement[2][1] = fM02;
			mElement[2][2] = -fM01;
			m_bIsRotation = false;
		}
		else
		{
			m_afDiag[1] = fM11;
			m_afDiag[2] = fM22;
			m_afSubd[0] = fM01;
			m_afSubd[1] = fM12;
			mElement[0][0] = (Type)1.0;
			mElement[0][1] = (Type)0.0;
			mElement[0][2] = (Type)0.0;
			mElement[1][0] = (Type)0.0;
			mElement[1][1] = (Type)1.0;
			mElement[1][2] = (Type)0.0;
			mElement[2][0] = (Type)0.0;
			mElement[2][1] = (Type)0.0;
			mElement[2][2] = (Type)1.0;
			m_bIsRotation = true;
		}
	}

	bool QLAlgorithm(void)
	{
		const int32_t iMaxIter = 32;

		for (int32_t i0 = 0; i0 <3; i0++)
		{
			int32_t i1;
			for (i1 = 0; i1 < iMaxIter; i1++)
			{
				int32_t i2;
				for (i2 = i0; i2 <= (3-2); i2++)
				{
					Type fTmp = static_cast<Type>(fabs(m_afDiag[i2]) + fabs(m_afDiag[i2+1]));
					if ( fabs(m_afSubd[i2]) + fTmp == fTmp )
						break;
				}
				if (i2 == i0)
				{
					break;
				}

				Type fG = (m_afDiag[i0+1] - m_afDiag[i0])/(((Type)2.0) * m_afSubd[i0]);
				Type fR = static_cast<Type>(::sqrt(fG*fG+(Type)1.0));
				if (fG < (Type)0.0)
				{
					fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG-fR);
				}
				else
				{
					fG = m_afDiag[i2]-m_afDiag[i0]+m_afSubd[i0]/(fG+fR);
				}
				Type fSin = (Type)1.0, fCos = (Type)1.0, fP = (Type)0.0;
				for (int32_t i3 = i2-1; i3 >= i0; i3--)
				{
					Type fF = fSin*m_afSubd[i3];
					Type fB = fCos*m_afSubd[i3];
					if (fabs(fF) >= fabs(fG))
					{
						fCos = fG/fF;
						fR = static_cast<Type>(::sqrt(fCos*fCos+(Type)1.0));
						m_afSubd[i3+1] = fF*fR;
						fSin = ((Type)1.0)/fR;
						fCos *= fSin;
					}
					else
					{
						fSin = fF/fG;
						fR = static_cast<Type>(::sqrt(fSin*fSin+(Type)1.0));
						m_afSubd[i3+1] = fG*fR;
						fCos = ((Type)1.0)/fR;
						fSin *= fCos;
					}
					fG = m_afDiag[i3+1]-fP;
					fR = (m_afDiag[i3]-fG)*fSin+((Type)2.0)*fB*fCos;
					fP = fSin*fR;
					m_afDiag[i3+1] = fG+fP;
					fG = fCos*fR-fB;
					for (int32_t i4 = 0; i4 < 3; i4++)
					{
						fF = mElement[i4][i3+1];
						mElement[i4][i3+1] = fSin*mElement[i4][i3]+fCos*fF;
						mElement[i4][i3] = fCos*mElement[i4][i3]-fSin*fF;
					}
				}
				m_afDiag[i0] -= fP;
				m_afSubd[i0] = fG;
				m_afSubd[i2] = (Type)0.0;
			}
			if (i1 == iMaxIter)
			{
				return false;
			}
		}
		return true;
	}

	void DecreasingSort(void)
	{
		//sort eigenvalues in decreasing order, e[0] >= ... >= e[iSize-1]
		for (int32_t i0 = 0, i1; i0 <= 3-2; i0++)
		{
			// locate maximum eigenvalue
			i1 = i0;
			Type fMax = m_afDiag[i1];
			int32_t i2;
			for (i2 = i0+1; i2 < 3; i2++)
			{
				if (m_afDiag[i2] > fMax)
				{
					i1 = i2;
					fMax = m_afDiag[i1];
				}
			}

			if (i1 != i0)
			{
				// swap eigenvalues
				m_afDiag[i1] = m_afDiag[i0];
				m_afDiag[i0] = fMax;
				// swap eigenvectors
				for (i2 = 0; i2 < 3; i2++)
				{
					Type fTmp = mElement[i2][i0];
					mElement[i2][i0] = mElement[i2][i1];
					mElement[i2][i1] = fTmp;
					m_bIsRotation = !m_bIsRotation;
				}
			}
		}
	}


	void GuaranteeRotation(void)
	{
		if (!m_bIsRotation)
		{
			// change sign on the first column
			for (int32_t iRow = 0; iRow <3; iRow++)
			{
				mElement[iRow][0] = -mElement[iRow][0];
			}
		}
	}

	Type mElement[3][3];
	Type m_afDiag[3];
	Type m_afSubd[3];
	bool m_bIsRotation;
};


static bool fm_computeBestFitPlane(uint32_t vcount,
							const float *points,
							uint32_t vstride,
							const float *weights,
							uint32_t wstride,
							float *plane)
{
	bool ret = false;

	float kOrigin[3] = { 0, 0, 0 };

	float wtotal = 0;

	{
		const char *source  = (const char *) points;
		const char *wsource = (const char *) weights;

		for (uint32_t i=0; i<vcount; i++)
		{

			const float *p = (const float *) source;

			float w = 1;

			if ( wsource )
			{
				const float *ws = (const float *) wsource;
				w = *ws; //
				wsource+=wstride;
			}

			kOrigin[0]+=p[0]*w;
			kOrigin[1]+=p[1]*w;
			kOrigin[2]+=p[2]*w;

			wtotal+=w;

			source+=vstride;
		}
	}

	float recip = 1.0f / wtotal; // reciprocol of total weighting

	kOrigin[0]*=recip;
	kOrigin[1]*=recip;
	kOrigin[2]*=recip;


	float fSumXX=0;
	float fSumXY=0;
	float fSumXZ=0;

	float fSumYY=0;
	float fSumYZ=0;
	float fSumZZ=0;


	{
		const char *source  = (const char *) points;
		const char *wsource = (const char *) weights;

		for (uint32_t i=0; i<vcount; i++)
		{

			const float *p = (const float *) source;

			float w = 1;

			if ( wsource )
			{
				const float *ws = (const float *) wsource;
				w = *ws; //
				wsource+=wstride;
			}

			float kDiff[3];

			kDiff[0] = w*(p[0] - kOrigin[0]); // apply vertex weighting!
			kDiff[1] = w*(p[1] - kOrigin[1]);
			kDiff[2] = w*(p[2] - kOrigin[2]);

			fSumXX+= kDiff[0] * kDiff[0]; // sume of the squares of the differences.
			fSumXY+= kDiff[0] * kDiff[1]; // sume of the squares of the differences.
			fSumXZ+= kDiff[0] * kDiff[2]; // sume of the squares of the differences.

			fSumYY+= kDiff[1] * kDiff[1];
			fSumYZ+= kDiff[1] * kDiff[2];
			fSumZZ+= kDiff[2] * kDiff[2];


			source+=vstride;
		}
	}

	fSumXX *= recip;
	fSumXY *= recip;
	fSumXZ *= recip;
	fSumYY *= recip;
	fSumYZ *= recip;
	fSumZZ *= recip;

	// setup the eigensolver
	Eigen<float> kES;

	kES.mElement[0][0] = fSumXX;
	kES.mElement[0][1] = fSumXY;
	kES.mElement[0][2] = fSumXZ;

	kES.mElement[1][0] = fSumXY;
	kES.mElement[1][1] = fSumYY;
	kES.mElement[1][2] = fSumYZ;

	kES.mElement[2][0] = fSumXZ;
	kES.mElement[2][1] = fSumYZ;
	kES.mElement[2][2] = fSumZZ;

	// compute eigenstuff, smallest eigenvalue is in last position
	kES.DecrSortEigenStuff();

	float kNormal[3];

	kNormal[0] = kES.mElement[0][2];
	kNormal[1] = kES.mElement[1][2];
	kNormal[2] = kES.mElement[2][2];

	// the minimum energy
	plane[0] = kNormal[0];
	plane[1] = kNormal[1];
	plane[2] = kNormal[2];

	plane[3] = 0 - fm_dot(kNormal,kOrigin);

	ret = true;

	return ret;
}




// computes the OBB for this set of points relative to this transform matrix.
void computeOBB(uint32_t vcount,const float *points,uint32_t pstride,float *sides,float *matrix)
{
	const char *src = (const char *) points;

	float bmin[3] = { 1e9, 1e9, 1e9 };
	float bmax[3] = { -1e9, -1e9, -1e9 };

	for (uint32_t i=0; i<vcount; i++)
	{
		const float *p = (const float *) src;
		float t[3];

		fm_inverseRT(matrix, p, t ); // inverse rotate translate

		if ( t[0] < bmin[0] ) bmin[0] = t[0];
		if ( t[1] < bmin[1] ) bmin[1] = t[1];
		if ( t[2] < bmin[2] ) bmin[2] = t[2];

		if ( t[0] > bmax[0] ) bmax[0] = t[0];
		if ( t[1] > bmax[1] ) bmax[1] = t[1];
		if ( t[2] > bmax[2] ) bmax[2] = t[2];

		src+=pstride;
	}

	float center[3];

	sides[0] = bmax[0]-bmin[0];
	sides[1] = bmax[1]-bmin[1];
	sides[2] = bmax[2]-bmin[2];

	center[0] = sides[0]*0.5f+bmin[0];
	center[1] = sides[1]*0.5f+bmin[1];
	center[2] = sides[2]*0.5f+bmin[2];

	float ocenter[3];

	fm_rotate(matrix,center,ocenter);

	matrix[12]+=ocenter[0];
	matrix[13]+=ocenter[1];
	matrix[14]+=ocenter[2];

}

// Reference, from Stan Melax in Game Gems I
//  Quaternion q;
//  vector3 c = CrossProduct(v0,v1);
//  float   d = DotProduct(v0,v1);
//  float   s = (float)::sqrt((1+d)*2);
//  q.x = c.x / s;
//  q.y = c.y / s;
//  q.z = c.z / s;
//  q.w = s /2.0f;
//  return q;
static void fm_rotationArc(const float *v0,const float *v1,float *quat)
{
	float cross[3];

	fm_cross(cross,v0,v1);
	float d = fm_dot(v0,v1);

	if(d<=-1.0f) // 180 about x axis
	{
		quat[0] = 1.0f;
		quat[1] = quat[2] = quat[3] =0.0f;
		return;
	}
	else
	{
		float s = ::sqrtf((1+d)*2);
		float recip = 1.0f / s;

		quat[0] = cross[0] * recip;
		quat[1] = cross[1] * recip;
		quat[2] = cross[2] * recip;
		quat[3] = s * 0.5f;
	}
}

static void  fm_setTranslation(const float *translation,float *matrix)
{
	matrix[12] = translation[0];
	matrix[13] = translation[1];
	matrix[14] = translation[2];
}


static void fm_eulerToQuat(float roll,float pitch,float yaw,float *quat) // convert euler angles to quaternion.
{
	roll  *= 0.5f;
	pitch *= 0.5f;
	yaw   *= 0.5f;

	float cr = ::cosf(roll);
	float cp = ::cosf(pitch);
	float cy = ::cosf(yaw);

	float sr = ::sinf(roll);
	float sp = ::sinf(pitch);
	float sy = ::sinf(yaw);

	float cpcy = cp * cy;
	float spsy = sp * sy;
	float spcy = sp * cy;
	float cpsy = cp * sy;

	quat[0]   = ( sr * cpcy - cr * spsy);
	quat[1]   = ( cr * spcy + sr * cpsy);
	quat[2]   = ( cr * cpsy - sr * spcy);
	quat[3]   = cr * cpcy + sr * spsy;
}


static void fm_quatToMatrix(const float *quat,float *matrix) // convert quaterinion rotation to matrix, zeros out the translation component.
{

	float xx = quat[0]*quat[0];
	float yy = quat[1]*quat[1];
	float zz = quat[2]*quat[2];
	float xy = quat[0]*quat[1];
	float xz = quat[0]*quat[2];
	float yz = quat[1]*quat[2];
	float wx = quat[3]*quat[0];
	float wy = quat[3]*quat[1];
	float wz = quat[3]*quat[2];

	matrix[0*4+0] = 1 - 2 * ( yy + zz );
	matrix[1*4+0] =     2 * ( xy - wz );
	matrix[2*4+0] =     2 * ( xz + wy );

	matrix[0*4+1] =     2 * ( xy + wz );
	matrix[1*4+1] = 1 - 2 * ( xx + zz );
	matrix[2*4+1] =     2 * ( yz - wx );

	matrix[0*4+2] =     2 * ( xz - wy );
	matrix[1*4+2] =     2 * ( yz + wx );
	matrix[2*4+2] = 1 - 2 * ( xx + yy );

	matrix[3*4+0] = matrix[3*4+1] = matrix[3*4+2] = (float) 0.0f;
	matrix[0*4+3] = matrix[1*4+3] = matrix[2*4+3] = (float) 0.0f;
	matrix[3*4+3] =(float) 1.0f;

}

static void  fm_matrixMultiply(const float *pA,const float *pB,float *pM)
{
	float a = pA[0*4+0] * pB[0*4+0] + pA[0*4+1] * pB[1*4+0] + pA[0*4+2] * pB[2*4+0] + pA[0*4+3] * pB[3*4+0];
	float b = pA[0*4+0] * pB[0*4+1] + pA[0*4+1] * pB[1*4+1] + pA[0*4+2] * pB[2*4+1] + pA[0*4+3] * pB[3*4+1];
	float c = pA[0*4+0] * pB[0*4+2] + pA[0*4+1] * pB[1*4+2] + pA[0*4+2] * pB[2*4+2] + pA[0*4+3] * pB[3*4+2];
	float d = pA[0*4+0] * pB[0*4+3] + pA[0*4+1] * pB[1*4+3] + pA[0*4+2] * pB[2*4+3] + pA[0*4+3] * pB[3*4+3];

	float e = pA[1*4+0] * pB[0*4+0] + pA[1*4+1] * pB[1*4+0] + pA[1*4+2] * pB[2*4+0] + pA[1*4+3] * pB[3*4+0];
	float f = pA[1*4+0] * pB[0*4+1] + pA[1*4+1] * pB[1*4+1] + pA[1*4+2] * pB[2*4+1] + pA[1*4+3] * pB[3*4+1];
	float g = pA[1*4+0] * pB[0*4+2] + pA[1*4+1] * pB[1*4+2] + pA[1*4+2] * pB[2*4+2] + pA[1*4+3] * pB[3*4+2];
	float h = pA[1*4+0] * pB[0*4+3] + pA[1*4+1] * pB[1*4+3] + pA[1*4+2] * pB[2*4+3] + pA[1*4+3] * pB[3*4+3];

	float i = pA[2*4+0] * pB[0*4+0] + pA[2*4+1] * pB[1*4+0] + pA[2*4+2] * pB[2*4+0] + pA[2*4+3] * pB[3*4+0];
	float j = pA[2*4+0] * pB[0*4+1] + pA[2*4+1] * pB[1*4+1] + pA[2*4+2] * pB[2*4+1] + pA[2*4+3] * pB[3*4+1];
	float k = pA[2*4+0] * pB[0*4+2] + pA[2*4+1] * pB[1*4+2] + pA[2*4+2] * pB[2*4+2] + pA[2*4+3] * pB[3*4+2];
	float l = pA[2*4+0] * pB[0*4+3] + pA[2*4+1] * pB[1*4+3] + pA[2*4+2] * pB[2*4+3] + pA[2*4+3] * pB[3*4+3];

	float m = pA[3*4+0] * pB[0*4+0] + pA[3*4+1] * pB[1*4+0] + pA[3*4+2] * pB[2*4+0] + pA[3*4+3] * pB[3*4+0];
	float n = pA[3*4+0] * pB[0*4+1] + pA[3*4+1] * pB[1*4+1] + pA[3*4+2] * pB[2*4+1] + pA[3*4+3] * pB[3*4+1];
	float o = pA[3*4+0] * pB[0*4+2] + pA[3*4+1] * pB[1*4+2] + pA[3*4+2] * pB[2*4+2] + pA[3*4+3] * pB[3*4+2];
	float p = pA[3*4+0] * pB[0*4+3] + pA[3*4+1] * pB[1*4+3] + pA[3*4+2] * pB[2*4+3] + pA[3*4+3] * pB[3*4+3];

	pM[0] = a;
	pM[1] = b;
	pM[2] = c;
	pM[3] = d;

	pM[4] = e;
	pM[5] = f;
	pM[6] = g;
	pM[7] = h;

	pM[8] = i;
	pM[9] = j;
	pM[10] = k;
	pM[11] = l;

	pM[12] = m;
	pM[13] = n;
	pM[14] = o;
	pM[15] = p;
}




static void fm_planeToMatrix(const float *plane,float *matrix) // convert a plane equation to a 4x4 rotation matrix
{
	float ref[3] = { 0, 1, 0 };
	float quat[4];
	fm_rotationArc(ref,plane,quat);
	fm_quatToMatrix(quat,matrix);
	float origin[3] = { 0, -plane[3], 0 };
	float center[3];
	fm_transform(matrix,origin,center);
	fm_setTranslation(center,matrix);
}


static void fm_computeBestFitOBB(uint32_t vcount,
						  const float *points,
						  uint32_t pstride,
						  float *sides,
						  float *matrix,
						  bool bruteForce)
{
	float plane[4];
	fm_computeBestFitPlane(vcount,points,pstride,0,0,plane);
	fm_planeToMatrix(plane,matrix);
	computeOBB( vcount, points, pstride, sides, matrix );

	float refmatrix[16];
	memcpy(refmatrix,matrix,16*sizeof(float));

	float volume = sides[0]*sides[1]*sides[2];
	if ( bruteForce )
	{
		for (float a=10; a<180; a+=10)
		{
			float quat[4];
			fm_eulerToQuat(0,a*FM_DEG_TO_RAD,0,quat);
			float temp[16];
			float pmatrix[16];
			fm_quatToMatrix(quat,temp);
			fm_matrixMultiply(temp,refmatrix,pmatrix);
			float psides[3];
			computeOBB( vcount, points, pstride, psides, pmatrix );
			float v = psides[0]*psides[1]*psides[2];
			if ( v < volume )
			{
				volume = v;
				memcpy(matrix,pmatrix,sizeof(float)*16);
				sides[0] = psides[0];
				sides[1] = psides[1];
				sides[2] = psides[2];
			}
		}
	}
}

template <class T> class Rect3d
{
public:
	Rect3d(void) { };

	Rect3d(const T *bmin,const T *bmax)
	{

		mMin[0] = bmin[0];
		mMin[1] = bmin[1];
		mMin[2] = bmin[2];

		mMax[0] = bmax[0];
		mMax[1] = bmax[1];
		mMax[2] = bmax[2];

	}

	void SetMin(const T *bmin)
	{
		mMin[0] = bmin[0];
		mMin[1] = bmin[1];
		mMin[2] = bmin[2];
	}

	void SetMax(const T *bmax)
	{
		mMax[0] = bmax[0];
		mMax[1] = bmax[1];
		mMax[2] = bmax[2];
	}

	void SetMin(T x,T y,T z)
	{
		mMin[0] = x;
		mMin[1] = y;
		mMin[2] = z;
	}

	void SetMax(T x,T y,T z)
	{
		mMax[0] = x;
		mMax[1] = y;
		mMax[2] = z;
	}

	T mMin[3];
	T mMax[3];
};

void splitRect(uint32_t axis,
			   const Rect3d<float> &source,
			   Rect3d<float> &b1,
			   Rect3d<float> &b2,
			   const float *midpoint)
{
	switch ( axis )
	{
	case 0:
		b1.SetMin(source.mMin);
		b1.SetMax( midpoint[0], source.mMax[1], source.mMax[2] );

		b2.SetMin( midpoint[0], source.mMin[1], source.mMin[2] );
		b2.SetMax(source.mMax);

		break;
	case 1:
		b1.SetMin(source.mMin);
		b1.SetMax( source.mMax[0], midpoint[1], source.mMax[2] );

		b2.SetMin( source.mMin[0], midpoint[1], source.mMin[2] );
		b2.SetMax(source.mMax);

		break;
	case 2:
		b1.SetMin(source.mMin);
		b1.SetMax( source.mMax[0], source.mMax[1], midpoint[2] );

		b2.SetMin( source.mMin[0], source.mMin[1], midpoint[2] );
		b2.SetMax(source.mMax);

		break;
	}
}



static bool fm_computeSplitPlane(uint32_t vcount,
						  const float *vertices,
						  uint32_t /* tcount */,
						  const uint32_t * /* indices */,
						  float *plane)
{

	float sides[3];
	float matrix[16];

	fm_computeBestFitOBB( vcount, vertices, sizeof(float)*3, sides, matrix, false );

	float bmax[3];
	float bmin[3];

	bmax[0] = sides[0]*0.5f;
	bmax[1] = sides[1]*0.5f;
	bmax[2] = sides[2]*0.5f;

	bmin[0] = -bmax[0];
	bmin[1] = -bmax[1];
	bmin[2] = -bmax[2];


	float dx = sides[0];
	float dy = sides[1];
	float dz = sides[2];


	uint32_t axis = 0;

	if ( dy > dx )
	{
		axis = 1;
	}

	if ( dz > dx && dz > dy )
	{
		axis = 2;
	}

	float p1[3];
	float p2[3];
	float p3[3];

	p3[0] = p2[0] = p1[0] = bmin[0] + dx*0.5f;
	p3[1] = p2[1] = p1[1] = bmin[1] + dy*0.5f;
	p3[2] = p2[2] = p1[2] = bmin[2] + dz*0.5f;

	Rect3d<float> b(bmin,bmax);

	Rect3d<float> b1,b2;

	splitRect(axis,b,b1,b2,p1);


	switch ( axis )
	{
	case 0:
		p2[1] = bmin[1];
		p2[2] = bmin[2];

		if ( dz > dy )
		{
			p3[1] = bmax[1];
			p3[2] = bmin[2];
		}
		else
		{
			p3[1] = bmin[1];
			p3[2] = bmax[2];
		}

		break;
	case 1:
		p2[0] = bmin[0];
		p2[2] = bmin[2];

		if ( dx > dz )
		{
			p3[0] = bmax[0];
			p3[2] = bmin[2];
		}
		else
		{
			p3[0] = bmin[0];
			p3[2] = bmax[2];
		}

		break;
	case 2:
		p2[0] = bmin[0];
		p2[1] = bmin[1];

		if ( dx > dy )
		{
			p3[0] = bmax[0];
			p3[1] = bmin[1];
		}
		else
		{
			p3[0] = bmin[0];
			p3[1] = bmax[1];
		}

		break;
	}

	float tp1[3];
	float tp2[3];
	float tp3[3];

	fm_transform(matrix,p1,tp1);
	fm_transform(matrix,p2,tp2);
	fm_transform(matrix,p3,tp3);

	plane[3] = fm_computePlane(tp1,tp2,tp3,plane);

	return true;

}



#define FM_DEFAULT_GRANULARITY 0.001f  // 1 millimeter is the default granularity

class fm_VertexIndex
{
public:
	virtual uint32_t          getIndex(const float pos[3],bool &newPos) = 0;  // get welded index for this float vector[3]
	virtual uint32_t          getIndex(const double pos[3],bool &newPos) = 0;  // get welded index for this double vector[3]
	virtual const float *   getVerticesFloat(void) const = 0;
	virtual const double *  getVerticesDouble(void) const = 0;
	virtual const float *   getVertexFloat(uint32_t index) const = 0;
	virtual const double *  getVertexDouble(uint32_t index) const = 0;
	virtual uint32_t          getVcount(void) const = 0;
	virtual bool            isDouble(void) const = 0;
	virtual bool            saveAsObj(const char *fname,uint32_t tcount,uint32_t *indices) = 0;
};

//static fm_VertexIndex * fm_createVertexIndex(double granularity,bool snapToGrid); // create an indexed vertex system for doubles
static void             fm_releaseVertexIndex(fm_VertexIndex *vindex);



class KdTreeNode;

typedef hacd::vector< KdTreeNode * > KdTreeNodeVector;

enum Axes
{
	X_AXIS = 0,
	Y_AXIS = 1,
	Z_AXIS = 2
};

class KdTreeFindNode
{
public:
	KdTreeFindNode(void)
	{
		mNode = 0;
		mDistance = 0;
	}
	KdTreeNode  *mNode;
	double        mDistance;
};

class KdTreeInterface
{
public:
	virtual const double * getPositionDouble(uint32_t index) const = 0;
	virtual const float  * getPositionFloat(uint32_t index) const = 0;
};

class KdTreeNode
{
public:
	KdTreeNode(void)
	{
		mIndex = 0;
		mLeft = 0;
		mRight = 0;
	}

	KdTreeNode(uint32_t index)
	{
		mIndex = index;
		mLeft = 0;
		mRight = 0;
	};

	~KdTreeNode(void)
	{
	}


	void addDouble(KdTreeNode *node,Axes dim,const KdTreeInterface *iface)
	{
		const double *nodePosition = iface->getPositionDouble( node->mIndex );
		const double *position     = iface->getPositionDouble( mIndex );
		switch ( dim )
		{
		case X_AXIS:
			if ( nodePosition[0] <= position[0] )
			{
				if ( mLeft )
					mLeft->addDouble(node,Y_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addDouble(node,Y_AXIS,iface);
				else
					mRight = node;
			}
			break;
		case Y_AXIS:
			if ( nodePosition[1] <= position[1] )
			{
				if ( mLeft )
					mLeft->addDouble(node,Z_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addDouble(node,Z_AXIS,iface);
				else
					mRight = node;
			}
			break;
		case Z_AXIS:
			if ( nodePosition[2] <= position[2] )
			{
				if ( mLeft )
					mLeft->addDouble(node,X_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addDouble(node,X_AXIS,iface);
				else
					mRight = node;
			}
			break;
		}

	}


	void addFloat(KdTreeNode *node,Axes dim,const KdTreeInterface *iface)
	{
		const float *nodePosition = iface->getPositionFloat( node->mIndex );
		const float *position     = iface->getPositionFloat( mIndex );
		switch ( dim )
		{
		case X_AXIS:
			if ( nodePosition[0] <= position[0] )
			{
				if ( mLeft )
					mLeft->addFloat(node,Y_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addFloat(node,Y_AXIS,iface);
				else
					mRight = node;
			}
			break;
		case Y_AXIS:
			if ( nodePosition[1] <= position[1] )
			{
				if ( mLeft )
					mLeft->addFloat(node,Z_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addFloat(node,Z_AXIS,iface);
				else
					mRight = node;
			}
			break;
		case Z_AXIS:
			if ( nodePosition[2] <= position[2] )
			{
				if ( mLeft )
					mLeft->addFloat(node,X_AXIS,iface);
				else
					mLeft = node;
			}
			else
			{
				if ( mRight )
					mRight->addFloat(node,X_AXIS,iface);
				else
					mRight = node;
			}
			break;
		}

	}


	uint32_t getIndex(void) const { return mIndex; };

	void search(Axes axis,const double *pos,double radius,uint32_t &count,uint32_t maxObjects,KdTreeFindNode *found,const KdTreeInterface *iface)
	{

		const double *position = iface->getPositionDouble(mIndex);

		double dx = pos[0] - position[0];
		double dy = pos[1] - position[1];
		double dz = pos[2] - position[2];

		KdTreeNode *search1 = 0;
		KdTreeNode *search2 = 0;

		switch ( axis )
		{
		case X_AXIS:
			if ( dx <= 0 )     // JWR  if we are to the left
			{
				search1 = mLeft; // JWR  then search to the left
				if ( -dx < radius )  // JWR  if distance to the right is less than our search radius, continue on the right as well.
					search2 = mRight;
			}
			else
			{
				search1 = mRight; // JWR  ok, we go down the left tree
				if ( dx < radius ) // JWR  if the distance from the right is less than our search radius
					search2 = mLeft;
			}
			axis = Y_AXIS;
			break;
		case Y_AXIS:
			if ( dy <= 0 )
			{
				search1 = mLeft;
				if ( -dy < radius )
					search2 = mRight;
			}
			else
			{
				search1 = mRight;
				if ( dy < radius )
					search2 = mLeft;
			}
			axis = Z_AXIS;
			break;
		case Z_AXIS:
			if ( dz <= 0 )
			{
				search1 = mLeft;
				if ( -dz < radius )
					search2 = mRight;
			}
			else
			{
				search1 = mRight;
				if ( dz < radius )
					search2 = mLeft;
			}
			axis = X_AXIS;
			break;
		}

		double r2 = radius*radius;
		double m  = dx*dx+dy*dy+dz*dz;

		if ( m < r2 )
		{
			switch ( count )
			{
			case 0:
				found[count].mNode = this;
				found[count].mDistance = m;
				break;
			case 1:
				if ( m < found[0].mDistance )
				{
					if ( maxObjects == 1 )
					{
						found[0].mNode = this;
						found[0].mDistance = m;
					}
					else
					{
						found[1] = found[0];
						found[0].mNode = this;
						found[0].mDistance = m;
					}
				}
				else if ( maxObjects > 1)
				{
					found[1].mNode = this;
					found[1].mDistance = m;
				}
				break;
			default:
				{
					bool inserted = false;

					for (uint32_t i=0; i<count; i++)
					{
						if ( m < found[i].mDistance ) // if this one is closer than a pre-existing one...
						{
							// insertion sort...
							uint32_t scan = count;
							if ( scan >= maxObjects ) scan=maxObjects-1;
							for (uint32_t j=scan; j>i; j--)
							{
								found[j] = found[j-1];
							}
							found[i].mNode = this;
							found[i].mDistance = m;
							inserted = true;
							break;
						}
					}

					if ( !inserted && count < maxObjects )
					{
						found[count].mNode = this;
						found[count].mDistance = m;
					}
				}
				break;
			}
			count++;
			if ( count > maxObjects )
			{
				count = maxObjects;
			}
		}


		if ( search1 )
			search1->search( axis, pos,radius, count, maxObjects, found, iface);

		if ( search2 )
			search2->search( axis, pos,radius, count, maxObjects, found, iface);

	}

	void search(Axes axis,const float *pos,float radius,uint32_t &count,uint32_t maxObjects,KdTreeFindNode *found,const KdTreeInterface *iface)
	{

		const float *position = iface->getPositionFloat(mIndex);

		float dx = pos[0] - position[0];
		float dy = pos[1] - position[1];
		float dz = pos[2] - position[2];

		KdTreeNode *search1 = 0;
		KdTreeNode *search2 = 0;

		switch ( axis )
		{
		case X_AXIS:
			if ( dx <= 0 )     // JWR  if we are to the left
			{
				search1 = mLeft; // JWR  then search to the left
				if ( -dx < radius )  // JWR  if distance to the right is less than our search radius, continue on the right as well.
					search2 = mRight;
			}
			else
			{
				search1 = mRight; // JWR  ok, we go down the left tree
				if ( dx < radius ) // JWR  if the distance from the right is less than our search radius
					search2 = mLeft;
			}
			axis = Y_AXIS;
			break;
		case Y_AXIS:
			if ( dy <= 0 )
			{
				search1 = mLeft;
				if ( -dy < radius )
					search2 = mRight;
			}
			else
			{
				search1 = mRight;
				if ( dy < radius )
					search2 = mLeft;
			}
			axis = Z_AXIS;
			break;
		case Z_AXIS:
			if ( dz <= 0 )
			{
				search1 = mLeft;
				if ( -dz < radius )
					search2 = mRight;
			}
			else
			{
				search1 = mRight;
				if ( dz < radius )
					search2 = mLeft;
			}
			axis = X_AXIS;
			break;
		}

		float r2 = radius*radius;
		float m  = dx*dx+dy*dy+dz*dz;

		if ( m < r2 )
		{
			switch ( count )
			{
			case 0:
				found[count].mNode = this;
				found[count].mDistance = m;
				break;
			case 1:
				if ( m < found[0].mDistance )
				{
					if ( maxObjects == 1 )
					{
						found[0].mNode = this;
						found[0].mDistance = m;
					}
					else
					{
						found[1] = found[0];
						found[0].mNode = this;
						found[0].mDistance = m;
					}
				}
				else if ( maxObjects > 1)
				{
					found[1].mNode = this;
					found[1].mDistance = m;
				}
				break;
			default:
				{
					bool inserted = false;

					for (uint32_t i=0; i<count; i++)
					{
						if ( m < found[i].mDistance ) // if this one is closer than a pre-existing one...
						{
							// insertion sort...
							uint32_t scan = count;
							if ( scan >= maxObjects ) scan=maxObjects-1;
							for (uint32_t j=scan; j>i; j--)
							{
								found[j] = found[j-1];
							}
							found[i].mNode = this;
							found[i].mDistance = m;
							inserted = true;
							break;
						}
					}

					if ( !inserted && count < maxObjects )
					{
						found[count].mNode = this;
						found[count].mDistance = m;
					}
				}
				break;
			}
			count++;
			if ( count > maxObjects )
			{
				count = maxObjects;
			}
		}


		if ( search1 )
			search1->search( axis, pos,radius, count, maxObjects, found, iface);

		if ( search2 )
			search2->search( axis, pos,radius, count, maxObjects, found, iface);

	}

private:

	void setLeft(KdTreeNode *left) { mLeft = left; };
	void setRight(KdTreeNode *right) { mRight = right; };

	KdTreeNode *getLeft(void)         { return mLeft; }
	KdTreeNode *getRight(void)        { return mRight; }

	uint32_t          mIndex;
	KdTreeNode     *mLeft;
	KdTreeNode     *mRight;
};


#define MAX_BUNDLE_SIZE 1024  // 1024 nodes at a time, to minimize memory allocation and guarentee that pointers are persistent.

class KdTreeNodeBundle : public UANS::UserAllocated
{
public:

	KdTreeNodeBundle(void)
	{
		mNext = 0;
		mIndex = 0;
	}

	bool isFull(void) const
	{
		return (bool)( mIndex == MAX_BUNDLE_SIZE );
	}

	KdTreeNode * getNextNode(void)
	{
		assert(mIndex<MAX_BUNDLE_SIZE);
		KdTreeNode *ret = &mNodes[mIndex];
		mIndex++;
		return ret;
	}

	KdTreeNodeBundle  *mNext;
	uint32_t             mIndex;
	KdTreeNode         mNodes[MAX_BUNDLE_SIZE];
};


typedef hacd::vector< double > DoubleVector;
typedef hacd::vector< float >  FloatVector;

class KdTree : public KdTreeInterface, public UANS::UserAllocated
{
public:
	KdTree(void)
	{
		mRoot = 0;
		mBundle = 0;
		mVcount = 0;
		mUseDouble = false;
	}

	virtual ~KdTree(void)
	{
		reset();
	}

	const double * getPositionDouble(uint32_t index) const
	{
		assert( mUseDouble );
		assert ( index < mVcount );
		return  &mVerticesDouble[index*3];
	}

	const float * getPositionFloat(uint32_t index) const
	{
		assert( !mUseDouble );
		assert ( index < mVcount );
		return  &mVerticesFloat[index*3];
	}

	uint32_t search(const double *pos,double radius,uint32_t maxObjects,KdTreeFindNode *found) const
	{
		assert( mUseDouble );
		if ( !mRoot )	return 0;
		uint32_t count = 0;
		mRoot->search(X_AXIS,pos,radius,count,maxObjects,found,this);
		return count;
	}

	uint32_t search(const float *pos,float radius,uint32_t maxObjects,KdTreeFindNode *found) const
	{
		assert( !mUseDouble );
		if ( !mRoot )	return 0;
		uint32_t count = 0;
		mRoot->search(X_AXIS,pos,radius,count,maxObjects,found,this);
		return count;
	}

	void reset(void)
	{
		mRoot = 0;
		mVerticesDouble.clear();
		mVerticesFloat.clear();
		KdTreeNodeBundle *bundle = mBundle;
		while ( bundle )
		{
			KdTreeNodeBundle *next = bundle->mNext;
			delete bundle;
			bundle = next;
		}
		mBundle = 0;
		mVcount = 0;
	}

	uint32_t add(double x,double y,double z)
	{
		assert(mUseDouble);
		uint32_t ret = mVcount;
		mVerticesDouble.push_back(x);
		mVerticesDouble.push_back(y);
		mVerticesDouble.push_back(z);
		mVcount++;
		KdTreeNode *node = getNewNode(ret);
		if ( mRoot )
		{
			mRoot->addDouble(node,X_AXIS,this);
		}
		else
		{
			mRoot = node;
		}
		return ret;
	}

	uint32_t add(float x,float y,float z)
	{
		assert(!mUseDouble);
		uint32_t ret = mVcount;
		mVerticesFloat.push_back(x);
		mVerticesFloat.push_back(y);
		mVerticesFloat.push_back(z);
		mVcount++;
		KdTreeNode *node = getNewNode(ret);
		if ( mRoot )
		{
			mRoot->addFloat(node,X_AXIS,this);
		}
		else
		{
			mRoot = node;
		}
		return ret;
	}

	KdTreeNode * getNewNode(uint32_t index)
	{
		if ( mBundle == 0 )
		{
			mBundle = HACD_NEW(KdTreeNodeBundle);
		}
		if ( mBundle->isFull() )
		{
			KdTreeNodeBundle *bundle = HACD_NEW(KdTreeNodeBundle);
			mBundle->mNext = bundle;
			mBundle = bundle;
		}
		KdTreeNode *node = mBundle->getNextNode();
		new ( node ) KdTreeNode(index);
		return node;
	}

	uint32_t getNearest(const double *pos,double radius,bool &_found) const // returns the nearest possible neighbor's index.
	{
		assert( mUseDouble );
		uint32_t ret = 0;

		_found = false;
		KdTreeFindNode found[1];
		uint32_t count = search(pos,radius,1,found);
		if ( count )
		{
			KdTreeNode *node = found[0].mNode;
			ret = node->getIndex();
			_found = true;
		}
		return ret;
	}

	uint32_t getNearest(const float *pos,float radius,bool &_found) const // returns the nearest possible neighbor's index.
	{
		assert( !mUseDouble );
		uint32_t ret = 0;

		_found = false;
		KdTreeFindNode found[1];
		uint32_t count = search(pos,radius,1,found);
		if ( count )
		{
			KdTreeNode *node = found[0].mNode;
			ret = node->getIndex();
			_found = true;
		}
		return ret;
	}

	const double * getVerticesDouble(void) const
	{
		assert( mUseDouble );
		const double *ret = 0;
		if ( !mVerticesDouble.empty() )
		{
			ret = &mVerticesDouble[0];
		}
		return ret;
	}

	const float * getVerticesFloat(void) const
	{
		assert( !mUseDouble );
		const float * ret = 0;
		if ( !mVerticesFloat.empty() )
		{
			ret = &mVerticesFloat[0];
		}
		return ret;
	}

	uint32_t getVcount(void) const { return mVcount; };

	void setUseDouble(bool useDouble)
	{
		mUseDouble = useDouble;
	}

private:
	bool                    mUseDouble;
	KdTreeNode             *mRoot;
	KdTreeNodeBundle       *mBundle;
	uint32_t                  mVcount;
	DoubleVector            mVerticesDouble;
	FloatVector             mVerticesFloat;
};

class MyVertexIndex : public fm_VertexIndex, public UANS::UserAllocated
{
public:
	MyVertexIndex(double granularity,bool snapToGrid)
	{
		mDoubleGranularity = granularity;
		mFloatGranularity  = (float)granularity;
		mSnapToGrid        = snapToGrid;
		mUseDouble         = true;
		mKdTree.setUseDouble(true);
	}

	MyVertexIndex(float granularity,bool snapToGrid)
	{
		mDoubleGranularity = granularity;
		mFloatGranularity  = (float)granularity;
		mSnapToGrid        = snapToGrid;
		mUseDouble         = false;
		mKdTree.setUseDouble(false);
	}

	virtual ~MyVertexIndex(void)
	{

	}


	double snapToGrid(double p)
	{
		double m = fmod(p,mDoubleGranularity);
		p-=m;
		return p;
	}

	float snapToGrid(float p)
	{
		float m = fmodf(p,mFloatGranularity);
		p-=m;
		return p;
	}

	uint32_t    getIndex(const float *_p,bool &newPos)  // get index for a vector float
	{
		uint32_t ret;

		if ( mUseDouble )
		{
			double p[3];
			p[0] = _p[0];
			p[1] = _p[1];
			p[2] = _p[2];
			return getIndex(p,newPos);
		}

		newPos = false;

		float p[3];

		if ( mSnapToGrid )
		{
			p[0] = snapToGrid(_p[0]);
			p[1] = snapToGrid(_p[1]);
			p[2] = snapToGrid(_p[2]);
		}
		else
		{
			p[0] = _p[0];
			p[1] = _p[1];
			p[2] = _p[2];
		}

		bool found;
		ret = mKdTree.getNearest(p,mFloatGranularity,found);
		if ( !found )
		{
			newPos = true;
			ret = mKdTree.add(p[0],p[1],p[2]);
		}


		return ret;
	}

	uint32_t    getIndex(const double *_p,bool &newPos)  // get index for a vector double
	{
		uint32_t ret;

		if ( !mUseDouble )
		{
			float p[3];
			p[0] = (float)_p[0];
			p[1] = (float)_p[1];
			p[2] = (float)_p[2];
			return getIndex(p,newPos);
		}

		newPos = false;

		double p[3];

		if ( mSnapToGrid )
		{
			p[0] = snapToGrid(_p[0]);
			p[1] = snapToGrid(_p[1]);
			p[2] = snapToGrid(_p[2]);
		}
		else
		{
			p[0] = _p[0];
			p[1] = _p[1];
			p[2] = _p[2];
		}

		bool found;
		ret = mKdTree.getNearest(p,mDoubleGranularity,found);
		if ( !found )
		{
			newPos = true;
			ret = mKdTree.add(p[0],p[1],p[2]);
		}


		return ret;
	}

	const float *   getVerticesFloat(void) const
	{
		const float * ret = 0;

		assert( !mUseDouble );

		ret = mKdTree.getVerticesFloat();

		return ret;
	}

	const double *  getVerticesDouble(void) const
	{
		const double * ret = 0;

		assert( mUseDouble );

		ret = mKdTree.getVerticesDouble();

		return ret;
	}

	const float *   getVertexFloat(uint32_t index) const
	{
		const float * ret  = 0;
		assert( !mUseDouble );
#ifdef _DEBUG
		uint32_t vcount = mKdTree.getVcount();
		assert( index < vcount );
#endif
		ret =  mKdTree.getVerticesFloat();
		ret = &ret[index*3];
		return ret;
	}

	const double *   getVertexDouble(uint32_t index) const
	{
		const double * ret = 0;
		assert( mUseDouble );
#ifdef _DEBUG
		uint32_t vcount = mKdTree.getVcount();
		assert( index < vcount );
#endif
		ret =  mKdTree.getVerticesDouble();
		ret = &ret[index*3];

		return ret;
	}

	uint32_t    getVcount(void) const
	{
		return mKdTree.getVcount();
	}

	bool isDouble(void) const
	{
		return mUseDouble;
	}


	bool            saveAsObj(const char *fname,uint32_t tcount,uint32_t *indices)
	{
		bool ret = false;


		FILE *fph = fopen(fname,"wb");
		if ( fph )
		{
			ret = true;

			uint32_t vcount    = getVcount();
			if ( mUseDouble )
			{
				const double *v  = getVerticesDouble();
				for (uint32_t i=0; i<vcount; i++)
				{
					fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", (float)v[0], (float)v[1], (float)v[2] );
					v+=3;
				}
			}
			else
			{
				const float *v  = getVerticesFloat();
				for (uint32_t i=0; i<vcount; i++)
				{
					fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", v[0], v[1], v[2] );
					v+=3;
				}
			}

			for (uint32_t i=0; i<tcount; i++)
			{
				uint32_t i1 = *indices++;
				uint32_t i2 = *indices++;
				uint32_t i3 = *indices++;
				fprintf(fph,"f %d %d %d\r\n", i1+1, i2+1, i3+1 );
			}
			fclose(fph);
		}

		return ret;
	}

private:
	bool    mUseDouble:1;
	bool    mSnapToGrid:1;
	double  mDoubleGranularity;
	float   mFloatGranularity;
	KdTree  mKdTree;
};

static fm_VertexIndex * fm_createVertexIndex(float granularity,bool snapToGrid)  // create an indexed vertext system for floats
{
	MyVertexIndex *ret = HACD_NEW(MyVertexIndex)(granularity,snapToGrid);
	return static_cast< fm_VertexIndex *>(ret);
}

static void          fm_releaseVertexIndex(fm_VertexIndex *vindex)
{
	MyVertexIndex *m = static_cast< MyVertexIndex *>(vindex);
	delete m;
}


//

// Split Mesh

class SimpleMesh
{
public:
	SimpleMesh(void)
	{
		mVcount = 0;
		mTcount = 0;
		mVertices = NULL;
		mIndices = NULL;
	}
	SimpleMesh(uint32_t vcount,uint32_t tcount,const float *vertices,const uint32_t *indices)
	{
		mVcount = 0;
		mTcount = 0;
		mVertices = NULL;
		mIndices = NULL;
		set(vcount,tcount,vertices,indices);
	}

	void set(uint32_t vcount,uint32_t tcount,const float *vertices,const uint32_t *indices)
	{
		release();
		mVcount = vcount;
		mTcount = tcount;
		mVertices = (float *)HACD_ALLOC(sizeof(float)*3*mVcount);
		memcpy(mVertices,vertices,sizeof(float)*3*mVcount);
		mIndices = (uint32_t *)HACD_ALLOC(sizeof(uint32_t)*3*mTcount);
		memcpy(mIndices,indices,sizeof(uint32_t)*3*mTcount);
	}


	~SimpleMesh(void)
	{
		release();
	}

	void release(void)
	{
		HACD_FREE(mVertices);
		HACD_FREE(mIndices);
		mVertices = NULL;
		mIndices = NULL;
		mVcount = 0;
		mTcount = 0;
	}


	uint32_t	mVcount;
	uint32_t	mTcount;
	float	*mVertices;
	uint32_t	*mIndices;
};


void splitMesh(const float *planeEquation,const SimpleMesh &input,SimpleMesh &left,SimpleMesh &right,bool closedMesh);

static void addTri(const float *p1,
				   const float *p2,
				   const float *p3,
				   hacd::vector< uint32_t > &indices,
				   fm_VertexIndex *vertices)
{
	bool newPos;

	uint32_t i1 = vertices->getIndex(p1,newPos);
	uint32_t i2 = vertices->getIndex(p2,newPos);
	uint32_t i3 = vertices->getIndex(p3,newPos);

	indices.push_back(i1);
	indices.push_back(i2);
	indices.push_back(i3);
}

enum PlaneTriResult
{
	PTR_ON_PLANE,
	PTR_FRONT,
	PTR_BACK,
	PTR_SPLIT,
};

static PlaneTriResult fm_getSidePlane(const float *p,const float *plane,float epsilon)
{
	PlaneTriResult ret = PTR_ON_PLANE;

	float d = fm_distToPlane(plane,p);

	if ( d < -epsilon || d > epsilon )
	{
		if ( d > 0 )
			ret =  PTR_FRONT; // it is 'in front' within the provided epsilon value.
		else
			ret = PTR_BACK;
	}

	return ret;
}




static PlaneTriResult fm_planeTriIntersection(const float plane[4],    // the plane equation in Ax+By+Cz+D format
									   const float *triangle, // the source triangle.
									   uint32_t tstride,  // stride in bytes of the input and output *vertices*
									   float        epsilon,  // the co-planer epsilon value.
									   float       *front,    // the triangle in front of the
									   uint32_t &fcount,  // number of vertices in the 'front' triangle
									   float       *back,     // the triangle in back of the plane
									   uint32_t &bcount); // the number of vertices in the 'back' triangle.

static inline void add(const float *p,float *dest,uint32_t tstride,uint32_t &pcount)
{
	char *d = (char *) dest;
	d = d + pcount*tstride;
	dest = (float *) d;
	dest[0] = p[0];
	dest[1] = p[1];
	dest[2] = p[2];
	pcount++;
	HACD_ASSERT( pcount <= 4 );
}

#define MAXPTS 256

template <class Type> class point
{
public:

	void set(const Type *p)
	{
		x = p[0];
		y = p[1];
		z = p[2];
	}

	Type x;
	Type y;
	Type z;
};

template <class Type> class plane
{
public:
	plane(const Type *p)
	{
		normal.x = p[0];
		normal.y = p[1];
		normal.z = p[2];
		D        = p[3];
	}

	Type Classify_Point(const point<Type> &p)
	{
		return p.x*normal.x + p.y*normal.y + p.z*normal.z + D;
	}

	point<Type> normal;
	Type  D;
};

template <class Type> class polygon
{
public:
	polygon(void)
	{
		mVcount = 0;
	}

	polygon(const Type *p1,const Type *p2,const Type *p3)
	{
		mVcount = 3;
		mVertices[0].set(p1);
		mVertices[1].set(p2);
		mVertices[2].set(p3);
	}


	int32_t NumVertices(void) const { return mVcount; };

	const point<Type>& Vertex(int32_t index)
	{
		if ( index < 0 ) index+=mVcount;
		return mVertices[index];
	};


	void set(const point<Type> *pts,int32_t count)
	{
		for (int32_t i=0; i<count; i++)
		{
			mVertices[i] = pts[i];
		}
		mVcount = count;
	}


	void Split_Polygon(polygon<Type> *poly,plane<Type> *part, polygon<Type> &front, polygon<Type> &back)
	{
		int32_t   count = poly->NumVertices ();
		int32_t   out_c = 0, in_c = 0;
		point<Type> ptA, ptB,outpts[MAXPTS],inpts[MAXPTS];
		Type sideA, sideB;
		ptA = poly->Vertex (count - 1);
		sideA = part->Classify_Point (ptA);
		for (int32_t i = -1; ++i < count;)
		{
			ptB = poly->Vertex(i);
			sideB = part->Classify_Point(ptB);
			if (sideB > 0)
			{
				if (sideA < 0)
				{
					point<Type> v;
					fm_intersectPointPlane(&ptB.x, &ptA.x, &v.x, &part->normal.x );
					outpts[out_c++] = inpts[in_c++] = v;
				}
				outpts[out_c++] = ptB;
			}
			else if (sideB < 0)
			{
				if (sideA > 0)
				{
					point<Type> v;
					fm_intersectPointPlane(&ptB.x, &ptA.x, &v.x, &part->normal.x );
					outpts[out_c++] = inpts[in_c++] = v;
				}
				inpts[in_c++] = ptB;
			}
			else
				outpts[out_c++] = inpts[in_c++] = ptB;
			ptA = ptB;
			sideA = sideB;
		}

		front.set(&outpts[0], out_c);
		back.set(&inpts[0], in_c);
	}

	int32_t           mVcount;
	point<Type>   mVertices[MAXPTS];
};
/* a = b - c */
#define vector(a,b,c) \
	(a)[0] = (b)[0] - (c)[0];	\
	(a)[1] = (b)[1] - (c)[1];	\
	(a)[2] = (b)[2] - (c)[2];



#define innerProduct(v,q) \
	((v)[0] * (q)[0] + \
	(v)[1] * (q)[1] + \
	(v)[2] * (q)[2])

#define crossProduct(a,b,c) \
	(a)[0] = (b)[1] * (c)[2] - (c)[1] * (b)[2]; \
	(a)[1] = (b)[2] * (c)[0] - (c)[2] * (b)[0]; \
	(a)[2] = (b)[0] * (c)[1] - (c)[0] * (b)[1];


static bool fm_rayIntersectsTriangle(const float *p,const float *d,const float *v0,const float *v1,const float *v2,float &t)
{
	float e1[3],e2[3],h[3],s[3],q[3];
	float a,f,u,v;

	vector(e1,v1,v0);
	vector(e2,v2,v0);
	crossProduct(h,d,e2);
	a = innerProduct(e1,h);

	if (a > -0.00001 && a < 0.00001)
		return(false);

	f = 1/a;
	vector(s,p,v0);
	u = f * (innerProduct(s,h));

	if (u < 0.0 || u > 1.0)
		return(false);

	crossProduct(q,s,e1);
	v = f * innerProduct(d,q);
	if (v < 0.0 || u + v > 1.0)
		return(false);
	// at this stage we can compute t to find out where
	// the intersection point is on the line
	t = f * innerProduct(e2,q);
	if (t > 0) // ray intersection
		return(true);
	else // this means that there is a line intersection
		// but not a ray intersection
		return (false);
}



static PlaneTriResult fm_planeTriIntersection(const float *_plane,    // the plane equation in Ax+By+Cz+D format
									   const float *triangle, // the source triangle.
									   uint32_t tstride,  // stride in bytes of the input and output *vertices*
									   float        epsilon,  // the co-planar epsilon value.
									   float       *front,    // the triangle in front of the
									   uint32_t &fcount,  // number of vertices in the 'front' triangle
									   float       *back,     // the triangle in back of the plane
									   uint32_t &bcount) // the number of vertices in the 'back' triangle.
{

	fcount = 0;
	bcount = 0;

	const char *tsource = (const char *) triangle;

	// get the three vertices of the triangle.
	const float *p1     = (const float *) (tsource);
	const float *p2     = (const float *) (tsource+tstride);
	const float *p3     = (const float *) (tsource+tstride*2);


	PlaneTriResult r1   = fm_getSidePlane(p1,_plane,epsilon); // compute the side of the plane each vertex is on
	PlaneTriResult r2   = fm_getSidePlane(p2,_plane,epsilon);
	PlaneTriResult r3   = fm_getSidePlane(p3,_plane,epsilon);

	// If any of the points lay right *on* the plane....
	if ( r1 == PTR_ON_PLANE || r2 == PTR_ON_PLANE || r3 == PTR_ON_PLANE )
	{
		// If the triangle is completely co-planar, then just treat it as 'front' and return!
		if ( r1 == PTR_ON_PLANE && r2 == PTR_ON_PLANE && r3 == PTR_ON_PLANE )
		{
			add(p1,front,tstride,fcount);
			add(p2,front,tstride,fcount);
			add(p3,front,tstride,fcount);
			return PTR_FRONT;
		}
		// Decide to place the co-planar points on the same side as the co-planar point.
		PlaneTriResult r= PTR_ON_PLANE;
		if ( r1 != PTR_ON_PLANE )
			r = r1;
		else if ( r2 != PTR_ON_PLANE )
			r = r2;
		else if ( r3 != PTR_ON_PLANE )
			r = r3;

		if ( r1 == PTR_ON_PLANE ) r1 = r;
		if ( r2 == PTR_ON_PLANE ) r2 = r;
		if ( r3 == PTR_ON_PLANE ) r3 = r;

	}

	if ( r1 == r2 && r1 == r3 ) // if all three vertices are on the same side of the plane.
	{
		if ( r1 == PTR_FRONT ) // if all three are in front of the plane, then copy to the 'front' output triangle.
		{
			add(p1,front,tstride,fcount);
			add(p2,front,tstride,fcount);
			add(p3,front,tstride,fcount);
		}
		else
		{
			add(p1,back,tstride,bcount); // if all three are in 'back' then copy to the 'back' output triangle.
			add(p2,back,tstride,bcount);
			add(p3,back,tstride,bcount);
		}
		return r1; // if all three points are on the same side of the plane return result
	}


	polygon<float> pi(p1,p2,p3);
	polygon<float>  pfront,pback;

	plane<float>    part(_plane);

	pi.Split_Polygon(&pi,&part,pfront,pback);

	for (int32_t i=0; i<pfront.mVcount; i++)
	{
		add( &pfront.mVertices[i].x, front, tstride, fcount );
	}

	for (int32_t i=0; i<pback.mVcount; i++)
	{
		add( &pback.mVertices[i].x, back, tstride, bcount );
	}

	PlaneTriResult ret = PTR_SPLIT;

	if ( fcount < 3 ) fcount = 0;
	if ( bcount < 3 ) bcount = 0;

	if ( fcount == 0 && bcount )
		ret = PTR_BACK;

	if ( bcount == 0 && fcount )
		ret = PTR_FRONT;


	return ret;
}



void splitMesh(const float *planeEquation,const SimpleMesh &input,SimpleMesh &leftMesh,SimpleMesh &rightMesh)
{
	hacd::vector< uint32_t > leftIndices;
	hacd::vector< uint32_t > rightIndices;

	fm_VertexIndex *leftVertices = fm_createVertexIndex(0.00001f,false);
	fm_VertexIndex *rightVertices = fm_createVertexIndex(0.00001f,false);

	{
		for (uint32_t i=0; i<input.mTcount; i++)
		{
			uint32_t i1 = input.mIndices[i*3+0];
			uint32_t i2 = input.mIndices[i*3+1];
			uint32_t i3 = input.mIndices[i*3+2];

			float *p1 = &input.mVertices[i1*3];
			float *p2 = &input.mVertices[i2*3];
			float *p3 = &input.mVertices[i3*3];

			float tri[3*3];

			tri[0] = p1[0];
			tri[1] = p1[1];
			tri[2] = p1[2];

			tri[3] = p2[0];
			tri[4] = p2[1];
			tri[5] = p2[2];

			tri[6] = p3[0];
			tri[7] = p3[1];
			tri[8] = p3[2];

			float	front[3*5];
			float	back[3*5];

			uint32_t fcount,bcount;

			PlaneTriResult result = fm_planeTriIntersection(planeEquation,tri,sizeof(float)*3,0.00001f,front,fcount,back,bcount);

			switch ( result )
			{
			case PTR_FRONT:
				addTri(p1,p2,p3,leftIndices,leftVertices);
				break;
			case PTR_BACK:
				addTri(p1,p2,p3,rightIndices,rightVertices);
				break;
			case PTR_SPLIT:
				if ( fcount )
				{
					addTri(&front[0],&front[3],&front[6],leftIndices,leftVertices);
					if ( fcount == 4 )
					{
						addTri(&front[0],&front[6],&front[9],leftIndices,leftVertices);
					}
				}
				if ( bcount )
				{
					addTri(&back[0],&back[3],&back[6],rightIndices,rightVertices);
					if ( bcount == 4 )
					{
						addTri(&back[0],&back[6],&back[9],rightIndices,rightVertices);
					}
				}
				break;
			case PTR_ON_PLANE: // Make compiler happy
				break;
			}
		}
	}

	if ( !leftIndices.empty() )
	{
		leftMesh.set(leftVertices->getVcount(),leftIndices.size()/3,leftVertices->getVerticesFloat(),&leftIndices[0]);
	}

	if ( !rightIndices.empty() )
	{
		rightMesh.set(rightVertices->getVcount(),rightIndices.size()/3,rightVertices->getVerticesFloat(),&rightIndices[0]);
	}
	fm_releaseVertexIndex(leftVertices);
	fm_releaseVertexIndex(rightVertices);
}


//

static float enorm0_3d ( float x0, float y0, float z0, float x1, float y1, float z1 )

/**********************************************************************/

/*
Purpose:

ENORM0_3D computes the Euclidean norm of (P1-P0) in 3D.

Modified:

18 April 1999

Author:

John Burkardt

Parameters:

Input, float X0, Y0, Z0, X1, Y1, Z1, the coordinates of the points 
P0 and P1.

Output, float ENORM0_3D, the Euclidean norm of (P1-P0).
*/
{
  float value;

  value = ::sqrtf (
    ( x1 - x0 ) * ( x1 - x0 ) + 
    ( y1 - y0 ) * ( y1 - y0 ) + 
    ( z1 - z0 ) * ( z1 - z0 ) );

  return value;
}


static float triangle_area_3d ( float x1, float y1, float z1, float x2,float y2, float z2, float x3, float y3, float z3 )

                        /**********************************************************************/

                        /*
                        Purpose:

                        TRIANGLE_AREA_3D computes the area of a triangle in 3D.

                        Modified:

                        22 April 1999

                        Author:

                        John Burkardt

                        Parameters:

                        Input, float X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3, the (X,Y,Z)
                        coordinates of the corners of the triangle.

                        Output, float TRIANGLE_AREA_3D, the area of the triangle.
                        */
{
  float a;
  float alpha;
  float area;
  float b;
  float base;
  float c;
  float dot;
  float height;
  /*
  Find the projection of (P3-P1) onto (P2-P1).
  */
  dot = 
    ( x2 - x1 ) * ( x3 - x1 ) +
    ( y2 - y1 ) * ( y3 - y1 ) +
    ( z2 - z1 ) * ( z3 - z1 );

  base = enorm0_3d ( x1, y1, z1, x2, y2, z2 );
  /*
  The height of the triangle is the length of (P3-P1) after its
  projection onto (P2-P1) has been subtracted.
  */
  if ( base == 0.0 ) {

    height = 0.0;

  }
  else {

    alpha = dot / ( base * base );

    a = x3 - x1 - alpha * ( x2 - x1 );
    b = y3 - y1 - alpha * ( y2 - y1 );
    c = z3 - z1 - alpha * ( z2 - z1 );

    height = ::sqrtf ( a * a + b * b + c * c );

  }

  area = 0.5f * base * height;

  return area;
}


float fm_computeArea(const float *p1,const float *p2,const float *p3)
{
  float ret = 0;

  ret = triangle_area_3d(p1[0],p1[1],p1[2],p2[0],p2[1],p2[2],p3[0],p3[1],p3[2]);

  return ret;
}

void validate(float v)
{
	HACD_UNUSED(v);
	HACD_ASSERT( v >= -1000 && v < 1000 );
}

void validate(const float *v)
{
	validate(v[0]);
	validate(v[1]);
	validate(v[2]);
}


void addVertex(const float *p,float *dest,uint32_t index)
{
	dest[index*3+0] = p[0];
	dest[index*3+1] = p[1];
	dest[index*3+2] = p[2];

	validate( &dest[index*3]);

}

void addTriangle(uint32_t *indices,uint32_t i1,uint32_t i2,uint32_t i3,uint32_t index)
{
	indices[index*3+0] = i1;
	indices[index*3+1] = i2;
	indices[index*3+2] = i3;
}

bool projectRay(const float *p,const float *n,float *t,const HACD::HullResult &hull)
{
	bool ret = false;

	t[0] = p[0];
	t[1] = p[1];
	t[2] = p[2];
	validate(p);
	validate(n);

	for (uint32_t i=0; i<hull.mNumTriangles; i++)
	{
		uint32_t i1 = hull.mIndices[i*3+0];
		uint32_t i2 = hull.mIndices[i*3+1];
		uint32_t i3 = hull.mIndices[i*3+2];

		const float *p1 = &hull.mOutputVertices[i1*3];
		const float *p2 = &hull.mOutputVertices[i2*3];
		const float *p3 = &hull.mOutputVertices[i3*3];

		float tm;
		if ( fm_rayIntersectsTriangle(p,n,p1,p2,p3,tm))
		{
			if ( tm > 100 )
			{
				fm_rayIntersectsTriangle(p,n,p1,p2,p3,tm);
			}
			t[0] = p[0]+n[0]*tm;
			t[1] = p[1]+n[1]*tm;
			t[2] = p[2]+n[2]*tm;
			ret = true;
			break;
		}
	}

	if ( ret )
	{
		validate(t);
	}

	return ret;
}

float computeProjectedVolume(const float *p1,const float *p2,const float *p3,const HACD::HullResult &hull)
{
	float ret = 0;

	float area = fm_computeArea(p1,p2,p3);
	if ( area <= 0 ) 
	{
		return 0;
	}

	float normal[3];
	fm_computePlane(p3,p2,p1,normal);

	float t1[3];
	float t2[3];
	float t3[3];

	bool hit1 = projectRay(p1,normal,t1,hull);
	bool hit2 = projectRay(p2,normal,t2,hull);
	bool hit3 = projectRay(p3,normal,t3,hull);

	if ( hit1 || hit2 || hit3 )
	{
		// now we build the little triangle mesh piece...
		uint32_t indices[8*3];

		float vertices[6*3];
		addVertex(p1,vertices,0);
		addVertex(p2,vertices,1);
		addVertex(p3,vertices,2);
		addVertex(t1,vertices,3);
		addVertex(t2,vertices,4);
		addVertex(t3,vertices,5);

		addTriangle(indices,2,1,0,0);
		addTriangle(indices,3,4,5,1);

		addTriangle(indices,0,3,4,2);
		addTriangle(indices,0,4,1,3);

		addTriangle(indices,2,5,3,4);
		addTriangle(indices,2,3,0,5);

		addTriangle(indices,1,4,5,6);
		addTriangle(indices,1,5,2,7);

		ret = fm_computeMeshVolume(vertices,8,indices);

#if 0
		static FILE *fph = fopen("project.obj", "wb" );
		static int baseVertex = 1;
		for (int i=0; i<6; i++)
		{
			fprintf(fph,"v %0.9f %0.9f %0.9f\r\n", vertices[i*3+0], vertices[i*3+1], vertices[i*3+2] );
		}
		for (int i=0; i<8; i++)
		{
			fprintf(fph,"f %d %d %d\r\n", indices[i*3+0]+baseVertex, indices[i*3+1]+baseVertex, indices[i*3+2]+baseVertex );
		}
		fflush(fph);
		baseVertex+=6;
#endif
	}

	return ret;
}
float computeConcavityVolume(uint32_t /*vcount*/,
						   const float *vertices,
						   uint32_t tcount,
						   const uint32_t *indices,
						   const HACD::HullResult &result)
{
	float ret = 0;

	for (uint32_t i=0; i<tcount; i++)
	{
		uint32_t i1 = indices[i*3+0];
		uint32_t i2 = indices[i*3+1];
		uint32_t i3 = indices[i*3+2];

		const float *p1 = &vertices[i1*3];
		const float *p2 = &vertices[i2*3];
		const float *p3 = &vertices[i3*3];

		ret+=computeProjectedVolume(p1,p2,p3,result);

	}

	return ret;
}

//


typedef hacd::vector< ConvexResult > ConvexResultVector;

class ConvexBuilder : public ConvexDecompInterface, public ConvexDecomposition, public UANS::UserAllocated
{
public:
	ConvexBuilder(void)
	{
	};

	virtual ~ConvexBuilder(void)
	{
		for (uint32_t i=0; i<mResults.size(); i++)
		{
			ConvexResult &r = mResults[i];
			HACD_FREE(r.mHullIndices);
			HACD_FREE(r.mHullVertices);
		}
	}

	virtual void ConvexDecompResult(const ConvexResult &result)
	{
		ConvexResult r;
		r.mHullTcount = result.mHullTcount;
		r.mHullVcount = result.mHullVcount;
		r.mHullIndices = (uint32_t *)HACD_ALLOC(sizeof(uint32_t)*r.mHullTcount*3);
		memcpy(r.mHullIndices,result.mHullIndices,sizeof(uint32_t)*r.mHullTcount*3);
		r.mHullVertices = (float *)HACD_ALLOC(sizeof(float)*r.mHullVcount*3);
		memcpy(r.mHullVertices,result.mHullVertices,sizeof(float)*r.mHullVcount*3);
		mResults.push_back(r);
	}

void doConvexDecomposition(uint32_t vcount,
							const float *vertices,
							uint32_t tcount,
							const uint32_t *indices,
							Cdesc &cdesc,
							uint32_t depth)
{

	// first see if the input mesh is co-planar.
	// If it is, then we return because we can't do anything with a co-planer mesh
	bool isCoplanar = fm_isMeshCoplanar(tcount,indices,vertices,true);
	if ( isCoplanar ) return;


	// Next build a convex hull for the input vertices for this mesh fragment
	HACD::HullResult result;
	HACD::HullLibrary hl;
	HACD::HullDesc   desc;
	desc.mVcount = vcount;
	desc.mVertices = vertices;
	desc.mVertexStride = sizeof(float)*3;
	HACD::HullError ret = hl.CreateConvexHull(desc,result);
	if ( ret != HACD::QE_OK )
	{
		return; // unable to build a hull for this remaining piece of mesh; so return.
	}

	bool split = false;
	if ( depth < cdesc.mMaxDepth ) // if have not reached the maximum depth
	{
		// compute the volume of the convex hull prior to the plist.
		float hullVolume = fm_computeMeshVolume(result.mOutputVertices,result.mNumTriangles,result.mIndices);
		if (depth == 0 )
		{
			cdesc.mMasterVolume = hullVolume;
		}
		float percent = (hullVolume*100)/cdesc.mMasterVolume;
		// if this convex hull is still considered significant enough in size to keep splitting...
		if ( percent > cdesc.mMeshVolumePercent ) // if not too small of a feature...
		{
			// find the split plane by computing the OBB and slicing in half
			float plane[4];
			split = fm_computeSplitPlane(result.mNumOutputVertices,result.mOutputVertices,result.mNumTriangles,result.mIndices,plane);
			if ( split )
			{
				{
					float concaveVolume = computeConcavityVolume(vcount,vertices,tcount,indices,result);
					float percentVolume = concaveVolume*100 / hullVolume;

					if ( percentVolume < cdesc.mConcavePercent )
					{
						split = false;
					}
				}

				SimpleMesh mesh(vcount, tcount, vertices, indices);
				SimpleMesh leftMesh;
				SimpleMesh rightMesh;
				splitMesh(plane,mesh,leftMesh,rightMesh);

				if ( split )
				{

					if ( leftMesh.mTcount )
					{
						doConvexDecomposition(leftMesh.mVcount, leftMesh.mVertices, leftMesh.mTcount,leftMesh.mIndices,cdesc,depth+1);
					}
					if ( rightMesh.mTcount )
					{
						doConvexDecomposition(rightMesh.mVcount, rightMesh.mVertices, rightMesh.mTcount,rightMesh.mIndices, cdesc, depth+1);
					}
				}
			}
		}
	}
	if ( !split )
	{
		ConvexResult r;
		r.mHullIndices = result.mIndices;
		r.mHullVertices = result.mOutputVertices;
		r.mHullTcount = result.mNumTriangles;
		r.mHullVcount = result.mNumOutputVertices;
		cdesc.mCallback->ConvexDecompResult(r);
		hl.ReleaseResult(result); // do not release the result!

		if ( cdesc.mICallback )
		{
			float progress = (float)cdesc.mOutputCount / (float)cdesc.mOutputPow2;
			cdesc.mOutputCount++;
			cdesc.mICallback->ReportProgress("SplittingMesh", progress );
		}


	}
}

	virtual uint32_t performConvexDecomposition(const DecompDesc &desc) // returns the number of hulls produced.
	{
		Cdesc cdesc;
		cdesc.mMaxDepth			= desc.mDepth;
		cdesc.mConcavePercent	= desc.mCpercent;
		cdesc.mMeshVolumePercent= desc.mMeshVolumePercent;
		cdesc.mCallback			= this;
		cdesc.mICallback		= desc.mCallback;
		cdesc.mOutputCount = 0;
		uint32_t p2[17] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
		if ( cdesc.mMaxDepth > 10 )
			cdesc.mMaxDepth = 10;
		cdesc.mOutputPow2 = p2[ cdesc.mMaxDepth];
		doConvexDecomposition(desc.mVcount, desc.mVertices, desc.mTcount, desc.mIndices, cdesc, 0);
		return mResults.size();
	}

  virtual void release(void)
  {
	  delete this;
  }

  virtual ConvexResult * getConvexResult(uint32_t index,bool takeMemoryOwnership)
  {
	  ConvexResult *ret = NULL;
	  if ( index < mResults.size() )
	  {
		  ret = &mResults[index];
		  if ( takeMemoryOwnership )
		  {
			  mTempResult = *ret;
			  ret->mHullIndices = NULL;
			  ret->mHullVertices = NULL;
			  ret = &mTempResult;
		  }
	  }
	  return ret;
  }

	ConvexResult		mTempResult;
	ConvexResultVector	mResults;
};

ConvexDecomposition * createConvexDecomposition(void)
{
	ConvexBuilder *m = HACD_NEW(ConvexBuilder);
	return static_cast<ConvexDecomposition *>(m);
}


}; // end of namespace

