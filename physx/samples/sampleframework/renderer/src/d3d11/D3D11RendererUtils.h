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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.

#ifndef D3D11_RENDERER_UTILS_H
#define D3D11_RENDERER_UTILS_H

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11Renderer.h"
#include "D3D11RendererTraits.h"

#include <d3dx11tex.h>

struct ID3DX11ThreadPump;

namespace SampleRenderer
{

PX_INLINE PxU32 fast_hash(const char *str, SIZE_T wrdlen)
{
	// "FNV" string hash
	const PxU32 PRIME = 1607;

	PxU32 hash32 = 2166136261;
	const char *p = str;

	for(; wrdlen >= sizeof(PxU32); wrdlen -= sizeof(PxU32), p += sizeof(PxU32)) {
		hash32 = (hash32 ^ *(PxU32 *)p) * PRIME;
	}
	if (wrdlen & sizeof(PxU16)) {
		hash32 = (hash32 ^ *(PxU16*)p) * PRIME;
		p += sizeof(PxU16);
	}
	if (wrdlen & 1) 
		hash32 = (hash32 ^ *p) * PRIME;

	return hash32 ^ (hash32 >> 16);
}

class D3D11StringKey
{
public:
	D3D11StringKey(const char* stringKey) : mStringHash(fast_hash(stringKey, strlen(stringKey))) { }
	D3D11StringKey(const std::string& stringKey) : mStringHash(fast_hash(stringKey.c_str(), stringKey.length())) { }
	bool operator<(const D3D11StringKey& other) const { return mStringHash < other.mStringHash; }
private:
	D3D11StringKey();
	PxU32 mStringHash;
};

class D3DX11
{
	friend class D3D11Renderer;
private:
	D3DX11(void);
	~D3DX11(void);

public:
	HRESULT compileShaderFromFile(LPCSTR pSrcFile, CONST D3D10_SHADER_MACRO* pDefines, LPD3D10INCLUDE pInclude,
		LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, ID3DX11ThreadPump* pPump, ID3D10Blob** ppShader, ID3D10Blob** ppErrorMsgs, HRESULT* pHResult) const;
	HRESULT compileShaderFromMemory(LPCSTR pSrcData, SIZE_T SrcDataLen, LPCSTR pFileName, CONST D3D10_SHADER_MACRO* pDefines, LPD3D10INCLUDE pInclude,
		LPCSTR pFunctionName, LPCSTR pProfile, UINT Flags1, UINT Flags2, ID3DX11ThreadPump* pPump, ID3D10Blob** ppShader, ID3D10Blob** ppErrorMsgs, HRESULT* pHResult) const;
	HRESULT reflect(LPCVOID pSrcData, SIZE_T SrcDataSize, REFIID pInterface, void** ppReflector);
	HRESULT saveTextureToMemory(ID3D11DeviceContext *pContext, ID3D11Resource *pSrcTexture, D3DX11_IMAGE_FILE_FORMAT DestFormat, LPD3D10BLOB *ppDestBuf, UINT Flags);
	HRESULT createBlob(SIZE_T Size, ID3DBlob **ppBlob);

public:
	static void processCompileErrors(ID3DBlob* errors);

	static PxU64 getInputHash(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputElementDesc);
	static PxU64 getInputHash(const D3D11_INPUT_ELEMENT_DESC* inputElementDesc, UINT numInputElementDescs);

private:
#if defined(RENDERER_WINDOWS)
	HMODULE  m_library;
	HMODULE  m_compiler_library;

	typedef HRESULT(WINAPI* D3DX11COMPILEFROMFILE)(LPCSTR, CONST D3D10_SHADER_MACRO*, LPD3D10INCLUDE, LPCSTR, LPCSTR, UINT, UINT, ID3DX11ThreadPump*, ID3D10Blob**, ID3D10Blob**, HRESULT*);
	D3DX11COMPILEFROMFILE m_pD3DX11CompileFromFile;
	typedef HRESULT(WINAPI* D3DX11COMPILEFROMMEMORY)(LPCSTR, SIZE_T, LPCSTR, CONST D3D10_SHADER_MACRO*, LPD3D10INCLUDE, LPCSTR, LPCSTR, UINT, UINT, ID3DX11ThreadPump*, ID3D10Blob**, ID3D10Blob**, HRESULT*);
	D3DX11COMPILEFROMMEMORY m_pD3DX11CompileFromMemory;
	typedef HRESULT(WINAPI* D3DREFLECT)(LPCVOID, SIZE_T, REFIID, void**);
	D3DREFLECT m_pD3DReflect;
	typedef HRESULT(WINAPI* D3DX11SAVETEXTURETOMEMORY)(ID3D11DeviceContext *, ID3D11Resource *, D3DX11_IMAGE_FILE_FORMAT, LPD3D10BLOB *, UINT);
	D3DX11SAVETEXTURETOMEMORY m_pD3DX11SaveTextureToMemory;
	typedef HRESULT(WINAPI* D3DCREATEBLOB)(SIZE_T, ID3DBlob**);
	D3DCREATEBLOB m_pD3DCreateBlob;

#endif

};

class D3D11ShaderIncluder : public ID3DInclude
{
public:
	D3D11ShaderIncluder(const char* assetDir);

private:
	STDMETHOD(Open(
	              D3D_INCLUDE_TYPE includeType,
	              LPCSTR fileName,
	              LPCVOID parentData,
	              LPCVOID* data,
	              UINT* dataSize
	          ));

	STDMETHOD(Close(LPCVOID data));

	const char* m_assetDir;
};


class D3D11ShaderCacher
{
public:
	D3D11ShaderCacher(D3D11RendererResourceManager* pResourceManager);

	template<typename d3d_type>
	bool check(const typename D3DTraits<d3d_type>::key_type& key,
	           d3d_type*& d3dResource,
	           ID3DBlob*& d3dResourceBlob) const
	{
		typename D3DTraits<d3d_type>::value_type value = mResourceManager->hasResource<d3d_type>(key);
		d3dResource = value.first;
		d3dResourceBlob = value.second;
		return !((NULL == d3dResource) || (NULL == d3dResourceBlob));
	}

	template<typename d3d_type>
	void cache(const typename D3DTraits<d3d_type>::key_type& key,
	           const typename D3DTraits<d3d_type>::value_type& value)
	{
		mResourceManager->registerResource<d3d_type>(key, value);
	}

private:
	D3D11ShaderCacher();

	D3D11RendererResourceManager* mResourceManager;
};

static void PxConvertToWchar(const char* inString, WCHAR* outString, int outSize)
{
	// convert to unicode
	int succ = MultiByteToWideChar(CP_ACP, 0, inString, -1, outString, outSize);

	// validate
	if (succ < 0)
		succ = 0;
	if (succ < outSize)
		outString[succ] = 0;
	else if (outString[outSize-1])
		outString[0] = 0;
}

class D3D11ShaderLoader
{
public:
	D3D11ShaderLoader(D3D11Renderer& renderer);

	template<typename d3d_type>
	HRESULT load(typename D3DTraits<d3d_type>::key_type shaderKey,
	             const char *pShaderPath,
	             const D3D10_SHADER_MACRO* pDefines, 
	             d3d_type **ppShader               = NULL,
	             ID3DBlob **ppShaderBlob           = NULL,
	             bool bCheckCacheBeforeLoading     = true,
	             bool *pbLoadedFromCache           = NULL)
	{
		HRESULT result        = S_OK;
		d3d_type* pShader     = NULL;
		ID3DBlob* pShaderBlob = NULL;

		if (!bCheckCacheBeforeLoading || !mCacher.check<d3d_type>(shaderKey, pShader, pShaderBlob))
		{
			result = internalLoad(pShaderPath, pDefines, &pShader, &pShaderBlob);
			mCacher.cache<d3d_type>(shaderKey, std::make_pair(pShader, pShaderBlob));
			if (pbLoadedFromCache) *pbLoadedFromCache = false;
		}
		else if (pbLoadedFromCache)
		{
			*pbLoadedFromCache = true;
		}

		if (SUCCEEDED(result))
		{
			if (ppShader)         *ppShader = pShader;
			if (ppShaderBlob) *ppShaderBlob = pShaderBlob;
		}
		else
		{
			RENDERER_ASSERT(0, "Error loading D3D11 shader");
		}

		return result;
	}

protected:
	template<typename d3d_type>
	HRESULT internalLoad(const char* pShaderName, const D3D10_SHADER_MACRO* pDefines, d3d_type** ppShader, ID3DBlob** ppBlob)
	{
		HRESULT result          = S_OK;
		ID3DBlob* pErrorsBlob   = NULL;

#ifdef PX_USE_DX11_PRECOMPILED_SHADERS
		//if(pDefines[0].Name == "RENDERER_FRAGMENT")
		{
			WCHAR bufferWchar[512];
			PxConvertToWchar(pShaderName,bufferWchar,512);
			result = D3DReadFileToBlob(bufferWchar,ppBlob);
			RENDERER_ASSERT(SUCCEEDED(result) && *ppBlob, "Failed to load precompiled shader.");

			char szBuff[1024];
			sprintf(szBuff,"Precompiled shader loaded: %s \n",pShaderName);
			OutputDebugStringA(szBuff);	
		}
#else
		const DWORD shaderFlags = D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

		result = mCompiler->compileShaderFromFile(
			pShaderName,
			pDefines,
			&mIncluder,
			D3DTraits<d3d_type>::getEntry(),
			D3DTraits<d3d_type>::getProfile(mFeatureLevel),
			shaderFlags,
			0,
			NULL,
			ppBlob,
			&pErrorsBlob,
			NULL
			);
		D3DX11::processCompileErrors(pErrorsBlob);
		RENDERER_ASSERT(SUCCEEDED(result) && *ppBlob, "Failed to compile shader.");
#endif		
		
		if (SUCCEEDED(result) && *ppBlob)
		{
			result = D3DTraits<d3d_type>::create(mDevice, (*ppBlob)->GetBufferPointer(), (*ppBlob)->GetBufferSize(), NULL, ppShader);
			RENDERER_ASSERT(SUCCEEDED(result) && *ppShader, "Failed to load Fragment Shader.");
		}
		if (pErrorsBlob)
		{
			pErrorsBlob->Release();
		}
		return result;
	}
	
private:
	D3D11ShaderLoader();
	D3D11ShaderLoader& operator=(const D3D11ShaderLoader&);

	D3D_FEATURE_LEVEL       mFeatureLevel;
	D3D11ShaderCacher       mCacher;
	D3D11ShaderIncluder     mIncluder;
	D3DX11*                 mCompiler;
	ID3D11Device*           mDevice;
};


}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
#endif
