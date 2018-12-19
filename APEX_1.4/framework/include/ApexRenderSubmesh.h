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



#ifndef APEX_RENDER_SUBMESH_H
#define APEX_RENDER_SUBMESH_H

#include "RenderMeshAssetIntl.h"
#include "ApexVertexBuffer.h"
#include "SubmeshParameters.h"

namespace nvidia
{
namespace apex
{

class ApexRenderSubmesh : public RenderSubmeshIntl, public UserAllocated
{
public:
	ApexRenderSubmesh() : mParams(NULL) {}
	~ApexRenderSubmesh() {}

	// from RenderSubmesh
	virtual uint32_t				getVertexCount(uint32_t partIndex) const
	{
		return mParams->vertexPartition.buf[partIndex + 1] - mParams->vertexPartition.buf[partIndex];
	}

	virtual const VertexBufferIntl&	getVertexBuffer() const
	{
		return mVertexBuffer;
	}

	virtual uint32_t				getFirstVertexIndex(uint32_t partIndex) const
	{
		return mParams->vertexPartition.buf[partIndex];
	}

	virtual uint32_t				getIndexCount(uint32_t partIndex) const
	{
		return mParams->indexPartition.buf[partIndex + 1] - mParams->indexPartition.buf[partIndex];
	}

	virtual const uint32_t*			getIndexBuffer(uint32_t partIndex) const
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	virtual const uint32_t*			getSmoothingGroups(uint32_t partIndex) const
	{
		return mParams->smoothingGroups.buf != NULL ? (mParams->smoothingGroups.buf + mParams->indexPartition.buf[partIndex]/3) : NULL;
	}


	// from RenderSubmeshIntl
	virtual VertexBufferIntl&			getVertexBufferWritable()
	{
		return mVertexBuffer;
	}

	virtual uint32_t*						getIndexBufferWritable(uint32_t partIndex)
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	virtual void						applyPermutation(const Array<uint32_t>& old2new, const Array<uint32_t>& new2old);

	// own methods

	uint32_t						getTotalIndexCount() const
	{
		return (uint32_t)mParams->indexBuffer.arraySizes[0];
	}

	uint32_t*						getIndexBufferWritable(uint32_t partIndex) const
	{
		return mParams->indexBuffer.buf + mParams->indexPartition.buf[partIndex];
	}

	bool								createFromParameters(SubmeshParameters* params);

	void								setParams(SubmeshParameters* submeshParams, VertexBufferParameters* vertexBufferParams);

	void								addStats(RenderMeshAssetStats& stats) const;

	void								buildVertexBuffer(const VertexFormat& format, uint32_t vertexCount);

	SubmeshParameters*  mParams;

private:
	ApexVertexBuffer    mVertexBuffer;

	// No assignment
	ApexRenderSubmesh&					operator = (const ApexRenderSubmesh&);
};

} // namespace apex
} // namespace nvidia

#endif // APEX_RENDER_SUBMESH_H
