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
// Copyright (c) 2009-2018 NVIDIA Corporation. All rights reserved.


#ifndef STREAM_IO_H
#define STREAM_IO_H

/*!
\file
\brief NsIOStream class
*/
#include "Ps.h"
#include "PsString.h"
#include "PxFileBuf.h"
#include <string.h>
#include <stdlib.h>

#define safePrintf nvidia::string::sprintf_s

PX_PUSH_PACK_DEFAULT

namespace nvidia
{

/**
\brief A wrapper class for physx::PxFileBuf that provides both binary and ASCII streaming capabilities
*/
class StreamIO
{
	static const uint32_t MAX_STREAM_STRING = 1024;
public:
	/**
	\param [in] stream the physx::PxFileBuf through which all reads and writes will be performed
	\param [in] streamLen the length of the input data stream when de-serializing
	*/
	StreamIO(physx::PxFileBuf &stream,uint32_t streamLen) : mStreamLen(streamLen), mStream(stream) { }
	~StreamIO(void) { }

	PX_INLINE StreamIO& operator<<(bool v);
	PX_INLINE StreamIO& operator<<(char c);
	PX_INLINE StreamIO& operator<<(uint8_t v);
	PX_INLINE StreamIO& operator<<(int8_t v);

	PX_INLINE StreamIO& operator<<(const char *c);
	PX_INLINE StreamIO& operator<<(int64_t v);
	PX_INLINE StreamIO& operator<<(uint64_t v);
	PX_INLINE StreamIO& operator<<(double v);
	PX_INLINE StreamIO& operator<<(float v);
	PX_INLINE StreamIO& operator<<(uint32_t v);
	PX_INLINE StreamIO& operator<<(int32_t v);
	PX_INLINE StreamIO& operator<<(uint16_t v);
	PX_INLINE StreamIO& operator<<(int16_t v);
	PX_INLINE StreamIO& operator<<(const physx::PxVec3 &v);
	PX_INLINE StreamIO& operator<<(const physx::PxQuat &v);
	PX_INLINE StreamIO& operator<<(const physx::PxBounds3 &v);

	PX_INLINE StreamIO& operator>>(const char *&c);
	PX_INLINE StreamIO& operator>>(bool &v);
	PX_INLINE StreamIO& operator>>(char &c);
	PX_INLINE StreamIO& operator>>(uint8_t &v);
	PX_INLINE StreamIO& operator>>(int8_t &v);
	PX_INLINE StreamIO& operator>>(int64_t &v);
	PX_INLINE StreamIO& operator>>(uint64_t &v);
	PX_INLINE StreamIO& operator>>(double &v);
	PX_INLINE StreamIO& operator>>(float &v);
	PX_INLINE StreamIO& operator>>(uint32_t &v);
	PX_INLINE StreamIO& operator>>(int32_t &v);
	PX_INLINE StreamIO& operator>>(uint16_t &v);
	PX_INLINE StreamIO& operator>>(int16_t &v);
	PX_INLINE StreamIO& operator>>(physx::PxVec3 &v);
	PX_INLINE StreamIO& operator>>(physx::PxQuat &v);
	PX_INLINE StreamIO& operator>>(physx::PxBounds3 &v);

	uint32_t getStreamLen(void) const { return mStreamLen; }

	physx::PxFileBuf& getStream(void) { return mStream; }

	PX_INLINE void storeString(const char *c,bool zeroTerminate=false);

private:
	StreamIO& operator=( const StreamIO& );


	uint32_t			mStreamLen; // the length of the input data stream when de-serializing.
	physx::PxFileBuf	&mStream;
	char				mReadString[MAX_STREAM_STRING]; // a temp buffer for streaming strings on input.
};

#include "StreamIO.inl" // inline methods...

} // end of nvidia namespace

PX_POP_PACK

#endif // STREAM_IO_H
