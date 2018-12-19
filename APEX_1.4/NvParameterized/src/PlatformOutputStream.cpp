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

#include "PlatformOutputStream.h"
#include <stdint.h>

using namespace NvParameterized;

#ifndef WITHOUT_APEX_SERIALIZATION

Reloc::Reloc(RelocType type_, uint32_t ptrPos_, const PlatformOutputStream &parent)
	: type(type_),
	ptrPos(ptrPos_),
	traits(parent.mTraits)
{
	ptrData = reinterpret_cast<PlatformOutputStream *>(traits->alloc(sizeof(PlatformOutputStream)));
	PX_PLACEMENT_NEW(ptrData, PlatformOutputStream)(parent.mTargetParams, parent.mTraits, parent.dict);
}

Reloc::Reloc(const Reloc &cinfo): type(cinfo.type), ptrPos(cinfo.ptrPos), traits(cinfo.traits)
{
	Reloc &info = (Reloc &)cinfo;

	//Take ownership of stream to avoid slow recursive copies (especially when reallocating array elements)
	ptrData = info.ptrData;
	info.ptrData = 0;
}

Reloc::~Reloc()
{
	if( ptrData )
	{
		ptrData->~PlatformOutputStream();
		traits->free(ptrData);
	}
}

PlatformOutputStream::PlatformOutputStream(const PlatformABI &targetParams, Traits *traits, Dictionary &dict_)
	: PlatformStream(targetParams, traits),
	data(traits),
	mRelocs(Traits::Allocator(traits)),
	mStrings(Traits::Allocator(traits)),
	mMerges(Traits::Allocator(traits)),
	dict(dict_),
	mTotalAlign(1)
{}

PlatformOutputStream::PlatformOutputStream(const PlatformOutputStream &s)
	: PlatformStream(s),
	data(s.data),
	mRelocs(Traits::Allocator(s.mTraits)),
	mStrings(Traits::Allocator(s.mTraits)),
	mMerges(Traits::Allocator(s.mTraits)),
	dict(s.dict),
	mTotalAlign(1)
{
	mRelocs.reserve(s.mRelocs.size());
	for(uint32_t i = 0; i < s.mRelocs.size(); ++i)
		mRelocs.pushBack(s.mRelocs[i]);

	mStrings.reserve(s.mStrings.size());
	for(uint32_t i = 0; i < s.mStrings.size(); ++i)
		mStrings.pushBack(s.mStrings[i]);

	mMerges.reserve(s.mMerges.size());
	for(uint32_t i = 0; i < s.mMerges.size(); ++i)
		mMerges.pushBack(s.mMerges[i]);
}

#ifndef NDEBUG
void PlatformOutputStream::dump() const
{
	PlatformStream::dump();

	dumpBytes(data, size());

	fflush(stdout);
	for(uint32_t i = 0; i < mRelocs.size(); ++i)
	{
		printf("Relocation %d at %x:\n", (int)i, mRelocs[i].ptrPos);
		mRelocs[i].ptrData->dump();
	}

	fflush(stdout);
}
#endif

void PlatformOutputStream::storeU32At(uint32_t x, uint32_t i)
{
	if( mCurParams.endian != mTargetParams.endian )
		SwapBytes(reinterpret_cast<char *>(&x), 4U, TYPE_U32);

	*reinterpret_cast<uint32_t *>(&data[i]) = x;
}

uint32_t PlatformOutputStream::storeString(const char *s)
{
	uint32_t off = storeSimple((uint8_t)*s);
	while( *s++ )
		storeSimple((uint8_t)*s);

	return off;
}

uint32_t PlatformOutputStream::storeBytes(const char *s, uint32_t n)
{
	if( !n )
		return size();

	uint32_t off = storeSimple((uint8_t)s[0]);
	for(uint32_t i = 1; i < n; ++i)
		storeSimple((uint8_t)s[i]);

	return off;
}

uint32_t PlatformOutputStream::beginStruct(uint32_t align_, uint32_t pad_)
{
	uint32_t off = size();
	mStack.pushBack(Agregate(Agregate::STRUCT, pad_));
	align(align_); // Align _after_ we push struct to avoid ignored align() when inside array
	return off;
}

uint32_t PlatformOutputStream::beginStruct(uint32_t align_)
{
	return beginStruct(align_, align_);
}

uint32_t PlatformOutputStream::beginStruct(const Definition *pd)
{
	return beginStruct(getTargetAlignment(pd), getTargetPadding(pd));
}

void PlatformOutputStream::closeStruct()
{
	PX_ASSERT(mStack.size() > 0);

	//Tail padding
	align(mStack.back().align);// Align _before_ we pop struct to avoid ignored align() when inside array
	mStack.popBack();
}

uint32_t PlatformOutputStream::beginString()
{
	return beginStruct(physx::PxMax(mTargetParams.aligns.pointer, mTargetParams.aligns.Bool));
}

void PlatformOutputStream::closeString()
{
	closeStruct();
}

uint32_t PlatformOutputStream::beginArray(const Definition *pd)
{
	return beginArray(getTargetAlignment(pd));
}

uint32_t PlatformOutputStream::beginArray(uint32_t align_)
{
	align(align_); // Align _before_ we push array because otherwise align() would be ignored
	uint32_t off = size();
	mStack.pushBack(Agregate(Agregate::ARRAY, align_));
	return off;
}

void PlatformOutputStream::closeArray()
{
	// No tail padding when in array
	mStack.popBack();
}

void PlatformOutputStream::skipBytes(uint32_t nbytes)
{
	data.skipBytes(nbytes);
}

void PlatformOutputStream::align(uint32_t border)
{
	bool isAligned;
	uint32_t newSize = getAlign(size(), border, isAligned);

	if( isAligned )
		mTotalAlign = physx::PxMax(mTotalAlign, border);

	data.skipBytes(newSize - size());
}

void PlatformOutputStream::mergeDict()
{
	for(uint32_t i = 0; i < dict.size(); ++i)
	{
		const char *s = dict.get(i);
		uint32_t off = storeString(s);
		dict.setOffset(s, off);
	}
}

uint32_t PlatformOutputStream::storeNullPtr()
{
	//Do not align on uint32_t or uint64_t boundary (already aligned at pointer boundary)
	align(mTargetParams.aligns.pointer);
	uint32_t off = size();
	if( 4 == mTargetParams.sizes.pointer )
		data.skipBytes(4);
	else
	{
		PX_ASSERT( 8 == mTargetParams.sizes.pointer );
		data.skipBytes(8);
	}
	return off;
}

Reloc &PlatformOutputStream::storePtr(RelocType type, uint32_t align)
{
	uint32_t off = storeNullPtr();
	mRelocs.pushBack(Reloc(type, off, *this));
	mRelocs.back().ptrData->setAlignment(align);
	return mRelocs.back();
}

Reloc &PlatformOutputStream::storePtr(RelocType type, const Definition *pd)
{
	return storePtr(type, getTargetAlignment(pd));
}

void PlatformOutputStream::storeStringPtr(const char *s)
{
	uint32_t off = storeNullPtr();
	if( s )
	{
		mStrings.pushBack(StringReloc(off, s));
		dict.put(s);
	}
}

uint32_t PlatformOutputStream::storeSimpleStructArray(Handle &handle)
{
	int32_t n;
	handle.getArraySize(n);

	const NvParameterized::Definition *pdStruct = handle.parameterDefinition()->child(0);
	int32_t nfields = pdStruct->numChildren();

	uint32_t align_ = getTargetAlignment(pdStruct),
		size_ = getTargetSize(pdStruct),
		pad_ = getTargetPadding(pdStruct);

	align(align_);
	uint32_t off = size();

	data.reserve(size() + n * physx::PxMax(align_, size_));

	char *p = data;
	p += data.size();

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
				skipBytes(getTargetSize(pdField));
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
						handle.getParam##type_name(val); \
						storeSimple<c_type>(val); \
						break; \
					}
#					include "nvparameterized/NvParameterized_types.h"

				case TYPE_MAT34:
				{
					float val[12];
					handle.getParamMat34Legacy(val);
					storeSimple(val, 12);
					break;
				}

				NV_PARAMETRIZED_NO_MATH_DATATYPE_LABELS 
				default:
					DEBUG_ASSERT( 0 && "Unexpected type" );
					return UINT32_MAX;
				}
			}

			handle.popIndex();
		} //j

		handle.popIndex();
		closeStruct();
	} //i

	return off;
}

uint32_t PlatformOutputStream::storeObjHeader(const NvParameterized::Interface &obj, bool isIncluded)
{
	uint32_t align_ = NvMax3(mTargetParams.aligns.Bool, mTargetParams.aligns.i32, mTargetParams.aligns.pointer);

	uint32_t off = beginStruct(align_);

	uint32_t hdrOff = data.size();
	PX_ASSERT( hdrOff % sizeof(uint32_t) == 0 );

	storeSimple(uint32_t(0)); //Data offset

	//className
	storeStringPtr(obj.className());

	//name
	storeStringPtr(obj.name());

	//isIncluded
	storeSimple(isIncluded);

	//version
	storeSimple<uint32_t>(obj.version());

	//checksum size
	uint32_t bits = (uint32_t)-1;
	const uint32_t *checksum = obj.checksum(bits);
	PX_ASSERT( bits % 32 == 0 ); //32 bits in uint32_t
	uint32_t i32s = bits / 32;
	storeSimple(i32s);

	//checksum pointer
	Reloc &reloc = storePtr(RELOC_ABS_RAW, mTargetParams.aligns.i32);
	for(uint32_t i = 0; i < i32s; ++i)
		reloc.ptrData->storeSimple(checksum[i]);

	closeStruct();

	//We force alignment to calculate dataOffset
	//(when object is inserted no additional padding will be inserted)

	const Definition *pd = obj.rootParameterDefinition();
	uint32_t customAlign = pd ? getTargetAlignment(pd) : 1;
	align(physx::PxMax(16U, customAlign)); //16 for safety

	storeU32At(data.size() - hdrOff, hdrOff); //Now we know object data offset

	return off;
}

uint32_t PlatformOutputStream::beginObject(const NvParameterized::Interface &obj, bool /*isRoot*/, const Definition *pd)
{
	//NvParameterized objects is derived from NvParameters so we need to store its fields as well.

	//WARN: this implementation _heavily_ depends on implementation of NvParameters

	//Alignment of NvParameters
	uint32_t parentAlign = physx::PxMax(mTargetParams.aligns.pointer, mTargetParams.aligns.Bool),
		childAlign = pd ? getTargetAlignment(pd) : 1,
		totalAlign = physx::PxMax(parentAlign, childAlign);

	uint32_t off = beginStruct(totalAlign);

	//NvParameters fields
	for(uint32_t i = 0; i < 6; storeNullPtr(), ++i); //vtable and other fields
	storeStringPtr(obj.name()); //mName
	storeStringPtr(obj.className()); //mClassName
	storeSimple(true); //mDoDeallocateSelf (all objects are responsible for memory deallocation)
	storeSimple(false); //mDoDeallocateName
	storeSimple(false); //mDoDeallocateClassName

	//Some general theory of alignment handling
	//Imagine that we have class:
	//class A: A1, A2, ... An
	//{
	//	T1 f1;
	//	T2 f2;
	//};
	//Then all Ai/fi are aligned on natural boundary. Whether or not padding bytes for Ai are inserted
	//and whether or not Ai+1 or fi may reuse those bytes depends on compiler;
	//we store this info in PlatformABI's doReusePadding flag.
	//doReusePadding == false means that padding bytes are always there.
	//doReusePadding == true means that padding bytes are _not_ inserted for non-POD Ai but are inserted for POD Ai.
	//(I have yet to see compiler that does not insert padding bytes for POD base class!).

	//Compilers may handle derived classes in two different ways.
	//Say we have
	//	class B {int x; char y; };
	//	class A: public B {char z; };
	//Then in pure C code this may look like either as
	//	struct A { struct { int x; char y; } b; char z; };
	//or as
	//	struct A { int x; char y; char z; };
	//(the latter is usual if B is not POD).
	//Take care of that here (NvParameters is not POD!).
	if( !mTargetParams.doReuseParentPadding )
		align(parentAlign); //Insert tail padding for NvParameters

	//ParametersStruct is aligned on natural boundary
	align(childAlign);

	return off;
}

uint32_t PlatformOutputStream::merge(const PlatformOutputStream &mergee)
{
	//All structs should be closed
	PX_ASSERT( !mergee.mStack.size() );

	align(mergee.alignment());

	uint32_t base = data.size();

	data.appendBytes(mergee.data, mergee.data.size());

	//Update relocations

	mRelocs.reserve(mRelocs.size() + mergee.mRelocs.size());
	for(uint32_t i = 0; i < mergee.mRelocs.size(); ++i)
	{
		mRelocs.pushBack(mergee.mRelocs[i]);
		mRelocs.back().ptrPos += base;
	}

	mStrings.reserve(mStrings.size() + mergee.mStrings.size());
	for(uint32_t i = 0; i < mergee.mStrings.size(); ++i)
	{
		mStrings.pushBack(mergee.mStrings[i]);
		mStrings.back().ptrPos += base;
	}

	mMerges.reserve(mMerges.size() + mergee.mMerges.size());
	for(uint32_t i = 0; i < mergee.mMerges.size(); ++i)
	{
		mMerges.pushBack(mergee.mMerges[i]);

		mMerges.back().ptrPos += base;
		if( !mergee.mMerges[i].isExtern )
			mMerges.back().targetPos += base;
	}

	return base;
}

void PlatformOutputStream::flatten()
{
	//It's very important that data for child objects is stored
	//after current object to allow safe initialization

	//Generic pointers
	for(uint32_t i = 0; i < mRelocs.size(); ++i)
	{
		Reloc &reloc = mRelocs[i];

		//Recursively add data which is pointed-to

		align( reloc.ptrData->alignment() );

		MergedReloc m = { reloc.ptrPos, data.size(), reloc.type, false };
		mMerges.pushBack(m);

		merge(*reloc.ptrData); //Internal pointers are recursively added here
	}

	//String pointers
	for(uint32_t i = 0; i < mStrings.size(); ++i)
	{
		//String pointers are external and absolute
		MergedReloc m = { mStrings[i].ptrPos, dict.getOffset(mStrings[i].s), RELOC_ABS_RAW, true };
		mMerges.pushBack(m);
	}

	mRelocs.clear();
	mStrings.clear();
}

uint32_t PlatformOutputStream::writeRelocs()
{
	uint32_t ptrOff = storeSimple<uint32_t>(mMerges.size()); //Offset of relocation table

	data.reserve(mMerges.size() * 2 * physx::PxMax(4U, mTargetParams.aligns.i32));

	for(uint32_t i = 0; i < mMerges.size(); ++i)
	{
		char *ptr = &data[mMerges[i].ptrPos];
		if( 4 == mTargetParams.sizes.pointer )
		{
			uint32_t *ptrAsInt = reinterpret_cast<uint32_t *>(ptr);
			*ptrAsInt = mMerges[i].targetPos;
			if( mTargetParams.endian != mCurParams.endian )
				SwapBytes(ptr, 4U, TYPE_U32);
		}
		else
		{
			PX_ASSERT(8 == mTargetParams.sizes.pointer);

			uint64_t *ptrAsInt = reinterpret_cast<uint64_t *>(ptr);
			*ptrAsInt = mMerges[i].targetPos;
			if( mTargetParams.endian != mCurParams.endian )
				SwapBytes(ptr, 8U, TYPE_U64);
		}

		//BinaryReloc struct
		beginStruct(mTargetParams.aligns.i32);
		storeSimple<uint32_t>(static_cast<uint32_t>(mMerges[i].type));
		storeSimple<uint32_t>(mMerges[i].ptrPos);
		closeStruct();
	}

	mMerges.clear();

	return ptrOff;
}

#endif
