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



#include "MultiClientRenderResourceManager.h"

#include "UserRenderVertexBuffer.h"
#include "UserRenderIndexBuffer.h"
#include "UserRenderBoneBuffer.h"
#include "UserRenderInstanceBuffer.h"
#include "UserRenderSpriteBuffer.h"
#include "UserRenderSurfaceBuffer.h"

#include "UserRenderResource.h"
#include "UserRenderResourceDesc.h"

#include "RenderContext.h"


#include <assert.h>
#include <algorithm>	// for std::min


MultiClientRenderResourceManager::MultiClientRenderResourceManager()
{

}



MultiClientRenderResourceManager::~MultiClientRenderResourceManager()
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		if (mChildren[i].destroyRrm)
		{
			delete mChildren[i].rrm;
		}
		mChildren[i].rrm = NULL;
	}
}



void MultiClientRenderResourceManager::addChild(nvidia::apex::UserRenderResourceManager* rrm, bool destroyAutomatic)
{
	for (size_t i = 0; i < mChildren.size(); i++)
	{
		if (mChildren[i].rrm == rrm)
		{
			return;
		}
	}

	mChildren.push_back(Child(rrm, destroyAutomatic));
}

bool MultiClientRenderResourceManager::getSpriteLayoutData(uint32_t spriteCount, uint32_t spriteSemanticsBitmap, nvidia::apex::UserRenderSpriteBufferDesc* bufferDesc)
{
	PX_UNUSED(spriteCount);
	PX_UNUSED(spriteSemanticsBitmap);
	PX_UNUSED(bufferDesc);
	return false;
}

bool MultiClientRenderResourceManager::getInstanceLayoutData(uint32_t particleCount, uint32_t particleSemanticsBitmap, nvidia::apex::UserRenderInstanceBufferDesc* bufferDesc)
{
	PX_UNUSED(particleCount);
	PX_UNUSED(particleSemanticsBitmap);
	PX_UNUSED(bufferDesc);
	return false;
}

template<typename T>
class MultiClientBuffer
{
public:
	MultiClientBuffer() {}
	~MultiClientBuffer() {}


	void addChild(T* vb)
	{
		mChildren.push_back(vb);
	}

	T* getChild(size_t index)
	{
		assert(index < mChildren.size());
		return mChildren[index];
	}

protected:
	std::vector<T*> mChildren;
};




class MultiClientVertexBuffer : public nvidia::apex::UserRenderVertexBuffer, public MultiClientBuffer<nvidia::apex::UserRenderVertexBuffer>
{
public:
	MultiClientVertexBuffer() {}
	~MultiClientVertexBuffer() {}

	virtual void writeBuffer(const nvidia::apex::RenderVertexBufferData& data, unsigned int firstVertex, unsigned int numVertices)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->writeBuffer(data, firstVertex, numVertices);
		}
	}

};


nvidia::apex::UserRenderVertexBuffer* MultiClientRenderResourceManager::createVertexBuffer(const nvidia::apex::UserRenderVertexBufferDesc& desc)
{
	MultiClientVertexBuffer* vb = new MultiClientVertexBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		vb->addChild(mChildren[i].rrm->createVertexBuffer(desc));
	}

	return vb;
}

void MultiClientRenderResourceManager::releaseVertexBuffer(nvidia::apex::UserRenderVertexBuffer& buffer)
{
	MultiClientVertexBuffer* vb = static_cast<MultiClientVertexBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderVertexBuffer* childVb = vb->getChild(i);
		mChildren[i].rrm->releaseVertexBuffer(*childVb);
	}

	delete vb;
}






class MultiClientIndexBuffer : public nvidia::apex::UserRenderIndexBuffer, public MultiClientBuffer<nvidia::apex::UserRenderIndexBuffer>
{
public:
	MultiClientIndexBuffer() {}
	~MultiClientIndexBuffer() {}

	virtual void writeBuffer(const void* srcData, unsigned int srcStride, unsigned int firstDestElement, unsigned int numElements)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->writeBuffer(srcData, srcStride, firstDestElement, numElements);
		}
	}
};


nvidia::apex::UserRenderIndexBuffer* MultiClientRenderResourceManager::createIndexBuffer(const nvidia::apex::UserRenderIndexBufferDesc& desc)
{
	MultiClientIndexBuffer* ib = new MultiClientIndexBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		ib->addChild(mChildren[i].rrm->createIndexBuffer(desc));
	}

	return ib;
}

void MultiClientRenderResourceManager::releaseIndexBuffer(nvidia::apex::UserRenderIndexBuffer& buffer)
{
	MultiClientIndexBuffer* ib = static_cast<MultiClientIndexBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderIndexBuffer* childIb = ib->getChild(i);
		mChildren[i].rrm->releaseIndexBuffer(*childIb);
	}

	delete ib;
}








class MultiClientBoneBuffer : public nvidia::apex::UserRenderBoneBuffer, public MultiClientBuffer<nvidia::apex::UserRenderBoneBuffer>
{
public:
	MultiClientBoneBuffer() {}
	~MultiClientBoneBuffer() {}

	virtual void writeBuffer(const nvidia::apex::RenderBoneBufferData& data, unsigned int firstBone, unsigned int numBones)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->writeBuffer(data, firstBone, numBones);
		}
	}
};



nvidia::apex::UserRenderBoneBuffer* MultiClientRenderResourceManager::createBoneBuffer(const nvidia::apex::UserRenderBoneBufferDesc& desc)
{
	MultiClientBoneBuffer* bb = new MultiClientBoneBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		bb->addChild(mChildren[i].rrm->createBoneBuffer(desc));
	}

	return bb;
}



void MultiClientRenderResourceManager::releaseBoneBuffer(nvidia::apex::UserRenderBoneBuffer& buffer)
{
	MultiClientBoneBuffer* bb = static_cast<MultiClientBoneBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderBoneBuffer* childBb = bb->getChild(i);
		mChildren[i].rrm->releaseBoneBuffer(*childBb);
	}

	delete bb;
}





class MultiClientInstanceBuffer : public nvidia::apex::UserRenderInstanceBuffer, public MultiClientBuffer<nvidia::apex::UserRenderInstanceBuffer>
{
public:
	MultiClientInstanceBuffer() {}
	~MultiClientInstanceBuffer() {}

	virtual void writeBuffer(const void* data, unsigned int firstInstance, unsigned int numInstances)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->writeBuffer(data, firstInstance, numInstances);
		}
	}
};




nvidia::apex::UserRenderInstanceBuffer* MultiClientRenderResourceManager::createInstanceBuffer(const nvidia::apex::UserRenderInstanceBufferDesc& desc)
{
	MultiClientInstanceBuffer* ib = new MultiClientInstanceBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		ib->addChild(mChildren[i].rrm->createInstanceBuffer(desc));
	}

	return ib;
}



void MultiClientRenderResourceManager::releaseInstanceBuffer(nvidia::apex::UserRenderInstanceBuffer& buffer)
{
	MultiClientInstanceBuffer* ib = static_cast<MultiClientInstanceBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderInstanceBuffer* childIb = ib->getChild(i);
		mChildren[i].rrm->releaseInstanceBuffer(*childIb);
	}

	delete ib;
}





class MultiClientSpriteBuffer : public nvidia::apex::UserRenderSpriteBuffer, public MultiClientBuffer<nvidia::apex::UserRenderSpriteBuffer>
{
public:
	MultiClientSpriteBuffer() {}
	~MultiClientSpriteBuffer() {}

	virtual void writeBuffer(const void* data, unsigned int firstSprite, unsigned int numSprites)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->writeBuffer(data, firstSprite, numSprites);
		}
	}
};



nvidia::apex::UserRenderSpriteBuffer* MultiClientRenderResourceManager::createSpriteBuffer(const nvidia::apex::UserRenderSpriteBufferDesc& desc)
{
	MultiClientSpriteBuffer* sb = new MultiClientSpriteBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		sb->addChild(mChildren[i].rrm->createSpriteBuffer(desc));
	}

	return sb;
}



void MultiClientRenderResourceManager::releaseSpriteBuffer(nvidia::apex::UserRenderSpriteBuffer& buffer)
{
	MultiClientSpriteBuffer* sb = static_cast<MultiClientSpriteBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderSpriteBuffer* childSb = sb->getChild(i);
		mChildren[i].rrm->releaseSpriteBuffer(*childSb);
	}

	delete sb;
}


class MultiClientSurfaceBuffer : public nvidia::apex::UserRenderSurfaceBuffer, public MultiClientBuffer<nvidia::apex::UserRenderSurfaceBuffer>
{
public:
	MultiClientSurfaceBuffer() {}
	~MultiClientSurfaceBuffer() {}

	virtual void writeBuffer(const void* /*srcData*/, 
							uint32_t /*srcPitch*/, 
							uint32_t /*srcHeight*/, 
							uint32_t /*dstX*/, 
							uint32_t /*dstY*/, 
							uint32_t /*dstZ*/, 
							uint32_t /*width*/, 
							uint32_t /*height*/, 
							uint32_t /*depth*/)
	{
		//for (size_t i = 0; i < mChildren.size(); i++)
		//{
		//	mChildren[i]->writeBuffer(data, firstSprite, numSprites);
		//}
	}
};

nvidia::apex::UserRenderSurfaceBuffer*	 MultiClientRenderResourceManager::createSurfaceBuffer( const nvidia::apex::UserRenderSurfaceBufferDesc &desc )
{
	MultiClientSurfaceBuffer* sb = new MultiClientSurfaceBuffer();

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		sb->addChild(mChildren[i].rrm->createSurfaceBuffer(desc));
	}

	return sb;
}



void MultiClientRenderResourceManager::releaseSurfaceBuffer( nvidia::apex::UserRenderSurfaceBuffer &buffer )
{
	MultiClientSurfaceBuffer* sb = static_cast<MultiClientSurfaceBuffer*>(&buffer);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::UserRenderSurfaceBuffer* childSb = sb->getChild(i);
		mChildren[i].rrm->releaseSurfaceBuffer(*childSb);
	}

	delete sb;
}







class MultiClientRenderResource : public nvidia::apex::UserRenderResource
{
public:
	MultiClientRenderResource(const nvidia::apex::UserRenderResourceDesc& desc) : mDescriptor(desc)
	{
		assert(desc.numVertexBuffers > 0);

		mVertexBufferOriginal.resize(desc.numVertexBuffers);
		for (size_t i = 0; i < mVertexBufferOriginal.size(); i++)
		{
			mVertexBufferOriginal[i] = desc.vertexBuffers[i];
		}

		mDescriptor.vertexBuffers = &mVertexBufferOriginal[0];

	}

	~MultiClientRenderResource()
	{

	}



	void addChild(nvidia::apex::UserRenderResourceManager* rrm)
	{
		nvidia::apex::UserRenderResourceDesc newDesc(mDescriptor);

		std::vector<nvidia::apex::UserRenderVertexBuffer*> childVertexBuffers(mVertexBufferOriginal.size());

		size_t nextChild = mChildren.size();

		for (size_t i = 0; i < mVertexBufferOriginal.size(); i++)
		{
			MultiClientVertexBuffer* vb = static_cast<MultiClientVertexBuffer*>(mVertexBufferOriginal[i]);
			childVertexBuffers[i] = vb->getChild(nextChild);
		}

		newDesc.vertexBuffers = &childVertexBuffers[0];

		if (mDescriptor.indexBuffer != NULL)
		{
			newDesc.indexBuffer = static_cast<MultiClientIndexBuffer*>(mDescriptor.indexBuffer)->getChild(nextChild);
		}

		if (mDescriptor.boneBuffer != NULL)
		{
			newDesc.boneBuffer = static_cast<MultiClientBoneBuffer*>(mDescriptor.boneBuffer)->getChild(nextChild);
		}

		if (mDescriptor.spriteBuffer != NULL)
		{
			newDesc.spriteBuffer = static_cast<MultiClientSpriteBuffer*>(mDescriptor.spriteBuffer)->getChild(nextChild);
		}

		if (rrm != NULL)
		{
			mChildren.push_back(rrm->createResource(newDesc));
		}
	}


	nvidia::apex::UserRenderResource* getChild(size_t index)
	{
		assert(index < mChildren.size());
		return mChildren[index];
	}



	void setVertexBufferRange(unsigned int firstVertex, unsigned int numVerts)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setVertexBufferRange(firstVertex, numVerts);
		}

		mDescriptor.firstVertex = firstVertex;
		mDescriptor.numVerts = numVerts;
	}



	void setIndexBufferRange(unsigned int firstIndex, unsigned int numIndices)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setIndexBufferRange(firstIndex, numIndices);
		}

		mDescriptor.firstIndex = firstIndex;
		mDescriptor.numIndices = numIndices;
	}



	void setBoneBufferRange(unsigned int firstBone, unsigned int numBones)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setBoneBufferRange(firstBone, numBones);
		}

		mDescriptor.firstBone = firstBone;
		mDescriptor.numBones = numBones;
	}



	void setInstanceBufferRange(unsigned int firstInstance, unsigned int numInstances)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setInstanceBufferRange(firstInstance, numInstances);
		}

		mDescriptor.firstInstance = firstInstance;
		mDescriptor.numInstances = numInstances;
	}



	void setSpriteBufferRange(unsigned int firstSprite, unsigned int numSprites)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setSpriteBufferRange(firstSprite, numSprites);
		}

		mDescriptor.firstSprite = firstSprite;
		mDescriptor.numSprites = numSprites;
	}




	void setMaterial(void* material)
	{
		for (size_t i = 0; i < mChildren.size(); i++)
		{
			mChildren[i]->setMaterial(material);
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

protected:
	std::vector<nvidia::apex::UserRenderVertexBuffer*> mVertexBufferOriginal;
	std::vector<nvidia::apex::UserRenderResource*> mChildren;

	nvidia::apex::UserRenderResourceDesc mDescriptor;
};




nvidia::apex::UserRenderResource* MultiClientRenderResourceManager::createResource(const nvidia::apex::UserRenderResourceDesc& desc)
{
	MultiClientRenderResource* rr = new MultiClientRenderResource(desc);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		rr->addChild(mChildren[i].rrm);
	}

	return rr;
}



void MultiClientRenderResourceManager::releaseResource(nvidia::apex::UserRenderResource& resource)
{
	MultiClientRenderResource* rr = static_cast<MultiClientRenderResource*>(&resource);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		mChildren[i].rrm->releaseResource(*rr->getChild(i));
	}

	delete rr;
}



unsigned int MultiClientRenderResourceManager::getMaxBonesForMaterial(void* material)
{
	unsigned int smallestMax = 10000;

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		unsigned int childMax = mChildren[i].rrm->getMaxBonesForMaterial(material);
		if (childMax > 0)
		{
			smallestMax = std::min(smallestMax, childMax);
		}
	}

	return smallestMax;
}



void MultiClientUserRenderer::addChild(nvidia::apex::UserRenderer* child)
{
	mChildren.push_back(child);
}



void MultiClientUserRenderer::renderResource(const nvidia::apex::RenderContext& context)
{
	MultiClientRenderResource* rr = static_cast<MultiClientRenderResource*>(context.renderResource);

	for (size_t i = 0; i < mChildren.size(); i++)
	{
		nvidia::apex::RenderContext newContext(context);
		newContext.renderResource = rr->getChild(i);
		mChildren[i]->renderResource(newContext);
	}
}
