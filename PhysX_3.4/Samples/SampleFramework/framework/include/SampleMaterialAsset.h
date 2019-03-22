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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.


#ifndef SAMPLE_MATERIAL_ASSET_H
#define SAMPLE_MATERIAL_ASSET_H

#include <SampleAsset.h>
#include <vector>

namespace FAST_XML
{
	class xml_node;
}

namespace SampleRenderer
{
	class RendererMaterial;
	class RendererMaterialInstance;
}

namespace SampleFramework
{
	class SampleAssetManager;

	class SampleMaterialAsset : public SampleAsset
	{
		friend class SampleAssetManager;
	protected:
		SampleMaterialAsset(SampleAssetManager &assetManager, FAST_XML::xml_node &xmlroot, const char *path);
		SampleMaterialAsset(SampleAssetManager &assetManager, Type type, const char *path);
		virtual ~SampleMaterialAsset(void);

	public:
		size_t                                    getNumVertexShaders() const;
		SampleRenderer::RendererMaterial         *getMaterial(size_t vertexShaderIndex = 0);
		SampleRenderer::RendererMaterialInstance *getMaterialInstance(size_t vertexShaderIndex = 0);
		unsigned int                              getMaxBones(size_t vertexShaderIndex) const;

	public:
		virtual bool isOk(void) const;

	protected:
		SampleAssetManager       &m_assetManager;
		struct MaterialStruct
		{
			SampleRenderer::RendererMaterial         *m_material;
			SampleRenderer::RendererMaterialInstance *m_materialInstance;
			unsigned int                              m_maxBones;
		};
		std::vector<MaterialStruct> m_vertexShaders;
		std::vector<SampleAsset*> m_assets;
	};

} // namespace SampleFramework

#endif
