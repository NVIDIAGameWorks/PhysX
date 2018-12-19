// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.


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
