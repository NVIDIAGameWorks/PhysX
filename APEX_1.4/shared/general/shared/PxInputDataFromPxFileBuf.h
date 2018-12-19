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

#ifndef PX_INPUT_DATA_FROM_PX_FILE_BUF_H

#define PX_INPUT_DATA_FROM_PX_FILE_BUF_H

#include "PxFileBuf.h"
#include "PxIO.h"

namespace physx
{
	class PxInputDataFromPxFileBuf: public physx::PxInputData
	{
	public:
		PxInputDataFromPxFileBuf(physx::PxFileBuf& fileBuf) : mFileBuf(fileBuf) {}

		// physx::PxInputData interface
		virtual uint32_t	getLength() const
		{
			return mFileBuf.getFileLength();
		}

		virtual void	seek(uint32_t offset)
		{
			mFileBuf.seekRead(offset);
		}

		virtual uint32_t	tell() const
		{
			return mFileBuf.tellRead();
		}

		// physx::PxInputStream interface
		virtual uint32_t read(void* dest, uint32_t count)
		{
			return mFileBuf.read(dest, count);
		}

		PX_NOCOPY(PxInputDataFromPxFileBuf)
	private:
		physx::PxFileBuf& mFileBuf;
	};
};

#endif
