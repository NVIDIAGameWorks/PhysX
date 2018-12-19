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



#include "TextRenderResourceManager.h"

#include "UserRenderResource.h"
#include "UserRenderResourceDesc.h"

#include "RenderContext.h"


TextRenderResourceManager::TextRenderResourceManager() :
mVerbosity(0),
	mOutputFile(NULL),
	mIO(NULL),
	mVertexBufferCount(0),
	mIndexBufferCount(0),
	mBoneBufferCount(0),
	mInstanceBufferCount(0),
	mSpriteBufferCount(0),
	mRenderResourceCount(0),
	mSurfaceBufferCount(0)
{
}


TextRenderResourceManager::TextRenderResourceManager(int verbosity, const char* outputFilename) :
	mVerbosity(verbosity),
	mOutputFile(NULL),
	mVertexBufferCount(0),
	mIndexBufferCount(0),
	mBoneBufferCount(0),
	mInstanceBufferCount(0),
	mSpriteBufferCount(0),
	mRenderResourceCount(0),
	mSurfaceBufferCount(0)
{
	if (outputFilename != NULL)
	{
		mOutputFile = fopen(outputFilename, "w");
	}
	else
	{
		mOutputFile = stdout;
	}
	PX_ASSERT(mOutputFile != NULL);
	
	mIO = new Writer(mOutputFile);
}


TextRenderResourceManager::~TextRenderResourceManager()
{
	if (mOutputFile != NULL && mOutputFile != stdout)
	{
		fclose(mOutputFile);
	}

	if (mIO != NULL)
	{
		delete mIO;
		mIO = NULL;
	}
}

unsigned int TextRenderResourceManager::material2Id(void* material)
{
	std::map<void*, unsigned int>::iterator it = mMaterial2Id.find(material);
	if (it != mMaterial2Id.end())
	{
		return it->second;
	}

	unsigned int result = (unsigned int)mMaterial2Id.size();
	mMaterial2Id[material] = result;

	return result;
}

bool TextRenderResourceManager::getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, nvidia::apex::UserRenderSpriteBufferDesc* bufferDesc)
{
	PX_UNUSED(spriteCount);
	PX_UNUSED(spriteSemanticsBitmap);
	PX_UNUSED(bufferDesc);
	return false;
}

bool TextRenderResourceManager::getInstanceLayoutData(uint32_t particleCount, uint32_t particleSemanticsBitmap, nvidia::apex::UserRenderInstanceBufferDesc* bufferDesc)
{
	PX_UNUSED(particleCount);
	PX_UNUSED(particleSemanticsBitmap);
	PX_UNUSED(bufferDesc);
	return false;
}



Writer::Writer(FILE* outputFile) : mOutputFile(outputFile)
{
	mIsStdout = mOutputFile == stdout;
}

Writer::~Writer() {}

void Writer::printAndScan(const char* format)
{
	fprintf(mOutputFile,"%s", format);
}

void Writer::printAndScan(const char* format, const char* arg)
{
	fprintf(mOutputFile, format, arg);
}

void Writer::printAndScan(const char* format, int arg)
{
	fprintf(mOutputFile, format, arg);
}

void Writer::printAndScan(float /*tol*/, nvidia::apex::RenderVertexSemantic::Enum /*s*/, const char* format, float arg)
{
	fprintf(mOutputFile, format, arg);
}

const char* Writer::semanticToString(nvidia::apex::RenderVertexSemantic::Enum semantic)
{
	const char* result = NULL;
	switch (semantic)
	{
#define CASE(_SEMANTIC) case nvidia::apex::RenderVertexSemantic::_SEMANTIC: result = #_SEMANTIC; break
		CASE(POSITION);
		CASE(NORMAL);
		CASE(TANGENT);
		CASE(BINORMAL);
		CASE(COLOR);
		CASE(TEXCOORD0);
		CASE(TEXCOORD1);
		CASE(TEXCOORD2);
		CASE(TEXCOORD3);
		CASE(BONE_INDEX);
		CASE(BONE_WEIGHT);
#undef CASE

	default:
		PX_ALWAYS_ASSERT();
	}

	return result;
}



const char* Writer::semanticToString(nvidia::apex::RenderBoneSemantic::Enum semantic)
{
	const char* result = NULL;
	switch (semantic)
	{
#define CASE(_SEMANTIC) case nvidia::apex::RenderBoneSemantic::_SEMANTIC: result = #_SEMANTIC; break
		CASE(POSE);
		CASE(PREVIOUS_POSE);
#undef CASE

	// if this assert is hit add/remove semantics above to match RenderBoneSemantic
	PX_COMPILE_TIME_ASSERT(nvidia::apex::RenderBoneSemantic::NUM_SEMANTICS == 2);

	default:
		PX_ALWAYS_ASSERT();
	}

	return result;
}



void Writer::writeElem(const char* name, unsigned int val)
{
	const bool isStdout = mIsStdout;

	if (isStdout)
	{
		printAndScan("\n  ");
	}
	printAndScan("%s=", name);
	printAndScan("%d", (int)val);
	if (!isStdout)
	{
		printAndScan(" ");
	}
}



void Writer::writeArray(nvidia::apex::RenderDataFormat::Enum format, unsigned int stride, unsigned int numElements, const void* data, float tolerance, nvidia::apex::RenderVertexSemantic::Enum s)
{
	if (mIsStdout)
	{
		unsigned int maxNumElements = 1000000;

		switch (format)
		{
		case nvidia::apex::RenderDataFormat::USHORT1:
		case nvidia::apex::RenderDataFormat::UINT1:
			maxNumElements = 10;
			break;
		case nvidia::apex::RenderDataFormat::FLOAT2:
			maxNumElements = 3;
			break;
		case nvidia::apex::RenderDataFormat::FLOAT3:
			maxNumElements = 2;
			break;
		case nvidia::apex::RenderDataFormat::FLOAT3x4:
		case nvidia::apex::RenderDataFormat::FLOAT4x4:
			maxNumElements = 1;
			break;
		default:
			PX_ALWAYS_ASSERT();
		}

		if (maxNumElements < numElements)
		{
			numElements = maxNumElements;
		}
	}

	for (unsigned int i = 0; i < numElements; i++)
	{
		const char* dataSample = ((const char*)data) + stride * i;

		switch (format)
		{
		case nvidia::apex::RenderDataFormat::USHORT1:
		{
			const short* sh = (const short*)dataSample;
			printAndScan("%d ", sh[0]);
		}
		break;
		case nvidia::apex::RenderDataFormat::USHORT2:
		{
			const short* sh = (const short*)dataSample;
			printAndScan("(%d ", sh[0]);
			printAndScan("%d )", sh[1]);
		}
		break;
		case nvidia::apex::RenderDataFormat::USHORT3:
			{
				const short* sh = (const short*)dataSample;
				printAndScan("(%d ", sh[0]);
				printAndScan("%d ", sh[2]);
				printAndScan("%d )", sh[1]);
			}
			break;
		case nvidia::apex::RenderDataFormat::USHORT4:
		{
			const short* sh = (const short*)dataSample;
			printAndScan("(%d ", sh[0]);
			printAndScan("%d ", sh[1]);
			printAndScan("%d ", sh[2]);
			printAndScan("%d )", sh[3]);
		}
		break;
		case nvidia::apex::RenderDataFormat::UINT1:
		{
			const unsigned int* ui = (const unsigned int*)dataSample;
			printAndScan("%d ", (int)ui[0]);
		}
		break;
		case nvidia::apex::RenderDataFormat::FLOAT2:
		{
			const float* fl = (const float*)dataSample;
			printAndScan(tolerance, s, "(%f ", fl[0]);
			printAndScan(tolerance, s, "%f ) ", fl[1]);
		}
		break;
		case nvidia::apex::RenderDataFormat::FLOAT3:
		{
			const physx::PxVec3* vec3 = (const physx::PxVec3*)dataSample;
			printAndScan(tolerance, s, "(%f ", vec3->x);
			printAndScan(tolerance, s, "%f ", vec3->y);
			printAndScan(tolerance, s, "%f ) ", vec3->z);
		}
		break;
		case nvidia::apex::RenderDataFormat::FLOAT4:
		{
			const physx::PxVec4* vec4 = (const physx::PxVec4*)dataSample;
			printAndScan(tolerance, s, "(%f ", vec4->x);
			printAndScan(tolerance, s, "%f ", vec4->y);
			printAndScan(tolerance, s, "%f ", vec4->z);
			printAndScan(tolerance, s, "%f ) ", vec4->w);
		}
		break;
		case nvidia::apex::RenderDataFormat::FLOAT3x4:
		{
			const physx::PxVec3* vec3 = (const physx::PxVec3*)dataSample;
			printAndScan("(");
			for (unsigned int j = 0; j < 4; j++)
			{
				printAndScan(tolerance, s, "%f ", vec3[j].x);
				printAndScan(tolerance, s, "%f ", vec3[j].y);
				printAndScan(tolerance, s, "%f ", vec3[j].z);
			}
			printAndScan(") ");
		}
		break;
		case nvidia::apex::RenderDataFormat::FLOAT4x4:
			{
				const physx::PxVec4* vec4 = (const physx::PxVec4*)dataSample;
				printAndScan("(");
				for (unsigned int j = 0; j < 4; j++)
				{
					printAndScan(tolerance, s, "%f ", vec4[j].x);
					printAndScan(tolerance, s, "%f ", vec4[j].y);
					printAndScan(tolerance, s, "%f ", vec4[j].z);
					printAndScan(tolerance, s, "%f ", vec4[j].w);
				}
				printAndScan(") ");
			}
			break;
		case nvidia::apex::RenderDataFormat::R8G8B8A8:
		case nvidia::apex::RenderDataFormat::B8G8R8A8:
			{
				const unsigned int* ui = (const unsigned int*)dataSample;
				printAndScan("0x%x ", (int)ui[0]);
			}
		break;
		default:
			PX_ALWAYS_ASSERT();
		}
	}
}


#define WRITE_ITEM(_A) mIO->writeElem(#_A, _A)
#define WRITE_DESC_ELEM(_A) mIO->writeElem(#_A, (uint32_t)desc._A)
#define WRITE_VREQUEST(_A) if (desc.buffersRequest[nvidia::apex::RenderVertexSemantic::_A] != nvidia::apex::RenderDataFormat::UNSPECIFIED) mIO->writeElem(#_A, desc.buffersRequest[nvidia::apex::RenderVertexSemantic::_A])
#define WRITE_BREQUEST(_A) if (desc.buffersRequest[nvidia::apex::RenderBoneSemantic::_A] != nvidia::apex::RenderDataFormat::UNSPECIFIED) mIO->writeElem(#_A, desc.buffersRequest[nvidia::apex::RenderBoneSemantic::_A])











class TextUserRenderVertexBuffer : public nvidia::apex::UserRenderVertexBuffer
{
public:
	TextUserRenderVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mWriteCalls(0), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("VertexBuffer[%d]::create(", mBufferId);

			WRITE_DESC_ELEM(maxVerts);
			WRITE_DESC_ELEM(hint);
			WRITE_VREQUEST(POSITION);
			WRITE_VREQUEST(NORMAL);
			WRITE_VREQUEST(TANGENT);
			WRITE_VREQUEST(BINORMAL);
			WRITE_VREQUEST(COLOR);
			WRITE_VREQUEST(TEXCOORD0);
			WRITE_VREQUEST(TEXCOORD1);
			WRITE_VREQUEST(TEXCOORD2);
			WRITE_VREQUEST(TEXCOORD3);
			WRITE_VREQUEST(BONE_INDEX);
			WRITE_VREQUEST(BONE_WEIGHT);
			// PH: not done on purpose (yet)
			//WRITE_DESC_ELEM(numCustomBuffers);
			//void** 						customBuffersIdents;
			//RenderDataFormat::Enum*	customBuffersRequest;
			WRITE_DESC_ELEM(moduleIdentifier);
			WRITE_DESC_ELEM(uvOrigin);
			WRITE_DESC_ELEM(canBeShared);

			mIO->printAndScan(")\n");
		}
	}

	~TextUserRenderVertexBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("VertexBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const nvidia::RenderVertexBufferData& data, uint32_t firstVertex, uint32_t numVertices)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("VertexBuffer[%d]", mBufferId);
			mIO->printAndScan("::writeBuffer#%d", (int)mWriteCalls);
			mIO->printAndScan("(firstVertex=%d ", (int)firstVertex);
			mIO->printAndScan("numVertices=%d)\n", (int)numVertices);

			if (mManager->getVerbosity() >= 3)
			{
				for (unsigned int i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
				{
					nvidia::apex::RenderVertexSemantic::Enum s = nvidia::apex::RenderVertexSemantic::Enum(i);
					const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(s);

					if (semanticData.data != NULL)
					{
						mIO->printAndScan(" %s: ", mIO->semanticToString(s));
						mIO->writeArray(semanticData.format, semanticData.stride, numVertices, semanticData.data, mManager->getVBTolerance(s), s);
						mIO->printAndScan("\n");
					}
				}
			}
		}

		mWriteCalls++;
	}


	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}


protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	int mBufferId;
	unsigned int mWriteCalls;
	nvidia::apex::UserRenderVertexBufferDesc mDescriptor;
};




nvidia::apex::UserRenderVertexBuffer* TextRenderResourceManager::createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc)
{
	nvidia::apex::UserRenderVertexBuffer* vb = new TextUserRenderVertexBuffer(desc, this, mIO, mVertexBufferCount++);
	return vb;
};



void TextRenderResourceManager::releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer)
{
	delete &buffer;
}


















class TextUserRenderIndexBuffer : public nvidia::apex::UserRenderIndexBuffer
{
public:
	TextUserRenderIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mWriteCalls(0), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("IndexBuffer[%d]::create(", mBufferId);

			WRITE_DESC_ELEM(maxIndices);
			WRITE_DESC_ELEM(hint);
			WRITE_DESC_ELEM(format);
			WRITE_DESC_ELEM(primitives);
			WRITE_DESC_ELEM(registerInCUDA);

			mIO->printAndScan(")\n");
		}
	}

	~TextUserRenderIndexBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("IndexBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const void* srcData, uint32_t srcStride, uint32_t firstDestElement, uint32_t numElements)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("IndexBuffer[%d]", mBufferId);
			mIO->printAndScan("::writeBuffer#%d", (int)mWriteCalls);
			mIO->printAndScan("(srcStride=%d ", (int)srcStride);
			mIO->printAndScan("firstDestElement=%d ", (int)firstDestElement);
			mIO->printAndScan("numElements=%d)\n ", (int)numElements);

			mIO->writeArray(mDescriptor.format, srcStride, numElements, srcData, 0.0f, nvidia::apex::RenderVertexSemantic::CUSTOM);

			mIO->printAndScan("\n");
		}

		mWriteCalls++;
	}


	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	int mBufferId;
	unsigned int mWriteCalls;
	nvidia::apex::UserRenderIndexBufferDesc mDescriptor;
};



nvidia::apex::UserRenderIndexBuffer* TextRenderResourceManager::createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc)
{
	nvidia::apex::UserRenderIndexBuffer* ib = new TextUserRenderIndexBuffer(desc, this, mIO, mIndexBufferCount++);
	return ib;
}



void TextRenderResourceManager::releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer)
{
	delete &buffer;
}


















class TextUserRenderBoneBuffer : public nvidia::apex::UserRenderBoneBuffer
{
public:
	TextUserRenderBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mWriteCalls(0), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("BoneBuffer[%d]::create(", mBufferId);
			WRITE_DESC_ELEM(maxBones);
			WRITE_DESC_ELEM(hint);
			WRITE_BREQUEST(POSE);
			mIO->printAndScan(")\n");
		}
	}

	~TextUserRenderBoneBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("BoneBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const nvidia::RenderBoneBufferData& data, uint32_t firstBone, uint32_t numBones)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("BoneBuffer[%d]", mBufferId);
			mIO->printAndScan("::writeBuffer#%d", (int)mWriteCalls);
			mIO->printAndScan("(firstBone=%d ", (int)firstBone);
			mIO->printAndScan("numBones=%d)\n", (int)numBones);

			for (unsigned int i = 0; i < nvidia::apex::RenderBoneSemantic::NUM_SEMANTICS; i++)
			{
				nvidia::apex::RenderBoneSemantic::Enum s = nvidia::apex::RenderBoneSemantic::Enum(i);
				const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(s);

				if (semanticData.data != NULL)
				{
					mIO->printAndScan(" %s: ", mIO->semanticToString(s));
					mIO->writeArray(semanticData.format, semanticData.stride, numBones, semanticData.data, mManager->getBonePoseTolerance(), nvidia::apex::RenderVertexSemantic::CUSTOM);
					mIO->printAndScan("\n");
				}
			}
		}

		mWriteCalls++;
	}


	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	int mBufferId;
	unsigned int mWriteCalls;
	nvidia::apex::UserRenderBoneBufferDesc mDescriptor;
};



nvidia::apex::UserRenderBoneBuffer* TextRenderResourceManager::createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc)
{
	nvidia::apex::UserRenderBoneBuffer* bb = new TextUserRenderBoneBuffer(desc, this, mIO, mBoneBufferCount++);
	return bb;
}



void TextRenderResourceManager::releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer)
{
	delete &buffer;
}















class TextUserRenderInstanceBuffer : public nvidia::apex::UserRenderInstanceBuffer
{
public:
	TextUserRenderInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("InstanceBuffer[%d]::create\n", mBufferId);
		}
	}

	~TextUserRenderInstanceBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("InstanceBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const void* /*data*/, uint32_t /*firstInstance*/, uint32_t /*numInstances*/)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("InstanceBuffer[%d]::writeBuffer\n", mBufferId);

		}
	}

	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	FILE* mOutputFile;
	int mBufferId;
	nvidia::apex::UserRenderInstanceBufferDesc mDescriptor;
};



nvidia::apex::UserRenderInstanceBuffer* TextRenderResourceManager::createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc)
{
	nvidia::apex::UserRenderInstanceBuffer* ib = new TextUserRenderInstanceBuffer(desc, this, mIO, mInstanceBufferCount++);
	return ib;
}



void TextRenderResourceManager::releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer)
{
	delete &buffer;
}




















class TextUserRenderSpriteBuffer : public nvidia::apex::UserRenderSpriteBuffer
{
public:
	TextUserRenderSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("SpriteBuffer[%d]::create\n", mBufferId);
		}
	}

	~TextUserRenderSpriteBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("SpriteBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const void* /*data*/, uint32_t /*firstSprite*/, uint32_t /*numSprites*/)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("SpriteBuffer[%d]::writeBuffer\n", mBufferId);
		}
	}


	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	FILE* mOutputFile;
	int mBufferId;
	nvidia::apex::UserRenderSpriteBufferDesc mDescriptor;
};



nvidia::apex::UserRenderSpriteBuffer* TextRenderResourceManager::createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc)
{
	nvidia::apex::UserRenderSpriteBuffer* sb = new TextUserRenderSpriteBuffer(desc, this, mIO, mSpriteBufferCount++);
	return sb;
}



void TextRenderResourceManager::releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer)
{
	delete &buffer;
}





class TextUserRenderSurfaceBuffer : public nvidia::apex::UserRenderSurfaceBuffer
{
public:
	TextUserRenderSurfaceBuffer(const nvidia::apex::UserRenderSurfaceBufferDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("SurfaceBuffer[%d]::create\n", mBufferId);
		}
	}

	~TextUserRenderSurfaceBuffer()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("SurfaceBuffer[%d]::destroy\n", mBufferId);
		}
	}

	virtual void writeBuffer(const void* /*srcData*/, uint32_t /*srcPitch*/, uint32_t /*srcHeight*/, uint32_t /*dstX*/, uint32_t /*dstY*/, uint32_t /*dstZ*/, uint32_t /*width*/, uint32_t /*height*/, uint32_t /*depth*/)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("SurfaceBuffer[%d]::writeBuffer\n", mBufferId);
		}
	}


	unsigned int getId()
	{
		return (uint32_t)mBufferId;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	FILE* mOutputFile;
	int mBufferId;
	nvidia::apex::UserRenderSurfaceBufferDesc mDescriptor;
};


nvidia::apex::UserRenderSurfaceBuffer*	 TextRenderResourceManager::createSurfaceBuffer( const nvidia::apex::UserRenderSurfaceBufferDesc &desc )
{
	nvidia::apex::UserRenderSurfaceBuffer* sb = new TextUserRenderSurfaceBuffer(desc, this, mIO, mSurfaceBufferCount++);
	return sb;
}



void TextRenderResourceManager::releaseSurfaceBuffer( nvidia::apex::UserRenderSurfaceBuffer &buffer )
{
	delete &buffer;
}











class TextUserRenderResource : public nvidia::apex::UserRenderResource
{
public:
	TextUserRenderResource(const nvidia::apex::UserRenderResourceDesc& desc, TextRenderResourceManager* manager, Writer* readerWriter, int bufferId) : mManager(manager), mIO(readerWriter), mRenderCount(0), mDescriptor(desc)
	{
		mBufferId = bufferId;

		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("RenderResource[%d]::create(", mBufferId);

			for (unsigned int i = 0; i < mDescriptor.numVertexBuffers; i++)
			{
				TextUserRenderVertexBuffer* vb = static_cast<TextUserRenderVertexBuffer*>(mDescriptor.vertexBuffers[i]);
				mIO->writeElem("VertexBuffer", vb->getId());
			}

			if (mDescriptor.indexBuffer != NULL)
			{
				TextUserRenderIndexBuffer* ib = static_cast<TextUserRenderIndexBuffer*>(mDescriptor.indexBuffer);
				mIO->writeElem("IndexBuffer", ib->getId());
			}

			if (mDescriptor.boneBuffer != NULL)
			{
				TextUserRenderBoneBuffer* bb = static_cast<TextUserRenderBoneBuffer*>(mDescriptor.boneBuffer);
				mIO->writeElem("BoneBuffer", bb->getId());
			}

			if (mDescriptor.instanceBuffer != NULL)
			{
				TextUserRenderInstanceBuffer* ib = static_cast<TextUserRenderInstanceBuffer*>(mDescriptor.instanceBuffer);
				mIO->writeElem("InstanceBuffer", ib->getId());
			}

			if (mDescriptor.spriteBuffer != NULL)
			{
				TextUserRenderSpriteBuffer* sp = static_cast<TextUserRenderSpriteBuffer*>(mDescriptor.spriteBuffer);
				mIO->writeElem("SpriteBuffer", sp->getId());
			}

			{
				unsigned int materialId = manager->material2Id(mDescriptor.material);
				mIO->writeElem("Material", materialId);
			}

			WRITE_DESC_ELEM(numVertexBuffers);
			WRITE_DESC_ELEM(firstVertex);
			WRITE_DESC_ELEM(numVerts);
			WRITE_DESC_ELEM(firstIndex);
			WRITE_DESC_ELEM(numIndices);
			WRITE_DESC_ELEM(firstBone);
			WRITE_DESC_ELEM(numBones);
			WRITE_DESC_ELEM(firstInstance);
			WRITE_DESC_ELEM(numInstances);
			WRITE_DESC_ELEM(firstSprite);
			WRITE_DESC_ELEM(numSprites);
			WRITE_DESC_ELEM(submeshIndex);
			WRITE_DESC_ELEM(cullMode);
			WRITE_DESC_ELEM(primitives);

			mIO->printAndScan(")\n");
		}
	}

	~TextUserRenderResource()
	{
		if (mManager->getVerbosity() >= 1)
		{
			mIO->printAndScan("RenderResource[%d]::destroy\n", mBufferId);
		}
	}



	void setVertexBufferRange(unsigned int firstVertex, unsigned int numVerts)
	{
		mDescriptor.firstVertex = firstVertex;
		mDescriptor.numVerts = numVerts;

		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::setVertexBufferRange(firstVertex=%d", (int)firstVertex);
			mIO->printAndScan(" numVerts=%d)\n", (int)numVerts);
		}
	}



	void setIndexBufferRange(unsigned int firstIndex, unsigned int numIndices)
	{
		mDescriptor.firstIndex = firstIndex;
		mDescriptor.numIndices = numIndices;

		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::setIndexBufferRange(firstIndex=%d ", (int)firstIndex);
			mIO->printAndScan("numIndices=%d)\n", (int)numIndices);
		}
	}



	void setBoneBufferRange(unsigned int firstBone, unsigned int numBones)
	{
		mDescriptor.firstBone = firstBone;
		mDescriptor.numBones = numBones;

		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::setBoneBufferRange(firstBone=%d ", (int)firstBone);
			mIO->printAndScan("numBones=%d)\n", (int)numBones);
		}
	}



	void setInstanceBufferRange(unsigned int firstInstance, unsigned int numInstances)
	{
		mDescriptor.firstInstance = firstInstance;
		mDescriptor.numInstances = numInstances;

		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::setInstanceBufferRange(firstInstance=%d ", (int)firstInstance);
			mIO->printAndScan("numInstances=%d)\n", (int)numInstances);
		}
	}



	void setSpriteBufferRange(unsigned int firstSprite, unsigned int numSprites)
	{
		mDescriptor.firstSprite = firstSprite;
		mDescriptor.numSprites = numSprites;

		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("(firstSprite=%d ", (int)firstSprite);
			mIO->printAndScan("numSprites=%d)\n", (int)numSprites);
		}
	}



	void setMaterial(void* material)
	{
		mDescriptor.material = material;

		if (mManager->getVerbosity() >= 2 && material != mDescriptor.material)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::setMaterial(material=%d)\n", (int)mManager->material2Id(material));
		}
	}



	unsigned int getNbVertexBuffers() const
	{
		return mDescriptor.numVertexBuffers;
	}



	nvidia::apex::UserRenderVertexBuffer* getVertexBuffer(unsigned int index) const
	{
		return mDescriptor.vertexBuffers[index];
	}



	nvidia::apex::UserRenderIndexBuffer* getIndexBuffer() const
	{
		return mDescriptor.indexBuffer;
	}




	nvidia::apex::UserRenderBoneBuffer* getBoneBuffer()	const
	{
		return mDescriptor.boneBuffer;
	}



	nvidia::apex::UserRenderInstanceBuffer* getInstanceBuffer()	const
	{
		return mDescriptor.instanceBuffer;
	}



	nvidia::apex::UserRenderSpriteBuffer* getSpriteBuffer() const
	{
		return mDescriptor.spriteBuffer;
	}



	void render(const nvidia::apex::RenderContext& context)
	{
		if (mManager->getVerbosity() >= 2)
		{
			mIO->printAndScan("RenderResource[%d]", mBufferId);
			mIO->printAndScan("::render#%d(", (int)mRenderCount);

			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					mIO->printAndScan(mManager->getRenderPoseTolerance(), nvidia::apex::RenderVertexSemantic::CUSTOM, "%f ", context.local2world[i][j]);
				}
			}

			mIO->printAndScan(")\n");
		}

		mRenderCount++;
	}

protected:
	TextRenderResourceManager* mManager;
	Writer* mIO;
	int mBufferId;
	unsigned int mRenderCount;
	nvidia::apex::UserRenderResourceDesc mDescriptor;
};



nvidia::apex::UserRenderResource* TextRenderResourceManager::createResource(const nvidia::apex::UserRenderResourceDesc& desc)
{
	nvidia::apex::UserRenderResource* rr = new TextUserRenderResource(desc, this, mIO, mRenderResourceCount++);
	return rr;
}



void TextRenderResourceManager::releaseResource(nvidia::apex::UserRenderResource& resource)
{
	delete &resource;
}



unsigned int TextRenderResourceManager::getMaxBonesForMaterial(void* /*material*/)
{
	return 0;
}



void TextUserRenderer::renderResource(const nvidia::apex::RenderContext& context)
{
	TextUserRenderResource* rr = static_cast<TextUserRenderResource*>(context.renderResource);
	rr->render(context);
}
