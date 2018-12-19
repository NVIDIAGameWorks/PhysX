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


#ifndef __APEX_STREAM_H__
#define __APEX_STREAM_H__

#include "ApexDefs.h"
#include "PxPlane.h"


namespace nvidia
{
namespace apex
{

// Public, useful operators for serializing nonversioned data follow.
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, bool& b)
{
	b = (0 != stream.readByte());
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, int8_t& b)
{
	b = (int8_t)stream.readByte();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, int16_t& w)
{
	w = (int16_t)stream.readWord();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, int32_t& d)
{
	d = (int32_t)stream.readDword();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, uint8_t& b)
{
	b = stream.readByte();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, uint16_t& w)
{
	w = stream.readWord();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, uint32_t& d)
{
	d = stream.readDword();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, float& f)
{
	f = stream.readFloat();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, double& f)
{
	f = stream.readDouble();
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec2& v)
{
	stream >> v.x >> v.y;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec3& v)
{
	stream >> v.x >> v.y >> v.z;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxVec4& v)
{
	stream >> v.x >> v.y >> v.z >> v.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxBounds3& b)
{
	stream >> b.minimum >> b.maximum;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxQuat& q)
{
	stream >> q.x >> q.y >> q.z >> q.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxPlane& p)
{
	stream >> p.n.x >> p.n.y >> p.n.z >> p.d;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator >> (physx::PxFileBuf& stream, physx::PxMat44& m)
{
	stream >> m.column0 >> m.column1 >> m.column2 >> m.column3;
	return stream;
}

// The opposite of the above operators--takes data and writes it to a stream.
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const bool b)
{
	stream.storeByte(b ? (uint8_t)1 : (uint8_t)0);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const int8_t b)
{
	stream.storeByte((uint8_t)b);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const int16_t w)
{
	stream.storeWord((uint16_t)w);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const int32_t d)
{
	stream.storeDword((uint32_t)d);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const uint8_t b)
{
	stream.storeByte(b);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const uint16_t w)
{
	stream.storeWord(w);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const uint32_t d)
{
	stream.storeDword(d);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const float f)
{
	stream.storeFloat(f);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const double f)
{
	stream.storeDouble(f);
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec2& v)
{
	stream << v.x << v.y;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec3& v)
{
	stream << v.x << v.y << v.z;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxVec4& v)
{
	stream << v.x << v.y << v.z << v.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxBounds3& b)
{
	stream << b.minimum << b.maximum;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxQuat& q)
{
	stream << q.x << q.y << q.z << q.w;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxPlane& p)
{
	stream << p.n.x << p.n.y << p.n.z << p.d;
	return stream;
}
PX_INLINE physx::PxFileBuf& operator << (physx::PxFileBuf& stream, const physx::PxMat44& m)
{
	stream << m.column0 << m.column1 << m.column2 << m.column3;
	return stream;
}


}
} // end namespace apex

#endif // __APEX_STREAM_H__