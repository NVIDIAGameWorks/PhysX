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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PhysXSample.h"
#include "SampleInputMappingAsset.h"
#include "ODBlock.h"
#include "PxTkFile.h"

using namespace SampleFramework;

static const int MAPPING_VERSION = 4;

SampleInputMappingAsset::SampleInputMappingAsset(SampleFramework::File* file, const char *path, bool empty,PxU32 userInputCS, PxU32 inputEventCS)
: mSettingsBlock(NULL), mMapping(NULL) , mFile(file) , mPath(path), mUserInputCS(userInputCS), mInputEventCS(inputEventCS)
{
	mSampleInputData.reserve(128);	

	PX_ASSERT(file);

	mIsOk = true;
	if(empty)
	{		
		createNewFile(false);
	}
	else
	{
		mMapping = new ODBlock();
		if (!mMapping->loadScript(file))
		{
			shdfnd::printFormatted("ODS parse error: %s in file: %s\n", mMapping->lastError, path);		
			mIsOk = false;
			createNewFile(true);
		}
		else
		{
			mSettingsBlock = mMapping->getBlock("InputMapping");
			if (!mSettingsBlock)
			{
				shdfnd::printFormatted("No \"InputEventSettings\" block found!\n");
				mIsOk = false;
				createNewFile(true);
			}
			else
			{
				int versionNumber = 0;
				if(!mSettingsBlock->getBlockInt("Version",&versionNumber))
				{
					mIsOk = false;
					createNewFile(true);
				}
				else
				{					
					if(versionNumber != MAPPING_VERSION)
					{
						mIsOk = false;
						createNewFile(true);
					}
					else
					{
						if(!checksumCheck())
						{
							mIsOk = false;
							createNewFile(true);
						}
						else
						{
							loadData(mSettingsBlock);
						}
					}
				}
			}
		}
	}
}

bool SampleInputMappingAsset::checksumCheck()
{
	PxU32 userInputCS = 0;
	if(!mSettingsBlock->getBlockU32("UserInputCS",&userInputCS))
	{
		return false;
	}

	PxU32 inputEventCS = 0;
	if(!mSettingsBlock->getBlockU32("InputEventCS",&inputEventCS))
	{
		return false;
	}

	if(mUserInputCS != userInputCS)
	{
		return false;
	}

	if(mInputEventCS != inputEventCS)
	{
		return false;
	}

	return true;
}

SampleInputMappingAsset::~SampleInputMappingAsset()
{
	if(mFile)
	{
		fclose(mFile);
	}

	if(mMapping)
		delete mMapping;
}

void SampleInputMappingAsset::loadData(ODBlock* odsSettings)
{	
	odsSettings->reset();
	while (odsSettings->moreSubBlocks())
	{
		ODBlock* subBlock = odsSettings->nextSubBlock();
		subBlock->reset();

		SampleInputData inputData;
		if (!strcmp(subBlock->ident(), "Map"))
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

			mSampleInputData.push_back(inputData);
		}
	}
}

void SampleInputMappingAsset::addMapping(const char* uiName, const char* ieName)
{		
	if(!mIsOk)
		return;

	ODBlock & mapping = *new ODBlock();
	mapping.ident("Map");
	mSettingsBlock->addStatement(mapping);

	ODBlock & userInputBlock = *new ODBlock(); 
	mapping.addStatement(userInputBlock);
	userInputBlock.ident(uiName);

	ODBlock & inputEventBlock = *new ODBlock(); 
	mapping.addStatement(inputEventBlock);
	inputEventBlock.ident(ieName);
}

bool SampleInputMappingAsset::createNewFile(bool rewriteFile)
{
	if(rewriteFile)
	{
		if(mFile)
		{
			fclose(mFile);
			mFile = NULL;
		}
		PxToolkit::fopen_s(&mFile,mPath , "w");

		if(mFile)
			mIsOk = true;
	}

	if(mMapping)
	{
		delete mMapping;
		mMapping = NULL;
	}

	mMapping = new ODBlock();
	mMapping->ident("InputMapping");
	mSettingsBlock = mMapping;

	ODBlock & version = *new ODBlock();
	version.ident("Version");
	mSettingsBlock->addStatement(version);

	ODBlock & nb = *new ODBlock(); 
	version.addStatement(nb);
	char temps[64];
	sprintf(temps,"%d",MAPPING_VERSION);
	nb.ident(temps);

	ODBlock & userInputCSB = *new ODBlock();
	userInputCSB.ident("UserInputCS");
	mSettingsBlock->addStatement(userInputCSB);

	ODBlock &nb2 = *new ODBlock(); 
	userInputCSB.addStatement(nb2);	
	sprintf(temps,"%d",mUserInputCS);
	nb2.ident(temps);

	ODBlock & inputEventCSB = *new ODBlock();
	inputEventCSB.ident("InputEventCS");
	mSettingsBlock->addStatement(inputEventCSB);

	ODBlock &nb3 = *new ODBlock(); 
	inputEventCSB.addStatement(nb3);	
	sprintf(temps,"%d",mInputEventCS);
	nb3.ident(temps);

	return true;
}

void SampleInputMappingAsset::saveMappings()
{	
	if(!mIsOk)
		return;

	if(mFile)
	{
		fclose(mFile);
		mFile = NULL;
	}
	PxToolkit::fopen_s(&mFile,mPath , "w");
	if(mFile)
	{
		mMapping->saveScript(mFile,false);
		fclose(mFile);	
	}

	mFile = NULL;
}

bool SampleInputMappingAsset::isOk(void) const
{
	return mIsOk;
}
