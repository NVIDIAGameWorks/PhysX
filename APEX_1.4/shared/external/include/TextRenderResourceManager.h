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



#ifndef TEXT_RENDER_RESOURCE_MANAGER_H
#define TEXT_RENDER_RESOURCE_MANAGER_H

#include "UserRenderVertexBuffer.h"
#include "UserRenderIndexBuffer.h"
#include "UserRenderIndexBufferDesc.h"
#include "UserRenderBoneBuffer.h"
#include "UserRenderInstanceBuffer.h"
#include "UserRenderSpriteBuffer.h"
#include "UserRenderSurfaceBuffer.h"

#include "UserRenderResourceManager.h"
#include "UserRenderer.h"

#include <stdio.h>
#include <map>

class Writer
{
public:
	Writer(FILE* outputFile);
	virtual ~Writer();

	virtual void printAndScan(const char* format);
	virtual void printAndScan(const char* format, const char* arg);
	virtual void printAndScan(const char* format, int arg);
	virtual void printAndScan(float tolerance, nvidia::apex::RenderVertexSemantic::Enum s, const char* format, float arg);

	const char* semanticToString(nvidia::apex::RenderVertexSemantic::Enum semantic);
	const char* semanticToString(nvidia::apex::RenderBoneSemantic::Enum semantic);

	void writeElem(const char* name, unsigned int val);
	void writeArray(nvidia::apex::RenderDataFormat::Enum format, unsigned int stride, unsigned int numElements, const void* data, float tolerance, nvidia::apex::RenderVertexSemantic::Enum s);

protected:
	FILE* mReferenceFile;
	FILE* mOutputFile;
	bool mIsStdout;
};



class TextRenderResourceManager : public nvidia::apex::UserRenderResourceManager
{
protected:
	TextRenderResourceManager();

public:

	TextRenderResourceManager(int verbosity, const char* outputFilename);
	~TextRenderResourceManager();

	virtual nvidia::apex::UserRenderVertexBuffer*   createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc);
	virtual void                                     releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer);

	virtual nvidia::apex::UserRenderIndexBuffer*    createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc);
	virtual void                                     releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer);

	virtual nvidia::apex::UserRenderBoneBuffer*     createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc);
	virtual void                                     releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer);

	virtual nvidia::apex::UserRenderInstanceBuffer* createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc);
	virtual void                                     releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer);

	virtual nvidia::apex::UserRenderSpriteBuffer*   createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc);
	virtual void                                     releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer);

	virtual nvidia::apex::UserRenderSurfaceBuffer*	 createSurfaceBuffer( const nvidia::apex::UserRenderSurfaceBufferDesc &desc );
	virtual void									 releaseSurfaceBuffer( nvidia::apex::UserRenderSurfaceBuffer &buffer );

	virtual nvidia::apex::UserRenderResource*       createResource(const nvidia::apex::UserRenderResourceDesc& desc);
	virtual void                                     releaseResource(nvidia::apex::UserRenderResource& resource);

	virtual uint32_t                             getMaxBonesForMaterial(void* material);

	unsigned int									 material2Id(void* material);

	void											 setVerbosity(int v) { mVerbosity = v; }
	int												 getVerbosity() { return mVerbosity; }

	virtual float									 getVBTolerance(nvidia::apex::RenderVertexSemantic::Enum /*s*/) 
	{ 
		return 0.0f; 
	}
	virtual float									 getRenderPoseTolerance() { return 0.0f; }
	virtual float									 getBonePoseTolerance() { return 0.0f; }

	virtual bool									getSpriteLayoutData(uint32_t spriteCount, 
																		uint32_t spriteSemanticsBitmap, 
																		nvidia::apex::UserRenderSpriteBufferDesc* bufferDesc);
	virtual bool									getInstanceLayoutData(uint32_t particleCount, 
																		uint32_t particleSemanticsBitmap, 
																		nvidia::apex::UserRenderInstanceBufferDesc* bufferDesc);
protected:

	int mVerbosity;
	FILE* mOutputFile;
	Writer* mIO;

	std::map<void*, unsigned int> mMaterial2Id;

	int mVertexBufferCount;
	int mIndexBufferCount;
	int mBoneBufferCount;
	int mInstanceBufferCount;
	int mSpriteBufferCount;
	int mRenderResourceCount;
	int mSurfaceBufferCount;
};



class TextUserRenderer : public nvidia::apex::UserRenderer
{
public:
	TextUserRenderer() {}
	virtual ~TextUserRenderer() {}

	virtual void renderResource(const nvidia::apex::RenderContext& context);
};

#endif // TEXT_RENDER_RESOURCE_MANAGER_H
