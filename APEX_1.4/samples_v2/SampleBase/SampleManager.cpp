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


#include "SampleManager.h"

#include "Utils.h"
#pragma warning(push)
#pragma warning(disable : 4917)
#pragma warning(disable : 4365)
#include "XInput.h"
#include "DXUTMisc.h"
#pragma warning(pop)


SampleManager::SampleManager(LPWSTR sampleName)
: mSampleName(sampleName)
{
	mDeviceManager = new DeviceManager();
}

void SampleManager::addControllerToFront(ISampleController* controller)
{
	mControllers.push_back(controller);
}

int SampleManager::run()
{
	// FirstPersonCamera uses this timer, without it it will be FPS-dependent
	DXUTGetGlobalTimer()->Start();

	for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		mDeviceManager->AddControllerToFront(*it);

	DeviceCreationParameters deviceParams;
	deviceParams.swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	deviceParams.swapChainSampleCount = 1;
	deviceParams.startFullscreen = false;
	deviceParams.backBufferWidth = 1280;
	deviceParams.backBufferHeight = 720;
#if defined(DEBUG) | defined(_DEBUG)
	deviceParams.createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	deviceParams.featureLevel = D3D_FEATURE_LEVEL_11_0;

	if (FAILED(mDeviceManager->CreateWindowDeviceAndSwapChain(deviceParams, mSampleName)))
	{
		MessageBox(nullptr, "Cannot initialize the D3D11 device with the requested parameters", "Error",
			MB_OK | MB_ICONERROR);
		return 1;
	}

	for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		(*it)->onInitialize();

	for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		(*it)->onSampleStart();

	mDeviceManager->SetVsyncEnabled(false);
	mDeviceManager->MessageLoop();

	for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		(*it)->onSampleStop();

	for (auto it = mControllers.begin(); it != mControllers.end(); it++)
		(*it)->onTerminate();

	mDeviceManager->Shutdown();
	delete mDeviceManager;

	return 0;
}
