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



#ifndef __APEXAUTHORABLEOBJECT_H__
#define __APEXAUTHORABLEOBJECT_H__

#include "Asset.h"

#include "AuthorableObjectIntl.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "nvparameterized/NvParameterized.h"

namespace nvidia
{
namespace apex
{

class ResourceList;

/**
 *	ApexAuthorableObject
 *	This class is the implementation for AuthorableObjectIntl (except for the
 *	ApexResource stuff).  It's disappointing that it has to be templated like
 *	this, but there were issues with multiple inheritance (an Resource ptr
 *	cannot be cast to an Asset ptr - Asset should inherit from Resource
 *	in the future.
 *
 *	Template expectations:
 *	T_Module -	must inherit from ModuleIntl
 *				the T_Asset type typically uses T_Module->mSdk
 *
 *	T_Asset -	T_Asset( T_Module *m, ResourceList &list, const char *name )
 *				must inherit from Asset
 *
 *	T_AssetAuthoring -	T_AssetAuthoring( T_Module *m, ResourceList &list )
 *						must inherit from AssetAuthoring
 */

template <class T_Module, class T_Asset, class T_AssetAuthoring>
class ApexAuthorableObject : public AuthorableObjectIntl
{
public:
	ApexAuthorableObject(ModuleIntl* m, ResourceList& list)
		:	AuthorableObjectIntl(m, list, T_Asset::getClassName())
	{
		// Register the authorable object type name in the NRP
		mAOResID = GetInternalApexSDK()->getInternalResourceProvider()->createNameSpace(mAOTypeName.c_str());
		mAOPtrResID = GetInternalApexSDK()->registerAuthObjType(mAOTypeName.c_str(), this);

		PX_ASSERT(!"This constructor is no longer valid, you MUST provide a parameterizedName!");
	}

	// This constructor is for assets that are based on NvParameterized, they provide the string
	// defined in the NvParameterized structure.  This will be used to map the NvParameterized object
	// to the AuthorableObject class to create the assets after they are deserialized
	ApexAuthorableObject(ModuleIntl* m, ResourceList& list, const char* parameterizedName)
		:	AuthorableObjectIntl(m, list, T_Asset::getClassName())
	{
		mParameterizedName = parameterizedName;

		// Register the authorable object type name in the NRP
		mAOResID = GetInternalApexSDK()->getInternalResourceProvider()->createNameSpace(mAOTypeName.c_str());
		mAOPtrResID = GetInternalApexSDK()->registerAuthObjType(mAOTypeName.c_str(), this);

		// Register the parameterized name in the NRP to point to this authorable object
		GetInternalApexSDK()->registerNvParamAuthType(mParameterizedName.c_str(), this);
	}

	virtual Asset* 			createAsset(AssetAuthoring& author, const char* name);
	virtual Asset* 			createAsset(NvParameterized::Interface* params, const char* name);
	virtual void					releaseAsset(Asset& nxasset);

	virtual AssetAuthoring* 	createAssetAuthoring();
	virtual AssetAuthoring* 	createAssetAuthoring(const char* name);
	virtual AssetAuthoring* 	createAssetAuthoring(NvParameterized::Interface* params, const char* name);
	virtual void					releaseAssetAuthoring(AssetAuthoring& nxauthor);

	virtual uint32_t					forceLoadAssets();

	virtual uint32_t					getAssetCount()
	{
		return mAssets.getSize();
	}
	virtual bool					getAssetList(Asset** outAssets, uint32_t& outAssetCount, uint32_t inAssetCount);

	virtual ResID					getResID()
	{
		return mAOResID;
	}

	virtual ApexSimpleString&		getName()
	{
		return mAOTypeName;
	}

	// Resource methods
	virtual void					release();

	virtual void					destroy()
	{
		delete this;
	}

};

template <class T_Module, class T_Asset, class T_AssetAuthoring>
Asset* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAsset(AssetAuthoring& author, const char* name)
{
	if (mParameterizedName.len())
	{
		NvParameterized::Interface* params = 0;
		NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
		params = traits->createNvParameterized(mParameterizedName.c_str());
		PX_ASSERT(params);
		if (params)
		{
			NvParameterized::Interface* authorParams = author.getNvParameterized();
			PX_ASSERT(authorParams);
			if (authorParams)
			{
				if (NvParameterized::ERROR_NONE != authorParams->callPreSerializeCallback())
				{
					return NULL;
				}

				NvParameterized::ErrorType err = params->copy(*authorParams);

				PX_ASSERT(err == NvParameterized::ERROR_NONE);

				if (err == NvParameterized::ERROR_NONE)
				{
					return createAsset(params, name);
				}
				else
				{
					return NULL;
				}
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		APEX_INVALID_OPERATION("Authorable Asset needs a parameterized name");
		return NULL;
	}
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
void ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
release()
{
	// test this by releasing the module before the individual assets

	// remove all assets that we loaded (must do now else we cannot unregister)
	mAssets.clear();
	mAssetAuthors.clear();

	// remove this AO's name from the authorable namespace
	GetInternalApexSDK()->unregisterAuthObjType(mAOTypeName.c_str());

	if (mParameterizedName.len())
	{
		GetInternalApexSDK()->unregisterNvParamAuthType(mParameterizedName.c_str());
	}
	destroy();
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
Asset* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAsset(NvParameterized::Interface* params, const char* name)
{
	T_Asset* asset = PX_NEW(T_Asset)(DYNAMIC_CAST(T_Module*)(mModule), mAssets, params, name);
	if (asset)
	{
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, asset);
	}
	return asset;
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
void ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
releaseAsset(Asset& nxasset)
{
	T_Asset* asset = DYNAMIC_CAST(T_Asset*)(&nxasset);

	GetInternalApexSDK()->getInternalResourceProvider()->setResource(mAOTypeName.c_str(), nxasset.getName(), NULL, false, false);
	asset->destroy();
}

#ifdef WITHOUT_APEX_AUTHORING

// this should no longer be called now that we're auto-assigning names in createAssetAuthoring()
template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring()
{
	APEX_INVALID_OPERATION("Asset authoring has been disabled");
	return NULL;
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring(const char*)
{
	APEX_INVALID_OPERATION("Asset authoring has been disabled");
	return NULL;
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring(NvParameterized::Interface*, const char*)
{
	APEX_INVALID_OPERATION("Asset authoring has been disabled");
	return NULL;
}

#else // WITHOUT_APEX_AUTHORING

// this should no longer be called now that we're auto-assigning names in createAssetAuthoring()
template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring()
{
	return PX_NEW(T_AssetAuthoring)(DYNAMIC_CAST(T_Module*)(mModule), mAssetAuthors);
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring(const char* name)
{
	T_AssetAuthoring* assetAuthor = PX_NEW(T_AssetAuthoring)(DYNAMIC_CAST(T_Module*)(mModule), mAssetAuthors, name);

	if (assetAuthor)
	{
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, assetAuthor);
	}
	return assetAuthor;
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
AssetAuthoring* ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
createAssetAuthoring(NvParameterized::Interface* params, const char* name)
{
	T_AssetAuthoring* assetAuthor = PX_NEW(T_AssetAuthoring)(DYNAMIC_CAST(T_Module*)(mModule), mAssetAuthors, params, name);

	if (assetAuthor)
	{
		GetInternalApexSDK()->getNamedResourceProvider()->setResource(mAOTypeName.c_str(), name, assetAuthor);
	}
	return assetAuthor;
}

#endif // WITHOUT_APEX_AUTHORING

template <class T_Module, class T_Asset, class T_AssetAuthoring>
void ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
releaseAssetAuthoring(AssetAuthoring& nxauthor)
{
	T_AssetAuthoring* author = DYNAMIC_CAST(T_AssetAuthoring*)(&nxauthor);

	GetInternalApexSDK()->getInternalResourceProvider()->setResource(mAOTypeName.c_str(), nxauthor.getName(), NULL, false, false);
	author->destroy();
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
uint32_t ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
forceLoadAssets()
{
	uint32_t loadedAssetCount = 0;

	for (uint32_t i = 0; i < mAssets.getSize(); i++)
	{
		T_Asset* asset = DYNAMIC_CAST(T_Asset*)(mAssets.getResource(i));
		loadedAssetCount += asset->forceLoadAssets();
	}
	return loadedAssetCount;
}

template <class T_Module, class T_Asset, class T_AssetAuthoring>
bool ApexAuthorableObject<T_Module, T_Asset, T_AssetAuthoring>::
getAssetList(Asset** outAssets, uint32_t& outAssetCount, uint32_t inAssetCount)
{
	PX_ASSERT(outAssets);
	PX_ASSERT(inAssetCount >= mAssets.getSize());

	if (!outAssets || inAssetCount < mAssets.getSize())
	{
		outAssetCount = 0;
		return false;
	}

	outAssetCount = mAssets.getSize();
	for (uint32_t i = 0; i < mAssets.getSize(); i++)
	{
		T_Asset* asset = DYNAMIC_CAST(T_Asset*)(mAssets.getResource(i));
		outAssets[i] = static_cast<Asset*>(asset);
	}

	return true;
}

}
} // end namespace nvidia::apex

#endif	// __APEXAUTHORABLEOBJECT_H__
