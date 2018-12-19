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

#ifndef PLATFORM_STREAM_H_
#define PLATFORM_STREAM_H_

#include <stdio.h>

#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvSerializer.h"
#include "NvSerializerInternal.h"
#include "PlatformABI.h"

namespace NvParameterized
{

// Base class for ABI-aware streams

// TODO:
// PlatformInput/OutputStream should probably be visitors

class PlatformStream
{
	void operator =(const PlatformStream &); //Don't

protected:
	Traits *mTraits;

	PlatformABI mTargetParams, mCurParams;

	struct Agregate
	{
		enum Type
		{
			STRUCT,
			ARRAY
		};

		Type type;
		uint32_t align;

		Agregate(Type t, uint32_t a): type(t), align(a) {}
	};

	//Agregate stack holds information about structs and their nesting (for automatic insertion of tail pad)
	physx::shdfnd::Array<Agregate, Traits::Allocator> mStack;

public:
	PlatformStream(const PlatformABI &targetParams, Traits *traits)
		: mTraits(traits),
		mTargetParams(targetParams),
		mStack(Traits::Allocator(traits))
	{
		// TODO: constructors should not fail...
		if( Serializer::ERROR_NONE != PlatformABI::GetPredefinedABI(GetCurrentPlatform(), mCurParams) )
		{
			PX_ALWAYS_ASSERT();
			return;
		}
	}

	//Array's copy constructor is broken so we implement it by hand
	PlatformStream(const PlatformStream &s)
		: mTraits(s.mTraits),
		mTargetParams(s.mTargetParams),
		mCurParams(s.mCurParams),
		mStack(Traits::Allocator(s.mTraits))
	{
		mStack.reserve(s.mStack.size());
		for(uint32_t i = 0; i < s.mStack.size(); ++i)
			mStack.pushBack(s.mStack[i]);
	}

	void dump() const
	{
		printf("Dumping PlatformStream at %p:\n", this);
	}

	const PlatformABI &getTargetABI() const
	{
		return mTargetParams;
	}

	uint32_t getTargetSize(const Definition *pd) const
	{
		return mTargetParams.getSize(pd);
	}

	uint32_t getTargetAlignment(const Definition *pd) const
	{
		return mTargetParams.getAlignment(pd);
	}

	uint32_t getTargetPadding(const Definition *pd) const
	{
		return mTargetParams.getPadding(pd);
	}

	//Align offset to be n*border
	PX_INLINE uint32_t getAlign(uint32_t off, uint32_t border, bool &isAligned) const
	{
		PX_ASSERT(border <= 128);

		//Array members are not aligned
		//See http://www.gamasutra.com/view/feature/3975/data_alignment_part_2_objects_on_.php
		if( !mStack.empty() && Agregate::ARRAY == mStack.back().type )
		{
			isAligned = false;
			return off;
		}

		isAligned = true;
		const uint32_t mask = border - 1;
		return (off + mask) & ~mask;
	}

	//Align offset to be n*border
	PX_INLINE uint32_t getAlign(uint32_t off, uint32_t border) const
	{
		bool tmp;
		return getAlign(off, border, tmp);
	}
};

}

#endif
