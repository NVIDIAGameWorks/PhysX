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


#ifndef COMMON_UI_CONTROLLER_H
#define COMMON_UI_CONTROLLER_H

#include "SampleManager.h"
#include <DirectXMath.h>
#include "AntTweakBar.h"
#pragma warning(push)
#pragma warning(disable : 4350)
#include <string>
#include <list>
#pragma warning(pop)
#include "PxPhysicsAPI.h"

class CFirstPersonCamera;
class ApexRenderer;
class ApexController;

class CommonUIController : public ISampleController
{
  public:
	CommonUIController(CFirstPersonCamera* cam, ApexRenderer* r, ApexController* a);
	virtual ~CommonUIController() {};

	virtual HRESULT DeviceCreated(ID3D11Device* pDevice);
	virtual void DeviceDestroyed();
	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Animate(double fElapsedTimeSeconds);
	virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*);
	virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

	void addHintLine(std::string hintLine);

	void addApexDebugRenderParam(std::string name, std::string module = "", float value = 1.0f, std::string uiName = "");
	void addPhysXDebugRenderParam(physx::PxVisualizationParameter::Enum parameter);

	static void TW_CALL setWireframeEnabled(const void* value, void* clientData);
	static void TW_CALL getWireframeEnabled(void* value, void* clientData);

	static void TW_CALL setDebugRenderParam(const void* value, void* clientData);
	static void TW_CALL getDebugRenderParam(void* value, void* clientData);

	static void TW_CALL onReloadShadersButton(void* clientData);

	static void TW_CALL setFixedTimestepEnabled(const void* value, void* clientData);
	static void TW_CALL getFixedTimestepEnabled(void* value, void* clientData);

	static void TW_CALL setFixedSimFrequency(const void* value, void* clientData);
	static void TW_CALL getFixedSimFrequency(void* value, void* clientData);

private:
	void toggleCameraSpeed(bool overspeed);

	CFirstPersonCamera* mCamera;
	ApexRenderer* mApexRenderer;
	ApexController* mApexController;
	TwBar* mSettingsBar;

	UINT mWidth;
	UINT mHeight;

	std::list<std::string> mHintOnLines;
	std::list<std::string> mHintOffLines;
	bool mShowHint;

	class IDebugRenderParam
	{
	public:
		virtual ~IDebugRenderParam() {}
		virtual bool isParamEnabled() = 0;
		virtual void setParamEnabled(bool) = 0;
	};

	class ApexDebugRenderParam : public IDebugRenderParam
	{
	public:
		ApexDebugRenderParam(CommonUIController* controller, std::string name, std::string module, float value)
			: mController(controller), mName(name), mModule(module), mValue(value) {}

		virtual bool isParamEnabled();
		virtual void setParamEnabled(bool);

	private:
		CommonUIController* mController;
		std::string mName;
		std::string mModule;
		float mValue;
	};

	class PhysXDebugRenderParam : public IDebugRenderParam
	{
	public:
		PhysXDebugRenderParam(CommonUIController* controller, physx::PxVisualizationParameter::Enum parameter)
			: mController(controller), mParameter(parameter) {}

		virtual bool isParamEnabled();
		virtual void setParamEnabled(bool);

	private:
		CommonUIController* mController;
		physx::PxVisualizationParameter::Enum mParameter;
	};

	std::list<IDebugRenderParam*> mDebugRenderParams;
};

#endif