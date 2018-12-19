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



#include "ApexAssetTracker.h"
#include "Apex.h"
#include "ApexSDKIntl.h"
#include "ApexResource.h"
#include "ApexSDKHelpers.h"
#include "AuthorableObjectIntl.h"

namespace nvidia
{
namespace apex
{

/******************************************************************************
 * ApexAssetTracker class
 * Intended to be a base class for asset classes that have named sub-assets.
 * - Allows their actors to easily get asset pointers
 * - Uses the NRP in an appropriate fashion
 *   - calls to checkResource(), createResource(), and getResource()
 *   - handles that tricky IOS asset double call mechanism
 * - Sub class must implement initializeAssetNameTable()
 *
 */
ApexAssetTracker::~ApexAssetTracker()
{
	/* Get the NRP */
	if (mSdk)
	{
		ResourceProviderIntl* nrp = mSdk->getInternalResourceProvider();

		/* release references to rendermesh assets */
		for (uint32_t j = 0 ; j < mNameIdList.size() ; j++)
		{
			AssetNameIDMapping *nameId = mNameIdList[j];
			if (nameId->resID != INVALID_RESOURCE_ID)
			{
				if ( nameId->isOpaqueMesh )
				{
					ResID opaqueMeshId = nrp->createResource(mSdk->getOpaqueMeshNameSpace(),nameId->assetName.c_str(),false);
					PX_ASSERT( opaqueMeshId != INVALID_RESOURCE_ID );
					if (nrp->checkResource(opaqueMeshId))
					{
						nrp->releaseResource(opaqueMeshId);
					}
					uint32_t refCount;
					bool found =nrp->findRefCount(RENDER_MESH_AUTHORING_TYPE_NAME,nameId->assetName.c_str(),refCount);
					PX_ASSERT(found);
					PX_UNUSED(found);
					if (nrp->checkResource(nameId->resID))
					{
						if ( refCount == 1 )
						{
							void *asset = nrp->getResource(nameId->resID);
							PX_ASSERT(asset);
							if ( asset )
							{
								Asset *apexAsset = (Asset *)asset;
								apexAsset->release();
							}
						}
						nrp->releaseResource(nameId->resID);
					}
				}
				else
				{
					if (nrp->checkResource(nameId->resID))
					{
						nrp->releaseResource(nameId->resID);
					}
				}
			}
			delete nameId;
		}
	}
}

bool ApexAssetTracker::addAssetName(const char* assetName, bool isOpaqueMesh)
{
	/* first see if the name is already here */
	for (uint32_t i = 0; i < mNameIdList.size(); i++)
	{
		if (mNameIdList[i]->assetName == assetName && mNameIdList[i]->isOpaqueMesh == isOpaqueMesh)
		{
			return false;
		}
	}

	/* now add it to the list */
	mNameIdList.pushBack(PX_NEW(AssetNameIDMapping)(assetName, isOpaqueMesh));

	return true;
}

bool ApexAssetTracker::addAssetName(const char* iosTypeName, const char* assetName)
{
	/* first see if the name is already here */
	for (uint32_t i = 0; i < mNameIdList.size(); i++)
	{
		if (mNameIdList[i]->assetName == assetName &&
		        mNameIdList[i]->iosAssetTypeName == iosTypeName)
		{
			return false;
		}
	}

	/* now add it to the list */
	mNameIdList.pushBack(PX_NEW(AssetNameIDMapping)(assetName, iosTypeName));

	return true;
}

void ApexAssetTracker::removeAllAssetNames()
{
	/* Get the NRP */
	ResourceProviderIntl* nrp = mSdk->getInternalResourceProvider();

	/* release references to rendermesh assets */
	for (uint32_t j = 0 ; j < mNameIdList.size() ; j++)
	{
		nrp->releaseResource(mNameIdList[j]->resID);
		delete mNameIdList[j];
	}
	mNameIdList.reset();
}

IosAsset* ApexAssetTracker::getIosAssetFromName(const char* iosTypeName, const char* assetName)
{
	/* This will actually call the NRP to force the asset to be loaded (if it isn't already loaded)
	 * loading the APS will cause the particle module to call setResource on the iosans
	 */
	void* asset = ApexAssetHelper::getAssetFromNameList(mSdk,
	              iosTypeName,
	              mNameIdList,
	              assetName);

	Asset* aa = static_cast<Asset*>(asset);
	return DYNAMIC_CAST(IosAsset*)(aa);
}

Asset* ApexAssetTracker::getAssetFromName(const char* assetName)
{
	/* handle the material namespace, which is different (not authorable) */
	ResID resID = INVALID_RESOURCE_ID;
	if (mAuthoringTypeName == "")
	{
		resID = mSdk->getMaterialNameSpace();
	}

	void* tmp = ApexAssetHelper::getAssetFromNameList(mSdk,
	            mAuthoringTypeName.c_str(),
	            mNameIdList,
	            assetName,
	            resID);

	return static_cast<Asset*>(tmp);
}

Asset* ApexAssetTracker::getMeshAssetFromName(const char* assetName, bool isOpaqueMesh)
{
	PX_UNUSED(isOpaqueMesh);
	/* handle the material namespace, which is different (not authorable) */
	ResID resID = INVALID_RESOURCE_ID;
	if (isOpaqueMesh)
	{
		resID = mSdk->getOpaqueMeshNameSpace();
	}
	else if (mAuthoringTypeName == "")
	{
		resID = mSdk->getMaterialNameSpace();
	}

	void* tmp = ApexAssetHelper::getAssetFromNameList(mSdk,
	            mAuthoringTypeName.c_str(),
	            mNameIdList,
	            assetName,
	            resID);

	return static_cast<Asset*>(tmp);
}

ResID ApexAssetTracker::getResourceIdFromName(const char* assetName, bool isOpaqueMesh)
{
	/* handle the material namespace, which is different (not authorable) */
	ResID assetPsId = INVALID_RESOURCE_ID;
	if (isOpaqueMesh)
	{
		assetPsId = mSdk->getOpaqueMeshNameSpace();
	}
	else if (mAuthoringTypeName == "")
	{
		assetPsId = mSdk->getMaterialNameSpace();
	}

	// find the index of the asset name in our list of name->resID maps
	uint32_t assetIdx = 0;
	for (assetIdx = 0; assetIdx < mNameIdList.size(); assetIdx++)
	{
		if (mNameIdList[assetIdx]->assetName == assetName)
		{
			break;
		}
	}
	// This can't ever happen
	PX_ASSERT(assetIdx < mNameIdList.size());
	if (assetIdx < mNameIdList.size())
	{
		ApexAssetHelper::getAssetFromName(mSdk,
		        mAuthoringTypeName.c_str(),
		        assetName,
		        mNameIdList[assetIdx]->resID,
		        assetPsId);

		return mNameIdList[assetIdx]->resID;
	}
	else
	{
		APEX_DEBUG_WARNING("Request for asset %s of type %s not registered in asset tracker's list", assetName, mAuthoringTypeName.c_str());
		return INVALID_RESOURCE_ID;
	}
}

uint32_t ApexAssetTracker::forceLoadAssets()
{
	if (mNameIdList.size() == 0)
	{
		return 0;
	}

	uint32_t assetLoadedCount = 0;

	/* handle the material namespace, which is different (not authorable) */
	ResID assetPsID = INVALID_RESOURCE_ID;
	if (mAuthoringTypeName == "")
	{
		assetPsID = mSdk->getMaterialNameSpace();
	}
	else
	{
		AuthorableObjectIntl* ao = mSdk->getAuthorableObject(mAuthoringTypeName.c_str());
		if (ao)
		{
			assetPsID = ao->getResID();
		}
		else
		{
			APEX_INTERNAL_ERROR("Unknown authorable type: %s, please load all required modules.", mAuthoringTypeName.c_str());
		}
	}

	for (uint32_t i = 0 ; i < mNameIdList.size() ; i++)
	{
		bool useIosNamespace = false;
		/* Check if we are using the special IOS namespace ID */
		if (!(mNameIdList[i]->iosAssetTypeName == ""))
		{
			AuthorableObjectIntl* ao = mSdk->getAuthorableObject(mNameIdList[i]->iosAssetTypeName.c_str());
			if (!ao)
			{
				APEX_INTERNAL_ERROR("Particles Module is not loaded, cannot create particle system asset.");
				return 0;
			}

			assetPsID = ao->getResID();

			useIosNamespace = true;
		}

		// check if the asset has loaded yet
		if (mNameIdList[i]->resID == INVALID_RESOURCE_ID)
		{
			// if not, go ahead and ask the NRP for them
			if (useIosNamespace)
			{
				getIosAssetFromName(mNameIdList[i]->iosAssetTypeName.c_str(), mNameIdList[i]->assetName.c_str());
			}
			else if (mNameIdList[i]->isOpaqueMesh)
			{
				getMeshAssetFromName(mNameIdList[i]->assetName.c_str(), true);
			}
			else
			{
				getAssetFromName(mNameIdList[i]->assetName.c_str());
			}
			assetLoadedCount++;
		}
	}

	return assetLoadedCount;
}

}
} // end namespace nvidia::apex

