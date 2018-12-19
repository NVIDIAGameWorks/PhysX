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


#ifndef PX_MODULE_DEFAULT_TRAITS_H
#define PX_MODULE_DEFAULT_TRAITS_H

// overload new and delete operators
#include "PsAllocator.h"

// NxParameterized headers
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"
#include "PxAssert.h"
#include "PxErrorCallback.h"

// NsFoundation and NvFoundation
#include "PsArray.h"
#include "PsAlignedMalloc.h"
#include "PxFoundation.h"

// system
#include <stdio.h>
#include <stdint.h>

namespace NvParameterized
{
	class DefaultTraits : public NvParameterized::Traits
	{
	public:
		struct BehaviourFlags
		{
			enum Enum
			{
				CLEAN_FACTORIES = 0x1 << 0,             ///< DefaultTraits is responsible destruct factories				
				CLEAN_CONVERTES = 0x1 << 1,             ///< DefaultTraits is responsible destruct converters
				COLLECT_ALLOCATE_OBJECTS    = 0x1 << 2, ///< Collect allocated NvParameterized::Interface objects for profiling
				WARN_IF_CONVERSION_HAPPENED = 0x1 << 3, ///< Emit warning if conversion happened
				

				DEFAULT_POLICY = CLEAN_FACTORIES | CLEAN_CONVERTES,
			};
		};

	protected:
		struct ConversionInfo
		{
			ConversionInfo(::NvParameterized::Conversion* theConv = 0, 
				const char * theClassName = 0,
				uint32_t theFrom = 0,
				uint32_t theTo = 0)
				: conv(theConv)
				, className(theClassName)
				, from(theFrom)
				, to(theTo)
			{}

			::NvParameterized::Conversion* conv;
			const char * className;
			uint32_t from;
			uint32_t to;

			bool operator == (const ConversionInfo& rhs) 
			{
				// ignore "conv" field
				return from == rhs.from && to == rhs.to && strcmp(className, rhs.className) == 0;
			}
		};

		physx::shdfnd::Array<NvParameterized::Factory*> factories;
		physx::shdfnd::Array<NvParameterized::Interface*> objects;
		physx::shdfnd::Array<ConversionInfo> converters;

		template <class T>
		static T removeAndReturn(physx::shdfnd::Array<T>& ctr, uint32_t index)
		{
			T res = ctr[index];
			ctr.remove(index);
			return res;
		}

		bool findLastFactoryIndexByClassName(uint32_t& index, const char* className) const {
			if (!className)
				return false;
			index = 0;
			bool findSomeFactory = false;
			for (uint32_t i = 0; i < factories.size(); ++i) {
				if (const char* factoryClassName = factories[i]->getClassName())
				{
					if (strcmp(factoryClassName, className) == 0) {
						if (!findSomeFactory || factories[i]->getVersion() > factories[index]->getVersion())
							index = i;
						findSomeFactory = true;
					}
				}
			}
			return findSomeFactory;
		}

		bool findFactoryIndexByClassNameAndVersion(uint32_t& index, const char* className, uint32_t version) const 
		{
			if (!className)
				return false;

			for (uint32_t i = 0; i < factories.size(); ++i) {
				if (const char* factoryClassName = factories[i]->getClassName())
				{
					if (strcmp(factoryClassName, className) == 0 && factories[i]->getVersion() == version) 
					{
						index = i;
						return true;
					}
				}
			}
			return false;
		}

		void unregisterAllFactories() 
		{
			if ((behaviourPolicy & BehaviourFlags::CLEAN_FACTORIES) == 0)
				return;

			for (uint32_t i = 0; i < factories.size(); ++i) 
			{
					factories[i]->freeParameterDefinitionTable(this);
					delete factories[i];
			}
			factories.clear();
		}

		void unregisterAllConverters() 
		{
			if ((behaviourPolicy & BehaviourFlags::CLEAN_CONVERTES) == 0)
				return;

			for (uint32_t i = 0; i < converters.size(); ++i) 
			{
				converters[i].conv->release();
				converters[i].conv = 0;
			}
			converters.clear();
		}

	protected:
		uint32_t behaviourPolicy;

	public:

		DefaultTraits(uint32_t behaviourFlags)
		: behaviourPolicy(behaviourFlags)
		{
		}

		virtual ~DefaultTraits()
		{
			PX_ASSERT(objects.size() == 0);
			unregisterAllFactories();
			unregisterAllConverters();
		}

		virtual void registerFactory(::NvParameterized::Factory& factory) 
		{
			if (!doesFactoryExist(factory.getClassName(), factory.getVersion())) 
			{
				factories.pushBack(&factory);
			} 
			else
			{
				PX_ASSERT_WITH_MESSAGE(0, "Factory has already exist");
			}
		}

		virtual ::NvParameterized::Factory *removeFactory(const char * className) 
		{
			uint32_t index = 0;
			if (!findLastFactoryIndexByClassName(index, className))
				return NULL;
			
			::NvParameterized::Factory* res = removeAndReturn(factories, index);
			return res;
		}

		virtual ::NvParameterized::Factory *removeFactory(const char * className, uint32_t version) 
		{
			uint32_t index = 0;
			if (!findFactoryIndexByClassNameAndVersion(index, className, version))
				return NULL;

			::NvParameterized::Factory* res = removeAndReturn(factories, index);
			return res;
		}

		virtual bool doesFactoryExist(const char* className)
		{
			uint32_t index = 0;
			return findLastFactoryIndexByClassName(index, className);
		}

		virtual bool doesFactoryExist(const char* className, uint32_t version) 
		{
			uint32_t index = 0;
			return findFactoryIndexByClassNameAndVersion(index, className, version);
		}

		virtual ::NvParameterized::Interface* createNvParameterized(const char * name) 
		{
			uint32_t index = 0;
			if (findLastFactoryIndexByClassName(index, name))
			{
				::NvParameterized::Interface* result = factories[index]->create(this);
				if (behaviourPolicy & BehaviourFlags::COLLECT_ALLOCATE_OBJECTS) 
				{
					objects.pushBack(result);
				}
				return result;
			}
			return NULL;
		}

		virtual ::NvParameterized::Interface * createNvParameterized(const char * name, uint32_t ver) 
		{
			uint32_t index = 0;
			if (!findFactoryIndexByClassNameAndVersion(index, name, ver))
				return NULL;
			::NvParameterized::Interface* result = factories[index]->create(this);

			if (behaviourPolicy & BehaviourFlags::COLLECT_ALLOCATE_OBJECTS) 
			{
				objects.pushBack(result);
			}
			return result;
		}

		virtual ::NvParameterized::Interface * finishNvParameterized(const char * name, void *obj, void *buf, int32_t *refCount) 
		{
			uint32_t index = 0;
			if (!findLastFactoryIndexByClassName(index, name))
				return 0;
			return factories[index]->finish(this, obj, buf, refCount);
		}

		virtual ::NvParameterized::Interface * finishNvParameterized( const char * name, uint32_t ver, void *obj, void *buf, int32_t *refCount ) 
		{
			uint32_t index = 0;
			if (!findFactoryIndexByClassNameAndVersion(index, name, ver))
				return 0;
			return factories[index]->finish(this, obj, buf, refCount);
		}

		virtual uint32_t getCurrentVersion(const char *className) const {
			uint32_t index = 0;
			if (!findLastFactoryIndexByClassName(index, className))			
				return 0;
			return factories[index]->getVersion();
		}

		virtual uint32_t getAlignment(const char *className, uint32_t classVersion) const 
		{
			uint32_t index = 0;
			if (!findFactoryIndexByClassNameAndVersion(index, className, classVersion))
				return uint32_t(1);
			return factories[index]->getAlignment();
		}

		virtual bool getNvParameterizedNames( const char ** names, uint32_t &outCount, uint32_t inCount) const 
		{
			uint32_t currentOutSize = 0;
			outCount = 0;
			for (uint32_t i = 0; i < factories.size(); ++i) 
			{
				const char* curClassName = factories[i]->getClassName();
				bool foundClassName = false;
				for (uint32_t j = 0; j < currentOutSize; ++j) {
					if (strcmp(names[j], curClassName) == 0)
					{
						foundClassName = true;
						break;
					}
				}

				if (foundClassName)
					continue;

				if (outCount < inCount)
					names[currentOutSize++] = factories[i]->getClassName();

				outCount++;
			}

			return outCount <= inCount;
		}

		virtual bool getNvParameterizedVersions(const char* className, uint32_t* versions, uint32_t &outCount, uint32_t inCount) const 
		{
			outCount = 0;
			for (uint32_t i = 0; i < factories.size(); ++i) {
				if (strcmp(factories[i]->getClassName(), className) == 0)
				{
					if (outCount < inCount)
						versions[outCount] = factories[i]->getVersion();
					outCount++;
				}
			}
			return outCount <= inCount;
		}


		virtual void registerConversion(const char * className, uint32_t from, uint32_t to, ::NvParameterized::Conversion & conv) 
		{
			ConversionInfo convInfo(&conv, className, from, to);
			for (uint32_t i = 0; i < converters.size(); ++i) 
			{
				if (converters[i] == convInfo)
				{
					PX_ASSERT_WITH_MESSAGE(false, "Conversion has already been registered");
					return;
				}
			}
			converters.pushBack(convInfo);
		}

		virtual ::NvParameterized::Conversion *removeConversion(const char * className, uint32_t from, uint32_t to) 
		{
			ConversionInfo convInfo(NULL, className, from, to);
			for (uint32_t i = 0; i < converters.size(); ++i) {
				if (converters[i] == convInfo)
					return removeAndReturn(converters, i).conv;
			}
			return NULL; 
		}

		virtual bool updateLegacyNvParameterized(::NvParameterized::Interface &legacyObj, ::NvParameterized::Interface &obj)
		{
			// updateLegacyNvParameterized should not destroy legacyObj and obj. The code before this stackframe was responsible for it
			if (legacyObj.version() > obj.version())
			{
				PX_ASSERT_WITH_MESSAGE(false, "Downgrade is not permited");
				return false;
			}

			if (legacyObj.version() == obj.version())
			{
				PX_ASSERT_WITH_MESSAGE(false, "Object to upgrade already up to date version");
				return false;
			}

			const char* legacyClassName = legacyObj.className();
			const char* newClassName = obj.className();
			if (!legacyClassName || !newClassName)
			{
				PX_ASSERT_WITH_MESSAGE(false, "updateLegacyNvParameterized get empty class names");
			}

			::NvParameterized::Interface* prevObj = &legacyObj;
			::NvParameterized::Interface* nextObj = NULL;
			bool res = true;

			for (;prevObj->version() != obj.version() && res;) 
			{
				// TODO: Here is the problem:
				// We store our version in 32 bits - higher 16 bits store major version, lower 16 bits store minor version.
				// If we agree on the fact that there are only 10 minor version (0-9) within one 1 major version, then this code should behave correctly
				// otherwise there will be problems
				const uint32_t version = prevObj->version();
				uint32_t nextVersion = 0;
				if ((version & 0xFFFF) == 9)
				{
					nextVersion = (((version & 0xFFFF0000) >> 16) + 1) << 16;
				}
				else
				{
					nextVersion = version + 1;
				}
				bool findConverter = false;

				for (uint32_t i = 0; i < converters.size(); ++i) 
				{
					if (strcmp(converters[i].className, legacyObj.className()) == 0 && converters[i].from == version) 
					{
						if (converters[i].to == nextVersion || converters[i].to == version + 1)
						{
							if (converters[i].to == version + 1)
							{
								nextVersion = version + 1;
							}
							findConverter = true;

							if (nextVersion == obj.version())					
								nextObj = &obj;
							else
								nextObj = createNvParameterized(legacyObj.className(), nextVersion);

							if (behaviourPolicy & BehaviourFlags::WARN_IF_CONVERSION_HAPPENED)
							{
								char buff[512] = {0};
								sprintf(buff, "Conversion %s,%i=>%i", legacyObj.className(), int(version), int(nextVersion));
								traitsWarn(buff);
							}

							if (!(*converters[i].conv)(*prevObj, *nextObj))
							{
								res = false;
								break;;
							}

							if (prevObj != &legacyObj && prevObj != &obj)
							{
								prevObj->destroy();
							}
							prevObj = nextObj;
							nextObj = NULL;
						}
					}
				}

				if (!findConverter)
				{
					char buff[512] = {0};
					sprintf(buff, "Needed conversion routine doesn't exist %s,%i=>%i !", legacyObj.className(), int(version), int(nextVersion));
					PX_ASSERT_WITH_MESSAGE(false, buff);

					break;
				}
			}

			if (prevObj && prevObj != &legacyObj && prevObj != &obj)
				prevObj->destroy();

			if (nextObj && nextObj != &legacyObj && nextObj != &obj)			
				nextObj->destroy();

			PX_ASSERT_WITH_MESSAGE(res, "Failed to upgrade NvParameterized::Interface");

			return res;
		}

		virtual int32_t incRefCount(int32_t *refCount) 
		{
			(*refCount)++;
			return *refCount;
		}

		virtual int32_t decRefCount(int32_t *refCount)
		{
			(*refCount)--;
			return *refCount;
		}

		virtual void *alloc(uint32_t nbytes) 
		{
			return alloc(nbytes, 16);
		}

		virtual void *alloc(uint32_t nbytes, uint32_t align) 
		{
			if (align <= 16)
			{
				return physx::shdfnd::AlignedAllocator<16>().allocate(nbytes, __FILE__, __LINE__);			
			}
			else
			{
				switch (align){
				case 32:
					return physx::shdfnd::AlignedAllocator<32>().allocate(nbytes, __FILE__, __LINE__);			
				case 64:
					return physx::shdfnd::AlignedAllocator<64>().allocate(nbytes, __FILE__, __LINE__);			
				case 128:
					return physx::shdfnd::AlignedAllocator<128>().allocate(nbytes, __FILE__, __LINE__);			
				default:
					PX_ASSERT_WITH_MESSAGE(false, "Unsupported alignment");
					return 0;
				}
			}
		}

		virtual void free(void *buf) 
		{
			physx::shdfnd::AlignedAllocator<16>().deallocate(buf);

			if (behaviourPolicy & BehaviourFlags::COLLECT_ALLOCATE_OBJECTS) 
			{
				// Try to find in objects
				for (uint32_t i = 0; i < objects.size(); ++i) 
				{
					if (objects[i] == reinterpret_cast<NvParameterized::Interface*>(buf))
					{

						objects.remove(i);
						break;
					}
				}
			}
		}

		virtual void traitsWarn(const char * msg) const 
		{
			PxGetFoundation().getErrorCallback().reportError(physx::PxErrorCode::eDEBUG_WARNING, msg, __FILE__, __LINE__);
		}

		virtual void release(void) 
		{
			delete this;
		}
	};
}

#endif
