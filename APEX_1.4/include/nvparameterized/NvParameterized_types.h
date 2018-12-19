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
 
/*!
\brief NvParameterized type X-macro template
\note See http://en.wikipedia.org/wiki/C_preprocessor#X-Macros for more details
*/


// NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type)

#if PX_VC && !PX_PS4
	#pragma warning(push)
	#pragma warning(disable:4555)
#endif	//!PX_PS4

PX_PUSH_PACK_DEFAULT

#ifndef NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#ifndef NV_PARAMETERIZED_TYPES_ONLY_SCALAR_TYPES
NV_PARAMETERIZED_TYPE(Array, ARRAY, void *)
NV_PARAMETERIZED_TYPE(Struct, STRUCT, void *)
NV_PARAMETERIZED_TYPE(Ref, REF, NvParameterized::Interface *)
#endif
#endif

#ifndef NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#ifndef NV_PARAMETERIZED_TYPES_ONLY_SCALAR_TYPES
NV_PARAMETERIZED_TYPE(String, STRING, const char *)
NV_PARAMETERIZED_TYPE(Enum, ENUM, const char *)
#endif
#endif

NV_PARAMETERIZED_TYPE(Bool, BOOL, bool)

NV_PARAMETERIZED_TYPE(I8,  I8,  int8_t)
NV_PARAMETERIZED_TYPE(I16, I16, int16_t)
NV_PARAMETERIZED_TYPE(I32, I32, int32_t)
NV_PARAMETERIZED_TYPE(I64, I64, int64_t)

NV_PARAMETERIZED_TYPE(U8,  U8,  uint8_t)
NV_PARAMETERIZED_TYPE(U16, U16, uint16_t)
NV_PARAMETERIZED_TYPE(U32, U32, uint32_t)
NV_PARAMETERIZED_TYPE(U64, U64, uint64_t)

NV_PARAMETERIZED_TYPE(F32, F32, float)
NV_PARAMETERIZED_TYPE(F64, F64, double)

#ifndef NV_PARAMETERIZED_TYPES_ONLY_SCALAR_TYPES
NV_PARAMETERIZED_TYPE(Vec2,      VEC2,      physx::PxVec2)
NV_PARAMETERIZED_TYPE(Vec3,      VEC3,      physx::PxVec3)
NV_PARAMETERIZED_TYPE(Vec4,      VEC4,      physx::PxVec4)
NV_PARAMETERIZED_TYPE(Quat,      QUAT,      physx::PxQuat)
NV_PARAMETERIZED_TYPE(Bounds3,   BOUNDS3,   physx::PxBounds3)
NV_PARAMETERIZED_TYPE(Mat33,     MAT33,     physx::PxMat33)
NV_PARAMETERIZED_TYPE(Mat44,     MAT44,     physx::PxMat44)
NV_PARAMETERIZED_TYPE(Transform, TRANSFORM, physx::PxTransform)
#endif


#ifdef NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#   undef NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#endif

#ifdef NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#   undef NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#endif

#ifdef NV_PARAMETERIZED_TYPES_ONLY_SCALAR_TYPES
#   undef NV_PARAMETERIZED_TYPES_ONLY_SCALAR_TYPES
#endif

#ifdef NV_PARAMETERIZED_TYPES_NO_LEGACY_TYPES
#	undef NV_PARAMETERIZED_TYPES_NO_LEGACY_TYPES
#endif

#if PX_VC && !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

PX_POP_PACK

#undef NV_PARAMETERIZED_TYPE
