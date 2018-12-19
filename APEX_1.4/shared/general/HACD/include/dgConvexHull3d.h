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

#ifndef __DG_CONVEXHULL_3D__
#define __DG_CONVEXHULL_3D__

#include "dgTypes.h"
#include "dgList.h"
#include "dgArray.h"
#include "dgPlane.h"
#include "dgVector.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"

class dgAABBPointTree3d;

class dgConvexHull3DFace
{
	public:
	dgConvexHull3DFace();
	int32_t m_index[3]; 
	
	void SetMark(int32_t mark) 
	{
		m_mark = mark;
	}

	int32_t GetMark() const 
	{
		return m_mark;
	}
	dgList<dgConvexHull3DFace>::dgListNode* GetTwin(int32_t index) const { return m_twin[index];}

	private:
	double Evalue (const dgBigVector* const pointArray, const dgBigVector& point) const;
	dgBigPlane GetPlaneEquation (const dgBigVector* const pointArray) const;

	int32_t m_mark;
	dgList<dgConvexHull3DFace>::dgListNode* m_twin[3];
	friend class dgConvexHull3d;
};

class dgHullVertex;

class dgConvexHull3d: public dgList<dgConvexHull3DFace>, public UANS::UserAllocated
{
	public:
	dgConvexHull3d(const dgConvexHull3d& source);
	dgConvexHull3d(const double* const vertexCloud, int32_t strideInBytes, int32_t count, double distTol, int32_t maxVertexCount = 0x7fffffff);
	virtual ~dgConvexHull3d();

	int32_t GetVertexCount() const;
	const dgBigVector* GetVertexPool() const;
	const dgBigVector& GetVertex(int32_t i) const;

	double GetDiagonal() const;
	void GetAABB (dgBigVector& boxP0, dgBigVector& boxP1) const;
	double RayCast (const dgBigVector& localP0, const dgBigVector& localP1) const;
	void CalculateVolumeAndSurfaceArea (double& volume, double& surcafeArea) const;

	protected:
	dgConvexHull3d(void);
	void BuildHull (const double* const vertexCloud, int32_t strideInBytes, int32_t count, double distTol, int32_t maxVertexCount);

	virtual dgListNode* AddFace (int32_t i0, int32_t i1, int32_t i2);
	virtual void DeleteFace (dgListNode* const node) ;
	virtual int32_t InitVertexArray(dgHullVertex* const points, const double* const vertexCloud, int32_t strideInBytes, int32_t count, void* const memoryPool, int32_t maxMemSize);

	void CalculateConvexHull (dgAABBPointTree3d* vertexTree, dgHullVertex* const points, int32_t count, double distTol, int32_t maxVertexCount);
	int32_t BuildNormalList (dgBigVector* const normalArray) const;
	int32_t SupportVertex (dgAABBPointTree3d** const tree, const dgHullVertex* const points, const dgBigVector& dir) const;
	double TetrahedrumVolume (const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3) const;
	void TessellateTriangle (int32_t level, const dgVector& p0, const dgVector& p1, const dgVector& p2, int32_t& count, dgBigVector* const ouput, int32_t& start) const;

	dgAABBPointTree3d* BuildTree (dgAABBPointTree3d* const parent, dgHullVertex* const points, int32_t count, int32_t baseIndex, int8_t** const memoryPool, int32_t& maxMemSize) const;
	static int32_t ConvexCompareVertex(const dgHullVertex* const  A, const dgHullVertex* const B, void* const context);
	bool Sanity() const;

	int32_t m_count;
	double m_diag;
	dgBigVector m_aabbP0;
	dgBigVector m_aabbP1;
	dgArray<dgBigVector> m_points;
};


inline int32_t dgConvexHull3d::GetVertexCount() const
{
	return m_count;
}

inline const dgBigVector* dgConvexHull3d::GetVertexPool() const
{
	return &m_points[0];
}

inline const dgBigVector& dgConvexHull3d::GetVertex(int32_t index) const
{
	return m_points[index];
}

inline double dgConvexHull3d::GetDiagonal() const
{
	return m_diag;
}


inline void dgConvexHull3d::GetAABB (dgBigVector& boxP0, dgBigVector& boxP1) const
{
	boxP0 = m_aabbP0;
	boxP1 = m_aabbP1;
}

#endif
