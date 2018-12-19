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
#ifndef __dgPolygonSoupDatabaseBuilder0x23413452233__
#define __dgPolygonSoupDatabaseBuilder0x23413452233__


#include "dgTypes.h"
#include "dgArray.h"
#include "dgIntersections.h"


class AdjacentdFaces
{
	public:
	int32_t m_count;
	int32_t *m_index;
	dgPlane m_normal;
	int64_t m_edgeMap[256];
};

class dgPolygonSoupDatabaseBuilder : public UANS::UserAllocated
{
	public:
	dgPolygonSoupDatabaseBuilder (void);
	~dgPolygonSoupDatabaseBuilder ();

	void Begin();
	void End(bool optimize);
	void AddMesh (const float* const vertex, int32_t vertexCount, int32_t strideInBytes, int32_t faceCount, 
		          const int32_t* const faceArray, const int32_t* const indexArray, const int32_t* const faceTagsData, const dgMatrix& worldMatrix); 

	void SingleFaceFixup();

	private:

	void Optimize(bool optimize);
	void EndAndOptimize(bool optimize);
	void OptimizeByGroupID();
	void OptimizeByIndividualFaces();
	int32_t FilterFace (int32_t count, int32_t* const indexArray);
	int32_t AddConvexFace (int32_t count, int32_t* const indexArray, int32_t* const  facesArray);
	void OptimizeByGroupID (dgPolygonSoupDatabaseBuilder& source, int32_t faceNumber, int32_t faceIndexNumber, dgPolygonSoupDatabaseBuilder& leftOver); 

	void PackArray();

//	void WriteDebugOutput (const char* name);

	public:
	struct VertexArray: public dgArray<dgBigVector>
	{
		VertexArray(void)
			:dgArray<dgBigVector>(1024 * 256)
		{
		}
	};

	struct IndexArray: public dgArray<int32_t>
	{
		IndexArray(void)
			:dgArray<int32_t>(1024 * 256)
		{
		}
	};

	int32_t m_run;
	int32_t m_faceCount;
	int32_t m_indexCount;
	int32_t m_vertexCount;
	int32_t m_normalCount;
	IndexArray m_faceVertexCount;
	IndexArray m_vertexIndex;
	IndexArray m_normalIndex;
	VertexArray	m_vertexPoints;
	VertexArray	m_normalPoints;
};





#endif

