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

#include <ctype.h>
#include <stdio.h>

#include "BinaryHelper.h"

namespace NvParameterized
{

void dumpBytes(const char *data, uint32_t nbytes)
{
	printf("Total = %d bytes\n", nbytes);

	for(uint32_t i = 0; i < nbytes; i += 16)
	{
		printf("%08x: ", i);

		//Print bytes
		for(uint32_t j = i; j < i + 16; ++j)
		{
			if( nbytes < j )
			{
				//Pad with whites
				for(; j < i + 16; ++j)
					printf("   ");

				break;
			}

			unsigned char c = static_cast<unsigned char>(data[j]);
			printf("%02x ", c);
		}

		//Print chars
		for(uint32_t j = i; j < i + 16; ++j)
		{
			if( nbytes < j )
				break;

			unsigned char c = static_cast<unsigned char>(data[j]);
			printf("%c", isprint(c) ? c : '.');
		}

		printf("\n");
	}
}

void Dictionary::setOffset(const char *s, uint32_t off)
{
	for(uint32_t i = 0; i < entries.size(); ++i)
		if( 0 == strcmp(s, entries[i].s) )
		{
			entries[i].offset = off;
			return;
		}

	PX_ASSERT(0 && "String not found");
}

uint32_t Dictionary::getOffset(const char *s) const
{
	for(uint32_t i = 0; i < entries.size(); ++i)
		if( 0 == strcmp(s, entries[i].s) )
			return entries[i].offset;

	PX_ASSERT(0 && "String not found");
	return (uint32_t)-1;
}

void Dictionary::serialize(StringBuf &res) const
{
	res.append(Canonize(entries.size()));

	for(uint32_t i = 0; i < entries.size(); ++i)
	{
		const char *s = entries[i].s;
		res.appendBytes(s, 1 + (uint32_t)strlen(s));
	}
}

uint32_t Dictionary::put(const char *s)
{
	PX_ASSERT(s && "NULL in dictionary");

	for(uint32_t i = 0; i < entries.size(); ++i)
		if( 0 == strcmp(s, entries[i].s) )
			return i;

	Entry e = {s, 0};
	entries.pushBack(e);
	return entries.size() - 1;
}

}