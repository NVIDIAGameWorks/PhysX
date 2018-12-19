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



#ifndef MODULEUPDATELOADER_H
#define MODULEUPDATELOADER_H

#ifdef WIN32

#if PX_X64
#define UPDATE_LOADER_DLL_NAME "PhysXUpdateLoader64.dll"
#else
#define UPDATE_LOADER_DLL_NAME "PhysXUpdateLoader.dll"
#endif

// This GUID should change any time we release APEX with a public interface change.
#if PX_PHYSICS_VERSION_MAJOR == 0
#define DEFAULT_APP_GUID "165F143C-15CB-47FA-ACE3-2002B3684026"
#else
#define DEFAULT_APP_GUID "1AE7180B-79E5-4234-91A7-E387331B5993"
#endif

//#include "PsWindowsInclude.h"
#include <windows.h>

class ModuleUpdateLoader
{
public:
	ModuleUpdateLoader(const char* updateLoaderDllName);
	~ModuleUpdateLoader();

	// Loads the given module through the update loader. Loads it from the path if 
	// the update loader doesn't find the requested module. Returns NULL if no
	// module found.
	HMODULE loadModule(const char* moduleName, const char* appGuid);

protected:
	HMODULE mUpdateLoaderDllHandle;
	FARPROC mGetUpdatedModuleFunc;

	// unit test fixture
	friend class ModuleUpdateLoaderTest;
};

#endif	// WIN32
#endif	// MODULEUPDATELOADER_H
