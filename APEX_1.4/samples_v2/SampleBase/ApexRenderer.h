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


#ifndef APEX_RENDERER_H
#define APEX_RENDERER_H

#include "ApexRenderMaterial.h"
#include "Apex.h"
#include "ApexController.h"
#include <DirectXMath.h>
#include "SampleManager.h"

using namespace nvidia::apex;

class CFirstPersonCamera;
class PhysXPrimitive;
class RenderDebugImpl;

class ApexRenderer : public ISampleController, public UserRenderer
{
  public:
	ApexRenderer(CFirstPersonCamera* cam, ApexController& apex);
	~ApexRenderer();
	virtual void renderResource(const RenderContext& context);
	void renderResource(PhysXPrimitive* primitive);
	void renderDebugPrimitive(const RENDER_DEBUG::RenderDebugVertex *vertices, uint32_t verticesCount, D3D11_PRIMITIVE_TOPOLOGY topology);

	void reloadShaders();

	bool getWireframeMode()
	{
		return mWireframeMode;
	}

	void setWireframeMode(bool enabled)
	{
		if(mWireframeMode != enabled)
		{
			mWireframeMode = enabled;
			initializeDefaultRSState();
		}
	}

	DirectX::XMFLOAT3& getAmbientColor()
	{
		return mCBWorldData.ambientColor;
	}

	DirectX::XMFLOAT3& getPointLightPos()
	{
		return mCBWorldData.pointLightPos;
	}

	DirectX::XMFLOAT3& getPointLightColor()
	{
		return mCBWorldData.pointLightColor;
	}

	DirectX::XMFLOAT3& getDirLightDir()
	{
		return mCBWorldData.dirLightDir;
	}

	DirectX::XMFLOAT3& getDirLightColor()
	{
		return mCBWorldData.dirLightColor;
	}

	float& getSpecularIntensity()
	{
		return mCBWorldData.specularIntensity;
	}

	float& getSpecularPower()
	{
		return mCBWorldData.specularPower;
	}

  protected:
	// callbacks:
	virtual HRESULT DeviceCreated(ID3D11Device* pDevice);
	virtual void DeviceDestroyed();
	virtual void onInitialize();
	virtual void onTerminate();
	virtual void BackBufferResized(ID3D11Device* pDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc);
	virtual void Render(ID3D11Device* /*device*/, ID3D11DeviceContext* ctx, ID3D11RenderTargetView* pRTV,
	                    ID3D11DepthStencilView* pDSV);

  private:
	void initializeDefaultRSState();

	ApexController& mApex;
	struct CBCamera
	{
		DirectX::XMMATRIX projection;
		DirectX::XMMATRIX projectionInv;
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT3 viewPos;
		float unusedPad;
	};
	struct CBWorld
	{
		DirectX::XMFLOAT3 ambientColor;
		float unusedPad1;
		DirectX::XMFLOAT3 pointLightPos;
		float unusedPad2;
		DirectX::XMFLOAT3 pointLightColor;
		float unusedPad3;
		DirectX::XMFLOAT3 dirLightDir;
		float specularPower;
		DirectX::XMFLOAT3 dirLightColor;
		float specularIntensity; // TODO: actually it's per object property
	};
	struct CBObject
	{
		DirectX::XMMATRIX world;
		DirectX::XMFLOAT3 color;
		float unusedPad;
	};

	CBWorld mCBWorldData;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mCurrentContext;

	CFirstPersonCamera* mCamera;

	ID3D11Buffer* mCameraCB;
	ID3D11Buffer* mWorldCB;
	ID3D11Buffer* mObjectCB;

	ID3D11RasterizerState* mRSState;
	ID3D11DepthStencilState* mDSState;

	ID3D11Texture2D* mDSTexture;
	ID3D11DepthStencilView*	mDSView;
	ID3D11ShaderResourceView* mDSTextureSRV;


	ID3D11SamplerState* mPointSampler;
	ID3D11SamplerState* mLinearSampler;

	ApexRenderMaterial* mPhysXPrimitiveRenderMaterial;
	ApexRenderMaterial::Instance* mPhysXPrimitiveRenderMaterialInstance;

	ApexRenderMaterial* mDebugPrimitiveRenderMaterial;
	ApexRenderMaterial::Instance* mDebugPrimitiveRenderMaterialInstance;;

	ID3D11Buffer* mDebugPrimitiveVB;
	uint32_t mDebugPrimitiveVBVerticesCount;

	RenderDebugImpl* mRenderDebugImpl;

	bool mWireframeMode;
};

static nvidia::PxMat44 XMMATRIXToPxMat44(const DirectX::XMMATRIX& mat)
{
	nvidia::PxMat44 m;
	memcpy(const_cast<float*>(m.front()), &mat.r[0], 4 * 4 * sizeof(float));
	return m;
}

static DirectX::XMMATRIX PxMat44ToXMMATRIX(const nvidia::PxMat44& mat)
{
	return DirectX::XMMATRIX(mat.front());
}

static nvidia::PxVec4 XMVECTORToPxVec4(const DirectX::XMVECTOR& vec)
{
	DirectX::XMFLOAT4 f;
	DirectX::XMStoreFloat4(&f, vec);
	return nvidia::PxVec4(f.x, f.y, f.z, f.w);
}

#endif