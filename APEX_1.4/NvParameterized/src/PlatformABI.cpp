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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

#include "PxSimpleTypes.h"
#include "PlatformABI.h"
#include "NvSerializerInternal.h"
#include "SerializerCommon.h"

using namespace NvParameterized;

//Returns ABI for predefined platforms
Serializer::ErrorType PlatformABI::GetPredefinedABI(const SerializePlatform &platform, PlatformABI &params)
{
	//Most common parameters
	params.endian = LITTLE;
	params.sizes.Char = 1;
	params.sizes.Bool = 1;
	params.sizes.pointer = 4;
	params.sizes.real = 4; //float
	params.aligns.Char = 1;
	params.aligns.pointer = 4;
	params.aligns.Bool = 1;
	params.aligns.i8 = 1;
	params.aligns.i16 = 2;
	params.aligns.i32 = 4;
	params.aligns.i64 = 8;
	params.aligns.f32 = 4;
	params.aligns.f64 = 8;
	params.aligns.real = params.aligns.f32;
	params.doReuseParentPadding = false;
	params.doEbo = true;

	SerializePlatform knownPlatform;

	//TODO: all those GetPlatforms are ugly

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("VcWin32", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		//Default params are ok
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("VcWin64", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("VcXbox360", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		//Pointers remain 32-bit
		params.endian = BIG;
		return Serializer::ERROR_NONE;
	}

	// Snc says that it's binary compatible with Gcc...
	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("GccPs3", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		//Pointers remain 32-bit
		params.doReuseParentPadding = true;
		params.endian = BIG;
		return Serializer::ERROR_NONE;
	}

	// Same as ps3 but little endian
	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("AndroidARM", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		//Pointers remain 32-bit
		params.doReuseParentPadding = true;
		params.endian = LITTLE;
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN(GetPlatform("GccLinux32", knownPlatform), Serializer::ERROR_UNKNOWN);
	if (knownPlatform == platform)
	{
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN(GetPlatform("GccLinux64", knownPlatform), Serializer::ERROR_UNKNOWN);
	if (knownPlatform == platform)
	{
		params.doReuseParentPadding = true;
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}

	// FIXME: true ABI is much more complicated (sizeof(bool) is 4, etc.)
	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("GccOsX32", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		params.doReuseParentPadding = true; // TODO (JPB): Is this correct?
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("GccOsX64", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		params.doReuseParentPadding = true;
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("Pib", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		params.endian = BIG;

		params.sizes.Char = params.sizes.Bool = 1;
		params.sizes.pointer = 4;
		params.sizes.real = 4; //float

		//All alignments are 1 to minimize space
		uint32_t *aligns = (uint32_t *)&params.aligns;
		for(uint32_t i = 0; i < sizeof(params.aligns)/sizeof(uint32_t); ++i)
			aligns[i] = 1;

		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("VcXboxOne", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN( GetPlatform("GccPs4", knownPlatform), Serializer::ERROR_UNKNOWN );
	if( knownPlatform == platform )
	{
		// if you don't set this then the in-place binary files could contain padding
		// between the NvParameters class and the parameterized data.
		params.doReuseParentPadding = true;
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN(GetPlatform("HOSARM32", knownPlatform), Serializer::ERROR_UNKNOWN);
	if (knownPlatform == platform)
	{
		//Default params are ok
		return Serializer::ERROR_NONE;
	}

	NV_BOOL_ERR_CHECK_RETURN(GetPlatform("HOSARM64", knownPlatform), Serializer::ERROR_UNKNOWN);
	if (knownPlatform == platform)
	{
		params.doReuseParentPadding = true;
		params.sizes.pointer = params.aligns.pointer = 8;
		return Serializer::ERROR_NONE;
	}	

	//Add new platforms here

	return Serializer::ERROR_INVALID_PLATFORM_NAME;
}

uint32_t PlatformABI::getNatAlignment(const Definition *pd) const
{
	switch( pd->type() )
	{
	case TYPE_ARRAY:
		return pd->arraySizeIsFixed()
			? getAlignment(pd->child(0)) //Array alignment = mermber alignment
			: NvMax3(aligns.pointer, aligns.Bool, aligns.i32); //Dynamic array is DummyDynamicArrayStruct

	case TYPE_STRUCT:
		{
			//Struct alignment is max of fields' alignment
			uint32_t align = 1;
			for(int32_t i = 0; i < pd->numChildren(); ++i)
				align = physx::PxMax(align, getAlignment(pd->child(i)));

			return align;
		}

	case TYPE_STRING:
		return physx::PxMax(aligns.pointer, aligns.Bool); //String = DummyDynamicStringStruct

	case TYPE_I8:
	case TYPE_U8:
		return aligns.i8;

	case TYPE_I16:
	case TYPE_U16:
		return aligns.i16;

	case TYPE_I32:
	case TYPE_U32:
		return aligns.i32;

	case TYPE_I64:
	case TYPE_U64:
		return aligns.i64;

	case TYPE_F32:
	case TYPE_VEC2:
	case TYPE_VEC3:
	case TYPE_VEC4:
	case TYPE_QUAT:
	case TYPE_MAT33:
	case TYPE_BOUNDS3:
	case TYPE_MAT34:
	case TYPE_MAT44:
	case TYPE_TRANSFORM:
		return aligns.f32;

	case TYPE_F64:
		return aligns.f64;

	case TYPE_ENUM:
	case TYPE_REF:
	case TYPE_POINTER:
		return aligns.pointer;

	case TYPE_BOOL:
		return aligns.Bool;

	NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
	default:
		PX_ASSERT( 0 && "Unexpected type" );
	}

	return UINT32_MAX;
}

//Returns alignment of given DataType
uint32_t PlatformABI::getAlignment(const Definition *pd) const
{
	uint32_t natAlign = getNatAlignment(pd),
		customAlign = pd->alignment();

	// Alignment of dynamic array means alignment of dynamic memory
	return !customAlign || ( TYPE_ARRAY == pd->type() && !pd->arraySizeIsFixed() )
		? natAlign
		: physx::PxMax(natAlign, customAlign);
}

//Returns alignment of given DataType
uint32_t PlatformABI::getPadding(const Definition *pd) const
{
	uint32_t natAlign = getNatAlignment(pd),
		customPad = pd->padding();

	// Alignment of dynamic array means alignment of dynamic memory
	return !customPad || ( TYPE_ARRAY == pd->type() && !pd->arraySizeIsFixed() )
		? natAlign
		: physx::PxMax(natAlign, customPad);
}

//Returns size of given DataType
uint32_t PlatformABI::getSize(const Definition *pd) const
{
	switch( pd->type() )
	{
	case TYPE_ARRAY:
		if( pd->arraySizeIsFixed() )
		{
			//Size of static array = number of elements * size of element
			const Definition *elemPd = pd;
			uint32_t totalSize = 1;
			for(int32_t i = 0; i < pd->arrayDimension(); ++i)
			{
				// Currently no nested dynamic arrays
				NV_BOOL_ERR_CHECK_RETURN( elemPd->arraySizeIsFixed(), Serializer::ERROR_NOT_IMPLEMENTED );

				int32_t size = elemPd->arraySize();
				totalSize *= size;

				elemPd = elemPd->child(0);
			}
			return totalSize * getSize(elemPd);
		}
		else
		{
			//Dynamic array = DummyDynamicArrayStruct

			uint32_t totalAlign = NvMax3(aligns.pointer, aligns.Bool, aligns.i32);

			uint32_t size = sizes.pointer; //buf
			size = align(size, aligns.Bool) + 1U; //isAllocated
			size = align(size, aligns.i32) + 4U; //elementSize
			size = align(size, aligns.i32) + pd->arrayDimension() * 4U; //arraySizes

			uint32_t paddedSize = align(size, totalAlign);

			return paddedSize;
		}

	case TYPE_STRUCT:
		{
			if( !pd->numChildren() )
				return 1;

			//Size of struct = sum of member sizes + sum of padding bytes + tail padding

			uint32_t totalAlign = 1, size = 0;
			for(int32_t i = 0; i < pd->numChildren(); ++i)
			{
				uint32_t childAlign = getAlignment(pd->child(i));
				totalAlign = physx::PxMax(totalAlign, childAlign);
				size = align(size, childAlign) + getSize(pd->child(i));
			}

			uint32_t customPad = pd->padding();
			if( customPad )
				totalAlign = physx::PxMax(totalAlign, customPad);

			return align(size, totalAlign); //Tail padding bytes
		}

	case TYPE_STRING:
		{
			//String = DummyDynamicStringStruct

			uint32_t totalAlign = physx::PxMax(aligns.pointer, aligns.Bool);

			uint32_t size = sizes.pointer; //buf
			size = align(size, aligns.Bool) + 1U; //isAllocated

			uint32_t paddedSize = align(size, totalAlign);

			return paddedSize;
		}

	case TYPE_I8:
	case TYPE_U8:
		return 1;

	case TYPE_I16:
	case TYPE_U16:
		return 2;

	case TYPE_I32:
	case TYPE_U32:
	case TYPE_F32:
		return 4;

	case TYPE_I64:
	case TYPE_U64:
	case TYPE_F64:
		return 8;

	// Vectors and matrices are structs so we need tail padding

	case TYPE_VEC2:
		return align(2 * sizes.real, aligns.real);
	case TYPE_VEC3:
		return align(3 * sizes.real, aligns.real);
	case TYPE_VEC4:
		return align(4 * sizes.real, aligns.real);
	case TYPE_QUAT:
		return align(4 * sizes.real, aligns.real);
	case TYPE_MAT33:
		return align(9 * sizes.real, aligns.real);
	case TYPE_MAT34:
		return align(12 * sizes.real, aligns.real);
	case TYPE_MAT44:
		return align(16 * sizes.real, aligns.real);
	case TYPE_BOUNDS3:
		return align(6 * sizes.real, aligns.real);
	case TYPE_TRANSFORM:
		return align(7 * sizes.real, aligns.real);

	case TYPE_ENUM:
	case TYPE_REF:
	case TYPE_POINTER:
		return sizes.pointer;

	case TYPE_BOOL:
		return sizes.Bool;

	NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
	default:
		PX_ASSERT( 0 && "Unexpected type" );
	}

	return UINT32_MAX; //This is never reached
}

bool PlatformABI::VerifyCurrentPlatform()
{
	//See PlatformABI::isNormal

	uint8_t one = 1,
		zero = 0;

	struct Empty {};

	return 1 == GetAlignment<bool>::value && 1 == sizeof(bool)
		&& 1 == GetAlignment<uint8_t>::value
		&& 1 == GetAlignment<char>::value
		&& 4 == sizeof(float)
		&& *(bool *)&one //We assume that 0x1 corresponds to true internally
		&& !*(bool *)&zero //We assume that 0x0 corresponds to false internally
		&& 1 == sizeof(Empty); // Serializer expects sizeof empty struct to be 1
}
