// TAGRELEASE: PUBLIC
#pragma once

class NvSimpleRawMesh
{
public:

    class Vertex
    {
    public:
        float Position[3];
        float Normal[3];
        float UV[2];
        float Tangent[3];
    };

    NvSimpleRawMesh();
    ~NvSimpleRawMesh();

    UINT GetIndexSize();
    INT GetVertexStride();
    INT GetNumVertices();
    INT GetNumIndices();

    Vertex* GetVertices();
    BYTE* GetRawVertices();
    BYTE* GetRawIndices();

    float* GetExtents() {return m_extents;}
    float* GetCenter() {return m_center;}

    static HRESULT TryGuessFilename(WCHAR *szDestBuffer,WCHAR *szMeshFilename, WCHAR *szGuessSuffix);
    ID3D11Texture2D *CreateD3D11DiffuseTextureFor(ID3D11Device *pd3dDevice);
    ID3D11Texture2D *CreateD3D11NormalsTextureFor(ID3D11Device *pd3dDevice);
    ID3D11Buffer *CreateD3D11IndexBufferFor(ID3D11Device *pd3dDevice);
    ID3D11Buffer *CreateD3D11VertexBufferFor(ID3D11Device *pd3dDevice);

    Vertex *m_pVertexData;
    BYTE *m_pIndexData;
    UINT m_iNumVertices;
    UINT m_iNumIndices;
    UINT m_IndexSize;

    float m_extents[3];
    float m_center[3];

    WCHAR m_szMeshFilename[MAX_PATH];
    WCHAR m_szDiffuseTexture[MAX_PATH];
    WCHAR m_szNormalTexture[MAX_PATH];

    // Utils to wrap making d3d11 render buffers from a loader mesh
    static const D3D11_INPUT_ELEMENT_DESC D3D11InputElements[];
    static const int D3D11ElementsSize; 
};