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


// PT: this file is a loader for "raw" binary files. It should NOT create SDK objects directly.

#include <stdio.h>
#include "foundation/PxPreprocessor.h"
#include "foundation/PxAssert.h"
#include "RawLoader.h"
#include "PxTkFile.h"

typedef ::FILE	File;

using namespace physx;

RAWObject::RAWObject() : mName(NULL)
{
	mTransform = PxTransform(PxIdentity);
}

RAWTexture::RAWTexture() :
mID(0xffffffff),
mWidth(0),
mHeight(0),
mHasAlpha(false)
//mPixels(NULL)
{
}

RAWMesh::RAWMesh() :
mNbVerts(0),
mNbFaces(0),
mMaterialID(0xffffffff),
mVerts(NULL),
mVertexNormals(NULL),
mVertexColors(NULL),
mUVs(NULL),
mIndices(NULL)
{
}

RAWShape::RAWShape() :
mNbVerts(0),
mVerts(NULL)
{
}

RAWHelper::RAWHelper()
{
}

RAWMaterial::RAWMaterial() :
mID(0xffffffff),
mDiffuseID(0xffffffff),
mOpacity(1.0f),
mDoubleSided(false)
{
	mAmbientColor = PxVec3(0);
	mDiffuseColor = PxVec3(0);
	mSpecularColor = PxVec3(0);
}

#if PX_INTEL_FAMILY
static const bool gFlip = false;
#elif PX_PPC
static const bool gFlip = true;
#elif PX_ARM_FAMILY
static const bool gFlip = false;
#else
#error Unknown platform!
#endif

PX_INLINE void Flip(PxU32& v)
{
	PxU8* b = (PxU8*)&v;

	PxU8 temp = b[0];
	b[0] = b[3];
	b[3] = temp;
	temp = b[1];
	b[1] = b[2];
	b[2] = temp;
}

PX_INLINE void Flip(PxI32& v)
{
	Flip((PxU32&)v);
}

PX_INLINE void Flip(PxF32& v)
{
	Flip((PxU32&)v);
}

static PxU8 read8(File* fp)
{
	PxU8 data;
	size_t numRead = fread(&data, 1, 1, fp);
	if (numRead != 1) { return 0; }
	return data;
}

static PxU32 read32(File* fp)
{
	PxU32 data;
	size_t numRead = fread(&data, 1, 4, fp);
	if (numRead != 4) { return 0; }
	if (gFlip)
		Flip(data);
	return data;
}

static PxF32 readFloat(File* fp)
{
	PxF32 data;
	size_t numRead = fread(&data, 1, 4, fp);
	if (numRead != 4) { return 0; }
	if (gFlip)
		Flip(data);
	return data;
}

static PxTransform readTransform(File* fp, PxReal scale)
{
	PxTransform tr;
	tr.p.x = scale * readFloat(fp);
	tr.p.y = scale * readFloat(fp);
	tr.p.z = scale * readFloat(fp);

	tr.q.x = readFloat(fp);
	tr.q.y = readFloat(fp);
	tr.q.z = readFloat(fp);
	tr.q.w = readFloat(fp);

	PX_ASSERT(tr.isValid());

	return tr;
}

static void readVertexArray(File* fp, PxVec3* verts, PxU32 nbVerts)
{
	const size_t size = 4 * 3 * nbVerts;
	size_t numRead = fread(verts, 1, size, fp);
	if (numRead != size) { return; }

	if (gFlip)
	{
		for (PxU32 j = 0; j<nbVerts; j++)
		{
			Flip(verts[j].x);
			Flip(verts[j].y);
			Flip(verts[j].z);
		}
	}
}

static void readUVs(File* fp, PxReal* uvs, PxU32 nbVerts)
{
	const size_t size = 4 * 2 * nbVerts;
	size_t numRead = fread(uvs, 1, size, fp);
	if (numRead != size) { return; }

	if (gFlip)
	{
		for (PxU32 j = 0; j<nbVerts * 2; j++)
			Flip(uvs[j]);
	}
}

static void readVertices(File* fp, PxVec3* verts, PxU32 nbVerts, PxReal scale)
{
	readVertexArray(fp, verts, nbVerts);

	for (PxU32 j = 0; j<nbVerts; j++)
		verts[j] *= scale;
}

static void readNormals(File* fp, PxVec3* verts, PxU32 nbVerts)
{
	readVertexArray(fp, verts, nbVerts);
}

static void readVertexColors(File* fp, PxVec3* colors, PxU32 nbVerts)
{
	readVertexArray(fp, colors, nbVerts);
}

static void readName(File* fp, char* objectName)
{
	PxU32 offset = 0;
	char c;
	do
	{
		size_t numRead = fread(&c, 1, 1, fp);
		if (numRead != 1) { c = '\0'; }
		objectName[offset++] = c;
	} while (c);
	objectName[offset] = 0;
}


bool loadRAWfile(const char* filename, RAWImportCallback& cb, PxReal scale)
{
	File* fp = NULL;
	fopen_s(&fp, filename, "rb");
	if (!fp)
		return false;

	// General info
	const PxU32 tag = read32(fp);
	const PxU32 generalVersion = read32(fp);
	const PxU32 nbMaterials = read32(fp);
	const PxU32 nbTextures = read32(fp);
	const PxU32 nbMeshes = read32(fp);
	const PxU32 nbShapes = read32(fp);
	const PxU32 nbHelpers = read32(fp);

	(void)tag;
	(void)generalVersion;
	char objectName[512];

	// Textures
	for (PxU32 i = 0; i<nbTextures; i++)
	{
		RAWTexture data;

		readName(fp, objectName);
		data.mName = objectName;
		data.mTransform = PxTransform(PxIdentity);	// PT: texture transform not supported yet
		data.mID = read32(fp);

		PxU32* pixels = NULL;
		PxU8 r, g, b, a;
		if (read8(fp))
		{
			data.mWidth = read32(fp);
			data.mHeight = read32(fp);
			data.mHasAlpha = read8(fp) != 0;
			const PxU32 nbPixels = data.mWidth*data.mHeight;
			pixels = new PxU32[nbPixels];
			data.mPixels = pixels;
			for (PxU32 i = 0; i<nbPixels; i++)
			{
				r = read8(fp);
				g = read8(fp);
				b = read8(fp);

				if (data.mHasAlpha)
					a = read8(fp);
				else
					a = 0xff;
				
				//b, g, r, a
				pixels[i] = b << 24 | g << 16 | r << 8 | a;
				/*pixels[i].r = read8(fp);
				pixels[i].g = read8(fp);
				pixels[i].b = read8(fp);
				if (data.mHasAlpha)
					pixels[i].a = read8(fp);
				else
					pixels[i].a = 0xff;*/
			}
		}
		else
		{
			data.mWidth = 0;
			data.mHeight = 0;
			data.mHasAlpha = false;
			data.mPixels = NULL;
		}

		cb.newTexture(data);
		delete[] pixels;
	}

	// Materials
	for (PxU32 i = 0; i<nbMaterials; i++)
	{
		RAWMaterial data;
		data.mID = read32(fp);
		data.mDiffuseID = read32(fp);
		data.mOpacity = readFloat(fp);
		data.mDoubleSided = read32(fp) != 0;

		data.mAmbientColor.x = readFloat(fp);
		data.mAmbientColor.y = readFloat(fp);
		data.mAmbientColor.z = readFloat(fp);

		data.mDiffuseColor.x = readFloat(fp);
		data.mDiffuseColor.y = readFloat(fp);
		data.mDiffuseColor.z = readFloat(fp);

		data.mSpecularColor.x = readFloat(fp);
		data.mSpecularColor.y = readFloat(fp);
		data.mSpecularColor.z = readFloat(fp);

		cb.newMaterial(data);
	}

	// Meshes
	for (PxU32 i = 0; i<nbMeshes; i++)
	{
		RAWMesh data;

		readName(fp, objectName);
		data.mName = objectName;
		data.mTransform = readTransform(fp, scale);
		//
		data.mNbVerts = read32(fp);
		data.mNbFaces = read32(fp);
		const PxU32 hasVertexColors = read32(fp);
		const PxU32 hasUVs = read32(fp);
		data.mMaterialID = read32(fp);

		PxVec3* tmpVerts = new PxVec3[data.mNbVerts];
		PxVec3* tmpNormals = new PxVec3[data.mNbVerts];
		PxVec3* tmpColors = NULL;
		PxReal* tmpUVs = NULL;

		data.mVerts = tmpVerts;
		data.mVertexNormals = tmpNormals;
		data.mVertexColors = NULL;
		data.mUVs = NULL;

		readVertices(fp, tmpVerts, data.mNbVerts, scale);
		readNormals(fp, tmpNormals, data.mNbVerts);

		if (hasVertexColors)
		{
			tmpColors = new PxVec3[data.mNbVerts];
			data.mVertexColors = tmpColors;
			readVertexColors(fp, tmpColors, data.mNbVerts);
		}

		if (hasUVs)
		{
			tmpUVs = new PxReal[(sizeof(PxReal)*data.mNbVerts * 2)];
			data.mUVs = tmpUVs;
			readUVs(fp, tmpUVs, data.mNbVerts);
		}

		PxU32* tmpIndices = new PxU32[sizeof(PxU32)*data.mNbFaces * 3];
		data.mIndices = tmpIndices;
		const size_t size = 4 * 3 * data.mNbFaces;
		size_t numRead = fread(tmpIndices, 1, size, fp);
		if (numRead != size)
		{
			delete[] tmpIndices;
			delete[] tmpUVs;
			delete[] tmpColors;
			delete[] tmpNormals;
			delete[] tmpVerts;
			/*SAMPLE_FREE(tmpIndices);
			SAMPLE_FREE(tmpUVs);

			DELETEARRAY(tmpColors);
			DELETEARRAY(tmpNormals);
			DELETEARRAY(tmpVerts);*/
			return false;
		}
		if (gFlip)
		{
			for (PxU32 j = 0; j<data.mNbFaces * 3; j++)
			{
				Flip(tmpIndices[j]);
			}
		}

		cb.newMesh(data);

		delete[] tmpIndices;
		delete[] tmpUVs;
		delete[] tmpColors;
		delete[] tmpNormals;
		delete[] tmpVerts;

	/*	SAMPLE_FREE(tmpIndices);
		SAMPLE_FREE(tmpUVs);
		DELETEARRAY(tmpColors);
		DELETEARRAY(tmpNormals);
		DELETEARRAY(tmpVerts);*/
	}

	// Shapes
	for (PxU32 i = 0; i<nbShapes; i++)
	{
		RAWShape data;

		readName(fp, objectName);
		data.mName = objectName;
		data.mTransform = readTransform(fp, scale);
		//
		data.mNbVerts = read32(fp);
		PxVec3* tmp = new PxVec3[data.mNbVerts];
		data.mVerts = tmp;
		readVertices(fp, tmp, data.mNbVerts, scale);

		cb.newShape(data);

		delete[] tmp;
	}

	// Helpers
	for (PxU32 i = 0; i<nbHelpers; i++)
	{
		RAWHelper data;

		readName(fp, objectName);
		data.mName = objectName;
		data.mTransform = readTransform(fp, scale);
	}
	return true;
}
