// TAGRELEASE: PUBLIC
#pragma once
#include <DirectXMath.h>

class NvSimpleRawMesh;
class NvSimpleMeshLoader;

class NvSimpleMesh
{
public:
    NvSimpleMesh();

    HRESULT Initialize(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh);
    HRESULT InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleRawMesh *pRawMesh,BYTE*pIAsig, SIZE_T pIAsigSize);
    HRESULT CreateInputLayout(ID3D11Device *pd3dDevice,BYTE*pIAsig, SIZE_T pIAsigSize);
    void Release();

    void SetupDraw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot=-1, int iNormalsTexSlot=-1);
    void Draw(ID3D11DeviceContext *pd3dContext);
    void DrawInstanced(ID3D11DeviceContext *pd3dContext, int iNumInstances);

    UINT iNumVertices;
    UINT iNumIndices;
    DXGI_FORMAT IndexFormat;
    UINT VertexStride;

    DirectX::XMFLOAT3 Extents;
	DirectX::XMFLOAT3 Center;

    ID3D11InputLayout *pInputLayout;

    ID3D11Buffer *pVB;
    ID3D11Buffer *pIB;
    ID3D11Texture2D *pDiffuseTexture;
    ID3D11ShaderResourceView *pDiffuseSRV;
    ID3D11Texture2D *pNormalsTexture;
    ID3D11ShaderResourceView *pNormalsSRV;
    char szName[260];
};

class NvAggregateSimpleMesh
{
public:
    NvAggregateSimpleMesh();
    ~NvAggregateSimpleMesh();

    HRESULT Initialize(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader);
    HRESULT InitializeWithInputLayout(ID3D11Device *pd3dDevice,NvSimpleMeshLoader *pMeshLoader,BYTE*pIAsig, SIZE_T pIAsigSize);
    void Release();

    void Draw(ID3D11DeviceContext *pd3dContext, int iDiffuseTexSlot=-1, int iNormalsTexSlot=-1);

    int NumSimpleMeshes;
    NvSimpleMesh *pSimpleMeshes;
};