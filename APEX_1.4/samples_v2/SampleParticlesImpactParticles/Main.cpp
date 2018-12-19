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


#include "Utils.h"

#include <DirectXMath.h>
#include "XInput.h"
#include "DXUTMisc.h"
#include "DXUTCamera.h"


#include "ApexController.h"
#include "ApexRenderer.h"
#include "CommonUIController.h"
#include "SampleUIController.h"
#include "SampleSceneController.h"

#include "SampleManager.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	PX_UNUSED(hInstance);
	PX_UNUSED(hPrevInstance);
	PX_UNUSED(lpCmdLine);
	PX_UNUSED(nCmdShow);

// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	AllocConsole();
#endif

	SampleManager* sampleManager = new SampleManager(L"APEX Particles Sample: Impact Particles");

	CFirstPersonCamera camera;

	auto apexController = ApexController(PxDefaultSimulationFilterShader, &camera);
	auto apexRender = ApexRenderer(&camera, apexController);
	auto sceneController = SampleSceneController(&camera, apexController);
	auto commonUiController = CommonUIController(&camera, &apexRender, &apexController);
	auto sampleUIController = SampleUIController(&sceneController, &commonUiController);

	sampleManager->addControllerToFront(&apexController);
	sampleManager->addControllerToFront(&apexRender);
	sampleManager->addControllerToFront(&sceneController);
	sampleManager->addControllerToFront(&sampleUIController);
	sampleManager->addControllerToFront(&commonUiController);

	int result = sampleManager->run();

	delete sampleManager;

	return result;
}
