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


#include "ApexRenderer.h"
#include "ApexRenderResources.h"


#include "XInput.h"
#include "DXUTMisc.h"
#include "DXUTCamera.h"

#include "PhysXPrimitive.h"

#include "ApexResourceCallback.h"

using namespace physx;
using namespace nvidia;

const float CAMERA_CLIP_NEAR = 1.0f;
const float CAMERA_CLIP_FAR = 10000.00f;

const float CLEAR_SCENE_COLOR[4] = { 0.0f, 0.0f, 0.0f, 0.0f };


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//									RenderDebugInterface implementation wrapper
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class RenderDebugImpl : public RENDER_DEBUG::RenderDebugInterface
{
public:
	RenderDebugImpl(ApexRenderer* renderer) : mRenderer(renderer) {}

	virtual void debugRenderLines(uint32_t lcount, const RENDER_DEBUG::RenderDebugVertex *vertices,
		bool useZ, bool isScreenSpace)
	{
		PX_UNUSED(useZ);
		PX_UNUSED(isScreenSpace);
		mRenderer->renderDebugPrimitive(vertices, lcount * 2, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	virtual void debugRenderTriangles(uint32_t tcount, const RENDER_DEBUG::RenderDebugSolidVertex *vertices,
		bool useZ, bool isScreenSpace)
	{
		PX_UNUSED(useZ);
		PX_UNUSED(isScreenSpace);
		RENDER_DEBUG::RenderDebugVertex* verts = new RENDER_DEBUG::RenderDebugVertex[tcount * 3 * 2];

		for (uint32_t i = 0; i < tcount * 3 * 2; ++i)
		{
			verts[i].mPos[0] = vertices[i].mPos[0];
			verts[i].mPos[1] = vertices[i].mPos[1];
			verts[i].mPos[2] = vertices[i].mPos[2];
			verts[i].mColor = vertices[i].mColor;
		}

		mRenderer->renderDebugPrimitive(verts, tcount * 3 * 2, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		delete[] verts;
	}

	// unused API:
	virtual void debugMessage(const char * /* msg */) {}
	virtual void renderTriangleMeshInstances(uint32_t, uint32_t, float, uint32_t, float, uint32_t, const RENDER_DEBUG::RenderDebugInstance*) {}
	virtual void createTriangleMesh(uint32_t, uint32_t, const RENDER_DEBUG::RenderDebugMeshVertex*, uint32_t, const uint32_t*) {}
	virtual void refreshTriangleMeshVertices(uint32_t, uint32_t, const RENDER_DEBUG::RenderDebugMeshVertex*, const uint32_t*) {}
	virtual void releaseTriangleMesh(uint32_t) {}
	virtual void debugText2D(float, float, float, float, bool, uint32_t, const char*) {}
	virtual void createCustomTexture(uint32_t, const char*) {}
	virtual void debugPoints(RENDER_DEBUG::PointRenderMode, uint32_t, uint32_t, float, uint32_t, float, uint32_t, float, uint32_t, const float*) {}
	virtual void registerDigitalInputEvent(RENDER_DEBUG::InputEventIds::Enum, RENDER_DEBUG::InputIds::Enum) {}
	virtual void registerAnalogInputEvent(RENDER_DEBUG::InputEventIds::Enum, float, RENDER_DEBUG::InputIds::Enum) {}
	virtual void unregisterInputEvent(RENDER_DEBUG::InputEventIds::Enum) {}
	virtual void resetInputEvents(void) {}

private:
	ApexRenderer* mRenderer;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//												Apex Renderer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


ApexRenderer::ApexRenderer(CFirstPersonCamera* cam, ApexController& apex)
: mApex(apex)
, mCamera(cam)
, mCameraCB(nullptr)
, mWorldCB(nullptr)
, mObjectCB(nullptr)
, mRSState(nullptr)
, mDSState(nullptr)
, mDSTexture(nullptr)
, mDSView(nullptr)
, mDSTextureSRV(nullptr)
, mPointSampler(nullptr)
, mLinearSampler(nullptr)
, mWireframeMode(false)
, mDebugPrimitiveVB(nullptr)
, mDebugPrimitiveVBVerticesCount(0)
{
	mCBWorldData.ambientColor = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
	mCBWorldData.pointLightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	mCBWorldData.pointLightPos = DirectX::XMFLOAT3(-6.5f, 13.0f, 12.0f);
	mCBWorldData.dirLightColor = DirectX::XMFLOAT3(0.21f, 0.21f, 0.21f);
	mCBWorldData.dirLightDir = DirectX::XMFLOAT3(-0.08f, -0.34f, -0.91f);
	mCBWorldData.specularPower = 140.0f;
	mCBWorldData.specularIntensity = 0.4f;

	// render debug interface
	mRenderDebugImpl = new (RenderDebugImpl)(this);
	mApex.setRenderDebugInterface(mRenderDebugImpl);
}

ApexRenderer::~ApexRenderer()
{
	delete mRenderDebugImpl;
}

void ApexRenderer::initializeDefaultRSState()
{
	SAFE_RELEASE(mRSState);
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.CullMode = D3D11_CULL_NONE;
	desc.FillMode = mWireframeMode ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	desc.AntialiasedLineEnable = FALSE;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0;
	desc.DepthClipEnable = TRUE;
	desc.FrontCounterClockwise = FALSE;
	desc.MultisampleEnable = TRUE;
	desc.ScissorEnable = FALSE;
	desc.SlopeScaledDepthBias = 0;

	V(mDevice->CreateRasterizerState(&desc, &mRSState));
}

HRESULT ApexRenderer::DeviceCreated(ID3D11Device* device)
{
	mDevice = device;

	// Camera constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBCamera);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &mCameraCB));
	}

	// World constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBWorld);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &mWorldCB));
	}

	// Object constant buffer
	{
		D3D11_BUFFER_DESC buffer_desc;
		ZeroMemory(&buffer_desc, sizeof(buffer_desc));
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.ByteWidth = sizeof(CBObject);
		_ASSERT((buffer_desc.ByteWidth % 16) == 0);

		V(device->CreateBuffer(&buffer_desc, nullptr, &mObjectCB));
	}

	// Depth-Stencil state
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.StencilEnable = FALSE;
		desc.DepthEnable = TRUE;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		V(device->CreateDepthStencilState(&desc, &mDSState));
	}

	// Linear sampler
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		V(device->CreateSamplerState(&desc, &mLinearSampler));
	}

	// Point sampler
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		V(device->CreateSamplerState(&desc, &mPointSampler));
	}

	// Rasterizer state
	initializeDefaultRSState();

	return S_OK;
}

void ApexRenderer::DeviceDestroyed()
{
	SAFE_RELEASE(mCameraCB);
	SAFE_RELEASE(mWorldCB);
	SAFE_RELEASE(mObjectCB);
	SAFE_RELEASE(mRSState);
	SAFE_RELEASE(mDSState);
	SAFE_RELEASE(mPointSampler);
	SAFE_RELEASE(mLinearSampler);
	SAFE_RELEASE(mDebugPrimitiveVB);
	SAFE_RELEASE(mDSTexture);
	SAFE_RELEASE(mDSView);
	SAFE_RELEASE(mDSTextureSRV);
}

#include "Shlwapi.h"

void ApexRenderer::onInitialize()
{
	char buf[256];
	sprintf(buf, "%s", GetCommandLineA());
	PathRemoveFileSpecA(buf);
	sprintf(buf + strlen(buf), "/../../samples_v2/");

	mApex.getResourceCallback()->addSearchDir(&buf[1]);

	// physx primitive render material and input layout
	{
		mPhysXPrimitiveRenderMaterial = new ApexRenderMaterial(mApex.getResourceCallback(), "physx_primitive", "");

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		mPhysXPrimitiveRenderMaterialInstance = mPhysXPrimitiveRenderMaterial->getMaterialInstance(layout, ARRAYSIZE(layout));
	}

	// debug primitive render material and input layout
	{
		mDebugPrimitiveRenderMaterial = new ApexRenderMaterial(mApex.getResourceCallback(), "debug_primitive", "");

		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		mDebugPrimitiveRenderMaterialInstance = mDebugPrimitiveRenderMaterial->getMaterialInstance(layout, ARRAYSIZE(layout));
	}
}

void ApexRenderer::onTerminate()
{
	SAFE_DELETE(mPhysXPrimitiveRenderMaterial);
	SAFE_DELETE(mDebugPrimitiveRenderMaterial);
}

void ApexRenderer::BackBufferResized(ID3D11Device* /*device*/, const DXGI_SURFACE_DESC* sd)
{
	// Setup the camera's projection parameters
	float fAspectRatio = sd->Width / (FLOAT)sd->Height;
	mCamera->SetProjParams(DirectX::XM_PIDIV4, fAspectRatio, CAMERA_CLIP_NEAR, CAMERA_CLIP_FAR);

	SAFE_RELEASE(mDSTexture);
	SAFE_RELEASE(mDSView);
	SAFE_RELEASE(mDSTextureSRV);

	// create a new Depth-Stencil texture 
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = sd->Width;
		desc.Height = sd->Height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R32_TYPELESS; // Use a typeless type here so that it can be both depth-stencil and shader resource.
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		V(mDevice->CreateTexture2D(&desc, NULL, &mDSTexture));
	}

	// create Depth-Stencil view
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		desc.Format = DXGI_FORMAT_D32_FLOAT;	// Make the view see this as D32_FLOAT instead of typeless
		desc.Texture2D.MipSlice = 0;
		V(mDevice->CreateDepthStencilView(mDSTexture, &desc, &mDSView));
	}

	// create Depth-Stencil shader resource view
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Format = DXGI_FORMAT_R32_FLOAT;	// Make the shaders see this as R32_FLOAT instead of typeless
		desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		desc.Texture2D.MostDetailedMip = 0;
		V(mDevice->CreateShaderResourceView(mDSTexture, &desc, &mDSTextureSRV));
	}
}

void ApexRenderer::Render(ID3D11Device* /*device*/, ID3D11DeviceContext* ctx, ID3D11RenderTargetView* pRTV,
                          ID3D11DepthStencilView*)
{
	mCurrentContext = ctx;

	ctx->ClearRenderTargetView(pRTV, CLEAR_SCENE_COLOR);
	ctx->ClearDepthStencilView(mDSView, D3D11_CLEAR_DEPTH, 1.0, 0);

	// Fill Camera constant buffer
	{
		DirectX::XMMATRIX viewMatrix = mCamera->GetViewMatrix();
		DirectX::XMMATRIX projMatrix = mCamera->GetProjMatrix();
		DirectX::XMMATRIX projMatrixInv = DirectX::XMMatrixInverse(NULL, projMatrix);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ctx->Map(mCameraCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		CBCamera* cameraBuffer = (CBCamera*)mappedResource.pData;
		cameraBuffer->projection = projMatrix;
		cameraBuffer->projectionInv = projMatrixInv;
		cameraBuffer->view = viewMatrix;
		DirectX::XMStoreFloat3(&(cameraBuffer->viewPos), mCamera->GetEyePt());
		ctx->Unmap(mCameraCB, 0);
	}

	// Fill World constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ctx->Map(mWorldCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		CBWorld* worldBuffer = (CBWorld*)mappedResource.pData;
		worldBuffer->ambientColor = mCBWorldData.ambientColor;
		worldBuffer->pointLightPos = mCBWorldData.pointLightPos;
		worldBuffer->pointLightColor = mCBWorldData.pointLightColor;
		worldBuffer->dirLightDir = mCBWorldData.dirLightDir;
		worldBuffer->specularPower = mCBWorldData.specularPower;
		worldBuffer->dirLightColor = mCBWorldData.dirLightColor;
		worldBuffer->specularIntensity = mCBWorldData.specularIntensity;
		ctx->Unmap(mWorldCB, 0);
	}

	// set constants buffers
	ID3D11Buffer* cbs[4] = { nullptr };
	cbs[0] = mCameraCB;
	cbs[1] = mWorldCB;
	cbs[2] = mObjectCB;
	ctx->VSSetConstantBuffers(0, 3, cbs);
	ctx->GSSetConstantBuffers(0, 3, cbs);
	ctx->PSSetConstantBuffers(0, 3, cbs);

	ctx->RSSetState(mRSState);
	ctx->PSSetSamplers(0, 1, &mLinearSampler);
	ctx->PSSetSamplers(1, 1, &mPointSampler);

	// Opaque render
	ctx->OMSetRenderTargets(1, &pRTV, mDSView);
	ctx->OMSetDepthStencilState(mDSState, 0xFF);

	mApex.renderOpaque(this);

	// Transparency render
	ctx->OMSetRenderTargets(1, &pRTV, NULL);
	ctx->PSSetShaderResources(1, 1, &mDSTextureSRV);

	mApex.renderTransparency(this);

	// Reset RT and SRV state
	ID3D11ShaderResourceView* nullAttach[16] = { nullptr };
	ctx->PSSetShaderResources(0, 16, nullAttach);
	ctx->OMSetRenderTargets(0, nullptr, nullptr);

}

void ApexRenderer::renderResource(const RenderContext& context)
{
	if(context.renderResource)
	{
		// Fill Object constant buffer
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			mCurrentContext->Map(mObjectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			CBObject* objectBuffer = (CBObject*)mappedResource.pData;

			objectBuffer->world = PxMat44ToXMMATRIX(context.local2world);

			mCurrentContext->Unmap(mObjectCB, 0);
		}

		static_cast<SampleApexRendererMesh*>(context.renderResource)->render(context);
	}
}

void ApexRenderer::renderResource(PhysXPrimitive* primitive)
{
	mPhysXPrimitiveRenderMaterialInstance->bind(*mCurrentContext, 0);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	mCurrentContext->Map(mObjectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CBObject* objectBuffer = (CBObject*)mappedResource.pData;

	objectBuffer->world = PxMat44ToXMMATRIX(primitive->getModelMatrix());
	objectBuffer->color = primitive->getColor();

	mCurrentContext->Unmap(mObjectCB, 0);

	primitive->render(*mCurrentContext);
}

void ApexRenderer::renderDebugPrimitive(const RENDER_DEBUG::RenderDebugVertex *vertices, uint32_t verticesCount, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	mCurrentContext->IASetPrimitiveTopology(topology);

	mDebugPrimitiveRenderMaterialInstance->bind(*mCurrentContext, 0);

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	mCurrentContext->Map(mObjectCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	CBObject* objectBuffer = (CBObject*)mappedResource.pData;

	objectBuffer->world = PxMat44ToXMMATRIX(PxMat44(PxIdentity));

	mCurrentContext->Unmap(mObjectCB, 0);

	if (mDebugPrimitiveVBVerticesCount < verticesCount)
	{
		mDebugPrimitiveVBVerticesCount = verticesCount;
		SAFE_RELEASE(mDebugPrimitiveVB);

		D3D11_BUFFER_DESC bufferDesc;

		memset(&bufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(RENDER_DEBUG::RenderDebugVertex) * mDebugPrimitiveVBVerticesCount;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		V(mDevice->CreateBuffer(&bufferDesc, NULL, &mDebugPrimitiveVB));
	}

	CD3D11_BOX box(0, 0, 0, (LONG)(sizeof(RENDER_DEBUG::RenderDebugVertex) * verticesCount), 1, 1);
	mCurrentContext->UpdateSubresource(mDebugPrimitiveVB, 0, &box, vertices, 0, 0);

	ID3D11Buffer* pBuffers[1] = { mDebugPrimitiveVB };
	UINT strides[1] = { sizeof(RENDER_DEBUG::RenderDebugVertex) };
	UINT offsets[1] = { 0 };
	mCurrentContext->IASetVertexBuffers(0, 1, pBuffers, strides, offsets);

	mCurrentContext->Draw(verticesCount, 0);
}

void ApexRenderer::reloadShaders()
{
	uint32_t count;
	void** materials = mApex.getResourceProvider()->findAllResources(APEX_MATERIALS_NAME_SPACE, count);
	for (uint32_t i = 0; i < count; i++)
	{
		ApexRenderMaterial* a = ((ApexRenderMaterial**)materials)[i];
		a->reload();
	}

	mPhysXPrimitiveRenderMaterial->reload();
	mDebugPrimitiveRenderMaterial->reload();
}


