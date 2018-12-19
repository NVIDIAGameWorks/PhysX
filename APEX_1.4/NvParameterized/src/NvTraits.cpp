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


#include "NvTraits.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PsUserAllocated.h"
#include "PsArray.h"
#include "PsAtomic.h"
#include "PsAlignedMalloc.h"
#include "PxFoundation.h"
#include "PxErrorCallback.h"
#include "PsString.h"

#define ALIGNED_ALLOC(n, align) physx::shdfnd::AlignedAllocator<align>().allocate(n, __FILE__, __LINE__)

// Does not depend on alignment in AlignedAllocator
#define ALIGNED_FREE(p) physx::shdfnd::AlignedAllocator<16>().deallocate(p)


namespace NvParameterized
{

class NvTraits : public Traits, public physx::shdfnd::UserAllocated
{
public:
	NvTraits(void)
	{

	}

	virtual ~NvTraits(void)
	{
	}

	virtual void release(void)
	{
		delete this;
	}

	/**
	\brief Register NvParameterized class factory
	*/
	virtual void registerFactory( ::NvParameterized::Factory & factory ) 
	{
		bool ok = true;

		for (uint32_t i=0; i<mFactories.size(); ++i)
		{
			Factory *f = mFactories[i];
			if ( f == &factory )
			{
				ok = false;
				traitsWarn("Factory already registered.");
				break;
			}
			if ( strcmp(f->getClassName(),factory.getClassName()) == 0 && f->getVersion() == factory.getVersion() )
			{
				ok = false;
				traitsWarn("factory with this name and version already registered.");
				break;
			}
		}
		if ( ok )
		{
			mFactories.pushBack(&factory);
		}
	}

	/**
	\brief Remove NvParameterized class factory for current version of class
	\return Removed factory or NULL if it is not found
	*/
	virtual ::NvParameterized::Factory *removeFactory( const char * className ) 
	{
		NvParameterized::Factory *f = NULL;
		uint32_t index=0;
		uint32_t maxVersion = 0;
		for (uint32_t i=0; i<mFactories.size(); i++)
		{
			if ( strcmp(mFactories[i]->getClassName(),className) == 0 )
			{
				if ( mFactories[i]->getVersion() >= maxVersion )
				{
					f = mFactories[i];
					maxVersion = f->getVersion();
					index = i;
				}
			}
		}
		if ( f )
		{
			mFactories.remove(index);
		}
		else
		{
			traitsWarn("Unable to remove factory.");
		}
		return f;
	}

	/**
	\brief Remove NvParameterized class factory for given version of class
	\return Removed factory or NULL if it is not found
	*/
	virtual ::NvParameterized::Factory *removeFactory( const char * className, uint32_t version ) 
	{
		Factory *f = NULL;
		for (uint32_t i=0; i<mFactories.size(); ++i)
		{
			if ( strcmp(mFactories[i]->getClassName(),className) == 0 && mFactories[i]->getVersion() == version )
			{
				f = mFactories[i];
				mFactories.remove(i);
				break;
			}
		}
		if ( !f )
		{
			traitsWarn("Unable to remove factory, not found");
		}
		return f;
	}

	/**
	\brief Checks whether any class factory is registered
	*/
	virtual bool doesFactoryExist(const char* className) 
	{
		bool ret = false;
		for (uint32_t i=0; i<mFactories.size(); ++i)
		{
			if ( strcmp(mFactories[i]->getClassName(),className) == 0 )
			{
				ret = true;
				break;
			}
		}
		return ret;
	}

	/**
	\brief Checks whether class factory for given version is registered
	*/
	virtual bool doesFactoryExist(const char* className, uint32_t version) 
	{
		bool ret = false;
		for (uint32_t i=0; i<mFactories.size(); ++i)
		{
			if ( strcmp(mFactories[i]->getClassName(),className) == 0 && mFactories[i]->getVersion() == version )
			{
				ret = true;
				break;
			}
		}
		return ret;
	}

	// Helper method, locate a factory of this name and exact version, of it specific version not being searched for, return the highest registered version number.
	Factory * locateFactory(const char *className,uint32_t version,bool useVersion) const
	{
		NvParameterized::Factory *f = NULL;
		uint32_t maxVersion = 0;
		for (uint32_t i=0; i<mFactories.size(); i++)
		{
			if ( strcmp(mFactories[i]->getClassName(),className) == 0 )
			{
				if ( useVersion )
				{
					if ( mFactories[i]->getVersion() == version )
					{
						f = mFactories[i];
					}
				}
				else if ( mFactories[i]->getVersion() >= maxVersion )
				{
					f = mFactories[i];
					maxVersion = f->getVersion();
				}
			}
		}
		return f;
	}

	/**
	\brief Create object of NvParameterized class
	
	Most probably this just calls Factory::create on appropriate factory.
	*/
	virtual ::NvParameterized::Interface * createNvParameterized( const char * name ) 
	{
		NvParameterized::Interface *ret = NULL;
		Factory *f = locateFactory(name,0,false);
		if ( f )
		{
			ret = f->create(this);
		}
		return ret;
	}

	/**
	\brief Create object of NvParameterized class
	
	Most probably this just calls Factory::create on appropriate factory.
	*/
	virtual ::NvParameterized::Interface * createNvParameterized( const char * name, uint32_t ver )
	{
		NvParameterized::Interface *ret = NULL;
		Factory *f = locateFactory(name,ver,true);
		if ( f )
		{
			ret = f->create(this);
		}
		return ret;
	}

	/**
	\brief Finish construction of inplace object of NvParameterized class
	
	Most probably this just calls Factory::finish using appropriate factory.
	*/
	virtual ::NvParameterized::Interface * finishNvParameterized( const char * name, void *obj, void *buf, int32_t *refCount ) 
	{
		Factory *f = locateFactory(name,0,false);
		return f ? f->finish(this,obj,buf,refCount) : NULL;
	}

	/**
	\brief Finish construction of inplace object of NvParameterized class
	
	Most probably this just calls Factory::finish using appropriate factory.
	*/
	virtual ::NvParameterized::Interface * finishNvParameterized( const char * name, uint32_t ver, void *obj, void *buf, int32_t *refCount ) 
	{
		Factory *f = locateFactory(name,ver,true);
		return f ? f->finish(this,obj,buf,refCount) : NULL;
	}

	/**
	\brief Get version of class which is currently used
	*/
	virtual uint32_t getCurrentVersion(const char *className) const 
	{
		Factory *f = locateFactory(className,0,false);
		return f ? f->getVersion() : 0;
	}

	/**
	\brief Get memory alignment required for objects of class
	*/
	virtual uint32_t getAlignment(const char *className, uint32_t classVersion) const 
	{
		Factory *f = locateFactory(className,classVersion,true);
		return f ? f->getAlignment() : 16;
	}

	/**
	\brief Register converter for legacy version of class
	*/
	virtual void registerConversion(const char * /*className*/, uint32_t /*from*/, uint32_t /*to*/, Conversion & /*conv*/) 
	{
		PX_ALWAYS_ASSERT(); // TODO : Not yet implemented
	}

	/**
	\brief Remove converter for legacy version of class
	*/
	virtual ::NvParameterized::Conversion *removeConversion(const char * /*className*/, uint32_t /*from*/, uint32_t /*to*/) 
	{ 
		PX_ALWAYS_ASSERT(); // TODO : Not yet implemented
		return 0; 
	}

	/**
	\brief Update legacy object (most probably using appropriate registered converter)
	\param [in] legacyObj legacy object to be converted
	\param [in] obj destination object
	\return True if conversion was successful, false otherwise
	\warning Note that update is intrusive - legacyObj may be modified as a result of update
	*/
	virtual bool updateLegacyNvParameterized(::NvParameterized::Interface &legacyObj, ::NvParameterized::Interface &obj)
	{
		PX_ALWAYS_ASSERT(); // TODO : Not yet implemented		
		PX_UNUSED(&legacyObj);
		PX_UNUSED(&obj);

		return false;
	}

	/**
	\brief Get a list of the NvParameterized class type names

	\param [in] names buffer for names
	\param [out] outCount minimal required length of buffer
	\param [in] inCount length of buffer
	\return False if 'inCount' is not large enough to contain all of the names, true otherwise
	
	\warning The memory for the strings returned is owned by the traits class
	         and should only be read, not written or freed.
	*/
	virtual bool getNvParameterizedNames( const char ** names, uint32_t &outCount, uint32_t inCount) const 
	{
		bool ret = true;

		outCount = 0;
		for (uint32_t i=0; i<mFactories.size(); i++)
		{
			Factory *f = mFactories[i];
			const char *name = f->getClassName();
			for (uint32_t j=0; j<outCount; j++)
			{
				if ( strcmp(name,names[j]) == 0 )
				{
					name = NULL;
					break;
				}
			}
			if ( name )
			{
				if ( outCount == inCount )
				{
					ret = false;
					break;
				}
				else
				{
					names[outCount] = name;
					outCount++;
				}
			}
		}
		return ret;
	}

	/**
	\brief Get a list of versions of particular NvParameterized class

	\param [in] className Name of the class
	\param [in] versions buffer for versions
	\param [out] outCount minimal required length of buffer
	\param [in] inCount length of buffer
	\return False if 'inCount' is not large enough to contain all of version names, true otherwise
	
	\warning The memory for the strings returned is owned by the traits class
	         and should only be read, not written or freed.
	*/
	virtual bool getNvParameterizedVersions(const char* className, uint32_t* versions, uint32_t &outCount, uint32_t inCount) const 
	{
		bool ret = true;

		outCount = 0;
		for (uint32_t i=0; i<mFactories.size(); i++)
		{
			Factory *f = mFactories[i];
			const char *name = f->getClassName();
			if ( strcmp(name,className) == 0 )
			{
				if ( outCount == inCount )
				{
					ret = false;
					break;
				}
				else
				{
					versions[outCount] = f->getVersion();
					outCount++;
				}
			}
		}
		return ret;
	}

	/**
	\brief Called when inplace object is destroyed
	*/
	virtual void onInplaceObjectDestroyed(void * /*buf*/, ::NvParameterized::Interface * /*obj*/) 
	{

	}

	/**
	\brief Called when all inplace objects are destroyed
	*/
	virtual void onAllInplaceObjectsDestroyed(void *buf) 
	{ 
		free(buf); 
	}

	void* alloc(uint32_t nbytes)
	{
		return alloc(nbytes, 16);
	}

	void* alloc(uint32_t nbytes, uint32_t align)
	{
		if (align <= 16)
		{
			return ALIGNED_ALLOC(nbytes, 16);
		}
		else switch (align)
		{
		case 32:
			return ALIGNED_ALLOC(nbytes, 32);
		case 64:
			return ALIGNED_ALLOC(nbytes, 64);
		case 128:
			return ALIGNED_ALLOC(nbytes, 128);
		}

		// Do not support larger alignments

		return 0;
	}

	void free(void* buf)
	{
		ALIGNED_FREE(buf);
	}

	int32_t incRefCount(int32_t* refCount)
	{
		return physx::shdfnd::atomicIncrement(refCount);
	}

	virtual int32_t decRefCount(int32_t* refCount)
	{
		return physx::shdfnd::atomicDecrement(refCount);
	}


 	/**
	\brief Warns user
	*/
	virtual void traitsWarn(const char * msg) const 
	{
		char scratch[512];
		physx::shdfnd::snprintf(scratch,512,"NvParameterized::Traits::Warning(%s)", msg);
		PxGetFoundation().getErrorCallback().reportError(physx::PxErrorCode::eDEBUG_WARNING,scratch,__FILE__,__LINE__ );
	}

	physx::shdfnd::Array< Factory * > mFactories;
};

Traits *createTraits(void)
{
	NvTraits *n = PX_NEW(NvTraits);
	return static_cast< Traits *>(n);
}

}
