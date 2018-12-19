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



#include "MaterialList.h"

#include "PxAssert.h"

#if PX_WINDOWS_FAMILY
#define NOMINMAX
#include <windows.h>
#endif

#include "PsFastXml.h"
#include "PsFileBuffer.h"
#include "PsString.h"

#include "PxInputDataFromPxFileBuf.h"

namespace Samples
{

MaterialList::MaterialList()
{
}


MaterialList::~MaterialList()
{
}



void MaterialList::clear()
{
	mMaterialNames.clear();
	mTextureNames.clear();

	mPaths.clear();
}



void MaterialList::addPath(const char* path)
{
	PX_UNUSED(path);

#if PX_WINDOWS_FAMILY
	const char* matPrefixes[2] = { "", "materials/" };

	PX_ASSERT(strlen(path) < 240);
	char fileMask[256];

	unsigned int materialsAdded = 0;

	for (unsigned int pass = 0; pass < 2; pass++)
	{
		physx::shdfnd::snprintf(fileMask, 255, "%s/%s*.xml", path, matPrefixes[(unsigned int)pass]);

		WIN32_FIND_DATA ffd;
		HANDLE hFind = ::FindFirstFile(fileMask, &ffd);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				materialsAdded += addMaterial(path, matPrefixes[pass], ffd.cFileName);
			}
			while (FindNextFile(hFind, &ffd) != 0);

			FindClose(hFind);
		}
	}


	const char* texPrefixes[2] = { "", "textures/" };
	const char* texSuffixes[2] = { "dds", "tga" };

	unsigned int texturesAdded = 0;

	for (unsigned int prefixes = 0; prefixes < 2; prefixes++)
	{
		for (unsigned int suffixes = 0; suffixes < 2; suffixes++)
		{
			physx::shdfnd::snprintf(fileMask, 255, "%s/%s*.%s", path, texPrefixes[prefixes], texSuffixes[suffixes]);

			WIN32_FIND_DATA ffd;
			HANDLE hFind = ::FindFirstFile(fileMask, &ffd);

			if (hFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					texturesAdded += addTexture(path, texPrefixes[prefixes], ffd.cFileName);
				}
				while (FindNextFile(hFind, &ffd) != 0);

				FindClose(hFind);
			}
		}
	}
	
	if (materialsAdded > 0 || texturesAdded > 0)
	{
		mPaths.push_back(path);
	}

#if 0
	// verification step
	for (tMaterialNames::const_iterator it = mMaterialNames.begin(); it != mMaterialNames.end(); ++it)
	{
		if (!it->second.diffuseTexture.empty())
		{
			tTextureNames::const_iterator tex = mTextureNames.find(it->second.diffuseTexture);
			if (tex == mTextureNames.end())
			{
				PX_ASSERT(!"Texture not found");
			}
			else
			{
				PX_ASSERT(tex->second.fromPath <= it->second.fromPath);
			}
		}

		if (!it->second.normalTexture.empty())
		{
			tTextureNames::const_iterator tex = mTextureNames.find(it->second.normalTexture);
			if (tex == mTextureNames.end())
			{
				PX_ASSERT(!"Texture not found");
			}
			else
			{
				PX_ASSERT(tex->second.fromPath <= it->second.fromPath);
			}
		}
	}
#endif

#endif
}



const MaterialList::MaterialInfo* MaterialList::containsMaterial(const char* materialName) const
{
	tMaterialNames::const_iterator it = mMaterialNames.find(materialName);

	if (it != mMaterialNames.end())
	{
		return &it->second;
	}

	return NULL;
}



const char* MaterialList::findClosest(const char* materialName) const
{
	for (std::map<std::string, MaterialInfo>::const_iterator it = mMaterialNames.begin(); it != mMaterialNames.end(); ++it)
	{
		if (it->first.find(materialName) != std::string::npos)
		{
			return it->first.c_str();
		}
	}

	//return "materials/debug_texture.xml";
	return "materials/simple_lit.xml";
}


	
const MaterialList::TextureInfo* MaterialList::containsTexture(const char* textureName) const
{
	tTextureNames::const_iterator it = mTextureNames.find(textureName);

	if (it != mTextureNames.end())
	{
		return &it->second;
	}

	return NULL;
}




MaterialList::MaterialInfo::MaterialInfo() :
	isLit(false),
	vshaderStatic(false),
	vshader1bone(false),
	vshader4bones(false),
	fromPath(0xffffffff)
{
}



MaterialList::TextureInfo::TextureInfo() :
	fromPath(0xffffffff)
{
}


void MaterialList::getFirstMaterial(std::string& name, MaterialInfo& info)
{
	mMaterialIterator = mMaterialNames.begin();

	name = mMaterialIterator->first;
	info = mMaterialIterator->second;
}



bool MaterialList::getNextMaterial(std::string& name, MaterialInfo& info)
{
	++mMaterialIterator;
	if (mMaterialIterator != mMaterialNames.end())
	{
		name = mMaterialIterator->first;
		info = mMaterialIterator->second;

		return true;
	}

	return false;
}



class XmlParser : public physx::shdfnd::FastXml::Callback
{
public:
	XmlParser(unsigned int fromPath) : mIsMaterial(false)
	{
		mMaterialInfo.fromPath = fromPath;
	}

	virtual ~XmlParser() {}

	const MaterialList::MaterialInfo& getMaterialInfo() { return mMaterialInfo; }

	bool isMaterial() { return mIsMaterial; }

protected:

	// encountered a comment in the XML
	virtual bool processComment(const char * /*comment*/)
	{
		return true;
	}

	// 'element' is the name of the element that is being closed.
	// depth is the recursion depth of this element.
	// Return true to continue processing the XML file.
	// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
	// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
	virtual bool processClose(const char * /*element*/, unsigned int /*depth*/, bool & /*isError*/)
	{
		return true;
	}

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(
		const char *elementName,   // name of the element
		const char  *elementData,  // element data, null if none
		const physx::shdfnd::FastXml::AttributePairs& attr,
		int /*lineno*/)  // line number in the source XML file
	{
		if (!mIsMaterial && ::strcmp(elementName, "material") != 0)
		{
			// not a material file
			return false;
		}

		if (::strcmp(elementName, "material") == 0)
		{
			for (int i = 0; i < attr.getNbAttr(); i++)
			{
				if (::strcmp(attr.getKey(i), "type") == 0)
				{
					mMaterialInfo.isLit = ::strcmp(attr.getValue(i), "lit") == 0;
				}
			}
			mIsMaterial = true;
		}
		else if (::strcmp(elementName, "shader") == 0)
		{
			int nameIndex = -1;
			for (int i = 0; i < attr.getNbAttr(); i++)
			{
				if (::strcmp(attr.getKey(i), "name") == 0)
				{
					nameIndex = i;
					break;
				}
			}

			if (::strcmp(attr.getValue(nameIndex), "vertex") == 0)
			{
				// identify the three
				mMaterialInfo.vshaderStatic |= strstr(elementData, "staticmesh") != NULL;
				mMaterialInfo.vshader1bone |= strstr(elementData, "skeletalmesh_1bone") != NULL;
				mMaterialInfo.vshader4bones |= strstr(elementData, "skeletalmesh_4bone") != NULL;
			}
		}
		else if (::strcmp(elementName, "sampler2D") == 0)
		{
			int nameIndex = -1;
			for (int i = 0; i < attr.getNbAttr(); i += 2)
			{
				if (::strcmp(attr.getKey(i), "name") == 0)
				{
					nameIndex = i;
					break;
				}
			}

			if (::strcmp(attr.getValue(nameIndex), "diffuseTexture") == 0)
			{
				mMaterialInfo.diffuseTexture = elementData;
			}
			else if (::strcmp(attr.getValue(nameIndex), "normalTexture") == 0)
			{
				mMaterialInfo.normalTexture = elementData;
			}
		}

		return true;
	}

	virtual void* fastxml_malloc(unsigned int size)
	{
		return ::malloc(size);
	}

	virtual void fastxml_free(void *mem)
	{
		::free(mem);
	}

private:
	bool mIsMaterial;
	MaterialList::MaterialInfo mMaterialInfo;
};



unsigned int MaterialList::addMaterial(const char* directory, const char* prefix, const char* materialName)
{
	char filename[256];
	physx::shdfnd::snprintf(filename, 256, "%s/%s%s", directory, prefix, materialName);

	XmlParser parser((unsigned int)mPaths.size());
	physx::shdfnd::FastXml* fastXml = physx::shdfnd::createFastXml(&parser);
	if (fastXml != NULL)
	{
		physx::PsFileBuffer fileBuffer(filename, physx::PxFileBuf::OPEN_READ_ONLY);
		physx::PxInputDataFromPxFileBuf id(fileBuffer);
		fastXml->processXml(id);

		int errorLineNumber = 0;
		const char* xmlError = fastXml->getError(errorLineNumber);
		
		if (xmlError != NULL)
		{
			if (strncmp(xmlError, "User aborted", 12) == 0)
			{

			}
			else
			{
#if PX_WINDOWS_FAMILY
				char error[512];
				sprintf_s(error, 512, "%s\n%s", filename, xmlError);
				::MessageBox(NULL, error, "MaterialList XML Parse error", MB_OK);
#endif
			}
		}

		fastXml->release();
	}

	if (parser.isMaterial())
	{
		char saneFileName[128];
		physx::shdfnd::snprintf(saneFileName, 128, "%s%s", prefix, materialName);

		if (mMaterialNames.find(saneFileName) != mMaterialNames.end())
		{
			PX_ASSERT(!"Duplicated material name found");
		}
		else
		{
			mMaterialNames[saneFileName] = parser.getMaterialInfo();
			return 1;
		}
	}

	return 0;
}



unsigned int MaterialList::addTexture(const char* /*directory*/, const char* prefix, const char* textureName)
{
	char saneFileName[128];
	physx::shdfnd::snprintf(saneFileName, 128, "%s%s", prefix, textureName);

	if (mTextureNames.find(saneFileName) != mTextureNames.end())
	{
		PX_ASSERT(!"duplicated texture found!");
	}
	else
	{
		TextureInfo info;
		info.fromPath = (unsigned int)mPaths.size();
		mTextureNames[saneFileName] = info;
		return 1;
	}

	return 0;
}



} // namespace Samples
