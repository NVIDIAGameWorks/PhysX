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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#pragma warning(disable:4702) // disabling a warning that only shows up when building VC7

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#pragma  warning(disable:4555)
#include "MeshImport.h"
#include "ImportEzm.h"
#include "MiStringDictionary.h"
#include "MiSutil.h"
#include "MiStringTable.h"
#include "MiAsc2Bin.h"
#include "MiInparser.h"
#include "MiFastXml.h"
#include "MiMemoryBuffer.h"

#pragma warning(disable:4100)
#pragma warning(disable:4996)

namespace mimp
{

#define DEBUG_LOG 0

    enum NodeType
    {
        NT_NONE,
        NT_SCENE_GRAPH,
        NT_MESH,
        NT_ANIMATION,
        NT_SKELETON,
        NT_BONE,
        NT_MESH_SECTION,
        NT_VERTEX_BUFFER,
        NT_INDEX_BUFFER,
        NT_NODE_TRIANGLE,
        NT_NODE_INSTANCE,
        NT_ANIM_TRACK,
        NT_MESH_SYSTEM,
        NT_MESH_AABB,
        NT_SKELETONS,
        NT_ANIMATIONS,
        NT_MATERIALS,
        NT_MATERIAL,
        NT_MESHES,
        NT_MESH_COLLISION_REPRESENTATIONS,
        NT_MESH_COLLISION_REPRESENTATION,
        NT_MESH_COLLISION,
        NT_MESH_COLLISION_CONVEX,
        NT_LAST
    };

    enum AttributeType
    {
        AT_NONE,
        AT_ASSET_NAME,
        AT_ASSET_INFO,
        AT_NAME,
        AT_COUNT,
        AT_TRIANGLE_COUNT,
        AT_PARENT,
        AT_MATERIAL,
        AT_CTYPE,
        AT_SEMANTIC,
        AT_POSITION,
        AT_ORIENTATION,
        AT_DURATION,
        AT_DTIME,
        AT_TRACK_COUNT,
        AT_FRAME_COUNT,
        AT_HAS_SCALE,
        AT_MESH_SYSTEM_VERSION,
        AT_MESH_SYSTEM_ASSET_VERSION,
        AT_MIN,
        AT_MAX,
        AT_SCALE,
        AT_META_DATA,
        AT_SKELETON,
        AT_SUBMESH_COUNT,
        AT_INFO,
        AT_TYPE,
        AT_TRANSFORM,
        AT_LAST
    };

    class MeshImportEZM : public MeshImporter, public FastXml::Callback
    {
    public:
        MeshImportEZM(void)
        {
            mType     = NT_NONE;
            mBone     = 0;
            mFrameCount = 0;
            mDuration   = 0;
            mTrackCount = 0;
            mDtime      = 0;
            mTrackIndex = 0;
            mVertexFlags = 0;


            mToElement.Add("SceneGraph",                       NT_SCENE_GRAPH);
            mToElement.Add("Mesh",                             NT_MESH);
            mToElement.Add("Animation",                        NT_ANIMATION);
            mToElement.Add("Skeleton",                         NT_SKELETON);
            mToElement.Add("Bone",                             NT_BONE);
            mToElement.Add("MeshSection",                      NT_MESH_SECTION);
            mToElement.Add("VertexBuffer",                     NT_VERTEX_BUFFER);
            mToElement.Add("IndexBuffer",                      NT_INDEX_BUFFER);
            mToElement.Add("NodeTriangle",                     NT_NODE_TRIANGLE);
            mToElement.Add("NodeInstance",                     NT_NODE_INSTANCE);
            mToElement.Add("AnimTrack",                        NT_ANIM_TRACK);
            mToElement.Add("MeshSystem",                       NT_MESH_SYSTEM);
            mToElement.Add("MeshAABB",                         NT_MESH_AABB);
            mToElement.Add("Skeletons",                        NT_SKELETONS);
            mToElement.Add("Animations",                       NT_ANIMATIONS);
            mToElement.Add("Materials",                        NT_MATERIALS);
            mToElement.Add("Material",                         NT_MATERIAL);
            mToElement.Add("Meshes",                           NT_MESHES);
            mToElement.Add("MeshCollisionRepresentations",     NT_MESH_COLLISION_REPRESENTATIONS);
            mToElement.Add("MeshCollisionRepresentation",      NT_MESH_COLLISION_REPRESENTATION);
            mToElement.Add("MeshCollision",                    NT_MESH_COLLISION);
            mToElement.Add("MeshCollisionConvex",              NT_MESH_COLLISION_CONVEX);

            mToAttribute.Add("name",                           AT_NAME);
            mToAttribute.Add("count",                          AT_COUNT);
            mToAttribute.Add("triangle_count",                 AT_TRIANGLE_COUNT);
            mToAttribute.Add("parent",                         AT_PARENT);
            mToAttribute.Add("material",                       AT_MATERIAL);
            mToAttribute.Add("ctype",                          AT_CTYPE);
            mToAttribute.Add("semantic",                       AT_SEMANTIC);
            mToAttribute.Add("position",                       AT_POSITION);
            mToAttribute.Add("orientation",                    AT_ORIENTATION);
            mToAttribute.Add("duration",                       AT_DURATION);
            mToAttribute.Add("dtime",                          AT_DTIME);
            mToAttribute.Add("trackcount",                     AT_TRACK_COUNT);
            mToAttribute.Add("framecount",                     AT_FRAME_COUNT);
            mToAttribute.Add("has_scale",                      AT_HAS_SCALE);
            mToAttribute.Add("asset_name",                     AT_ASSET_NAME);
            mToAttribute.Add("asset_info",                     AT_ASSET_INFO);
            mToAttribute.Add("mesh_system_version",            AT_MESH_SYSTEM_VERSION);
            mToAttribute.Add("mesh_system_asset_version",                  AT_MESH_SYSTEM_ASSET_VERSION);
            mToAttribute.Add("min", AT_MIN);
            mToAttribute.Add("max", AT_MAX);
            mToAttribute.Add("scale", AT_SCALE);
            mToAttribute.Add("meta_data", AT_META_DATA);
            mToAttribute.Add("skeleton", AT_SKELETON);
            mToAttribute.Add("submesh_count", AT_SUBMESH_COUNT);
            mToAttribute.Add("info", AT_INFO);
            mToAttribute.Add("type", AT_TYPE);
            mToAttribute.Add("transform", AT_TRANSFORM);

            mHasScale      = false;
            mName          = 0;
            mCount         = 0;
            mParent        = 0;
            mCtype         = 0;
            mSemantic      = 0;
            mSkeleton      = 0;
            mBoneIndex     = 0;
            mIndexBuffer   = 0;
            mVertexBuffer  = 0;
            mVertices      = NULL;
            mVertexCount   = 0;
            mIndexCount    = 0;
            mAnimTrack     = 0;
            mAnimation     = 0;
            mMeshSystemVersion = 0;
            mMeshSystemAssetVersion = 0;
            mMeshCollisionRepresentation = 0;
            mMeshCollision = 0;
            mMeshCollisionConvex = 0;

        }

        virtual ~MeshImportEZM(void)
        {

        }

        const MiU8 * getVertex(const MiU8 *src,MeshVertex &v,const char **types,MiI32 tcount)
        {
            bool firstTexel =true;

            for (MiI32 i=0; i<tcount; i++)
            {
                const char *type = types[i];
                if ( MESH_IMPORT_STRING::stricmp(type,"position") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mPos[0] = scan[0];
                    v.mPos[1] = scan[1];
                    v.mPos[2] = scan[2];
                    src+=sizeof(MiF32)*3;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"normal") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mNormal[0] = scan[0];
                    v.mNormal[1] = scan[1];
                    v.mNormal[2] = scan[2];
                    src+=sizeof(MiF32)*3;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"color") == 0 )
                {
                    const MiU32 *scan = (const MiU32 *)src;
                    v.mColor = scan[0];
                    src+=sizeof(MiU32);
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"texcoord") == 0 || MESH_IMPORT_STRING::stricmp(type,"texcoord1") == 0 || MESH_IMPORT_STRING::stricmp(type,"texel1") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    if ( firstTexel )
                    {
                        v.mTexel1[0] = scan[0];
                        v.mTexel1[1] = scan[1];
                        firstTexel =false;
                    }
                    else
                    {
                        v.mTexel2[0] = scan[0];
                        v.mTexel2[1] = scan[1];
                    }
                    src+=sizeof(MiF32)*2;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"texcoord2") == 0 || MESH_IMPORT_STRING::stricmp(type,"texel2") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mTexel2[0] = scan[0];
                    v.mTexel2[1] = scan[1];
                    src+=sizeof(MiF32)*2;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"texcoord3") == 0 || MESH_IMPORT_STRING::stricmp(type,"texel3") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mTexel3[0] = scan[0];
                    v.mTexel3[1] = scan[1];
                    src+=sizeof(MiF32)*2;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"texcoord4") == 0 || MESH_IMPORT_STRING::stricmp(type,"texel4") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mTexel4[0] = scan[0];
                    v.mTexel4[1] = scan[1];
                    src+=sizeof(MiF32)*2;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp1") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp1[0] = scan[0];
                    v.mInterp1[1] = scan[1];
                    v.mInterp1[2] = scan[2];
                    v.mInterp1[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp2") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp2[0] = scan[0];
                    v.mInterp2[1] = scan[1];
                    v.mInterp2[2] = scan[2];
                    v.mInterp2[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp3") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp3[0] = scan[0];
                    v.mInterp3[1] = scan[1];
                    v.mInterp3[2] = scan[2];
                    v.mInterp3[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp4") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp4[0] = scan[0];
                    v.mInterp4[1] = scan[1];
                    v.mInterp4[2] = scan[2];
                    v.mInterp4[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp5") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp5[0] = scan[0];
                    v.mInterp5[1] = scan[1];
                    v.mInterp5[2] = scan[2];
                    v.mInterp5[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp6") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp6[0] = scan[0];
                    v.mInterp6[1] = scan[1];
                    v.mInterp6[2] = scan[2];
                    v.mInterp6[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp7") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp7[0] = scan[0];
                    v.mInterp7[1] = scan[1];
                    v.mInterp7[2] = scan[2];
                    v.mInterp7[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"interp8") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mInterp8[0] = scan[0];
                    v.mInterp8[1] = scan[1];
                    v.mInterp8[2] = scan[2];
                    v.mInterp8[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"tangent") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mTangent[0] = scan[0];
                    v.mTangent[1] = scan[1];
                    v.mTangent[2] = scan[2];
                    src+=sizeof(MiF32)*3;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"binormal") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mBiNormal[0] = scan[0];
                    v.mBiNormal[1] = scan[1];
                    v.mBiNormal[2] = scan[2];
                    src+=sizeof(MiF32)*3;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"blendweights") == 0 || MESH_IMPORT_STRING::stricmp(type,"blendweighting") == 0 || MESH_IMPORT_STRING::stricmp(type,"boneweighting") == 0 )
                {
                    const MiF32 * scan = (const MiF32 *)src;
                    v.mWeight[0] = scan[0];
                    v.mWeight[1] = scan[1];
                    v.mWeight[2] = scan[2];
                    v.mWeight[3] = scan[3];
                    src+=sizeof(MiF32)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"blendindices") == 0 )
                {
                    const unsigned short * scan = (const unsigned short *)src;
                    v.mBone[0] = scan[0];
                    v.mBone[1] = scan[1];
                    v.mBone[2] = scan[2];
                    v.mBone[3] = scan[3];
                    if ( v.mWeight[0] == 0 )
                    {
                        v.mBone[0] = 0;
                    }
                    if ( v.mWeight[1] == 0 )
                    {
                        v.mBone[1] = 0;
                    }
                    if ( v.mWeight[2] == 0 )
                    {
                        v.mBone[2] = 0;
                    }
                    if ( v.mWeight[3] == 0 )
                    {
                        v.mBone[3] = 0;
                    }

                    assert( v.mBone[0] < 256 );
                    assert( v.mBone[1] < 256 );
                    assert( v.mBone[2] < 256 );
                    assert( v.mBone[3] < 256 );
                    src+=sizeof(unsigned short)*4;
                }
                else if ( MESH_IMPORT_STRING::stricmp(type,"radius") == 0 )
                {
                    const MiF32 *scan = (const MiF32 *)src;
                    v.mRadius = scan[0];
                    src+=sizeof(MiF32);
                }
                else
                {
                    assert(0);
                }

            }
            return src;
        }

        MiU32 validateSemantics(const char **a1,const char **a2,MiI32 count)
        {
            bool ret = true;

            for (MiI32 i=0; i<count; i++)
            {
                const char *p = a1[i];
                const char *t = a2[i];

                if (MESH_IMPORT_STRING::stricmp(t,"position") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"normal") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"tangent") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"binormal") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp1") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp2") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp3") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp4") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp5") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp6") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp7") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"interp8") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"fff") != 0 )
                    {
                        ret = false;
                        break;
                    }
                }
                else if ( MESH_IMPORT_STRING::stricmp(t,"color") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"x4") != 0 )
                    {
                        ret = false;
                        break;
                    }
                }
                else if ( MESH_IMPORT_STRING::stricmp(t,"texel1") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texel2") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texel3") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texel4") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"tecoord") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texcoord1") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texcoord2") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texcoord3") == 0 ||
                    MESH_IMPORT_STRING::stricmp(t,"texcoord4") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"ff") != 0 )
                    {
                        ret =false;
                        break;
                    }
                }
                else if ( MESH_IMPORT_STRING::stricmp(t,"blendweights") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"ffff") != 0 )
                    {
                        ret = false;
                        break;
                    }
                }
                else if ( MESH_IMPORT_STRING::stricmp(t,"blendindices") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"hhhh") != 0 )
                    {
                        ret = false;
                        break;
                    }
                }
                else if ( MESH_IMPORT_STRING::stricmp(t,"radius") == 0 )
                {
                    if ( MESH_IMPORT_STRING::stricmp(p,"f") != 0 )
                    {
                        ret = false;
                        break;
                    }
                }
            }

            MiU32 flags = 0;

            if ( ret )
            {
                for (MiI32 i=0; i<count; i++)
                {
                    const char *t = a2[i];

                    if ( MESH_IMPORT_STRING::stricmp(t,"position") == 0 ) flags|=MIVF_POSITION;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"normal") == 0 )   flags|=MIVF_NORMAL;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"color") == 0 )    flags|=MIVF_COLOR;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texel1") == 0 )   flags|=MIVF_TEXEL1;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texel2") == 0 )   flags|=MIVF_TEXEL2;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texel3") == 0 )   flags|=MIVF_TEXEL3;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texel4") == 0 )   flags|=MIVF_TEXEL4;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texcoord1") == 0 )   flags|=MIVF_TEXEL1;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texcoord2") == 0 )   flags|=MIVF_TEXEL2;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texcoord3") == 0 )   flags|=MIVF_TEXEL3;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texcoord4") == 0 )   flags|=MIVF_TEXEL4;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"texcoord") == 0 )
                    {
                        if ( flags & MIVF_TEXEL1 )
                            flags|=MIVF_TEXEL2;
                        else
                            flags|=MIVF_TEXEL1;
                    }
                    else if ( MESH_IMPORT_STRING::stricmp(t,"tangent") == 0) flags|=MIVF_TANGENT;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp1") == 0) flags|=MIVF_INTERP1;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp2") == 0) flags|=MIVF_INTERP2;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp3") == 0) flags|=MIVF_INTERP3;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp4") == 0) flags|=MIVF_INTERP4;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp5") == 0) flags|=MIVF_INTERP5;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp6") == 0) flags|=MIVF_INTERP6;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp7") == 0) flags|=MIVF_INTERP7;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"interp8") == 0) flags|=MIVF_INTERP8;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"binormal") == 0 ) flags|=MIVF_BINORMAL;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"blendweights") == 0 ) flags|=MIVF_BONE_WEIGHTING;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"blendindices") == 0 ) flags|=MIVF_BONE_WEIGHTING;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"boneweights") == 0 ) flags|=MIVF_BONE_WEIGHTING;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"boneindices") == 0 ) flags|=MIVF_BONE_WEIGHTING;
                    else if ( MESH_IMPORT_STRING::stricmp(t,"radius") == 0 ) flags|=MIVF_RADIUS;
                }
            }

            return flags;
        }

        virtual bool processComment(const char * /*comment*/) // encountered a comment in the XML
        {
            return true;
        }

        // 'element' is the name of the element that is being closed.
        // depth is the recursion depth of this element.
        // Return true to continue processing the XML file.
        // Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
        // The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
        virtual bool processClose(const char * /*element*/,MiU32 /*depth*/,bool &isError)	  // process the 'close' indicator for a previously encountered element
        {
            isError = false;
            return true;
        }

        virtual void *  fastxml_malloc(MiU32 size) 
        {
            return MI_ALLOC(size);
        }

        virtual void	fastxml_free(void *mem) 
        {
            MI_FREE(mem);
        }





        virtual const char * getExtension(MiI32 /*index*/)  // report the default file name extension for this mesh type.
        {
            return ".ezm";
        }

        virtual const char * getDescription(MiI32 /*index*/)
        {
            return "PhysX Rocket EZ-Mesh format";
        }

        virtual bool  importMesh(const char * /*meshName*/,const void *data,MiU32 dlen,MeshImportInterface *callback,const char * /*options*/,MeshImportApplicationResource * /*appResource*/)
        {
            bool ret = false;

            mCallback = callback;

            if ( data && mCallback )
            {
                FastXml *f = createFastXml(this);
                MiMemoryBuffer mb(data,dlen);
                bool ok = f->processXml(mb,false);
                if ( ok )
                {
                    mCallback->importAssetName(mAssetName.Get(), mAssetInfo.Get());
                    ret = true;
                }
                if ( mAnimation )
                {
                    mCallback->importAnimation(*mAnimation);
                    for (MiI32 i=0; i<mAnimation->mTrackCount; i++)
                    {
                        MeshAnimTrack *t = mAnimation->mTracks[i];
                        MI_FREE(t->mPose);
                        delete t;
                    }
                    MI_FREE(mAnimation->mTracks);
                    delete mAnimation;
                    mAnimation = 0;
                }

                delete mMeshCollisionRepresentation;
                delete mMeshCollision;
                delete mMeshCollisionConvex;
                mMeshCollisionRepresentation = 0;
                mMeshCollision = 0;
                mMeshCollisionConvex = 0;

                MI_FREE( mVertexBuffer);
                mVertexBuffer = NULL;
                MI_FREE(mVertices);
                mVertices = NULL;


                f->release();

            }

            return ret;
        }

        bool isHex(char c)
        {
            bool ret = false;
            if ( c >= '0' && c <= '9' )
                ret = true;
            else if ( c >= 'a' && c <= 'f' )
                ret = true;
            else if ( c >= 'A' && c <= 'F' )
                ret = true;
            return ret;
        }

        char getHex(char c)
        {
            char ret = 0;
            if ( c >= '0' && c <= '9' )
                ret = c-'0';
            else if ( c >= 'a' && c <= 'f' )
                ret = (c-'a')+10;
            else if ( c >= 'A' && c <= 'F' )
                ret = (c-'A')+10;
            return ret;
        }

        const char * cleanAttribute(const char *input)
        {
            static char scratch[1024];
            const char *source = input;
            char *dest = scratch;
            char *end = &scratch[1024];
            while ( source && *source && dest < end )
            {
                if ( *source == '%' && isHex(source[1]) && isHex(source[2]))
                {
                    char a = getHex(source[1]);
                    char b = getHex(source[2]);
                    char c = a<<4|b;
                    *dest++ = c;
                    source+=3;
                }
                else
                {
                    *dest++ = *source++;
                }
            }

            *dest = 0;
            return scratch;
        }

        void ProcessNode(const char *svalue,MiU32 argc,const char **argv)
        {
            mType = (NodeType)mToElement.Get(svalue);
            switch ( mType )
            {
            case NT_MATERIAL:
                {
                    const char *name = getAttribute("name",(MiI32)argc,argv);
                    const char *meta_data = getAttribute("meta_data",(MiI32)argc,argv);
                    meta_data = cleanAttribute(meta_data);
                    mCallback->importMaterial(name,meta_data);
                }
                break;
            case NT_NONE:
                break;
#if 0 // TODO TODO
            case NT_MESH_COLLISION_REPRESENTATION:
                delete mMeshCollisionRepresentation;
                mMeshCollisionRepresentation = new MeshCollisionRepresentation;
                break;
            case NT_MESH_COLLISION:
                if ( mMeshCollisionRepresentation )
                {
                    mCallback->importCollisionRepresentation( mMeshCollisionRepresentation->mName, mMeshCollisionRepresentation->mInfo );
                    mCollisionRepName = SGET(mMeshCollisionRepresentation->mName);
                    delete mMeshCollisionRepresentation;
                    mMeshCollisionRepresentation = 0;
                }
                delete mMeshCollision;
                mMeshCollision = new MeshCollision;
                break;
            case NT_MESH_COLLISION_CONVEX:
                assert(mMeshCollision);
                if ( mMeshCollision )
                {
                    delete mMeshCollisionConvex;
                    mMeshCollisionConvex = new MeshCollisionConvex;
                    MeshCollision *d = static_cast< MeshCollision *>(mMeshCollisionConvex);
                    *d = *mMeshCollision;
                    delete mMeshCollision;
                    mMeshCollision = 0;
                }
                break;
#endif
            case NT_ANIMATION:
                if ( mAnimation )
                {
                    mCallback->importAnimation(*mAnimation);
                    delete mAnimation;
                    mAnimation = 0;
                }
                mName       = 0;
                mFrameCount = 0;
                mDuration   = 0;
                mTrackCount = 0;
                mDtime      = 0;
                mTrackIndex = 0;
                break;
            case NT_ANIM_TRACK:
                if ( mAnimation == 0 )
                {
                    if ( mName && mFrameCount && mDuration && mTrackCount && mDtime )
                    {
                        MiI32 framecount = atoi( mFrameCount );
                        MiF32 duration = (MiF32) atof( mDuration );
                        MiI32 trackcount = atoi(mTrackCount);
                        MiF32 dtime = (MiF32) atof(mDtime);
                        if ( trackcount >= 1 && framecount >= 1 )
                        {
                            mAnimation = new MeshAnimation;
                            mAnimation->mName = mName;
                            mAnimation->mTrackCount = trackcount;
                            mAnimation->mFrameCount = framecount;
                            mAnimation->mDuration = duration;
                            mAnimation->mDtime = dtime;
                            mAnimation->mTracks = (MeshAnimTrack **)MI_ALLOC(sizeof(MeshAnimTrack *)*mAnimation->mTrackCount);
                            for (MiI32 i=0; i<mAnimation->mTrackCount; i++)
                            {
                                MeshAnimTrack *track = new MeshAnimTrack;
                                track->mDtime = mAnimation->mDuration;
                                track->mFrameCount = mAnimation->mFrameCount;
                                track->mDuration = mAnimation->mDuration;
                                track->mPose = (MeshAnimPose *)MI_ALLOC(sizeof(MeshAnimPose)*track->mFrameCount);
                                mAnimation->mTracks[i] = track;
                            }
                        }
                    }
                }
                if ( mAnimation )
                {
                    mAnimTrack = mAnimation->GetTrack(mTrackIndex);
                    mTrackIndex++;
                }
                break;
            case NT_SKELETON:
                {
                    delete mSkeleton;
                    mSkeleton = new MeshSkeleton;
                }
            case NT_BONE:
                if ( mSkeleton )
                {
                    mBone = mSkeleton->GetBonePtr(mBoneIndex);
                }
                break;
            default:
                break;
            }
        }
        void ProcessData(const char *svalue)
        {
            if ( svalue )
            {
                switch ( mType )
                {
                default:
                    break;
                case NT_ANIM_TRACK:
                    if ( mAnimTrack )
                    {
                        mAnimTrack->SetName(mStrings.Get(mName).Get());
                        MiI32 count = atoi( mCount );
                        if ( count == mAnimTrack->GetFrameCount() )
                        {
                            if ( mHasScale )
                            {
                                MiF32 *buff = (MiF32 *) MI_ALLOC(sizeof(MiF32)*10*count);
                                Asc2Bin(svalue, count, "fff ffff fff", buff );
                                for (MiI32 i=0; i<count; i++)
                                {
                                    MeshAnimPose *p = mAnimTrack->GetPose(i);
                                    const MiF32 *src = &buff[i*10];

                                    p->mPos[0]  = src[0];
                                    p->mPos[1]  = src[1];
                                    p->mPos[2]  = src[2];

                                    p->mQuat[0] = src[3];
                                    p->mQuat[1] = src[4];
                                    p->mQuat[2] = src[5];
                                    p->mQuat[3] = src[6];

                                    p->mScale[0] = src[7];
                                    p->mScale[1] = src[8];
                                    p->mScale[2] = src[9];
                                }
                                MI_FREE(buff);
                            }
                            else
                            {
                                MiF32 *buff = (MiF32 *) MI_ALLOC(sizeof(MiF32)*7*count);
                                Asc2Bin(svalue, count, "fff ffff", buff );
                                for (MiI32 i=0; i<count; i++)
                                {
                                    MeshAnimPose *p = mAnimTrack->GetPose(i);
                                    const MiF32 *src = &buff[i*7];

                                    p->mPos[0]  = src[0];
                                    p->mPos[1]  = src[1];
                                    p->mPos[2]  = src[2];

                                    p->mQuat[0] = src[3];
                                    p->mQuat[1] = src[4];
                                    p->mQuat[2] = src[5];
                                    p->mQuat[3] = src[6];

                                    p->mScale[0] = 1;
                                    p->mScale[1] = 1;
                                    p->mScale[2] = 1;
                                }
                                MI_FREE(buff);
                            }
                        }
                    }
                    break;
                case NT_NODE_INSTANCE:
#if 0 // TODO TODO
                    if ( mName )
                    {
                        MiF32 transform[4*4];
                        Asc2Bin(svalue, 4, "ffff", transform );
                        MeshBone b;
                        b.SetTransform(transform);
                        MiF32 pos[3];
                        MiF32 quat[3];
                        MiF32 scale[3] = { 1, 1, 1 };
                        b.ExtractOrientation(quat);
                        b.GetPos(pos);
                        mCallback->importMeshInstance(mName,pos,quat,scale);
                        mName = 0;
                    }
#endif
                    break;
                case NT_NODE_TRIANGLE:
                    if ( mCtype && mSemantic )
                    {
                        MiI32 c1,c2;
                        char scratch1[2048];
                        char scratch2[2048];
                        MESH_IMPORT_STRING::strlcpy(scratch1,2048,mCtype);
                        MESH_IMPORT_STRING::strlcpy(scratch2,2048,mSemantic);
                        const char **a1 = mParser1.GetArglist(scratch1,c1);
                        const char **a2 = mParser2.GetArglist(scratch2,c2);
                        if ( c1 > 0 && c2 > 0 && c1 == c2 )
                        {
                            mVertexFlags = validateSemantics(a1,a2,c1);
                            if ( mVertexFlags )
                            {
                                MeshVertex vtx[3];
                                char scratch[1024];

                                const MiU8 *temp = (const MiU8 *)Asc2Bin(svalue, 3, mCtype, scratch );

                                temp = getVertex(temp,vtx[0],a2,c2);
                                temp = getVertex(temp,vtx[1],a2,c2);
                                temp = getVertex(temp,vtx[2],a2,c2);
                                mCallback->importTriangle(mCurrentMesh.Get(),mCurrentMaterial.Get(),mVertexFlags,vtx[0],vtx[1],vtx[2]);
                                //MI_FREE((void *)temp);
                            }
                        }
                        mCtype = 0;
                        mSemantic = 0;
                    }
                    break;
                case NT_VERTEX_BUFFER:
                    MI_FREE( mVertexBuffer);
                    MI_FREE(mVertices);
                    mVertices = 0;
                    mVertexCount = 0;
                    mVertexBuffer = 0;

                    if ( mCtype && mCount )
                    {
                        mVertexCount  = atoi(mCount);
                        if ( mVertexCount > 0 )
                        {
                            mVertexBuffer = Asc2Bin(svalue, mVertexCount, mCtype, 0 );

                            if ( mVertexBuffer )
                            {

                                MiI32 c1,c2;
                                char scratch1[2048];
                                char scratch2[2048];
                                MESH_IMPORT_STRING::strlcpy(scratch1,2048,mCtype);
                                MESH_IMPORT_STRING::strlcpy(scratch2,2048,mSemantic);

                                const char **a1 = mParser1.GetArglist(scratch1,c1);
                                const char **a2 = mParser2.GetArglist(scratch2,c2);

                                if ( c1 > 0 && c2 > 0 && c1 == c2 )
                                {
                                    mVertexFlags = validateSemantics(a1,a2,c1);
                                    if ( mVertexFlags )
                                    {
                                        mVertices = (MeshVertex *)MI_ALLOC(sizeof(MeshVertex)*mVertexCount);
                                        for (MiI32 i=0; i<mVertexCount; i++)
                                        {
                                            new ( &mVertices[i]) MeshVertex;
                                        }
                                        const MiU8 *scan = (const MiU8 *)mVertexBuffer;
                                        for (MiI32 i=0; i<mVertexCount; i++)
                                        {
                                            scan = getVertex(scan,mVertices[i],a2,c2);
                                        }
                                    }
                                }
                                MI_FREE( mVertexBuffer);
                                mVertexBuffer = 0;
                            }

                        }
                        mCtype = 0;
                        mCount = 0;
                    }
                    break;
                case NT_INDEX_BUFFER:
                    if ( mCount )
                    {
                        mIndexCount = atoi(mCount);
                        if ( mIndexCount > 0 )
                        {
                            mIndexBuffer = Asc2Bin(svalue, mIndexCount, "ddd", 0 );
                        }
                    }

                    if ( mIndexBuffer && mVertices )
                    {
#if 0 // TODO TODO
                        if ( mMeshCollisionConvex )
                        {
                            MiF32 *vertices = (MiF32 *)MI_ALLOC(sizeof(MiF32)*mVertexCount*3);
                            MiF32 *dest = vertices;
                            for (MiI32 i=0; i<mVertexCount; i++)
                            {
                                dest[0] = mVertices[i].mPos[0];
                                dest[1] = mVertices[i].mPos[1];
                                dest[2] = mVertices[i].mPos[2];
                                dest+=3;
                            }

                            mCallback->importConvexHull(mCollisionRepName.Get(),
                                mMeshCollisionConvex->mName,
                                mMeshCollisionConvex->mTransform,
                                mVertexCount,
                                vertices,
                                mIndexCount,
                                (const MiU32 *)mIndexBuffer);

                            MI_FREE(vertices);
                            delete mMeshCollisionConvex;
                            mMeshCollisionConvex = 0;
                        }
                        else
#endif
                        {
                            mCallback->importIndexedTriangleList(mCurrentMesh.Get(),mCurrentMaterial.Get(),mVertexFlags,(MiU32)mVertexCount,mVertices,(MiU32)mIndexCount,(const MiU32 *)mIndexBuffer );
                        }
                    }


                    MI_FREE( mIndexBuffer);
                    mIndexBuffer = 0;
                    mIndexCount = 0;
                    break;
                }
            }
        }

        void ProcessAttribute(const char *aname,  // the name of the attribute
            const char *savalue) // the value of the attribute
        {
            AttributeType attrib = (AttributeType) mToAttribute.Get(aname);
            switch ( attrib )
            {
            default:
                break;
            case AT_NONE:
                assert(0);
                break;
            case AT_MESH_SYSTEM_VERSION:
                mMeshSystemVersion = atoi(savalue);
                break;
            case AT_MESH_SYSTEM_ASSET_VERSION:
                mMeshSystemAssetVersion = atoi(savalue);
                break;
            case AT_ASSET_NAME:
                mAssetName = mStrings.Get(savalue);
                break;
            case AT_ASSET_INFO:
                mAssetInfo = mStrings.Get(savalue);
                break;
            case AT_SCALE:
                if ( mType == NT_BONE && mBone )
                {
                    Asc2Bin(savalue,1,"fff", mBone->mScale );
                }
                break;
            case AT_POSITION:
                if ( mType == NT_BONE && mBone )
                {
                    Asc2Bin(savalue,1,"fff", mBone->mPosition );
                    mBoneIndex++;
                    if ( mBoneIndex == mSkeleton->GetBoneCount() )
                    {
                        mCallback->importSkeleton(*mSkeleton);
                        delete mSkeleton;
                        mSkeleton = 0;
                        mBoneIndex = 0;
                    }
                }
                break;
            case AT_ORIENTATION:
                if ( mType == NT_BONE && mBone )
                {
                    Asc2Bin(savalue,1,"ffff", mBone->mOrientation );
                }
                break;
            case AT_HAS_SCALE:
                mHasScale = getBool(savalue);
                break;
            case AT_DURATION:
                mDuration = savalue;
                break;

            case AT_DTIME:
                mDtime = savalue;
                break;

            case AT_TRACK_COUNT:
                mTrackCount = savalue;
                break;

            case AT_FRAME_COUNT:
                mFrameCount = savalue;
                break;
            case AT_INFO:
                switch ( mType )
                {
                default:
                    break;
                case NT_MESH_COLLISION_REPRESENTATION:
                    assert(mMeshCollisionRepresentation);
                    if ( mMeshCollisionRepresentation )
                    {
                        mMeshCollisionRepresentation->mInfo = mStrings.Get(savalue).Get();
                    }
                    break;
                }
                break;
#if 0 // TODO TODO
            case AT_TRANSFORM:
                if (mType == NT_MESH_COLLISION )
                {
                    assert( mMeshCollision );
                    if ( mMeshCollision )
                    {
                        Asc2Bin(savalue,4,"ffff", mMeshCollision->mTransform );
                    }
                }
                break;
#endif
            case AT_NAME:
                mName = savalue;

                switch ( mType )
                {
                default:
                    break;
                case NT_MESH_COLLISION:
                    assert( mMeshCollision );
                    if ( mMeshCollision )
                    {
                        mMeshCollision->mName = mStrings.Get(savalue).Get();
                    }
                    break;
                case NT_MESH_COLLISION_REPRESENTATION:
                    assert(mMeshCollisionRepresentation);
                    if ( mMeshCollisionRepresentation )
                    {
                        mMeshCollisionRepresentation->mName = mStrings.Get(savalue).Get();
                    }
                    break;
                case NT_MESH:
                    mCurrentMesh = mStrings.Get(savalue);
                    mCallback->importMesh(savalue,0);
                    break;
                case NT_SKELETON:
                    if ( mSkeleton )
                    {
                        mSkeleton->SetName(mStrings.Get(savalue).Get());
                    }
                    break;
                case NT_BONE:
                    if ( mBone )
                    {
                        mBone->SetName(mStrings.Get(savalue).Get());
                    }
                    break;
                }
                break;
            case AT_TRIANGLE_COUNT:
                mCount = savalue;
                break;
            case AT_COUNT:
                mCount = savalue;
                if ( mType == NT_SKELETON )
                {
                    if ( mSkeleton )
                    {
                        MiI32 count = atoi( savalue );
                        if ( count > 0 )
                        {
                            MeshBone *bones;
                            bones = new MeshBone[(MiU32)count];
                            mSkeleton->SetBones(count,bones);
                            mBoneIndex = 0;
                        }
                    }
                }
                break;
            case AT_PARENT:
                mParent = savalue;
                if ( mBone )
                {
                    for (MiI32 i=0; i<mBoneIndex; i++)
                    {
                        const MeshBone &b = mSkeleton->GetBone(i);
                        if ( strcmp(mParent,b.mName) == 0 )
                        {
                            mBone->mParentIndex = i;
                            break;
                        }
                    }
                }
                break;
            case AT_MATERIAL:
                if ( mType == NT_MESH_SECTION )
                {
                    mCallback->importMaterial(savalue,0);
                    mCurrentMaterial = mStrings.Get(savalue);
                }
                break;
            case AT_CTYPE:
                mCtype = savalue;
                break;
            case AT_SEMANTIC:
                mSemantic = savalue;
                break;
            }

        }

        virtual bool processElement(const char *elementName,         // name of the element
            MiI32         argc,                // number of attributes
            const char **argv,               // list of attributes.
            const char  *elementData,        // element data, null if none
            MiI32         /*lineno*/)         // line number in the source XML file
        {
            ProcessNode(elementName,(MiU32)argc,argv);
            MiI32 acount = argc/2;
            for (MiI32 i=0; i<acount; i++)
            {
                const char *key = argv[i*2];
                const char *value = argv[i*2+1];
                ProcessAttribute(key,value);
            }
            ProcessData(elementData);
            return true;
        }




    private:
        MeshImportInterface     *mCallback;

        bool                   mHasScale;

        StringTableInt         mToElement;         // convert string to element enumeration.
        StringTableInt         mToAttribute;       // convert string to attribute enumeration
        NodeType               mType;

        InPlaceParser mParser1;
        InPlaceParser mParser2;

        const char * mName;
        const char * mCount;
        const char * mParent;
        const char * mCtype;
        const char * mSemantic;

        const char * mFrameCount;
        const char * mDuration;
        const char * mTrackCount;
        const char * mDtime;

        MiI32          mTrackIndex;
        MiI32          mBoneIndex;
        MeshBone       * mBone;

        MeshAnimation  * mAnimation;
        MeshAnimTrack  * mAnimTrack;
        MeshSkeleton   * mSkeleton;
        MiI32          mVertexCount;
        MiI32          mIndexCount;
        void       * mVertexBuffer;
        void       * mIndexBuffer;

        MiI32          mMeshSystemVersion;
        MiI32          mMeshSystemAssetVersion;

        StringRef    mCurrentMesh;
        StringRef    mCurrentMaterial;
        StringDict   mStrings;
        StringRef    mAssetName;
        StringRef    mAssetInfo;

        MiU32 mVertexFlags;
        MeshVertex  *mVertices;

        StringRef                    mCollisionRepName;
        MeshCollisionRepresentation *mMeshCollisionRepresentation;
        MeshCollision               *mMeshCollision;
        MeshCollisionConvex         *mMeshCollisionConvex;


    };


    MeshImporter * createMeshImportEZM(void)
    {
        MeshImportEZM *m = new MeshImportEZM;
        return static_cast< MeshImporter *>(m);
    }

    void         releaseMeshImportEZM(MeshImporter *iface)
    {
        MeshImportEZM *m = static_cast< MeshImportEZM *>(iface);
        delete m;
    }

}; // end of namepsace
