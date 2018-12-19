//--------------------------------------------------------------------------------------
// File: SDKMesh.cpp
//
// The SDK Mesh format (.sdkmesh) is not a recommended file format for games.  
// It was designed to meet the specific needs of the SDK samples.  Any real-world 
// applications should avoid this file format in favor of a destination format that 
// meets the specific needs of the application.
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
#include "DXUT.h"
#include "SDKMesh.h"
#include "SDKMisc.h"

using namespace DirectX;

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::LoadMaterials( ID3D11Device* pd3dDevice, SDKMESH_MATERIAL* pMaterials, UINT numMaterials,
                                  SDKMESH_CALLBACKS11* pLoaderCallbacks )
{
    char strPath[MAX_PATH];

    if( pLoaderCallbacks && pLoaderCallbacks->pCreateTextureFromFile )
    {
        for( UINT m = 0; m < numMaterials; m++ )
        {
            pMaterials[m].pDiffuseTexture11 = nullptr;
            pMaterials[m].pNormalTexture11 = nullptr;
            pMaterials[m].pSpecularTexture11 = nullptr;
            pMaterials[m].pDiffuseRV11 = nullptr;
            pMaterials[m].pNormalRV11 = nullptr;
            pMaterials[m].pSpecularRV11 = nullptr;

            // load textures
            if( pMaterials[m].DiffuseTexture[0] != 0 )
            {
                pLoaderCallbacks->pCreateTextureFromFile( pd3dDevice,
                                                          pMaterials[m].DiffuseTexture, &pMaterials[m].pDiffuseRV11,
                                                          pLoaderCallbacks->pContext );
            }
            if( pMaterials[m].NormalTexture[0] != 0 )
            {
                pLoaderCallbacks->pCreateTextureFromFile( pd3dDevice,
                                                          pMaterials[m].NormalTexture, &pMaterials[m].pNormalRV11,
                                                          pLoaderCallbacks->pContext );
            }
            if( pMaterials[m].SpecularTexture[0] != 0 )
            {
                pLoaderCallbacks->pCreateTextureFromFile( pd3dDevice,
                                                          pMaterials[m].SpecularTexture, &pMaterials[m].pSpecularRV11,
                                                          pLoaderCallbacks->pContext );
            }
        }
    }
    else
    {
        for( UINT m = 0; m < numMaterials; m++ )
        {
            pMaterials[m].pDiffuseTexture11 = nullptr;
            pMaterials[m].pNormalTexture11 = nullptr;
            pMaterials[m].pSpecularTexture11 = nullptr;
            pMaterials[m].pDiffuseRV11 = nullptr;
            pMaterials[m].pNormalRV11 = nullptr;
            pMaterials[m].pSpecularRV11 = nullptr;

            // load textures
            if( pMaterials[m].DiffuseTexture[0] != 0 )
            {
                sprintf_s( strPath, MAX_PATH, "%s%s", m_strPath, pMaterials[m].DiffuseTexture );
                if( FAILED( DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, DXUTGetD3D11DeviceContext(),
                                                                                strPath, &pMaterials[m].pDiffuseRV11,
                                                                                true ) ) )
                    pMaterials[m].pDiffuseRV11 = ( ID3D11ShaderResourceView* )ERROR_RESOURCE_VALUE;

            }
            if( pMaterials[m].NormalTexture[0] != 0 )
            {
                sprintf_s( strPath, MAX_PATH, "%s%s", m_strPath, pMaterials[m].NormalTexture );
                if( FAILED( DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, DXUTGetD3D11DeviceContext(),
                                                                                strPath,
                                                                                &pMaterials[m].pNormalRV11 ) ) )
                    pMaterials[m].pNormalRV11 = ( ID3D11ShaderResourceView* )ERROR_RESOURCE_VALUE;
            }
            if( pMaterials[m].SpecularTexture[0] != 0 )
            {
                sprintf_s( strPath, MAX_PATH, "%s%s", m_strPath, pMaterials[m].SpecularTexture );
                if( FAILED( DXUTGetGlobalResourceCache().CreateTextureFromFile( pd3dDevice, DXUTGetD3D11DeviceContext(),
                                                                                strPath,
                                                                                &pMaterials[m].pSpecularRV11 ) ) )
                    pMaterials[m].pSpecularRV11 = ( ID3D11ShaderResourceView* )ERROR_RESOURCE_VALUE;
            }
        }
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTSDKMesh::CreateVertexBuffer( ID3D11Device* pd3dDevice, SDKMESH_VERTEX_BUFFER_HEADER* pHeader,
                                          void* pVertices, SDKMESH_CALLBACKS11* pLoaderCallbacks )
{
    HRESULT hr = S_OK;
    pHeader->DataOffset = 0;
    //Vertex Buffer
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = ( UINT )( pHeader->SizeBytes );
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    if( pLoaderCallbacks && pLoaderCallbacks->pCreateVertexBuffer )
    {
        pLoaderCallbacks->pCreateVertexBuffer( pd3dDevice, &pHeader->pVB11, bufferDesc, pVertices,
                                               pLoaderCallbacks->pContext );
    }
    else
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pVertices;
        hr = pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &pHeader->pVB11 );
        if (SUCCEEDED(hr))
        {
            DXUT_SetDebugName(pHeader->pVB11, "CDXUTSDKMesh");
        }
    }

    return hr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTSDKMesh::CreateIndexBuffer( ID3D11Device* pd3dDevice, SDKMESH_INDEX_BUFFER_HEADER* pHeader,
                                         void* pIndices, SDKMESH_CALLBACKS11* pLoaderCallbacks )
{
    HRESULT hr = S_OK;
    pHeader->DataOffset = 0;
    //Index Buffer
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth = ( UINT )( pHeader->SizeBytes );
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;

    if( pLoaderCallbacks && pLoaderCallbacks->pCreateIndexBuffer )
    {
        pLoaderCallbacks->pCreateIndexBuffer( pd3dDevice, &pHeader->pIB11, bufferDesc, pIndices,
                                              pLoaderCallbacks->pContext );
    }
    else
    {
        D3D11_SUBRESOURCE_DATA InitData;
        InitData.pSysMem = pIndices;
        hr = pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &pHeader->pIB11 );
        if (SUCCEEDED(hr))
        {
            DXUT_SetDebugName(pHeader->pIB11, "CDXUTSDKMesh");
        }
    }

    return hr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTSDKMesh::CreateFromFile( ID3D11Device* pDev11,
                                      LPCWSTR szFileName,
                                      SDKMESH_CALLBACKS11* pLoaderCallbacks11 )
{
    HRESULT hr = S_OK;

    // Find the path for the file
    V_RETURN( DXUTFindDXSDKMediaFileCch( m_strPathW, sizeof( m_strPathW ) / sizeof( WCHAR ), szFileName ) );

    // Open the file
    m_hFile = CreateFile( m_strPathW, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
                          nullptr );
    if( INVALID_HANDLE_VALUE == m_hFile )
        return DXUTERR_MEDIANOTFOUND;

    // Change the path to just the directory
    WCHAR* pLastBSlash = wcsrchr( m_strPathW, L'\\' );
    if( pLastBSlash )
        *( pLastBSlash + 1 ) = L'\0';
    else
        *m_strPathW = L'\0';

    WideCharToMultiByte( CP_ACP, 0, m_strPathW, -1, m_strPath, MAX_PATH, nullptr, FALSE );

    // Get the file size
    LARGE_INTEGER FileSize;
    GetFileSizeEx( m_hFile, &FileSize );
    UINT cBytes = FileSize.LowPart;

    // Allocate memory
    m_pStaticMeshData = new (std::nothrow) BYTE[ cBytes ];
    if( !m_pStaticMeshData )
    {
        CloseHandle( m_hFile );
        return E_OUTOFMEMORY;
    }

    // Read in the file
    DWORD dwBytesRead;
    if( !ReadFile( m_hFile, m_pStaticMeshData, cBytes, &dwBytesRead, nullptr ) )
        hr = E_FAIL;

    CloseHandle( m_hFile );

    if( SUCCEEDED( hr ) )
    {
        hr = CreateFromMemory( pDev11,
                               m_pStaticMeshData,
                               cBytes,
                               false,
                               pLoaderCallbacks11 );
        if( FAILED( hr ) )
            delete []m_pStaticMeshData;
    }

    return hr;
}

_Use_decl_annotations_
HRESULT CDXUTSDKMesh::CreateFromMemory( ID3D11Device* pDev11,
                                        BYTE* pData,
                                        size_t DataBytes,
                                        bool bCopyStatic,
                                        SDKMESH_CALLBACKS11* pLoaderCallbacks11 )
{
    HRESULT hr = E_FAIL;
    XMFLOAT3 lower; 
    XMFLOAT3 upper; 
    
    m_pDev11 = pDev11;

    if ( DataBytes < sizeof(SDKMESH_HEADER) )
        return E_FAIL;

    // Set outstanding resources to zero
    m_NumOutstandingResources = 0;

    if( bCopyStatic )
    {
        auto pHeader = reinterpret_cast<SDKMESH_HEADER*>( pData );

        SIZE_T StaticSize = ( SIZE_T )( pHeader->HeaderSize + pHeader->NonBufferDataSize );
        if ( DataBytes < StaticSize )
            return E_FAIL;

        m_pHeapData = new (std::nothrow) BYTE[ StaticSize ];
        if( !m_pHeapData )
            return E_OUTOFMEMORY;

        m_pStaticMeshData = m_pHeapData;

        memcpy( m_pStaticMeshData, pData, StaticSize );
    }
    else
    {
        m_pHeapData = pData;
        m_pStaticMeshData = pData;
    }

    // Pointer fixup
    m_pMeshHeader = reinterpret_cast<SDKMESH_HEADER*>( m_pStaticMeshData );

    m_pVertexBufferArray = ( SDKMESH_VERTEX_BUFFER_HEADER* )( m_pStaticMeshData +
                                                              m_pMeshHeader->VertexStreamHeadersOffset );
    m_pIndexBufferArray = ( SDKMESH_INDEX_BUFFER_HEADER* )( m_pStaticMeshData +
                                                            m_pMeshHeader->IndexStreamHeadersOffset );
    m_pMeshArray = ( SDKMESH_MESH* )( m_pStaticMeshData + m_pMeshHeader->MeshDataOffset );
    m_pSubsetArray = ( SDKMESH_SUBSET* )( m_pStaticMeshData + m_pMeshHeader->SubsetDataOffset );
    m_pFrameArray = ( SDKMESH_FRAME* )( m_pStaticMeshData + m_pMeshHeader->FrameDataOffset );
    m_pMaterialArray = ( SDKMESH_MATERIAL* )( m_pStaticMeshData + m_pMeshHeader->MaterialDataOffset );

    // Setup subsets
    for( UINT i = 0; i < m_pMeshHeader->NumMeshes; i++ )
    {
        m_pMeshArray[i].pSubsets = ( UINT* )( m_pStaticMeshData + m_pMeshArray[i].SubsetOffset );
        m_pMeshArray[i].pFrameInfluences = ( UINT* )( m_pStaticMeshData + m_pMeshArray[i].FrameInfluenceOffset );
    }

    // error condition
    if( m_pMeshHeader->Version != SDKMESH_FILE_VERSION )
    {
        hr = E_NOINTERFACE;
        goto Error;
    }

    // Setup buffer data pointer
    BYTE* pBufferData = pData + m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Get the start of the buffer data
    UINT64 BufferDataStart = m_pMeshHeader->HeaderSize + m_pMeshHeader->NonBufferDataSize;

    // Create VBs
    m_ppVertices = new (std::nothrow) BYTE*[m_pMeshHeader->NumVertexBuffers];
    if ( !m_ppVertices )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }
    for( UINT i = 0; i < m_pMeshHeader->NumVertexBuffers; i++ )
    {
        BYTE* pVertices = nullptr;
        pVertices = ( BYTE* )( pBufferData + ( m_pVertexBufferArray[i].DataOffset - BufferDataStart ) );

        if( pDev11 )
            CreateVertexBuffer( pDev11, &m_pVertexBufferArray[i], pVertices, pLoaderCallbacks11 );

        m_ppVertices[i] = pVertices;
    }

    // Create IBs
    m_ppIndices = new (std::nothrow) BYTE*[m_pMeshHeader->NumIndexBuffers];
    if ( !m_ppIndices )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    for( UINT i = 0; i < m_pMeshHeader->NumIndexBuffers; i++ )
    {
        BYTE* pIndices = nullptr;
        pIndices = ( BYTE* )( pBufferData + ( m_pIndexBufferArray[i].DataOffset - BufferDataStart ) );

        if( pDev11 )
            CreateIndexBuffer( pDev11, &m_pIndexBufferArray[i], pIndices, pLoaderCallbacks11 );

        m_ppIndices[i] = pIndices;
    }

    // Load Materials
    if( pDev11 )
        LoadMaterials( pDev11, m_pMaterialArray, m_pMeshHeader->NumMaterials, pLoaderCallbacks11 );

    // Create a place to store our bind pose frame matrices
    m_pBindPoseFrameMatrices = new (std::nothrow) XMFLOAT4X4[ m_pMeshHeader->NumFrames ];
    if( !m_pBindPoseFrameMatrices )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // Create a place to store our transformed frame matrices
    m_pTransformedFrameMatrices = new (std::nothrow) XMFLOAT4X4[ m_pMeshHeader->NumFrames ];
    if( !m_pTransformedFrameMatrices )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    m_pWorldPoseFrameMatrices = new (std::nothrow) XMFLOAT4X4[ m_pMeshHeader->NumFrames ];
    if( !m_pWorldPoseFrameMatrices )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    SDKMESH_SUBSET* pSubset = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

    // update bounding volume 
    SDKMESH_MESH* currentMesh = &m_pMeshArray[0];
    int tris = 0;
    for (UINT meshi=0; meshi < m_pMeshHeader->NumMeshes; ++meshi) {
        lower.x = FLT_MAX; lower.y = FLT_MAX; lower.z = FLT_MAX;
        upper.x = -FLT_MAX; upper.y = -FLT_MAX; upper.z = -FLT_MAX;
        currentMesh = GetMesh( meshi );
        INT indsize;
        if (m_pIndexBufferArray[currentMesh->IndexBuffer].IndexType == IT_16BIT ) {
            indsize = 2;
        }else {
            indsize = 4;        
        }

        for( UINT subset = 0; subset < currentMesh->NumSubsets; subset++ )
        {
            pSubset = GetSubset( meshi, subset ); //&m_pSubsetArray[ currentMesh->pSubsets[subset] ];

            PrimType = GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
            assert( PrimType == D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );// only triangle lists are handled.

            UINT IndexCount = ( UINT )pSubset->IndexCount;
            UINT IndexStart = ( UINT )pSubset->IndexStart;

            /*if( bAdjacent )
            {
                IndexCount *= 2;
                IndexStart *= 2;
            }*/
     
        //BYTE* pIndices = nullptr;
            //m_ppIndices[i]
            UINT *ind = ( UINT * )m_ppIndices[currentMesh->IndexBuffer];
            float *verts =  ( float* )m_ppVertices[currentMesh->VertexBuffers[0]];
            UINT stride = (UINT)m_pVertexBufferArray[currentMesh->VertexBuffers[0]].StrideBytes;
            assert (stride % 4 == 0);
            stride /=4;
            for (UINT vertind = IndexStart; vertind < IndexStart + IndexCount; ++vertind) {
                UINT current_ind=0;
                if (indsize == 2) {
                    UINT ind_div2 = vertind / 2;
                    current_ind = ind[ind_div2];
                    if (vertind %2 ==0) {
                        current_ind = current_ind << 16;
                        current_ind = current_ind >> 16;
                    }else {
                        current_ind = current_ind >> 16;
                    }
                }else {
                    current_ind = ind[vertind];
                }
                tris++;
                XMFLOAT3 *pt = (XMFLOAT3*)&(verts[stride * current_ind]);
                if (pt->x < lower.x) {
                    lower.x = pt->x;
                }
                if (pt->y < lower.y) {
                    lower.y = pt->y;
                }
                if (pt->z < lower.z) {
                    lower.z = pt->z;
                }
                if (pt->x > upper.x) {
                    upper.x = pt->x;
                }
                if (pt->y > upper.y) {
                    upper.y = pt->y;
                }
                if (pt->z > upper.z) {
                    upper.z = pt->z;
                }
                //BYTE** m_ppVertices;
                //BYTE** m_ppIndices;
            }
            //pd3dDeviceContext->DrawIndexed( IndexCount, IndexStart, VertexStart );
        }

        XMFLOAT3 half( ( upper.x - lower.x ) * 0.5f,
                       ( upper.y - lower.y ) * 0.5f,
                       ( upper.z - lower.z ) * 0.5f );

        currentMesh->BoundingBoxCenter.x = lower.x + half.x;
        currentMesh->BoundingBoxCenter.y = lower.y + half.y;
        currentMesh->BoundingBoxCenter.z = lower.z + half.z;

        currentMesh->BoundingBoxExtents = half;

    }
    // Update 
        


    hr = S_OK;
Error:
    return hr;
}


//--------------------------------------------------------------------------------------
// transform bind pose frame using a recursive traversal
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::TransformBindPoseFrame( UINT iFrame, CXMMATRIX parentWorld )
{
    if( !m_pBindPoseFrameMatrices )
        return;

    // Transform ourselves
    XMMATRIX m = XMLoadFloat4x4( &m_pFrameArray[iFrame].Matrix );
    XMMATRIX mLocalWorld = XMMatrixMultiply( m, parentWorld );
    XMStoreFloat4x4( &m_pBindPoseFrameMatrices[iFrame], mLocalWorld );

    // Transform our siblings
    if( m_pFrameArray[iFrame].SiblingFrame != INVALID_FRAME )
    {
        TransformBindPoseFrame( m_pFrameArray[iFrame].SiblingFrame, parentWorld );
    }

    // Transform our children
    if( m_pFrameArray[iFrame].ChildFrame != INVALID_FRAME )
    {
        TransformBindPoseFrame( m_pFrameArray[iFrame].ChildFrame, mLocalWorld );
    }
}


//--------------------------------------------------------------------------------------
// transform frame using a recursive traversal
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::TransformFrame( UINT iFrame, CXMMATRIX parentWorld, double fTime )
{
    // Get the tick data
    XMMATRIX mLocalTransform;

    UINT iTick = GetAnimationKeyFromTime( fTime );

    if( INVALID_ANIMATION_DATA != m_pFrameArray[iFrame].AnimationDataIndex )
    {
        auto pFrameData = &m_pAnimationFrameData[ m_pFrameArray[iFrame].AnimationDataIndex ];
        auto pData = &pFrameData->pAnimationData[ iTick ];

        // turn it into a matrix (Ignore scaling for now)
        XMFLOAT3 parentPos = pData->Translation;
        XMMATRIX mTranslate = XMMatrixTranslation( parentPos.x, parentPos.y, parentPos.z );

        XMVECTOR quat = XMVectorSet( pData->Orientation.x, pData->Orientation.y, pData->Orientation.z, pData->Orientation.w );
        if ( XMVector4Equal( quat, g_XMZero ) )
            quat = XMQuaternionIdentity();
        quat = XMQuaternionNormalize( quat );
        XMMATRIX mQuat = XMMatrixRotationQuaternion( quat );
        mLocalTransform = ( mQuat * mTranslate );
    }
    else
    {
        mLocalTransform = XMLoadFloat4x4( &m_pFrameArray[iFrame].Matrix );
    }

    // Transform ourselves
    XMMATRIX mLocalWorld = XMMatrixMultiply( mLocalTransform, parentWorld );
    XMStoreFloat4x4( &m_pTransformedFrameMatrices[iFrame], mLocalWorld );
    XMStoreFloat4x4( &m_pWorldPoseFrameMatrices[iFrame], mLocalWorld );

    // Transform our siblings
    if( m_pFrameArray[iFrame].SiblingFrame != INVALID_FRAME )
    {
        TransformFrame( m_pFrameArray[iFrame].SiblingFrame, parentWorld, fTime );
    }

    // Transform our children
    if( m_pFrameArray[iFrame].ChildFrame != INVALID_FRAME )
    {
        TransformFrame( m_pFrameArray[iFrame].ChildFrame, mLocalWorld, fTime );
    }
}


//--------------------------------------------------------------------------------------
// transform frame assuming that it is an absolute transformation
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::TransformFrameAbsolute( UINT iFrame, double fTime )
{
    UINT iTick = GetAnimationKeyFromTime( fTime );

    if( INVALID_ANIMATION_DATA != m_pFrameArray[iFrame].AnimationDataIndex )
    {
        auto pFrameData = &m_pAnimationFrameData[ m_pFrameArray[iFrame].AnimationDataIndex ];
        auto pData = &pFrameData->pAnimationData[ iTick ];
        auto pDataOrig = &pFrameData->pAnimationData[ 0 ];

        XMMATRIX mTrans1 = XMMatrixTranslation( -pDataOrig->Translation.x, -pDataOrig->Translation.y, -pDataOrig->Translation.z );
        XMMATRIX mTrans2 = XMMatrixTranslation( pData->Translation.x, pData->Translation.y, pData->Translation.z );

        XMVECTOR quat1 = XMVectorSet( pDataOrig->Orientation.x, pDataOrig->Orientation.y, pDataOrig->Orientation.z, pDataOrig->Orientation.w );
        quat1 = XMQuaternionInverse( quat1 );
        XMMATRIX mRot1 = XMMatrixRotationQuaternion( quat1 );
        XMMATRIX mInvTo = mTrans1 * mRot1;

        XMVECTOR quat2 = XMVectorSet( pData->Orientation.x, pData->Orientation.y, pData->Orientation.z, pData->Orientation.w );
        XMMATRIX mRot2 = XMMatrixRotationQuaternion( quat2 );
        XMMATRIX mFrom = mRot2 * mTrans2;

        XMMATRIX mOutput = mInvTo * mFrom;
        XMStoreFloat4x4( &m_pTransformedFrameMatrices[iFrame], mOutput );
    }
}

#define MAX_D3D11_VERTEX_STREAMS D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::RenderMesh( UINT iMesh,
                               bool bAdjacent,
                               ID3D11DeviceContext* pd3dDeviceContext,
                               UINT iDiffuseSlot,
                               UINT iNormalSlot,
                               UINT iSpecularSlot )
{
    if( 0 < GetOutstandingBufferResources() )
        return;

    auto pMesh = &m_pMeshArray[iMesh];

    UINT Strides[MAX_D3D11_VERTEX_STREAMS];
    UINT Offsets[MAX_D3D11_VERTEX_STREAMS];
    ID3D11Buffer* pVB[MAX_D3D11_VERTEX_STREAMS];

    if( pMesh->NumVertexBuffers > MAX_D3D11_VERTEX_STREAMS )
        return;

    for( UINT64 i = 0; i < pMesh->NumVertexBuffers; i++ )
    {
        pVB[i] = m_pVertexBufferArray[ pMesh->VertexBuffers[i] ].pVB11;
        Strides[i] = ( UINT )m_pVertexBufferArray[ pMesh->VertexBuffers[i] ].StrideBytes;
        Offsets[i] = 0;
    }

    SDKMESH_INDEX_BUFFER_HEADER* pIndexBufferArray;
    if( bAdjacent )
        pIndexBufferArray = m_pAdjacencyIndexBufferArray;
    else
        pIndexBufferArray = m_pIndexBufferArray;

    auto pIB = pIndexBufferArray[ pMesh->IndexBuffer ].pIB11;
    DXGI_FORMAT ibFormat = DXGI_FORMAT_R16_UINT;
    switch( pIndexBufferArray[ pMesh->IndexBuffer ].IndexType )
    {
    case IT_16BIT:
        ibFormat = DXGI_FORMAT_R16_UINT;
        break;
    case IT_32BIT:
        ibFormat = DXGI_FORMAT_R32_UINT;
        break;
    };

    pd3dDeviceContext->IASetVertexBuffers( 0, pMesh->NumVertexBuffers, pVB, Strides, Offsets );
    pd3dDeviceContext->IASetIndexBuffer( pIB, ibFormat, 0 );

    SDKMESH_SUBSET* pSubset = nullptr;
    SDKMESH_MATERIAL* pMat = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY PrimType;

    for( UINT subset = 0; subset < pMesh->NumSubsets; subset++ )
    {
        pSubset = &m_pSubsetArray[ pMesh->pSubsets[subset] ];

        PrimType = GetPrimitiveType11( ( SDKMESH_PRIMITIVE_TYPE )pSubset->PrimitiveType );
        if( bAdjacent )
        {
            switch( PrimType )
            {
            case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
                PrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
                break;
            case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
                PrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
                break;
            case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
                PrimType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
                break;
            case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:
                PrimType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
                break;
            }
        }

        pd3dDeviceContext->IASetPrimitiveTopology( PrimType );

        pMat = &m_pMaterialArray[ pSubset->MaterialID ];
        if( iDiffuseSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pDiffuseRV11 ) )
            pd3dDeviceContext->PSSetShaderResources( iDiffuseSlot, 1, &pMat->pDiffuseRV11 );
        if( iNormalSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pNormalRV11 ) )
            pd3dDeviceContext->PSSetShaderResources( iNormalSlot, 1, &pMat->pNormalRV11 );
        if( iSpecularSlot != INVALID_SAMPLER_SLOT && !IsErrorResource( pMat->pSpecularRV11 ) )
            pd3dDeviceContext->PSSetShaderResources( iSpecularSlot, 1, &pMat->pSpecularRV11 );

        UINT IndexCount = ( UINT )pSubset->IndexCount;
        UINT IndexStart = ( UINT )pSubset->IndexStart;
        UINT VertexStart = ( UINT )pSubset->VertexStart;
        if( bAdjacent )
        {
            IndexCount *= 2;
            IndexStart *= 2;
        }

        pd3dDeviceContext->DrawIndexed( IndexCount, IndexStart, VertexStart );
    }
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::RenderFrame( UINT iFrame,
                                bool bAdjacent,
                                ID3D11DeviceContext* pd3dDeviceContext,
                                UINT iDiffuseSlot,
                                UINT iNormalSlot,
                                UINT iSpecularSlot )
{
    if( !m_pStaticMeshData || !m_pFrameArray )
        return;

    if( m_pFrameArray[iFrame].Mesh != INVALID_MESH )
    {
        RenderMesh( m_pFrameArray[iFrame].Mesh,
                    bAdjacent,
                    pd3dDeviceContext,
                    iDiffuseSlot,
                    iNormalSlot,
                    iSpecularSlot );
    }

    // Render our children
    if( m_pFrameArray[iFrame].ChildFrame != INVALID_FRAME )
        RenderFrame( m_pFrameArray[iFrame].ChildFrame, bAdjacent, pd3dDeviceContext, iDiffuseSlot, 
                     iNormalSlot, iSpecularSlot );

    // Render our siblings
    if( m_pFrameArray[iFrame].SiblingFrame != INVALID_FRAME )
        RenderFrame( m_pFrameArray[iFrame].SiblingFrame, bAdjacent, pd3dDeviceContext, iDiffuseSlot, 
                     iNormalSlot, iSpecularSlot );
}

//--------------------------------------------------------------------------------------
CDXUTSDKMesh::CDXUTSDKMesh() : m_NumOutstandingResources( 0 ),
                               m_bLoading( false ),
                               m_hFile( 0 ),
                               m_hFileMappingObject( 0 ),
                               m_pMeshHeader( nullptr ),
                               m_pStaticMeshData( nullptr ),
                               m_pHeapData( nullptr ),
                               m_pAdjacencyIndexBufferArray( nullptr ),
                               m_pAnimationData( nullptr ),
                               m_pAnimationHeader( nullptr ),
                               m_ppVertices( nullptr ),
                               m_ppIndices( nullptr ),
                               m_pBindPoseFrameMatrices( nullptr ),
                               m_pTransformedFrameMatrices( nullptr ),
                               m_pWorldPoseFrameMatrices( nullptr ),
                               m_pDev11( nullptr )
{
}


//--------------------------------------------------------------------------------------
CDXUTSDKMesh::~CDXUTSDKMesh()
{
    Destroy();
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTSDKMesh::Create( ID3D11Device* pDev11, LPCWSTR szFileName, SDKMESH_CALLBACKS11* pLoaderCallbacks )
{
    return CreateFromFile( pDev11, szFileName, pLoaderCallbacks );
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT CDXUTSDKMesh::Create( ID3D11Device* pDev11, BYTE* pData, size_t DataBytes, bool bCopyStatic, SDKMESH_CALLBACKS11* pLoaderCallbacks )
{
    return CreateFromMemory( pDev11, pData, DataBytes, bCopyStatic, pLoaderCallbacks );
}


//--------------------------------------------------------------------------------------
HRESULT CDXUTSDKMesh::LoadAnimation( _In_z_ const WCHAR* szFileName )
{
    HRESULT hr = E_FAIL;
    DWORD dwBytesRead = 0;
    LARGE_INTEGER liMove;
    WCHAR strPath[MAX_PATH];

    // Find the path for the file
    V_RETURN( DXUTFindDXSDKMediaFileCch( strPath, MAX_PATH, szFileName ) );

    // Open the file
    HANDLE hFile = CreateFile( strPath, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_FLAG_SEQUENTIAL_SCAN, nullptr );
    if( INVALID_HANDLE_VALUE == hFile )
        return DXUTERR_MEDIANOTFOUND;

    /////////////////////////
    // Header
    SDKANIMATION_FILE_HEADER fileheader;
    if( !ReadFile( hFile, &fileheader, sizeof( SDKANIMATION_FILE_HEADER ), &dwBytesRead, nullptr ) )
        goto Error;

    //allocate
    m_pAnimationData = new (std::nothrow) BYTE[ ( size_t )( sizeof( SDKANIMATION_FILE_HEADER ) + fileheader.AnimationDataSize ) ];
    if( !m_pAnimationData )
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // read it all in
    liMove.QuadPart = 0;
    if( !SetFilePointerEx( hFile, liMove, nullptr, FILE_BEGIN ) )
        goto Error;
    if( !ReadFile( hFile, m_pAnimationData, ( DWORD )( sizeof( SDKANIMATION_FILE_HEADER ) +
                                                       fileheader.AnimationDataSize ), &dwBytesRead, nullptr ) )
        goto Error;

    // pointer fixup
    m_pAnimationHeader = ( SDKANIMATION_FILE_HEADER* )m_pAnimationData;
    m_pAnimationFrameData = ( SDKANIMATION_FRAME_DATA* )( m_pAnimationData + m_pAnimationHeader->AnimationDataOffset );

    UINT64 BaseOffset = sizeof( SDKANIMATION_FILE_HEADER );
    for( UINT i = 0; i < m_pAnimationHeader->NumFrames; i++ )
    {
        m_pAnimationFrameData[i].pAnimationData = ( SDKANIMATION_DATA* )( m_pAnimationData +
                                                                          m_pAnimationFrameData[i].DataOffset +
                                                                          BaseOffset );
        auto pFrame = FindFrame( m_pAnimationFrameData[i].FrameName );
        if( pFrame )
        {
            pFrame->AnimationDataIndex = i;
        }
    }

    hr = S_OK;
Error:
    CloseHandle( hFile );
    return hr;
}

//--------------------------------------------------------------------------------------
void CDXUTSDKMesh::Destroy()
{
    if( !CheckLoadDone() )
        return;

    if( m_pStaticMeshData )
    {
        if( m_pMaterialArray )
        {
            for( UINT64 m = 0; m < m_pMeshHeader->NumMaterials; m++ )
            {
                if( m_pDev11 )
                {
                    if( m_pMaterialArray[m].pDiffuseRV11 && !IsErrorResource( m_pMaterialArray[m].pDiffuseRV11 ) )
                    {
                        //m_pMaterialArray[m].pDiffuseRV11->GetResource( &pRes );
                        //SAFE_RELEASE( pRes );

                        SAFE_RELEASE( m_pMaterialArray[m].pDiffuseRV11 );
                    }
                    if( m_pMaterialArray[m].pNormalRV11 && !IsErrorResource( m_pMaterialArray[m].pNormalRV11 ) )
                    {
                        //m_pMaterialArray[m].pNormalRV11->GetResource( &pRes );
                        //SAFE_RELEASE( pRes );

                        SAFE_RELEASE( m_pMaterialArray[m].pNormalRV11 );
                    }
                    if( m_pMaterialArray[m].pSpecularRV11 && !IsErrorResource( m_pMaterialArray[m].pSpecularRV11 ) )
                    {
                        //m_pMaterialArray[m].pSpecularRV11->GetResource( &pRes );
                        //SAFE_RELEASE( pRes );

                        SAFE_RELEASE( m_pMaterialArray[m].pSpecularRV11 );
                    }
                }
            }
        }
        for( UINT64 i = 0; i < m_pMeshHeader->NumVertexBuffers; i++ )
        {
            SAFE_RELEASE( m_pVertexBufferArray[i].pVB11 );
        }

        for( UINT64 i = 0; i < m_pMeshHeader->NumIndexBuffers; i++ )
        {
            SAFE_RELEASE( m_pIndexBufferArray[i].pIB11 );
        }
    }

    if( m_pAdjacencyIndexBufferArray )
    {
        for( UINT64 i = 0; i < m_pMeshHeader->NumIndexBuffers; i++ )
        {
            SAFE_RELEASE( m_pAdjacencyIndexBufferArray[i].pIB11 );
        }
    }
    SAFE_DELETE_ARRAY( m_pAdjacencyIndexBufferArray );

    SAFE_DELETE_ARRAY( m_pHeapData );
    m_pStaticMeshData = nullptr;
    SAFE_DELETE_ARRAY( m_pAnimationData );
    SAFE_DELETE_ARRAY( m_pBindPoseFrameMatrices );
    SAFE_DELETE_ARRAY( m_pTransformedFrameMatrices );
    SAFE_DELETE_ARRAY( m_pWorldPoseFrameMatrices );

    SAFE_DELETE_ARRAY( m_ppVertices );
    SAFE_DELETE_ARRAY( m_ppIndices );

    m_pMeshHeader = nullptr;
    m_pVertexBufferArray = nullptr;
    m_pIndexBufferArray = nullptr;
    m_pMeshArray = nullptr;
    m_pSubsetArray = nullptr;
    m_pFrameArray = nullptr;
    m_pMaterialArray = nullptr;

    m_pAnimationHeader = nullptr;
    m_pAnimationFrameData = nullptr;

}


//--------------------------------------------------------------------------------------
// transform the mesh frames according to the animation for time fTime
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::TransformMesh( CXMMATRIX world, double fTime )
{
    if( !m_pAnimationHeader || FTT_RELATIVE == m_pAnimationHeader->FrameTransformType )
    {
        TransformFrame( 0, world, fTime );

        // For each frame, move the transform to the bind pose, then
        // move it to the final position
        for( UINT i = 0; i < m_pMeshHeader->NumFrames; i++ )
        {
            XMMATRIX m = XMLoadFloat4x4( &m_pBindPoseFrameMatrices[i] );
            XMMATRIX mInvBindPose = XMMatrixInverse( nullptr, m );
            m = XMLoadFloat4x4( &m_pTransformedFrameMatrices[i] );
            XMMATRIX mFinal = mInvBindPose * m;
            XMStoreFloat4x4( &m_pTransformedFrameMatrices[i], mFinal );
        }
    }
    else if( FTT_ABSOLUTE == m_pAnimationHeader->FrameTransformType )
    {
        for( UINT i = 0; i < m_pAnimationHeader->NumFrames; i++ )
            TransformFrameAbsolute( i, fTime );
    }
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::Render( ID3D11DeviceContext* pd3dDeviceContext,
                           UINT iDiffuseSlot,
                           UINT iNormalSlot,
                           UINT iSpecularSlot )
{
    RenderFrame( 0, false, pd3dDeviceContext, iDiffuseSlot, iNormalSlot, iSpecularSlot );
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
void CDXUTSDKMesh::RenderAdjacent( ID3D11DeviceContext* pd3dDeviceContext,
                                   UINT iDiffuseSlot,
                                   UINT iNormalSlot,
                                   UINT iSpecularSlot )
{
    RenderFrame( 0, true, pd3dDeviceContext, iDiffuseSlot, iNormalSlot, iSpecularSlot );
}


//--------------------------------------------------------------------------------------
D3D11_PRIMITIVE_TOPOLOGY CDXUTSDKMesh::GetPrimitiveType11( _In_ SDKMESH_PRIMITIVE_TYPE PrimType )
{
    D3D11_PRIMITIVE_TOPOLOGY retType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    switch( PrimType )
    {
        case PT_TRIANGLE_LIST:
            retType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case PT_TRIANGLE_STRIP:
            retType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        case PT_LINE_LIST:
            retType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case PT_LINE_STRIP:
            retType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case PT_POINT_LIST:
            retType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case PT_TRIANGLE_LIST_ADJ:
            retType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
            break;
        case PT_TRIANGLE_STRIP_ADJ:
            retType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
            break;
        case PT_LINE_LIST_ADJ:
            retType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
            break;
        case PT_LINE_STRIP_ADJ:
            retType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
            break;
    };

    return retType;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT CDXUTSDKMesh::GetIBFormat11( _In_ UINT iMesh ) const
{
    switch( m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].IndexType )
    {
        case IT_16BIT:
            return DXGI_FORMAT_R16_UINT;
        case IT_32BIT:
            return DXGI_FORMAT_R32_UINT;
    };
    return DXGI_FORMAT_R16_UINT;
}

//--------------------------------------------------------------------------------------
ID3D11Buffer* CDXUTSDKMesh::GetVB11( _In_ UINT iMesh, _In_ UINT iVB ) const
{
    return m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].pVB11;
}

//--------------------------------------------------------------------------------------
ID3D11Buffer* CDXUTSDKMesh::GetIB11( _In_ UINT iMesh ) const
{
    return m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].pIB11;
}
SDKMESH_INDEX_TYPE CDXUTSDKMesh::GetIndexType( _In_ UINT iMesh ) const
{
    return ( SDKMESH_INDEX_TYPE ) m_pIndexBufferArray[m_pMeshArray[ iMesh ].IndexBuffer].IndexType;
}
//--------------------------------------------------------------------------------------
ID3D11Buffer* CDXUTSDKMesh::GetAdjIB11( _In_ UINT iMesh ) const
{
    return m_pAdjacencyIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].pIB11;
}

//--------------------------------------------------------------------------------------
const char* CDXUTSDKMesh::GetMeshPathA() const
{
    return m_strPath;
}

//--------------------------------------------------------------------------------------
const WCHAR* CDXUTSDKMesh::GetMeshPathW() const
{
    return m_strPathW;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumMeshes() const
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumMeshes;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumMaterials() const
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumMaterials;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumVBs() const
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumVertexBuffers;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumIBs() const
{
    if( !m_pMeshHeader )
        return 0;
    return m_pMeshHeader->NumIndexBuffers;
}

//--------------------------------------------------------------------------------------
ID3D11Buffer* CDXUTSDKMesh::GetVB11At( _In_ UINT iVB ) const
{
    return m_pVertexBufferArray[ iVB ].pVB11;
}

//--------------------------------------------------------------------------------------
ID3D11Buffer* CDXUTSDKMesh::GetIB11At( _In_ UINT iIB ) const
{
    return m_pIndexBufferArray[ iIB ].pIB11;
}

//--------------------------------------------------------------------------------------
BYTE* CDXUTSDKMesh::GetRawVerticesAt( _In_ UINT iVB ) const
{
    return m_ppVertices[iVB];
}

//--------------------------------------------------------------------------------------
BYTE* CDXUTSDKMesh::GetRawIndicesAt( _In_ UINT iIB ) const
{
    return m_ppIndices[iIB];
}

//--------------------------------------------------------------------------------------
SDKMESH_MATERIAL* CDXUTSDKMesh::GetMaterial( _In_ UINT iMaterial ) const
{
    return &m_pMaterialArray[ iMaterial ];
}

//--------------------------------------------------------------------------------------
SDKMESH_MESH* CDXUTSDKMesh::GetMesh( _In_ UINT iMesh ) const
{
    return &m_pMeshArray[ iMesh ];
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumSubsets( _In_ UINT iMesh ) const
{
    return m_pMeshArray[ iMesh ].NumSubsets;
}

//--------------------------------------------------------------------------------------
SDKMESH_SUBSET* CDXUTSDKMesh::GetSubset( _In_ UINT iMesh, _In_ UINT iSubset ) const
{
    return &m_pSubsetArray[ m_pMeshArray[ iMesh ].pSubsets[iSubset] ];
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetVertexStride( _In_ UINT iMesh, _In_ UINT iVB ) const
{
    return ( UINT )m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].StrideBytes;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumFrames() const
{
    return m_pMeshHeader->NumFrames;
}

//--------------------------------------------------------------------------------------
SDKMESH_FRAME* CDXUTSDKMesh::GetFrame( _In_ UINT iFrame ) const
{
    assert( iFrame < m_pMeshHeader->NumFrames );
    return &m_pFrameArray[ iFrame ];
}

//--------------------------------------------------------------------------------------
SDKMESH_FRAME* CDXUTSDKMesh::FindFrame( _In_z_ const char* pszName ) const
{
    for( UINT i = 0; i < m_pMeshHeader->NumFrames; i++ )
    {
        if( _stricmp( m_pFrameArray[i].Name, pszName ) == 0 )
        {
            return &m_pFrameArray[i];
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------
UINT64 CDXUTSDKMesh::GetNumVertices( _In_ UINT iMesh, _In_ UINT iVB ) const
{
    return m_pVertexBufferArray[ m_pMeshArray[ iMesh ].VertexBuffers[iVB] ].NumVertices;
}

//--------------------------------------------------------------------------------------
UINT64 CDXUTSDKMesh::GetNumIndices( _In_ UINT iMesh ) const
{
    return m_pIndexBufferArray[ m_pMeshArray[ iMesh ].IndexBuffer ].NumIndices;
}

//--------------------------------------------------------------------------------------
XMVECTOR CDXUTSDKMesh::GetMeshBBoxCenter( _In_ UINT iMesh ) const
{
    return XMLoadFloat3( &m_pMeshArray[iMesh].BoundingBoxCenter );
}

//--------------------------------------------------------------------------------------
XMVECTOR CDXUTSDKMesh::GetMeshBBoxExtents( _In_ UINT iMesh ) const
{
    return XMLoadFloat3( &m_pMeshArray[iMesh].BoundingBoxExtents );
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetOutstandingResources() const
{
    UINT outstandingResources = 0;
    if( !m_pMeshHeader )
        return 1;

    outstandingResources += GetOutstandingBufferResources();

    if( m_pDev11 )
    {
        for( UINT i = 0; i < m_pMeshHeader->NumMaterials; i++ )
        {
            if( m_pMaterialArray[i].DiffuseTexture[0] != 0 )
            {
                if( !m_pMaterialArray[i].pDiffuseRV11 && !IsErrorResource( m_pMaterialArray[i].pDiffuseRV11 ) )
                    outstandingResources ++;
            }

            if( m_pMaterialArray[i].NormalTexture[0] != 0 )
            {
                if( !m_pMaterialArray[i].pNormalRV11 && !IsErrorResource( m_pMaterialArray[i].pNormalRV11 ) )
                    outstandingResources ++;
            }

            if( m_pMaterialArray[i].SpecularTexture[0] != 0 )
            {
                if( !m_pMaterialArray[i].pSpecularRV11 && !IsErrorResource( m_pMaterialArray[i].pSpecularRV11 ) )
                    outstandingResources ++;
            }
        }
    }

    return outstandingResources;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetOutstandingBufferResources() const
{
    UINT outstandingResources = 0;
    if( !m_pMeshHeader )
        return 1;

    return outstandingResources;
}

//--------------------------------------------------------------------------------------
bool CDXUTSDKMesh::CheckLoadDone()
{
    if( 0 == GetOutstandingResources() )
    {
        m_bLoading = false;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------
bool CDXUTSDKMesh::IsLoaded() const
{
    if( m_pStaticMeshData && !m_bLoading )
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------
bool CDXUTSDKMesh::IsLoading() const
{
    return m_bLoading;
}

//--------------------------------------------------------------------------------------
void CDXUTSDKMesh::SetLoading( _In_ bool bLoading )
{
    m_bLoading = bLoading;
}

//--------------------------------------------------------------------------------------
BOOL CDXUTSDKMesh::HadLoadingError() const
{
    return FALSE;
}

//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetNumInfluences( _In_ UINT iMesh ) const
{
    return m_pMeshArray[iMesh].NumFrameInfluences;
}

//--------------------------------------------------------------------------------------
XMMATRIX CDXUTSDKMesh::GetMeshInfluenceMatrix( _In_ UINT iMesh, _In_ UINT iInfluence ) const
{
    UINT iFrame = m_pMeshArray[iMesh].pFrameInfluences[ iInfluence ];
    return XMLoadFloat4x4( &m_pTransformedFrameMatrices[iFrame] );
}

XMMATRIX CDXUTSDKMesh::GetWorldMatrix( _In_ UINT iFrameIndex ) const
{
    return XMLoadFloat4x4( &m_pWorldPoseFrameMatrices[iFrameIndex] );
}

XMMATRIX CDXUTSDKMesh::GetInfluenceMatrix( _In_ UINT iFrameIndex ) const
{
    return XMLoadFloat4x4( &m_pTransformedFrameMatrices[iFrameIndex] );
}


//--------------------------------------------------------------------------------------
UINT CDXUTSDKMesh::GetAnimationKeyFromTime( _In_ double fTime ) const
{
    if( !m_pAnimationHeader )
    {
        return 0;
    }

    UINT iTick = ( UINT )( m_pAnimationHeader->AnimationFPS * fTime );

    iTick = iTick % ( m_pAnimationHeader->NumAnimationKeys - 1 );
    iTick ++;

    return iTick;
}

_Use_decl_annotations_
bool CDXUTSDKMesh::GetAnimationProperties( UINT* pNumKeys, float* pFrameTime ) const
{
    if( !m_pAnimationHeader )
    {
        *pNumKeys = 0;
        *pFrameTime = 0;
        return false;
    }

    *pNumKeys = m_pAnimationHeader->NumAnimationKeys;
    *pFrameTime = 1.0f / (float)m_pAnimationHeader->AnimationFPS;

    return true;
}
