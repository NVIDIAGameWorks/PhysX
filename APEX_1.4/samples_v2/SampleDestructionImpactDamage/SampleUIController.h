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


#ifndef SAMPLE_UI_CONTROLLER_H
#define SAMPLE_UI_CONTROLLER_H

#include "SampleManager.h"
#include <DirectXMath.h>
#include "AntTweakBar.h"

class SampleSceneController;
class CommonUIController;

class SampleUIController : public ISampleController
{
  public:
	SampleUIController(SampleSceneController* s, CommonUIController* c);

	virtual void onInitialize();
	virtual LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Render(ID3D11Device*, ID3D11DeviceContext*, ID3D11RenderTargetView*, ID3D11DepthStencilView*);
	virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);

	static void TW_CALL setForceToDamage(const void* value, void* clientData);
	static void TW_CALL getForceToDamage(void* value, void* clientData);

	static void TW_CALL setDamageThreshold(const void* value, void* clientData);
	static void TW_CALL getDamageThreshold(void* value, void* clientData);

	static void TW_CALL setDamageCap(const void* value, void* clientData);
	static void TW_CALL getDamageCap(void* value, void* clientData);

  private:
	SampleSceneController* mScene;
	CommonUIController* mCommonUIController;
	TwBar* mSettingsBar;

	UINT mWidth;
	UINT mHeight;
};

#endif