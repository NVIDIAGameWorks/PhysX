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


#include "SampleUIController.h"
#include "SampleSceneController.h"
#include "CommonUIController.h"


SampleUIController::SampleUIController(SampleSceneController* s, CommonUIController* c) : mScene(s), mCommonUIController(c)
{
}

void SampleUIController::onInitialize()
{
	TwBar* sampleBar = TwNewBar("Sample");
	PX_UNUSED(sampleBar);
	TwDefine("Sample color='19 25 59' alpha=128 text=light size='200 150' iconified=false valueswidth=150 position='12 480' label='Select Asset'");

	UINT assetsCount = (UINT)SampleSceneController::getAssetsCount();
	TwEnumVal* enumAssets = new TwEnumVal[assetsCount];
	for(UINT i = 0; i < assetsCount; i++)
	{
		enumAssets[i].Value = (int)i;
		enumAssets[i].Label = SampleSceneController::ASSETS[i].uiName;
	}
	TwType enumSceneType = TwDefineEnum("Assets", enumAssets, assetsCount);
	delete[] enumAssets;
	TwAddVarCB(sampleBar, "Assets", enumSceneType, SampleUIController::setCurrentScene,
	           SampleUIController::getCurrentScene, this, "group='Select Scene'");

	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eBODY_ANG_VELOCITY);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eBODY_LIN_VELOCITY);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eBODY_MASS_AXES);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_AABBS);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_SHAPES);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_AXES);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_COMPOUNDS);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_FNORMALS);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_EDGES);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_STATIC);
	mCommonUIController->addPhysXDebugRenderParam(PxVisualizationParameter::eCOLLISION_DYNAMIC);

	mCommonUIController->addApexDebugRenderParam("LodBenefits");
	mCommonUIController->addApexDebugRenderParam("VISUALIZE_DESTRUCTIBLE_SUPPORT", "Destructible", 1.0f, "DestructibleSupport");
	mCommonUIController->addApexDebugRenderParam("RenderNormals");
	mCommonUIController->addApexDebugRenderParam("RenderTangents");
	mCommonUIController->addApexDebugRenderParam("Bounds");

	mCommonUIController->addHintLine("Apply damage - LMB");
}

LRESULT SampleUIController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PX_UNUSED(hWnd);
	PX_UNUSED(wParam);
	PX_UNUSED(lParam);

	if(uMsg == WM_LBUTTONDOWN)
	{
		short mouseX = (short)LOWORD(lParam);
		short mouseY = (short)HIWORD(lParam);
		mScene->fire(mouseX / static_cast<float>(mWidth), mouseY / static_cast<float>(mHeight));
	}

	return 1;
}

void SampleUIController::BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	PX_UNUSED(pDevice);

	mWidth = pBackBufferSurfaceDesc->Width;
	mHeight = pBackBufferSurfaceDesc->Height;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												UI Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TW_CALL SampleUIController::setCurrentScene(const void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	controller->mScene->setCurrentAsset(*static_cast<const int*>(value));
}

void TW_CALL SampleUIController::getCurrentScene(void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	*static_cast<int*>(value) = controller->mScene->getCurrentAsset();
}
