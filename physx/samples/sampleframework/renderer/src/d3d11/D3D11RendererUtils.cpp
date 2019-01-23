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

#include <RendererConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D11)

#include "D3D11RendererUtils.h"
#include "D3D11RendererMemoryMacros.h"

#include "D3DX11.h"
#include "D3Dcompiler.h"

// for PsString.h
#include <PsString.h>
#include <PxTkFile.h>

using namespace SampleRenderer;
namespace Ps = physx::shdfnd;


/******************************************
* D3DX11::D3DX11                          *
*******************************************/

D3DX11::D3DX11(void)
	: m_pD3DX11CompileFromFile(NULL),
	m_pD3DX11CompileFromMemory(NULL),
	m_pD3DReflect(NULL)
{
#if defined(RENDERER_WINDOWS)
	m_library = LoadLibraryA(D3DX11_DLL);
	if (!m_library)
	{
		// This version should be present on all valid DX11 machines
		m_library = LoadLibraryA("d3dx11_42.dll");
		if (!m_library)
		{
			MessageBoxA(0, "Unable to load " D3DX11_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
	}
	if (m_library)
	{
		m_pD3DX11CompileFromFile = (D3DX11COMPILEFROMFILE)GetProcAddress(m_library, "D3DX11CompileFromFileA");
		RENDERER_ASSERT(m_pD3DX11CompileFromFile, "Unable to find D3DX11 Function D3DX11CompileFromFile in " D3DX11_DLL ".");
		m_pD3DX11CompileFromMemory = (D3DX11COMPILEFROMMEMORY)GetProcAddress(m_library, "D3DX11CompileFromMemory");
		RENDERER_ASSERT(m_pD3DX11CompileFromMemory, "Unable to find D3DX11 Function D3DX11CompileFromFile in " D3DX11_DLL ".");
		m_pD3DX11SaveTextureToMemory = (D3DX11SAVETEXTURETOMEMORY)GetProcAddress(m_library, "D3DX11SaveTextureToMemory");
		RENDERER_ASSERT(m_pD3DX11SaveTextureToMemory, "Unable to find D3DX11 Function D3DX11SaveTextureToMemory in " D3DX11_DLL ".");
	}

	m_compiler_library = LoadLibraryA(D3DCOMPILER_DLL);
	if (!m_compiler_library)
	{
		// This version should be present on all valid DX11 machines
		m_library = LoadLibraryA("D3DCompiler_42.dll");
		if (!m_compiler_library)
		{
			MessageBoxA(0, "Unable to load " D3DCOMPILER_DLL ". Please install the latest DirectX End User Runtime available at www.microsoft.com/directx.", "Renderer Error.", MB_OK);
		}
	}
	if (m_compiler_library)
	{
		m_pD3DReflect = (D3DREFLECT)GetProcAddress(m_compiler_library, "D3DReflect");
		RENDERER_ASSERT(m_pD3DReflect, "Unable to find D3D Function D3DReflect in " D3DCOMPILER_DLL ".");
		m_pD3DCreateBlob = (D3DCREATEBLOB)GetProcAddress(m_compiler_library, "D3DCreateBlob");
		RENDERER_ASSERT(m_pD3DCreateBlob, "Unable to find D3D Function D3DCreateBlob in " D3DCOMPILER_DLL ".");
	}
#endif
}

D3DX11::~D3DX11(void)
{
#if defined(RENDERER_WINDOWS)
	if (m_library)
	{
		FreeLibrary(m_library);
		m_library = 0;
	}
	if (m_compiler_library)
	{
		FreeLibrary(m_compiler_library);
		m_compiler_library = 0;
	}
#endif
}

HRESULT D3DX11::compileShaderFromFile(LPCSTR pSrcFile,
        CONST D3D10_SHADER_MACRO* pDefines,
        LPD3D10INCLUDE pInclude,
        LPCSTR pFunctionName,
        LPCSTR pProfile,
        UINT Flags1,
        UINT Flags2,
        ID3DX11ThreadPump* pPump,
        ID3D10Blob** ppShader,
        ID3D10Blob** ppErrorMsgs,
        HRESULT* pHResult) const
{
	return m_pD3DX11CompileFromFile(pSrcFile,
		pDefines,
		pInclude,
		pFunctionName,
		pProfile,
		Flags1,
		Flags2,
		pPump,
		ppShader,
		ppErrorMsgs,
		pHResult);
}

HRESULT D3DX11::compileShaderFromMemory(LPCSTR pSrcData,
        SIZE_T SrcDataLen,
        LPCSTR pFileName,
        CONST D3D10_SHADER_MACRO* pDefines,
        LPD3D10INCLUDE pInclude,
        LPCSTR pFunctionName,
        LPCSTR pProfile,
        UINT Flags1,
        UINT Flags2,
        ID3DX11ThreadPump* pPump,
        ID3D10Blob** ppShader,
        ID3D10Blob** ppErrorMsgs,
        HRESULT* pHResult) const
{
	return m_pD3DX11CompileFromMemory(pSrcData,
		SrcDataLen,
		pFileName,
		pDefines,
		pInclude,
		pFunctionName,
		pProfile,
		Flags1,
		Flags2,
		pPump,
		ppShader,
		ppErrorMsgs,
		pHResult);
}

HRESULT D3DX11::reflect(LPCVOID pSrcData, SIZE_T SrcDataSize, REFIID pInterface, void** ppReflector)
{
	return m_pD3DReflect(pSrcData, SrcDataSize, pInterface, ppReflector);
}

HRESULT D3DX11::saveTextureToMemory( ID3D11DeviceContext *pContext, ID3D11Resource *pSrcTexture, D3DX11_IMAGE_FILE_FORMAT DestFormat, LPD3D10BLOB *ppDestBuf, UINT Flags )
{
	return m_pD3DX11SaveTextureToMemory(pContext, pSrcTexture, DestFormat, ppDestBuf, Flags);
}

HRESULT D3DX11::createBlob(SIZE_T Size, ID3DBlob **ppBlob)
{
	return m_pD3DCreateBlob(Size, ppBlob);
}

void D3DX11::processCompileErrors(ID3DBlob* errors)
{
#if defined(RENDERER_WINDOWS)
	if (errors)
	{
		const char* errorStr = (const char*)errors->GetBufferPointer();
		if (errorStr)
		{
			static bool ignoreErrors = false;
			if (!ignoreErrors)
			{
				int ret = MessageBoxA(0, errorStr, "D3DXCompileShaderFromFile Error", MB_ABORTRETRYIGNORE);
				if (ret == IDABORT)
				{
					exit(0);
				}
				else if (ret == IDIGNORE)
				{
					ignoreErrors = true;
				}
			}
		}
	}
#endif
}

PxU64 D3DX11::getInputHash(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc)
{
	return getInputHash(&inputDesc[0], (UINT)inputDesc.size());
}

static PxU8 sizeOfFormatElement(DXGI_FORMAT format)
{
	switch( format )
	{
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return 32;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
		return 16;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
		return 8;

		// Compressed format; http://msdn2.microso.../bb694531(VS.85).aspx
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
		return 128;

		// Compressed format; http://msdn2.microso.../bb694531(VS.85).aspx
	case DXGI_FORMAT_R1_UNORM:
	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 64;

		// Compressed format; http://msdn2.microso.../bb694531(VS.85).aspx
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		return 32;

		// These are compressed, but bit-size information is unclear.
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
		return 32;

	case DXGI_FORMAT_UNKNOWN:
	default:
		return 0;
	}
}

PxU64 D3DX11::getInputHash(const D3D11_INPUT_ELEMENT_DESC* inputDesc, UINT numInputDescs)
{
	enum ShaderInputFlag
	{
		USE_NONE         = 0x00,
		USE_POSITION     = 0x01,
		USE_NORMAL       = USE_POSITION << 1,
		USE_TANGENT      = USE_POSITION << 2,
		USE_COLOR        = USE_POSITION << 3,
		USE_TEXCOORD     = USE_POSITION << 4,
		NUM_INPUT_TYPES   = 5,
	};

	// Each input element gets a certain number of bits in the hash
	const PxU8 bitsPerElement =  physx::PxMax((PxU8)((sizeof(PxU64) * 8) / numInputDescs), (PxU8)1);

	// Restrict the maximum amount each set of element bits can be shifted 
	const PxU8 maxSizeShift = physx::PxMin(bitsPerElement, (PxU8)NUM_INPUT_TYPES);

	PxU64 inputHash = 0;
	for (UINT i = 0; i < numInputDescs; ++i)
	{
		if(inputDesc->SemanticName)
		{
			PxU8 flag = USE_NONE;

			if (Ps::stricmp(inputDesc[i].SemanticName,      "TEXCOORD") == 0)
			{
				flag = USE_TEXCOORD;
			}
			else if (Ps::stricmp(inputDesc[i].SemanticName, "NORMAL") == 0)
			{
				flag = USE_NORMAL;
			}
			else if (Ps::stricmp(inputDesc[i].SemanticName, "TANGENT") == 0)
			{
				flag = USE_TANGENT;
			}
			else if (Ps::stricmp(inputDesc[i].SemanticName, "COLOR") == 0)
			{
				flag = USE_COLOR;
			}
			else if (Ps::stricmp(inputDesc[i].SemanticName, "POSITION") == 0)
			{
				flag = USE_POSITION;
			}

			static const PxU8 bitsPerDefaultFormat = sizeOfFormatElement(DXGI_FORMAT_R8G8B8A8_UINT);
			PX_ASSERT(bitsPerDefaultFormat > 0);
			
			// Shift the semantic bit depending on the element's relative format size
			const PxU8 elementSizeShift  = (sizeOfFormatElement(inputDesc[i].Format) / bitsPerDefaultFormat) & maxSizeShift;

			// Shift the semantic bit depending on it's order in the element array
			const PxU8 elementOrderShift = (PxU8)(i * bitsPerElement);

			// Add the element's bits to the hash, shifted by it's order in the element array
			inputHash |= (flag << (elementSizeShift + elementOrderShift));
		}
	}

	return inputHash;
}


/*******************************************
* D3D11ShaderIncluder::D3D11ShaderIncluder *
*******************************************/

D3D11ShaderIncluder::D3D11ShaderIncluder(const char* assetDir) : m_assetDir(assetDir) {}

HRESULT D3D11ShaderIncluder::Open(
    D3D_INCLUDE_TYPE includeType,
    LPCSTR fileName,
    LPCVOID parentData,
    LPCVOID* data,
    UINT* dataSize
)
{
	HRESULT result = D3D11_ERROR_FILE_NOT_FOUND;

	char fullpath[1024];
	Ps::strlcpy(fullpath, 1024, m_assetDir);
	Ps::strlcat(fullpath, 1024, "shaders/");
	if (includeType == D3D10_INCLUDE_SYSTEM)
	{
		Ps::strlcat(fullpath, 1024, "include/");
	}
	Ps::strlcat(fullpath, 1024, fileName);

	FILE* file = 0;
	PxToolkit::fopen_s(&file, fullpath, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		size_t fileLen = ftell(file);
		if (fileLen > 1)
		{
			fseek(file, 0, SEEK_SET);
			char* fileData = new char[fileLen + 1];
			fileLen = fread(fileData, 1, fileLen, file);
			fileData[fileLen] = 0;
			*data     = fileData;
			*dataSize = (UINT)fileLen;
		}
		fclose(file);
		result = S_OK;
	}
	RENDERER_ASSERT(result == S_OK, "Failed to include shader header.");
	return result;
}

HRESULT D3D11ShaderIncluder::Close(LPCVOID data)
{
	delete []((char*)data);
	return S_OK;
}

/***************************************
* D3D11ShaderCacher::D3D11ShaderCacher *
***************************************/

D3D11ShaderCacher::D3D11ShaderCacher(D3D11RendererResourceManager* pResourceManager)
: mResourceManager(pResourceManager) 
{
	RENDERER_ASSERT(pResourceManager, "Invalid D3D resource manager");
}


/***************************************
* D3D11ShaderLoader::D3D11ShaderLoader *
***************************************/

D3D11ShaderLoader::D3D11ShaderLoader(D3D11Renderer& renderer)
	: mFeatureLevel(renderer.getFeatureLevel()),
	mCacher(renderer.getResourceManager()),
	mCompiler(renderer.getD3DX11()),
	mIncluder(renderer.getAssetDir()),
	mDevice(renderer.getD3DDevice())
{ 
	RENDERER_ASSERT(mCompiler, "Invalid D3D compiler");
	RENDERER_ASSERT(mDevice,   "Invalid D3D device");
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D11)
