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
