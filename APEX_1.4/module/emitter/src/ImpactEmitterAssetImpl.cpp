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



#include "Apex.h"
#include "IofxAsset.h"

#include "ImpactEmitterAssetImpl.h"
#include "ImpactEmitterActorImpl.h"
#include "PsMemoryBuffer.h"

#include "nvparameterized/NvParamUtils.h"
#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace emitter
{



void ImpactEmitterAssetImpl::initializeAssetNameTable()
{
	/* initialize the exlosion, iofx, and ios asset names to resID tables */
	NvParameterized::Handle eventSetHandle(*mParams);
	int numSets;

	mParams->getParameterHandle("eventSetList", eventSetHandle);
	PX_ASSERT(eventSetHandle.isValid());

	mParams->getArraySize(eventSetHandle, numSets);
	for (int i = 0; i < numSets; i++)
	{
		NvParameterized::Handle ih(*mParams);
		NvParameterized::Interface* eventPtr = 0;

		eventSetHandle.getChildHandle(i, ih);
		PX_ASSERT(ih.isValid());

		mParams->getParamRef(ih, eventPtr);
		PX_ASSERT(eventPtr);

		ApexSimpleString tmpClassName(eventPtr->className());


		if (tmpClassName == "ImpactExplosionEvent")
		{
			APEX_DEBUG_WARNING("Invalid asset. ImpactExplosionEvent is not supported under PhysX 3.");
			PX_ALWAYS_ASSERT();
		}
		else if (tmpClassName == "ImpactObjectEvent")
		{
			ImpactObjectEvent* paramPtr = (ImpactObjectEvent*)eventPtr;

			mIofxAssetTracker.addAssetName(paramPtr->parameters().iofxAssetName->name(), false);
			mIosAssetTracker.addAssetName(paramPtr->parameters().iosAssetName->className(),
			                              paramPtr->parameters().iosAssetName->name());
		}
	}
}

void ImpactEmitterAssetImpl::preSerialize(void* userData_)
{
	PX_UNUSED(userData_);
}

void ImpactEmitterAssetImpl::postDeserialize(void* userData_)
{
	PX_UNUSED(userData_);
	initializeAssetNameTable();
	buildEventNameIndexMap();
}

void ImpactEmitterAssetImpl::buildEventNameIndexMap()
{
	// destroy old mappings
	for (uint32_t j = 0; j < mEventNameIndexMaps.size(); j++)
	{
		if (mEventNameIndexMaps[j])
		{
			delete mEventNameIndexMaps[j];
		}
	}

	// loop through all event sets, build a mapping for each new name found
	// and store all indices for that event set
	ParamArray<NvParameterized::Interface*> assetEventSets(mParams,
	        "eventSetList",
	        (ParamDynamicArrayStruct*) & (mParams->eventSetList));

	for (uint32_t i = 0; i < assetEventSets.size(); i++)
	{
		NvParameterized::Handle hEventSetName(*assetEventSets[i]);
		const char* paramEventSetName = 0;

		NvParameterized::getParamString(*(assetEventSets[i]), "eventSetName", paramEventSetName);
		if (!paramEventSetName)
		{
			APEX_DEBUG_WARNING("Invalid eventSetName from Impact Emitter event sets");
			continue;
		}

		/* first see if the name is already here */
		bool foundEventSet = false;
		for (uint32_t j = 0; j < mEventNameIndexMaps.size(); j++)
		{
			if (mEventNameIndexMaps[j]->eventSetName == paramEventSetName)
			{
				// add index to list
				mEventNameIndexMaps[j]->eventIndices.pushBack((uint16_t)i);
				foundEventSet = true;
			}
		}

		if (!foundEventSet)
		{
			/* now add it to the list */
			EventNameIndexMap* newMap = PX_NEW(EventNameIndexMap)();
			newMap->eventSetName = paramEventSetName;
			newMap->eventIndices.pushBack((uint16_t)i);
			mEventNameIndexMaps.pushBack(newMap);
		}
	}
}

ImpactEmitterAssetImpl::ImpactEmitterAssetImpl(ModuleEmitterImpl* m, ResourceList& list, const char* name) :
	mModule(m),
	mName(name),
	mIofxAssetTracker(m->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(m->mSdk),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	mParams = (ImpactEmitterAssetParameters*)traits->createNvParameterized(ImpactEmitterAssetParameters::staticClassName());

	PX_ASSERT(mParams);

	mParams->setSerializationCallback(this);
	list.add(*this);
}

ImpactEmitterAssetImpl::ImpactEmitterAssetImpl(ModuleEmitterImpl* m,
                                       ResourceList& list,
                                       NvParameterized::Interface* params,
                                       const char* name) :
	mModule(m),
	mName(name),
	mIofxAssetTracker(m->mSdk, IOFX_AUTHORING_TYPE_NAME),
	mIosAssetTracker(m->mSdk),
	mParams((ImpactEmitterAssetParameters*)params),
	mDefaultActorParams(NULL),
	mDefaultPreviewParams(NULL)
{
	// this may no longer make any sense
	mParams->setSerializationCallback(this);

	// call this now to "initialize" the asset
	postDeserialize();

	list.add(*this);
}

void ImpactEmitterAssetImpl::destroy()
{
	/* Assets that were forceloaded or loaded by actors will be automatically
	 * released by the ApexAssetTracker member destructors.
	 */

	if (mParams)
	{
		mParams->destroy();
		mParams = NULL;
	}

	if (mDefaultActorParams)
	{
		mDefaultActorParams->destroy();
		mDefaultActorParams = 0;
	}

	if (mDefaultPreviewParams)
	{
		mDefaultPreviewParams->destroy();
		mDefaultPreviewParams = 0;
	}

	delete this;
}

ImpactEmitterAssetImpl::~ImpactEmitterAssetImpl()
{
	// cleanup the eventset name to indices map
	for (uint32_t j = 0; j < mEventNameIndexMaps.size(); j++)
	{
		if (mEventNameIndexMaps[j])
		{
			delete mEventNameIndexMaps[j];
		}
	}
}

NvParameterized::Interface* ImpactEmitterAssetImpl::getDefaultActorDesc()
{
	WRITE_ZONE();
	NvParameterized::ErrorType error = NvParameterized::ERROR_NONE;
	PX_UNUSED(error);

	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultActorParams)
	{
		const char* className = ImpactEmitterActorParameters::staticClassName();
		NvParameterized::Interface* param = traits->createNvParameterized(className);
		mDefaultActorParams = static_cast<ImpactEmitterActorParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}
	NvParameterized::Handle hDest(*mDefaultActorParams);
	
	return mDefaultActorParams;
}

NvParameterized::Interface* ImpactEmitterAssetImpl::getDefaultAssetPreviewDesc()
{
	WRITE_ZONE();
	NvParameterized::Traits* traits = GetInternalApexSDK()->getParameterizedTraits();
	PX_ASSERT(traits);
	if (!traits)
	{
		return NULL;
	}

	// create if not yet created
	if (!mDefaultPreviewParams)
	{
		const char* className = EmitterAssetPreviewParameters::staticClassName();
		NvParameterized::Interface* param = traits->createNvParameterized(className);
		mDefaultPreviewParams = static_cast<EmitterAssetPreviewParameters*>(param);

		PX_ASSERT(param);
		if (!param)
		{
			return NULL;
		}
	}

	return mDefaultPreviewParams;
}

Actor* ImpactEmitterAssetImpl::createApexActor(const NvParameterized::Interface& parms, Scene& apexScene)
{
	WRITE_ZONE();
	if (!isValidForActorCreation(parms, apexScene))
	{
		return NULL;
	}

	Actor* ret = 0;

	const char* className = parms.className();
	if (nvidia::strcmp(className, ImpactEmitterActorParameters::staticClassName()) == 0)
	{
		EmitterScene* es = mModule->getEmitterScene(apexScene);
		ImpactEmitterActorImpl* actor = PX_NEW(ImpactEmitterActorImpl)(parms, *this, mEmitterActors, *es);
		if (!actor->isValid())
		{
			actor->destroy();
			return NULL;
		}

		ret = actor;
	}
	else
	{
		APEX_INVALID_OPERATION("%s is not a valid descriptor class, expecting %s", className, ImpactEmitterActorParameters::staticClassName());
	}

	return ret;
}

AssetPreview* ImpactEmitterAssetImpl::createApexAssetPreview(const NvParameterized::Interface& /*params*/, AssetPreviewScene* /*previewScene*/)
{
	WRITE_ZONE();
	APEX_INVALID_OPERATION("Not yet implemented!");
	return NULL;
}

uint32_t ImpactEmitterAssetImpl::querySetID(const char* setName)
{
	WRITE_ZONE();
	for (uint32_t i = 0; i < mEventNameIndexMaps.size(); ++i)
	{
		if (mEventNameIndexMaps[i]->eventSetName == setName)
		{
			return i;
		}
	}
	return 0;   // return the default event set if no match is found
}

void ImpactEmitterAssetImpl::getSetNames(const char** outSetNames, uint32_t& inOutNameCount) const
{
	READ_ZONE();
	if (mEventNameIndexMaps.size() > inOutNameCount)
	{
		inOutNameCount = 0xffffffff;
		return;
	}

	inOutNameCount = mEventNameIndexMaps.size();
	for (uint32_t i = 0; i < mEventNameIndexMaps.size(); ++i)
	{
		outSetNames[i] = mEventNameIndexMaps[i]->eventSetName.c_str();
	}
}

uint32_t ImpactEmitterAssetImpl::forceLoadAssets()
{
	WRITE_ZONE();
	uint32_t assetLoadedCount = 0;

	assetLoadedCount += mIofxAssetTracker.forceLoadAssets();
	assetLoadedCount += mIosAssetTracker.forceLoadAssets();

	return assetLoadedCount;
}


void ImpactEmitterAssetImpl::release()
{
	mModule->mSdk->releaseAsset(*this);
}

#ifndef WITHOUT_APEX_AUTHORING
void ImpactEmitterAssetAuthoringImpl::release()
{
	mModule->mSdk->releaseAssetAuthoring(*this);
}

#endif

}
} // namespace nvidia::apex
