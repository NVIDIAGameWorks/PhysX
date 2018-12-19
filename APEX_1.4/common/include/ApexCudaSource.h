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



#ifndef __APEX_CUDA_SOURCE_H__
#define __APEX_CUDA_SOURCE_H__


#undef APEX_CUDA_TEXTURE_1D
#undef APEX_CUDA_TEXTURE_2D
#undef APEX_CUDA_TEXTURE_3D
#undef APEX_CUDA_TEXTURE_3D_FILTER
#undef APEX_CUDA_SURFACE_1D
#undef APEX_CUDA_SURFACE_2D
#undef APEX_CUDA_SURFACE_3D
#undef APEX_CUDA_STORAGE_SIZE
#undef APEX_CUDA_FREE_KERNEL
#undef APEX_CUDA_FREE_KERNEL_2D
#undef APEX_CUDA_FREE_KERNEL_3D
#undef APEX_CUDA_SYNC_KERNEL
#undef APEX_CUDA_BOUND_KERNEL

#define __APEX_CUDA_OBJ(name) initCudaObj( APEX_CUDA_OBJ_NAME(name) );

#define APEX_CUDA_TEXTURE_1D(name, format) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_TEXTURE_2D(name, format) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_TEXTURE_3D(name, format) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_TEXTURE_3D_FILTER(name, format, filter) __APEX_CUDA_OBJ(name)

#define APEX_CUDA_SURFACE_1D(name) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_SURFACE_2D(name) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_SURFACE_3D(name) __APEX_CUDA_OBJ(name)

#define APEX_CUDA_STORAGE_SIZE(name, size) __APEX_CUDA_OBJ(name)

#define APEX_CUDA_FREE_KERNEL(warps, name, argseq) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_FREE_KERNEL_2D(warps, name, argseq) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_FREE_KERNEL_3D(warps, name, argseq) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_SYNC_KERNEL(warps, name, argseq) __APEX_CUDA_OBJ(name)
#define APEX_CUDA_BOUND_KERNEL(warps, name, argseq) __APEX_CUDA_OBJ(name)


#endif //__APEX_CUDA_SOURCE_H__
