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
#include <RendererMaterialInstance.h>
#include <RendererMaterial.h>
#include <foundation/PxAssert.h>

using namespace SampleRenderer;

RendererMaterialInstance::RendererMaterialInstance(RendererMaterial &material) :
m_material(material)
{
	m_data = 0;
	PxU32 dataSize = m_material.getMaterialInstanceDataSize();
	if(dataSize > 0)
	{
		m_data = new PxU8[dataSize];
		memset(m_data, 0, dataSize);
	}
}

RendererMaterialInstance::RendererMaterialInstance(const RendererMaterialInstance& other) :
m_material(other.m_material)
{
	PxU32 dataSize = m_material.getMaterialInstanceDataSize();
	if (dataSize > 0)
	{
		m_data = new PxU8[dataSize];
		memcpy(m_data, other.m_data, dataSize);
	}
}

RendererMaterialInstance::~RendererMaterialInstance(void)
{
	if(m_data) delete [] m_data;
}

void RendererMaterialInstance::writeData(const RendererMaterial::Variable &var, const void *data)
{
	if(m_data && data)
	{
		memcpy(m_data+var.getDataOffset(), data, var.getDataSize());
	}
}

RendererMaterialInstance &RendererMaterialInstance::operator=(const RendererMaterialInstance &b)
{
	PX_ASSERT(&m_material == &b.m_material);
	if(&m_material == &b.m_material)
	{
		memcpy(m_data, b.m_data, m_material.getMaterialInstanceDataSize());
	}
	return *this;
}
