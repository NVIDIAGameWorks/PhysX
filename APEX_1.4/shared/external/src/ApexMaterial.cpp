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


#define NOMINMAX
#include <stdio.h>

#include "RenderMesh.h"
#include "PxVec3.h"
#include "PxFileBuf.h"
#include "PsAllocator.h"
#include "ApexUsingNamespace.h"
#include "ApexMaterial.h"

// Local utilities

PX_INLINE bool pathsMatch(const char* pathA, const char* pathB, int length)
{
	while (length-- > 0)
	{
		char a = *pathA++;
		char b = *pathB++;
		if (a == '\\')
		{
			a = '/';
		}
		if (b == '\\')
		{
			b = '/';
		}
		if (a != b)
		{
			return false;
		}
		if (a == '\0')
		{
			return true;
		}
	}

	return true;
}

struct MaterialNameComponents
{
	const char* path;
	int pathLen;
	const char* filename;
	int filenameLen;
	const char* ext;
	int extLen;
	const char* name;
	int nameLen;

	bool	operator <= (const MaterialNameComponents& c) const
	{
		if (pathLen > 0 && !pathsMatch(path, c.path, pathLen))
		{
			return false;
		}
		if (filenameLen > 0 && strncmp(filename, c.filename, (uint32_t)filenameLen))
		{
			return false;
		}
		if (extLen > 0 && strncmp(ext, c.ext, (uint32_t)extLen))
		{
			return false;
		}
		return 0 == strncmp(name, c.name, (uint32_t)nameLen);
	}
};

PX_INLINE void decomposeMaterialName(MaterialNameComponents& components, const char* materialName)
{
	components.path = materialName;
	components.pathLen = 0;
	components.filename = materialName;
	components.filenameLen = 0;
	components.ext = materialName;
	components.extLen = 0;
	components.name = materialName;
	components.nameLen = 0;

	if (materialName == NULL)
	{
		return;
	}

	const int len = (int)strlen(materialName);

	// Get name - will exclude any '#' deliniator
	components.name += len;
	while (components.name > materialName)
	{
		if (*(components.name - 1) == '#')
		{
			break;
		}
		--components.name;
	}
	components.nameLen = len - (int)(components.name - materialName);
	if (components.name == materialName)
	{
		return;
	}

	// Get extension - will include '.'
	components.ext = components.name;
	while (components.ext > materialName)
	{
		if (*(--components.ext) == '.')
		{
			break;
		}
	}
	if (components.ext != materialName)
	{
		components.extLen = (int)(components.name - components.ext) - 1;
	}

	// Get filename
	components.filename = components.ext;
	while (components.filename > materialName)
	{
		if (*(components.filename - 1) == '/' || *(components.filename - 1) == '\\')
		{
			break;
		}
		--components.filename;
	}
	if (components.filename != materialName)
	{
		components.filenameLen = (int)(components.ext - components.filename);
	}

	// Get path
	components.path = materialName;
	components.pathLen = (int)(components.filename - materialName);
}

PX_INLINE void copyToStringBufferSafe(char*& buffer, int& bufferSize, const char* src, int copySize)
{
	if (copySize >= bufferSize)
	{
		copySize = bufferSize - 1;
	}
	memcpy(buffer, src, (uint32_t)copySize);
	buffer += copySize;
	bufferSize -= copySize;
}

struct ApexDefaultMaterialLibraryVersion
{
	enum
	{
		Initial = 0,
		AddedBumpMapType,
		AddedMaterialNamingConvention,
		RemovedMaterialNamingConvention,

		Count,
		Current = Count - 1
	};
};


PX_INLINE void serialize_string(physx::PxFileBuf& stream, const std::string& string)
{
	const uint32_t length = (uint32_t)string.length();
	stream.storeDword(length);
	stream.write(string.c_str(), length);
}

PX_INLINE void deserialize_string(physx::PxFileBuf& stream, std::string& string)
{
	const uint32_t length = stream.readDword();
	char* cstr = (char*)PxAlloca(length + 1);
	stream.read(cstr, length);
	cstr[length] = '\0';
	string = cstr;
}


// ApexDefaultTextureMap functions

ApexDefaultTextureMap::ApexDefaultTextureMap() :
	mPixelFormat(PIXEL_FORMAT_UNKNOWN),
	mWidth(0),
	mHeight(0),
	mComponentCount(0),
	mPixelBufferSize(0),
	mPixelBuffer(NULL)
{
}

ApexDefaultTextureMap& ApexDefaultTextureMap::operator = (const ApexDefaultTextureMap& textureMap)
{
	mPixelFormat = textureMap.mPixelFormat;
	mWidth = textureMap.mWidth;
	mHeight = textureMap.mHeight;
	mComponentCount = textureMap.mComponentCount;
	mPixelBufferSize = textureMap.mPixelBufferSize;
	mPixelBuffer = new uint8_t[mPixelBufferSize];
	memcpy(mPixelBuffer, textureMap.mPixelBuffer, mPixelBufferSize);
	return *this;
}

ApexDefaultTextureMap::~ApexDefaultTextureMap()
{
	unload();
}

void
ApexDefaultTextureMap::build(PixelFormat format, uint32_t width, uint32_t height, uint32_t* fillColor)
{
	uint8_t fillBuffer[4];
	int componentCount;

	switch (format)
	{
	case PIXEL_FORMAT_RGB:
		componentCount = 3;
		if (fillColor != NULL)
		{
			fillBuffer[0] = (*fillColor >> 16) & 0xFF;
			fillBuffer[1] = (*fillColor >> 8) & 0xFF;
			fillBuffer[2] = (*fillColor) & 0xFF;
		}
		break;
	case PIXEL_FORMAT_BGR_EXT:
		componentCount = 3;
		if (fillColor != NULL)
		{
			fillBuffer[0] = (*fillColor) & 0xFF;
			fillBuffer[1] = (*fillColor >> 8) & 0xFF;
			fillBuffer[2] = (*fillColor >> 16) & 0xFF;
		}
		break;
	case PIXEL_FORMAT_BGRA_EXT:
		componentCount = 4;
		if (fillColor != NULL)
		{
			fillBuffer[0] = (*fillColor) & 0xFF;
			fillBuffer[1] = (*fillColor >> 8) & 0xFF;
			fillBuffer[2] = (*fillColor >> 16) & 0xFF;
			fillBuffer[3] = (*fillColor >> 24) & 0xFF;
		}
		break;
	default:
		return;	// Not supported
	}

	unload();

	mPixelBufferSize = componentCount * width * height;
	mPixelBuffer = new uint8_t[mPixelBufferSize];
	mPixelFormat = format;
	mComponentCount = (uint32_t)componentCount;
	mWidth = width;
	mHeight = height;

	if (fillColor != NULL)
	{
		uint8_t* write = mPixelBuffer;
		uint8_t* writeStop = mPixelBuffer + mPixelBufferSize;
		uint8_t* read = fillBuffer;
		uint8_t* readStop = fillBuffer + componentCount;
		while (write < writeStop)
		{
			*write++ = *read++;
			if (read == readStop)
			{
				read = fillBuffer;
			}
		}
	}
}

void ApexDefaultTextureMap::unload()
{
	mPixelFormat = PIXEL_FORMAT_UNKNOWN;
	mWidth = 0;
	mHeight = 0;
	mComponentCount = 0;
	mPixelBufferSize = 0;
	delete [] mPixelBuffer;
	mPixelBuffer = NULL;
}

void ApexDefaultTextureMap::serialize(physx::PxFileBuf& stream) const
{
	stream.storeDword((uint32_t)mPixelFormat);
	stream.storeDword(mWidth);
	stream.storeDword(mHeight);
	stream.storeDword(mComponentCount);
	stream.storeDword(mPixelBufferSize);
	if (mPixelBufferSize != 0)
	{
		stream.write(mPixelBuffer, mPixelBufferSize);
	}
}

void ApexDefaultTextureMap::deserialize(physx::PxFileBuf& stream, uint32_t /*version*/)
{
	unload();
	mPixelFormat = (PixelFormat)stream.readDword();
	mWidth = stream.readDword();
	mHeight = stream.readDword();
	mComponentCount = stream.readDword();
	mPixelBufferSize = stream.readDword();
	if (mPixelBufferSize != 0)
	{
		mPixelBuffer = new uint8_t[mPixelBufferSize];
		stream.read(mPixelBuffer, mPixelBufferSize);
	}
}


// ApexDefaultMaterial functions
ApexDefaultMaterial::ApexDefaultMaterial() :
	mAmbient(0.0f),
	mDiffuse(0.0f),
	mSpecular(0.0f),
	mAlpha(0.0f),
	mShininess(0.0f)
{
	for (uint32_t i = 0; i < TEXTURE_MAP_TYPE_COUNT; ++i)
	{
		mTextureMaps[i] = NULL;
	}
}

ApexDefaultMaterial& ApexDefaultMaterial::operator = (const ApexDefaultMaterial& material)
{
	mName = material.mName;

	for (uint32_t i = 0; i < TEXTURE_MAP_TYPE_COUNT; ++i)
	{
		if (material.mTextureMaps[i])
		{
			mTextureMaps[i] = new ApexDefaultTextureMap(*material.mTextureMaps[i]);
		}
		else
		{
			mTextureMaps[i] = NULL;
		}
	}

	mAmbient = material.mAmbient;
	mDiffuse = material.mDiffuse;
	mSpecular = material.mSpecular;
	mAlpha = material.mAlpha;
	mShininess = material.mShininess;

	return *this;
}

ApexDefaultMaterial::~ApexDefaultMaterial()
{
	unload();
}

void ApexDefaultMaterial::setName(const char* name)
{
	mName = name;
}

bool ApexDefaultMaterial::setTextureMap(TextureMapType type, ApexDefaultTextureMap* textureMap)
{
	if (type < 0 || type >= TEXTURE_MAP_TYPE_COUNT)
	{
		return false;
	}

	delete mTextureMaps[type];

	mTextureMaps[type] = textureMap;

	return true;
}

void ApexDefaultMaterial::unload()
{
	mName.clear();

	for (uint32_t i = 0; i < TEXTURE_MAP_TYPE_COUNT; ++i)
	{
		delete mTextureMaps[i];
		mTextureMaps[i] = NULL;
	}

	mAmbient = physx::PxVec3(0.0f);
	mDiffuse = physx::PxVec3(0.0f);
	mSpecular = physx::PxVec3(0.0f);
	mAlpha = 0.0f;
	mShininess = 0.0f;
}


void ApexDefaultMaterial::serialize(physx::PxFileBuf& stream) const
{
	serialize_string(stream, mName);

	for (uint32_t i = 0; i < TEXTURE_MAP_TYPE_COUNT; ++i)
	{
		if (mTextureMaps[i] == NULL)
		{
			stream.storeDword((uint32_t)0);
		}
		else
		{
			stream.storeDword((uint32_t)1);
			mTextureMaps[i]->serialize(stream);
		}
	}

	stream.storeFloat(mAmbient.x);
	stream.storeFloat(mAmbient.y);
	stream.storeFloat(mAmbient.z);
	stream.storeFloat(mDiffuse.x);
	stream.storeFloat(mDiffuse.y);
	stream.storeFloat(mDiffuse.z);
	stream.storeFloat(mSpecular.x);
	stream.storeFloat(mSpecular.y);
	stream.storeFloat(mSpecular.z);
	stream.storeFloat(mAlpha);
	stream.storeFloat(mShininess);
}

void ApexDefaultMaterial::deserialize(physx::PxFileBuf& stream, uint32_t version)
{
	unload();

	deserialize_string(stream, mName);

	if (version < ApexDefaultMaterialLibraryVersion::AddedBumpMapType)
	{
		for (uint32_t i = 0; i < 2; ++i)
		{
			const uint32_t pointerIsValid = stream.readDword();
			if (pointerIsValid)
			{
				mTextureMaps[i] = new ApexDefaultTextureMap();
				mTextureMaps[i]->deserialize(stream, version);
			}
		}
		mTextureMaps[2] = mTextureMaps[1];
		mTextureMaps[1] = NULL;
	}
	else
	{
		for (uint32_t i = 0; i < TEXTURE_MAP_TYPE_COUNT; ++i)
		{
			const uint32_t pointerIsValid = stream.readDword();
			if (pointerIsValid)
			{
				mTextureMaps[i] = new ApexDefaultTextureMap();
				mTextureMaps[i]->deserialize(stream, version);
			}
		}
	}

	mAmbient.x = stream.readFloat();
	mAmbient.y = stream.readFloat();
	mAmbient.z = stream.readFloat();
	mDiffuse.x = stream.readFloat();
	mDiffuse.y = stream.readFloat();
	mDiffuse.z = stream.readFloat();
	mSpecular.x = stream.readFloat();
	mSpecular.y = stream.readFloat();
	mSpecular.z = stream.readFloat();
	mAlpha = stream.readFloat();
	mShininess = stream.readFloat();
}

TextureMap* ApexDefaultMaterial::getTextureMap(TextureMapType type) const
{
	if (type < 0 || type >= TEXTURE_MAP_TYPE_COUNT)
	{
		return NULL;
	}

	return mTextureMaps[type];
}


// ApexDefaultMaterialLibrary functions
ApexDefaultMaterialLibrary::ApexDefaultMaterialLibrary()
{
}

ApexDefaultMaterialLibrary& ApexDefaultMaterialLibrary::operator = (const ApexDefaultMaterialLibrary& materialLibrary)
{
	mMaterials.resize(materialLibrary.getMaterialCount());
	for (uint32_t i = 0; i < materialLibrary.getMaterialCount(); ++i)
	{
		ApexDefaultMaterial* material = materialLibrary.getMaterial(i);
		PX_ASSERT(material != NULL);
		mMaterials[i] = new ApexDefaultMaterial(*material);
	}

	return *this;
}

ApexDefaultMaterialLibrary::~ApexDefaultMaterialLibrary()
{
	unload();
}

void ApexDefaultMaterialLibrary::unload()
{
	const uint32_t size = (uint32_t)mMaterials.size();
	for (uint32_t i = 0; i < size; ++i)
	{
		delete mMaterials[i];
		mMaterials[i] = NULL;
	}
	mMaterials.resize(0);
}

ApexDefaultMaterial* ApexDefaultMaterialLibrary::getMaterial(uint32_t materialIndex) const
{
	if (materialIndex >= mMaterials.size())
	{
		return NULL;
	}

	return mMaterials[materialIndex];
}

void ApexDefaultMaterialLibrary::merge(const ApexDefaultMaterialLibrary& materialLibrary)
{
	for (uint32_t i = 0; i < materialLibrary.getMaterialCount(); ++i)
	{
		ApexDefaultMaterial* material = materialLibrary.getMaterial(i);
		PX_ASSERT(material != NULL);
		const uint32_t size = (uint32_t)mMaterials.size();
		uint32_t j = 0;
		for (; j < size; ++j)
		{
			if (!strcmp(mMaterials[j]->getName(), material->getName()))
			{
				break;
			}
		}
		if (j == size)
		{
			ApexDefaultMaterial* newMaterial = new ApexDefaultMaterial(*material);
			mMaterials.push_back(newMaterial);
		}
	}
}

void ApexDefaultMaterialLibrary::serialize(physx::PxFileBuf& stream) const
{
	stream.storeDword((uint32_t)ApexDefaultMaterialLibraryVersion::Current);

	const uint32_t size = (uint32_t)mMaterials.size();
	stream.storeDword(size);
	for (uint32_t i = 0; i < size; ++i)
	{
		mMaterials[i]->serialize(stream);
	}
}

void ApexDefaultMaterialLibrary::deserialize(physx::PxFileBuf& stream)
{
	unload();

	uint32_t version = stream.readDword();

	uint32_t size = stream.readDword();
	mMaterials.resize(size);
	for (uint32_t i = 0; i < size; ++i)
	{
		mMaterials[i] = new ApexDefaultMaterial();
		mMaterials[i]->deserialize(stream, version);
	}

	if (version >= ApexDefaultMaterialLibraryVersion::AddedMaterialNamingConvention  && version < ApexDefaultMaterialLibraryVersion::RemovedMaterialNamingConvention)
	{
		stream.readDword();	// Eat naming convention
	}
}

Material* ApexDefaultMaterialLibrary::getMaterial(const char* materialName, bool& created)
{
	int32_t index = findMaterialIndex(materialName);
	if (index >= 0)
	{
		created = false;
		return mMaterials[(uint32_t)index];
	}

	ApexDefaultMaterial* newMaterial = new ApexDefaultMaterial();
	newMaterial->setName(materialName);
	mMaterials.push_back(newMaterial);
	created = true;
	return newMaterial;
}

bool ApexDefaultMaterialLibrary::deleteMaterial(const char* materialName)
{
	int32_t index = findMaterialIndex(materialName);
	if (index < 0)
	{
		return false;
	}

	ApexDefaultMaterial* material = mMaterials[(uint32_t)index];
	delete material;

	mMaterials[(uint32_t)index] = mMaterials[mMaterials.size() - 1];
	mMaterials.resize(mMaterials.size() - 1);

	return true;
}

int32_t ApexDefaultMaterialLibrary::findMaterialIndex(const char* materialName)
{
	const char blank[] = "";
	materialName = materialName ? materialName : blank;

	const uint32_t size = (uint32_t)mMaterials.size();
	int32_t index = 0;
	for (; index < (int32_t)size; ++index)
	{
		const char* existingMaterialName = mMaterials[(uint32_t)index]->getName() ? mMaterials[(uint32_t)index]->getName() : blank;
		if (!strcmp(existingMaterialName, materialName))
		{
			return index;
		}
	}

	return -1;
}
