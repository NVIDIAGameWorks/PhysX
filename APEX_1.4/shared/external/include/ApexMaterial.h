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


#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <stdio.h>
#include <string>
#include "ApexUsingNamespace.h"
#include "RenderMesh.h"
#include "MaterialLibrary.h"
#include "PxVec3.h"
#include <vector>

/**
	A generic texture map.  Loads from a variety of file formats, but is stored in a unified basic format.
	May be (de)serialized from/to an physx::PxFileBuf.
*/
class ApexDefaultTextureMap : public TextureMap
{
public:
	ApexDefaultTextureMap();
	ApexDefaultTextureMap(const ApexDefaultTextureMap& textureMap)
	{
		*this = textureMap;
	}
	ApexDefaultTextureMap&	operator = (const ApexDefaultTextureMap& textureMap);
	virtual					~ApexDefaultTextureMap();

	void					build(PixelFormat format, uint32_t width, uint32_t height, uint32_t* fillColor = NULL);

	/** Deallocates all buffers and sets all values to the default constructor values. */
	void					unload();

	/** Saves the generic texture data to an physx::PxFileBuf. */
	void					serialize(physx::PxFileBuf& stream) const;

	/** Loads generic texture data from an physx::PxFileBuf. */
	void					deserialize(physx::PxFileBuf& stream, uint32_t version);

	// Texture API
	PixelFormat		getPixelFormat() const
	{
		return mPixelFormat;
	}
	uint32_t					getWidth() const
	{
		return mWidth;
	}
	uint32_t					getHeight() const
	{
		return mHeight;
	}
	uint32_t					getComponentCount() const
	{
		return mComponentCount;
	}
	uint32_t					getPixelBufferSize() const
	{
		return mPixelBufferSize;
	}
	uint8_t*					getPixels() const
	{
		return mPixelBuffer;
	}

protected:

	PixelFormat	mPixelFormat;
	uint32_t				mWidth;
	uint32_t				mHeight;
	uint32_t				mComponentCount;
	uint32_t				mPixelBufferSize;
	uint8_t*				mPixelBuffer;
};


class ApexDefaultMaterial : public Material
{
public:

	ApexDefaultMaterial();
	ApexDefaultMaterial(const ApexDefaultMaterial& material)
	{
		*this = material;
	}
	ApexDefaultMaterial&	operator = (const ApexDefaultMaterial& material);
	virtual					~ApexDefaultMaterial();

	/** Sets the name of the material, for lookup by the named resource provider. */
	void					setName(const char* name);

	/** Sets one of the material's texture maps (diffuse or normal) */
	bool					setTextureMap(TextureMapType type, ApexDefaultTextureMap* textureMap);

	/** Sets the ambient lighting color. */
	void					setAmbient(const physx::PxVec3& ambient)
	{
		mAmbient = ambient;
	}

	/** Sets the diffuse lighting color. */
	void					setDiffuse(const physx::PxVec3& diffuse)
	{
		mDiffuse = diffuse;
	}

	/** Sets the specular lighting color. */
	void					setSpecular(const physx::PxVec3& specular)
	{
		mSpecular = specular;
	}

	/** Sets material's opacity. */
	void					setAlpha(float alpha)
	{
		mAlpha = alpha;
	}

	/** Sets the material's shininess (specular power). */
	void					setShininess(float shininess)
	{
		mShininess = shininess;
	}

	/** Deallocates all buffers and sets all values to the default constructor values. */
	void					unload();

	/** Saves the material to an physx::PxFileBuf. */
	void					serialize(physx::PxFileBuf& stream) const;

	/** Loads material from an physx::PxFileBuf. */
	void					deserialize(physx::PxFileBuf& stream, uint32_t version);

	// Material API
	const char*				getName() const
	{
		return mName.c_str();
	}
	TextureMap*		getTextureMap(TextureMapType type) const;
	const physx::PxVec3&			getAmbient() const
	{
		return mAmbient;
	}
	const physx::PxVec3&			getDiffuse() const
	{
		return mDiffuse;
	}
	const physx::PxVec3&			getSpecular() const
	{
		return mSpecular;
	}
	float					getAlpha() const
	{
		return mAlpha;
	}
	float					getShininess() const
	{
		return mShininess;
	}

private:

	std::string				mName;

	ApexDefaultTextureMap*	mTextureMaps[TEXTURE_MAP_TYPE_COUNT];

	physx::PxVec3					mAmbient;
	physx::PxVec3					mDiffuse;
	physx::PxVec3					mSpecular;
	float					mAlpha;
	float					mShininess;
};


class ApexDefaultMaterialLibrary : public MaterialLibrary
{
public:

	ApexDefaultMaterialLibrary();
	ApexDefaultMaterialLibrary(const ApexDefaultMaterialLibrary& materialLibrary)
	{
		*this = materialLibrary;
	}
	ApexDefaultMaterialLibrary&	operator = (const ApexDefaultMaterialLibrary& material);
	virtual						~ApexDefaultMaterialLibrary();

	/** Deallocates all buffers and sets all values to the default constructor values. */
	void						unload();

	/** Returns the number of materials in the library */
	uint32_t						getMaterialCount() const
	{
		return (uint32_t)mMaterials.size();
	}

	/**
		Access to the materials by index.
		Valid range of materialIndex is 0 to getMaterialCount()-1.
	*/
	ApexDefaultMaterial*		getMaterial(uint32_t materialIndex) const;

	/**
		Remove and delete named material.
		Returns true if the material was found, false if it was not.
	*/
	virtual bool				deleteMaterial(const char* materialName);

	/**
		Adds the materials from the given materialLibrary, which
		aren't already in this material library.  (Based upon name.)
	*/
	void						merge(const ApexDefaultMaterialLibrary& materialLibrary);

	// MaterialLibrary API

	/** Saves the material to an physx::PxFileBuf. */
	void						serialize(physx::PxFileBuf& stream) const;

	/** Loads material from an physx::PxFileBuf. */
	void						deserialize(physx::PxFileBuf& stream);

	Material*				getMaterial(const char* materialName, bool& created);

	/* Returns -1 if the material is not found */
	int32_t						findMaterialIndex(const char* materialName);

private:

	std::vector<ApexDefaultMaterial*>	mMaterials;
};


#endif // #ifndef _MATERIAL_H_
