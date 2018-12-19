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



#ifndef CONVEX_HULL_METHOD_H
#define CONVEX_HULL_METHOD_H

/*!
\file
\brief Misc utility classes
*/

#include "Module.h"
#include "foundation/PxMath.h"

namespace nvidia
{
namespace apex
{

PX_PUSH_PACK_DEFAULT


/**
\brief Method by which chunk mesh collision hulls are generated.
*/
struct ConvexHullMethod
{
	/**
	\brief Enum of methods by which chunk mesh collision hulls are generated.
	*/
	enum Enum
	{
		USE_6_DOP,
		USE_10_DOP_X,
		USE_10_DOP_Y,
		USE_10_DOP_Z,
		USE_14_DOP_XY,
		USE_14_DOP_YZ,
		USE_14_DOP_ZX,
		USE_18_DOP,
		USE_26_DOP,
		WRAP_GRAPHICS_MESH,
		CONVEX_DECOMPOSITION,

		COUNT
	};
};



PX_POP_PACK

}
} // end namespace nvidia::apex

#endif // CONVEX_HULL_METHOD_H
