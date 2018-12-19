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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#pragma warning(push)
#pragma warning(disable : 4668) //'symbol' is not defined as a preprocessor macro, replacing with '0' for 'directives'
#pragma warning(disable : 4987) // nonstandard extension used: 'throw (...)'
#include <intrin.h>
#pragma warning(pop)

#pragma warning(disable : 4127) // conditional expression is constant

typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;

namespace avx
{
__m128 sMaskYZW;
__m256 sOne, sEpsilon, sMinusOneXYZOneW, sMaskXY;

void initialize()
{
	sMaskYZW = _mm_castsi128_ps(_mm_setr_epi32(0, ~0, ~0, ~0));
	sOne = _mm256_set1_ps(1.0f);
	sEpsilon = _mm256_set1_ps(1.192092896e-07f);
	sMinusOneXYZOneW = _mm256_setr_ps(-1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f);
	sMaskXY = _mm256_castsi256_ps(_mm256_setr_epi32(~0, ~0, 0, 0, ~0, ~0, 0, 0));
}

template <uint32_t>
__m256 fmadd_ps(__m256 a, __m256 b, __m256 c)
{
	return _mm256_add_ps(_mm256_mul_ps(a, b), c);
}
template <uint32_t>
__m256 fnmadd_ps(__m256 a, __m256 b, __m256 c)
{
	return _mm256_sub_ps(c, _mm256_mul_ps(a, b));
}
#if _MSC_VER >= 1700
template <>
__m256 fmadd_ps<2>(__m256 a, __m256 b, __m256 c)
{
	return _mm256_fmadd_ps(a, b, c);
}
template <>
__m256 fnmadd_ps<2>(__m256 a, __m256 b, __m256 c)
{
	return _mm256_fnmadd_ps(a, b, c);
}
#endif

// roughly same perf as SSE2 intrinsics, the asm version below is about 10% faster
template <bool useMultiplier, uint32_t avx>
void solveConstraints(float* __restrict posIt, const float* __restrict rIt, const float* __restrict rEnd,
                      const uint16_t* __restrict iIt, const __m128& stiffnessRef)
{
	__m256 stiffness, stretchLimit, compressionLimit, multiplier;

	if(useMultiplier)
	{
		stiffness = _mm256_broadcast_ps(&stiffnessRef);
		stretchLimit = _mm256_permute_ps(stiffness, 0xff);
		compressionLimit = _mm256_permute_ps(stiffness, 0xaa);
		multiplier = _mm256_permute_ps(stiffness, 0x55);
		stiffness = _mm256_permute_ps(stiffness, 0x00);
	}
	else
	{
		stiffness = _mm256_broadcast_ss((const float*)&stiffnessRef);
	}

	for(; rIt < rEnd; rIt += 8, iIt += 16)
	{
		float* p0i = posIt + iIt[0] * 4;
		float* p4i = posIt + iIt[8] * 4;
		float* p0j = posIt + iIt[1] * 4;
		float* p4j = posIt + iIt[9] * 4;
		float* p1i = posIt + iIt[2] * 4;
		float* p5i = posIt + iIt[10] * 4;
		float* p1j = posIt + iIt[3] * 4;
		float* p5j = posIt + iIt[11] * 4;

		__m128 v0i = _mm_load_ps(p0i);
		__m128 v4i = _mm_load_ps(p4i);
		__m128 v0j = _mm_load_ps(p0j);
		__m128 v4j = _mm_load_ps(p4j);
		__m128 v1i = _mm_load_ps(p1i);
		__m128 v5i = _mm_load_ps(p5i);
		__m128 v1j = _mm_load_ps(p1j);
		__m128 v5j = _mm_load_ps(p5j);

		__m256 v04i = _mm256_insertf128_ps(_mm256_castps128_ps256(v0i), v4i, 1);
		__m256 v04j = _mm256_insertf128_ps(_mm256_castps128_ps256(v0j), v4j, 1);
		__m256 v15i = _mm256_insertf128_ps(_mm256_castps128_ps256(v1i), v5i, 1);
		__m256 v15j = _mm256_insertf128_ps(_mm256_castps128_ps256(v1j), v5j, 1);

		__m256 h04ij = fmadd_ps<avx>(sMinusOneXYZOneW, v04i, v04j);
		__m256 h15ij = fmadd_ps<avx>(sMinusOneXYZOneW, v15i, v15j);

		float* p2i = posIt + iIt[4] * 4;
		float* p6i = posIt + iIt[12] * 4;
		float* p2j = posIt + iIt[5] * 4;
		float* p6j = posIt + iIt[13] * 4;
		float* p3i = posIt + iIt[6] * 4;
		float* p7i = posIt + iIt[14] * 4;
		float* p3j = posIt + iIt[7] * 4;
		float* p7j = posIt + iIt[15] * 4;

		__m128 v2i = _mm_load_ps(p2i);
		__m128 v6i = _mm_load_ps(p6i);
		__m128 v2j = _mm_load_ps(p2j);
		__m128 v6j = _mm_load_ps(p6j);
		__m128 v3i = _mm_load_ps(p3i);
		__m128 v7i = _mm_load_ps(p7i);
		__m128 v3j = _mm_load_ps(p3j);
		__m128 v7j = _mm_load_ps(p7j);

		__m256 v26i = _mm256_insertf128_ps(_mm256_castps128_ps256(v2i), v6i, 1);
		__m256 v26j = _mm256_insertf128_ps(_mm256_castps128_ps256(v2j), v6j, 1);
		__m256 v37i = _mm256_insertf128_ps(_mm256_castps128_ps256(v3i), v7i, 1);
		__m256 v37j = _mm256_insertf128_ps(_mm256_castps128_ps256(v3j), v7j, 1);

		__m256 h26ij = fmadd_ps<avx>(sMinusOneXYZOneW, v26i, v26j);
		__m256 h37ij = fmadd_ps<avx>(sMinusOneXYZOneW, v37i, v37j);

		__m256 a = _mm256_unpacklo_ps(h04ij, h26ij);
		__m256 b = _mm256_unpackhi_ps(h04ij, h26ij);
		__m256 c = _mm256_unpacklo_ps(h15ij, h37ij);
		__m256 d = _mm256_unpackhi_ps(h15ij, h37ij);

		__m256 hxij = _mm256_unpacklo_ps(a, c);
		__m256 hyij = _mm256_unpackhi_ps(a, c);
		__m256 hzij = _mm256_unpacklo_ps(b, d);
		__m256 vwij = _mm256_unpackhi_ps(b, d);

		__m256 e2ij = fmadd_ps<avx>(hxij, hxij, fmadd_ps<avx>(hyij, hyij, fmadd_ps<avx>(hzij, hzij, sEpsilon)));

		__m256 rij = _mm256_load_ps(rIt);
		__m256 mask = _mm256_cmp_ps(rij, sEpsilon, _CMP_GT_OQ);
		__m256 erij = _mm256_and_ps(fnmadd_ps<avx>(rij, _mm256_rsqrt_ps(e2ij), sOne), mask);

		if(useMultiplier)
		{
			erij = fnmadd_ps<avx>(multiplier, _mm256_max_ps(compressionLimit, _mm256_min_ps(erij, stretchLimit)), erij);
		}

		__m256 exij = _mm256_mul_ps(erij, _mm256_mul_ps(stiffness, _mm256_rcp_ps(_mm256_add_ps(sEpsilon, vwij))));

		// replace these two instructions with _mm_maskstore_ps below?
		__m256 exlo = _mm256_and_ps(sMaskXY, exij);
		__m256 exhi = _mm256_andnot_ps(sMaskXY, exij);

		__m256 f04ij = _mm256_mul_ps(h04ij, _mm256_permute_ps(exlo, 0xc0));
		__m256 u04i = fmadd_ps<avx>(f04ij, _mm256_permute_ps(v04i, 0xff), v04i);
		__m256 u04j = fnmadd_ps<avx>(f04ij, _mm256_permute_ps(v04j, 0xff), v04j);

		_mm_store_ps(p0i, _mm256_extractf128_ps(u04i, 0));
		_mm_store_ps(p0j, _mm256_extractf128_ps(u04j, 0));
		_mm_store_ps(p4i, _mm256_extractf128_ps(u04i, 1));
		_mm_store_ps(p4j, _mm256_extractf128_ps(u04j, 1));

		__m256 f15ij = _mm256_mul_ps(h15ij, _mm256_permute_ps(exlo, 0xd5));
		__m256 u15i = fmadd_ps<avx>(f15ij, _mm256_permute_ps(v15i, 0xff), v15i);
		__m256 u15j = fnmadd_ps<avx>(f15ij, _mm256_permute_ps(v15j, 0xff), v15j);

		_mm_store_ps(p1i, _mm256_extractf128_ps(u15i, 0));
		_mm_store_ps(p1j, _mm256_extractf128_ps(u15j, 0));
		_mm_store_ps(p5i, _mm256_extractf128_ps(u15i, 1));
		_mm_store_ps(p5j, _mm256_extractf128_ps(u15j, 1));

		__m256 f26ij = _mm256_mul_ps(h26ij, _mm256_permute_ps(exhi, 0x2a));
		__m256 u26i = fmadd_ps<avx>(f26ij, _mm256_permute_ps(v26i, 0xff), v26i);
		__m256 u26j = fnmadd_ps<avx>(f26ij, _mm256_permute_ps(v26j, 0xff), v26j);

		_mm_store_ps(p2i, _mm256_extractf128_ps(u26i, 0));
		_mm_store_ps(p2j, _mm256_extractf128_ps(u26j, 0));
		_mm_store_ps(p6i, _mm256_extractf128_ps(u26i, 1));
		_mm_store_ps(p6j, _mm256_extractf128_ps(u26j, 1));

		__m256 f37ij = _mm256_mul_ps(h37ij, _mm256_permute_ps(exhi, 0x3f));
		__m256 u37i = fmadd_ps<avx>(f37ij, _mm256_permute_ps(v37i, 0xff), v37i);
		__m256 u37j = fnmadd_ps<avx>(f37ij, _mm256_permute_ps(v37j, 0xff), v37j);

		_mm_store_ps(p3i, _mm256_extractf128_ps(u37i, 0));
		_mm_store_ps(p3j, _mm256_extractf128_ps(u37j, 0));
		_mm_store_ps(p7i, _mm256_extractf128_ps(u37i, 1));
		_mm_store_ps(p7j, _mm256_extractf128_ps(u37j, 1));
	}

	_mm256_zeroupper();
}

#ifdef _M_IX86

// clang-format:disable

/* full template specializations of above functions in assembler */

// AVX without useMultiplier
template <>
void solveConstraints<false, 1>(float* __restrict posIt, const float* __restrict rIt,
                                const float* __restrict rEnd, const uint16_t* __restrict iIt, const __m128& stiffnessRef)
{
	__m256 stiffness = _mm256_broadcast_ss((const float*)&stiffnessRef);

	__m256 vtmp[8], htmp[4];
	float* ptmp[16];

	__asm 
	{
		mov edx, rIt
		mov esi, rEnd

		cmp edx, esi
		jae forEnd

		mov eax, iIt
		mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v0i
		movzx edi, WORD PTR [eax+16] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v4i
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v0j
		movzx edi, WORD PTR [eax+18] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v4j
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v1i
		movzx edi, WORD PTR [eax+20] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v5i
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v1j
		movzx edi, WORD PTR [eax+22] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v5j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp    ], ymm0 // v04i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+ 32], ymm2 // v04j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+ 64], ymm4 // v15i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+ 96], ymm6 // v15j

		vmovaps ymm7, sMinusOneXYZOneW
		vmulps ymm2, ymm2, ymm7 __asm vaddps ymm0, ymm0, ymm2 __asm vmovaps YMMWORD PTR [htmp   ], ymm0 // h04ij
		vmulps ymm6, ymm6, ymm7 __asm vaddps ymm4, ymm4, ymm6 __asm vmovaps YMMWORD PTR [htmp+32], ymm4 // h15ij

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+32], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v2i
		movzx edi, WORD PTR [eax+24] __asm shl edi, 4 __asm mov [ptmp+36], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v6i
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+40], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v2j
		movzx edi, WORD PTR [eax+26] __asm shl edi, 4 __asm mov [ptmp+44], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v6j
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+48], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v3i
		movzx edi, WORD PTR [eax+28] __asm shl edi, 4 __asm mov [ptmp+52], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v7i
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+56], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v3j
		movzx edi, WORD PTR [eax+30] __asm shl edi, 4 __asm mov [ptmp+60], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v7j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp+128], ymm0 // v26i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+160], ymm2 // v26j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+192], ymm4 // v37i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+224], ymm6 // v37j

		vmovaps ymm7, sMinusOneXYZOneW
		vmulps ymm2, ymm2, ymm7 __asm vaddps ymm2, ymm0, ymm2 __asm vmovaps YMMWORD PTR [htmp+64], ymm2 // h26ij
		vmulps ymm6, ymm6, ymm7 __asm vaddps ymm6, ymm4, ymm6 __asm vmovaps YMMWORD PTR [htmp+96], ymm6 // h37ij

		vmovaps ymm0, YMMWORD PTR [htmp   ] // h04ij
		vmovaps ymm4, YMMWORD PTR [htmp+32] // h15ij

		vunpcklps ymm1, ymm0, ymm2 // a
		vunpckhps ymm3, ymm0, ymm2 // b
		vunpcklps ymm5, ymm4, ymm6 // c
		vunpckhps ymm7, ymm4, ymm6 // d

		vunpcklps ymm0, ymm1, ymm5 // hxij
		vunpckhps ymm2, ymm1, ymm5 // hyij
		vunpcklps ymm4, ymm3, ymm7 // hzij
		vunpckhps ymm6, ymm3, ymm7 // vwij

		vmovaps ymm7, sEpsilon
		vmovaps ymm5, sOne
		vmovaps ymm3, stiffness
		vmovaps ymm1, YMMWORD PTR [edx] // rij

		vmulps ymm0, ymm0, ymm0 __asm vaddps ymm0, ymm0, ymm7 // e2ij
		vmulps ymm2, ymm2, ymm2 __asm vaddps ymm0, ymm0, ymm2
		vmulps ymm4, ymm4, ymm4 __asm vaddps ymm0, ymm0, ymm4

		vcmpgt_oqps ymm2, ymm1, ymm7 // mask
		vrsqrtps ymm0, ymm0 __asm vmulps ymm0, ymm0, ymm1 // erij
		vsubps ymm5, ymm5, ymm0 __asm vandps ymm5, ymm5, ymm2
		vaddps ymm6, ymm6, ymm7 __asm vrcpps ymm6, ymm6

		vmulps ymm6, ymm6, ymm3 __asm vmulps ymm6, ymm6, ymm5 // exij

		vmovaps ymm7, sMaskXY
		vandps ymm7, ymm7, ymm6 // exlo
		vxorps ymm6, ymm6, ymm7 // exhi

		vmovaps ymm4, YMMWORD PTR [htmp    ] // h04ij
		vmovaps ymm0, YMMWORD PTR [vtmp    ] // v04i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 32] // v04j

		vpermilps ymm5, ymm7, 0xc0 __asm vmulps ymm4, ymm4, ymm5 // f04ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u04i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u04j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp   ] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v0i
		mov edi, [ptmp+ 8] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v0j
		mov edi, [ptmp+ 4] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v4i
		mov edi, [ptmp+12] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v4j

		vmovaps ymm4, YMMWORD PTR [htmp+ 32] // h15ij
		vmovaps ymm0, YMMWORD PTR [vtmp+ 64] // v15i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 96] // v15j

		vpermilps ymm5, ymm7, 0xd5 __asm vmulps ymm4, ymm4, ymm5 // f15ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u15i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u15j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+16] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v1i
		mov edi, [ptmp+24] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v1j
		mov edi, [ptmp+20] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v5i
		mov edi, [ptmp+28] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v5j

		vmovaps ymm4, YMMWORD PTR [htmp+ 64] // h26ij
		vmovaps ymm0, YMMWORD PTR [vtmp+128] // v26i
		vmovaps ymm1, YMMWORD PTR [vtmp+160] // v26j

		vpermilps ymm5, ymm6, 0x2a __asm vmulps ymm4, ymm4, ymm5 // f26ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u26i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u26j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+32] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v2i
		mov edi, [ptmp+40] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v2j
		mov edi, [ptmp+36] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v6i
		mov edi, [ptmp+44] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v6j

		vmovaps ymm4, YMMWORD PTR [htmp+ 96] // h37ij
		vmovaps ymm0, YMMWORD PTR [vtmp+192] // v37i
		vmovaps ymm1, YMMWORD PTR [vtmp+224] // v37j

		vpermilps ymm5, ymm6, 0x3f __asm vmulps ymm4, ymm4, ymm5 // f37ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u37i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u37j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+48] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v3i
		mov edi, [ptmp+56] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v3j
		mov edi, [ptmp+52] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v7i
		mov edi, [ptmp+60] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v7j

		add eax, 32
		add edx, 32

		cmp edx, esi
		jb forBegin
forEnd:
	}

	_mm256_zeroupper();
}

// AVX with useMultiplier
template <>
void solveConstraints<true, 1>(float* __restrict posIt, const float* __restrict rIt,
                               const float* __restrict rEnd, const uint16_t* __restrict iIt, const __m128& stiffnessRef)
{
	__m256 stiffness = _mm256_broadcast_ps(&stiffnessRef);
	__m256 stretchLimit = _mm256_permute_ps(stiffness, 0xff);
	__m256 compressionLimit = _mm256_permute_ps(stiffness, 0xaa);
	__m256 multiplier = _mm256_permute_ps(stiffness, 0x55);
	stiffness = _mm256_permute_ps(stiffness, 0x00);

	__m256 vtmp[8], htmp[4];
	float* ptmp[16];

	__asm 
	{
		mov edx, rIt
		mov esi, rEnd

		cmp edx, esi
		jae forEnd

		mov eax, iIt
		mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v0i
		movzx edi, WORD PTR [eax+16] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v4i
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v0j
		movzx edi, WORD PTR [eax+18] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v4j
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v1i
		movzx edi, WORD PTR [eax+20] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v5i
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v1j
		movzx edi, WORD PTR [eax+22] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v5j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp    ], ymm0 // v04i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+ 32], ymm2 // v04j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+ 64], ymm4 // v15i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+ 96], ymm6 // v15j

		vmovaps ymm7, sMinusOneXYZOneW
		vmulps ymm2, ymm2, ymm7 __asm vaddps ymm0, ymm0, ymm2 __asm vmovaps YMMWORD PTR [htmp   ], ymm0 // h04ij
		vmulps ymm6, ymm6, ymm7 __asm vaddps ymm4, ymm4, ymm6 __asm vmovaps YMMWORD PTR [htmp+32], ymm4 // h15ij

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+32], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v2i
		movzx edi, WORD PTR [eax+24] __asm shl edi, 4 __asm mov [ptmp+36], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v6i
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+40], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v2j
		movzx edi, WORD PTR [eax+26] __asm shl edi, 4 __asm mov [ptmp+44], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v6j
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+48], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v3i
		movzx edi, WORD PTR [eax+28] __asm shl edi, 4 __asm mov [ptmp+52], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v7i
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+56], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v3j
		movzx edi, WORD PTR [eax+30] __asm shl edi, 4 __asm mov [ptmp+60], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v7j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp+128], ymm0 // v26i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+160], ymm2 // v26j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+192], ymm4 // v37i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+224], ymm6 // v37j

		vmovaps ymm7, sMinusOneXYZOneW
		vmulps ymm2, ymm2, ymm7 __asm vaddps ymm2, ymm0, ymm2 __asm vmovaps YMMWORD PTR [htmp+64], ymm2 // h26ij
		vmulps ymm6, ymm6, ymm7 __asm vaddps ymm6, ymm4, ymm6 __asm vmovaps YMMWORD PTR [htmp+96], ymm6 // h37ij

		vmovaps ymm0, YMMWORD PTR [htmp   ] // h04ij
		vmovaps ymm4, YMMWORD PTR [htmp+32] // h15ij

		vunpcklps ymm1, ymm0, ymm2 // a
		vunpckhps ymm3, ymm0, ymm2 // b
		vunpcklps ymm5, ymm4, ymm6 // c
		vunpckhps ymm7, ymm4, ymm6 // d

		vunpcklps ymm0, ymm1, ymm5 // hxij
		vunpckhps ymm2, ymm1, ymm5 // hyij
		vunpcklps ymm4, ymm3, ymm7 // hzij
		vunpckhps ymm6, ymm3, ymm7 // vwij

		vmovaps ymm7, sEpsilon
		vmovaps ymm5, sOne
		vmovaps ymm3, stiffness
		vmovaps ymm1, YMMWORD PTR [edx] // rij

		vmulps ymm0, ymm0, ymm0 __asm vaddps ymm0, ymm0, ymm7 // e2ij
		vmulps ymm2, ymm2, ymm2 __asm vaddps ymm0, ymm0, ymm2
		vmulps ymm4, ymm4, ymm4 __asm vaddps ymm0, ymm0, ymm4

		vcmpgt_oqps ymm2, ymm1, ymm7 // mask
		vrsqrtps ymm0, ymm0 __asm vmulps ymm0, ymm0, ymm1 // erij
		vsubps ymm5, ymm5, ymm0 __asm vandps ymm5, ymm5, ymm2
		vaddps ymm6, ymm6, ymm7 __asm vrcpps ymm6, ymm6

		vmovaps ymm0, stretchLimit // multiplier block
		vmovaps ymm1, compressionLimit
		vmovaps ymm2, multiplier
		vminps ymm0, ymm0, ymm5
		vmaxps ymm1, ymm1, ymm0
		vmulps ymm2, ymm2, ymm1
		vsubps ymm5, ymm5, ymm2

		vmulps ymm6, ymm6, ymm3 __asm vmulps ymm6, ymm6, ymm5 // exij

		vmovaps ymm7, sMaskXY
		vandps ymm7, ymm7, ymm6 // exlo
		vxorps ymm6, ymm6, ymm7 // exhi

		vmovaps ymm4, YMMWORD PTR [htmp    ] // h04ij
		vmovaps ymm0, YMMWORD PTR [vtmp    ] // v04i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 32] // v04j

		vpermilps ymm5, ymm7, 0xc0 __asm vmulps ymm4, ymm4, ymm5 // f04ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u04i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u04j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp   ] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v0i
		mov edi, [ptmp+ 8] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v0j
		mov edi, [ptmp+ 4] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v4i
		mov edi, [ptmp+12] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v4j

		vmovaps ymm4, YMMWORD PTR [htmp+ 32] // h15ij
		vmovaps ymm0, YMMWORD PTR [vtmp+ 64] // v15i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 96] // v15j

		vpermilps ymm5, ymm7, 0xd5 __asm vmulps ymm4, ymm4, ymm5 // f15ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u15i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u15j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+16] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v1i
		mov edi, [ptmp+24] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v1j
		mov edi, [ptmp+20] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v5i
		mov edi, [ptmp+28] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v5j

		vmovaps ymm4, YMMWORD PTR [htmp+ 64] // h26ij
		vmovaps ymm0, YMMWORD PTR [vtmp+128] // v26i
		vmovaps ymm1, YMMWORD PTR [vtmp+160] // v26j

		vpermilps ymm5, ymm6, 0x2a __asm vmulps ymm4, ymm4, ymm5 // f26ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u26i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u26j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+32] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v2i
		mov edi, [ptmp+40] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v2j
		mov edi, [ptmp+36] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v6i
		mov edi, [ptmp+44] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v6j

		vmovaps ymm4, YMMWORD PTR [htmp+ 96] // h37ij
		vmovaps ymm0, YMMWORD PTR [vtmp+192] // v37i
		vmovaps ymm1, YMMWORD PTR [vtmp+224] // v37j

		vpermilps ymm5, ymm6, 0x3f __asm vmulps ymm4, ymm4, ymm5 // f37ij
		vpermilps ymm2, ymm0, 0xff __asm vmulps ymm2, ymm2, ymm4 __asm vsubps ymm0, ymm0, ymm2 // u37i
		vpermilps ymm3, ymm1, 0xff __asm vmulps ymm3, ymm3, ymm4 __asm vaddps ymm1, ymm1, ymm3 // u37j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+48] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v3i
		mov edi, [ptmp+56] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v3j
		mov edi, [ptmp+52] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v7i
		mov edi, [ptmp+60] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v7j

		add eax, 32
		add edx, 32

		cmp edx, esi
		jb forBegin
forEnd:
	}

	_mm256_zeroupper();
}

#if _MSC_VER >= 1700
// AVX2 without useMultiplier
template <>
void solveConstraints<false, 2>(float* __restrict posIt, const float* __restrict rIt, 
                                const float* __restrict rEnd, const uint16_t* __restrict iIt, const __m128& stiffnessRef)
{
	__m256 stiffness = _mm256_broadcast_ss((const float*)&stiffnessRef);

	__m256 vtmp[8], htmp[4];
	float* ptmp[16];

	__asm 
	{
		mov edx, rIt
			mov esi, rEnd

			cmp edx, esi
			jae forEnd

			mov eax, iIt
			mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v0i
		movzx edi, WORD PTR [eax+16] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v4i
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v0j
		movzx edi, WORD PTR [eax+18] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v4j
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v1i
		movzx edi, WORD PTR [eax+20] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v5i
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v1j
		movzx edi, WORD PTR [eax+22] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v5j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp    ], ymm0 // v04i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+ 32], ymm2 // v04j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+ 64], ymm4 // v15i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+ 96], ymm6 // v15j

		vmovaps ymm7, sMinusOneXYZOneW
		vfmadd213ps ymm2, ymm7, ymm0 __asm vmovaps YMMWORD PTR [htmp   ], ymm2 // h04ij
		vfmadd213ps ymm6, ymm7, ymm4 __asm vmovaps YMMWORD PTR [htmp+32], ymm6 // h15ij

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+32], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v2i
		movzx edi, WORD PTR [eax+24] __asm shl edi, 4 __asm mov [ptmp+36], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v6i
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+40], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v2j
		movzx edi, WORD PTR [eax+26] __asm shl edi, 4 __asm mov [ptmp+44], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v6j
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+48], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v3i
		movzx edi, WORD PTR [eax+28] __asm shl edi, 4 __asm mov [ptmp+52], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v7i
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+56], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v3j
		movzx edi, WORD PTR [eax+30] __asm shl edi, 4 __asm mov [ptmp+60], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v7j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp+128], ymm0 // v26i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+160], ymm2 // v26j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+192], ymm4 // v37i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+224], ymm6 // v37j

		vmovaps ymm7, sMinusOneXYZOneW
		vfmadd213ps ymm2, ymm7, ymm0  __asm vmovaps YMMWORD PTR [htmp+64], ymm2 // h26ij
		vfmadd213ps ymm6, ymm7, ymm4  __asm vmovaps YMMWORD PTR [htmp+96], ymm6 // h37ij

		vmovaps ymm0, YMMWORD PTR [htmp   ] // h04ij
		vmovaps ymm4, YMMWORD PTR [htmp+32] // h15ij

		vunpcklps ymm1, ymm0, ymm2 // a
		vunpckhps ymm3, ymm0, ymm2 // b
		vunpcklps ymm5, ymm4, ymm6 // c
		vunpckhps ymm7, ymm4, ymm6 // d

		vunpcklps ymm0, ymm1, ymm5 // hxij
		vunpckhps ymm2, ymm1, ymm5 // hyij
		vunpcklps ymm4, ymm3, ymm7 // hzij
		vunpckhps ymm6, ymm3, ymm7 // vwij

		vmovaps ymm7, sEpsilon
		vmovaps ymm5, sOne
		vmovaps ymm3, stiffness
		vmovaps ymm1, YMMWORD PTR [edx] // rij

		vfmadd213ps ymm4, ymm4, ymm7 // e2ij
		vfmadd213ps ymm2, ymm2, ymm4
		vfmadd213ps ymm0, ymm0, ymm2

		vcmpgt_oqps ymm2, ymm1, ymm7 // mask
		vrsqrtps ymm0, ymm0 __asm vfnmadd231ps ymm5, ymm0, ymm1 // erij
		vandps ymm5, ymm5, ymm2
		vaddps ymm6, ymm6, ymm7 __asm vrcpps ymm6, ymm6

		vmulps ymm6, ymm6, ymm3 __asm vmulps ymm6, ymm6, ymm5 // exij

		vmovaps ymm7, sMaskXY
		vandps ymm7, ymm7, ymm6 // exlo
		vxorps ymm6, ymm6, ymm7 // exhi

		vmovaps ymm4, YMMWORD PTR [htmp    ] // h04ij
		vmovaps ymm0, YMMWORD PTR [vtmp    ] // v04i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 32] // v04j

		vpermilps ymm5, ymm7, 0xc0 __asm vmulps ymm4, ymm4, ymm5 // f04ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u04i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4  // u04j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp   ] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v0i
		mov edi, [ptmp+ 8] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v0j
		mov edi, [ptmp+ 4] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v4i
		mov edi, [ptmp+12] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v4j

		vmovaps ymm4, YMMWORD PTR [htmp+ 32] // h15ij
		vmovaps ymm0, YMMWORD PTR [vtmp+ 64] // v15i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 96] // v15j

		vpermilps ymm5, ymm7, 0xd5 __asm vmulps ymm4, ymm4, ymm5 // f15ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u15i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u15j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+16] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v1i
		mov edi, [ptmp+24] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v1j
		mov edi, [ptmp+20] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v5i
		mov edi, [ptmp+28] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v5j

		vmovaps ymm4, YMMWORD PTR [htmp+ 64] // h26ij
		vmovaps ymm0, YMMWORD PTR [vtmp+128] // v26i
		vmovaps ymm1, YMMWORD PTR [vtmp+160] // v26j

		vpermilps ymm5, ymm6, 0x2a __asm vmulps ymm4, ymm4, ymm5 // f26ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u26i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u26j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+32] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v2i
		mov edi, [ptmp+40] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v2j
		mov edi, [ptmp+36] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v6i
		mov edi, [ptmp+44] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v6j

		vmovaps ymm4, YMMWORD PTR [htmp+ 96] // h37ij
		vmovaps ymm0, YMMWORD PTR [vtmp+192] // v37i
		vmovaps ymm1, YMMWORD PTR [vtmp+224] // v37j

		vpermilps ymm5, ymm6, 0x3f __asm vmulps ymm4, ymm4, ymm5 // f37ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u37i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u37j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+48] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v3i
		mov edi, [ptmp+56] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v3j
		mov edi, [ptmp+52] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v7i
		mov edi, [ptmp+60] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v7j

		add eax, 32
		add edx, 32

		cmp edx, esi
		jb forBegin
forEnd:
	}

	_mm256_zeroupper();
}

// AVX2 with useMultiplier
template <>
void solveConstraints<true, 2>(float* __restrict posIt, const float* __restrict rIt, 
                               const float* __restrict rEnd, const uint16_t* __restrict iIt, const __m128& stiffnessRef)
{
	__m256 stiffness = _mm256_broadcast_ps(&stiffnessRef);
	__m256 stretchLimit = _mm256_permute_ps(stiffness, 0xff);
	__m256 compressionLimit = _mm256_permute_ps(stiffness, 0xaa);
	__m256 multiplier = _mm256_permute_ps(stiffness, 0x55);
	stiffness = _mm256_permute_ps(stiffness, 0x00);

	__m256 vtmp[8], htmp[4];
	float* ptmp[16];

	__asm 
	{
		mov edx, rIt
		mov esi, rEnd

		cmp edx, esi
		jae forEnd

		mov eax, iIt
		mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v0i
		movzx edi, WORD PTR [eax+16] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v4i
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v0j
		movzx edi, WORD PTR [eax+18] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v4j
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v1i
		movzx edi, WORD PTR [eax+20] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v5i
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v1j
		movzx edi, WORD PTR [eax+22] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v5j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp    ], ymm0 // v04i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+ 32], ymm2 // v04j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+ 64], ymm4 // v15i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+ 96], ymm6 // v15j

		vmovaps ymm7, sMinusOneXYZOneW
		vfmadd213ps ymm2, ymm7, ymm0 __asm vmovaps YMMWORD PTR [htmp   ], ymm2 // h04ij
		vfmadd213ps ymm6, ymm7, ymm4 __asm vmovaps YMMWORD PTR [htmp+32], ymm6 // h15ij

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+32], edi __asm vmovaps xmm0, XMMWORD PTR [edi + ecx] // v2i
		movzx edi, WORD PTR [eax+24] __asm shl edi, 4 __asm mov [ptmp+36], edi __asm vmovaps xmm1, XMMWORD PTR [edi + ecx] // v6i
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+40], edi __asm vmovaps xmm2, XMMWORD PTR [edi + ecx] // v2j
		movzx edi, WORD PTR [eax+26] __asm shl edi, 4 __asm mov [ptmp+44], edi __asm vmovaps xmm3, XMMWORD PTR [edi + ecx] // v6j
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+48], edi __asm vmovaps xmm4, XMMWORD PTR [edi + ecx] // v3i
		movzx edi, WORD PTR [eax+28] __asm shl edi, 4 __asm mov [ptmp+52], edi __asm vmovaps xmm5, XMMWORD PTR [edi + ecx] // v7i
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+56], edi __asm vmovaps xmm6, XMMWORD PTR [edi + ecx] // v3j
		movzx edi, WORD PTR [eax+30] __asm shl edi, 4 __asm mov [ptmp+60], edi __asm vmovaps xmm7, XMMWORD PTR [edi + ecx] // v7j

		vinsertf128 ymm0, ymm0, xmm1, 1 __asm vmovaps YMMWORD PTR [vtmp+128], ymm0 // v26i
		vinsertf128 ymm2, ymm2, xmm3, 1 __asm vmovaps YMMWORD PTR [vtmp+160], ymm2 // v26j
		vinsertf128 ymm4, ymm4, xmm5, 1 __asm vmovaps YMMWORD PTR [vtmp+192], ymm4 // v37i
		vinsertf128 ymm6, ymm6, xmm7, 1 __asm vmovaps YMMWORD PTR [vtmp+224], ymm6 // v37j

		vmovaps ymm7, sMinusOneXYZOneW
		vfmadd213ps ymm2, ymm7, ymm0  __asm vmovaps YMMWORD PTR [htmp+64], ymm2 // h26ij
		vfmadd213ps ymm6, ymm7, ymm4  __asm vmovaps YMMWORD PTR [htmp+96], ymm6 // h37ij

		vmovaps ymm0, YMMWORD PTR [htmp   ] // h04ij
		vmovaps ymm4, YMMWORD PTR [htmp+32] // h15ij

		vunpcklps ymm1, ymm0, ymm2 // a
		vunpckhps ymm3, ymm0, ymm2 // b
		vunpcklps ymm5, ymm4, ymm6 // c
		vunpckhps ymm7, ymm4, ymm6 // d

		vunpcklps ymm0, ymm1, ymm5 // hxij
		vunpckhps ymm2, ymm1, ymm5 // hyij
		vunpcklps ymm4, ymm3, ymm7 // hzij
		vunpckhps ymm6, ymm3, ymm7 // vwij

		vmovaps ymm7, sEpsilon
		vmovaps ymm5, sOne
		vmovaps ymm3, stiffness
		vmovaps ymm1, YMMWORD PTR [edx] // rij

		vfmadd213ps ymm4, ymm4, ymm7 // e2ij
		vfmadd213ps ymm2, ymm2, ymm4
		vfmadd213ps ymm0, ymm0, ymm2

		vcmpgt_oqps ymm2, ymm1, ymm7 // mask
		vrsqrtps ymm0, ymm0 __asm vfnmadd231ps ymm5, ymm0, ymm1 // erij
		vandps ymm5, ymm5, ymm2
		vaddps ymm6, ymm6, ymm7 __asm vrcpps ymm6, ymm6

		vmovaps ymm0, stretchLimit // multiplier block
		vmovaps ymm1, compressionLimit
		vmovaps ymm2, multiplier
		vminps ymm0, ymm0, ymm5
		vmaxps ymm1, ymm1, ymm0
		vfnmadd231ps ymm5, ymm1, ymm2

		vmulps ymm6, ymm6, ymm3 __asm vmulps ymm6, ymm6, ymm5 // exij

		vmovaps ymm7, sMaskXY
		vandps ymm7, ymm7, ymm6 // exlo
		vxorps ymm6, ymm6, ymm7 // exhi

		vmovaps ymm4, YMMWORD PTR [htmp    ] // h04ij
		vmovaps ymm0, YMMWORD PTR [vtmp    ] // v04i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 32] // v04j

		vpermilps ymm5, ymm7, 0xc0 __asm vmulps ymm4, ymm4, ymm5 // f04ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u04i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4  // u04j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp   ] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v0i
		mov edi, [ptmp+ 8] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v0j
		mov edi, [ptmp+ 4] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v4i
		mov edi, [ptmp+12] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v4j

		vmovaps ymm4, YMMWORD PTR [htmp+ 32] // h15ij
		vmovaps ymm0, YMMWORD PTR [vtmp+ 64] // v15i
		vmovaps ymm1, YMMWORD PTR [vtmp+ 96] // v15j

		vpermilps ymm5, ymm7, 0xd5 __asm vmulps ymm4, ymm4, ymm5 // f15ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u15i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u15j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+16] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v1i
		mov edi, [ptmp+24] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v1j
		mov edi, [ptmp+20] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v5i
		mov edi, [ptmp+28] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v5j

		vmovaps ymm4, YMMWORD PTR [htmp+ 64] // h26ij
		vmovaps ymm0, YMMWORD PTR [vtmp+128] // v26i
		vmovaps ymm1, YMMWORD PTR [vtmp+160] // v26j

		vpermilps ymm5, ymm6, 0x2a __asm vmulps ymm4, ymm4, ymm5 // f26ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u26i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u26j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+32] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v2i
		mov edi, [ptmp+40] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v2j
		mov edi, [ptmp+36] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v6i
		mov edi, [ptmp+44] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v6j

		vmovaps ymm4, YMMWORD PTR [htmp+ 96] // h37ij
		vmovaps ymm0, YMMWORD PTR [vtmp+192] // v37i
		vmovaps ymm1, YMMWORD PTR [vtmp+224] // v37j

		vpermilps ymm5, ymm6, 0x3f __asm vmulps ymm4, ymm4, ymm5 // f37ij
		vpermilps ymm2, ymm0, 0xff __asm vfnmadd231ps ymm0, ymm2, ymm4 // u37i
		vpermilps ymm3, ymm1, 0xff __asm vfmadd231ps ymm1, ymm3, ymm4 // u37j

		vextractf128 xmm2, ymm0, 1
		vextractf128 xmm3, ymm1, 1

		mov edi, [ptmp+48] __asm vmovaps XMMWORD PTR [edi + ecx], xmm0 // v3i
		mov edi, [ptmp+56] __asm vmovaps XMMWORD PTR [edi + ecx], xmm1 // v3j
		mov edi, [ptmp+52] __asm vmovaps XMMWORD PTR [edi + ecx], xmm2 // v7i
		mov edi, [ptmp+60] __asm vmovaps XMMWORD PTR [edi + ecx], xmm3 // v7j

		add eax, 32
		add edx, 32

		cmp edx, esi
		jb forBegin
forEnd:
	}

	_mm256_zeroupper();
}
#endif // _MSC_VER >= 1700

// clang-format:enable

#else // _M_IX86

template void solveConstraints<false, 1>(float* __restrict, const float* __restrict, const float* __restrict,
                                         const uint16_t* __restrict, const __m128&);

template void solveConstraints<true, 1>(float* __restrict, const float* __restrict, const float* __restrict,
                                        const uint16_t* __restrict, const __m128&);

template void solveConstraints<false, 2>(float* __restrict, const float* __restrict, const float* __restrict,
                                         const uint16_t* __restrict, const __m128&);

template void solveConstraints<true, 2>(float* __restrict, const float* __restrict, const float* __restrict,
                                        const uint16_t* __restrict, const __m128&);

#endif // _M_IX86

} // namespace avx
