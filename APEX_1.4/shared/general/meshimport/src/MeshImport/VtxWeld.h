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



#ifndef VTX_WELD_H

#define VTX_WELD_H

#include "MeshImport.h"
#include "MiPlatformConfig.h"

namespace mimp
{

template <class Type> class VertexLess
{
public:
	typedef STDNAME::vector< Type > VertexVector;

	bool operator()(MiI32 v1,MiI32 v2) const;

	static void SetSearch(const Type& match,VertexVector *list)
	{
		mFind = match;
		mList = list;
	};

private:
	const Type& Get(MiI32 index) const
	{
		if ( index == -1 ) return mFind;
		VertexVector &vlist = *mList;
		return vlist[(MiU32)index];
	}
	static Type mFind; // vertice to locate.
	static VertexVector  *mList;
};

template <class Type> class VertexPool : public mimp::MeshImportAllocated
{
public:
  typedef STDNAME::set<MiI32, VertexLess<Type> > VertexSet;
	typedef STDNAME::vector< Type > VertexVector;

	MiI32 GetVertex(const Type& vtx)
	{
		VertexLess<Type>::SetSearch(vtx,&mVtxs);
		typename VertexSet::iterator found;
		found = mVertSet.find( -1 );
		if ( found != mVertSet.end() )
		{
			return *found;
		}
		MiI32 idx = (MiI32)mVtxs.size();
		mVtxs.push_back( vtx );
		mVertSet.insert( idx );
		return idx;
	};

	void GetPos(MiI32 idx,MiF32 pos[3]) const
	{
		pos[0] = mVtxs[idx].mPos[0];
    pos[1] = mVtxs[idx].mPos[1];
    pos[2] = mVtxs[idx].mPos[2];
	}

	const Type& Get(MiI32 idx) const
	{
		return mVtxs[idx];
	};

	MiI32 GetSize(void) const
	{
		return (MiI32)mVtxs.size();
	};

	void Clear(MiI32 reservesize)  // clear the vertice pool.
	{
		mVertSet.clear();
		mVtxs.clear();
		mVtxs.reserve(reservesize);
	};

	const VertexVector& GetVertexList(void) const { return mVtxs; };

	void Set(const Type& vtx)
	{
		mVtxs.push_back(vtx);
	}

	MiI32 GetVertexCount(void) const
	{
		return (MiI32)mVtxs.size();
	};

	bool GetVertex(MiI32 i,MiF32 vect[3]) const
	{
		vect[0] = mVtxs[i].mPos[0];
    vect[1] = mVtxs[i].mPos[1];
    vect[2] = mVtxs[i].mPos[2];
		return true;
	};


	Type * GetBuffer(void)
	{
		return &mVtxs[0];
	};
private:
	VertexSet      mVertSet; // ordered list.
	VertexVector   mVtxs;  // set of vertices.
};

}; // end of namespace

#endif
