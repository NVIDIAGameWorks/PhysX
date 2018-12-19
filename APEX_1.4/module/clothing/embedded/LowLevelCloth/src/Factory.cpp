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

#include "SwFactory.h"

#if ENABLE_CUFACTORY
#include "CuFactory.h"
#endif

#if ENABLE_DXFACTORY
#include "windows/DxFactory.h"
//#include "PxGraphicsContextManager.h"
#pragma warning(disable : 4668 4917 4365 4061 4005)
#if PX_XBOXONE
#include <d3d11_x.h>
#else
#include <d3d11.h>
#endif
#endif

namespace nvidia
{
namespace cloth
{
uint32_t getNextFabricId()
{
	static uint32_t sNextFabricId = 0;
	return sNextFabricId++;
}
}
}

using namespace nvidia;

cloth::Factory* cloth::Factory::createFactory(Platform platform, void* contextManager)
{
	PX_UNUSED(contextManager);

	if(platform == Factory::CPU)
		return new SwFactory;

#if ENABLE_CUFACTORY
	if(platform == Factory::CUDA)
		return new CuFactory((PxCudaContextManager*)contextManager);
#endif

#if ENABLE_DXFACTORY
	if(platform == Factory::DirectCompute)
	{
		//physx::PxGraphicsContextManager* graphicsContextManager = (physx::PxGraphicsContextManager*)contextManager;
		//if(graphicsContextManager->getDevice()->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0)
		//	return new DxFactory(graphicsContextManager);
	}
#endif

	return 0;
}
