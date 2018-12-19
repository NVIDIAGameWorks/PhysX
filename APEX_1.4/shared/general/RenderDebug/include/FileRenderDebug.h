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


#ifndef FILE_RENDER_DEBUG_H
#define FILE_RENDER_DEBUG_H

#include "ProcessRenderDebug.h"

class ClientServer;

namespace RENDER_DEBUG
{

class FileRenderDebug : public ProcessRenderDebug
{
public:
	virtual ProcessRenderDebug * getEchoLocal(void) const = 0;
	virtual uint32_t getFrameCount(void) const = 0;
	virtual void setFrame(uint32_t frameNo) = 0;
	virtual void processReadFrame(RENDER_DEBUG::RenderDebugInterface *iface) = 0;
protected:
	virtual ~FileRenderDebug(void) { }
};


FileRenderDebug * createFileRenderDebug(const char *fileName,
										  bool readAccess,
										  ProcessRenderDebug *echoLocally,
										  ClientServer *clientServer);


}

#endif
