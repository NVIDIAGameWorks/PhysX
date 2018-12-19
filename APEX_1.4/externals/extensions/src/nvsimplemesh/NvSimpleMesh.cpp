// TAGRELEASE: PUBLIC
#include <assert.h>

// Direct3D9 includes
#include <d3d9.h>

// Direct3D11 includes
#include <d3dcommon.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>

// XInput includes
#include <xinput.h>

#ifndef V
#define V(x)           { hr = (x); }
#endif
#ifndef V_RETURN
#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif


#include "strsafe.h"
#include <string>

#include "NvSimpleRawMesh.h"
#include "NvSimpleMesh.h"
#include "NvSimpleMeshLoader.h"

NvSimpleMesh::NvSimpleMesh() :
    iNumVertices(0),
    iNumIndices(0),
    IndexFormat(DXGI_FORMAT_R16_UINT),
    VertexStride(0),
    pVB(NULL),
    pIB(NULL),
    pDiffuseTexture(NULL),
    pDiffuseSRV(NULL),
    pNormalsTexture(NULL),
    pNormalsSRV(NULL),
    pInputLayout(NULL)
{
    memset(szName,0,260);
}

inline ID3D11ShaderResourceView * UtilCreateTexture2DSRV(ID3D11Device *pd3dDevice,ID3D11Texture2D *pTex)
{
    HRESULT hr;
    ID3D11ShaderResourceView *pSRV = NULL;

    D3D11_TEXTURE2D_DESC TexDesc;
    ((ID3D11Texture2D*)pTex)->GetDesc(&TexDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    ::ZeroMemory(&SRVDesc,sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    SRVDesc.Texture2D.MipLevels = TexDesc.MipLevels;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format = TexDesc.Format;

    hr = pd3dDevice->CreateShaderResourceView(pTex,&SRVDesc,&pSRV);

    return pSRV;
}

HRESULT NvSimpleMesh::Initialize(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh)
{
    HRESULT hr = S_OK;
    
    if(pRawMesh)
    {
        memcpy(&Extents,pRawMesh->GetExtents(),3*sizeof(float));
        memcpy(&Center,pRawMesh->GetCenter(),3*sizeof(float));

        // Copy out d3d buffers and data for rendering and add references input mesh can be cleaned.
        pVB = pRawMesh->CreateD3D11VertexBufferFor(pd3dDevice);
        pIB = pRawMesh->CreateD3D11IndexBufferFor(pd3dDevice);
        IndexFormat = pRawMesh->GetIndexSize()==2?DXGI_FORMAT_R16_UINT:DXGI_FORMAT_R32_UINT;
        iNumIndices = pRawMesh->GetNumIndices();
        iNumVertices = pRawMesh->GetNumVertices();
        VertexStride = pRawMesh->GetVertexStride();

        // Make a texture object and SRV for it.
        pDiffuseTexture = pRawMesh->CreateD3D11DiffuseTextureFor(pd3dDevice);
        if(pDiffuseTexture)
            pDiffuseSRV = UtilCreateTexture2DSRV(pd3dDevice,pDiffuseTexture);
        pNormalsTexture = pRawMesh->CreateD3D11NormalsTextureFor(pd3dDevice);
        if(pNormalsTexture)
            pNormalsSRV = UtilCreateTexture2DSRV(pd3dDevice,pNormalsTexture);
    }

    return hr;
}

HRESULT NvSimpleMesh::InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh,BYTE*pIAsig, SIZE_T pIAsigSize)
{
    HRESULT hr = S_OK;
    V_RETURN(CreateInputLayout(pd3dDevice,pIAsig,pIAsigSize));
    V_RETURN(Initialize(pd3dDevice,pRawMesh));
    return hr;
}

HRESULT NvSimpleMesh::CreateInputLayout(ID3D11Device *pd3dDevice,BYTE*pIAsig, SIZE_T pIAsigSize)
{
    HRESULT hr = S_OK;
    SAFE_RELEASE(pInputLayout);
    V_RETURN( pd3dDevice->CreateInputLayout( NvSimpleRawMesh::D3D11InputElements, NvSimpleRawMesh::D3D11ElementsSize, pIAsig, pIAsigSize, &pInputLayout ) );
    return hr;
}

void NvSimpleMesh::Release()
{
    SAFE_RELEASE(pVB);
    SAFE_RELEASE(pIB);
    SAFE_RELEASE(pDiffuseTexture);
    SAFE_RELEASE(pDiffuseSRV);
    SAFE_RELEASE(pNormalsTexture);
    SAFE_RELEASE(pNormalsSRV);
    SAFE_RELEASE(pInputLayout);
}

void NvSimpleMesh::SetupDraw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot, int iNormalsTexSlot)
{
    if(iDiffuseTexSlot >= 0)
        pd3dContext->PSSetShaderResources(iDiffuseTexSlot,1,&pDiffuseSRV);
    if(iNormalsTexSlot >= 0)
        pd3dContext->PSSetShaderResources(iNormalsTexSlot,1,&pNormalsSRV);

    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    UINT Offsets[1];
    UINT Strides[1];
    ID3D11Buffer *ppVB[1];
    Offsets[0] = 0;
    Strides[0] = VertexStride;
    ppVB[0] = pVB;

    if(pInputLayout)    // optionally set an input layout if we have one
        pd3dContext->IASetInputLayout(pInputLayout);
    pd3dContext->IASetVertexBuffers(0,1,ppVB,Strides,Offsets);
    pd3dContext->IASetIndexBuffer( pIB,IndexFormat, 0 );
    pd3dContext->IASetPrimitiveTopology(topology);
}

void NvSimpleMesh::Draw(ID3D11DeviceContext *pd3dContext)
{
    if(!pVB) 
    {
        return;
    }

    pd3dContext->DrawIndexed(iNumIndices,0,0);
}


void NvSimpleMesh::DrawInstanced(ID3D11DeviceContext *pd3dContext, int iNumInstances)
{
    pd3dContext->DrawIndexedInstanced(iNumIndices,iNumInstances,0,0,0);
}

NvAggregateSimpleMesh::NvAggregateSimpleMesh()
{
    pSimpleMeshes = NULL;
    NumSimpleMeshes = 0;
}

NvAggregateSimpleMesh::~NvAggregateSimpleMesh()
{
    SAFE_DELETE_ARRAY(pSimpleMeshes);
}

HRESULT NvAggregateSimpleMesh::Initialize(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader)
{
    HRESULT hr = S_OK;
    if(pMeshLoader->pMeshes == NULL) return E_FAIL;
    NumSimpleMeshes = pMeshLoader->NumMeshes;
    pSimpleMeshes = new NvSimpleMesh[NumSimpleMeshes];
    for(int iMesh=0;iMesh<NumSimpleMeshes;iMesh++)
    {
        V_RETURN(pSimpleMeshes[iMesh].Initialize(pd3dDevice,&pMeshLoader->pMeshes[iMesh]));
    }
    return hr;
}
HRESULT NvAggregateSimpleMesh::InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader,BYTE*pIAsig, SIZE_T pIAsigSize)
{
    HRESULT hr = S_OK;
    if(pMeshLoader->pMeshes == NULL) return E_FAIL;
    NumSimpleMeshes = pMeshLoader->NumMeshes;
    pSimpleMeshes = new NvSimpleMesh[NumSimpleMeshes];
    for(int iMesh=0;iMesh<NumSimpleMeshes;iMesh++)
    {
        V_RETURN(pSimpleMeshes[iMesh].InitializeWithInputLayout(pd3dDevice,&pMeshLoader->pMeshes[iMesh],pIAsig,pIAsigSize));
    }
    return hr;
}

void NvAggregateSimpleMesh::Release()
{
    if(pSimpleMeshes == NULL) return;
    for(int iMesh=0;iMesh<NumSimpleMeshes;iMesh++)
    {
        pSimpleMeshes[iMesh].Release();
    }
}

void NvAggregateSimpleMesh::Draw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot, int iNormalsTexSlot)
{
    if(pSimpleMeshes == NULL) return;
    for(int iMesh=0;iMesh<NumSimpleMeshes;iMesh++)
    {
        pSimpleMeshes[iMesh].SetupDraw(pd3dContext,iDiffuseTexSlot,iNormalsTexSlot);
        pSimpleMeshes[iMesh].Draw(pd3dContext);
    }
}
