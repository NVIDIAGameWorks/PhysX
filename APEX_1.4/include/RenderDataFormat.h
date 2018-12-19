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



#ifndef RENDER_DATA_FORMAT_H
#define RENDER_DATA_FORMAT_H

#include "ApexUsingNamespace.h"
#include "foundation/PxAssert.h"
#include "foundation/PxMat44.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT

/**
\brief Enumeration of possible formats of various buffer semantics

N.B.: DO NOT CHANGE THE VALUES OF OLD FORMATS.
*/
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

		FLOAT4x4 =				45,	//!< A 4x4 matrix (see PxMat44)
		FLOAT3x4 =				46,	//!< A 3x4 matrix (see float[12])
		FLOAT3x3 =				47,	//!< A 3x3 matrix (see PxMat33)

		FLOAT4_QUAT =			48,	//!< A quaternion (see PxQuat)
		BYTE_SNORM4_QUATXYZW =	49,	//!< A normalized quaternion with signed byte elements, X,Y,Z,W format (uint8_t[4])
		SHORT_SNORM4_QUATXYZW =	50,	//!< A normalized quaternion with signed short elements, X,Y,Z,W format (uint16_t[4])

		NUM_FORMATS
	};

	/// Get byte size of format type
	static PX_INLINE uint32_t getFormatDataSize(Enum format)
	{
		switch (format)
		{
		default:
			PX_ALWAYS_ASSERT();
		case UNSPECIFIED:
			return 0;

		case UBYTE1:
		case BYTE_UNORM1:
		case BYTE_SNORM1:
			return sizeof(uint8_t);
		case UBYTE2:
		case BYTE_UNORM2:
		case BYTE_SNORM2:
			return sizeof(uint8_t) * 2;
		case UBYTE3:
		case BYTE_UNORM3:
		case BYTE_SNORM3:
			return sizeof(uint8_t) * 3;
		case UBYTE4:
		case BYTE_UNORM4:
		case BYTE_SNORM4:
		case BYTE_SNORM4_QUATXYZW:
			return sizeof(uint8_t) * 4;

		case USHORT1:
		case SHORT1:
		case HALF1:
		case SHORT_UNORM1:
		case SHORT_SNORM1:
			return sizeof(uint16_t);
		case USHORT2:
		case SHORT2:
		case HALF2:
		case SHORT_UNORM2:
		case SHORT_SNORM2:
			return sizeof(uint16_t) * 2;
		case USHORT3:
		case SHORT3:
		case HALF3:
		case SHORT_UNORM3:
		case SHORT_SNORM3:
			return sizeof(uint16_t) * 3;
		case USHORT4:
		case SHORT4:
		case HALF4:
		case SHORT_UNORM4:
		case SHORT_SNORM4:
		case SHORT_SNORM4_QUATXYZW:
			return sizeof(uint16_t) * 4;

		case UINT1:
			return sizeof(uint32_t);
		case UINT2:
			return sizeof(uint32_t) * 2;
		case UINT3:
			return sizeof(uint32_t) * 3;
		case UINT4:
			return sizeof(uint32_t) * 4;

		case R8G8B8A8:
		case B8G8R8A8:
			return sizeof(uint8_t) * 4;

		case R32G32B32A32_FLOAT:
		case B32G32R32A32_FLOAT:
			return sizeof(float) * 4;

		case FLOAT1:
			return sizeof(float);
		case FLOAT2:
			return sizeof(float) * 2;
		case FLOAT3:
			return sizeof(float) * 3;
		case FLOAT4:
			return sizeof(float) * 4;

		case FLOAT4x4:
			return sizeof(PxMat44);

		case FLOAT3x4:
			return sizeof(float) * 12;

		case FLOAT3x3:
			return sizeof(PxMat33);

		case FLOAT4_QUAT:
			return sizeof(PxQuat);
		}
	}
};

PX_POP_PACK

}
} // end namespace nvidia::apex

#endif	// RENDER_DATA_FORMAT_H
