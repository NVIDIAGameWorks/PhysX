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



#ifndef MATERIAL_LIST_H
#define MATERIAL_LIST_H

#include <map>
#include <string>
#include <vector>

namespace Samples
{

class MaterialList
{
public:
	MaterialList();
	~MaterialList();

	void clear();
	void addPath(const char* path);

	struct MaterialInfo
	{
		MaterialInfo();
		bool isLit;
		bool vshaderStatic;
		bool vshader1bone;
		bool vshader4bones;
		unsigned int fromPath;

		std::string diffuseTexture;
		std::string normalTexture;
	};

	struct TextureInfo
	{
		TextureInfo();
		unsigned int fromPath;
	};

	const MaterialInfo* containsMaterial(const char* materialName) const;
	const char* findClosest(const char* materialName) const;

	const TextureInfo* containsTexture(const char* textureName) const;

	void getFirstMaterial(std::string& name, MaterialInfo& info);
	bool getNextMaterial(std::string& name, MaterialInfo& info);

private:
	unsigned int addMaterial(const char* directory, const char* prefix, const char* materialName);
	unsigned int addTexture(const char* directory, const char* prefix, const char* textureName);

	std::vector<std::string> mPaths;

	typedef std::map<std::string, MaterialInfo> tMaterialNames;
	tMaterialNames mMaterialNames;
	tMaterialNames::const_iterator mMaterialIterator;

	typedef std::map<std::string, TextureInfo> tTextureNames;
	tTextureNames mTextureNames;
};

} // namespace Samples


#endif // MATERIAL_LIST_H
