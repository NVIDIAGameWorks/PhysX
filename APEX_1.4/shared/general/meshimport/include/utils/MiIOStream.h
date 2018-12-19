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



#ifndef MI_IOSTREAM_H
#define MI_IOSTREAM_H

/*!
\file
\brief MiIOStream class
*/
#include "MiFileBuf.h"
#include <string.h>
#include <stdlib.h>
#include "MiAsciiConversion.h"

#define safePrintf string::sprintf_s

MI_PUSH_PACK_DEFAULT

namespace mimp
{

	/**
\brief A wrapper class for MiFileBuf that provides both binary and ASCII streaming capabilities
*/
class MiIOStream
{
	static const MiU32 MAX_STREAM_STRING = 1024;
public:
	/**
	\param [in] stream the MiFileBuf through which all reads and writes will be performed
	\param [in] streamLen the length of the input data stream when de-serializing
	*/
	MiIOStream(MiFileBuf &stream,MiU32 streamLen) : mBinary(true), mStreamLen(streamLen), mStream(stream) { };
	~MiIOStream(void) { };

	/**
	\brief Set the stream to binary or ASCII

	\param [in] state if true, stream is binary, if false, stream is ASCII

	If the stream is binary, stream access is passed straight through to the respecitve 
	MiFileBuf methods.  If the stream is ASCII, all stream reads and writes are converted to
	human readable ASCII.
	*/
	MI_INLINE void setBinary(bool state) { mBinary = state; }
	MI_INLINE bool getBinary() { return mBinary; }

	MI_INLINE MiIOStream& operator<<(bool v);
	MI_INLINE MiIOStream& operator<<(char c);
	MI_INLINE MiIOStream& operator<<(MiU8 v);
	MI_INLINE MiIOStream& operator<<(MiI8 v);

	MI_INLINE MiIOStream& operator<<(const char *c);
	MI_INLINE MiIOStream& operator<<(MiI64 v);
	MI_INLINE MiIOStream& operator<<(MiU64 v);
	MI_INLINE MiIOStream& operator<<(MiF64 v);
	MI_INLINE MiIOStream& operator<<(MiF32 v);
	MI_INLINE MiIOStream& operator<<(MiU32 v);
	MI_INLINE MiIOStream& operator<<(MiI32 v);
	MI_INLINE MiIOStream& operator<<(MiU16 v);
	MI_INLINE MiIOStream& operator<<(MiI16 v);

	MI_INLINE MiIOStream& operator>>(const char *&c);
	MI_INLINE MiIOStream& operator>>(bool &v);
	MI_INLINE MiIOStream& operator>>(char &c);
	MI_INLINE MiIOStream& operator>>(MiU8 &v);
	MI_INLINE MiIOStream& operator>>(MiI8 &v);
	MI_INLINE MiIOStream& operator>>(MiI64 &v);
	MI_INLINE MiIOStream& operator>>(MiU64 &v);
	MI_INLINE MiIOStream& operator>>(MiF64 &v);
	MI_INLINE MiIOStream& operator>>(MiF32 &v);
	MI_INLINE MiIOStream& operator>>(MiU32 &v);
	MI_INLINE MiIOStream& operator>>(MiI32 &v);
	MI_INLINE MiIOStream& operator>>(MiU16 &v);
	MI_INLINE MiIOStream& operator>>(MiI16 &v);

	MiU32 getStreamLen(void) const { return mStreamLen; }

	MiFileBuf& getStream(void) { return mStream; }

	MI_INLINE void storeString(const char *c,bool zeroTerminate=false);

private:
	MiIOStream& operator=( const MiIOStream& );


	bool      mBinary; // true if we are serializing binary data.  Otherwise, everything is assumed converted to ASCII
	MiU32     mStreamLen; // the length of the input data stream when de-serializing.
	MiFileBuf &mStream;
	char			mReadString[MAX_STREAM_STRING]; // a temp buffer for streaming strings on input.
};

#include "MiIOStream.inl" // inline methods...

}; // end of physx namespace

MI_POP_PACK

#endif // MI_IOSTREAM_H
