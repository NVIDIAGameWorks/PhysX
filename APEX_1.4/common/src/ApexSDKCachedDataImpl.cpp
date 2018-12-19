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



#include "ApexSDKCachedDataImpl.h"
#include "nvparameterized/NvParameterized.h"

namespace nvidia
{
namespace apex
{

bool ApexSDKCachedDataImpl::registerModuleDataCache(ModuleCachedDataIntl* cache)
{
	if (cache == NULL)
	{
		return false;
	}

	for (uint32_t i = 0; i < mModuleCaches.size(); ++i)
	{
		if (cache == mModuleCaches[i])
		{
			return false;
		}
	}

	mModuleCaches.pushBack(cache);

	return true;
}

bool ApexSDKCachedDataImpl::unregisterModuleDataCache(ModuleCachedDataIntl* cache)
{
	if (cache == NULL)
	{
		return false;
	}

	for (uint32_t i = mModuleCaches.size(); i--;)
	{
		if (cache == mModuleCaches[i])
		{
			mModuleCaches.replaceWithLast(i);
			break;
		}
	}

	return false;
}

ApexSDKCachedDataImpl::ApexSDKCachedDataImpl()
{
}

ApexSDKCachedDataImpl::~ApexSDKCachedDataImpl()
{
}

ModuleCachedData* ApexSDKCachedDataImpl::getCacheForModule(AuthObjTypeID moduleID)
{
	for (uint32_t i = 0; i < mModuleCaches.size(); ++i)
	{
		if (moduleID == mModuleCaches[i]->getModuleID())
		{
			return mModuleCaches[i];
		}
	}

	return NULL;
}

PxFileBuf& ApexSDKCachedDataImpl::serialize(PxFileBuf& stream) const
{
	stream.storeDword((uint32_t)Version::Current);

	for (uint32_t i = 0; i < mModuleCaches.size(); ++i)
	{
		mModuleCaches[i]->serialize(stream);
	}

	return stream;
}

PxFileBuf& ApexSDKCachedDataImpl::deserialize(PxFileBuf& stream)
{
	clear(false); // false => don't delete cached data for referenced sets

	/*const uint32_t version =*/
	stream.readDword();	// Original version

	for (uint32_t i = 0; i < mModuleCaches.size(); ++i)
	{
		mModuleCaches[i]->deserialize(stream);
	}

	return stream;
}

void ApexSDKCachedDataImpl::clear(bool force)
{
	for (uint32_t i = mModuleCaches.size(); i--;)
	{
		mModuleCaches[i]->clear(force);
	}
}

}
} // end namespace nvidia::apex
