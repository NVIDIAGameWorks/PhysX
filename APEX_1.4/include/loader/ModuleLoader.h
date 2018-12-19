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



#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "Apex.h"
#include "PsHashMap.h"

namespace nvidia
{
namespace apex
{

/**
\brief The ModuleLoader is a utility class for loading APEX modules

\note The most useful methods for rapid integration are "loadAllModules()",
      "loadAllLegacyModules()", and "releaseLoadedModules()".

\note If you need to release APEX modules loaded with ModuleLoader you should use one of
      the release-methods provided below. Note that you may never need it because SDK automatically
	  releases all modules when program ends.
*/
class ModuleLoader : public Module
{
public:
	/// \brief Load and initialize a specific APEX module
	virtual Module* loadModule(const char* name) = 0;

	/**
	\brief Load and initialize a list of specific APEX modules
	\param [in] names	The names of modules to load
	\param [in] size 	Number of modules to load
	\param [in] modules The modules array must be the same size as the names array to
	                    support storage of every loaded Module pointer. Use NULL if
	                    you do not need the list of created modules.
	 */
	virtual void loadModules(const char** names, uint32_t size, Module** modules = 0) = 0;

	/// \brief Load and initialize all APEX modules
	virtual void loadAllModules() = 0;

	/// \brief Load and initialize all legacy APEX modules (useful for deserializing legacy assets)
	virtual void loadAllLegacyModules() = 0;

	/// \brief Returns the number of loaded APEX modules
	virtual uint32_t getLoadedModuleCount() const = 0;

	/// \brief Returns the APEX module specified by the index if it was loaded by this ModuleLoader
	virtual Module* getLoadedModule(uint32_t idx) const = 0;

	/// \brief Returns the APEX module specified by the name if it was loaded by this ModuleLoader
	virtual Module* getLoadedModule(const char* name) const = 0;

	/// \brief Releases the APEX module specified by the index if it was loaded by this ModuleLoader
	virtual void releaseModule(uint32_t idx) = 0;

	/// \brief Releases the APEX module specified by the name if it was loaded by this ModuleLoader
	virtual void releaseModule(const char* name) = 0;

	/// \brief Releases the APEX module specified by the index if it was loaded by this ModuleLoader
	virtual void releaseModules(Module** modules, uint32_t size) = 0;

	/**
	\brief Releases the specified APEX module
	\note If the ModuleLoader is used to load modules, this method must be used to
	      release individual modules.
	      Do not use the Module::release() method.
	*/
	virtual void releaseModule(Module* module) = 0;

	/**
	\brief Releases the APEX modules specified in the names list
	\note If the ModuleLoader is used to load modules, this method must be used to
	      release individual modules.
	      Do not use the Module::release() method.
	*/
	virtual void releaseModules(const char** names, uint32_t size) = 0;

	/// Releases all APEX modules loaded by this ModuleLoader
	virtual void releaseLoadedModules() = 0;

	/// ModuleNameErrorMap
	typedef shdfnd::HashMap<const char*, nvidia::apex::ApexCreateError> ModuleNameErrorMap;
	
	/// Returns ModuleNameErrorMap
	virtual const ModuleNameErrorMap& getLoadedModulesErrors() const = 0;
};

#if !defined(_USRDLL)
/**
* If this module is distributed as a static library, the user must call this
* function before calling ApexSDK::createModule("Loader")
*/
void instantiateModuleLoader();
#endif

}
} // end namespace nvidia::apex

#endif // MODULE_LOADER_H
