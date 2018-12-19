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


#ifndef APEX_CSG_DEFS_H
#define APEX_CSG_DEFS_H

#include "ApexUsingNamespace.h"
#include "ApexSharedUtils.h"
#include "ApexRand.h"
#include "Link.h"
#include "authoring/ApexCSG.h"
#include "authoring/ApexCSGMath.h"
#include "authoring/ApexGSA.h"
#include "PsUserAllocated.h"
#include "ApexGSA.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace ApexCSG
{

// Binary tree node
class BinaryNode
{
public:
	PX_INLINE					BinaryNode();

	PX_INLINE	void			setChild(uint32_t index, BinaryNode* child);

	PX_INLINE	void			detach();

	PX_INLINE	BinaryNode*		getParent()						const
	{
		return m_parent;
	}

	PX_INLINE	BinaryNode*		getChild(uint32_t index)	const
	{
		PX_ASSERT((index & 1) == index);
		return m_children[index & 1];
	}

	PX_INLINE	uint32_t	getIndex()						const
	{
		return m_index;
	}

protected:
	BinaryNode*		m_parent;
	BinaryNode*		m_children[2];
	uint32_t	m_index;	// index of this node in parent (0xFFFFFFFF => not attached)
};

PX_INLINE
BinaryNode::BinaryNode()
{
	m_index = 0xFFFFFFFF;
	m_children[1] = m_children[0] = m_parent = NULL;
}

PX_INLINE void
BinaryNode::setChild(uint32_t index, BinaryNode* child)
{
	index &= 1;
	BinaryNode*& oldChild = m_children[index];

	if (oldChild != NULL)
	{
		oldChild->detach();
	}

	oldChild = child;

	if (child != NULL)
	{
		child->detach();
		child->m_parent = this;
		child->m_index = index;
	}
}

PX_INLINE void
BinaryNode::detach()
{
	if (m_parent != NULL)
	{
		PX_ASSERT(m_parent->getChild(m_index) == this);
		m_parent->m_children[m_index & 1] = NULL;
		m_parent = NULL;
		m_index = 0xFFFFFFFF;
	}
}


// CSG mesh representation

class UV : public Vec<Real, 2>
{
public:

	PX_INLINE				UV()							{}
	PX_INLINE				UV(const float* uv)
	{
		set((Real)uv[0], (Real)uv[1]);
	}
	PX_INLINE				UV(const double* uv)
	{
		set((Real)uv[0], (Real)uv[1]);
	}
	PX_INLINE	UV&			operator = (const UV& uv)
	{
		el[0] = uv.el[0];
		el[1] = uv.el[1];
		return *this;
	}

	PX_INLINE	void		set(Real u, Real v)
	{
		el[0] = u;
		el[1] = v;
	}

	PX_INLINE	const Real&	u() const
	{
		return el[0];
	}
	PX_INLINE	const Real&	v() const
	{
		return el[1];
	}
};

class Color : public Vec<Real, 4>
{
public:

	PX_INLINE					Color()									{}
	PX_INLINE					Color(const uint32_t c);
	PX_INLINE	Color&			operator = (const Color& c)
	{
		el[0] = c.el[0];
		el[1] = c.el[1];
		el[2] = c.el[2];
		el[3] = c.el[3];
		return *this;
	}

	PX_INLINE	void			set(Real r, Real g, Real b, Real a)
	{
		el[0] = r;
		el[1] = g;
		el[2] = b;
		el[3] = a;
	}

	PX_INLINE	uint32_t	toInt() const;

	PX_INLINE	const Real&		r() const
	{
		return el[0];
	}
	PX_INLINE	const Real&		g() const
	{
		return el[1];
	}
	PX_INLINE	const Real&		b() const
	{
		return el[2];
	}
	PX_INLINE	const Real&		a() const
	{
		return el[3];
	}
};

PX_INLINE
Color::Color(const uint32_t c)
{
	const Real recip255 = 1 / (Real)255;
	set((Real)(c & 0xFF)*recip255, (Real)((c >> 8) & 0xFF)*recip255, (Real)((c >> 16) & 0xFF)*recip255, (Real)(c >> 24)*recip255);
}

PX_INLINE uint32_t
Color::toInt() const
{
	return (uint32_t)((int)(255 * el[3] + (Real)0.5)) << 24 | (uint32_t)((int)(255 * el[2] + (Real)0.5)) << 16 | (uint32_t)((int)(255 * el[1] + (Real)0.5)) << 8 | (uint32_t)((int)(255 * el[0] + (Real)0.5));
}

struct VertexData
{
	Dir		normal;
	Dir		tangent;
	Dir		binormal;
	UV		uv[nvidia::VertexFormat::MAX_UV_COUNT];
	Color	color;
};

struct Triangle
{
	Pos				vertices[3];
	Dir				normal;
	Real			area;
	int32_t	submeshIndex;
	uint32_t	smoothingMask;
	uint32_t	extraDataIndex;

	void	fromExplicitRenderTriangle(VertexData vertexData[3], const nvidia::ExplicitRenderTriangle& tri)
	{
		for (unsigned i = 0; i < 3; ++i)
		{
			vertices[i] = Pos(tri.vertices[i].position);
			vertexData[i].normal = Dir(tri.vertices[i].normal);
			vertexData[i].tangent = Dir(tri.vertices[i].tangent);
			vertexData[i].binormal = Dir(tri.vertices[i].binormal);
			for (unsigned j = 0; j < nvidia::VertexFormat::MAX_UV_COUNT; ++j)
			{
				vertexData[i].uv[j] = UV(&tri.vertices[i].uv[j][0]);
			}
			vertexData[i].color.set((Real)tri.vertices[i].color.r, (Real)tri.vertices[i].color.g, (Real)tri.vertices[i].color.b, (Real)tri.vertices[i].color.a);
		}
		submeshIndex = tri.submeshIndex;
		smoothingMask = tri.smoothingMask;
		extraDataIndex = tri.extraDataIndex;
		calculateQuantities();
	}

	void	toExplicitRenderTriangle(nvidia::ExplicitRenderTriangle& tri, const VertexData vertexData[3]) const
	{
		for (unsigned i = 0; i < 3; ++i)
		{
			tri.vertices[i].position = ApexCSG::GSA::toPxVec3(vertices[i]);
			tri.vertices[i].normal = ApexCSG::GSA::toPxVec3(vertexData[i].normal);
			tri.vertices[i].tangent = ApexCSG::GSA::toPxVec3(vertexData[i].tangent);
			tri.vertices[i].binormal = ApexCSG::GSA::toPxVec3(vertexData[i].binormal);
			for (unsigned j = 0; j < nvidia::VertexFormat::MAX_UV_COUNT; ++j)
			{
				tri.vertices[i].uv[j].set((float)vertexData[i].uv[j][0], (float)vertexData[i].uv[j][1]);
			}
			tri.vertices[i].color.set((float)vertexData[i].color.r(), (float)vertexData[i].color.g(), (float)vertexData[i].color.b(), (float)vertexData[i].color.a());
		}
		tri.submeshIndex = submeshIndex;
		tri.smoothingMask = smoothingMask;
		tri.extraDataIndex = extraDataIndex;
	}

	void	calculateQuantities()
	{
		const Dir e0 = Dir(vertices[1] - vertices[0]);
		const Dir e1 = Dir(vertices[2] - vertices[1]);
		const Dir e2 = Dir(vertices[0] - vertices[2]);
		normal = (e0^e1) + (e1^e2) + (e2^e0);
		area = (Real)0.5 * normal.normalize();
	}

	void	transform(const Mat4Real& tm)
	{
		for (int i = 0; i < 3; ++i)
		{
			vertices[i] = tm*vertices[i];
		}
		calculateQuantities();
	}
};

struct LinkedVertex : public nvidia::Link
{
	LinkedVertex*	getAdj(uint32_t which) const
	{
		return (LinkedVertex*)nvidia::Link::getAdj(which);
	}

	Pos	vertex;
};

struct LinkedEdge2D : public nvidia::Link
{
	LinkedEdge2D() : loopID(-1) {}
	~LinkedEdge2D()
	{
		remove();
	}

	void			setAdj(uint32_t which, LinkedEdge2D* link)
	{
		// Ensure neighboring links' adjoining vertices are equal
		which &= 1;
		const uint32_t other = which ^ 1;
		v[which] = link->v[other];
		((LinkedEdge2D*)link->adj[other])->v[which] = ((LinkedEdge2D*)adj[which])->v[other];
		nvidia::Link::setAdj(which, link);
	}

	LinkedEdge2D*	getAdj(uint32_t which) const
	{
		return (LinkedEdge2D*)nvidia::Link::getAdj(which);
	}

	void			remove()
	{
		// Ensure neighboring links' adjoining vertices are equal
		((LinkedEdge2D*)adj[0])->v[1] = ((LinkedEdge2D*)adj[1])->v[0] = (Real)0.5 * (v[0] + v[1]);
		nvidia::Link::remove();
	}

	Vec2Real		v[2];
	int32_t	loopID;
};

struct Surface
{
	uint32_t	planeIndex;
	uint32_t	triangleIndexStart;
	uint32_t	triangleIndexStop;
	float	totalTriangleArea;	// Keeping it 32-bit real, since we don't need precision here
};

struct Region
{
	uint32_t	side;

	// Not to be serialized, but we have this extra space since Region is used in a union with Surface
	uint32_t	tempIndex1;
	uint32_t	tempIndex2;
	uint32_t	tempIndex3;
};


// Interpolator - calculates interpolation data for triangle quantities
class Interpolator
{
public:

	enum VertexField
	{
		Normal_x, Normal_y, Normal_z,
		Tangent_x, Tangent_y, Tangent_z,
		Binormal_x, Binormal_y, Binormal_z,
		UV0_u, UV0_v, UV1_u, UV1_v, UV2_u, UV2_v, UV3_u, UV3_v,
		Color_r, Color_g, Color_b, Color_a,

		VertexFieldCount
	};

	Interpolator()															{}
	Interpolator(const Triangle& tri, const VertexData vertexData[3])
	{
		setFromTriangle(tri, vertexData);
	}
	Interpolator(const Dir tangents[3], const Vec<Real, 2>& uvScale)
	{
		setFlat(tangents, uvScale);
	}

	PX_INLINE void	setFromTriangle(const Triangle& tri, const VertexData vertexData[3]);
	PX_INLINE void	setFlat(const Dir tangents[3], const Vec<Real, 2>& uvScale);

	PX_INLINE void	interpolateVertexData(VertexData& vertexData, const Pos& point) const;

	PX_INLINE bool	equals(const Interpolator& interpolator, Real frameDirTol, Real frameScaleTol, Real dirTol, Real uvTol, Real colorTol) const;

	PX_INLINE void	transform(Interpolator& transformedInterpolator, const Mat4Real& tm, const Mat4Real& cofTM) const;

	void	serialize(physx::PxFileBuf& stream) const;
	void	deserialize(physx::PxFileBuf& stream, uint32_t version);

private:
	ApexCSG::Plane	m_frames[VertexFieldCount];
	static size_t	s_offsets[VertexFieldCount];

	friend class InterpolatorBuilder;
};

PX_INLINE void
Interpolator::setFromTriangle(const Triangle& tri, const VertexData vertexData[3])
{
	const Pos& p0 = tri.vertices[0];
	const Pos& p1 = tri.vertices[1];
	const Pos& p2 = tri.vertices[2];
	const Dir p1xp2 = Dir(p1) ^ Dir(p2);
	const Dir p2xp0 = Dir(p2) ^ Dir(p0);
	const Dir p0xp1 = Dir(p0) ^ Dir(p1);
	const Dir n = p1xp2 + p2xp0 + p0xp1;
	const Real n2 = n | n;
	if (n2 < EPS_REAL * EPS_REAL)
	{
		for (uint32_t i = 0; i < VertexFieldCount; ++i)
		{
			m_frames[i].set(Dir((Real)0), 0);
		}
		return;
	}

	// Calculate inverse 4x4 matrix (only need first three columns):
	const Dir nP = n / n2;	// determinant is -n2
	const Dir Q0(nP[2] * (p1[1] - p2[1]) - nP[1] * (p1[2] - p2[2]), nP[2] * (p2[1] - p0[1]) - nP[1] * (p2[2] - p0[2]), nP[2] * (p0[1] - p1[1]) - nP[1] * (p0[2] - p1[2]));
	const Dir Q1(nP[0] * (p1[2] - p2[2]) - nP[2] * (p1[0] - p2[0]), nP[0] * (p2[2] - p0[2]) - nP[2] * (p2[0] - p0[0]), nP[0] * (p0[2] - p1[2]) - nP[2] * (p0[0] - p1[0]));
	const Dir Q2(nP[1] * (p1[0] - p2[0]) - nP[0] * (p1[1] - p2[1]), nP[1] * (p2[0] - p0[0]) - nP[0] * (p2[1] - p0[1]), nP[1] * (p0[0] - p1[0]) - nP[0] * (p0[1] - p1[1]));
	const Dir r(nP | p1xp2, nP | p2xp0, nP | p0xp1);

	for (uint32_t i = 0; i < VertexFieldCount; ++i)
	{
		const size_t offset = s_offsets[i];
		const Dir vi(*(Real*)(((uint8_t*)&vertexData[0]) + offset), *(Real*)(((uint8_t*)&vertexData[1]) + offset), *(Real*)(((uint8_t*)&vertexData[2]) + offset));
		Dir n(Q0 | vi, Q1 | vi, Q2 | vi);
		if ((n | n) < 100 * EPS_REAL * EPS_REAL)
		{
			n.set((Real)0, (Real)0, (Real)0);
		}
		Real o = r | vi;
		if (physx::PxAbs(o) < 100 * EPS_REAL)
		{
			o = (Real)0;
		}
		m_frames[i].set(n, o);
	}
}

PX_INLINE void
Interpolator::setFlat(const Dir tangents[3], const Vec<Real, 2>& uvScale)
{
	// Local z ~ normal = tangents[2], x ~ u and tangent = tangents[0], y ~ v and binormal = tangents[1]
	m_frames[Normal_x].set(Dir((Real)0), tangents[2][0]);
	m_frames[Normal_y].set(Dir((Real)0), tangents[2][1]);
	m_frames[Normal_z].set(Dir((Real)0), tangents[2][2]);
	m_frames[Tangent_x].set(Dir((Real)0), tangents[0][0]);
	m_frames[Tangent_y].set(Dir((Real)0), tangents[0][1]);
	m_frames[Tangent_z].set(Dir((Real)0), tangents[0][2]);
	m_frames[Binormal_x].set(Dir((Real)0), tangents[1][0]);
	m_frames[Binormal_y].set(Dir((Real)0), tangents[1][1]);
	m_frames[Binormal_z].set(Dir((Real)0), tangents[1][2]);
	const Dir su = (uvScale[0] ? 1 / uvScale[0] : (Real)0) * tangents[0];
	const Dir sv = (uvScale[1] ? 1 / uvScale[1] : (Real)0) * tangents[1];
	m_frames[UV0_u].set(su, 0);
	m_frames[UV0_v].set(sv, 0);
	m_frames[UV1_u].set(su, 0);
	m_frames[UV1_v].set(sv, 0);
	m_frames[UV2_u].set(su, 0);
	m_frames[UV2_v].set(sv, 0);
	m_frames[UV3_u].set(su, 0);
	m_frames[UV3_v].set(sv, 0);
	m_frames[Color_r].set(Dir((Real)0), (Real)1);
	m_frames[Color_g].set(Dir((Real)0), (Real)1);
	m_frames[Color_b].set(Dir((Real)0), (Real)1);
	m_frames[Color_a].set(Dir((Real)0), (Real)1);
}

PX_INLINE void
Interpolator::interpolateVertexData(VertexData& vertexData, const Pos& point) const
{
	for (uint32_t i = 0; i < VertexFieldCount; ++i)
	{
		Real& value = *(Real*)(((uint8_t*)&vertexData) + s_offsets[i]);
		value = m_frames[i].distance(point);
	}
}

PX_INLINE bool
framesEqual(const Plane& f0, const Plane& f1, Real twoFrameScaleTol2, Real sinFrameTol2, Real tol2)
{
	const Dir n0 = f0.normal();
	const Dir n1 = f1.normal();
	const Real n02 = n0 | n0;
	const Real n12 = n1 | n1;
	const Real n2Diff = n02 - n12;

	if (n2Diff * n2Diff > twoFrameScaleTol2 * (n02 + n12))
	{
		return false;	// Scales differ by more than frame scale tolerance
	}

	const Real n2Prod = n02 * n12;
	const Real unnormalizedSinFrameTheta2 = (n0 ^ n1).lengthSquared();
	if (unnormalizedSinFrameTheta2 > n2Prod * sinFrameTol2)
	{
		return false;	// Directions differ by more than frame angle tolerance
	}

	const Real unnormalizedOriginDiff = f0.d() - f1.d();
	const Real originScale = 0.5f * (physx::PxAbs(f0.d()) + physx::PxAbs(f1.d()));
	if (unnormalizedOriginDiff * unnormalizedOriginDiff > tol2 * originScale * originScale)
	{
		return false;	// Origins differ by more than tolerance
	}

	return true;
}

PX_INLINE bool
Interpolator::equals(const Interpolator& interpolator, Real frameDirTol, Real frameScaleTol, Real dirTol, Real uvTol, Real colorTol) const
{
	const Real twoFrameScaleTol2 = (Real)2 * frameScaleTol * frameScaleTol;
	const Real sinFrameTol2 = frameDirTol * frameDirTol;
	const Real dirTol2 = dirTol * dirTol;
	const Real uvTol2 = uvTol * uvTol;
	const Real colorTol2 = colorTol * colorTol;

	// Directions
	for (uint32_t i = Normal_x; i <= Binormal_z; ++i)
	{
		if (!framesEqual(m_frames[i], interpolator.m_frames[i], twoFrameScaleTol2, sinFrameTol2, dirTol2))
		{
			return false;
		}
	}

	// UVs
	for (uint32_t i = UV0_u; i <= UV3_v; ++i)
	{
		if (!framesEqual(m_frames[i], interpolator.m_frames[i], twoFrameScaleTol2, sinFrameTol2, uvTol2))
		{
			return false;
		}
	}

	// Color
	for (uint32_t i = Color_r; i <= Color_a; ++i)
	{
		if (!framesEqual(m_frames[i], interpolator.m_frames[i], twoFrameScaleTol2, sinFrameTol2, colorTol2))
		{
			return false;
		}
	}

	return true;
}

PX_INLINE void
Interpolator::transform(Interpolator& transformedInterpolator, const Mat4Real& tm, const Mat4Real& invTransposeTM) const
{
	// Apply left-hand transform.
	for (uint32_t i = 0; i < VertexFieldCount; ++i)
	{
		transformedInterpolator.m_frames[i] = invTransposeTM * m_frames[i];
	}
	// Apply right-hand transform.  This is specific to the quantities being transformed.
	for (int i = 0; i < 4; ++i)
	{
		// Normal, transform by invTransposeTM:
		Dir normal_frame_i(transformedInterpolator.m_frames[Interpolator::Normal_x][i], transformedInterpolator.m_frames[Interpolator::Normal_y][i], transformedInterpolator.m_frames[Interpolator::Normal_z][i]);
		normal_frame_i = invTransposeTM * normal_frame_i;
		transformedInterpolator.m_frames[Interpolator::Normal_x][i] = normal_frame_i[0];
		transformedInterpolator.m_frames[Interpolator::Normal_y][i] = normal_frame_i[1];
		transformedInterpolator.m_frames[Interpolator::Normal_z][i] = normal_frame_i[2];
		// Tangent, transform by tm:
		Dir tangent_frame_i(transformedInterpolator.m_frames[Interpolator::Tangent_x][i], transformedInterpolator.m_frames[Interpolator::Tangent_y][i], transformedInterpolator.m_frames[Interpolator::Tangent_z][i]);
		tangent_frame_i = tm * tangent_frame_i;
		transformedInterpolator.m_frames[Interpolator::Tangent_x][i] = tangent_frame_i[0];
		transformedInterpolator.m_frames[Interpolator::Tangent_y][i] = tangent_frame_i[1];
		transformedInterpolator.m_frames[Interpolator::Tangent_z][i] = tangent_frame_i[2];
		// Binormal, transform by tm:
		Dir binormal_frame_i(transformedInterpolator.m_frames[Interpolator::Binormal_x][i], transformedInterpolator.m_frames[Interpolator::Binormal_y][i], transformedInterpolator.m_frames[Interpolator::Binormal_z][i]);
		binormal_frame_i = tm * binormal_frame_i;
		transformedInterpolator.m_frames[Interpolator::Binormal_x][i] = binormal_frame_i[0];
		transformedInterpolator.m_frames[Interpolator::Binormal_y][i] = binormal_frame_i[1];
		transformedInterpolator.m_frames[Interpolator::Binormal_z][i] = binormal_frame_i[2];
		// Other quantities are scalars
	}
}


class InterpolatorBuilder
{
public:
	InterpolatorBuilder()
	{
#define CREATE_OFFSET( field )	(size_t)((uintptr_t)&vertexData.field-(uintptr_t)&vertexData)

		VertexData vertexData;
		Interpolator::s_offsets[Interpolator::Normal_x] =	CREATE_OFFSET(normal[0]);
		Interpolator::s_offsets[Interpolator::Normal_y] =	CREATE_OFFSET(normal[1]);
		Interpolator::s_offsets[Interpolator::Normal_z] =	CREATE_OFFSET(normal[2]);
		Interpolator::s_offsets[Interpolator::Tangent_x] =	CREATE_OFFSET(tangent[0]);
		Interpolator::s_offsets[Interpolator::Tangent_y] =	CREATE_OFFSET(tangent[1]);
		Interpolator::s_offsets[Interpolator::Tangent_z] =	CREATE_OFFSET(tangent[2]);
		Interpolator::s_offsets[Interpolator::Binormal_x] =	CREATE_OFFSET(binormal[0]);
		Interpolator::s_offsets[Interpolator::Binormal_y] =	CREATE_OFFSET(binormal[1]);
		Interpolator::s_offsets[Interpolator::Binormal_z] =	CREATE_OFFSET(binormal[2]);
		Interpolator::s_offsets[Interpolator::UV0_u] =		CREATE_OFFSET(uv[0].u());
		Interpolator::s_offsets[Interpolator::UV0_v] =		CREATE_OFFSET(uv[0].v());
		Interpolator::s_offsets[Interpolator::UV1_u] =		CREATE_OFFSET(uv[1].u());
		Interpolator::s_offsets[Interpolator::UV1_v] =		CREATE_OFFSET(uv[1].v());
		Interpolator::s_offsets[Interpolator::UV2_u] =		CREATE_OFFSET(uv[2].u());
		Interpolator::s_offsets[Interpolator::UV2_v] =		CREATE_OFFSET(uv[2].v());
		Interpolator::s_offsets[Interpolator::UV3_u] =		CREATE_OFFSET(uv[3].u());
		Interpolator::s_offsets[Interpolator::UV3_v] =		CREATE_OFFSET(uv[3].v());
		Interpolator::s_offsets[Interpolator::Color_r] =	CREATE_OFFSET(color.r());
		Interpolator::s_offsets[Interpolator::Color_g] =	CREATE_OFFSET(color.g());
		Interpolator::s_offsets[Interpolator::Color_b] =	CREATE_OFFSET(color.b());
		Interpolator::s_offsets[Interpolator::Color_a] =	CREATE_OFFSET(color.a());
	}
};


// ClippedTriangleInfo - used to map bsp output back to the original mesh
struct ClippedTriangleInfo
{
	uint32_t	planeIndex;
	uint32_t	originalTriangleIndex;
	uint32_t	clippedTriangleIndex;
	uint32_t	ccw;

	static	int	cmp(const void* a, const void* b)
	{
		const int planeIndexDiff = (int)((ClippedTriangleInfo*)a)->planeIndex - (int)((ClippedTriangleInfo*)b)->planeIndex;
		if (planeIndexDiff != 0)
		{
			return planeIndexDiff;
		}
		const int originalTriangleDiff = (int)((ClippedTriangleInfo*)a)->originalTriangleIndex - (int)((ClippedTriangleInfo*)b)->originalTriangleIndex;
		if (originalTriangleDiff != 0)
		{
			return originalTriangleDiff;
		}
		return (int)((ClippedTriangleInfo*)a)->clippedTriangleIndex - (int)((ClippedTriangleInfo*)b)->clippedTriangleIndex;
	}
};

// BSPLink - a link with an "isBSP" method to act as a stop
class BSPLink : public nvidia::Link, public nvidia::UserAllocated
{
public:
	virtual bool	isBSP()
	{
		return false;
	}

	BSPLink*		getAdjBSP(uint32_t which) const
	{
		if (isSolitary())
		{
			return NULL;
		}
		BSPLink* adjLink = static_cast<BSPLink*>(getAdj(which));
		return adjLink->isBSP() ? adjLink : NULL;
	}

	void			removeBSPLink()
	{
		BSPLink* adjLink = static_cast<BSPLink*>(getAdj(1));
		remove();
		if (!adjLink->isBSP() && adjLink->isSolitary())
		{
			delete adjLink;
		}
	}
};

// Specialized progress listener implementation
class QuantityProgressListener : public nvidia::IProgressListener
{
public:
	QuantityProgressListener(Real totalAmount, IProgressListener* parent) : 
		m_total((Real)0)
	,	m_parent(parent)
	{
		m_scale = totalAmount > (Real)0 ? (Real)100/(Real)totalAmount : (Real)0;
	}

	// IProgressListener interface
	virtual void	setProgress(int progress, const char* taskName = NULL)
	{
		if (m_parent != NULL)
		{
			m_parent->setProgress(progress, taskName);
		}
	}

	virtual void	add(Real amount)
	{
		m_total += amount;
		if (m_parent != NULL)
		{
			m_parent->setProgress((int)(m_total*m_scale + (Real)0.5));
		}
	}

private:
	Real				m_total;
	Real				m_scale;
	IProgressListener*	m_parent;
};


// IApexBSP implementation
class BSP : public IApexBSP, public BSPLink
{
public:
	BSP(IApexBSPMemCache* memCache = NULL, const physx::PxMat44& internalTransform = physx::PxMat44(physx::PxIdentity));
	~BSP();

	// IApexBSP implementation
	void			setTolerances(const BSPTolerances& tolerances);
	bool			fromMesh(const nvidia::ExplicitRenderTriangle* mesh, uint32_t meshSize, const BSPBuildParameters& params, nvidia::IProgressListener* progressListener = NULL, volatile bool* cancel = NULL);
	bool			fromConvexPolyhedron(const physx::PxPlane* poly, uint32_t polySize, const physx::PxMat44& internalTransform = physx::PxMat44(physx::PxIdentity), const nvidia::ExplicitRenderTriangle* mesh = NULL, uint32_t meshSize = 0);
	bool			combine(const IApexBSP& bsp);
	bool			op(const IApexBSP& combinedBSP, Operation::Enum operation);
	bool			complement();
	BSPType::Enum	getType() const;
	bool			getSurfaceAreaAndVolume(float& area, float& volume, bool inside, Operation::Enum operation = Operation::NOP) const;
	bool			pointInside(const physx::PxVec3& point, Operation::Enum operation = Operation::NOP) const;
	bool			toMesh(physx::Array<nvidia::ExplicitRenderTriangle>& mesh) const;
	void			copy(const IApexBSP& bsp, const physx::PxMat44& tm = physx::PxMat44(physx::PxIdentity), const physx::PxMat44& internalTransform = physx::PxMat44(physx::PxZero));
	physx::PxMat44	getInternalTransform() const
	{
		return m_internalTransform;
	}

	void			replaceInteriorSubmeshes(uint32_t frameCount, uint32_t* frameIndices, uint32_t submeshIndex);

	IApexBSP*		decomposeIntoIslands() const;
	IApexBSP*		getNext() const
	{
		return static_cast<BSP*>(getAdjBSP(1));
	}
	IApexBSP*		getPrev() const
	{
		return static_cast<BSP*>(getAdjBSP(0));
	}

	void			deleteTriangles();

	void			serialize(physx::PxFileBuf& stream) const;
	void			deserialize(physx::PxFileBuf& stream);
	void			visualize(nvidia::RenderDebugInterface& debugRender, uint32_t flags, uint32_t index = 0) const;
	void			release();

	// Debug
	void			performDiagnostics() const;

	// BSPLink
	bool			isBSP()
	{
		return true;
	}

	// Node, a binary node with geometric data
	class Node : public BinaryNode
	{
		Node& operator = (const Node&); // No assignment

	public:
		enum Type { Leaf, Branch };

		Node() : m_type(Leaf)
		{
			m_leafData.side = 1;
		}

		PX_INLINE	void		setLeafData(const Region& leafData)
		{
			m_type = Leaf;
			m_leafData = leafData;
		}
		PX_INLINE	void		setBranchData(const Surface& branchData)
		{
			m_type = Branch;
			m_branchData = branchData;
		}

		PX_INLINE	Type		getType()						const
		{
			return (Type)m_type;
		}

		PX_INLINE	Region*			getLeafData()
		{
			PX_ASSERT(getType() == Leaf);
			return &m_leafData;
		}
		PX_INLINE	Surface*		getBranchData()
		{
			PX_ASSERT(getType() == Branch);
			return &m_branchData;
		}
		PX_INLINE	const Region*	getLeafData()				const
		{
			PX_ASSERT(getType() == Leaf);
			return &m_leafData;
		}
		PX_INLINE	const Surface*	getBranchData()				const
		{
			PX_ASSERT(getType() == Branch);
			return &m_branchData;
		}

		PX_INLINE	Node*		getParent()						const
		{
			return (Node*)BinaryNode::getParent();
		}
		PX_INLINE	Node*		getChild(uint32_t index)	const
		{
			return (Node*)BinaryNode::getChild(index);
		}

		// Iterator (uses a stack, but no recursion)
		// Can handle branches with NULL children
		class It
		{
		public:
			PX_INLINE			It(const Node* root) : m_current(const_cast<Node*>(root)), m_valid(true) {}
			PX_INLINE			It(Node* root) : m_current(root), m_valid(true) {}

			PX_INLINE	bool	valid()	const
			{
				return m_valid;
			}

			PX_INLINE	Node*	node()	const
			{
				return m_current;
			}

			PX_INLINE	void	inc()
			{
				if (m_current != NULL && m_current->getType() == Branch)
				{
					m_stack.pushBack(m_current->getChild(1));
					m_current = m_current->getChild(0);
				}
				else
				if (!m_stack.empty())
				{
					m_current  = m_stack.popBack();
				}
				else
				{
					m_current = NULL;
					m_valid = false;
				}
			}

		private:
			Node*				m_current;
			physx::Array<Node*>	m_stack;
			bool				m_valid;
		};

	protected:
		uint32_t	m_type;

		union
		{
			Region		m_leafData;
			Surface		m_branchData;
		};
	};

	class Halfspace : public GSA::VS3D_Halfspace_Set
	{
	public:
							Halfspace(const Plane plane) : m_plane(plane) {}

			virtual	GSA::real	farthest_halfspace(GSA::real plane[4], const GSA::real point[4])
			{
				for (int i = 0; i < 4; ++i) plane[i] = (GSA::real)m_plane[i];
				return plane[0]*point[0] + plane[1]*point[1] + plane[2]*point[2] + plane[3]*point[3];
			}

			Halfspace&	operator = (const Halfspace& halfspace) { m_plane = halfspace.m_plane; return *this; }

	private:
		Plane	m_plane;
	};

	class RegionShape : public GSA::VS3D_Halfspace_Set
	{
	public:
		RegionShape(const Plane* planes, Real skinWidth = (Real)0) : m_planes(planes), m_leaf(NULL), m_nonempty(true), m_skinWidth(skinWidth) {}

		virtual	GSA::real	farthest_halfspace(GSA::real plane[4], const GSA::real point[4]);

		void		set_leaf(const BSP::Node* leaf)
		{
			m_leaf = leaf;
		}

		void		calculate()
		{
			m_nonempty = (1 == GSA::vs3d_test(*this));
		}

		bool		is_nonempty() const
		{
			return m_nonempty;
		}

#if 0
		bool		intersects_halfspace(const Plane* plane)
		{
			Halfspace halfspace(plane);
			set_shapes(this, &halfspace);
			return intersect();
		}
#endif

	private:
		const Plane*		m_planes;
		const BSP::Node*	m_leaf;
		bool				m_nonempty;
		Real				m_skinWidth;
	};

private:
	class BoolOp
	{
	public:
		BoolOp(Operation::Enum op) : c_ba(((uint32_t)op >> 3) & 1), c_b(((uint32_t)op >> 2) & 1), c_a(((uint32_t)op >> 1) & 1), c_k((uint32_t)op & 1)	{}

		uint32_t	operator()(uint32_t a, uint32_t b) const
		{
			return (c_ba & a & b) ^(c_b & b) ^(c_a & a) ^ c_k;
		}

	private:
		uint32_t	c_ba, c_b, c_a, c_k;
	};

	struct BuildConstants
	{
		BSPBuildParameters	m_params;
		float		m_recipMaxArea;
	};

	void		clear();

	void		transform(const Mat4Real& tm, bool transformFrames = true);

	// Returns the area and volume of the clipped mesh.  clippedMesh and triangleInfo may be NULL, in which case nothing is done but
	// the area and volume calculation.
	void		clipMeshToLeaf(Real& area, Real& volume, physx::Array<Triangle>* clippedMesh, physx::Array<ClippedTriangleInfo>* triangleInfo, const Node* leaf, float clipTolerance) const;

	// Called by buildTree - forcing no inline to ensure a small stack frame

							// Returns a new stackReadStop
	PX_INLINE uint32_t	removeRedundantSurfacesFromStack(physx::Array<Surface>& surfaceStack, uint32_t stackReadStart, uint32_t stackReadStop, Node* leaf);
	PX_INLINE void		assignLeafSide(Node* leaf, QuantityProgressListener* quantityListener);
	PX_INLINE void		createBranchSurfaceAndSplitStack(uint32_t childReadStart[2], uint32_t childReadStop[2], Node* node, physx::Array<Surface>& surfaceStack,
														 uint32_t stackReadStart, uint32_t stackReadStop, const BuildConstants& buildConstants);

	// Recursive functions
	void		complementLeaves(Node* root) const;
	void		mergeLeaves(const BoolOp& op, Node* root);
	void		clipMeshToLeaves(physx::Array<Triangle>& clippedMesh, physx::Array<ClippedTriangleInfo>& triangleInfo, Node* root, float clipTolerance) const;
	void		clone(Node* root, const Node* originalRoot);
	void		combineTrees(Node* root, const Node* combineRoot, uint32_t triangleIndexOffset, uint32_t planeIndexOffset);
	bool		buildTree(Node* root, physx::Array<Surface>& surfaceStack, uint32_t stackReadStart, uint32_t stackReadStop,
	                      const BuildConstants& buildConstants, QuantityProgressListener* quantityListener, volatile bool* cancel = NULL);
	void		visualizeNode(nvidia::RenderDebugInterface& debugRender, uint32_t flags, const Node* root) const;
	bool		addLeafAreasAndVolumes(Real& totalArea, Real& totalVolume, const Node* root, bool inside, const BoolOp& op) const;
	void		serializeNode(const Node* root, physx::PxFileBuf& stream) const;
	Node*		deserializeNode(uint32_t version, physx::PxFileBuf& stream);
	void		releaseNode(Node* node);
	void		indexInsideLeaves(uint32_t& index, Node* root) const;
	void		listInsideLeaves(physx::Array<Node*>& insideLeaves, Node* root) const;
	void		findInsideLeafNeighbors(physx::Array<nvidia::IntPair>& neighbors, Node* root) const;

	void		clean();

	// Parameters
	BSPTolerances				m_tolerarnces;

	// Tree
	Node*						m_root;

	// Internal mesh representation
	physx::Array<Triangle>		m_mesh;
	physx::Array<Interpolator>	m_frames;
	Real						m_meshSize;
	physx::PxBounds3			m_meshBounds;
	physx::PxMat44				m_internalTransform;
	Mat4Real					m_internalTransformInverse;
	bool						m_incidentalMesh;


	// Unique splitting planes
	physx::Array<Plane>			m_planes;

	// Combination data
	bool						m_combined;
	Real						m_combiningMeshSize;
	bool						m_combiningIncidentalMesh;

	// Memory cache
	class BSPMemCache*			m_memCache;
	bool						m_ownsMemCache;
};


// Surface iterator; walks from a leaf's parent to the root of a tree, allowing inspection of surfaces along the way
class SurfaceIt
{
public:
	PX_INLINE					SurfaceIt() : m_current(NULL), m_side(0xFFFFFFFF)				{}
	PX_INLINE					SurfaceIt(const BSP::Node* leaf) : m_current((BSP::Node*)leaf)
	{
		PX_ASSERT(leaf != NULL && leaf->getType() == BSP::Node::Leaf);
		inc();
	}

	PX_INLINE	bool			valid()	const
	{
		return m_current != NULL;
	}

	PX_INLINE	void			inc()
	{
		m_side = m_current->getIndex();
		m_current = m_current->getParent();
	}

	PX_INLINE	const Surface*	surface()	const
	{
		return m_current->getBranchData();
	}

	PX_INLINE	uint32_t	side()		const
	{
		return m_side;
	}

private:
	BSP::Node*			m_current;
	uint32_t		m_side;
};


// IBSPMemCache implementation, several pools and growable arrays.  Not global, so that concurrent calculations can use different pools
class BSPMemCache : public IApexBSPMemCache, public nvidia::UserAllocated
{
public:

	BSPMemCache();

	void	clearAll();
	void	clearTemp();

	void	release();

	// Persistent data
	nvidia::Pool<BSP::Node>		m_nodePool;

	// Temporary data
	nvidia::Pool<LinkedVertex>	m_linkedVertexPool;
	physx::Array<uint8_t>	m_surfaceFlags;
	physx::Array<uint8_t>	m_surfaceTestFlags;
};


// Mesh cleaning interface
void
cleanMesh(physx::Array<nvidia::ExplicitRenderTriangle>& cleanedMesh, const physx::Array<Triangle>& mesh, physx::Array<ClippedTriangleInfo>& triangleInfo, const physx::Array<Plane>& planes, const physx::Array<Triangle>& originalTriangles, const physx::Array<Interpolator>& frames, Real distanceTol, const Mat4Real& BSPToMeshTM);

};	// namespace ApexCSG

#endif

#endif // #define APEX_CSG_DEFS_H
