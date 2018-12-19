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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#ifndef __dgPolygonSoupDatabase0x23413452233__
#define __dgPolygonSoupDatabase0x23413452233__


#include "dgTypes.h"
#include "dgArray.h"
#include "dgIntersections.h"

class dgMatrix;



class dgPolygonSoupDatabase//: public dgRef
{
	public:
	float GetRadius() const;
	int32_t GetVertexCount() const;
	int32_t GetStrideInBytes() const;
	float* GetLocalVertexPool() const;

	uint32_t GetTagId(const int32_t* face) const;
	void SetTagId(const int32_t* face, uint32_t newID) const;


	virtual void GetAABB (dgVector& p0, dgVector& p1) const;

	
	protected:
	virtual void ForAllSectors (const dgVector& min, const dgVector& max, dgAABBIntersectCallback callback, void* const context) const;
	virtual void ForAllSectorsSimd (const dgVector& min, const dgVector& max, dgAABBIntersectCallback callback, void* const context) const;
	virtual void ForAllSectorsRayHit (const dgFastRayTest& ray, dgRayIntersectCallback callback, void* const context) const;
	virtual void ForAllSectorsRayHitSimd (const dgFastRayTest& ray, dgRayIntersectCallback callback, void* const context) const;

	dgPolygonSoupDatabase(const char *name = NULL);
	virtual ~dgPolygonSoupDatabase ();

	int32_t m_vertexCount;
	int32_t m_strideInBytes;
	float* m_localVertex;
};


inline dgPolygonSoupDatabase::dgPolygonSoupDatabase(const char *name)
{
	HACD_FORCE_PARAMETER_REFERENCE(name);
	m_vertexCount = 0;
	m_strideInBytes = 0;
	m_localVertex = NULL;
}

inline dgPolygonSoupDatabase::~dgPolygonSoupDatabase ()
{
	if (m_localVertex) {
		HACD_FREE(m_localVertex);
	}
}


inline uint32_t dgPolygonSoupDatabase::GetTagId(const int32_t* face) const
{
	return uint32_t (face[-1]);
}

inline void dgPolygonSoupDatabase::SetTagId(const int32_t* facePtr, uint32_t newID) const
{
	uint32_t* face;
	face = (uint32_t*) facePtr;
	face[-1] = newID;
}

inline int32_t dgPolygonSoupDatabase::GetVertexCount()	const
{
	return m_vertexCount;
}

inline float* dgPolygonSoupDatabase::GetLocalVertexPool() const
{
	return m_localVertex;
}

inline int32_t dgPolygonSoupDatabase::GetStrideInBytes() const
{
	return m_strideInBytes;
}

inline float dgPolygonSoupDatabase::GetRadius() const
{
	return 0.0f;
}

inline void dgPolygonSoupDatabase::ForAllSectorsSimd (const dgVector& min, const dgVector& max, dgAABBIntersectCallback callback, void* const context) const
{
	HACD_FORCE_PARAMETER_REFERENCE(min);
	HACD_FORCE_PARAMETER_REFERENCE(max);
	HACD_FORCE_PARAMETER_REFERENCE(callback);
	HACD_FORCE_PARAMETER_REFERENCE(context);
	HACD_ALWAYS_ASSERT();
}



inline void dgPolygonSoupDatabase::ForAllSectors (const dgVector& min, const dgVector& max, dgAABBIntersectCallback callback, void* const context) const
{
	HACD_FORCE_PARAMETER_REFERENCE(min);
	HACD_FORCE_PARAMETER_REFERENCE(max);
	HACD_FORCE_PARAMETER_REFERENCE(callback);
	HACD_FORCE_PARAMETER_REFERENCE(context);
	HACD_ALWAYS_ASSERT();
}


inline void dgPolygonSoupDatabase::GetAABB (dgVector& p0, dgVector& p1) const
{
	HACD_FORCE_PARAMETER_REFERENCE(p0);
	HACD_FORCE_PARAMETER_REFERENCE(p1);
	HACD_ALWAYS_ASSERT();// not yet implemented!
}


inline void dgPolygonSoupDatabase::ForAllSectorsRayHit (const dgFastRayTest& /*ray*/, dgRayIntersectCallback /*callback*/, void* const /*context*/) const
{
	HACD_ALWAYS_ASSERT();
}

inline void dgPolygonSoupDatabase::ForAllSectorsRayHitSimd (const dgFastRayTest& /*ray*/, dgRayIntersectCallback /*callback*/, void* const /*context*/) const
{
	HACD_ALWAYS_ASSERT();
}


#endif

