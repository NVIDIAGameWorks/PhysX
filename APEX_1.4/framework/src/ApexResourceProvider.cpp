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
#include "ResourceProviderIntl.h"
#include "ApexResourceProvider.h"
#include "PsUserAllocated.h"
#include "ApexSDKImpl.h"
#include "PsString.h"
#include <ctype.h> // for toupper()

namespace nvidia
{
namespace apex
{

#pragma warning(disable: 4355)

ApexResourceProvider::ApexResourceProvider() 
: mNSNames(this, 0, false, 0)
, mCaseSensitive(false)
{
}

ApexResourceProvider::~ApexResourceProvider()
{
}

/* == Public ResourceProvider interface == */
void ApexResourceProvider::registerCallback(ResourceCallback* func)
{
	mUserCallback = func;
}

void ApexResourceProvider::setResource(const char* nameSpace, const char* name, void* resource, bool incRefCount)
{
	setResource(nameSpace, name, resource, true, incRefCount);
}

void ApexResourceProvider::setResource(const char* nameSpace, const char* name, void* resource, bool valueIsSet, bool incRefCount)
{
	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			ApexResourceProvider::resource& res = mResources[id];
			res.ptr = resource;
			res.valueIsSet = (uint8_t)valueIsSet;
			if (incRefCount)
			{
				res.refCount++;
			}
		}
	}
}

void* ApexResourceProvider::getResource(const char* nameSpace, const char* name)
{
	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			return getResource(id);
		}
	}
	return NULL;
}

/* == Internal ResourceProviderIntl interface == */
ResID ApexResourceProvider::createNameSpaceInternal(const char* &nameSpace, bool releaseAtExit)
{
	/* create or get a name space */
	size_t  nextID = mResources.size();
	ResID nsID = mNSNames.getOrCreateID(nameSpace, "NameSpace");
	if (nsID == (ResID) nextID)
	{
		NameSpace* ns = PX_NEW(NameSpace)(this, nsID, releaseAtExit, nameSpace);
		if (ns)
		{
			ResID id = getNSID(nameSpace);
			mNSID.insert(id, mNameSpaces.size());

			mResources[nsID].ptr = (void*)(size_t) id;
			mNameSpaces.pushBack(ns);
		}
		else
		{
			return INVALID_RESOURCE_ID;
		}
	}
	ResID ret = (ResID)(size_t) mResources[nsID].ptr;
	return ret;
}

ResID ApexResourceProvider::createResource(ResID nameSpace, const char* name, bool refCount)
{
	uint32_t nsIndex = getNSIndex(nameSpace);
	if (nsIndex < mNameSpaces.size())
	{
		NameSpace* ns = mNameSpaces[nsIndex];
		ResID id = ns->getOrCreateID(name, mResources[ns->getID()].name);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size() && refCount)
		{
			mResources[id].refCount++;
		}
		return id;
	}
	else
	{
		return INVALID_RESOURCE_ID;
	}
}

bool ApexResourceProvider::checkResource(ResID nameSpace, const char* name)
{
	/* Return true is named resource has known non-null pointer */
	uint32_t nsIndex = getNSIndex(nameSpace);
	if (nsIndex < mNameSpaces.size())
	{
		NameSpace* ns = mNameSpaces[nsIndex];
		ResID id = ns->getOrCreateID(name, mResources[ns->getID()].name);
		PX_ASSERT(id < mResources.size());
		return checkResource(id);
	}
	return false;
}

bool ApexResourceProvider::checkResource(ResID id)
{
	if (mResources.size() <= id)
	{
		return false;
	}

	ApexResourceProvider::resource& res = mResources[id];

	if (!res.valueIsSet)
	{
		return false;
	}

	return true;
}

void ApexResourceProvider::generateUniqueName(ResID nameSpace, ApexSimpleString& name)
{
	uint32_t nsIndex = getNSIndex(nameSpace);
	if (nsIndex < mNameSpaces.size())
	{
		ApexSimpleString test;
		uint32_t   count = 1;
		char	buf[64];
		*buf = '.';

		do
		{
			shdfnd::snprintf(buf + 1,60, "%d", count);
			test = name + ApexSimpleString(buf);
			if (!checkResource(nameSpace, test.c_str()))
			{
				break;
			}
		}
		while (++count < 0xFFFFFFF0);

		name = test;
	}
}

void ApexResourceProvider::releaseResource(ResID id)
{
	if (mResources.size() <= id)
	{
		return;
	}

	ApexResourceProvider::resource& res = mResources[id];

	PX_ASSERT(res.refCount);
	if (res.refCount > 0)
	{
		res.refCount--;

		if (res.refCount == 0 && mUserCallback &&
		        res.valueIsSet && res.ptr != NULL)
		{
			if (mResources[id].usedGetResource)   // Defect DE641 only callback to the user, if this resource was created by a GetResource request
			{
				mUserCallback->releaseResource(res.nameSpace, res.name, res.ptr);
			}
			res.ptr = (void*) UnknownValue;
			res.valueIsSet = false;
			res.usedGetResource = false;
		}
		// if the ptr is NULL and we're releasing it, we do want to call requestResource next time it is requested, so valueIsSet = false
		else if (res.refCount == 0 && res.valueIsSet && res.ptr == NULL)
		{
			res.ptr = (void*) UnknownValue;
			res.valueIsSet = false;
			res.usedGetResource = false;
		}
	}
}

void* ApexResourceProvider::getResource(ResID id)
{
	if (mResources.size() <= id)
	{
		return NULL;
	}
	else if (!mResources[id].valueIsSet)
	{
		// PH: WARNING: This MUST not be a reference, mResource can be altered during the requestResource() operation!!!!
		ApexResourceProvider::resource res = mResources[id];
		if (mUserCallback)
		{
			// tmp ensures that the [] operator is called AFTER mResources is possibly
			// resized by something in requestResources
			void* tmp = mUserCallback->requestResource(res.nameSpace, res.name);
			res.ptr = tmp;
			res.valueIsSet = true;
			res.usedGetResource = true;
		}
		else
		{
			res.ptr = NULL;
		}
		mResources[id] = res;
	}
	return mResources[id].ptr;
}

const char* ApexResourceProvider::getResourceName(ResID id)
{
	if (mResources.size() <= id)
	{
		return NULL;
	}
	return mResources[id].name;
}
const char* ApexResourceProvider::getResourceNameSpace(ResID id)
{
	if (mResources.size() <= id)
	{
		return NULL;
	}
	return mResources[id].nameSpace;
}


bool ApexResourceProvider::getResourceIDs(const char* nameSpace, ResID* outResIDs, uint32_t& outCount, uint32_t inCount)
{
	outCount = 0;

	if (!outResIDs)
	{
		return false;
	}

	for (uint32_t i = 0; i < mResources.size(); i++)
	{
		if (stringsMatch(mResources[i].nameSpace, nameSpace))
		{
			if (outCount > inCount)
			{
				outCount = 0;
				return false;
			}
			outResIDs[outCount++] = i;
		}
	}

	return true;
}

void ApexResourceProvider::destroy()
{
	if (mUserCallback)
	{
		for (uint32_t i = 0 ; i < mResources.size() ; i++)
		{
			ApexResourceProvider::resource& res = mResources[i];
			if (res.refCount != 0 && res.valueIsSet && res.ptr != NULL)
			{
				ResID resIndex = mNSNames.getOrCreateID(res.nameSpace, "NameSpace");
				PX_ASSERT(mResources[resIndex].ptr);
				uint32_t nsIndex = getNSIndex((ResID)(size_t)mResources[resIndex].ptr);
				if (nsIndex < mNameSpaces.size() &&
					mNameSpaces[nsIndex]->releaseAtExit())
				{
					if (res.usedGetResource) // this check added for PhysXLab DE4349
				{
					mUserCallback->releaseResource(res.nameSpace, res.name, res.ptr);
				}
					else
					{
						APEX_DEBUG_WARNING("Unreleased resource found during teardown: Namespace <%s>, Name <%s>", res.nameSpace, res.name);
					}
				}
			}
		}
	}
	mResources.clear();
	for (uint32_t i = 0 ; i < mNameSpaces.size() ; i++)
	{
		PX_DELETE(mNameSpaces[i]);
	}
	mNameSpaces.clear();
	delete this;
}

ApexResourceProvider::NameSpace::NameSpace(ApexResourceProvider* arp, ResID nsid, bool releaseAtExit, const char* nameSpace) :
	mReleaseAtExit(releaseAtExit),
	mArp(arp),
	mId(nsid)
{
	memset(hash, 0, sizeof(hash));
	mNameSpace = 0;
	if (nameSpace)
	{
		uint32_t len = (uint32_t) strlen(nameSpace);
		mNameSpace = (char*)PX_ALLOC(len + 1, PX_DEBUG_EXP("ApexResourceProvider::NameSpace"));
		memcpy(mNameSpace, nameSpace, len + 1);
	}
}

ApexResourceProvider::NameSpace::~NameSpace()
{
	// Free up all collision chains in the hash table
	for (uint32_t i = 0 ; i < HashSize ; i++)
	{
		while (hash[i])
		{
			const char* entry = hash[i];
			const entryHeader* hdr = (const entryHeader*) entry;
			const char* next = hdr->nextEntry;
			PX_FREE((void*) entry);
			hash[i] = next;
		}
	}
	PX_FREE(mNameSpace);
}

ResID	ApexResourceProvider::NameSpace::getOrCreateID(const char* &name, const char* NSName)
{
	/* Hash Table Entry:   | nextEntry* | ResID | name     | */
	uint16_t h = genHash(name);
	const char* entry = hash[h];

	while (entry)
	{
		entryHeader* hdr = (entryHeader*) entry;
		const char* entryName = entry + sizeof(entryHeader);

		if (mArp->stringsMatch(name, entryName))
		{
			name = entryName;
			return hdr->id;
		}

		entry = hdr->nextEntry;
	}

	size_t len = strlen(name);
	size_t bufsize = len + 1 + sizeof(entryHeader);
	char* newEntry = (char*) PX_ALLOC(bufsize, PX_DEBUG_EXP("ApexResourceProvider::NameSpace::getOrCreateID"));
	if (newEntry)
	{
#if defined(WIN32)
		strncpy_s(newEntry + sizeof(entryHeader), bufsize - sizeof(entryHeader), name, len);
#else
		strcpy(newEntry + sizeof(entryHeader), name);
#endif
		entryHeader* hdr = (entryHeader*) newEntry;
		hdr->nextEntry = hash[h];
		hdr->id = mArp->mResources.size();

		resource res;
		res.ptr = (void*) UnknownValue;
		res.valueIsSet = false;
		res.name = newEntry + sizeof(entryHeader);
		res.nameSpace = NSName;
		res.refCount = 0;
		res.usedGetResource = 0;
		mArp->mResources.pushBack(res);

		hash[h] = (const char*) newEntry;

		name = res.name;
		return hdr->id;
	}

	return INVALID_RESOURCE_ID;
}

ResID ApexResourceProvider::getNSID(const char* nsName)
{
	physx::Hash<const char*> h;
	ResID id = h(nsName);
	const HashMapNSID::Entry* nsid = mNSID.find(id);
	if (nsid)
	{
		PX_ASSERT(nsid->second < mNameSpaces.size() && mNameSpaces[nsid->second] != NULL);
		if (nsid->second < mNameSpaces.size() && mNameSpaces[nsid->second] != NULL && !h.equal(nsName, mNameSpaces[nsid->second]->getNameSpace()))
		{
			PX_ALWAYS_ASSERT_MESSAGE("Hash collision detected for namespaces in ApexResourceProvider. Try to adjust hash function.");
			return INVALID_RESOURCE_ID;
		}
	}
	return id;
}

uint32_t ApexResourceProvider::getNSIndex(ResID nameSpace)
{
	PX_ASSERT(nameSpace != INVALID_RESOURCE_ID);
	if (nameSpace == INVALID_RESOURCE_ID) return INVALID_RESOURCE_ID;
	const HashMapNSID::Entry* ns = mNSID.find(nameSpace);
	PX_ASSERT(ns);
	uint32_t nsIndex = ns ? ns->second : INVALID_RESOURCE_ID;
	PX_ASSERT(nsIndex < mNameSpaces.size());
	return nsIndex;
}

uint16_t ApexResourceProvider::NameSpace::genHash(const char* name)
{
	PX_ASSERT(name != NULL);
	/* XOR each 32bit word together */
	uint32_t h = 0;
	uint32_t* read32 = (uint32_t*)name;
	size_t len = strlen(name);

	/* Add remaining bytes */
	uint8_t* read8 = (uint8_t*) read32;
	while (len)
	{
		if (mArp->isCaseSensitive())
		{
			h ^= *read8;
		}
		else
		{
			h ^= toupper(*read8);
		}
		read8++;
		len -= sizeof(uint8_t);
	}

	/* XOR fold top 16 bits over bottom 16 bits */
	h ^= (h >> 16);

	return (uint16_t)(h & (HashSize - 1));
}

void ApexResourceProvider::dumpResourceTable()
{
	APEX_DEBUG_INFO("ApexResourceProvider::dumpResourceTable");
	APEX_DEBUG_INFO("namespace name refcount pointer valueIsSet");

	for (uint32_t i = 0; i < mResources.size(); i++)
	{
		APEX_DEBUG_INFO("%s %s %d 0x%08x %d", mResources[i].nameSpace, mResources[i].name, mResources[i].refCount, mResources[i].ptr, mResources[i].valueIsSet);
	}
}


void   			ApexResourceProvider::setResourceU32(const char* nameSpace, const char* name, uint32_t id, bool incRefCount)
{
	setResource(nameSpace, name, (void*)(size_t)id, true, incRefCount);
}

uint32_t  			ApexResourceProvider::releaseAllResourcesInNamespace(const char* nameSpace)
{
	uint32_t ret = 0;

	for (uint32_t i = 0; i < mResources.size(); i++)
	{
		ApexResourceProvider::resource& res = mResources[i];
		if (stringsMatch(res.nameSpace, nameSpace) &&  res.valueIsSet)
		{
			ret++;
			PX_ASSERT(res.refCount);
			if (res.refCount > 0)
			{
				res.refCount--;
				if (res.refCount == 0 && mUserCallback &&
				        res.valueIsSet && res.ptr != NULL)
				{
					mUserCallback->releaseResource(res.nameSpace, res.name, res.ptr);
					res.ptr = (void*) UnknownValue;
					res.valueIsSet = false;
				}
			}
		}
	}

	return ret;
}

uint32_t  			ApexResourceProvider::releaseResource(const char* nameSpace, const char* name)
{
	uint32_t ret = 0;

	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			ApexResourceProvider::resource& res = mResources[id];
			if (res.valueIsSet)
			{
				ret = (uint32_t)res.refCount - 1;
				releaseResource(id);
			}
		}
	}


	return ret;
}

bool    		ApexResourceProvider::findRefCount(const char* nameSpace, const char* name, uint32_t& refCount)
{
	bool ret = false;
	refCount = 0;
	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			if (mResources[id].valueIsSet)
			{
				ret = true;
				refCount = mResources[id].refCount;
			}
		}
	}

	return ret;
}

void* 			ApexResourceProvider::findResource(const char* nameSpace, const char* name)
{
	void* ret = NULL;
	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			if (mResources[id].valueIsSet)
			{
				ret = mResources[id].ptr;
			}
		}
	}
	return ret;
}

uint32_t 			ApexResourceProvider::findResourceU32(const char* nameSpace, const char* name) // find an existing resource.
{
	uint32_t ret = 0;
	PX_ASSERT(nameSpace);
	PX_ASSERT(name);
	uint32_t nsIndex = getNSIndex(createNameSpaceInternal(nameSpace, true));
	if (nsIndex < mNameSpaces.size())
	{
		ResID id = mNameSpaces[nsIndex]->getOrCreateID(name, nameSpace);
		PX_ASSERT(id < mResources.size());
		if (id < mResources.size())
		{
			if (mResources[id].valueIsSet)
			{
#if defined(PX_X64) || defined(PX_A64)
				uint64_t ret64 = (uint64_t)mResources[id].ptr;
				ret = (uint32_t)ret64;
#else
				ret = (uint32_t)mResources[id].ptr;
#endif
			}
		}
	}
	return ret;

}

void** 		ApexResourceProvider::findAllResources(const char* nameSpace, uint32_t& count) // find all resources in this namespace
{
	void** ret = 0;
	count = 0;

	mCharResults.clear();
	for (uint32_t i = 0; i < mResources.size(); i++)
	{
		if (stringsMatch(nameSpace, mResources[i].nameSpace))
		{
			if (mResources[i].valueIsSet)
			{
				mCharResults.pushBack((const char*)mResources[i].ptr);
			}
		}
	}
	if (!mCharResults.empty())
	{
		ret = (void**)&mCharResults[0];
		count = mCharResults.size();
	}

	return ret;
}

const char** 		ApexResourceProvider::findAllResourceNames(const char* nameSpace, uint32_t& count) // find all resources in this namespace
{
	const char** ret = 0;
	count = 0;

	mCharResults.clear();
	for (uint32_t i = 0; i < mResources.size(); i++)
	{
		if (stringsMatch(nameSpace, mResources[i].nameSpace) && mResources[i].valueIsSet)
		{
			mCharResults.pushBack(mResources[i].name);
		}
	}
	if (!mCharResults.empty())
	{
		ret = &mCharResults[0];
		count = mCharResults.size();
	}

	return ret;
}

const char** 	ApexResourceProvider::findNameSpaces(uint32_t& count)
{
	const char** ret = 0;
	count = 0;

	mCharResults.clear();
	for (physx::Array<NameSpace*>::Iterator i = mNameSpaces.begin(); i != mNameSpaces.end(); ++i)
	{
		const char* nameSpace = (*i)->getNameSpace();
		if (nameSpace)
		{
			mCharResults.pushBack(nameSpace);
		}
	}

	if (!mCharResults.empty())
	{
		count = mCharResults.size();
		ret = &mCharResults[0];
	}

	return ret;
}

bool ApexResourceProvider::stringsMatch(const char* str0, const char* str1)
{
	if (mCaseSensitive)
	{
		return !nvidia::strcmp(str0, str1);
	}
	else
	{
		return !nvidia::stricmp(str0, str1);
	}

}


}
} // end namespace nvidia::apex
