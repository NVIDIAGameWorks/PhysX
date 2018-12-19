// TAGRELEASE: PUBLIC
#pragma once
#include <string>
#include <DirectXMath.h>

struct aiScene;
struct aiNode;

class NvSimpleRawMesh;

/*
    Allow loading of various mesh file formats and then simple extraction of various vertex/index streams
*/
class NvSimpleMeshLoader
{
public:

    NvSimpleMeshLoader();
    ~NvSimpleMeshLoader();

    bool LoadFile(LPWSTR szFilename);
    void XM_CALLCONV RecurseAddMeshes(const aiScene *scene, aiNode*pNode, DirectX::FXMMATRIX parentCompositeTransformD3D, bool bFlattenTransforms);

    int NumMeshes;
    NvSimpleRawMesh *pMeshes;
        
protected:

    std::string mediaPath;
};
