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


#include "ApexRenderResources.h"

#include "RenderDataFormat.h"
#include <sstream>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											   Helper functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static DXGI_FORMAT getD3DFormat(RenderDataFormat::Enum format)
{
	DXGI_FORMAT d3dFormat;
	switch(format)
	{
	case RenderDataFormat::FLOAT1:
		d3dFormat = DXGI_FORMAT_R32_FLOAT;
		break;
	case RenderDataFormat::FLOAT2:
		d3dFormat = DXGI_FORMAT_R32G32_FLOAT;
		break;
	case RenderDataFormat::FLOAT3:
		d3dFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		break;
	case RenderDataFormat::FLOAT4:
		d3dFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case RenderDataFormat::B8G8R8A8:
		d3dFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
		break;
	case RenderDataFormat::UINT1:
	case RenderDataFormat::UBYTE4:
	case RenderDataFormat::R8G8B8A8:
		d3dFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case RenderDataFormat::USHORT1:
		d3dFormat = DXGI_FORMAT_R16_UINT;
		break;
	case RenderDataFormat::USHORT2:
		d3dFormat = DXGI_FORMAT_R16G16_UINT;
		break;
	case RenderDataFormat::USHORT3:
	case RenderDataFormat::USHORT4:
		d3dFormat = DXGI_FORMAT_R16G16B16A16_UINT;
		break;
	default:
		d3dFormat = DXGI_FORMAT_UNKNOWN;
	}
	PX_ASSERT((d3dFormat != DXGI_FORMAT_UNKNOWN) && "Invalid DIRECT3D11 vertex type.");
	return d3dFormat;
}

static void getD3DUsage(RenderVertexSemantic::Enum semantic, LPCSTR& usageName, uint8_t& usageIndex)
{
	usageIndex = 0;
	if(semantic >= RenderVertexSemantic::TEXCOORD0 && semantic <= RenderVertexSemantic::TEXCOORD3)
	{
		usageName = "TEXCOORD";
		usageIndex = (uint8_t)(semantic - RenderVertexSemantic::TEXCOORD0);
	}
	else
	{
		switch(semantic)
		{
		case RenderVertexSemantic::POSITION:
			usageName = "POSITION";
			break;
		case RenderVertexSemantic::NORMAL:
			usageName = "NORMAL";
			break;
		case RenderVertexSemantic::TANGENT:
			usageName = "TANGENT";
			break;
		case RenderVertexSemantic::COLOR:
			usageName = "COLOR";
			break;
		case RenderVertexSemantic::BONE_INDEX:
			usageName = "TEXCOORD";
			usageIndex = 5;
			break;
		case RenderVertexSemantic::BONE_WEIGHT:
			usageName = "TEXCOORD";
			usageIndex = 6;
			break;
		case RenderVertexSemantic::DISPLACEMENT_TEXCOORD:
			usageName = "TEXCOORD";
			usageIndex = 7;
			break;
		case RenderVertexSemantic::DISPLACEMENT_FLAGS:
			usageName = "TEXCOORD";
			usageIndex = 8;
			break;
		default:
			usageName = "POSITION";
		}
	}
}

static void getD3DUsage(RenderSpriteSemantic::Enum semantic, LPCSTR& usageName, uint8_t& usageIndex)
{
	usageIndex = 0;
	switch (semantic)
	{
	case RenderSpriteSemantic::POSITION:
		usageName = "POSITION";
		break;
	case RenderSpriteSemantic::COLOR:
		usageName = "COLOR";
		break;
	case RenderSpriteSemantic::VELOCITY:
		usageName = "NORMAL";
		break;
	case RenderSpriteSemantic::SCALE:
		usageName = "TANGENT";
		break;
	case RenderSpriteSemantic::LIFE_REMAIN:
		usageName = "TEXCOORD";
		usageIndex = 0;
		break;
	case RenderSpriteSemantic::DENSITY:
		usageName = "TEXCOORD";
		usageIndex = 1;
		break;
	case RenderSpriteSemantic::SUBTEXTURE:
		usageName = "TEXCOORD";
		usageIndex = 2;
		break;
	case RenderSpriteSemantic::ORIENTATION:
		usageName = "TEXCOORD";
		usageIndex = 3;
		break;
	default:
		PX_ALWAYS_ASSERT();
		usageName = "POSITION";
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											SampleApexRendererVertexBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererVertexBuffer::SampleApexRendererVertexBuffer(ID3D11Device& d3dDevice,
                                                               const apex::UserRenderVertexBufferDesc& desc)
: mDevice(d3dDevice)
{
	uint32_t stride = 0;
	uint8_t pointer = 0;
	for(uint32_t i = 0; i < apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		apex::RenderDataFormat::Enum apexFormat = desc.buffersRequest[i];

		mDataFormat[i] = apexFormat;
		stride += RenderDataFormat::getFormatDataSize(apexFormat);

		mPointers[i] = pointer;
		pointer += (uint8_t)RenderDataFormat::getFormatDataSize(apexFormat);
	}

	mStride = stride;
	mVerticesCount = desc.maxVerts;

	D3D11_BUFFER_DESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = (UINT)(desc.maxVerts * mStride);
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	V(mDevice.CreateBuffer(&bufferDesc, NULL, &mVertexBuffer));
}

SampleApexRendererVertexBuffer::~SampleApexRendererVertexBuffer(void)
{
	if(mVertexBuffer)
	{
		mVertexBuffer->Release();
		mVertexBuffer = NULL;
	}
}

void SampleApexRendererVertexBuffer::addVertexElements(uint32_t streamIndex,
                                                       std::vector<D3D11_INPUT_ELEMENT_DESC>& vertexElements) const
{
	uint32_t offset = 0;

	for(uint32_t i = 0; i < apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		apex::RenderVertexSemantic::Enum semantic = (apex::RenderVertexSemantic::Enum)i;

		if(mDataFormat[i] != RenderDataFormat::UNSPECIFIED)
		{
			uint8_t d3dUsageIndex = 0;
			LPCSTR d3dUsageName = "";
			getD3DUsage(semantic, d3dUsageName, d3dUsageIndex);

			D3D11_INPUT_ELEMENT_DESC element;
			element.SemanticName = d3dUsageName;
			element.SemanticIndex = d3dUsageIndex;
			element.Format = getD3DFormat(mDataFormat[i]);
			element.InputSlot = streamIndex;
			element.AlignedByteOffset = offset;
			element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;

			vertexElements.push_back(element);

			offset += (uint32_t)RenderDataFormat::getFormatDataSize(mDataFormat[i]);
		}
	}
}

void SampleApexRendererVertexBuffer::writeBuffer(const apex::RenderVertexBufferData& data, uint32_t firstVertex,
                                                 uint32_t numVerts)
{
	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	D3D11_MAPPED_SUBRESOURCE mappedRead;
	V(context->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

	for(uint32_t i = 0; i < apex::RenderVertexSemantic::NUM_SEMANTICS; i++)
	{
		apex::RenderVertexSemantic::Enum apexSemantic = (apex::RenderVertexSemantic::Enum)i;
		const apex::RenderSemanticData& semanticData = data.getSemanticData(apexSemantic);
		if(semanticData.data)
		{
			const void* srcData = semanticData.data;
			const uint32_t srcStride = semanticData.stride;

			void* dstData = ((uint8_t*)mappedRead.pData) + firstVertex * mStride + mPointers[i];

			for(uint32_t j = 0; j < numVerts; j++)
			{
				memcpy(dstData, srcData, srcStride);
				dstData = ((uint8_t*)dstData) + mStride;
				srcData = ((uint8_t*)srcData) + srcStride;
			}
		}
	}

	context->Unmap(mVertexBuffer, 0);
}

void SampleApexRendererVertexBuffer::bind(ID3D11DeviceContext& context, uint32_t streamID, uint32_t firstVertex)
{
	ID3D11Buffer* pBuffers[1] = { mVertexBuffer };
	UINT strides[1] = { mStride };
	UINT offsets[1] = { firstVertex * mStride };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

void SampleApexRendererVertexBuffer::unbind(ID3D11DeviceContext& context, uint32_t streamID)
{
	ID3D11Buffer* pBuffers[1] = { NULL };
	UINT strides[1] = { 0 };
	UINT offsets[1] = { 0 };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//										SampleApexRendererIndexBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererIndexBuffer::SampleApexRendererIndexBuffer(ID3D11Device& d3dDevice,
                                                             const apex::UserRenderIndexBufferDesc& desc)
: mDevice(d3dDevice)
{
	mIndicesCount = desc.maxIndices;
	mStride = RenderDataFormat::getFormatDataSize(desc.format);

	D3D11_BUFFER_DESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.ByteWidth = (UINT)(RenderDataFormat::getFormatDataSize(desc.format) * desc.maxIndices);
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	V(mDevice.CreateBuffer(&bufferDesc, NULL, &mIndexBuffer));

	mFormat = (desc.format == RenderDataFormat::USHORT1 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
}

SampleApexRendererIndexBuffer::~SampleApexRendererIndexBuffer(void)
{
	if(mIndexBuffer)
	{
		mIndexBuffer->Release();
		mIndexBuffer = NULL;
	}
}

void SampleApexRendererIndexBuffer::writeBuffer(const void* srcData, uint32_t srcStride, uint32_t firstDestElement,
                                                uint32_t numElements)
{
	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	D3D11_MAPPED_SUBRESOURCE mappedRead;
	V(context->Map(mIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

	::memcpy(static_cast<char*>(mappedRead.pData) + firstDestElement * mStride, srcData, mStride * numElements);

	context->Unmap(mIndexBuffer, 0);
}

void SampleApexRendererIndexBuffer::bind(ID3D11DeviceContext& context)
{
	context.IASetIndexBuffer(mIndexBuffer, mFormat, 0);
}

void SampleApexRendererIndexBuffer::unbind(ID3D11DeviceContext& context)
{
	context.IASetIndexBuffer(NULL, DXGI_FORMAT(), 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											SampleApexRendererBoneBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererBoneBuffer::SampleApexRendererBoneBuffer(ID3D11Device& d3dDevice,
                                                           const apex::UserRenderBoneBufferDesc& desc)
: mDevice(d3dDevice)
{
	mMaxBones = desc.maxBones;
	mBones = NULL;
	mBones = new PxMat44[mMaxBones];

	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = 4;
		desc.Height = mMaxBones;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.CPUAccessFlags = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA subData;
		memset(&subData, 0, sizeof(D3D11_SUBRESOURCE_DATA));
		subData.pSysMem = mBones;
		subData.SysMemPitch = desc.Width * sizeof(unsigned int);
		subData.SysMemSlicePitch = 0;

		V(mDevice.CreateTexture2D(&desc, &subData, &mTexture));
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		V(mDevice.CreateShaderResourceView(mTexture, &desc, &mShaderResourceView));
	}
}

SampleApexRendererBoneBuffer::~SampleApexRendererBoneBuffer(void)
{
	if(mBones)
	{
		delete[] mBones;
	}

	if(mTexture)
	{
		mTexture->Release();
	}

	if(mShaderResourceView)
	{
		mShaderResourceView->Release();
	}
}

void SampleApexRendererBoneBuffer::writeBuffer(const apex::RenderBoneBufferData& data, uint32_t firstBone,
                                               uint32_t numBones)
{
	const apex::RenderSemanticData& semanticData = data.getSemanticData(apex::RenderBoneSemantic::POSE);
	if(!semanticData.data)
		return;

	const void* srcData = semanticData.data;
	const uint32_t srcStride = semanticData.stride;
	for(uint32_t i = 0; i < numBones; i++)
	{
		mBones[firstBone + i] = PxMat44(*((const PxMat44*)srcData));
		srcData = ((uint8_t*)srcData) + srcStride;
	}

	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	CD3D11_BOX box(0, (LONG)firstBone, 0, 4, (LONG)(firstBone + numBones), 1);
	context->UpdateSubresource(mTexture, 0, &box, mBones, 16 * 4, 16 * 4 * mMaxBones);
}

void SampleApexRendererBoneBuffer::bind(uint32_t slot)
{
	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	context->VSSetShaderResources(slot, 1, &mShaderResourceView);
	context->PSSetShaderResources(slot, 1, &mShaderResourceView);
}

void SampleApexRendererBoneBuffer::unbind()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											SampleApexRendererSpriteBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererSpriteBuffer::SampleApexRendererSpriteBuffer(ID3D11Device& d3dDevice, const nvidia::apex::UserRenderSpriteBufferDesc& desc)
: mDevice(d3dDevice)
{
	for (uint32_t i = 0; i < apex::RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
	{
		if (desc.semanticOffsets[i] != static_cast<uint32_t>(-1))
		{
			mDataFormat[i] = RenderSpriteLayoutElement::getSemanticFormat((apex::RenderSpriteLayoutElement::Enum)i);
		}
		else
		{
			mDataFormat[i] = RenderDataFormat::UNSPECIFIED;
		}
	}

	mStride = desc.stride;

	D3D11_BUFFER_DESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = (UINT)(desc.maxSprites * mStride);
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	V(mDevice.CreateBuffer(&bufferDesc, NULL, &mVertexBuffer));
}

SampleApexRendererSpriteBuffer::~SampleApexRendererSpriteBuffer(void)
{
	if (mVertexBuffer)
	{
		mVertexBuffer->Release();
		mVertexBuffer = NULL;
	}
}

void SampleApexRendererSpriteBuffer::addVertexElements(uint32_t streamIndex,
	std::vector<D3D11_INPUT_ELEMENT_DESC>& vertexElements) const
{
	uint32_t offset = 0;

	for (uint32_t i = 0; i < apex::RenderSpriteLayoutElement::NUM_SEMANTICS; i++)
	{
		apex::RenderSpriteSemantic::Enum semantic = RenderSpriteLayoutElement::getSemantic((apex::RenderSpriteLayoutElement::Enum)i);

		if (mDataFormat[i] != RenderDataFormat::UNSPECIFIED)
		{
			uint8_t d3dUsageIndex = 0;
			LPCSTR d3dUsageName = "";
			getD3DUsage(semantic, d3dUsageName, d3dUsageIndex);

			D3D11_INPUT_ELEMENT_DESC element;
			element.SemanticName = d3dUsageName;
			element.SemanticIndex = d3dUsageIndex;
			element.Format = getD3DFormat(mDataFormat[i]);
			element.InputSlot = streamIndex;
			element.AlignedByteOffset = offset;
			element.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;

			vertexElements.push_back(element);

			offset += (uint32_t)RenderDataFormat::getFormatDataSize(mDataFormat[i]);
		}
	}
}

void SampleApexRendererSpriteBuffer::writeBuffer(const void* srcData, uint32_t firstSprite, uint32_t numSprites)
{
	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	D3D11_MAPPED_SUBRESOURCE mappedRead;
	V(context->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

	::memcpy(static_cast<char*>(mappedRead.pData) + firstSprite * mStride, srcData, mStride * numSprites);

	context->Unmap(mVertexBuffer, 0);
}

void SampleApexRendererSpriteBuffer::writeTexture(uint32_t textureId, uint32_t numSprites, const void* srcData, size_t srcSize)
{
	PX_ALWAYS_ASSERT();
	PX_UNUSED(textureId);
	PX_UNUSED(numSprites);
	PX_UNUSED(srcData);
	PX_UNUSED(srcSize);
}

void SampleApexRendererSpriteBuffer::bind(ID3D11DeviceContext& context, uint32_t streamID, uint32_t firstVertex)
{
	ID3D11Buffer* pBuffers[1] = { mVertexBuffer };
	UINT strides[1] = { mStride };
	UINT offsets[1] = { firstVertex * mStride };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

void SampleApexRendererSpriteBuffer::unbind(ID3D11DeviceContext& context, uint32_t streamID)
{
	ID3D11Buffer* pBuffers[1] = { NULL };
	UINT strides[1] = { 0 };
	UINT offsets[1] = { 0 };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											SampleApexRendererInstanceBuffer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererInstanceBuffer::SampleApexRendererInstanceBuffer(ID3D11Device& d3dDevice, const nvidia::apex::UserRenderInstanceBufferDesc& desc)
	: mDevice(d3dDevice)
{
	for (uint32_t i = 0; i < apex::RenderInstanceLayoutElement::NUM_SEMANTICS; i++)
	{
		if (desc.semanticOffsets[i] != static_cast<uint32_t>(-1))
		{
			RenderDataFormat::Enum dataFormat = RenderInstanceLayoutElement::getSemanticFormat((apex::RenderInstanceLayoutElement::Enum)i);
			if (dataFormat == RenderDataFormat::FLOAT3x3)
			{
				mDataFormat.push_back(RenderDataFormat::FLOAT3);
				mDataFormat.push_back(RenderDataFormat::FLOAT3);
				mDataFormat.push_back(RenderDataFormat::FLOAT3);
			}
			else
			{
				mDataFormat.push_back(dataFormat);
			}
		}
		else
		{
		}
	}

	mStride = desc.stride;

	D3D11_BUFFER_DESC bufferDesc;

	memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = (UINT)(desc.maxInstances * mStride);
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

	V(mDevice.CreateBuffer(&bufferDesc, NULL, &mInstanceBuffer));
}

SampleApexRendererInstanceBuffer::~SampleApexRendererInstanceBuffer(void)
{
	if (mInstanceBuffer)
	{
		mInstanceBuffer->Release();
		mInstanceBuffer = NULL;
	}
}

void SampleApexRendererInstanceBuffer::addVertexElements(uint32_t streamIndex,
	std::vector<D3D11_INPUT_ELEMENT_DESC>& vertexElements) const
{
	uint32_t offset = 0;

	for (uint32_t i = 0; i < mDataFormat.size(); i++)
	{
		RenderDataFormat::Enum dataFormat = mDataFormat[i];

		D3D11_INPUT_ELEMENT_DESC element;
		element.SemanticName = "TEXCOORD";
		element.SemanticIndex = 9 + i;
		element.Format = getD3DFormat(dataFormat);
		element.InputSlot = streamIndex;
		element.AlignedByteOffset = offset;
		element.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
		element.InstanceDataStepRate = 1;

		vertexElements.push_back(element);

		offset += (uint32_t)RenderDataFormat::getFormatDataSize(dataFormat);
	}
}

void SampleApexRendererInstanceBuffer::writeBuffer(const void* srcData, uint32_t firstInstance, uint32_t numInstances)
{
	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	D3D11_MAPPED_SUBRESOURCE mappedRead;
	V(context->Map(mInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedRead));

	::memcpy(static_cast<char*>(mappedRead.pData) + firstInstance * mStride, srcData, mStride * numInstances);

	context->Unmap(mInstanceBuffer, 0);
}

void SampleApexRendererInstanceBuffer::bind(ID3D11DeviceContext& context, uint32_t streamID, uint32_t firstVertex)
{
	ID3D11Buffer* pBuffers[1] = { mInstanceBuffer };
	UINT strides[1] = { mStride };
	UINT offsets[1] = { firstVertex * mStride };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}

void SampleApexRendererInstanceBuffer::unbind(ID3D11DeviceContext& context, uint32_t streamID)
{
	ID3D11Buffer* pBuffers[1] = { NULL };
	UINT strides[1] = { 0 };
	UINT offsets[1] = { 0 };
	context.IASetVertexBuffers(streamID, 1, pBuffers, strides, offsets);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											  SampleApexRendererMesh
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

SampleApexRendererMesh::SampleApexRendererMesh(ID3D11Device& d3dDevice, const apex::UserRenderResourceDesc& desc)
: mDevice(d3dDevice)
{
	mVertexBuffers = NULL;
	mNumVertexBuffers = 0;
	mIndexBuffer = NULL;
	mSpriteBuffer = NULL;
	mInstanceBuffer = NULL;
	mMaterialInstance = NULL;

	mBoneBuffer = NULL;

	mNumVertexBuffers = desc.numVertexBuffers;
	if(mNumVertexBuffers > 0)
	{
		mVertexBuffers = new SampleApexRendererVertexBuffer* [mNumVertexBuffers];
		for(uint32_t i = 0; i < mNumVertexBuffers; i++)
		{
			mVertexBuffers[i] = static_cast<SampleApexRendererVertexBuffer*>(desc.vertexBuffers[i]);
		}
	}

	if (desc.indexBuffer)
		mIndexBuffer = static_cast<SampleApexRendererIndexBuffer*>(desc.indexBuffer);
	
	if (desc.boneBuffer)
		mBoneBuffer = static_cast<SampleApexRendererBoneBuffer*>(desc.boneBuffer);

	if (desc.spriteBuffer)
		mSpriteBuffer = static_cast<SampleApexRendererSpriteBuffer*>(desc.spriteBuffer);

	if (desc.instanceBuffer)
		mInstanceBuffer = static_cast<SampleApexRendererInstanceBuffer*>(desc.instanceBuffer);

	mCullMode = desc.cullMode;
	mPrimitiveTopology = desc.primitives == RenderPrimitiveType::POINT_SPRITES ? D3D_PRIMITIVE_TOPOLOGY_POINTLIST :
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	setVertexBufferRange(desc.firstVertex, desc.numVerts);
	setIndexBufferRange(desc.firstIndex, desc.numIndices);
	setBoneBufferRange(desc.firstBone, desc.numBones);
	setSpriteBufferRange(desc.firstSprite, desc.numSprites);
	setInstanceBufferRange(desc.firstInstance, desc.numInstances);

	if (desc.material)
		setMaterial(desc.material);
}

void SampleApexRendererMesh::setMaterial(void* material)
{
	ApexRenderMaterial* renderMaterial = static_cast<ApexRenderMaterial*>(material);
	if (mMaterial != renderMaterial)
	{
		mMaterial = renderMaterial;

		std::vector<D3D11_INPUT_ELEMENT_DESC> descs;

		for (uint32_t i = 0; i < mNumVertexBuffers; i++)
		{
			mVertexBuffers[i]->addVertexElements(i, descs);
		}

		if (mSpriteBuffer)
			mSpriteBuffer->addVertexElements(0, descs);

		if (mInstanceBuffer)
			mInstanceBuffer->addVertexElements(mNumVertexBuffers, descs);

		mMaterialInstance = renderMaterial->getMaterialInstance(&descs[0], (uint32_t)descs.size());
	}
}

SampleApexRendererMesh::~SampleApexRendererMesh(void)
{
	if(mVertexBuffers)
	{
		delete[] mVertexBuffers;
	}
}

void SampleApexRendererMesh::setVertexBufferRange(uint32_t firstVertex, uint32_t numVerts)
{
	mFirstVertex = firstVertex;
	mNumVertices = numVerts;
}

void SampleApexRendererMesh::setIndexBufferRange(uint32_t firstIndex, uint32_t numIndices)
{
	mFirstIndex = firstIndex;
	mNumIndices = numIndices;
}

void SampleApexRendererMesh::setBoneBufferRange(uint32_t firstBone, uint32_t numBones)
{
	mFirstBone = firstBone;
	mNumBones = numBones;
}

void SampleApexRendererMesh::setInstanceBufferRange(uint32_t firstInstance, uint32_t numInstances)
{
	mFirstInstance = firstInstance;
	mNumInstances = numInstances;
}

void SampleApexRendererMesh::setSpriteBufferRange(uint32_t firstSprite, uint32_t numSprites)
{
	mFirstSprite = firstSprite;
	mNumSprites = numSprites;
}

void SampleApexRendererMesh::render(const apex::RenderContext& c)
{
	PX_UNUSED(c);
	if (mMaterialInstance == NULL || !mMaterialInstance->isValid())
		return;

	ID3D11DeviceContext* context;
	mDevice.GetImmediateContext(&context);

	for(uint32_t i = 0; i < mNumVertexBuffers; i++)
	{
		mVertexBuffers[i]->bind(*context, i, 0);
	}

	if (mIndexBuffer)
		mIndexBuffer->bind(*context);

	mMaterialInstance->bind(*context, 0);

	if (mBoneBuffer)
		mBoneBuffer->bind(1);

	if (mSpriteBuffer)
		mSpriteBuffer->bind(*context, mNumVertexBuffers, mFirstSprite);

	if (mInstanceBuffer)
		mInstanceBuffer->bind(*context, mNumVertexBuffers, mFirstInstance);

	context->IASetPrimitiveTopology(mPrimitiveTopology);

	if (mIndexBuffer && mInstanceBuffer)
		context->DrawIndexedInstanced(mNumIndices, mNumInstances, mFirstIndex, 0, mFirstInstance);
	else if (mIndexBuffer)
		context->DrawIndexed(mNumIndices, mFirstIndex, 0);
	else if (mInstanceBuffer)
		context->DrawInstanced(mNumVertices, mNumInstances, mFirstVertex, mFirstInstance);
	else if (mSpriteBuffer)
		context->Draw(mNumSprites, mFirstSprite);

	for(uint32_t i = 0; i < mNumVertexBuffers; i++)
	{
		mVertexBuffers[i]->unbind(*context, i);
	}

	if (mIndexBuffer)
		mIndexBuffer->unbind(*context);

	if (mInstanceBuffer)
		mInstanceBuffer->unbind(*context, 0);

	if (mSpriteBuffer)
		mSpriteBuffer->unbind(*context, 0);

	if (mBoneBuffer != NULL)
		mBoneBuffer->unbind();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
