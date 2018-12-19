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



#ifndef AUTHORABLE_OBJECT_INTL_H
#define AUTHORABLE_OBJECT_INTL_H

#include "ApexString.h"
#include "ApexSDKIntl.h"
#include "ApexSDKHelpers.h"
#include "ResourceProviderIntl.h"
#include "ApexResource.h"

class ResourceList;

namespace NvParameterized
{
class Interface;
};

namespace nvidia
{
namespace apex
{

// This class currently contains implementation, this will be removed and put in APEXAuthorableObject
class AuthorableObjectIntl : public ApexResourceInterface, public ApexResource
{
public:

	AuthorableObjectIntl(ModuleIntl* m, ResourceList& list, const char* aoTypeName)
		:	mAOTypeName(aoTypeName),
		    mModule(m)
	{
		list.add(*this);
	}

	virtual Asset* 			createAsset(AssetAuthoring& author, const char* name) = 0;
	virtual Asset* 			createAsset(NvParameterized::Interface* params, const char* name) = 0;
	virtual void					releaseAsset(Asset& nxasset) = 0;

	virtual AssetAuthoring* 	createAssetAuthoring() = 0;
	virtual AssetAuthoring* 	createAssetAuthoring(const char* name) = 0;
	virtual AssetAuthoring* 	createAssetAuthoring(NvParameterized::Interface* params, const char* name) = 0;
	virtual void					releaseAssetAuthoring(AssetAuthoring& nxauthor) = 0;

	virtual uint32_t					forceLoadAssets() = 0;
	virtual uint32_t					getAssetCount() = 0;
	virtual bool					getAssetList(Asset** outAssets, uint32_t& outAssetCount, uint32_t inAssetCount) = 0;


	virtual ResID					getResID() = 0;
	virtual ApexSimpleString&		getName() = 0;

	// ApexResourceInterface methods
	virtual void					release() = 0;
	virtual void					destroy() = 0;

	// ApexResourceInterface methods
	uint32_t							getListIndex() const
	{
		return m_listIndex;
	}

	void							setListIndex(ResourceList& list, uint32_t index)
	{
		m_listIndex = index;
		m_list = &list;
	}

	ResID				mAOResID;
	ResID				mAOPtrResID;
	ApexSimpleString	mAOTypeName;
	ApexSimpleString	mParameterizedName;

	ResourceList		mAssets;
	ResourceList		mAssetAuthors;

	ModuleIntl* 			mModule;
};

}
} // end namespace nvidia::apex

#endif	// AUTHORABLE_OBJECT_INTL_H
