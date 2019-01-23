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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef RAW_LOADER_H
#define RAW_LOADER_H

#include "SampleAllocator.h"
#include "foundation/PxTransform.h"

namespace SampleRenderer
{
	class RendererColor;
}

	class RAWObject : public SampleAllocateable
	{
		public:
					RAWObject();

		const char*	mName;
		PxTransform	mTransform;
	};

	class RAWTexture : public RAWObject
	{
		public:
						RAWTexture();

		PxU32			mID;
		PxU32			mWidth;
		PxU32			mHeight;
		bool			mHasAlpha;
		const SampleRenderer::RendererColor*	mPixels;
	};

	class RAWMesh : public RAWObject
	{
		public:
						RAWMesh();

		PxU32			mNbVerts;
		PxU32			mNbFaces;
		PxU32			mMaterialID;
		const PxVec3*	mVerts;
		const PxVec3*	mVertexNormals;
		const PxVec3*	mVertexColors;
		const PxReal*	mUVs;
		const PxU32*	mIndices;
	};

	class RAWShape : public RAWObject
	{
		public:
						RAWShape();

		PxU32			mNbVerts;
		const PxVec3*	mVerts;
	};

	class RAWHelper : public RAWObject
	{
		public:
						RAWHelper();
	};

	class RAWMaterial : public SampleAllocateable
	{
		public:
						RAWMaterial();

		PxU32			mID;
		PxU32			mDiffuseID;
		PxReal			mOpacity;
		PxVec3			mAmbientColor;
		PxVec3			mDiffuseColor;
		PxVec3			mSpecularColor;
		bool			mDoubleSided;
	};

	class RAWImportCallback
	{
		public:
		virtual ~RAWImportCallback() {}
		virtual	void	newMaterial(const RAWMaterial&)	= 0;
		virtual	void	newMesh(const RAWMesh&)			= 0;
		virtual	void	newShape(const RAWShape&)		= 0;
		virtual	void	newHelper(const RAWHelper&)		= 0;
		virtual	void	newTexture(const RAWTexture&)	= 0;
	};

	bool loadRAWfile(const char* filename, RAWImportCallback& cb, PxReal scale);

#endif
