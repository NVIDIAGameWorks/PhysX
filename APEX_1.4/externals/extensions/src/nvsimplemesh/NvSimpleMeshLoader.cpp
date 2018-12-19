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

#pragma warning(push)
#pragma warning(disable: 4100)	// unreferenced formal parameter
#include "assimp\assimp.hpp"
#pragma warning(pop)
#include "assimp\aiScene.h"
#include "assimp\DefaultLogger.h"
#include "assimp\aiPostProcess.h"
#include "assimp\aiMesh.h"

using namespace Assimp;

#include "NvSimpleRawMesh.h"
#include "NvSimpleMeshLoader.h"


NvSimpleMeshLoader::NvSimpleMeshLoader()
{
    pMeshes = NULL;
    NumMeshes = 0;
}

NvSimpleMeshLoader::~NvSimpleMeshLoader()
{
    SAFE_DELETE_ARRAY(pMeshes);
}

bool NvSimpleMeshLoader::LoadFile(LPWSTR szFilename)
{
    bool bLoaded = false;
    (void)bLoaded;

    // Create a logger instance 
    Assimp::DefaultLogger::create("",Logger::VERBOSE);

    // Create an instance of the Importer class
    Assimp::Importer importer;

    CHAR szFilenameA[MAX_PATH];
    WideCharToMultiByte(CP_ACP,0,szFilename,MAX_PATH,szFilenameA,MAX_PATH,NULL,false);

    mediaPath = szFilenameA;
    auto index = mediaPath.find_last_of('\\');
    if(index != -1)
        mediaPath = mediaPath.substr(0,index) + "\\";
    else
        mediaPath = ".\\";

    // Set some flags for the removal of various data that we don't use
    importer.SetPropertyInteger("AI_CONFIG_PP_RVC_FLAGS",aiComponent_ANIMATIONS | aiComponent_BONEWEIGHTS | aiComponent_COLORS | aiComponent_LIGHTS | aiComponent_CAMERAS);
    //importer.SetPropertyInteger("AI_CONFIG_PP_SBP_REMOVE",aiPrimitiveType_POINTS | aiPrimitiveType_LINES );

    // load the scene and preprocess it into the form we want
    const aiScene *scene = importer.ReadFile(szFilenameA,
                                                aiProcess_Triangulate|    // only higher order primitives will be triangulated
                                                aiProcess_GenNormals|    // if normals exist, will not be generated
                                                aiProcess_CalcTangentSpace|
                                                aiProcess_PreTransformVertices| // rolls all node hierarchy(if existant) into the local space of meshes
                                                //aiProcess_RemoveRedundantMaterials|
                                                //aiProcess_FixInfacingNormals|
                                                aiProcess_FindDegenerates|
                                                aiProcess_SortByPType|
                                                aiProcess_RemoveComponent|    // processes the above flags set to remove data we don't want
                                                aiProcess_FindInvalidData|
                                                aiProcess_GenUVCoords|
                                                aiProcess_TransformUVCoords|
                                                aiProcess_OptimizeMeshes |

                                                aiProcessPreset_TargetRealtime_Quality
                                                );

    // can't load?
    if(!scene)
        return false;

    if(scene->HasMeshes())
    {
        pMeshes = new NvSimpleRawMesh[scene->mNumMeshes];
        NumMeshes = scene->mNumMeshes;

        DirectX::XMMATRIX identity = DirectX::XMMatrixIdentity();

        RecurseAddMeshes(scene,scene->mRootNode,identity,true);
    }

    // cleanup
    Assimp::DefaultLogger::kill();

    return NumMeshes > 0;
}

void XM_CALLCONV NvSimpleMeshLoader::RecurseAddMeshes(const aiScene *scene, aiNode*pNode, DirectX::FXMMATRIX parentCompositeTransformD3D, bool bFlattenTransforms)
{
    // Convert the scene node's transform into DirectXMath, transposing it as we go
    aiMatrix4x4* paiNodeTrans = &(pNode->mTransformation);
    DirectX::XMFLOAT4X4 LocalFrameTransformXM(paiNodeTrans->a1, paiNodeTrans->b1, paiNodeTrans->c1, paiNodeTrans->d1,
                                              paiNodeTrans->a2, paiNodeTrans->b2, paiNodeTrans->c2, paiNodeTrans->d2,
                                              paiNodeTrans->a3, paiNodeTrans->b3, paiNodeTrans->c3, paiNodeTrans->d3,
                                              paiNodeTrans->a4, paiNodeTrans->b4, paiNodeTrans->c4, paiNodeTrans->d4);

    DirectX::XMMATRIX LocalFrameTransformD3D = DirectX::XMLoadFloat4x4(&LocalFrameTransformXM);
    DirectX::XMMATRIX LocalCompositeTransformD3D = DirectX::XMMatrixMultiply(LocalFrameTransformD3D, parentCompositeTransformD3D);


    DirectX::XMMATRIX* pActiveTransform = &LocalFrameTransformD3D;
    if(bFlattenTransforms) pActiveTransform = &LocalCompositeTransformD3D;

    if(pNode->mNumMeshes > 0)
    {
        for(int iSubMesh=0;iSubMesh < (int)pNode->mNumMeshes;iSubMesh++)
        {
            aiMesh *pMesh = scene->mMeshes[pNode->mMeshes[iSubMesh]];
            NvSimpleRawMesh &activeMesh = pMeshes[pNode->mMeshes[iSubMesh]];    // we'll use the same ordering as the aiScene

            float emin[3]; ::ZeroMemory(emin,3*sizeof(float));
            float emax[3]; ::ZeroMemory(emax,3*sizeof(float));

            if(pMesh->HasPositions() && pMesh->HasNormals() && pMesh->HasTextureCoords(0) && pMesh->HasTangentsAndBitangents())
            {
                activeMesh.m_iNumIndices = pMesh->mNumFaces*3;
                activeMesh.m_iNumVertices = pMesh->mNumVertices;

                // copy loaded mesh data into our vertex struct
                activeMesh.m_pVertexData = new NvSimpleRawMesh::Vertex[pMesh->mNumVertices];
                for(unsigned int i=0;i<pMesh->mNumVertices;i++)
                {
                    memcpy((void*)&(activeMesh.m_pVertexData[i].Position),(void*)&(pMesh->mVertices[i]),sizeof(aiVector3D));
                    memcpy((void*)&(activeMesh.m_pVertexData[i].Normal),(void*)&(pMesh->mNormals[i]),sizeof(aiVector3D));
                    memcpy((void*)&(activeMesh.m_pVertexData[i].Tangent),(void*)&(pMesh->mTangents[i]),sizeof(aiVector3D));
                    memcpy((void*)&(activeMesh.m_pVertexData[i].UV),(void*)&(pMesh->mTextureCoords[0][i]),sizeof(aiVector2D));

                    for(int m=0;m<3;m++)
                    {
                        emin[m] = min(emin[m],activeMesh.m_pVertexData[i].Position[m]);
                        emax[m] = max(emin[m],activeMesh.m_pVertexData[i].Position[m]);
                    }
                }

                // create an index buffer
                activeMesh.m_IndexSize = sizeof(UINT16);
                if(pMesh->mNumFaces > MAXINT16)
                    activeMesh.m_IndexSize = sizeof(UINT32);

                activeMesh.m_pIndexData = new BYTE[pMesh->mNumFaces * 3 * activeMesh.m_IndexSize];
                for(unsigned int i=0;i<pMesh->mNumFaces;i++)
                {
                    assert(pMesh->mFaces[i].mNumIndices == 3);
                    if(activeMesh.m_IndexSize == sizeof(UINT32))
                    {
                        memcpy((void*)&(activeMesh.m_pIndexData[i*3*activeMesh.m_IndexSize]),(void*)pMesh->mFaces[i].mIndices,sizeof(3*activeMesh.m_IndexSize));
                    }
                    else    // 16 bit indices
                    {
                        UINT16*pFaceIndices = (UINT16*)&(activeMesh.m_pIndexData[i*3*activeMesh.m_IndexSize]);
                        pFaceIndices[0] = (UINT16)pMesh->mFaces[i].mIndices[0];
                        pFaceIndices[1] = (UINT16)pMesh->mFaces[i].mIndices[1];
                        pFaceIndices[2] = (UINT16)pMesh->mFaces[i].mIndices[2];
                    }
                }

                // assign extents
                activeMesh.m_extents[0] = (emax[0] - emin[0]) * 0.5f;
                activeMesh.m_extents[1] = (emax[1] - emin[1]) * 0.5f;
                activeMesh.m_extents[2] = (emax[2] - emin[2]) * 0.5f;

                // get the center
                activeMesh.m_center[0] = emin[0] + activeMesh.m_extents[0];
                activeMesh.m_center[1] = emin[1] + activeMesh.m_extents[1];
                activeMesh.m_center[2] = emin[2] + activeMesh.m_extents[2];

                // materials
                if(scene->HasMaterials())
                {
                    aiMaterial *pMaterial = scene->mMaterials[pMesh->mMaterialIndex];

                    if(pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
                    {
                        aiString texPath;
                        pMaterial->GetTexture(aiTextureType_DIFFUSE,0,&texPath);

                        CHAR szFilenameA[MAX_PATH];
                        StringCchCopyA(szFilenameA,MAX_PATH,texPath.data);
                        MultiByteToWideChar(CP_ACP,0,szFilenameA,MAX_PATH,activeMesh.m_szDiffuseTexture,MAX_PATH);

                        //WCHAR szFullPath[MAX_PATH];
                        //if(DXUTFindDXSDKMediaFileCch(szFullPath,MAX_PATH,activeMesh.m_szDiffuseTexture) != S_OK)
                        {
                            std::string qualifiedPath = mediaPath + texPath.data;

                            StringCchCopyA(szFilenameA,MAX_PATH,qualifiedPath.c_str());
                            MultiByteToWideChar(CP_ACP,0,szFilenameA,MAX_PATH,activeMesh.m_szDiffuseTexture,MAX_PATH);
                        }    
                    }
                    if(pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
                    {
                        aiString texPath;
                        pMaterial->GetTexture(aiTextureType_NORMALS,0,&texPath);

                        CHAR szFilenameA[MAX_PATH];
                        StringCchCopyA(szFilenameA,MAX_PATH,texPath.data);
                        MultiByteToWideChar(CP_ACP,0,szFilenameA,MAX_PATH,activeMesh.m_szNormalTexture,MAX_PATH);

                        //WCHAR szFullPath[MAX_PATH];
                        //if(DXUTFindDXSDKMediaFileCch(szFullPath,MAX_PATH,activeMesh.m_szNormalTexture) != S_OK)
                        {
                            std::string qualifiedPath = mediaPath + texPath.data;

                            StringCchCopyA(szFilenameA,MAX_PATH,qualifiedPath.c_str());
                            MultiByteToWideChar(CP_ACP,0,szFilenameA,MAX_PATH,activeMesh.m_szNormalTexture,MAX_PATH);
                        }    
                    }
                }
            }
        }
    }

    for(int iChild=0;iChild<(int)pNode->mNumChildren;iChild++)
    {
        RecurseAddMeshes(scene, pNode->mChildren[iChild],LocalCompositeTransformD3D,bFlattenTransforms);
    }
}
