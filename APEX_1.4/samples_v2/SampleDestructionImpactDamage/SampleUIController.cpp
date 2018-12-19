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
	TwDefine("Sample color='19 25 59' alpha=128 text=light size='380 200' iconified=false valueswidth=150 position='12 440' label='Setup Throw'");

	TwAddVarRO(sampleBar, "damageThreshold", TW_TYPE_FLOAT, mScene->getDamageThreshold(), "group='Destructible Parameters' min=0 max=100 step=0.1 help='The damage amount which will cause a chunk to fracture (break free) from the destructible.'");
	TwAddVarCB(sampleBar, "forceToDamage", TW_TYPE_FLOAT, SampleUIController::setForceToDamage,
		SampleUIController::getForceToDamage, this, "group='Destructible Parameters' min=0 max=1 step=0.001 help='Multiplier to calculate applied damage from an impact. applied damage = forceToDamage x impact force.'");
	TwAddVarCB(sampleBar, "damageCap", TW_TYPE_FLOAT, SampleUIController::setDamageCap,
		SampleUIController::getDamageCap, this, "group='Destructible Parameters' min=0 max=100 step=0.1  help='Limits the amount of damage applied to a chunk.'");
	TwAddVarRW(sampleBar, "cube velocity", TW_TYPE_FLOAT, &(mScene->getCubeVelocity()), "group='Throw Parameteres' min=10 max=1000 step=1 help='Thrown cube initial velocity.'");
	TwAddVarRW(sampleBar, "cube mass", TW_TYPE_FLOAT, &(mScene->getCubeMass()), "group='Throw Parameteres' min=0.01 max=1000 step=0.01 help='Thrown cube mass'");

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

	mCommonUIController->addHintLine("Throw cube - LMB");
}

LRESULT SampleUIController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PX_UNUSED(hWnd);
	PX_UNUSED(wParam);
	PX_UNUSED(lParam);

	if(uMsg == WM_LBUTTONDOWN)
	{
		mScene->throwCube();
	}

	return 1;
}

void SampleUIController::Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*)
{
	{
		char info[512];
		sprintf(info, "Last impact damage: %f", mScene->getLastImpactDamage());
		int tw, th;
		TwMeasureTextLine(info, &tw, &th);
		TwBeginText((int)mWidth - tw - 2, (int)mHeight - th - 2, 0, 0);
		TwAddTextLine(info, 0xFF00FF99, 0xFF000000);
		TwEndText();
	}

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

// forceToDamage

void TW_CALL SampleUIController::setForceToDamage(const void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleParameters params = controller->mScene->getCurrentActor()->getDestructibleParameters();
	params.forceToDamage = *static_cast<const float*>(value);
	controller->mScene->getCurrentActor()->setDestructibleParameters(params);

}

void TW_CALL SampleUIController::getForceToDamage(void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleParameters params = controller->mScene->getCurrentActor()->getDestructibleParameters();
	*static_cast<float*>(value) = params.forceToDamage;
}


// damageThreshold

void TW_CALL SampleUIController::setDamageThreshold(const void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleBehaviorGroupDesc desc;
	controller->mScene->getCurrentActor()->getBehaviorGroup(desc, 1);
	desc.damageThreshold = *static_cast<const float*>(value);

}

void TW_CALL SampleUIController::getDamageThreshold(void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleBehaviorGroupDesc desc;
	controller->mScene->getCurrentActor()->getBehaviorGroup(desc, 1);
	*static_cast<float*>(value) = desc.damageThreshold;
}


// damageCap

void TW_CALL SampleUIController::setDamageCap(const void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleParameters params = controller->mScene->getCurrentActor()->getDestructibleParameters();
	params.damageCap = *static_cast<const float*>(value);
	controller->mScene->getCurrentActor()->setDestructibleParameters(params);

}

void TW_CALL SampleUIController::getDamageCap(void* value, void* clientData)
{
	SampleUIController* controller = static_cast<SampleUIController*>(clientData);
	DestructibleParameters params = controller->mScene->getCurrentActor()->getDestructibleParameters();
	*static_cast<float*>(value) = params.damageCap;
}

