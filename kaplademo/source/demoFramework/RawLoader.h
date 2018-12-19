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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef RAW_LOADER_H
#define RAW_LOADER_H

#include "foundation/PxTransform.h"

class RAWObject 
{
public:
	RAWObject();

	const char*	mName;
	physx::PxTransform	mTransform;
};

class RAWTexture : public RAWObject
{
public:
	RAWTexture();

	physx::PxU32			mID;
	physx::PxU32			mWidth;
	physx::PxU32			mHeight;
	bool					mHasAlpha;
	physx::PxU32*					mPixels; //b, g, r, a(blue, green, red, alpha)
};

class RAWMesh : public RAWObject
{
public:
	RAWMesh();

	physx::PxU32			mNbVerts;
	physx::PxU32			mNbFaces;
	physx::PxU32			mMaterialID;
	const physx::PxVec3*	mVerts;
	const physx::PxVec3*	mVertexNormals;
	const physx::PxVec3*	mVertexColors;
	const physx::PxReal*	mUVs;
	const physx::PxU32*	mIndices;
};

class RAWShape : public RAWObject
{
public:
	RAWShape();

	physx::PxU32			mNbVerts;
	const physx::PxVec3*	mVerts;
};

class RAWHelper : public RAWObject
{
public:
	RAWHelper();
};

class RAWMaterial
{
public:
	RAWMaterial();

	physx::PxU32			mID;
	physx::PxU32			mDiffuseID;
	physx::PxReal			mOpacity;
	physx::PxVec3			mAmbientColor;
	physx::PxVec3			mDiffuseColor;
	physx::PxVec3			mSpecularColor;
	bool			mDoubleSided;
};

class RAWImportCallback
{
public:
	virtual ~RAWImportCallback() {}
	virtual	void	newMaterial(const RAWMaterial&) = 0;
	virtual	void	newMesh(const RAWMesh&) = 0;
	virtual	void	newShape(const RAWShape&) = 0;
	virtual	void	newHelper(const RAWHelper&) = 0;
	virtual	void	newTexture(const RAWTexture&) = 0;
};

bool loadRAWfile(const char* filename, RAWImportCallback& cb, physx::PxReal scale);

#endif
