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

#include "SampleInputAsset.h"
#include "ODBlock.h"
#include <PxTkFile.h>
#include <PsString.h>

using namespace SampleFramework;

SampleInputAsset::SampleInputAsset(File* file, const char *path)
: SampleAsset(SampleAsset::ASSET_INPUT, path)
, m_SettingsBlock(NULL)
, m_File(file)
{
	m_SampleInputData.reserve(128);	

	PX_ASSERT(file);

	if (!m_Mapping.loadScript(file))
	{
		shdfnd::printFormatted("ODS parse error: %s in file: %s\n", m_Mapping.lastError, path);		
		PX_ASSERT(0);
	}	

	m_SettingsBlock = m_Mapping.getBlock("InputMapping");
	if (!m_SettingsBlock)
	{
		shdfnd::printFormatted("No \"InputEventSettings\" block found!\n");
		PX_ASSERT(0);
	}
	else
	{
		LoadData(m_SettingsBlock);
	}
}

SampleInputAsset::~SampleInputAsset()
{
	if(m_File)
	{
		fclose(m_File);
	}
}

void SampleInputAsset::LoadData(ODBlock* odsSettings)
{	
	odsSettings->reset();
	while (odsSettings->moreSubBlocks())
	{
		ODBlock* subBlock = odsSettings->nextSubBlock();
		subBlock->reset();

		SampleInputData inputData;
		if (!strcmp(subBlock->ident(), "map"))
		{
			if (subBlock->moreTerminals())	
			{ 
				const char* p = subBlock->nextTerminal();
				strcpy(inputData.m_UserInputName, p);
			}
			if (subBlock->moreTerminals())	
			{ 
				const char* p = subBlock->nextTerminal();
				strcpy(inputData.m_InputEventName, p);
			}

			m_SampleInputData.push_back(inputData);
		}
	}
}

bool SampleInputAsset::isOk(void) const
{
	return true;
}
