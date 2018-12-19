//--------------------------------------------------------------------------------------
// File: SDKMesh.h
//
// Disclaimer:  
//   The SDK Mesh format (.sdkmesh) is not a recommended file format for shipping titles.  
//   It was designed to meet the specific needs of the SDK samples.  Any real-world 
//   applications should avoid this file format in favor of a destination format that 
//   meets the specific needs of the application.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=320437
//--------------------------------------------------------------------------------------
#pragma once

#undef D3DCOLOR_ARGB
#include <d3d9.h>

//--------------------------------------------------------------------------------------
// Hard Defines for the various structures
//--------------------------------------------------------------------------------------
#define SDKMESH_FILE_VERSION 101
#define MAX_VERTEX_ELEMENTS 32
#define MAX_VERTEX_STREAMS 16
#define MAX_FRAME_NAME 100
#define MAX_MESH_NAME 100
#define MAX_SUBSET_NAME 100
#define MAX_MATERIAL_NAME 100
#define MAX_TEXTURE_NAME MAX_PATH
#define MAX_MATERIAL_PATH MAX_PATH
#define INVALID_FRAME ((UINT)-1)
#define INVALID_MESH ((UINT)-1)
#define INVALID_MATERIAL ((UINT)-1)
#define INVALID_SUBSET ((UINT)-1)
#define INVALID_ANIMATION_DATA ((UINT)-1)
#define INVALID_SAMPLER_SLOT ((UINT)-1)
#define ERROR_RESOURCE_VALUE 1

template<typename TYPE> BOOL IsErrorResource( TYPE data )
{
    if( ( TYPE )ERROR_RESOURCE_VALUE == data )
        return TRUE;
    return FALSE;
}
//--------------------------------------------------------------------------------------
// Enumerated Types.
//--------------------------------------------------------------------------------------
enum SDKMESH_PRIMITIVE_TYPE
{
    PT_TRIANGLE_LIST = 0,
    PT_TRIANGLE_STRIP,
    PT_LINE_LIST,
    PT_LINE_STRIP,
    PT_POINT_LIST,
    PT_TRIANGLE_LIST_ADJ,
    PT_TRIANGLE_STRIP_ADJ,
    PT_LINE_LIST_ADJ,
    PT_LINE_STRIP_ADJ,
    PT_QUAD_PATCH_LIST,
    PT_TRIANGLE_PATCH_LIST,
};

enum SDKMESH_INDEX_TYPE
{
    IT_16BIT = 0,
    IT_32BIT,
};

enum FRAME_TRANSFORM_TYPE
{
    FTT_RELATIVE = 0,
    FTT_ABSOLUTE,		//This is not currently used but is here to support absolute transformations in the future
};

//--------------------------------------------------------------------------------------
// Structures.  Unions with pointers are forced to 64bit.
//--------------------------------------------------------------------------------------
#pragma pack(push,8)

struct SDKMESH_HEADER
{
    //Basic Info and sizes
    UINT Version;
    BYTE IsBigEndian;
    UINT64 HeaderSize;
    UINT64 NonBufferDataSize;
    UINT64 BufferDataSize;

    //Stats
    UINT NumVertexBuffers;
    UINT NumIndexBuffers;
    UINT NumMeshes;
    UINT NumTotalSubsets;
    UINT NumFrames;
    UINT NumMaterials;

    //Offsets to Data
    UINT64 VertexStreamHeadersOffset;
    UINT64 IndexStreamHeadersOffset;
    UINT64 MeshDataOffset;
    UINT64 SubsetDataOffset;
    UINT64 FrameDataOffset;
    UINT64 MaterialDataOffset;
};

struct SDKMESH_VERTEX_BUFFER_HEADER
{
    UINT64 NumVertices;
    UINT64 SizeBytes;
    UINT64 StrideBytes;
    D3DVERTEXELEMENT9 Decl[MAX_VERTEX_ELEMENTS];
    union
    {
        UINT64 DataOffset;				//(This also forces the union to 64bits)
        ID3D11Buffer* pVB11;
    };
};

struct SDKMESH_INDEX_BUFFER_HEADER
{
    UINT64 NumIndices;
    UINT64 SizeBytes;
    UINT IndexType;
    union
    {
        UINT64 DataOffset;				//(This also forces the union to 64bits)
        ID3D11Buffer* pIB11;
    };
};

struct SDKMESH_MESH
{
    char Name[MAX_MESH_NAME];
    BYTE NumVertexBuffers;
    UINT VertexBuffers[MAX_VERTEX_STREAMS];
    UINT IndexBuffer;
    UINT NumSubsets;
    UINT NumFrameInfluences; //aka bones

    DirectX::XMFLOAT3 BoundingBoxCenter;
    DirectX::XMFLOAT3 BoundingBoxExtents;

    union
    {
        UINT64 SubsetOffset;	//Offset to list of subsets (This also forces the union to 64bits)
        UINT* pSubsets;	    //Pointer to list of subsets
    };
    union
    {
        UINT64 FrameInfluenceOffset;  //Offset to list of frame influences (This also forces the union to 64bits)
        UINT* pFrameInfluences;      //Pointer to list of frame influences
    };
};

struct SDKMESH_SUBSET
{
    char Name[MAX_SUBSET_NAME];
    UINT MaterialID;
    UINT PrimitiveType;
    UINT64 IndexStart;
    UINT64 IndexCount;
    UINT64 VertexStart;
    UINT64 VertexCount;
};

struct SDKMESH_FRAME
{
    char Name[MAX_FRAME_NAME];
    UINT Mesh;
    UINT ParentFrame;
    UINT ChildFrame;
    UINT SiblingFrame;
    DirectX::XMFLOAT4X4 Matrix;
    UINT AnimationDataIndex;		//Used to index which set of keyframes transforms this frame
};

struct SDKMESH_MATERIAL
{
    char    Name[MAX_MATERIAL_NAME];

    // Use MaterialInstancePath
    char    MaterialInstancePath[MAX_MATERIAL_PATH];

    // Or fall back to d3d8-type materials
    char    DiffuseTexture[MAX_TEXTURE_NAME];
    char    NormalTexture[MAX_TEXTURE_NAME];
    char    SpecularTexture[MAX_TEXTURE_NAME];

    DirectX::XMFLOAT4 Diffuse;
    DirectX::XMFLOAT4 Ambient;
    DirectX::XMFLOAT4 Specular;
    DirectX::XMFLOAT4 Emissive;
    float Power;

    union
    {
        UINT64 Force64_1;			//Force the union to 64bits
        ID3D11Texture2D* pDiffuseTexture11;
    };
    union
    {
        UINT64 Force64_2;			//Force the union to 64bits
        ID3D11Texture2D* pNormalTexture11;
    };
    union
    {
        UINT64 Force64_3;			//Force the union to 64bits
        ID3D11Texture2D* pSpecularTexture11;
    };

    union
    {
        UINT64 Force64_4;			//Force the union to 64bits
        ID3D11ShaderResourceView* pDiffuseRV11;
    };
    union
    {
        UINT64 Force64_5;		    //Force the union to 64bits
        ID3D11ShaderResourceView* pNormalRV11;
    };
    union
    {
        UINT64 Force64_6;			//Force the union to 64bits
        ID3D11ShaderResourceView* pSpecularRV11;
    };

};

struct SDKANIMATION_FILE_HEADER
{
    UINT Version;
    BYTE IsBigEndian;
    UINT FrameTransformType;
    UINT NumFrames;
    UINT NumAnimationKeys;
    UINT AnimationFPS;
    UINT64 AnimationDataSize;
    UINT64 AnimationDataOffset;
};

struct SDKANIMATION_DATA
{
    DirectX::XMFLOAT3 Translation;
    DirectX::XMFLOAT4 Orientation;
    DirectX::XMFLOAT3 Scaling;
};

struct SDKANIMATION_FRAME_DATA
{
    char FrameName[MAX_FRAME_NAME];
    union
    {
        UINT64 DataOffset;
        SDKANIMATION_DATA* pAnimationData;
    };
};

#pragma pack(pop)

static_assert( sizeof(D3DVERTEXELEMENT9) == 8, "Direct3D9 Decl structure size incorrect" );
static_assert( sizeof(SDKMESH_HEADER)== 104, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_VERTEX_BUFFER_HEADER) == 288, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_INDEX_BUFFER_HEADER) == 32, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_MESH) == 224, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_SUBSET) == 144, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_FRAME) == 184, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKMESH_MATERIAL) == 1256, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKANIMATION_FILE_HEADER) == 40, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKANIMATION_DATA) == 40, "SDK Mesh structure size incorrect" );
static_assert( sizeof(SDKANIMATION_FRAME_DATA) == 112, "SDK Mesh structure size incorrect" );

#ifndef _CONVERTER_APP_

//--------------------------------------------------------------------------------------
// AsyncLoading callbacks
//--------------------------------------------------------------------------------------
typedef void ( CALLBACK*LPCREATETEXTUREFROMFILE11 )( _In_ ID3D11Device* pDev, _In_z_ char* szFileName,
                                                     _Outptr_ ID3D11ShaderResourceView** ppRV, _In_opt_ void* pContext );
typedef void ( CALLBACK*LPCREATEVERTEXBUFFER11 )( _In_ ID3D11Device* pDev, _Outptr_ ID3D11Buffer** ppBuffer,
                                                  _In_ D3D11_BUFFER_DESC BufferDesc, _In_ void* pData, _In_opt_ void* pContext );
typedef void ( CALLBACK*LPCREATEINDEXBUFFER11 )( _In_ ID3D11Device* pDev, _Outptr_ ID3D11Buffer** ppBuffer,
                                                 _In_ D3D11_BUFFER_DESC BufferDesc, _In_ void* pData, _In_opt_ void* pContext );
struct SDKMESH_CALLBACKS11
{
    LPCREATETEXTUREFROMFILE11 pCreateTextureFromFile;
    LPCREATEVERTEXBUFFER11 pCreateVertexBuffer;
    LPCREATEINDEXBUFFER11 pCreateIndexBuffer;
    void* pContext;
};

//--------------------------------------------------------------------------------------
// CDXUTSDKMesh class.  This class reads the sdkmesh file format for use by the samples
//--------------------------------------------------------------------------------------
class CDXUTSDKMesh
{
private:
    UINT m_NumOutstandingResources;
    bool m_bLoading;
    //BYTE*                         m_pBufferData;
    HANDLE m_hFile;
    HANDLE m_hFileMappingObject;
    std::vector<BYTE*> m_MappedPointers;
    ID3D11Device* m_pDev11;
    ID3D11DeviceContext* m_pDevContext11;

protected:
    //These are the pointers to the two chunks of data loaded in from the mesh file
    BYTE* m_pStaticMeshData;
    BYTE* m_pHeapData;
    BYTE* m_pAnimationData;
    BYTE** m_ppVertices;
    BYTE** m_ppIndices;

    //Keep track of the path
    WCHAR                           m_strPathW[MAX_PATH];
    char                            m_strPath[MAX_PATH];

    //General mesh info
    SDKMESH_HEADER* m_pMeshHeader;
    SDKMESH_VERTEX_BUFFER_HEADER* m_pVertexBufferArray;
    SDKMESH_INDEX_BUFFER_HEADER* m_pIndexBufferArray;
    SDKMESH_MESH* m_pMeshArray;
    SDKMESH_SUBSET* m_pSubsetArray;
    SDKMESH_FRAME* m_pFrameArray;
    SDKMESH_MATERIAL* m_pMaterialArray;

    // Adjacency information (not part of the m_pStaticMeshData, so it must be created and destroyed separately )
    SDKMESH_INDEX_BUFFER_HEADER* m_pAdjacencyIndexBufferArray;

    //Animation
    SDKANIMATION_FILE_HEADER* m_pAnimationHeader;
    SDKANIMATION_FRAME_DATA* m_pAnimationFrameData;
    DirectX::XMFLOAT4X4* m_pBindPoseFrameMatrices;
    DirectX::XMFLOAT4X4* m_pTransformedFrameMatrices;
    DirectX::XMFLOAT4X4* m_pWorldPoseFrameMatrices;

protected:
    void LoadMaterials( _In_ ID3D11Device* pd3dDevice, _In_reads_(NumMaterials) SDKMESH_MATERIAL* pMaterials,
                        _In_ UINT NumMaterials, _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks = nullptr );

    HRESULT CreateVertexBuffer( _In_ ID3D11Device* pd3dDevice,
                                _In_ SDKMESH_VERTEX_BUFFER_HEADER* pHeader, _In_reads_(pHeader->SizeBytes) void* pVertices,
                                _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks = nullptr );

    HRESULT CreateIndexBuffer( _In_ ID3D11Device* pd3dDevice,
                               _In_ SDKMESH_INDEX_BUFFER_HEADER* pHeader, _In_reads_(pHeader->SizeBytes) void* pIndices,
                               _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks = nullptr );

    virtual HRESULT CreateFromFile( _In_opt_ ID3D11Device* pDev11,
                                    _In_z_ LPCWSTR szFileName,
                                    _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks11 = nullptr );

    virtual HRESULT CreateFromMemory( _In_opt_ ID3D11Device* pDev11,
                                      _In_reads_(DataBytes) BYTE* pData,
                                      _In_ size_t DataBytes,
                                      _In_ bool bCopyStatic,
                                      _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks11 = nullptr );

    //frame manipulation
    void TransformBindPoseFrame( _In_ UINT iFrame, _In_ DirectX::CXMMATRIX parentWorld );
    void TransformFrame( _In_ UINT iFrame, _In_ DirectX::CXMMATRIX parentWorld, _In_ double fTime );
    void TransformFrameAbsolute( _In_ UINT iFrame, _In_ double fTime );

    //Direct3D 11 rendering helpers
    void RenderMesh( _In_ UINT iMesh,
                     _In_ bool bAdjacent,
                     _In_ ID3D11DeviceContext* pd3dDeviceContext,
                     _In_ UINT iDiffuseSlot,
                     _In_ UINT iNormalSlot,
                     _In_ UINT iSpecularSlot );
    void RenderFrame( _In_ UINT iFrame,
                      _In_ bool bAdjacent,
                      _In_ ID3D11DeviceContext* pd3dDeviceContext,
                      _In_ UINT iDiffuseSlot,
                      _In_ UINT iNormalSlot,
                      _In_ UINT iSpecularSlot );

public:
    CDXUTSDKMesh();
    virtual ~CDXUTSDKMesh();

    virtual HRESULT Create( _In_ ID3D11Device* pDev11, _In_z_ LPCWSTR szFileName, _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks = nullptr );
    virtual HRESULT Create( _In_ ID3D11Device* pDev11, BYTE* pData, size_t DataBytes, _In_ bool bCopyStatic=false,
                            _In_opt_ SDKMESH_CALLBACKS11* pLoaderCallbacks = nullptr );
    virtual HRESULT LoadAnimation( _In_z_ const WCHAR* szFileName );
    virtual void Destroy();

    //Frame manipulation
    void TransformBindPose( _In_ DirectX::CXMMATRIX world ) { TransformBindPoseFrame( 0, world ); };
    void TransformMesh( _In_ DirectX::CXMMATRIX world, _In_ double fTime );

    //Direct3D 11 Rendering
    virtual void Render( _In_ ID3D11DeviceContext* pd3dDeviceContext,
                         _In_ UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
                         _In_ UINT iNormalSlot = INVALID_SAMPLER_SLOT,
                         _In_ UINT iSpecularSlot = INVALID_SAMPLER_SLOT );
    virtual void RenderAdjacent( _In_ ID3D11DeviceContext* pd3dDeviceContext,
                                 _In_ UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
                                 _In_ UINT iNormalSlot = INVALID_SAMPLER_SLOT,
                                 _In_ UINT iSpecularSlot = INVALID_SAMPLER_SLOT );

    //Helpers (D3D11 specific)
    static D3D11_PRIMITIVE_TOPOLOGY GetPrimitiveType11( _In_ SDKMESH_PRIMITIVE_TYPE PrimType );
    DXGI_FORMAT GetIBFormat11( _In_ UINT iMesh ) const;
    ID3D11Buffer* GetVB11( _In_ UINT iMesh, _In_ UINT iVB ) const;
    ID3D11Buffer* GetIB11( _In_ UINT iMesh ) const;
    SDKMESH_INDEX_TYPE GetIndexType( _In_ UINT iMesh ) const; 

    ID3D11Buffer* GetAdjIB11( _In_ UINT iMesh ) const;

    //Helpers (general)
    const char* GetMeshPathA() const;
    const WCHAR* GetMeshPathW() const;
    UINT GetNumMeshes() const;
    UINT GetNumMaterials() const;
    UINT GetNumVBs() const;
    UINT GetNumIBs() const;

    ID3D11Buffer* GetVB11At( _In_ UINT iVB ) const;
    ID3D11Buffer* GetIB11At( _In_ UINT iIB ) const;

    BYTE* GetRawVerticesAt( _In_ UINT iVB ) const;
    BYTE* GetRawIndicesAt( _In_ UINT iIB ) const;

    SDKMESH_MATERIAL* GetMaterial( _In_ UINT iMaterial ) const;
    SDKMESH_MESH*     GetMesh( _In_ UINT iMesh ) const;
    UINT              GetNumSubsets( _In_ UINT iMesh ) const;
    SDKMESH_SUBSET*   GetSubset( _In_ UINT iMesh, _In_ UINT iSubset ) const;
    UINT              GetVertexStride( _In_ UINT iMesh, _In_ UINT iVB ) const;
    UINT              GetNumFrames() const;
    SDKMESH_FRAME*    GetFrame( _In_ UINT iFrame ) const; 
    SDKMESH_FRAME*    FindFrame( _In_z_ const char* pszName ) const;
    UINT64            GetNumVertices( _In_ UINT iMesh, _In_ UINT iVB ) const;
    UINT64            GetNumIndices( _In_ UINT iMesh ) const;
    DirectX::XMVECTOR GetMeshBBoxCenter( _In_ UINT iMesh ) const;
    DirectX::XMVECTOR GetMeshBBoxExtents( _In_ UINT iMesh ) const;
    UINT              GetOutstandingResources() const;
    UINT              GetOutstandingBufferResources() const;
    bool              CheckLoadDone();
    bool              IsLoaded() const;
    bool              IsLoading() const;
    void              SetLoading( _In_ bool bLoading );
    BOOL              HadLoadingError() const;

    //Animation
    UINT              GetNumInfluences( _In_ UINT iMesh ) const;
    DirectX::XMMATRIX GetMeshInfluenceMatrix( _In_ UINT iMesh, _In_ UINT iInfluence ) const;
    UINT              GetAnimationKeyFromTime( _In_ double fTime ) const;
    DirectX::XMMATRIX GetWorldMatrix( _In_ UINT iFrameIndex ) const;
    DirectX::XMMATRIX GetInfluenceMatrix( _In_ UINT iFrameIndex ) const;
    bool              GetAnimationProperties( _Out_ UINT* pNumKeys, _Out_ float* pFrameTime ) const;
};

#endif

