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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.

// WARNING: before doing any changes to this file
// check comments at the head of BinSerializer.cpp

PX_INLINE uint32_t PlatformABI::align(uint32_t len, uint32_t border)
{
	uint32_t n = (len + border - 1) / border;
	return n * border;
}

PX_INLINE bool PlatformABI::isNormal() const
{
	return 1 == sizes.Char && 1 == sizes.Bool				//Wide (> 1) bytes not supported
		&& 4 == sizes.real									//Some code relies on short float (float)
		&& ( 4 == sizes.pointer || 8 == sizes.pointer );	//Some code relies on pointers being either 32- or 64-bit
}

PX_INLINE uint32_t PlatformABI::getMetaEntryAlignment() const
{
	return physx::PxMax(aligns.i32, aligns.pointer);
}

PX_INLINE uint32_t PlatformABI::getMetaInfoAlignment() const
{
	return NvMax3(getHintAlignment(), aligns.i32, aligns.pointer);
}

PX_INLINE uint32_t PlatformABI::getHintAlignment() const
{
	return physx::PxMax(aligns.i32, getHintValueAlignment());
}

PX_INLINE uint32_t PlatformABI::getHintValueAlignment() const
{
	return NvMax3(aligns.pointer, aligns.i64, aligns.f64);
}

PX_INLINE uint32_t PlatformABI::getHintValueSize() const
{
	return align(8, getHintValueAlignment()); //Size of union = aligned size of maximum element
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<bool>() const
{
	return aligns.Bool;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<int8_t>() const
{
	return aligns.i8;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<int16_t>() const
{
	return aligns.i16;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<int32_t>() const
{
	return aligns.i32;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<int64_t>() const
{
	return aligns.i64;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<uint8_t>() const
{
	return getAlignment<int8_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<uint16_t>() const
{
	return getAlignment<int16_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<uint32_t>() const
{
	return getAlignment<int32_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<uint64_t>() const
{
	return getAlignment<int64_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<float>() const
{
	return aligns.f32;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<double>() const
{
	return aligns.f64;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxVec2>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxVec3>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxVec4>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxQuat>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxMat33>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxMat44>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxBounds3>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<void *>() const
{
	return aligns.pointer;
}

template <> PX_INLINE uint32_t PlatformABI::getAlignment<physx::PxTransform>() const
{
	return aligns.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<bool>() const
{
	return sizes.Bool;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<int8_t>() const
{
	return 1;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<int16_t>() const
{
	return 2;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<int32_t>() const
{
	return 4;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<int64_t>() const
{
	return 8;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<uint8_t>() const
{
	return getSize<int8_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getSize<uint16_t>() const
{
	return getSize<int16_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getSize<uint32_t>() const
{
	return getSize<int32_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getSize<uint64_t>() const
{
	return getSize<int64_t>();
}

template <> PX_INLINE uint32_t PlatformABI::getSize<float>() const
{
	return 4;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<double>() const
{
	return 8;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxVec2>() const
{
	return 2 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxVec3>() const
{
	return 3 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxVec4>() const
{
	return 4 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxQuat>() const
{
	return 4 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxMat33>() const
{
	return 9 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxMat44>() const
{
	return 16 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxBounds3>() const
{
	return 6 * sizes.real;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<void *>() const
{
	return sizes.pointer;
}

template <> PX_INLINE uint32_t PlatformABI::getSize<physx::PxTransform>() const
{
	return 7 * sizes.real;
}
