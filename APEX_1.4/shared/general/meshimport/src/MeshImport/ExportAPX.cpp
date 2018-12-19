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
#include "ExportAPX.h"
#include "MeshImport.h"
#include "MiFloatMath.h"
#include "MiMemoryBuffer.h"
#include "MiIOStream.h"
#include "MiFileInterface.h"


#pragma warning(disable:4100 4996)

namespace mimp
{

static const char * getFloatString(MiF32 v)
{
	static char data[64*16];
	static MiI32  index=0;

	char *ret = &data[index*64];
	index++;
	if (index == 16 ) index = 0;

// This is not cross-platform, removing for now
#if 0
	if ( !_finite(v) )
	{
		assert(0);
		strcpy(ret,"0"); // not a valid number!
	}
	else 
#endif
	if ( v == 1 )
	{
		strcpy(ret,"1");
	}
	else if ( v == 0 )
	{
		strcpy(ret,"0");
	}
	else if ( v == - 1 )
	{
		strcpy(ret,"-1");
	}
	else
	{
		sprintf(ret,"%.9f", v );
		const char *dot = strstr(ret,".");
		if ( dot )
		{
			MiI32 len = (MiI32)strlen(ret);
			char *foo = &ret[len-1];
			while ( *foo == '0' ) foo--;
			if ( *foo == '.' )
				*foo = 0;
			else
				foo[1] = 0;
		}
	}

	return ret;
}



// Support for ridiculous completely hard coded data format enum
struct RenderDataFormat
{
	/** \brief the enum type */
	enum Enum
	{
		UNSPECIFIED =			0,	//!< No format (semantic not used)

		//!< Integer formats
		UBYTE1 =				1,	//!< One unsigned 8-bit integer (uint8_t[1])
		UBYTE2 =				2,	//!< Two unsigned 8-bit integers (uint8_t[2])
		UBYTE3 =				3,	//!< Three unsigned 8-bit integers (uint8_t[3])
		UBYTE4 =				4,	//!< Four unsigned 8-bit integers (uint8_t[4])

		USHORT1 =				5,	//!< One unsigned 16-bit integer (uint16_t[1])
		USHORT2 =				6,	//!< Two unsigned 16-bit integers (uint16_t[2])
		USHORT3 =				7,	//!< Three unsigned 16-bit integers (uint16_t[3])
		USHORT4 =				8,	//!< Four unsigned 16-bit integers (uint16_t[4])

		SHORT1 =				9,	//!< One signed 16-bit integer (int16_t[1])
		SHORT2 =				10,	//!< Two signed 16-bit integers (int16_t[2])
		SHORT3 =				11,	//!< Three signed 16-bit integers (int16_t[3])
		SHORT4 =				12,	//!< Four signed 16-bit integers (int16_t[4])

		UINT1 =					13,	//!< One unsigned integer (uint32_t[1])
		UINT2 =					14,	//!< Two unsigned integers (uint32_t[2])
		UINT3 =					15,	//!< Three unsigned integers (uint32_t[3])
		UINT4 =					16,	//!< Four unsigned integers (uint32_t[4])

		//!< Color formats
		R8G8B8A8 =				17,	//!< Four unsigned bytes (uint8_t[4]) representing red, green, blue, alpha
		B8G8R8A8 =				18,	//!< Four unsigned bytes (uint8_t[4]) representing blue, green, red, alpha
		R32G32B32A32_FLOAT =	19,	//!< Four floats (float[4]) representing red, green, blue, alpha
		B32G32R32A32_FLOAT =	20,	//!< Four floats (float[4]) representing blue, green, red, alpha

		//!< Normalized formats
		BYTE_UNORM1 =			21,	//!< One unsigned normalized value in the range [0,1], packed into 8 bits (uint8_t[1])
		BYTE_UNORM2 =			22,	//!< Two unsigned normalized value in the range [0,1], each packed into 8 bits (uint8_t[2])
		BYTE_UNORM3 =			23,	//!< Three unsigned normalized value in the range [0,1], each packed into bits (uint8_t[3])
		BYTE_UNORM4 =			24,	//!< Four unsigned normalized value in the range [0,1], each packed into 8 bits (uint8_t[4])

		SHORT_UNORM1 =			25,	//!< One unsigned normalized value in the range [0,1], packed into 16 bits (uint16_t[1])
		SHORT_UNORM2 =			26,	//!< Two unsigned normalized value in the range [0,1], each packed into 16 bits (uint16_t[2])
		SHORT_UNORM3 =			27,	//!< Three unsigned normalized value in the range [0,1], each packed into 16 bits (uint16_t[3])
		SHORT_UNORM4 =			28,	//!< Four unsigned normalized value in the range [0,1], each packed into 16 bits (uint16_t[4])

		BYTE_SNORM1 =			29,	//!< One signed normalized value in the range [-1,1], packed into 8 bits (uint8_t[1])
		BYTE_SNORM2 =			30,	//!< Two signed normalized value in the range [-1,1], each packed into 8 bits (uint8_t[2])
		BYTE_SNORM3 =			31,	//!< Three signed normalized value in the range [-1,1], each packed into bits (uint8_t[3])
		BYTE_SNORM4 =			32,	//!< Four signed normalized value in the range [-1,1], each packed into 8 bits (uint8_t[4])

		SHORT_SNORM1 =			33,	//!< One signed normalized value in the range [-1,1], packed into 16 bits (uint16_t[1])
		SHORT_SNORM2 =			34,	//!< Two signed normalized value in the range [-1,1], each packed into 16 bits (uint16_t[2])
		SHORT_SNORM3 =			35,	//!< Three signed normalized value in the range [-1,1], each packed into 16 bits (uint16_t[3])
		SHORT_SNORM4 =			36,	//!< Four signed normalized value in the range [-1,1], each packed into 16 bits (uint16_t[4])

		//!< Float formats
		HALF1 =					37,	//!< One 16-bit floating point value
		HALF2 =					38,	//!< Two 16-bit floating point values
		HALF3 =					39,	//!< Three 16-bit floating point values
		HALF4 =					40,	//!< Four 16-bit floating point values

		FLOAT1 =				41,	//!< One 32-bit floating point value
		FLOAT2 =				42,	//!< Two 32-bit floating point values
		FLOAT3 =				43,	//!< Three 32-bit floating point values
		FLOAT4 =				44,	//!< Four 32-bit floating point values

		FLOAT4x4 =				45,	//!< A 4x4 matrix (see physx::PxMat34)
		FLOAT3x3 =				46,	//!< A 3x3 matrix (see physx::PxMat33)

		FLOAT4_QUAT =			47,	//!< A quaternion (see physx::PxQuat)
		BYTE_SNORM4_QUATXYZW =	48,	//!< A normalized quaternion with signed byte elements, X,Y,Z,W format (uint8_t[4])
		SHORT_SNORM4_QUATXYZW =	49,	//!< A normalized quaternion with signed short elements, X,Y,Z,W format (uint16_t[4])

		NUM_FORMATS
	};
};


struct TempSubMesh
{

	TempSubMesh(Mesh *m,SubMesh *sm)
	{
		mVertexCount = 0;
		mIndexRemap = new MiU32[m->mVertexCount];
		mVertices = new MeshVertex[m->mVertexCount];
		memset(mIndexRemap,0xFF,sizeof(MiU32)*m->mVertexCount);

		for (MiU32 i=0; i<sm->mTriCount*3; i++)
		{
			MiU32 index = sm->mIndices[i];
			if ( mIndexRemap[index] == 0xFFFFFFFF )
			{
				mIndexRemap[index] = mVertexCount;
				mVertices[mVertexCount] = m->mVertices[index];
				mVertexCount++;
			}
		}
		mIndices = new MiU32[sm->mTriCount*3];
		for (MiU32 i=0; i<sm->mTriCount*3; i++)
		{
			mIndices[i] = mIndexRemap[ sm->mIndices[i] ];
		}
	}

	~TempSubMesh(void)
	{
		delete []mVertices;
		delete []mIndices;
		delete []mIndexRemap;
	}


	MiU32		mVertexCount;
	MeshVertex	*mVertices;
	MiU32		*mIndices;
	MiU32		*mIndexRemap;
};

struct RenderVertexSemantic
{
	/**
	\brief Enum of vertex buffer semantics types
	*/
	enum Enum
	{
		CUSTOM = -1,			//!< User-defined

		POSITION = 0,			//!< Position of vertex
		NORMAL,					//!< Normal at vertex
		TANGENT,				//!< Tangent at vertex
		BINORMAL,				//!< Binormal at vertex
		COLOR,					//!< Color at vertex
		TEXCOORD0,				//!< Texture coord 0 of vertex
		TEXCOORD1,				//!< Texture coord 1 of vertex
		TEXCOORD2,				//!< Texture coord 2 of vertex
		TEXCOORD3,				//!< Texture coord 3 of vertex
		BONE_INDEX,				//!< Bone index of vertex
		BONE_WEIGHT,			//!< Bone weight of vertex

		DISPLACEMENT_TEXCOORD,	//!< X Displacement map texture coord of vertex
		DISPLACEMENT_FLAGS,		//!< Displacement map flags of vertex

		NUM_SEMANTICS			//!< Count of standard semantics, not a valid semantic
	};
};

MI_INLINE MiU32 hash(const char* string)
{
	// "DJB" string hash
	MiU32 h = 5381;
	char c;
	while ((c = *string++) != '\0')
	{
		h = ((h << 5) + h) ^ c;
	}
	return h;
}

struct SemanticNameAndID
{
	SemanticNameAndID(const char* name, MiU32 id) : m_name(name), m_id(id)
	{
		MI_ASSERT(m_id != 0 || strcmp(m_name, "SEMANTIC_INVALID") == 0);
	}
	const char*					m_name;
	MiU32	m_id;
};

#define SEMANTIC_NAME_AND_ID( name )	SemanticNameAndID( name, (MiU32)hash( name ) )

static const SemanticNameAndID sSemanticNamesAndIDs[] =
{
	SEMANTIC_NAME_AND_ID("SEMANTIC_POSITION"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_NORMAL"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TANGENT"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BINORMAL"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_COLOR"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD0"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD1"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD2"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_TEXCOORD3"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BONE_INDEX"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_BONE_WEIGHT"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_DISPLACEMENT_TEXCOORD"),
	SEMANTIC_NAME_AND_ID("SEMANTIC_DISPLACEMENT_FLAGS"),

	SemanticNameAndID("SEMANTIC_INVALID", (MiU32)0)
};

static const char* getSemanticName(RenderVertexSemantic::Enum semantic)
{
	MI_ASSERT((MiU32)semantic < RenderVertexSemantic::NUM_SEMANTICS);
	return (MiU32)semantic < RenderVertexSemantic::NUM_SEMANTICS ? sSemanticNamesAndIDs[semantic].m_name : NULL;
}


static MiU32 getSemanticID(RenderVertexSemantic::Enum semantic)
{
	MI_ASSERT((MiU32)semantic < RenderVertexSemantic::NUM_SEMANTICS);
	return (MiU32)semantic < RenderVertexSemantic::NUM_SEMANTICS ? sSemanticNamesAndIDs[semantic].m_id : (MiU32)0;
}



void semanticStruct(MiU32 flags,MiU32 mask,RenderVertexSemantic::Enum semantic,RenderDataFormat::Enum format,FILE_INTERFACE *fph)
{
	if ( flags & mask )
	{
		fi_fprintf(fph,"                    <struct>\r\n");
		fi_fprintf(fph,"                      <value name=\"name\" type=\"String\">%s</value>\r\n",getSemanticName(semantic));
		fi_fprintf(fph,"                      <value name=\"semantic\" type=\"I32\">%d</value>\r\n", semantic);
		fi_fprintf(fph,"                      <value name=\"id\" type=\"U32\">%u</value>\r\n", getSemanticID(semantic));
		fi_fprintf(fph,"                      <value name=\"format\" type=\"U32\">%d</value>\r\n",format);
		fi_fprintf(fph,"                      <value name=\"access\" type=\"U32\">0</value>\r\n");
		fi_fprintf(fph,"                      <value name=\"serialize\" type=\"Bool\">true</value>\r\n");
		fi_fprintf(fph,"                    </struct>\r\n");
	}
}

static const char * getFormatString(RenderDataFormat::Enum format)
{
	const char *ret = NULL;
	switch ( format )
	{
		case RenderDataFormat::FLOAT4:
			ret = "BufferF32x4";
			break;
		case RenderDataFormat::FLOAT3:
			ret = "BufferF32x3";
			break;
		case RenderDataFormat::FLOAT2:
			ret = "BufferF32x2";
			break;
		case RenderDataFormat::USHORT4:
			ret = "BufferU16x4";
			break;
        case RenderDataFormat::UINT1:
			ret = "BufferU32x1";
			break;
		default:
			MI_ALWAYS_ASSERT();
			break;
	}
	return ret;
}

static const char * typeString(RenderDataFormat::Enum format)
{
	const char *ret = NULL;
	switch ( format )
	{
		case RenderDataFormat::FLOAT3:
			ret = "Vec3";
			break;
		case RenderDataFormat::UINT1:
			ret = "U32";
			break;
		case RenderDataFormat::FLOAT4:
		case RenderDataFormat::USHORT4:
		case RenderDataFormat::FLOAT2:
			ret = "Struct";
			break;
		default:
			MI_ALWAYS_ASSERT();
			break;
	}
	return ret;
}

static void semanticBuffer(MiU32 flags,MiU32 mask,RenderDataFormat::Enum format,MiU32 bufferCount,const MiU8 *scan,MiU32 bufferStride,FILE_INTERFACE *fph)
{
	if ( !(flags & mask) ) return;

	fi_fprintf(fph,"                <value type=\"Ref\" included=\"1\" className=\"%s\" version=\"0.0\" >\r\n", getFormatString(format));
	fi_fprintf(fph,"                  <struct name=\"\">\r\n");

	switch ( format )
	{
		case RenderDataFormat::USHORT4:
			fi_fprintf(fph,"                    <array name=\"data\" size=\"%d\" type=\"%s\" structElements=\"x(U16),y(U16),z(U16),w(U16)\">\r\n", bufferCount, typeString(format));
			break;
		case RenderDataFormat::FLOAT4:
			fi_fprintf(fph,"                    <array name=\"data\" size=\"%d\" type=\"%s\" structElements=\"x(F32),y(F32),z(F32),w(F32)\">\r\n", bufferCount, typeString(format));
			break;
		case RenderDataFormat::FLOAT2:
			fi_fprintf(fph,"                    <array name=\"data\" size=\"%d\" type=\"%s\" structElements=\"x(F32),y(F32)\">\r\n", bufferCount, typeString(format));
			break;
		case RenderDataFormat::FLOAT3:
		case RenderDataFormat::UINT1:
			fi_fprintf(fph,"                    <array name=\"data\" size=\"%d\" type=\"%s\">\r\n", bufferCount, typeString(format));
			break;
		default:
			MI_ALWAYS_ASSERT();
			break;
	}

	fi_fprintf(fph,"        ");

	for (MiU32 i=0; i<bufferCount; i++)
	{
		const MiF32 *p = (const MiF32 *)scan;
		switch ( format )
		{
			case RenderDataFormat::FLOAT4:
				fi_fprintf(fph,"%s %s %s  ", getFloatString(p[0]), getFloatString(p[1]), getFloatString(p[2]), getFloatString(p[3]) );
				if ( (i&3) == 0 )
				{
					fi_fprintf(fph,"\r\n        ");
				}
				break;

			case RenderDataFormat::FLOAT3:
				fi_fprintf(fph,"%s %s %s  ", getFloatString(p[0]), getFloatString(p[1]), getFloatString(p[2]) );
				if ( (i&3) == 0 )
				{
					fi_fprintf(fph,"\r\n        ");
				}
				break;
			case RenderDataFormat::FLOAT2:
				fi_fprintf(fph,"%s %s  ", getFloatString(p[0]), getFloatString(p[1]) );
				if ( (i&7) == 0 )
				{
					fi_fprintf(fph,"\r\n        ");
				}
				break;
			case RenderDataFormat::USHORT4:
				{
					const MiU16 *us = (const MiU16 *)scan;
					fi_fprintf(fph,"%d %d %d %d  ", us[0], us[1], us[2], us[3] );
					if ( (i&7) == 0 )
					{
						fi_fprintf(fph,"\r\n        ");
					}
				}
				break;
			case RenderDataFormat::UINT1:
				{
					const MiU32 *us = (const MiU32 *)scan;
					fi_fprintf(fph,"%d  ", us[0]);
					if ( (i&31) == 0 )
					{
						fi_fprintf(fph,"\r\n        ");
					}
				}
				break;
			default:
				MI_ALWAYS_ASSERT();
				break;
		}
		scan+=bufferStride;
	}
	fi_fprintf(fph,"\r\n");
	fi_fprintf(fph,"                    </array>\r\n");
	fi_fprintf(fph,"                  </struct>\r\n");
	fi_fprintf(fph,"                </value>\r\n");

}

static void exportSubMesh(Mesh *m,SubMesh *sm,FILE_INTERFACE *fph)
{
	TempSubMesh tms(m,sm);

	MiU32 flags = m->mVertexFlags;
	MiU32 semanticCount = 0;

	if ( flags & MIVF_POSITION )
	{
		semanticCount++;
	}
	if ( flags & MIVF_NORMAL )
	{
		semanticCount++;
	}
	if ( flags & MIVF_TANGENT )
	{
		semanticCount++;
	}
	if ( flags & MIVF_BINORMAL )
	{
		semanticCount++;
	}
	if ( flags & MIVF_COLOR )
	{
		semanticCount++;
	}
	if ( flags & MIVF_TEXEL1 )
	{
		semanticCount++;
	}
	if ( flags & MIVF_TEXEL2 )
	{
		semanticCount++;
	}
	if ( flags & MIVF_TEXEL3 )
	{
		semanticCount++;
	}
	if ( flags & MIVF_TEXEL4 )
	{
		semanticCount++;
	}
	if ( flags & MIVF_BONE_WEIGHTING )
	{
		semanticCount+=2;
	}


  fi_fprintf(fph,"      <value type=\"Ref\" included=\"1\" className=\"SubmeshParameters\" version=\"0.0\" >\r\n");
  fi_fprintf(fph,"        <struct name=\"\">\r\n");
  fi_fprintf(fph,"          <value name=\"vertexBuffer\" type=\"Ref\" included=\"1\" className=\"VertexBufferParameters\" version=\"0.0\" >\r\n");
  fi_fprintf(fph,"            <struct name=\"\">\r\n");
  fi_fprintf(fph,"              <value name=\"vertexCount\" type=\"U32\">%d</value>\r\n", tms.mVertexCount);
  fi_fprintf(fph,"              <value name=\"vertexFormat\" type=\"Ref\" included=\"1\" className=\"VertexFormatParameters\" version=\"0.0\" >\r\n");
  fi_fprintf(fph,"                <struct name=\"\">\r\n");
  fi_fprintf(fph,"                  <value name=\"winding\" type=\"U32\">0</value>\r\n");
  fi_fprintf(fph,"                  <value name=\"hasSeparateBoneBuffer\" type=\"Bool\">false</value>\r\n");
  fi_fprintf(fph,"                  <array name=\"bufferFormats\" size=\"%d\" type=\"Struct\">\r\n", semanticCount);

  semanticStruct(flags,MIVF_POSITION,RenderVertexSemantic::POSITION,RenderDataFormat::FLOAT3,fph);
  semanticStruct(flags,MIVF_NORMAL,RenderVertexSemantic::NORMAL,RenderDataFormat::FLOAT3,fph);
  semanticStruct(flags,MIVF_TANGENT,RenderVertexSemantic::TANGENT,RenderDataFormat::FLOAT3,fph);
  semanticStruct(flags,MIVF_BINORMAL,RenderVertexSemantic::BINORMAL,RenderDataFormat::FLOAT3,fph);
  semanticStruct(flags,MIVF_COLOR,RenderVertexSemantic::COLOR,RenderDataFormat::UINT1,fph);
  semanticStruct(flags,MIVF_TEXEL1,RenderVertexSemantic::TEXCOORD0,RenderDataFormat::FLOAT2,fph);
  semanticStruct(flags,MIVF_TEXEL2,RenderVertexSemantic::TEXCOORD1,RenderDataFormat::FLOAT2,fph);
  semanticStruct(flags,MIVF_TEXEL3,RenderVertexSemantic::TEXCOORD2,RenderDataFormat::FLOAT2,fph);
  semanticStruct(flags,MIVF_TEXEL4,RenderVertexSemantic::TEXCOORD3,RenderDataFormat::FLOAT2,fph);
  semanticStruct(flags,MIVF_BONE_WEIGHTING,RenderVertexSemantic::BONE_INDEX,RenderDataFormat::USHORT4,fph);
  semanticStruct(flags,MIVF_BONE_WEIGHTING,RenderVertexSemantic::BONE_WEIGHT,RenderDataFormat::FLOAT4,fph);

  fi_fprintf(fph,"\r\n");
  fi_fprintf(fph,"                  </array>\r\n");
  fi_fprintf(fph,"                </struct>\r\n");
  fi_fprintf(fph,"              </value>\r\n");

  fi_fprintf(fph,"              <array name=\"buffers\" size=\"%d\" type=\"Ref\">\r\n", semanticCount);

  semanticBuffer(flags,MIVF_POSITION,RenderDataFormat::FLOAT3, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mPos,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_NORMAL,RenderDataFormat::FLOAT3, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mNormal,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_TANGENT,RenderDataFormat::FLOAT3, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mTangent,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_BINORMAL,RenderDataFormat::FLOAT3, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mBiNormal,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_COLOR,RenderDataFormat::UINT1, tms.mVertexCount, (const MiU8 *)(&tms.mVertices[0].mColor),sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_TEXEL1,RenderDataFormat::FLOAT2, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mTexel1,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_TEXEL2,RenderDataFormat::FLOAT2, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mTexel2,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_TEXEL3,RenderDataFormat::FLOAT2, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mTexel3,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_TEXEL4,RenderDataFormat::FLOAT2, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mTexel4,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_BONE_WEIGHTING,RenderDataFormat::USHORT4, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mBone,sizeof(MeshVertex),fph);
  semanticBuffer(flags,MIVF_BONE_WEIGHTING,RenderDataFormat::FLOAT4, tms.mVertexCount, (const MiU8 *)tms.mVertices[0].mWeight,sizeof(MeshVertex),fph);



  fi_fprintf(fph,"              </array>\r\n");
  fi_fprintf(fph,"            </struct>\r\n");
  fi_fprintf(fph,"          </value>\r\n");
  fi_fprintf(fph,"          <array name=\"indexBuffer\" size=\"%d\" type=\"U32\">\r\n", sm->mTriCount*3);
	fi_fprintf(fph,"        ");
  for (MiU32 i=0; i<sm->mTriCount*3; i++)
  {
	fi_fprintf(fph,"%d ", tms.mIndices[i]);
	if ( (i&15) == 0 )
	{
		fi_fprintf(fph,"\r\n        ");
	}
  }
	fi_fprintf(fph,"\r\n");

  fi_fprintf(fph,"          </array>\r\n");
  fi_fprintf(fph,"          <array name=\"vertexPartition\" size=\"2\" type=\"U32\">\r\n");
  fi_fprintf(fph,"            0 %d\r\n", tms.mVertexCount);
  fi_fprintf(fph,"          </array>\r\n");
  fi_fprintf(fph,"          <array name=\"indexPartition\" size=\"2\" type=\"U32\">\r\n");
  fi_fprintf(fph,"            0 %d\r\n", sm->mTriCount*3);
  fi_fprintf(fph,"          </array>\r\n");
  fi_fprintf(fph,"        </struct>\r\n");
  fi_fprintf(fph,"      </value>\r\n");







}

void *serializeAPX(const MeshSystem *ms,MiU32 &len)
{
	void * ret = NULL;
	len = 0;

	if ( ms )
	{
		FILE_INTERFACE *fph = fi_fopen(ms->mAssetName,"wmem",NULL,0);
		if ( fph )
		{


			fi_fprintf(fph,"<!DOCTYPE NvParameters>\r\n");
			fi_fprintf(fph,"	<NvParameters numObjects=\"1\" version=\"1.0\" >\r\n");


			fi_fprintf(fph,"<value name=\"\" type=\"Ref\" className=\"RenderMeshAssetParameters\" version=\"0.0\">\r\n");
			fi_fprintf(fph,"	<struct name=\"\">\r\n");

			MiU32 subMeshCount=0;
			for (MiU32 i=0; i<ms->mMeshCount; i++)
			{
				Mesh *m = ms->mMeshes[i];
				subMeshCount+=m->mSubMeshCount;
			}


			fi_fprintf(fph,"<array name=\"submeshes\" size=\"%d\" type=\"Ref\">\r\n", subMeshCount );


			for (MiU32 i=0; i<ms->mMeshCount; i++)
			{
				Mesh *m = ms->mMeshes[i];
				for (MiU32 j=0; j<m->mSubMeshCount; j++)
				{
					SubMesh *sm = m->mSubMeshes[j];
					exportSubMesh(m,sm,fph);
				}
			}

			fi_fprintf(fph,"</array>\r\n");


			{
				fi_fprintf(fph,"	<array name=\"materialNames\" size=\"%d\" type=\"String\">\r\n", subMeshCount );

				for (MiU32 i=0; i<ms->mMeshCount; i++)
				{
					Mesh *m = ms->mMeshes[i];
					for (MiU32 j=0; j<m->mSubMeshCount; j++)
					{
						SubMesh *sm = m->mSubMeshes[j];
						fi_fprintf(fph,"	<value type=\"String\">%s</value>\r\n", sm->mMaterialName );
					}
				}
				fi_fprintf(fph,"</array>\r\n");
			}

			fi_fprintf(fph,"<array name=\"partBounds\" size=\"1\" type=\"Bounds3\">\r\n");

			fi_fprintf(fph,"	%0.9f %0.9f %0.9f   %0.9f %0.9f %0.9f\r\n", 
				ms->mAABB.mMin[0],
				ms->mAABB.mMin[1],
				ms->mAABB.mMin[2],
				ms->mAABB.mMax[0],
				ms->mAABB.mMax[1],
				ms->mAABB.mMax[2]);

			fi_fprintf(fph,"	</array>\r\n");
			fi_fprintf(fph,"	<value name=\"textureUVOrigin\" type=\"U32\">0</value>\r\n");
			fi_fprintf(fph,"	<value name=\"boneCount\" type=\"U32\">1</value>\r\n");
			fi_fprintf(fph,"	<value name=\"deleteStaticBuffersAfterUse\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"	<value name=\"isReferenced\" type=\"Bool\">false</value>\r\n");

			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"	</value>\r\n");

			fi_fprintf(fph,"	</NvParameters>\r\n");

			size_t outLen;
			void *outMem = fi_getMemBuffer(fph,&outLen);
			if ( outMem )
			{
				len = (MiU32)outLen;
        if ( len )
        {
        				ret = MI_ALLOC(len);
				        memcpy(ret,outMem,len);
        }
			}
			fi_fclose(fph);
		}
	}
	return ret;
}


};// end of namespace




