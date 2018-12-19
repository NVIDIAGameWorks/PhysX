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



#include "RecordingRenderResourceManager.h"

#include <assert.h>
#include <time.h>

#include <vector>

#include "UserRenderVertexBuffer.h"
#include "UserRenderResource.h"
#include "UserRenderResourceDesc.h"
#include "UserRenderIndexBuffer.h"
#include "UserRenderIndexBufferDesc.h"
#include "UserRenderBoneBuffer.h"
#include "UserRenderBoneBufferDesc.h"

#include "RenderContext.h"

#define BREAK_ON_UNIMPLEMENTED 1

#if BREAK_ON_UNIMPLEMENTED
#define UNIMPLEMENTED assert(0)
#else
#define UNIMPLEMENTED /*void&*/
#endif

#if PX_WINDOWS_FAMILY
#define NOMINMAX
#include "windows.h"
#endif

RecordingRenderResourceManager::RecordingRenderResourceManager(nvidia::apex::UserRenderResourceManager* child, bool /*ownsChild*/, RecorderInterface* recorder) : mChild(child), mRecorder(recorder)
{
}



RecordingRenderResourceManager::~RecordingRenderResourceManager()
{
	if (mOwnsChild)
	{
		delete mChild;
	}
	mChild = NULL;

	if (mRecorder != NULL)
	{
		delete mRecorder;
		mRecorder = NULL;
	}
}


class RecordingVertexBuffer : public nvidia::apex::UserRenderVertexBuffer
{
public:
	RecordingVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc, nvidia::apex::UserRenderVertexBuffer* child, RecordingRenderResourceManager::RecorderInterface* recorder) :
		mDescriptor(desc), mChild(child), mBufferId(0), mRecorder(recorder)
	{
		mBufferId = vertexBufferId++;

		if (mRecorder != NULL)
		{
			mRecorder->createVertexBuffer(mBufferId, desc);
		}
	}

	~RecordingVertexBuffer()
	{
		if (mRecorder != NULL)
		{
			mRecorder->releaseVertexBuffer(mBufferId);
		}
	}

	virtual void writeBuffer(const nvidia::apex::RenderVertexBufferData& data, unsigned int firstVertex, unsigned int numVertices)
	{
		if (mChild != NULL)
		{
			mChild->writeBuffer(data, firstVertex, numVertices);
		}

		if (mRecorder != NULL)
		{
			mRecorder->writeVertexBuffer(mBufferId, data, firstVertex, numVertices);
		}
	}

	nvidia::apex::UserRenderVertexBuffer* getChild()
	{
		return mChild;
	}

protected:
	nvidia::apex::UserRenderVertexBufferDesc mDescriptor;
	nvidia::apex::UserRenderVertexBuffer* mChild;

	static unsigned int vertexBufferId;
	unsigned int mBufferId;

	RecordingRenderResourceManager::RecorderInterface* mRecorder;
};

unsigned int RecordingVertexBuffer::vertexBufferId = 0;


nvidia::apex::UserRenderVertexBuffer* RecordingRenderResourceManager::createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc)
{
	nvidia::apex::UserRenderVertexBuffer* child = NULL;
	if (mChild != NULL)
	{
		child = mChild->createVertexBuffer(desc);
	}

	return new RecordingVertexBuffer(desc, child, mRecorder);
}



void RecordingRenderResourceManager::releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseVertexBuffer(*reinterpret_cast<RecordingVertexBuffer&>(buffer).getChild());
	}

	delete &buffer;
}



class RecordingIndexBuffer : public nvidia::apex::UserRenderIndexBuffer
{
public:
	RecordingIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc, nvidia::apex::UserRenderIndexBuffer* child, RecordingRenderResourceManager::RecorderInterface* recorder) :
		mDescriptor(desc), mChild(child), mBufferId(0), mRecorder(recorder)
	{
		mBufferId = indexBufferId++;

		if (mRecorder != NULL)
		{
			mRecorder->createIndexBuffer(mBufferId, desc);
		}
	}

	~RecordingIndexBuffer()
	{
		if (mRecorder != NULL)
		{
			mRecorder->releaseIndexBuffer(mBufferId);
		}
	}

	virtual void writeBuffer(const void* srcData, uint32_t srcStride, unsigned int firstDestElement, unsigned int numElements)
	{
		if (mChild != NULL)
		{
			mChild->writeBuffer(srcData, srcStride, firstDestElement, numElements);
		}

		if (mRecorder != NULL)
		{
			mRecorder->writeIndexBuffer(mBufferId, srcData, srcStride, firstDestElement, numElements, mDescriptor.format);
		}
	}

	nvidia::apex::UserRenderIndexBuffer* getChild()
	{
		return mChild;
	}

protected:
	nvidia::apex::UserRenderIndexBufferDesc mDescriptor;
	nvidia::apex::UserRenderIndexBuffer* mChild;

	static unsigned int indexBufferId;
	unsigned int mBufferId;

	RecordingRenderResourceManager::RecorderInterface* mRecorder;
};

unsigned int RecordingIndexBuffer::indexBufferId = 0;


nvidia::apex::UserRenderIndexBuffer* RecordingRenderResourceManager::createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc)
{
	nvidia::apex::UserRenderIndexBuffer* child = NULL;
	if (mChild != NULL)
	{
		child = mChild->createIndexBuffer(desc);
	}

	return new RecordingIndexBuffer(desc, child, mRecorder);
}



void RecordingRenderResourceManager::releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseIndexBuffer(*reinterpret_cast<RecordingIndexBuffer&>(buffer).getChild());
	}

	delete &buffer;
}



class RecordingBoneBuffer : public nvidia::apex::UserRenderBoneBuffer
{
public:
	RecordingBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc, nvidia::apex::UserRenderBoneBuffer* child, RecordingRenderResourceManager::RecorderInterface* recorder) :
		mDescriptor(desc), mChild(child), mBufferId(0), mRecorder(recorder)
	{
		mBufferId = boneBufferId++;

		if (mRecorder != NULL)
		{
			mRecorder->createBoneBuffer(mBufferId, desc);
		}
	}

	~RecordingBoneBuffer()
	{
		if (mRecorder != NULL)
		{
			mRecorder->releaseBoneBuffer(mBufferId);
		}
	}

	virtual void writeBuffer(const nvidia::apex::RenderBoneBufferData& data, unsigned int firstBone, unsigned int numBones)
	{
		if (mChild != NULL)
		{
			mChild->writeBuffer(data, firstBone, numBones);
		}

		if (mRecorder != NULL)
		{
			mRecorder->writeBoneBuffer(mBufferId, data, firstBone, numBones);
		}
	}

	nvidia::apex::UserRenderBoneBuffer* getChild()
	{
		return mChild;
	}

protected:
	nvidia::apex::UserRenderBoneBufferDesc mDescriptor;
	nvidia::apex::UserRenderBoneBuffer* mChild;

	static unsigned int boneBufferId;
	unsigned int mBufferId;

	RecordingRenderResourceManager::RecorderInterface* mRecorder;
};

unsigned int RecordingBoneBuffer::boneBufferId = 0;


nvidia::apex::UserRenderBoneBuffer* RecordingRenderResourceManager::createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc)
{
	nvidia::apex::UserRenderBoneBuffer* child = NULL;
	if (mChild != NULL)
	{
		child = mChild->createBoneBuffer(desc);
	}

	return new RecordingBoneBuffer(desc, child, mRecorder);
}



void RecordingRenderResourceManager::releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseBoneBuffer(*reinterpret_cast<RecordingBoneBuffer&>(buffer).getChild());
	}
}



nvidia::apex::UserRenderInstanceBuffer* RecordingRenderResourceManager::createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc)
{
	UNIMPLEMENTED;
	if (mChild != NULL)
	{
		return mChild->createInstanceBuffer(desc);
	}

	return NULL;
}



void RecordingRenderResourceManager::releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseInstanceBuffer(buffer);
	}
}



nvidia::apex::UserRenderSpriteBuffer* RecordingRenderResourceManager::createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc)
{
	UNIMPLEMENTED;
	if (mChild != NULL)
	{
		return mChild->createSpriteBuffer(desc);
	}

	return NULL;
}



void RecordingRenderResourceManager::releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseSpriteBuffer(buffer);
	}
}


nvidia::apex::UserRenderSurfaceBuffer* RecordingRenderResourceManager::createSurfaceBuffer(const nvidia::apex::UserRenderSurfaceBufferDesc& desc)
{
	UNIMPLEMENTED;
	if (mChild != NULL)
	{
		return mChild->createSurfaceBuffer(desc);
	}

	return NULL;
}



void RecordingRenderResourceManager::releaseSurfaceBuffer(nvidia::apex::UserRenderSurfaceBuffer& buffer)
{
	if (mChild != NULL)
	{
		mChild->releaseSurfaceBuffer(buffer);
	}
}


class RecordingRenderResource : public nvidia::apex::UserRenderResource
{
public:
	RecordingRenderResource(const nvidia::apex::UserRenderResourceDesc& desc, nvidia::apex::UserRenderResourceManager* childRrm, RecordingRenderResourceManager::RecorderInterface* recorder) : mChild(NULL), mDescriptor(desc), mRecorder(recorder)
	{
		assert(desc.numVertexBuffers > 0);

		mResourceId = resourceIds++;

		vertexBufferOriginal.resize(desc.numVertexBuffers);
		vertexBufferChild.resize(desc.numVertexBuffers);
		for (size_t i = 0; i < vertexBufferOriginal.size(); i++)
		{
			vertexBufferOriginal[i] = desc.vertexBuffers[i];
			vertexBufferChild[i] = reinterpret_cast<RecordingVertexBuffer*>(desc.vertexBuffers[i])->getChild();
		}

		mDescriptor.vertexBuffers = &vertexBufferOriginal[0];

		nvidia::apex::UserRenderResourceDesc newDesc(desc);
		newDesc.vertexBuffers = &vertexBufferChild[0];

		if (desc.indexBuffer != NULL)
		{
			newDesc.indexBuffer = reinterpret_cast<RecordingIndexBuffer*>(desc.indexBuffer)->getChild();
		}

		if (desc.boneBuffer != NULL)
		{
			newDesc.boneBuffer = reinterpret_cast<RecordingBoneBuffer*>(desc.boneBuffer)->getChild();
		}

		if (childRrm != NULL)
		{
			mChild = childRrm->createResource(newDesc);
		}

		if (mRecorder != NULL)
		{
			mRecorder->createResource(mResourceId, desc);
		}
	}

	~RecordingRenderResource()
	{

	}


	nvidia::apex::UserRenderResource* getChild()
	{
		return mChild;
	}



	void setVertexBufferRange(unsigned int firstVertex, unsigned int numVerts)
	{
		if (mChild != NULL)
		{
			mChild->setVertexBufferRange(firstVertex, numVerts);
		}

		mDescriptor.firstVertex = firstVertex;
		mDescriptor.numVerts = numVerts;
	}



	void setIndexBufferRange(unsigned int firstIndex, unsigned int numIndices)
	{
		if (mChild != NULL)
		{
			mChild->setIndexBufferRange(firstIndex, numIndices);
		}

		mDescriptor.firstIndex = firstIndex;
		mDescriptor.numIndices = numIndices;
	}



	void setBoneBufferRange(unsigned int firstBone, unsigned int numBones)
	{
		if (mChild != NULL)
		{
			mChild->setBoneBufferRange(firstBone, numBones);
		}

		mDescriptor.firstBone = firstBone;
		mDescriptor.numBones = numBones;
	}



	void setInstanceBufferRange(unsigned int firstInstance, unsigned int numInstances)
	{
		if (mChild != NULL)
		{
			mChild->setInstanceBufferRange(firstInstance, numInstances);
		}

		mDescriptor.firstInstance = firstInstance;
		mDescriptor.numInstances = numInstances;
	}



	void setSpriteBufferRange(unsigned int firstSprite, unsigned int numSprites)
	{
		if (mChild != NULL)
		{
			mChild->setSpriteBufferRange(firstSprite, numSprites);
		}

		mDescriptor.firstSprite = firstSprite;
		mDescriptor.numSprites = numSprites;
	}




	void setMaterial(void* material)
	{
		if (mChild != NULL)
		{
			mChild->setMaterial(material);
		}

		mDescriptor.material = material;
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

	void render()
	{
		if (mRecorder != NULL)
		{
			mRecorder->renderResource(mResourceId, mDescriptor);
		}
	}


protected:
	std::vector<nvidia::apex::UserRenderVertexBuffer*> vertexBufferOriginal;
	std::vector<nvidia::apex::UserRenderVertexBuffer*> vertexBufferChild;
	nvidia::apex::UserRenderResource* mChild;

	nvidia::apex::UserRenderResourceDesc mDescriptor;

	static unsigned int resourceIds;
	unsigned int mResourceId;

	RecordingRenderResourceManager::RecorderInterface* mRecorder;
};

unsigned int RecordingRenderResource::resourceIds = 0;



nvidia::apex::UserRenderResource* RecordingRenderResourceManager::createResource(const nvidia::apex::UserRenderResourceDesc& desc)
{
	return new RecordingRenderResource(desc, mChild, mRecorder);
}



void RecordingRenderResourceManager::releaseResource(nvidia::apex::UserRenderResource& resource)
{
	if (mChild != NULL)
	{
		mChild->releaseResource(*reinterpret_cast<RecordingRenderResource&>(resource).getChild());
	}

	delete &resource;
}



unsigned int RecordingRenderResourceManager::getMaxBonesForMaterial(void* material)
{
	unsigned int maxBones = 60; // whatever
	if (mChild != NULL)
	{
		maxBones = mChild->getMaxBonesForMaterial(material);
	}

	if (mRecorder != NULL)
	{
		mRecorder->setMaxBonesForMaterial(material, maxBones);
	}

	return maxBones;
}




RecordingRenderer::RecordingRenderer(nvidia::apex::UserRenderer* child, RecordingRenderResourceManager::RecorderInterface* recorder) : mChild(child), mRecorder(recorder)
{

}



RecordingRenderer::~RecordingRenderer()
{

}



void RecordingRenderer::renderResource(const nvidia::apex::RenderContext& context)
{
	RecordingRenderResource* resource = reinterpret_cast<RecordingRenderResource*>(context.renderResource);

	resource->render();

	nvidia::apex::UserRenderResource* child = resource->getChild();
	if (mChild != NULL && child != NULL)
	{
		nvidia::apex::RenderContext newContext(context);
		newContext.renderResource = child;

		mChild->renderResource(newContext);
	}
}




FileRecorder::FileRecorder(const char* filename)
{

	if (filename != NULL)
	{
		mOutputFile = fopen(filename, "w");
		assert(mOutputFile);

		if (mOutputFile != NULL)
		{
			time_t curtime;
			::time(&curtime);

			fprintf(mOutputFile, "# Logfile created on %s\n\n", ctime(&curtime));
		}
	}
	else
	{
		// console
		mOutputFile = stdout;

#if PX_WINDOWS_FAMILY
		//open a console for printf:
		if (AllocConsole())
		{
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "wb", stdout);
			freopen_s(&stream, "CONOUT$", "wb", stderr);

			SetConsoleTitle("Recording Resource Manager Output");
			//SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);

			CONSOLE_SCREEN_BUFFER_INFO coninfo;
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
			coninfo.dwSize.Y = 1000;
			SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
		}
#endif
	}
}


FileRecorder::~FileRecorder()
{
	if (mOutputFile != stdout && mOutputFile != NULL)
	{
		fclose(mOutputFile);
	}
}


#define WRITE_ITEM(_A) writeElem(#_A, _A)
#define WRITE_DESC_ELEM(_A) writeElem(#_A, (uint32_t)desc._A)
#define WRITE_REQUEST(_A) if (desc.buffersRequest[nvidia::apex::RenderVertexSemantic::_A] != nvidia::apex::RenderDataFormat::UNSPECIFIED) writeElem(#_A, desc.buffersRequest[nvidia::apex::RenderVertexSemantic::_A])



void FileRecorder::createVertexBuffer(unsigned int id, const nvidia::apex::UserRenderVertexBufferDesc& desc)
{
	fprintf(mOutputFile, "VertexBuffer[%d]::create: ", id);

	WRITE_DESC_ELEM(maxVerts);
	WRITE_DESC_ELEM(hint);

	WRITE_REQUEST(POSITION);
	WRITE_REQUEST(NORMAL);
	WRITE_REQUEST(TANGENT);
	WRITE_REQUEST(BINORMAL);
	WRITE_REQUEST(COLOR);
	WRITE_REQUEST(TEXCOORD0);
	WRITE_REQUEST(TEXCOORD1);
	WRITE_REQUEST(TEXCOORD2);
	WRITE_REQUEST(TEXCOORD3);
	WRITE_REQUEST(BONE_INDEX);
	WRITE_REQUEST(BONE_WEIGHT);
	WRITE_REQUEST(DISPLACEMENT_TEXCOORD);
	WRITE_REQUEST(DISPLACEMENT_FLAGS);

	WRITE_DESC_ELEM(numCustomBuffers);
	// PH: not done on purpose (yet)
	//void** 						customBuffersIdents;
	//RenderDataFormat::Enum*	customBuffersRequest;
	WRITE_DESC_ELEM(moduleIdentifier);
	WRITE_DESC_ELEM(uvOrigin);
	WRITE_DESC_ELEM(canBeShared);

	fprintf(mOutputFile, "\n");

#if PX_X86
	// PH: Make sure that if the size of the descriptor changes, we get a compile error here and adapt the WRITE_REQUESTs from above accordingly
	PX_COMPILE_TIME_ASSERT(sizeof(desc) == 4 + 4 + (13 * 4) + 4 + sizeof(void*) + sizeof(void*) + 4 + 4 + 1 + 1 + 2/*padding*/ + sizeof(void*) );
#endif
}



#define WRITE_DATA_ITEM(_A) writeElem(#_A, semanticData._A)
void FileRecorder::writeVertexBuffer(unsigned int id, const nvidia::apex::RenderVertexBufferData& data, unsigned int firstVertex, unsigned int numVertices)
{
	fprintf(mOutputFile, "VertexBuffer[%d]::write: ", id);
	WRITE_ITEM(firstVertex);
	WRITE_ITEM(numVertices);

	WRITE_ITEM(data.moduleId);
	WRITE_ITEM(data.numModuleSpecificSemantics);

	fprintf(mOutputFile, "\n");

#if PX_X86
	PX_COMPILE_TIME_ASSERT(sizeof(nvidia::apex::RenderSemanticData) == (sizeof(void*) + 4 + sizeof(void*) + 4 + 4 + 1 + 3/*padding*/));
#endif

	for (unsigned int i = 0; i < nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		const nvidia::apex::RenderSemanticData& semanticData = data.getSemanticData(nvidia::apex::RenderVertexSemantic::Enum(i));
		if (semanticData.format != nvidia::apex::RenderDataFormat::UNSPECIFIED)
		{
			fprintf(mOutputFile, " [%d]: ", i);
			WRITE_DATA_ITEM(stride);
			WRITE_DATA_ITEM(format);
			WRITE_DATA_ITEM(srcFormat);
			fprintf(mOutputFile, "\n");

			//writeBufferData(semanticData.data, semanticData.stride, numVertices, semanticData.format);
		}
	}

	PX_COMPILE_TIME_ASSERT(nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS == 13);

#if PX_X86
	PX_COMPILE_TIME_ASSERT(sizeof(data) == sizeof(nvidia::apex::RenderSemanticData) * nvidia::apex::RenderVertexSemantic::NUM_SEMANTICS + 4 + sizeof(void*) + 4 + sizeof(void*) + 4);
#endif
}
#undef WRITE_DATA_ITEM



void FileRecorder::releaseVertexBuffer(unsigned int id)
{
	fprintf(mOutputFile, "VertexBuffer[%d]::release\n", id);
}




void FileRecorder::createIndexBuffer(unsigned int id, const nvidia::apex::UserRenderIndexBufferDesc& desc)
{
	fprintf(mOutputFile, "IndexBuffer[%d]::create: ", id);

	WRITE_DESC_ELEM(maxIndices);
	WRITE_DESC_ELEM(hint);
	WRITE_DESC_ELEM(format);
	WRITE_DESC_ELEM(primitives);
	WRITE_DESC_ELEM(registerInCUDA);
	//WRITE_DESC_ELEM(interopContext);

	fprintf(mOutputFile, "\n");

#if PX_X86
	PX_COMPILE_TIME_ASSERT(sizeof(desc) == 4 + 4 + 4 + 4 + 1 + 3/*padding*/ + sizeof(void*));
#endif
}



void FileRecorder::writeIndexBuffer(unsigned int id, const void* /*srcData*/, uint32_t /*srcStride*/, unsigned int firstDestElement, unsigned int numElements, nvidia::apex::RenderDataFormat::Enum /*format*/)
{
	fprintf(mOutputFile, "IndexBuffer[%d]::write ", id);
	WRITE_ITEM(firstDestElement);
	WRITE_ITEM(numElements);

	//writeBufferData(srcData, srcStride, numElements, format);

	fprintf(mOutputFile, "\n");
}



void FileRecorder::releaseIndexBuffer(unsigned int id)
{
	fprintf(mOutputFile, "IndexBuffer[%d]::release\n", id);
}



void FileRecorder::createBoneBuffer(unsigned int id, const nvidia::apex::UserRenderBoneBufferDesc& /*desc*/)
{
	fprintf(mOutputFile, "BoneBuffer[%d]::create\n", id);
}



void FileRecorder::writeBoneBuffer(unsigned int id, const nvidia::apex::RenderBoneBufferData& /*data*/, unsigned int /*firstBone*/, unsigned int /*numBones*/)
{
	fprintf(mOutputFile, "BoneBuffer[%d]::write\n", id);
}



void FileRecorder::releaseBoneBuffer(unsigned int id)
{
	fprintf(mOutputFile, "BoneBuffer[%d]::release\n", id);
}



void FileRecorder::createResource(unsigned int id, const nvidia::apex::UserRenderResourceDesc& /*desc*/)
{
	fprintf(mOutputFile, "Resource[%d]::create\n", id);
}



void FileRecorder::renderResource(unsigned int id, const nvidia::apex::UserRenderResourceDesc& /*desc*/)
{
	fprintf(mOutputFile, "Resource[%d]::render\n", id);
}



void FileRecorder::releaseResource(unsigned int id)
{
	fprintf(mOutputFile, "Resource[%d]::release\n", id);
}



void FileRecorder::setMaxBonesForMaterial(void* material, unsigned int maxBones)
{
	fprintf(mOutputFile, "MaterialMaxBones[%p]=%d\n", material, maxBones);
}



void FileRecorder::writeElem(const char* name, unsigned int value)
{
	fprintf(mOutputFile, "%s=%d ", name, value);
}


void FileRecorder::writeBufferData(const void* data, unsigned int stride, unsigned int numElements, nvidia::apex::RenderDataFormat::Enum format)
{
	switch (format)
	{
	case nvidia::apex::RenderDataFormat::FLOAT2:
		writeBufferDataFloat(data, stride, numElements, 2);
		break;
	case nvidia::apex::RenderDataFormat::FLOAT3:
		writeBufferDataFloat(data, stride, numElements, 3);
		break;
	case nvidia::apex::RenderDataFormat::FLOAT4:
		writeBufferDataFloat(data, stride, numElements, 4);
		break;
	case nvidia::apex::RenderDataFormat::USHORT4:
		writeBufferDataShort(data, stride, numElements, 4);
		break;
	case nvidia::apex::RenderDataFormat::UINT1:
		writeBufferDataLong(data, stride, numElements, 1);
		break;
	default:
		UNIMPLEMENTED;
		break;
	}
}




void FileRecorder::writeBufferDataFloat(const void* data, unsigned int stride, unsigned int numElements, unsigned int numFloatsPerDataSet)
{
	const char* startData = (const char*)data;
	for (unsigned int i = 0; i < numElements; i++)
	{
		const float* elemData = (const float*)(startData + stride * i);

		fprintf(mOutputFile, "(");
		for (unsigned int j = 0; j < numFloatsPerDataSet; j++)
		{
			fprintf(mOutputFile, "%f ", elemData[j]);
		}
		fprintf(mOutputFile, "),");
	}
}


void FileRecorder::writeBufferDataShort(const void* data, unsigned int stride, unsigned int numElements, unsigned int numFloatsPerDataSet)
{
	const char* startData = (const char*)data;
	for (unsigned int i = 0; i < numElements; i++)
	{
		const short* elemData = (const short*)(startData + stride * i);

		fprintf(mOutputFile, "(");
		for (unsigned int j = 0; j < numFloatsPerDataSet; j++)
		{
			fprintf(mOutputFile, "%d ", elemData[j]);
		}
		fprintf(mOutputFile, "),");
	}
}


void FileRecorder::writeBufferDataLong(const void* data, unsigned int stride, unsigned int numElements, unsigned int numFloatsPerDataSet)
{
	const char* startData = (const char*)data;
	for (unsigned int i = 0; i < numElements; i++)
	{
		const long* elemData = (const long*)(startData + stride * i);

		fprintf(mOutputFile, "(");
		for (unsigned int j = 0; j < numFloatsPerDataSet; j++)
		{
			fprintf(mOutputFile, "%d ", (int)elemData[j]);
		}
		fprintf(mOutputFile, "),");
	}
}
