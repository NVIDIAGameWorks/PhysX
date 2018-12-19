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


#ifndef PS_COMM_LAYER_H

#define PS_COMM_LAYER_H

// Simple class to create a client/server connection and pass messages back and forth over TCP/IP
// Supports compression

#include <stdint.h>

#define COMM_LAYER_DEFAULT_HOST "localhost"
#define COMM_LAYER_DEFAULT_PORT 5525

class CommLayer
{
public:

	virtual bool isServer(void) const = 0; // return true if we are in server mode.
	virtual bool hasClient(void) const = 0;	// return true if a client connection is currently established
	virtual bool isConnected(void) const = 0; // return true if we are still connected to the server.  The server is always in a 'connected' state.
	virtual int32_t getPendingReadSize(void) const = 0; // returns the number of bytes of data which is pending to be read.
	virtual int32_t getPendingSendSize(void) const = 0; // return the number of bytes of data pending to be sent.  This can be used for flow control

	virtual bool sendMessage(const void *msg,uint32_t len) = 0;

	virtual uint32_t peekMessage(bool &isBigEndianPacket) = 0; // return the length of the next pending message

	virtual uint32_t getMessage(void *msg,uint32_t maxLength,bool &isBigEndianPacket) = 0; // receives a pending message

	virtual void release(void) = 0;

protected:
	virtual ~CommLayer(void)
	{
	}
};

CommLayer *createCommLayer(const char *ipaddress,
						  uint16_t portNumber,
						  bool isServer);

#endif
