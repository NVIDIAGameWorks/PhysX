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


#include "CommonUIController.h"

#include <DirectXMath.h>

#include "XInput.h"
#include "DXUTMisc.h"
#pragma warning(push)
#pragma warning(disable : 4481) // Suppress "nonstandard extension used" warning
#include "DXUTCamera.h"
#pragma warning(pop)

#include "ApexRenderer.h"
#include "ApexController.h"

using namespace std;

CommonUIController::CommonUIController(CFirstPersonCamera* cam, ApexRenderer* r, ApexController* a)
: mCamera(cam)
, mApexRenderer(r)
, mApexController(a)
, mShowHint(false)
{
	mHintOffLines.push_back("Press F1 to toggle hint");
}

HRESULT CommonUIController::DeviceCreated(ID3D11Device* pDevice)
{
	TwInit(TW_DIRECT3D11, pDevice);
	TwDefine("GLOBAL fontstyle=fixed contained=true");

	mSettingsBar = TwNewBar("Settings");
	TwDefine(
		"Settings color='19 25 19' alpha=128 text=light size='380 350' iconified=true valueswidth=65 position='12 80'");

	TwAddVarCB(mSettingsBar, "WireFrame", TW_TYPE_BOOLCPP, CommonUIController::setWireframeEnabled,
		CommonUIController::getWireframeEnabled, this, "group=Main key=o");
	TwAddButton(mSettingsBar, "Reload Shaders", CommonUIController::onReloadShadersButton, this, "group=Main key=f5");
	TwAddVarCB(mSettingsBar, "Fixed Sim Frequency Enabled", TW_TYPE_BOOLCPP, CommonUIController::setFixedTimestepEnabled,
		CommonUIController::getFixedTimestepEnabled, this, "group=Main key=t");
	TwAddVarCB(mSettingsBar, "Fixed Sim Frequency", TW_TYPE_UINT32, CommonUIController::setFixedSimFrequency,
		CommonUIController::getFixedSimFrequency, this, "group=Main min=30 max=1000");
	TwAddVarRW(mSettingsBar, "Ambient Color", TW_TYPE_COLOR3F, &(mApexRenderer->getAmbientColor()), "group='Scene'");
	TwAddVarRW(mSettingsBar, "Point Light Color", TW_TYPE_COLOR3F, &(mApexRenderer->getPointLightColor()), "group='Scene'");
	TwAddVarRW(mSettingsBar, "Point Light Pos", TW_TYPE_DIR3F, &(mApexRenderer->getPointLightPos()), "group='Scene'");
	TwAddVarRW(mSettingsBar, "Dir Light Color", TW_TYPE_COLOR3F, &(mApexRenderer->getDirLightColor()), "group='Scene'");
	TwAddVarRW(mSettingsBar, "Dir Light Dir", TW_TYPE_DIR3F, &(mApexRenderer->getDirLightDir()), "group='Scene'");
	TwAddVarRW(mSettingsBar, "Specular Power", TW_TYPE_FLOAT, &(mApexRenderer->getSpecularPower()), "group='Scene' min=1 max=500 step=1");
	TwAddVarRW(mSettingsBar, "Specular Intensity", TW_TYPE_FLOAT, &(mApexRenderer->getSpecularIntensity()), "group='Scene' min=0 max=2 step=0.05");

	addPhysXDebugRenderParam(PxVisualizationParameter::eWORLD_AXES);
	addPhysXDebugRenderParam(PxVisualizationParameter::eBODY_AXES);
	addPhysXDebugRenderParam(PxVisualizationParameter::eACTOR_AXES);

	addHintLine("Rotate camera - RMB");
	addHintLine("Move camera - WASDQE(SHIFT)");
	addHintLine("Play/Pause - P");
	addHintLine("Fixed/Variable sim freq - T");
	addHintLine("Reload shaders - F5");
	addHintLine("Wireframe - O");
	

	toggleCameraSpeed(false);

	return S_OK;
}

void CommonUIController::DeviceDestroyed()
{
	TwTerminate();

	for (std::list<IDebugRenderParam*>::iterator it = mDebugRenderParams.begin(); it != mDebugRenderParams.end(); it++)
	{
		delete (*it);
	}
	mDebugRenderParams.clear();
}

LRESULT CommonUIController::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PX_UNUSED(hWnd);
	PX_UNUSED(wParam);
	PX_UNUSED(lParam);

	if(uMsg == WM_KEYDOWN || uMsg == WM_KEYUP)
	{
		// Camera overspeed event
		int iKeyPressed = static_cast<int>(wParam);
		if(iKeyPressed == VK_SHIFT)
		{
			toggleCameraSpeed(uMsg == WM_KEYDOWN);
		}

		// Play/Pause
		if (iKeyPressed == 'P' && uMsg == WM_KEYDOWN)
		{
			mApexController->togglePlayPause();
		}

		// Hint show/hide
		if (iKeyPressed == VK_F1 && uMsg == WM_KEYDOWN)
		{
			mShowHint = !mShowHint;
		}
	}

	// TW events capture
	if(TwEventWin(hWnd, uMsg, wParam, lParam))
	{
		return 0; // Event has been handled by AntTweakBar
	}

	// Camera events
	mCamera->HandleMessages(hWnd, uMsg, wParam, lParam);

	return 1;
}

void CommonUIController::Animate(double fElapsedTimeSeconds)
{
	mCamera->FrameMove((float)fElapsedTimeSeconds);
}

void CommonUIController::Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*)
{
	// Render stats text
	TwBeginText(2, 0, 0, 0);
	char msg[256];
	double averageTime = GetDeviceManager()->GetAverageFrameTime();
	double fps = (averageTime > 0) ? 1.0 / averageTime : 0.0;
	sprintf_s(msg, "FPS: %.1f", fps);
	TwAddTextLine(msg, 0xFF9BD839, 0xFF000000);
	sprintf_s(msg, "Simulation Time: %.5fs ", mApexController->getLastSimulationTime());
	TwAddTextLine(msg, 0xFF9BD839, 0xFF000000);
	TwEndText();

	// Render hint text
	list<string> lines = mShowHint ? mHintOnLines : mHintOffLines;
	uint32_t lineNum = 0;
	for (list<string>::iterator it = lines.begin(); it != lines.end(); it++)
	{
		int tw, th;
		TwMeasureTextLine(it->c_str(), &tw, &th);
		TwBeginText((int32_t)mWidth - tw - 2, th * (int32_t)lineNum, 0, 0);
		TwAddTextLine(it->c_str(), 0xFFFFFF99, 0xFF000000);
		TwEndText();

		lineNum++;
	}

	TwDraw();
}

void CommonUIController::BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	PX_UNUSED(pDevice);

	mWidth = pBackBufferSurfaceDesc->Width;
	mHeight = pBackBufferSurfaceDesc->Height;

	TwWindowSize((int32_t)mWidth, (int32_t)mHeight);
}

void CommonUIController::toggleCameraSpeed(bool overspeed)
{
	mCamera->SetScalers(0.002f, overspeed ? 150.f : 25.f);
}

void CommonUIController::addHintLine(std::string hintLine)
{
	mHintOnLines.push_back(hintLine);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												  UI Callbacks
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TW_CALL CommonUIController::setDebugRenderParam(const void* value, void* clientData)
{
	IDebugRenderParam* param = static_cast<IDebugRenderParam*>(clientData);
	param->setParamEnabled(*static_cast<const bool*>(value));
}

void TW_CALL CommonUIController::getDebugRenderParam(void* value, void* clientData)
{
	IDebugRenderParam* param = static_cast<IDebugRenderParam*>(clientData);
	*static_cast<bool*>(value) = param->isParamEnabled();
}


void TW_CALL CommonUIController::setWireframeEnabled(const void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	controller->mApexRenderer->setWireframeMode(*static_cast<const bool*>(value));
}

void TW_CALL CommonUIController::getWireframeEnabled(void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	*static_cast<bool*>(value) = controller->mApexRenderer->getWireframeMode();
}


void TW_CALL CommonUIController::setFixedTimestepEnabled(const void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	const bool enable = *static_cast<const bool*>(value);
	if (controller->mApexController->usingFixedTimestep() != enable)
	{
		controller->mApexController->toggleFixedTimestep();
	}
}

void TW_CALL CommonUIController::getFixedTimestepEnabled(void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	*static_cast<bool*>(value) = controller->mApexController->usingFixedTimestep();
}


void TW_CALL CommonUIController::setFixedSimFrequency(const void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	controller->mApexController->setFixedTimestep(1.0 / *static_cast<const uint32_t*>(value));
}

void TW_CALL CommonUIController::getFixedSimFrequency(void* value, void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	*static_cast<uint32_t*>(value) = static_cast<uint32_t>(1.0 / controller->mApexController->getFixedTimestep() + 0.5);
}


void TW_CALL CommonUIController::onReloadShadersButton(void* clientData)
{
	CommonUIController* controller = static_cast<CommonUIController*>(clientData);
	controller->mApexRenderer->reloadShaders();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//											  Debug Render Params
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool CommonUIController::ApexDebugRenderParam::isParamEnabled()
{
	Scene* scene = mController->mApexController->getApexScene();

	NvParameterized::Interface* debugRenderInterface = scene->getDebugRenderParams();
	if (!mModule.empty())
	{
		debugRenderInterface = scene->getModuleDebugRenderParams(mModule.c_str());
	}

	NvParameterized::Handle handle(*debugRenderInterface, mName.c_str());
	PX_ASSERT(handle.isValid());

	if (handle.parameterDefinition()->type() == NvParameterized::TYPE_F32)
	{
		float v;
		handle.getParamF32(v);
		return v == mValue;
	}
	else if (handle.parameterDefinition()->type() == NvParameterized::TYPE_U32)
	{
		uint32_t v; 
		handle.getParamU32(v);
		return v == uint32_t(mValue);
	}
	else if (handle.parameterDefinition()->type() == NvParameterized::TYPE_BOOL)
	{
		bool v;
		handle.getParamBool(v);
		return v;
	}
	else
	{
		PX_ALWAYS_ASSERT();
		return false;
	}
}

void CommonUIController::ApexDebugRenderParam::setParamEnabled(bool enabled)
{
	Scene* scene = mController->mApexController->getApexScene();

	NvParameterized::Interface* debugRenderInterface = scene->getDebugRenderParams();
	NvParameterized::setParamBool(*debugRenderInterface, "Enable", true);
	NvParameterized::setParamF32(*debugRenderInterface, "Scale", 1.0f);

	if (!mModule.empty())
	{
		debugRenderInterface = scene->getModuleDebugRenderParams(mModule.c_str());
	}

	NvParameterized::Handle handle(*debugRenderInterface, mName.c_str());
	PX_ASSERT(handle.isValid());

	if (handle.parameterDefinition()->type() == NvParameterized::TYPE_F32)
	{
		handle.setParamF32(enabled ? mValue : 0);
	}
	else if (handle.parameterDefinition()->type() == NvParameterized::TYPE_U32)
	{
		handle.setParamU32(mValue ? uint32_t(mValue) : 0);
	}
	else if (handle.parameterDefinition()->type() == NvParameterized::TYPE_BOOL)
	{
		handle.setParamBool(enabled);
	}
}

bool CommonUIController::PhysXDebugRenderParam::isParamEnabled()
{
	Scene* scene = mController->mApexController->getApexScene();
	return scene->getPhysXScene()->getVisualizationParameter(mParameter) != 0;
}

void CommonUIController::PhysXDebugRenderParam::setParamEnabled(bool enabled)
{
	Scene* scene = mController->mApexController->getApexScene();
	scene->getPhysXScene()->setVisualizationParameter(mParameter, enabled ? 1.0f : 0.0f);
}

static const char* getUINameForPhysXParam(physx::PxVisualizationParameter::Enum param)
{
	using physx::PxVisualizationParameter;

	switch (param)
	{
	case PxVisualizationParameter::eSCALE:
		return "Scale";
	case PxVisualizationParameter::eBODY_AXES:
		return "Body Axes";
	case PxVisualizationParameter::eWORLD_AXES:
		return "World Axes";
	case PxVisualizationParameter::eBODY_MASS_AXES:
		return "Body Mass Axes";
	case PxVisualizationParameter::eBODY_LIN_VELOCITY:
		return "Body Lin Velocity";
	case PxVisualizationParameter::eBODY_ANG_VELOCITY:
		return "Body Ang Velocity";
	case PxVisualizationParameter::eDEPRECATED_BODY_JOINT_GROUPS:
		return "Body Joint (Deprecated)";
	case PxVisualizationParameter::eCONTACT_POINT:
		return "Contact Point";
	case PxVisualizationParameter::eCONTACT_NORMAL:
		return "Contact Normal";
	case PxVisualizationParameter::eCONTACT_ERROR:
		return "Contact Error";
	case PxVisualizationParameter::eCONTACT_FORCE:
		return "Contact Force";
	case PxVisualizationParameter::eACTOR_AXES:
		return "Actor Axes";
	case PxVisualizationParameter::eCOLLISION_AABBS:
		return "Collision AABBs";
	case PxVisualizationParameter::eCOLLISION_SHAPES:
		return "Collision Shapes";
	case PxVisualizationParameter::eCOLLISION_AXES:
		return "Collision Axes";
	case PxVisualizationParameter::eCOLLISION_COMPOUNDS:
		return "Collision Compounds";
	case PxVisualizationParameter::eCOLLISION_FNORMALS:
		return "Collision FNormals";
	case PxVisualizationParameter::eCOLLISION_EDGES:
		return "Collision Edges";
	case PxVisualizationParameter::eCOLLISION_STATIC:
		return "Collision Static";
	case PxVisualizationParameter::eCOLLISION_DYNAMIC:
		return "Collision Dynamic";
	case PxVisualizationParameter::eJOINT_LOCAL_FRAMES:
		return "Joint Local Frames";
	case PxVisualizationParameter::eJOINT_LIMITS:
		return "Joint Limits";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_POSITION:
		return "PS Position";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_VELOCITY:
		return "PS Velocity";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_COLLISION_NORMAL:
		return "PS Collision Normal";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_BOUNDS:
		return "PS Bounds";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_GRID:
		return "PS Grid";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_BROADPHASE_BOUNDS:
		return "PS Broadphase Bounds";
	case PxVisualizationParameter::ePARTICLE_SYSTEM_MAX_MOTION_DISTANCE:
		return "PS Max Motion Distance";
	case PxVisualizationParameter::eCULL_BOX:
		return "Cull Box";
	case PxVisualizationParameter::eCLOTH_VERTICAL:
		return "Cloth Vertical";
	case PxVisualizationParameter::eCLOTH_HORIZONTAL:
		return "Cloth Horizontal";
	case PxVisualizationParameter::eCLOTH_BENDING:
		return "Cloth Bending";
	case PxVisualizationParameter::eCLOTH_SHEARING:
		return "Cloth Shearing";
	case PxVisualizationParameter::eCLOTH_VIRTUAL_PARTICLES:
		return "Cloth Virtual Particles";
	case PxVisualizationParameter::eMBP_REGIONS:
		return "MBP Regions";
	default:
		PX_ALWAYS_ASSERT();
		return "";
	}
}

void CommonUIController::addApexDebugRenderParam(std::string name, std::string module, float value, std::string uiName)
{
	PX_ASSERT(mSettingsBar);

	IDebugRenderParam* param = new ApexDebugRenderParam(this, name, module, value);
	mDebugRenderParams.push_back(param);

	TwAddVarCB(mSettingsBar, uiName.empty() ? name.c_str() : uiName.c_str(), TW_TYPE_BOOLCPP, CommonUIController::setDebugRenderParam,
		CommonUIController::getDebugRenderParam, param, "group='Debug Render'");
}

void CommonUIController::addPhysXDebugRenderParam(physx::PxVisualizationParameter::Enum parameter)
{
	PX_ASSERT(mSettingsBar);

	IDebugRenderParam* param = new PhysXDebugRenderParam(this, parameter);
	mDebugRenderParams.push_back(param);

	TwAddVarCB(mSettingsBar, getUINameForPhysXParam(parameter), TW_TYPE_BOOLCPP, CommonUIController::setDebugRenderParam,
		CommonUIController::getDebugRenderParam, param, "group='Debug Render'");
}