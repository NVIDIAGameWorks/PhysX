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

#ifndef __dgMeshEffect_H__
#define __dgMeshEffect_H__

#include "dgTypes.h"
#include "dgRefCounter.h"
#include "dgPolyhedra.h"
#include "dgVector.h"
#include "dgPlane.h"
#include "dgMatrix.h"
#include "SparseArray.h"

class dgMeshEffect;
class dgMeshEffectSolidTree;
class dgMeshTreeCSGEdgePool;
class dgConvexHull3d;
class dgThreadHive;


#define DG_MESH_EFFECT_INITIAL_VERTEX_SIZE	8
#define DG_MESH_EFFECT_BOLLEAN_STACK		512
#define DG_MESH_EFFECT_POINT_SPLITED		512
#define DG_MESH_EFFECT_POLYGON_SPLITED		256
#define DG_MESH_EFFECT_FLAT_CUT_BORDER_EDGE	0x01
#define DG_VERTEXLIST_INDEXLIST_TOL			(double (0.0f))


#define DG_MESH_EFFECT_PRECISION_BITS		30
#define DG_MESH_EFFECT_PRECISION_SCALE		double(1<<DG_MESH_EFFECT_PRECISION_BITS)
#define DG_MESH_EFFECT_PRECISION_SCALE_INV	(double (1.0f) / DG_MESH_EFFECT_PRECISION_SCALE)


#define DG_MESG_EFFECT_BOOLEAN_INIT()					\
	dgMeshEffect* result = NULL;						\
	dgMeshEffect* sourceCoplanar = NULL;				\
	dgMeshEffect* leftMeshSource = NULL;				\
	dgMeshEffect* rightMeshSource = NULL;				\
	dgMeshEffect* clipperCoplanar = NULL;				\
	dgMeshEffect* leftMeshClipper = NULL;				\
	dgMeshEffect* rightMeshClipper = NULL;

#define DG_MESG_EFFECT_BOOLEAN_FINISH()					\
	if (sourceCoplanar) {								\
		sourceCoplanar->Release();						\
	}													\
	if (clipperCoplanar) {								\
		clipperCoplanar->Release();						\
	}													\
	if (leftMeshClipper) {								\
		leftMeshClipper->Release();						\
	}													\
	if (rightMeshClipper) {								\
		rightMeshClipper->Release();					\
	}													\
	if (leftMeshSource) {								\
		leftMeshSource->Release();						\
	}													\
	if (rightMeshSource) {								\
		rightMeshSource->Release();						\
	}													\
	if (result) {										\
		result->ConvertToPolygons();					\
		dgStack<int32_t> map(result->m_pointCount + 1);	\
		result->RemoveUnusedVertices(&map[0]);			\
	}													



class dgMeshEffect: public dgPolyhedra, public dgRefCounter, public UANS::UserAllocated
{
	public:

	class dgVertexAtribute 
	{
		public:
		dgBigVector m_vertex;
		double m_normal_x;
		double m_normal_y;
		double m_normal_z;
		double m_u0;
		double m_v0;
		double m_u1;
		double m_v1;
		double m_material;
	};

	class dgIndexArray 
	{
		public:
		int32_t m_materialCount;
		int32_t m_indexCount;
		int32_t m_materials[256];
		int32_t m_materialsIndexCount[256];
		int32_t* m_indexList;
	};


	dgMeshEffect(bool preAllocaBuffers);
	dgMeshEffect(const dgMeshEffect& source);
	dgMeshEffect(dgPolyhedra& mesh, const dgMeshEffect& source);

	// Create a convex hull Mesh form point cloud
	dgMeshEffect (const double* const vertexCloud, int32_t count, int32_t strideInByte, double distTol);

	// create a convex approximation
	dgMeshEffect (const dgMeshEffect& source, float maxConcavity, int32_t maxCount = 32, hacd::ICallback* callback = NULL);

	// create a planar Mesh
	dgMeshEffect(const dgMatrix& planeMatrix, float witdth, float breadth, int32_t material, const dgMatrix& textureMatrix0, const dgMatrix& textureMatrix1);
	virtual ~dgMeshEffect(void);

	dgMatrix CalculateOOBB (dgBigVector& size) const;
	void CalculateAABB (dgBigVector& min, dgBigVector& max) const;

	void CalculateNormals (double angleInRadians);
	void SphericalMapping (int32_t material);
	void BoxMapping (int32_t front, int32_t side, int32_t top);
	void UniformBoxMapping (int32_t material, const dgMatrix& textruMatrix);
	void CylindricalMapping (int32_t cylinderMaterial, int32_t capMaterial);

	dgEdge* InsertEdgeVertex (dgEdge* const edge, double param);

	dgMeshEffect* GetFirstLayer ();
	dgMeshEffect* GetNextLayer (dgMeshEffect* const layer);

	void Triangulate ();
	void ConvertToPolygons ();

	void RemoveUnusedVertices(int32_t* const vertexRemapTable);
	
	void BeginPolygon ();
	void AddPolygon (int32_t count, const float* const vertexList, int32_t stride, int32_t material);
	void AddPolygon (int32_t count, const double* const vertexList, int32_t stride, int32_t material);
	void EndPolygon (double tol);

	void PackVertexArrays ();

	void BuildFromVertexListIndexList(int32_t faceCount, const int32_t * const faceIndexCount, const int32_t * const faceMaterialIndex, 
		const float* const vertex, int32_t  vertexStrideInBytes, const int32_t * const vertexIndex,
		const float* const normal, int32_t  normalStrideInBytes, const int32_t * const normalIndex,
		const float* const uv0, int32_t  uv0StrideInBytes, const int32_t * const uv0Index,
		const float* const uv1, int32_t  uv1StrideInBytes, const int32_t * const uv1Index);


	int32_t GetVertexCount() const;
	int32_t GetVertexStrideInByte() const;
	double* GetVertexPool () const;

	int32_t GetPropertiesCount() const;
	int32_t GetPropertiesStrideInByte() const;
	double* GetAttributePool() const;
	double* GetNormalPool() const;
	double* GetUV0Pool() const;
	double* GetUV1Pool() const;

	dgEdge* ConectVertex (dgEdge* const e0, dgEdge* const e1);

	int32_t GetTotalFaceCount() const;
	int32_t GetTotalIndexCount() const;
	void GetFaces (int32_t* const faceCount, int32_t* const materials, void** const faceNodeList) const;

	void RepairTJoints (bool triangulate);
	bool SeparateDuplicateLoops (dgEdge* const edge);
	bool HasOpenEdges () const;

	void GetVertexStreams (int32_t vetexStrideInByte, float* const vertex, 
						   int32_t normalStrideInByte, float* const normal, 
						   int32_t uvStrideInByte0, float* const uv0, 
						   int32_t uvStrideInByte1, float* const uv1);

	void GetIndirectVertexStreams(int32_t vetexStrideInByte, double* const vertex, int32_t* const vertexIndices, int32_t* const vertexCount,
								  int32_t normalStrideInByte, double* const normal, int32_t* const normalIndices, int32_t* const normalCount,
								  int32_t uvStrideInByte0, double* const uv0, int32_t* const uvIndices0, int32_t* const uvCount0,
								  int32_t uvStrideInByte1, double* const uv1, int32_t* const uvIndices1, int32_t* const uvCount1);

	

	dgIndexArray* MaterialGeometryBegin();
	void MaterialGeomteryEnd(dgIndexArray* const handle);
	int32_t GetFirstMaterial (dgIndexArray* const handle);
	int32_t GetNextMaterial (dgIndexArray* const handle, int32_t materialHandle);
	int32_t GetMaterialID (dgIndexArray* const handle, int32_t materialHandle);
	int32_t GetMaterialIndexCount (dgIndexArray* const handle, int32_t materialHandle);
	void GetMaterialGetIndexStream (dgIndexArray* const handle, int32_t materialHandle, int32_t* const index);
	void GetMaterialGetIndexStreamShort (dgIndexArray* const handle, int32_t materialHandle, int16_t* const index);
	
	dgConvexHull3d * CreateConvexHull(double tolerance,int32_t maxVertexCount) const;

	dgMeshEffect* CreateConvexApproximation(
		float maxConcavity, 
		float backFaceDistanceFactor, 
		int32_t maxHullsCount, 
		int32_t maxVertexPerHull,
		hacd::ICallback *reportProgressCallback) const;

	// Old, Legacy, 'fast' version
	dgMeshEffect(const dgMeshEffect& source,
				float absoluteconcavity,
				int32_t maxCount,
				hacd::ICallback *callback,
				bool legacyVersion);

	dgMeshEffect* CreateConvexApproximationFast(float maxConcavity, int32_t maxCount,hacd::ICallback *callback) const;

	dgMeshEffect* CreateSimplification(int32_t maxVertexCount, hacd::ICallback *reportProgressCallback) const;

	dgVertexAtribute& GetAttribute (int32_t index) const;
	void TransformMesh (const dgMatrix& matrix);


	void* GetFirstVertex ();
	void* GetNextVertex (const void* const vertex);
	int GetVertexIndex (const void* const vertex) const;

	void* GetFirstPoint ();
	void* GetNextPoint (const void* const point);
	int GetPointIndex (const void* const point) const;
	int GetVertexIndexFromPoint (const void* const point) const;


	void* GetFirstEdge ();
	void* GetNextEdge (const void* const edge);
	void GetEdgeIndex (const void* const edge, int32_t& v0, int32_t& v1) const;
//	void GetEdgeAttributeIndex (const void* edge, int32_t& v0, int32_t& v1) const;

	void* GetFirstFace ();
	void* GetNextFace (const void* const face);
	int IsFaceOpen (const void* const face) const;
	int GetFaceMaterial (const void* const face) const;
	int GetFaceIndexCount (const void* const face) const;
	void GetFaceIndex (const void* const face, int* const indices) const;
	void GetFaceAttributeIndex (const void* const face, int* const indices) const;

	bool Sanity () const;

	protected:
	void ClearAttributeArray ();
	void Init (bool preAllocaBuffers);
	dgBigVector GetOrigin ()const;
	int32_t CalculateMaxAttributes () const;
	double QuantizeCordinade(double val) const;
	int32_t EnumerateAttributeArray (dgVertexAtribute* const attib);
	void ApplyAttributeArray (dgVertexAtribute* const attib,int32_t maxCount);
	void AddVertex(const dgBigVector& vertex);
	void AddAtribute (const dgVertexAtribute& attib);
	void AddPoint(const double* vertexList, int32_t material);
	void FixCylindricalMapping (dgVertexAtribute* const attib) const;

	void MergeFaces (const dgMeshEffect* const source);
	void ReverseMergeFaces (dgMeshEffect* const source);
	dgVertexAtribute InterpolateEdge (dgEdge* const edge, double param) const;
	dgVertexAtribute InterpolateVertex (const dgBigVector& point, dgEdge* const face) const;

	dgMeshEffect* GetNextLayer (int32_t mark);

	void FilterCoplanarFaces (const dgMeshEffect* const otherCap, float sign);

	bool CheckSingleMesh() const;


	int32_t m_pointCount;
	int32_t m_maxPointCount;

	int32_t m_atribCount;
	int32_t m_maxAtribCount;

	dgBigVector* m_points;
	dgVertexAtribute* m_attib;

	
	friend class dgConvexHull3d;
	friend class dgConvexHull4d;
	friend class dgMeshTreeCSGFace;
	friend class dgMeshEffectSolidTree;
};



inline int32_t dgMeshEffect::GetVertexCount() const
{
	return m_pointCount;
}

inline int32_t dgMeshEffect::GetPropertiesCount() const
{
	return m_atribCount;
}

inline int32_t dgMeshEffect::GetMaterialID (dgIndexArray* const handle, int32_t materialHandle)
{
	return handle->m_materials[materialHandle];
}

inline int32_t dgMeshEffect::GetMaterialIndexCount (dgIndexArray* const handle, int32_t materialHandle)
{
	return handle->m_materialsIndexCount[materialHandle];
}

inline dgMeshEffect::dgVertexAtribute& dgMeshEffect::GetAttribute (int32_t index) const 
{
	return m_attib[index];
}

inline int32_t dgMeshEffect::GetPropertiesStrideInByte() const 
{
	return sizeof (dgVertexAtribute);
}

inline double* dgMeshEffect::GetAttributePool() const 
{
	return &m_attib->m_vertex.m_x;
}

inline double* dgMeshEffect::GetNormalPool() const 
{
	return &m_attib->m_normal_x;
}

inline double* dgMeshEffect::GetUV0Pool() const 
{
	return &m_attib->m_u0;
}

inline double* dgMeshEffect::GetUV1Pool() const 
{
	return &m_attib->m_u1;
}

inline int32_t dgMeshEffect::GetVertexStrideInByte() const 
{
	return sizeof (dgBigVector);
}

inline double* dgMeshEffect::GetVertexPool () const 
{
	return &m_points[0].m_x;
}


inline dgMeshEffect* dgMeshEffect::GetFirstLayer ()
{
	return GetNextLayer (IncLRU());
}

inline dgMeshEffect* dgMeshEffect::GetNextLayer (dgMeshEffect* const layerSegment)
{
	if (!layerSegment) {
		return NULL;
	}
	return GetNextLayer (layerSegment->IncLRU() - 1);
}


inline double dgMeshEffect::QuantizeCordinade(double x) const
{
	int32_t exp;
	double mantissa = frexp(x, &exp);
	mantissa = DG_MESH_EFFECT_PRECISION_SCALE_INV * floor (mantissa * DG_MESH_EFFECT_PRECISION_SCALE);

	double x1 = ldexp(mantissa, exp);
	return x1;
}

#endif
