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
#include "ExportPOVRay.h"
#include "MeshImport.h"
#include "MiFloatMath.h"
#include "MiFileInterface.h"

#pragma warning(disable:4100)

namespace mimp
{

static inline const MiF32 * getFloat3(const MiF32 *texel)
{
	static MiF32 pos[3];
	pos[0] = texel[0];
	pos[1] = texel[1];
	pos[2] = 0;
	return pos;
}

static void exportSubMesh(Mesh *m,SubMesh *sm,FILE_INTERFACE *fph)
{
	char scratch[512];
	MESH_IMPORT_STRING::snprintf(scratch,512,"%s_%s", m->mName, sm->mMaterialName );
	fm_VertexIndex *positions = fm_createVertexIndex(0.000001f,false);
	fm_VertexIndex *normals	  = fm_createVertexIndex(0.000001f,false);
	fm_VertexIndex *texels	  = fm_createVertexIndex(0.000001f,false);

	STDNAME::vector< MiU32 > positionIndices;
	STDNAME::vector< MiU32 > normalIndices;
	STDNAME::vector< MiU32 > texelIndices;

	for (MiU32 i=0; i<sm->mTriCount; i++)
	{
		MiU32 i1 = sm->mIndices[i*3+0];
		MiU32 i2 = sm->mIndices[i*3+1];
		MiU32 i3 = sm->mIndices[i*3+2];

		MeshVertex &v1 = m->mVertices[i1];
		MeshVertex &v2 = m->mVertices[i2];
		MeshVertex &v3 = m->mVertices[i3];
		bool newPos;

		MiU32 p1 = positions->getIndex(v1.mPos,newPos);
		MiU32 p2 = positions->getIndex(v2.mPos,newPos);
		MiU32 p3 = positions->getIndex(v3.mPos,newPos);
		positionIndices.push_back(p1);
		positionIndices.push_back(p2);
		positionIndices.push_back(p3);

		MiU32 n1 = normals->getIndex(v1.mNormal,newPos);
		MiU32 n2 = normals->getIndex(v2.mNormal,newPos);
		MiU32 n3 = normals->getIndex(v3.mNormal,newPos);
		normalIndices.push_back(n1);
		normalIndices.push_back(n2);
		normalIndices.push_back(n3);

		MiU32 t1 = texels->getIndex( getFloat3(v1.mTexel1), newPos );
		MiU32 t2 = texels->getIndex( getFloat3(v2.mTexel1), newPos );
		MiU32 t3 = texels->getIndex( getFloat3(v3.mTexel1), newPos );
		texelIndices.push_back(t1);
		texelIndices.push_back(t2);
		texelIndices.push_back(t3);

	}

	fi_fprintf(fph,"// ------- Mesh %s SubMesh %s\r\n", m->mName, sm->mMaterialName );
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"#declare %s=\r\n", scratch);
	fi_fprintf(fph,"			mesh2{\r\n" );

	fi_fprintf(fph,"				vertex_vectors{\r\n");
	fi_fprintf(fph,"					%d\r\n", positions->getVcount());
	for (MiU32 i=0; i<positions->getVcount(); i++)
	{
		const MiF32 *pos = positions->getVertexFloat(i);
		fi_fprintf(fph,"						<%0.9f, %0.9f, %0.9f>,\r\n", pos[0], pos[1], pos[2] );
	}
	fi_fprintf(fph,"				}\r\n");

	fi_fprintf(fph,"				normal_vectors{\r\n");
	fi_fprintf(fph,"					%d\r\n", normals->getVcount());
	for (MiU32 i=0; i<normals->getVcount(); i++)
	{
		const MiF32 *pos = normals->getVertexFloat(i);
		fi_fprintf(fph,"						<%0.9f, %0.9f, %0.9f>,\r\n", pos[0], pos[1], pos[2] );
	}
	fi_fprintf(fph,"				}\r\n");


	fi_fprintf(fph,"				uv_vectors{\r\n");
	fi_fprintf(fph,"					%d\r\n", texels->getVcount());
	for (MiU32 i=0; i<texels->getVcount(); i++)
	{
		const MiF32 *pos = texels->getVertexFloat(i);
		fi_fprintf(fph,"						<%0.9f, %0.9f>,\r\n", pos[0], pos[1] );
	}
	fi_fprintf(fph,"				}\r\n");

	fi_fprintf(fph,"				face_indices{\r\n");
	fi_fprintf(fph,"					%d,\r\n", positionIndices.size()/3);
	for (MiU32 i=0; i<positionIndices.size()/3; i++)
	{
		MiU32 i1 = positionIndices[i*3+0];
		MiU32 i2 = positionIndices[i*3+1];
		MiU32 i3 = positionIndices[i*3+2];
		fi_fprintf(fph,"						<%d,%d,%d>,\r\n", i1, i2, i3);
	}
	fi_fprintf(fph,"				}\r\n");

	fi_fprintf(fph,"				normal_indices{\r\n");
	fi_fprintf(fph,"					%d,\r\n", normalIndices.size()/3);
	for (MiU32 i=0; i<normalIndices.size()/3; i++)
	{
		MiU32 i1 = normalIndices[i*3+0];
		MiU32 i2 = normalIndices[i*3+1];
		MiU32 i3 = normalIndices[i*3+2];
		fi_fprintf(fph,"						<%d,%d,%d>,\r\n", i1, i2, i3);
	}
	fi_fprintf(fph,"				}\r\n");

	fi_fprintf(fph,"				uv_indices{\r\n");
	fi_fprintf(fph,"					%d,\r\n", texelIndices.size()/3);
	for (MiU32 i=0; i<texelIndices.size()/3; i++)
	{
		MiU32 i1 = texelIndices[i*3+0];
		MiU32 i2 = texelIndices[i*3+1];
		MiU32 i3 = texelIndices[i*3+2];
		fi_fprintf(fph,"						<%d,%d,%d>,\r\n", i1, i2, i3);
	}
	fi_fprintf(fph,"				}\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"			}\r\n");
	fi_fprintf(fph,"\r\n");


	fm_releaseVertexIndex(positions);
	fm_releaseVertexIndex(normals);
	fm_releaseVertexIndex(texels);
}
void *serializePOVRay(const MeshSystem *ms,MiU32 &len)
{
	void * ret = NULL;
	len = 0;

	if ( ms )
	{
		FILE_INTERFACE *fph = fi_fopen("foo", "wmem", 0, 0);

		if ( fph )
		{

			fi_fprintf(fph,"			// Persistence of Vision Ray Tracer Scene Description File\r\n");
			fi_fprintf(fph,"			// File: mesh2.pov\r\n");
			fi_fprintf(fph,"			// Vers: 3.5\r\n");
			fi_fprintf(fph,"			// Desc: mesh2 demonstration scene\r\n");
			fi_fprintf(fph,"			// Date: November/December 2001\r\n");
			fi_fprintf(fph,"			// Auth: Christoph Hormann\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			// -w320 -h240\r\n");
			fi_fprintf(fph,"			// -w512 -h384 +a0.3\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"#version 3.5;\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			global_settings {\r\n");
			fi_fprintf(fph,"				assumed_gamma 1\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			light_source {\r\n");
			fi_fprintf(fph,"				<-0.6, 1.6, 3.7>*10000\r\n");
			fi_fprintf(fph,"					rgb 1.3\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			camera {\r\n");
			fi_fprintf(fph,"				location    <7, 20, 20>\r\n");
			fi_fprintf(fph,"					direction   y\r\n");
			fi_fprintf(fph,"					sky         z\r\n");
			fi_fprintf(fph,"					up          z\r\n");
			fi_fprintf(fph,"					right       (4/3)*x\r\n");
			fi_fprintf(fph,"					look_at     <0.0, 0, 1.2>\r\n");
			fi_fprintf(fph,"					angle       20\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			background {\r\n");
			fi_fprintf(fph,"				color rgb < 0.60, 0.70, 0.95 >\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			plane {\r\n");
			fi_fprintf(fph,"				z, 0\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"					texture {\r\n");
			fi_fprintf(fph,"						pigment {\r\n");
			fi_fprintf(fph,"							bozo\r\n");
			fi_fprintf(fph,"								color_map {\r\n");
			fi_fprintf(fph,"									[ 0.0 color rgb<0.356, 0.321, 0.274> ]\r\n");
			fi_fprintf(fph,"									[ 0.1 color rgb<0.611, 0.500, 0.500> ]\r\n");
			fi_fprintf(fph,"									[ 0.4 color rgb<0.745, 0.623, 0.623> ]\r\n");
			fi_fprintf(fph,"									[ 1.0 color rgb<0.837, 0.782, 0.745> ]\r\n");
			fi_fprintf(fph,"							}\r\n");
			fi_fprintf(fph,"							warp { turbulence 0.6 }\r\n");
			fi_fprintf(fph,"						}\r\n");
			fi_fprintf(fph,"						finish {\r\n");
			fi_fprintf(fph,"							diffuse 0.6\r\n");
			fi_fprintf(fph,"								ambient 0.1\r\n");
			fi_fprintf(fph,"								specular 0.2\r\n");
			fi_fprintf(fph,"								reflection {\r\n");
			fi_fprintf(fph,"									0.2, 0.6\r\n");
			fi_fprintf(fph,"										fresnel on\r\n");
			fi_fprintf(fph,"							}\r\n");
			fi_fprintf(fph,"							conserve_energy\r\n");
			fi_fprintf(fph,"						}\r\n");
			fi_fprintf(fph,"				}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"#declare Mesh_TextureA=\r\n");
			fi_fprintf(fph,"			texture{\r\n");
			fi_fprintf(fph,"				pigment{\r\n");
			fi_fprintf(fph,"					uv_mapping\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"						spiral2 8\r\n");
			fi_fprintf(fph,"						color_map {\r\n");
			fi_fprintf(fph,"							[0.5 color rgb <0.2,0,0> ]\r\n");
			fi_fprintf(fph,"							[0.5 color rgb 1 ]\r\n");
			fi_fprintf(fph,"					}\r\n");
			fi_fprintf(fph,"					scale 0.8\r\n");
			fi_fprintf(fph,"				}\r\n");
			fi_fprintf(fph,"				finish {\r\n");
			fi_fprintf(fph,"					specular 0.3\r\n");
			fi_fprintf(fph,"						roughness 0.01\r\n");
			fi_fprintf(fph,"				}\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"#declare Mesh_TextureB=\r\n");
			fi_fprintf(fph,"			texture{\r\n");
			fi_fprintf(fph,"				pigment{\r\n");
			fi_fprintf(fph,"					uv_mapping\r\n");
			fi_fprintf(fph,"\r\n");
			fi_fprintf(fph,"						spiral2 8\r\n");
			fi_fprintf(fph,"						color_map {\r\n");
			fi_fprintf(fph,"							[0.5 color rgb 1 ]\r\n");
			fi_fprintf(fph,"							[0.5 color rgb <0,0,0.2> ]\r\n");
			fi_fprintf(fph,"					}\r\n");
			fi_fprintf(fph,"					scale 0.8\r\n");
			fi_fprintf(fph,"				}\r\n");
			fi_fprintf(fph,"				finish {\r\n");
			fi_fprintf(fph,"					specular 0.3\r\n");
			fi_fprintf(fph,"						roughness 0.01\r\n");
			fi_fprintf(fph,"				}\r\n");
			fi_fprintf(fph,"			}\r\n");
			fi_fprintf(fph,"\r\n");

// let's save each sub-mesh we encounter...
			for (MiU32 i=0; i<ms->mMeshCount; i++)
			{
				Mesh *m = ms->mMeshes[i];
				for (MiU32 j=0; j<m->mSubMeshCount; j++)
				{
					SubMesh *sm = m->mSubMeshes[j];
					exportSubMesh(m,sm,fph);
				}
			}

			for (MiU32 i=0; i<ms->mMeshCount; i++)
			{
				Mesh *m = ms->mMeshes[i];
				for (MiU32 j=0; j<m->mSubMeshCount; j++)
				{
					SubMesh *sm = m->mSubMeshes[j];
					char scratch[512];
					MESH_IMPORT_STRING::snprintf(scratch,512,"%s_%s", m->mName, sm->mMaterialName );
					fi_fprintf(fph,"\r\n");
					fi_fprintf(fph,"			object {\r\n");
					fi_fprintf(fph,"				%s\r\n", scratch);
					fi_fprintf(fph,"					texture { Mesh_TextureA }\r\n");
					fi_fprintf(fph,"			}\r\n");
				}
			}



			size_t outputLength;
			void *mem = fi_getMemBuffer(fph,&outputLength);
			if ( mem )
			{
				ret = MI_ALLOC(outputLength);
				if ( ret )
				{
					memcpy(ret,mem,outputLength);
					len = (MiU32)outputLength;
				}
			}
			fi_fclose(fph);
		}
	}
	return ret;
}


};// end of namespace

