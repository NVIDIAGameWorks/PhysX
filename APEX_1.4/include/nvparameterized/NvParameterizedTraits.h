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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

#ifndef NV_PARAMETERIZED_TRAITS_H
#define NV_PARAMETERIZED_TRAITS_H

/*!
\file
\brief NvParameterized traits class
*/

#include <string.h>
#include "foundation/PxAssert.h"

namespace NvParameterized
{

PX_PUSH_PACK_DEFAULT

class Traits;
class Interface;

/**
\brief Interface class for NvParameterized factories
*/
class Factory
{
public:

	/**
	\brief Creates NvParameterized object of class
	*/
	virtual ::NvParameterized::Interface *create( Traits *paramTraits ) = 0;

	/**
	\brief Finishes initialization of inplace-deserialized objects (vptr and stuff)
	*/
	virtual ::NvParameterized::Interface *finish( Traits *paramTraits, void *obj, void *buf, int32_t *refCount ) = 0;

	/**
	\brief Returns name of class whose objects are created by factory
	*/
	virtual const char * getClassName() = 0;

	/**
	\brief Returns version of class whose objects are created by factory
	*/
	virtual uint32_t getVersion() = 0;

	/**
	\brief Returns memory alignment required for objects of class
	*/
	virtual uint32_t getAlignment() = 0;

	/**
	\brief Returns checksum of class whose objects are created by factory
	*/
	virtual const uint32_t * getChecksum( uint32_t &bits ) = 0;

	/**
	\brief Destructor
	*/
	virtual ~Factory() {}

	/**
	\brief Clean reflection definition table. Call it if you don't have any instances of classes.
	*/	
	virtual void freeParameterDefinitionTable(NvParameterized::Traits* traits) = 0;
};

/**
\brief Interface class for legacy object conversions
*/
class Conversion
{
public:
	virtual ~Conversion() {}

	/**
	\brief Initialize object using data from legacy object
	\param [in] legacyObj legacy object to be converter
	\param [in] obj destination object
	\return true if conversion succeeded, false otherwise
	\warning You may assume that all nested references were already converted.
	*/
	virtual bool operator()(::NvParameterized::Interface &legacyObj, ::NvParameterized::Interface &obj) = 0;

	/**
	\brief Release converter and any memory allocations associated with it
	*/
	virtual void release() = 0;
};

/**
\brief Interface class for user traits

This class is a collection of loosely-related functions provided by application or framework
and used by NvParameterized library to do memory allocation, object creation, user notification, etc.
*/
class Traits
{
public:
	virtual ~Traits() {}

	/**
	\brief Register NvParameterized class factory
	*/
	virtual void registerFactory( ::NvParameterized::Factory & factory ) = 0;

	/**
	\brief Remove NvParameterized class factory for current version of class
	\return Removed factory or NULL if it is not found
	*/
	virtual ::NvParameterized::Factory *removeFactory( const char * className ) = 0;

	/**
	\brief Remove NvParameterized class factory for given version of class
	\return Removed factory or NULL if it is not found
	*/
	virtual ::NvParameterized::Factory *removeFactory( const char * className, uint32_t version ) = 0;

	/**
	\brief Checks whether any class factory is registered
	*/
	virtual bool doesFactoryExist(const char* className) = 0;

	/**
	\brief Checks whether class factory for given version is registered
	*/
	virtual bool doesFactoryExist(const char* className, uint32_t version) = 0;

	/**
	\brief Create object of NvParameterized class using its staticClassName()
	
	\param [in] name static class name of the instance to create
	
	Most probably this just calls Factory::create on appropriate factory.
	*/
	virtual ::NvParameterized::Interface * createNvParameterized( const char * name ) = 0;

	/**
	\brief Create object of NvParameterized class using its staticClassName()
	
	\param [in] name static class name of the instance to create
	\param [in] ver version of the class	

	Most probably this just calls Factory::create on appropriate factory.
	*/
	virtual ::NvParameterized::Interface * createNvParameterized( const char * name, uint32_t ver ) = 0;

	/**
	\brief Finish construction of inplace object of NvParameterized class
	
	Most probably this just calls Factory::finish using appropriate factory.
	*/
	virtual ::NvParameterized::Interface * finishNvParameterized( const char * name, void *obj, void *buf, int32_t *refCount ) = 0;

	/**
	\brief Finish construction of inplace object of NvParameterized class
	
	Most probably this just calls Factory::finish using appropriate factory.
	*/
	virtual ::NvParameterized::Interface * finishNvParameterized( const char * name, uint32_t ver, void *obj, void *buf, int32_t *refCount ) = 0;

	/**
	\brief Get version of class which is currently used
	*/
	virtual uint32_t getCurrentVersion(const char *className) const = 0;

	/**
	\brief Get memory alignment required for objects of class
	*/
	virtual uint32_t getAlignment(const char *className, uint32_t classVersion) const = 0;

	/**
	\brief Register converter for legacy version of class
	*/
	virtual void registerConversion(const char * /*className*/, uint32_t /*from*/, uint32_t /*to*/, Conversion & /*conv*/) {}

	/**
	\brief Remove converter for legacy version of class
	*/
	virtual ::NvParameterized::Conversion *removeConversion(const char * /*className*/, uint32_t /*from*/, uint32_t /*to*/) { return 0; }

	/**
	\brief Update legacy object (most probably using appropriate registered converter)
	\param [in] legacyObj legacy object to be converted
	\param [in] obj destination object
	\return True if conversion was successful, false otherwise
	\warning Note that update is intrusive - legacyObj may be modified as a result of update
	*/
	virtual bool updateLegacyNvParameterized(::NvParameterized::Interface &legacyObj, ::NvParameterized::Interface &obj)
	{
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
	virtual bool getNvParameterizedNames( const char ** names, uint32_t &outCount, uint32_t inCount) const = 0;

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
	virtual bool getNvParameterizedVersions(const char* className, uint32_t* versions, uint32_t &outCount, uint32_t inCount) const = 0;

	/**
	\brief Increment reference counter
	*/
	virtual int32_t incRefCount(int32_t *refCount) = 0;

	/**
	\brief Decrement reference counter
	*/
	virtual int32_t decRefCount(int32_t *refCount) = 0;

	/**
	\brief Called when inplace object is destroyed
	*/
	virtual void onInplaceObjectDestroyed(void * /*buf*/, ::NvParameterized::Interface * /*obj*/) {}

	/**
	\brief Called when all inplace objects are destroyed
	*/
	virtual void onAllInplaceObjectsDestroyed(void *buf) { free(buf); }

	/**
	\brief Allocate memory with default alignment of 8
	*/
	virtual void *alloc(uint32_t nbytes) = 0;

	/**
	\brief Allocate aligned memory
	*/
	virtual void *alloc(uint32_t nbytes, uint32_t align) = 0;

	/**
	\brief Deallocate memory
	*/
	virtual void free(void *buf) = 0;

	/**
	\brief Copy string
	*/
	virtual char *strdup(const char *str)
	{
		if( !str )
			return NULL;

		uint32_t strLen = (uint32_t)strlen(str) + 1;
		char *retStr = (char *)this->alloc(strLen, 1);
		
		PX_ASSERT( retStr );
		
		if( NULL != retStr )
#if PX_WINDOWS_FAMILY || PX_XBOXONE
			strcpy_s( retStr, strLen, str );
#else
			strncpy(retStr, str, strLen);
#endif
		return retStr;
	}

 	/**
	\brief Release copied string
	*/
	virtual void strfree(char *str)
	{
		if( NULL != str )
			this->free( str );
	}

 	/**
	\brief Warns user
	*/
	virtual void traitsWarn(const char * /*msg*/) const {}

	/**
	\brief Release Traits
	*/
	virtual void release(void) = 0;

	/**
	\brief Adapter for allocator classes in PxAlloctor.h
	*/
	class Allocator
	{
		::NvParameterized::Traits *mTraits;

	public:

		/**
		\brief Constructor
		*/
		Allocator(Traits *traits): mTraits(traits) {}

		/**
		\brief Allocate memory
		*/
		void *allocate(size_t size)
		{
			return allocate(size, __FILE__, __LINE__);
		}

		/**
		\brief Allocate memory
		*/
		void *allocate(size_t size, const char * /*filename*/, int /*line*/)
		{
			PX_ASSERT( static_cast<uint32_t>(size) == size );
			return mTraits->alloc(static_cast<uint32_t>(size));
		}

		/**
		\brief Release memory
		*/
		void deallocate(void *ptr)
		{
			return mTraits->free(ptr);
		}
	};
};


PX_POP_PACK

} // namespace NvParameterized

#endif // NV_PARAMETERIZED_TRAITS_H
