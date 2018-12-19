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



#ifndef CLOTHING_GLOBALS_H
#define CLOTHING_GLOBALS_H

#include "ApexUsingNamespace.h"

namespace nvidia
{
namespace clothing
{


struct ClothingConstants
{
	enum Enum
	{
		ImmediateClothingInSkinFlag =		0x80000000, // only highest (sign?) bit. The rest is the index in the clothSkinMapB
		ImmediateClothingInvertNormal =		0x40000000, // if second highest bit is set, invert the normal from Cloth
		ImmediateClothingBadNormal =		0x20000000, // the normal is neither correct nor inverted, just different, use mesh-mesh skinning from neighboring triangles
		ImmediateClothingInvalidValue =		0x1fffffff, // the lowest bit is set, all others are maxed out
		ImmediateClothingReadMask =			0x0fffffff, // read mask, use this to read the number (two flags can still be put there so far)
	};
};

}
} // namespace nvidia


#endif // CLOTHING_GLOBALS_H
