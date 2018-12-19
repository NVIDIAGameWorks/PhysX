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
#include "ExportFBX.h"


#include "MeshImport.h"
#include "MiFloatMath.h"

#pragma warning(disable:4100)

namespace mimp
{

#if 0 // not curruently fully implemented
void writeVertices(FILE_INTERFACE *fph,Mesh *mesh)
{
	if ( mesh->mVertexCount )
	{
		fi_fprintf(fph,"        Vertices: ");
		MiU32 col = 0;

		for (MiU32 i=0; i<mesh->mVertexCount; i++)
		{
			if (col == 0 && i > 0 )
			{
				fi_fprintf(fph,"        ,");
			}
			const MeshVertex &mv = mesh->mVertices[i];
			fi_fprintf(fph,"%0.9f,%0.9f,%0.9f", mv.mPos[0], mv.mPos[1], mv.mPos[2] );
			col++;
			if ( col == 4 )
			{
				fi_fprintf(fph,"\r\n");
				col = 0;
			}
			else if ( (i+1) < mesh->mVertexCount )
			{
				fi_fprintf(fph,",");
			}
		}
		fi_fprintf(fph,"\r\n");
	}

}

void writePolygonVertexIndex(FILE_INTERFACE *fph,Mesh *mesh)
{
	if ( mesh->mSubMeshCount )
	{

		fi_fprintf(fph,"        PolygonVertexIndex: ");
		MiU32 col = 0;
		MiU32 index = 0;
		while ( index < mesh->mSubMeshCount )
		{
			SubMesh *sm = mesh->mSubMeshes[index];
			for (MiU32 i=0; i<sm->mTriCount; i++)
			{
				if (col == 0 && (i > 0 || index > 0 ))
				{
					fi_fprintf(fph,"        ,");
				}
				MiI32 i1 = (MiI32)sm->mIndices[i*3+0];
				MiI32 i2 = (MiI32)sm->mIndices[i*3+1];
				MiI32 i3 = (MiI32)sm->mIndices[i*3+2];
				i3 = (i3+1)*-1;
				fi_fprintf(fph,"%d,%d,%d", i1, i2, i3 );
				col++;
				if ( col == 4 )
				{
					fi_fprintf(fph,"\r\n");
					col = 0;
				}
				else if ( (i+1) < sm->mTriCount )
				{
					fi_fprintf(fph,",");
				}
			}
			index++;
		}
		fi_fprintf(fph,"\r\n");
	}
}

void writeUV(FILE_INTERFACE *fph,Mesh *mesh)
{

#if 0
	fi_fprintf(fph,"        LayerElementUV: 0 {\r\n");
	fi_fprintf(fph,"            Version: 101\r\n");
	fi_fprintf(fph,"            Name: \"\"\r\n");
	fi_fprintf(fph,"            MappingInformationType: \"ByPolygonVertex\"\r\n");
	fi_fprintf(fph,"            ReferenceInformationType: \"IndexToDirect\"\r\n");
	fi_fprintf(fph,"            UV: 0.5,1,0.5,0,1,0,1,1,0,1,0,0,0.5,0,0.5,1,0.5,1,0,1,0,0,0.5,0,1,1,0.5,1,0.5,0,1,0,0.5,1,0,1,0,0,0.5,0,1,1,0.5,1\r\n");
	fi_fprintf(fph,"             ,0.5,0,1,0,1,1,0,1,0,0,1,0,0.5,1,0,1,0,0,0.5,0,1,1,0.5,1,0.5,0,1,0,1,1,0,1,0,0,1,0\r\n");
	fi_fprintf(fph,"            UVIndex: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"        }\r\n");
#endif

	fi_fprintf(fph,"        LayerElementUV: 0 {\r\n");
	fi_fprintf(fph,"            Version: 101\r\n");
	fi_fprintf(fph,"            Name: \"\"\r\n");
	fi_fprintf(fph,"            MappingInformationType: \"ByPolygonVertex\"\r\n");
	fi_fprintf(fph,"            ReferenceInformationType: \"IndexToDirect\"\r\n");
	fi_fprintf(fph,"            UV: ");

	MiU32 vcount = mesh->mVertexCount;

	MiU32 col = 0;
	for (MiU32 i=0; i<vcount; i++)
	{
		if ( col == 0 && i > 0 )
		{
			fi_fprintf(fph,"            ,");
		}
		const MeshVertex &mv = mesh->mVertices[i];
		fi_fprintf(fph,"%0.6f,%0.6f", mv.mTexel1[0], mv.mTexel1[1] );
		col++;
		if ( col == 32 )
		{
			fi_fprintf(fph,"\r\n");
			col = 0;
		}
		else if ( (i+1) < vcount )
		{
			fi_fprintf(fph,",");
		}
	}
	fi_fprintf(fph,"\r\n");

	fi_fprintf(fph,"            UVIndex: ");
	col = 0;
	for (MiU32 i=0; i<vcount; i++)
	{
		if ( col == 0 && i > 0 )
			fi_fprintf(fph,"            ,");
		fi_fprintf(fph,"%d",i);
		col++;
		if ( col == 64 )
		{
			fi_fprintf(fph,"\r\n");
			col = 0;
		}
		else if ( (i+1) < vcount )
		{
			fi_fprintf(fph,",");
		}
	}
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"        }\r\n");

}

void serializeFBX(FILE_INTERFACE *fph,MeshSystem *ms)
{

	fi_fprintf(fph,"; FBX 6.1.0 project file\r\n");
	fi_fprintf(fph,"; Copyright (C) 1997-2008 Autodesk Inc. and/or its licensors.\r\n");
	fi_fprintf(fph,"; All rights reserved.\r\n");
	fi_fprintf(fph,"; ----------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"FBXHeaderExtension:  {\r\n");
	fi_fprintf(fph,"    FBXHeaderVersion: 1003\r\n");
	fi_fprintf(fph,"    FBXVersion: 6100\r\n");
	fi_fprintf(fph,"    CreationTimeStamp:  {\r\n");
	fi_fprintf(fph,"        Version: 1000\r\n");
	fi_fprintf(fph,"        Year: 2009\r\n");
	fi_fprintf(fph,"        Month: 12\r\n");
	fi_fprintf(fph,"        Day: 3\r\n");
	fi_fprintf(fph,"        Hour: 18\r\n");
	fi_fprintf(fph,"        Minute: 6\r\n");
	fi_fprintf(fph,"        Second: 18\r\n");
	fi_fprintf(fph,"        Millisecond: 532\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    Creator: \"FBX SDK/FBX Plugins version 2010.2\"\r\n");
	fi_fprintf(fph,"    OtherFlags:  {\r\n");
	fi_fprintf(fph,"        FlagPLE: 0\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,"CreationTime: \"2009-12-03 18:06:18:532\"\r\n");
	fi_fprintf(fph,"Creator: \"FBX SDK/FBX Plugins build 20090731\"\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"; Document Description\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Document:  {\r\n");
	fi_fprintf(fph,"    Name: \"\"\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"; Document References\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"References:  {\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"; Object definitions\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Definitions:  {\r\n");
	fi_fprintf(fph,"    Version: 100\r\n");
	fi_fprintf(fph,"    Count: 5\r\n");
	fi_fprintf(fph,"    ObjectType: \"Model\" {\r\n");
	fi_fprintf(fph,"        Count: 2\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    ObjectType: \"Material\" {\r\n");
	fi_fprintf(fph,"        Count: 1\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    ObjectType: \"SceneInfo\" {\r\n");
	fi_fprintf(fph,"        Count: 1\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    ObjectType: \"GlobalSettings\" {\r\n");
	fi_fprintf(fph,"        Count: 1\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"; Object properties\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Objects:  {\r\n");
	fi_fprintf(fph,"    Model: \"Model::aconcave_root\", \"Null\" {\r\n");
	fi_fprintf(fph,"        Version: 232\r\n");
	fi_fprintf(fph,"        Properties60:  {\r\n");
	fi_fprintf(fph,"            Property: \"QuaternionInterpolate\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationOffset\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationPivot\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingOffset\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingPivot\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMin\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMax\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationOrder\", \"enum\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationSpaceForLimitOnly\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"AxisLen\", \"double\", \"\",10\r\n");
	fi_fprintf(fph,"            Property: \"PreRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"PostRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMin\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMax\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"InheritType\", \"enum\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMin\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMax\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricTranslation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricScaling\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"LookAtProperty\", \"object\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"UpVectorProperty\", \"object\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Show\", \"bool\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"NegativePercentShapeSupport\", \"bool\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"DefaultAttributeIndex\", \"int\", \"\",-1\r\n");
	fi_fprintf(fph,"            Property: \"Freeze\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"LODBox\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Translation\", \"Lcl Translation\", \"A+\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Rotation\", \"Lcl Rotation\", \"A+\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Scaling\", \"Lcl Scaling\", \"A+\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"Visibility\", \"Visibility\", \"A+\",1\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        MultiLayer: 0\r\n");
	fi_fprintf(fph,"        MultiTake: 1\r\n");
	fi_fprintf(fph,"        Shading: Y\r\n");
	fi_fprintf(fph,"        Culling: \"CullingOff\"\r\n");
	fi_fprintf(fph,"        TypeFlags: \"Null\"\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    Model: \"Model::Box01\", \"Mesh\" {\r\n");
	fi_fprintf(fph,"        Version: 232\r\n");
	fi_fprintf(fph,"        Properties60:  {\r\n");
	fi_fprintf(fph,"            Property: \"QuaternionInterpolate\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationOffset\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationPivot\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingOffset\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingPivot\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMin\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMax\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"TranslationMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationOrder\", \"enum\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationSpaceForLimitOnly\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationStiffnessZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"AxisLen\", \"double\", \"\",10\r\n");
	fi_fprintf(fph,"            Property: \"PreRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"PostRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMin\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMax\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"RotationMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"InheritType\", \"enum\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingActive\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMin\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMax\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMinZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxX\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxY\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"ScalingMaxZ\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricTranslation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricRotation\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"GeometricScaling\", \"Vector3D\", \"\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampRangeZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampRangeZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MinDampStrengthZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"MaxDampStrengthZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleX\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleY\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"PreferedAngleZ\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"LookAtProperty\", \"object\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"UpVectorProperty\", \"object\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Show\", \"bool\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"NegativePercentShapeSupport\", \"bool\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"DefaultAttributeIndex\", \"int\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"Freeze\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"LODBox\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Translation\", \"Lcl Translation\", \"A+\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Rotation\", \"Lcl Rotation\", \"A+\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"Lcl Scaling\", \"Lcl Scaling\", \"A+\",1,1,1\r\n");
	fi_fprintf(fph,"            Property: \"Visibility\", \"Visibility\", \"A+\",1\r\n");
	fi_fprintf(fph,"            Property: \"Color\", \"ColorRGB\", \"N\",0.8,0.8,0.8\r\n");
	fi_fprintf(fph,"            Property: \"BBoxMin\", \"Vector3D\", \"N\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"BBoxMax\", \"Vector3D\", \"N\",0,0,0\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        MultiLayer: 0\r\n");
	fi_fprintf(fph,"        MultiTake: 1\r\n");
	fi_fprintf(fph,"        Shading: Y\r\n");
	fi_fprintf(fph,"        Culling: \"CullingOff\"\r\n");

	if ( ms->mMeshCount )
	{
		Mesh *mesh = ms->mMeshes[0];
		writeVertices(fph,mesh);
		writePolygonVertexIndex(fph,mesh);

		fi_fprintf(fph,"        GeometryVersion: 124\r\n");

		writeUV(fph,mesh);
	}


	fi_fprintf(fph,"        LayerElementMaterial: 0 {\r\n");
	fi_fprintf(fph,"            Version: 101\r\n");
	fi_fprintf(fph,"            Name: \"\"\r\n");
	fi_fprintf(fph,"            MappingInformationType: \"AllSame\"\r\n");
	fi_fprintf(fph,"            ReferenceInformationType: \"IndexToDirect\"\r\n");
	fi_fprintf(fph,"            Materials: 0\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        LayerElementTexture: 0 {\r\n");
	fi_fprintf(fph,"            Version: 101\r\n");
	fi_fprintf(fph,"            Name: \"\"\r\n");
	fi_fprintf(fph,"            MappingInformationType: \"NoMappingInformation\"\r\n");
	fi_fprintf(fph,"            ReferenceInformationType: \"IndexToDirect\"\r\n");
	fi_fprintf(fph,"            BlendMode: \"Translucent\"\r\n");
	fi_fprintf(fph,"            TextureAlpha: 1\r\n");
	fi_fprintf(fph,"            TextureId:\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        Layer: 0 {\r\n");
	fi_fprintf(fph,"            Version: 100\r\n");
	fi_fprintf(fph,"            LayerElement:  {\r\n");
	fi_fprintf(fph,"                Type: \"LayerElementMaterial\"\r\n");
	fi_fprintf(fph,"                TypedIndex: 0\r\n");
	fi_fprintf(fph,"            }\r\n");
	fi_fprintf(fph,"            LayerElement:  {\r\n");
	fi_fprintf(fph,"                Type: \"LayerElementTexture\"\r\n");
	fi_fprintf(fph,"                TypedIndex: 0\r\n");
	fi_fprintf(fph,"            }\r\n");
	fi_fprintf(fph,"            LayerElement:  {\r\n");
	fi_fprintf(fph,"                Type: \"LayerElementUV\"\r\n");
	fi_fprintf(fph,"                TypedIndex: 0\r\n");
	fi_fprintf(fph,"            }\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        NodeAttributeName: \"Geometry::Box01_ncl1_1\"\r\n");
	fi_fprintf(fph,"    }\r\n");

	fi_fprintf(fph,"    SceneInfo: \"SceneInfo::GlobalInfo\", \"UserData\" {\r\n");
	fi_fprintf(fph,"        Type: \"UserData\"\r\n");
	fi_fprintf(fph,"        Version: 100\r\n");
	fi_fprintf(fph,"        MetaData:  {\r\n");
	fi_fprintf(fph,"            Version: 100\r\n");
	fi_fprintf(fph,"            Title: \"\"\r\n");
	fi_fprintf(fph,"            Subject: \"\"\r\n");
	fi_fprintf(fph,"            Author: \"\"\r\n");
	fi_fprintf(fph,"            Keywords: \"\"\r\n");
	fi_fprintf(fph,"            Revision: \"\"\r\n");
	fi_fprintf(fph,"            Comment: \"\"\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"        Properties60:  {\r\n");
	fi_fprintf(fph,"            Property: \"DocumentUrl\", \"KString\", \"\", \"aconcave.fbx\"\r\n");
	fi_fprintf(fph,"            Property: \"SrcDocumentUrl\", \"KString\", \"\", \"aconcave.fbx\"\r\n");
	fi_fprintf(fph,"            Property: \"Original\", \"Compound\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Original|ApplicationVendor\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Original|ApplicationName\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Original|ApplicationVersion\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Original|DateTime_GMT\", \"DateTime\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"Original|FileName\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"LastSaved\", \"Compound\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"LastSaved|ApplicationVendor\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"LastSaved|ApplicationName\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"LastSaved|ApplicationVersion\", \"KString\", \"\", \"\"\r\n");
	fi_fprintf(fph,"            Property: \"LastSaved|DateTime_GMT\", \"DateTime\", \"\", \"\"\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    Material: \"Material::Box01Mat\", \"\" {\r\n");
	fi_fprintf(fph,"        Version: 102\r\n");
	fi_fprintf(fph,"        ShadingModel: \"phong\"\r\n");
	fi_fprintf(fph,"        MultiLayer: 0\r\n");
	fi_fprintf(fph,"        Properties60:  {\r\n");
	fi_fprintf(fph,"            Property: \"ShadingModel\", \"KString\", \"\", \"Phong\"\r\n");
	fi_fprintf(fph,"            Property: \"MultiLayer\", \"bool\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"EmissiveColor\", \"ColorRGB\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"EmissiveFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"AmbientColor\", \"ColorRGB\", \"\",0.2,0.2,0.2\r\n");
	fi_fprintf(fph,"            Property: \"AmbientFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"DiffuseColor\", \"ColorRGB\", \"\",0.8,0.8,0.8\r\n");
	fi_fprintf(fph,"            Property: \"DiffuseFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"Bump\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"NormalMap\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"BumpFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"TransparentColor\", \"ColorRGB\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"TransparencyFactor\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"DisplacementColor\", \"ColorRGB\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"DisplacementFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"SpecularColor\", \"ColorRGB\", \"\",0.2,0.2,0.2\r\n");
	fi_fprintf(fph,"            Property: \"SpecularFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"ShininessExponent\", \"double\", \"\",20\r\n");
	fi_fprintf(fph,"            Property: \"ReflectionColor\", \"ColorRGB\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"ReflectionFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"Emissive\", \"Vector3D\", \"\",0,0,0\r\n");
	fi_fprintf(fph,"            Property: \"Ambient\", \"Vector3D\", \"\",0.2,0.2,0.2\r\n");
	fi_fprintf(fph,"            Property: \"Diffuse\", \"Vector3D\", \"\",0.8,0.8,0.8\r\n");
	fi_fprintf(fph,"            Property: \"Specular\", \"Vector3D\", \"\",0.2,0.2,0.2\r\n");
	fi_fprintf(fph,"            Property: \"Shininess\", \"double\", \"\",20\r\n");
	fi_fprintf(fph,"            Property: \"Opacity\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"Reflectivity\", \"double\", \"\",0\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    GlobalSettings:  {\r\n");
	fi_fprintf(fph,"        Version: 1000\r\n");
	fi_fprintf(fph,"        Properties60:  {\r\n");
	fi_fprintf(fph,"            Property: \"UpAxis\", \"int\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"UpAxisSign\", \"int\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"FrontAxis\", \"int\", \"\",2\r\n");
	fi_fprintf(fph,"            Property: \"FrontAxisSign\", \"int\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"CoordAxis\", \"int\", \"\",0\r\n");
	fi_fprintf(fph,"            Property: \"CoordAxisSign\", \"int\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"OriginalUpAxis\", \"int\", \"\",-1\r\n");
	fi_fprintf(fph,"            Property: \"OriginalUpAxisSign\", \"int\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"UnitScaleFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"            Property: \"OriginalUnitScaleFactor\", \"double\", \"\",1\r\n");
	fi_fprintf(fph,"        }\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"; Object connections\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Connections:  {\r\n");
	fi_fprintf(fph,"    Connect: \"OO\", \"Model::aconcave_root\", \"Model::Scene\"\r\n");
	fi_fprintf(fph,"    Connect: \"OO\", \"Model::Box01\", \"Model::aconcave_root\"\r\n");
	fi_fprintf(fph,"    Connect: \"OO\", \"Material::Box01Mat\", \"Model::Box01\"\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,";Takes and animation section\r\n");
	fi_fprintf(fph,";----------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Takes:  {\r\n");
	fi_fprintf(fph,"    Current: \"\"\r\n");
	fi_fprintf(fph,"}\r\n");
	fi_fprintf(fph,";Version 5 settings\r\n");
	fi_fprintf(fph,";------------------------------------------------------------------\r\n");
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"Version5:  {\r\n");
	fi_fprintf(fph,"    AmbientRenderSettings:  {\r\n");
	fi_fprintf(fph,"        Version: 101\r\n");
	fi_fprintf(fph,"        AmbientLightColor: 0.4,0.4,0.4,0\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    FogOptions:  {\r\n");
	fi_fprintf(fph,"        FlogEnable: 0\r\n");
	fi_fprintf(fph,"        FogMode: 0\r\n");
	fi_fprintf(fph,"        FogDensity: 0.002\r\n");
	fi_fprintf(fph,"        FogStart: 0.3\r\n");
	fi_fprintf(fph,"        FogEnd: 1000\r\n");
	fi_fprintf(fph,"        FogColor: 1,1,1,1\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    Settings:  {\r\n");
	fi_fprintf(fph,"        FrameRate: \"30\"\r\n");
	fi_fprintf(fph,"        TimeFormat: 1\r\n");
	fi_fprintf(fph,"        SnapOnFrames: 0\r\n");
	fi_fprintf(fph,"        ReferenceTimeIndex: -1\r\n");
	fi_fprintf(fph,"        TimeLineStartTime: 0\r\n");
	fi_fprintf(fph,"        TimeLineStopTime: 46186158000\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"    RendererSetting:  {\r\n");
	fi_fprintf(fph,"        DefaultCamera: \"\"\r\n");
	fi_fprintf(fph,"        DefaultViewingMode: 0\r\n");
	fi_fprintf(fph,"    }\r\n");
	fi_fprintf(fph,"}\r\n");


}
#else

void serializeFBX(FILE_INTERFACE * /*fph*/,MeshSystem * /*ms*/)
{
}

#endif

};// end of namespace

