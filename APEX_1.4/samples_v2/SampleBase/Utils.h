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


#ifndef UTILS_H
#define UTILS_H

#pragma warning(push)
#pragma warning(disable : 4917)
#pragma warning(disable : 4365)
#pragma warning(disable : 4350)
#include <DeviceManager.h>
#include <d3dcompiler.h>
#include <assert.h>
#include <sstream>
#pragma warning(pop)

#include "PxPreprocessor.h"

#pragma warning(disable : 4505)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//														MACROS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef V_RETURN
#define V_RETURN(x)                                                                                                    \
	{                                                                                                                  \
		hr = (x);                                                                                                      \
		if(FAILED(hr))                                                                                                 \
		{                                                                                                              \
			return hr;                                                                                                 \
		}                                                                                                              \
	}
#endif

#ifndef V
#define V(x)                                                                                                           \
	{                                                                                                                  \
		HRESULT hr = (x);                                                                                              \
		_ASSERT(SUCCEEDED(hr));                                                                                        \
	}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)                                                                                                \
	{                                                                                                                  \
		if(p)                                                                                                          \
		{                                                                                                              \
			(p)->Release();                                                                                            \
			(p) = NULL;                                                                                                \
		}                                                                                                              \
	}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)                                                                                                 \
	{                                                                                                                  \
		if(p)                                                                                                          \
		{                                                                                                              \
			delete (p);                                                                                                \
			(p) = NULL;                                                                                                \
		}                                                                                                              \
	}
#endif

#define ASSERT_PRINT(cond, format, ...)                                                                                \
	if(!(cond))                                                                                                        \
	{                                                                                                                  \
		messagebox_printf("Assertion Failed!", MB_OK | MB_ICONERROR, #cond "\n" format, __VA_ARGS__);                  \
		assert(cond);                                                                                                  \
	}

HRESULT messagebox_printf(const char* caption, UINT mb_type, const char* format, ...);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													SHADER HELPERS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static HRESULT CompileShaderFromFile(const char* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel,
                                     ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;
	ID3DBlob* pErrorBlob;

	WCHAR wFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, szFileName, -1, wFileName, MAX_PATH);
	wFileName[MAX_PATH - 1] = 0;
	hr = D3DCompileFromFile(wFileName, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel, D3D10_SHADER_ENABLE_STRICTNESS, 0,
	                        ppBlobOut, &pErrorBlob);
	if(FAILED(hr))
	{
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}
	SAFE_RELEASE(pErrorBlob);

	return S_OK;
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11VertexShader** ppShd, bool)
{
	return pDev->CreateVertexShader(pData, len, nullptr, ppShd);
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11GeometryShader** ppShd,
                            bool forceFast)
{
	PX_UNUSED(forceFast);
	return pDev->CreateGeometryShader(pData, len, nullptr, ppShd);
}

static HRESULT createShader(ID3D11Device* pDev, const void* pData, size_t len, ID3D11PixelShader** ppShd, bool)
{
	return pDev->CreatePixelShader(pData, len, nullptr, ppShd);
}

static const char* shaderModel(ID3D11VertexShader**)
{
	return "vs_5_0";
}

static const char* shaderModel(ID3D11GeometryShader**)
{
	return "gs_5_0";
}

static const char* shaderModel(ID3D11PixelShader**)
{
	return "ps_5_0";
}

// Give back the shader buffer blob for use in CreateVertexLayout.  Caller must release the blob.
template <class S>
static HRESULT createShaderFromFile(ID3D11Device* pDev, const char* szFileName, LPCSTR szEntryPoint, S** ppShd,
                                    ID3DBlob*& pShaderBuffer, bool forceFast = false)
{
	HRESULT hr = CompileShaderFromFile(szFileName, szEntryPoint, shaderModel(ppShd), &pShaderBuffer);
	if(SUCCEEDED(hr) && pShaderBuffer)
	{
		const void* shaderBufferData = pShaderBuffer->GetBufferPointer();
		const UINT shaderBufferSize = pShaderBuffer->GetBufferSize();
		createShader(pDev, shaderBufferData, shaderBufferSize, ppShd, forceFast);
	}
	return hr;
}

// Overloaded, same as above but don't give back the shader buffer blob.
template <class S>
static HRESULT createShaderFromFile(ID3D11Device* pDev, const char* szFileName, LPCSTR szEntryPoint, S** ppShd,
                                    bool forceFast = false)
{
	ID3DBlob* pShaderBuffer = NULL;
	HRESULT hr = createShaderFromFile(pDev, szFileName, szEntryPoint, ppShd, pShaderBuffer, forceFast);
	SAFE_RELEASE(pShaderBuffer);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* strext(const char* str)
{
	const char* ext = NULL; // by default no extension found!
	while (str)
	{
		str = strchr(str, '.');
		if (str)
		{
			str++;
			ext = str;
		}
	}
	return ext;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif