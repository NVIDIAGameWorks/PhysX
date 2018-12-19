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

#include "PlatformInputStream.h"

using namespace NvParameterized;

PlatformInputStream::PlatformInputStream(physx::PxFileBuf &stream, const PlatformABI &targetParams, Traits *traits)
	: PlatformStream(targetParams, traits), mStream(stream), mPos(traits), mStartPos(stream.tellRead())
{}

Serializer::ErrorType PlatformInputStream::skipBytes(uint32_t nbytes)
{
	uint32_t newPos = mStream.tellRead() + nbytes;

	if( newPos >= mStream.getFileLength() )
	{
		DEBUG_ALWAYS_ASSERT();
		return Serializer::ERROR_INVALID_INTERNAL_PTR;
	}

	mStream.seekRead(newPos);

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType PlatformInputStream::pushPos(uint32_t newPos)
{
	if( !newPos )
	{
		//Trying to access NULL
		DEBUG_ALWAYS_ASSERT();
		return Serializer::ERROR_INVALID_INTERNAL_PTR;
	}

	if( mStartPos + newPos >= mStream.getFileLength() )
	{
		DEBUG_ALWAYS_ASSERT();
		return Serializer::ERROR_INVALID_INTERNAL_PTR;
	}

	mPos.pushBack(mStream.tellRead());
	mStream.seekRead(mStartPos + newPos);

	return Serializer::ERROR_NONE;
}

void PlatformInputStream::popPos()
{
	mStream.seekRead(mPos.back());
	mPos.popBack();
}

uint32_t PlatformInputStream::getPos() const
{
	return mStream.tellRead() - mStartPos;
}

Serializer::ErrorType PlatformInputStream::readString(uint32_t off, const char *&s)
{
	if( !off )
	{
		s = 0;
		return Serializer::ERROR_NONE;
	}

	NV_ERR_CHECK_RETURN( pushPos(off) );

	StringBuf buf(mTraits);
	buf.reserve(10);

	int8_t c;
	do
	{
		NV_ERR_CHECK_RETURN( read(c) );
		buf.append((char)c);
	} while( c );

	popPos();

	s = buf.getBuffer();
	return Serializer::ERROR_NONE;
}

Serializer::ErrorType PlatformInputStream::readObjHeader(ObjHeader &hdr)
{
	uint32_t align_ = NvMax3(mTargetParams.aligns.Bool, mTargetParams.aligns.i32, mTargetParams.aligns.pointer);

	beginStruct(align_);

	uint32_t prevPos = getPos();

	uint32_t relOffset;
	NV_ERR_CHECK_RETURN(read(relOffset));

	hdr.dataOffset = prevPos + relOffset; //Convert relative offset to absolute

	uint32_t classNameOff;
	NV_ERR_CHECK_RETURN( readPtr(classNameOff) );
	NV_ERR_CHECK_RETURN( readString(classNameOff, hdr.className) );

	uint32_t nameOff;
	NV_ERR_CHECK_RETURN( readPtr(nameOff) );
	NV_ERR_CHECK_RETURN( readString(nameOff, hdr.name) );

	NV_ERR_CHECK_RETURN( read(hdr.isIncluded) );
	NV_ERR_CHECK_RETURN( read(hdr.version) );
	NV_ERR_CHECK_RETURN( read(hdr.checksumSize) );

	uint32_t chkOff;
	NV_ERR_CHECK_RETURN( readPtr(chkOff) );

	if( !chkOff )
		hdr.checksum = 0;
	else
	{
		NV_ERR_CHECK_RETURN( pushPos(chkOff) );

		//TODO: usually we can avoid memory allocation here because checksumSize is 4
		hdr.checksum = (uint32_t *)mTraits->alloc(4 * hdr.checksumSize);
		for(uint32_t i = 0; i < hdr.checksumSize; ++i)
			NV_ERR_CHECK_RETURN( read( const_cast<uint32_t &>(hdr.checksum[i]) ) );

		popPos();
	}

	closeStruct();

	NV_ERR_CHECK_RETURN( pushPos(hdr.dataOffset) );

	//Skip NvParameters fields
	//WARN: this depends on implementation of NvParameters

	beginStruct(physx::PxMax(mTargetParams.aligns.pointer, mTargetParams.aligns.Bool));

	uint32_t tmp;
	for(uint32_t i = 0; i < 8; readPtr(tmp), ++i); //vtable and other fields

	bool mDoDeallocateSelf;
	NV_ERR_CHECK_RETURN( read(mDoDeallocateSelf) );

	bool mDoDeallocateName;
	NV_ERR_CHECK_RETURN( read(mDoDeallocateName) );

	bool mDoDeallocateClassName;
	NV_ERR_CHECK_RETURN( read(mDoDeallocateClassName) );

	//See note in beginObject
	if( !mTargetParams.doReuseParentPadding )
		closeStruct();

	hdr.dataOffset = getPos();

	popPos();

	return Serializer::ERROR_NONE;
}

Serializer::ErrorType PlatformInputStream::readSimpleStructArray(Handle &handle)
{
	int32_t n;
	handle.getArraySize(n);

	const NvParameterized::Definition *pdStruct = handle.parameterDefinition()->child(0);
	int32_t nfields = pdStruct->numChildren();

	uint32_t align_ = getTargetAlignment(pdStruct),
		pad_ = getTargetPadding(pdStruct); 

	align(align_);

	for(int32_t i = 0; i < n; ++i)
	{
		beginStruct(align_, pad_);
		handle.set(i);

		for(int32_t j = 0; j < nfields; ++j)
		{
			handle.set(j);

			const Definition *pdField = pdStruct->child(j);

			if( pdField->alignment() )
				align( pdField->alignment() );

			if( pdField->hint("DONOTSERIALIZE") )
			{
				//Simply skip bytes
				align(getTargetAlignment(pdField));
				NV_ERR_CHECK_RETURN( skipBytes(getTargetSize(pdField)) );
			}
			else
			{
				//No need to align structs because of tail padding
				switch( pdField->type() )
				{
#				define NV_PARAMETERIZED_TYPES_NO_LEGACY_TYPES
#				define NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#				define NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#				define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
				case TYPE_##enum_name: \
					{ \
						c_type val; \
						NV_ERR_CHECK_RETURN(read(val)); \
						handle.setParam##type_name(val); \
						break; \
					}
#					include "nvparameterized/NvParameterized_types.h"

				case TYPE_MAT34:
					{
						float val[12];
						for (int k = 0; k < 12; ++k)
						{
							NV_ERR_CHECK_RETURN(read(val[k]));
						}
						handle.setParamMat34Legacy(val);
						break;
					}
				NV_PARAMETRIZED_NO_MATH_DATATYPE_LABELS
				default:
					DEBUG_ASSERT( 0 && "Unexpected type" );
					return Serializer::ERROR_UNKNOWN;
				}
			}

			handle.popIndex();
		} //j

		handle.popIndex();
		closeStruct();
	} //i

	return Serializer::ERROR_NONE;
}

void PlatformInputStream::beginStruct(uint32_t align_, uint32_t pad_)
{
	mStack.pushBack(Agregate(Agregate::STRUCT, pad_));
	align(align_); // Align _after_ we push array because otherwise align() would be ignored
}

void PlatformInputStream::beginStruct(uint32_t align_)
{
	beginStruct(align_, align_);
}

void PlatformInputStream::beginStruct(const Definition *pd)
{
	beginStruct(getTargetAlignment(pd), getTargetPadding(pd));
}

void PlatformInputStream::closeStruct()
{
	PX_ASSERT(mStack.size() > 0);

	align(mStack.back().align); //Tail padding
	mStack.popBack();
}

void PlatformInputStream::beginString()
{
	beginStruct(physx::PxMax(mTargetParams.aligns.pointer, mTargetParams.aligns.Bool));
}

void PlatformInputStream::closeString()
{
	closeStruct();
}

void PlatformInputStream::beginArray(const Definition *pd)
{
	uint32_t align_ = getTargetAlignment(pd);
	align(align_);  // Align _before_ we push array because otherwise align() would be ignored
	mStack.pushBack(Agregate(Agregate::ARRAY, align_));
}

void PlatformInputStream::closeArray()
{
	// No padding in arrays
	mStack.popBack();
}

void PlatformInputStream::align(uint32_t border)
{
	mStream.seekRead(mStartPos + getAlign(getPos(), border));
}

Serializer::ErrorType PlatformInputStream::readPtr(uint32_t &val)
{
	//Do not align on uint32_t or uint64_t boundary (already aligned at pointer boundary)
	align(mTargetParams.aligns.pointer);
	if( 4 == mTargetParams.sizes.pointer )
	{
		return read(val, false);
	}
	else
	{
		PX_ASSERT( 8 == mTargetParams.sizes.pointer );

		uint64_t val64;
		NV_ERR_CHECK_RETURN( read(val64, false) );

		val = (uint32_t)val64;
		PX_ASSERT( val == val64 );

		return Serializer::ERROR_NONE;
	}
}
