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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PhysXIndicator.h"
#include "AgMMFile.h"

#include <windows.h>
#if _MSC_VER >= 1800
#include <VersionHelpers.h>	// for IsWindowsVistaOrGreater
#endif
#include <stdio.h>

using namespace nvidia;
using namespace physx;

// Scope-based to indicate to NV driver that CPU PhysX is happening
PhysXCpuIndicator::PhysXCpuIndicator() :
	mPhysXDataPtr(NULL)
{
	bool alreadyExists = false;
	
	mPhysXDataPtr = (physx::NvPhysXToDrv_Data_V1*)PhysXCpuIndicator::createIndicatorBlock(mSharedMemConfig, alreadyExists);
	
	if (!mPhysXDataPtr)
	{
		return;
	}
	
	if (!alreadyExists)
	{
		mPhysXDataPtr->bGpuPhysicsPresent = 0;
		mPhysXDataPtr->bCpuPhysicsPresent = 1;
	}
	else
	{
		mPhysXDataPtr->bCpuPhysicsPresent++;
	}

	// init header last to prevent race conditions
	// this must be done because the driver may have already created the shared memory block,
	// thus alreadyExists may be true, even if PhysX hasn't been initialized
	NvPhysXToDrv_Header_Init(mPhysXDataPtr->header);
}

PhysXCpuIndicator::~PhysXCpuIndicator()
{
	if (!mPhysXDataPtr)
	{
		return;
	}
	
	mPhysXDataPtr->bCpuPhysicsPresent--;
	
	mPhysXDataPtr = NULL;
	mSharedMemConfig.destroy();
}

void* PhysXCpuIndicator::createIndicatorBlock(AgMMFile &mmfile, bool &alreadyExists)
{
	char configName[128];

    // Get the windows version (we can only create Global\\ namespace objects in XP)
#if _MSC_VER >= 1800
	// Windows 8.1 SDK, which comes with VS2013, deprecated the GetVersionEx function
	// Windows 8.1 SDK added the IsWindowsVistaOrGreater helper function which we use instead
	BOOL bIsVistaOrGreater = IsWindowsVistaOrGreater();
#else
	OSVERSIONINFOEX windowsVersionInfo;

	/**
		Operating system		Version number
		----------------		--------------
		Windows 7				6.1
		Windows Server 2008 R2	6.1
		Windows Server 2008		6.0
		Windows Vista			6.0
		Windows Server 2003 R2	5.2
		Windows Server 2003		5.2
		Windows XP				5.1
		Windows 2000			5.0
	**/
	windowsVersionInfo.dwOSVersionInfoSize = sizeof(windowsVersionInfo);
	GetVersionEx((LPOSVERSIONINFO)&windowsVersionInfo);

	bool bIsVistaOrGreater = (windowsVersionInfo.dwMajorVersion >= 6);
#endif

	if (bIsVistaOrGreater)
	{
		NvPhysXToDrv_Build_SectionName((uint32_t)GetCurrentProcessId(), configName);
	}
	else
	{
		NvPhysXToDrv_Build_SectionNameXP((uint32_t)GetCurrentProcessId(), configName);
	}

	mmfile.create(configName, sizeof(physx::NvPhysXToDrv_Data_V1), alreadyExists);

	return mmfile.getAddr();
}

//-----------------------------------------------------------------------------------------------------------

PhysXGpuIndicator::PhysXGpuIndicator() :
	mPhysXDataPtr(NULL),
	mAlreadyExists(false),
	mGpuTrigger(false)
{
	mPhysXDataPtr = (physx::NvPhysXToDrv_Data_V1*)PhysXCpuIndicator::createIndicatorBlock(mSharedMemConfig, mAlreadyExists);
	
	// init header last to prevent race conditions
	// this must be done because the driver may have already created the shared memory block,
	// thus alreadyExists may be true, even if PhysX hasn't been initialized
	NvPhysXToDrv_Header_Init(mPhysXDataPtr->header);
}

PhysXGpuIndicator::~PhysXGpuIndicator()
{
	gpuOff();
	
	mPhysXDataPtr = NULL;
	mSharedMemConfig.destroy();
}

// Explicit set functions to indicate to NV driver that GPU PhysX is happening
void PhysXGpuIndicator::gpuOn()
{
	if (!mPhysXDataPtr || mGpuTrigger)
	{
		return;
	}

	if (!mAlreadyExists)
	{
		mPhysXDataPtr->bGpuPhysicsPresent = 1;
		mPhysXDataPtr->bCpuPhysicsPresent = 0;
	}
	else
	{
		mPhysXDataPtr->bGpuPhysicsPresent++;
	}

	mGpuTrigger = true;
}

void PhysXGpuIndicator::gpuOff()
{
	if (!mPhysXDataPtr || !mGpuTrigger)
	{
		return;
	}

	mPhysXDataPtr->bGpuPhysicsPresent--;

	mGpuTrigger = false;
}
