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


#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include "ApexUsingNamespace.h"
#include "PxVec3.h"

/**
	Texture map types.  Currently the MaterialLibrary interface only supports one map of each type.
*/
enum TextureMapType
{
	DIFFUSE_MAP = 0,
	BUMP_MAP,
	NORMAL_MAP,

	TEXTURE_MAP_TYPE_COUNT
};


/**
	Texture pixel format types, a supported by the Destruction tool's bmp, dds, and tga texture loaders.
*/
enum PixelFormat
{
	PIXEL_FORMAT_UNKNOWN,

	PIXEL_FORMAT_RGB,
	PIXEL_FORMAT_BGR_EXT,
	PIXEL_FORMAT_BGRA_EXT,
	PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	PIXEL_FORMAT_COMPRESSED_RGBA_S3TC_DXT5_EXT,
};


/**
	Basic interface to query generic texture map data.
*/
class TextureMap
{
public:

	/** The texture pixel format, supported by the Destruction tool's texture loaders. */
	virtual PixelFormat	getPixelFormat() const = 0;

	/** The horizontal texture map size, in pixels. */
	virtual uint32_t				getWidth() const = 0;

	/** The vertical texture map size, in pixels. */
	virtual uint32_t				getHeight() const = 0;

	/** The number of color/alpha/etc. channels. */
	virtual uint32_t				getComponentCount() const = 0;

	/** The size, in bytes, of the pixel buffer. */
	virtual uint32_t				getPixelBufferSize() const = 0;

	/** The beginning address of the pixel buffer. */
	virtual uint8_t*				getPixels() const = 0;
};


/**
	Basic interface to query generic render material data.
*/
class Material
{
public:

	/** The material's name, used by the named resource provider. */
	virtual const char*			getName() const = 0;

	/** Access to the material's texture maps, indexed by TextureMapType. */
	virtual TextureMap*	getTextureMap(TextureMapType type) const = 0;

	/** The ambient lighting color of the material. */
	virtual const physx::PxVec3&		getAmbient() const = 0;

	/** The diffuse lighting color of the material. */
	virtual const physx::PxVec3&		getDiffuse() const = 0;

	/** The specular lighting color of the material. */
	virtual const physx::PxVec3&		getSpecular() const = 0;

	/** The opacity of the material. */
	virtual float				getAlpha() const = 0;

	/** The shininess (specular power) of the material. */
	virtual float				getShininess() const = 0;
};


/**
	Material library skeleton interface.
*/
class MaterialLibrary
{
public:

	/** Saves the material to an physx::PxFileBuf. */
	virtual void			serialize(physx::PxFileBuf& stream) const = 0;

	/** Loads material from an physx::PxFileBuf. */
	virtual void			deserialize(physx::PxFileBuf& stream) = 0;

	/**
		Query a material by name.
		If the material already exists, it is returned and 'created' is set to false.
		If the material did not exist before, it is created, returned, and 'created' is set to true.
	*/
	virtual Material*	getMaterial(const char* materialName, bool& created) = 0;

	/**
		Remove and delete named material.
		Returns true if the material was found, false if it was not.
	*/
	virtual bool			deleteMaterial(const char* materialName) = 0;
};


#endif // #ifndef MATERIAL_LIBRARY_H
