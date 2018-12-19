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

template <bool useMultiplier>
void solveConstraints(float* __restrict posIt, const float* __restrict rIt, const float* __restrict rEnd,
                      const uint16_t* __restrict iIt, __m128 stiffness)
{
	__m128 sOne = _mm_set1_ps(1.0f);

	__m128 stretchLimit, compressionLimit, multiplier;
	if(useMultiplier)
	{
		stretchLimit = _mm_shuffle_ps(stiffness, stiffness, 0xff);
		compressionLimit = _mm_shuffle_ps(stiffness, stiffness, 0xaa);
		multiplier = _mm_shuffle_ps(stiffness, stiffness, 0x55);
	}
	stiffness = _mm_shuffle_ps(stiffness, stiffness, 0x00);

	for(; rIt != rEnd; rIt += 4, iIt += 8)
	{
		float* p0i = posIt + iIt[0] * 4;
		float* p0j = posIt + iIt[1] * 4;
		float* p1i = posIt + iIt[2] * 4;
		float* p1j = posIt + iIt[3] * 4;
		float* p2i = posIt + iIt[4] * 4;
		float* p2j = posIt + iIt[5] * 4;
		float* p3i = posIt + iIt[6] * 4;
		float* p3j = posIt + iIt[7] * 4;

		__m128 v0i = _mm_load_ps(p0i);
		__m128 v0j = _mm_load_ps(p0j);
		__m128 v1i = _mm_load_ps(p1i);
		__m128 v1j = _mm_load_ps(p1j);
		__m128 v2i = _mm_load_ps(p2i);
		__m128 v2j = _mm_load_ps(p2j);
		__m128 v3i = _mm_load_ps(p3i);
		__m128 v3j = _mm_load_ps(p3j);

		__m128 h0ij = _mm_add_ps(v0j, _mm_mul_ps(v0i, sMinusOneXYZOneW));
		__m128 h1ij = _mm_add_ps(v1j, _mm_mul_ps(v1i, sMinusOneXYZOneW));
		__m128 h2ij = _mm_add_ps(v2j, _mm_mul_ps(v2i, sMinusOneXYZOneW));
		__m128 h3ij = _mm_add_ps(v3j, _mm_mul_ps(v3i, sMinusOneXYZOneW));

		__m128 a = _mm_unpacklo_ps(h0ij, h2ij);
		__m128 b = _mm_unpackhi_ps(h0ij, h2ij);
		__m128 c = _mm_unpacklo_ps(h1ij, h3ij);
		__m128 d = _mm_unpackhi_ps(h1ij, h3ij);

		__m128 hxij = _mm_unpacklo_ps(a, c);
		__m128 hyij = _mm_unpackhi_ps(a, c);
		__m128 hzij = _mm_unpacklo_ps(b, d);
		__m128 vwij = _mm_unpackhi_ps(b, d);

		__m128 rij = _mm_load_ps(rIt);
		__m128 e2ij = _mm_add_ps(gSimd4fEpsilon, _mm_add_ps(_mm_mul_ps(hxij, hxij),
		                                                    _mm_add_ps(_mm_mul_ps(hyij, hyij), _mm_mul_ps(hzij, hzij))));
		__m128 mask = _mm_cmpnle_ps(rij, gSimd4fEpsilon);
		__m128 erij = _mm_and_ps(_mm_sub_ps(sOne, _mm_mul_ps(rij, _mm_rsqrt_ps(e2ij))), mask);

		if(useMultiplier)
		{
			erij = _mm_sub_ps(erij, _mm_mul_ps(multiplier, _mm_max_ps(compressionLimit, _mm_min_ps(erij, stretchLimit))));
		}
		__m128 exij = _mm_mul_ps(erij, _mm_mul_ps(stiffness, _mm_rcp_ps(_mm_add_ps(gSimd4fEpsilon, vwij))));

		__m128 exlo = _mm_and_ps(sMaskXY, exij);
		__m128 exhi = _mm_andnot_ps(sMaskXY, exij);

		__m128 f0ij = _mm_mul_ps(h0ij, _mm_shuffle_ps(exlo, exlo, 0xc0));
		__m128 f1ij = _mm_mul_ps(h1ij, _mm_shuffle_ps(exlo, exlo, 0xd5));
		__m128 f2ij = _mm_mul_ps(h2ij, _mm_shuffle_ps(exhi, exhi, 0x2a));
		__m128 f3ij = _mm_mul_ps(h3ij, _mm_shuffle_ps(exhi, exhi, 0x3f));

		__m128 u0i = _mm_add_ps(v0i, _mm_mul_ps(f0ij, _mm_shuffle_ps(v0i, v0i, 0xff)));
		__m128 u0j = _mm_sub_ps(v0j, _mm_mul_ps(f0ij, _mm_shuffle_ps(v0j, v0j, 0xff)));
		__m128 u1i = _mm_add_ps(v1i, _mm_mul_ps(f1ij, _mm_shuffle_ps(v1i, v1i, 0xff)));
		__m128 u1j = _mm_sub_ps(v1j, _mm_mul_ps(f1ij, _mm_shuffle_ps(v1j, v1j, 0xff)));
		__m128 u2i = _mm_add_ps(v2i, _mm_mul_ps(f2ij, _mm_shuffle_ps(v2i, v2i, 0xff)));
		__m128 u2j = _mm_sub_ps(v2j, _mm_mul_ps(f2ij, _mm_shuffle_ps(v2j, v2j, 0xff)));
		__m128 u3i = _mm_add_ps(v3i, _mm_mul_ps(f3ij, _mm_shuffle_ps(v3i, v3i, 0xff)));
		__m128 u3j = _mm_sub_ps(v3j, _mm_mul_ps(f3ij, _mm_shuffle_ps(v3j, v3j, 0xff)));

		_mm_store_ps(p0i, u0i);
		_mm_store_ps(p0j, u0j);
		_mm_store_ps(p1i, u1i);
		_mm_store_ps(p1j, u1j);
		_mm_store_ps(p2i, u2i);
		_mm_store_ps(p2j, u2j);
		_mm_store_ps(p3i, u3i);
		_mm_store_ps(p3j, u3j);
	}
}

#if PX_X86

// clang-format:disable

// asm blocks in static condition blocks don't get removed, specialize
template <>
void solveConstraints<false>(float* __restrict posIt, const float* __restrict rIt, const float* __restrict rEnd, 
                             const uint16_t* __restrict iIt, __m128 stiffness)
{
	__m128 sOne = _mm_set1_ps(1.0f);
	__m128 sEpsilon = gSimd4fEpsilon;
	stiffness = _mm_shuffle_ps(stiffness, stiffness, 0x00);

	__m128 htmp[4];
	float* ptmp[8];

	__asm 
	{
		mov edx, rIt
		mov esi, rEnd

		cmp edx, esi
		jae forEnd

		mov eax, iIt
		mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm movaps xmm0, XMMWORD PTR [edi + ecx] /* v0i */
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm movaps xmm2, XMMWORD PTR [edi + ecx] /* v0j */
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm movaps xmm1, XMMWORD PTR [edi + ecx] /* v1i */
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm movaps xmm3, XMMWORD PTR [edi + ecx] /* v1j */

		movaps xmm7, sMinusOneXYZOneW
		mulps xmm2, xmm7 __asm addps xmm0, xmm2 __asm movaps XMMWORD PTR [htmp   ], xmm0 /* h0ij */
		mulps xmm3, xmm7 __asm addps xmm1, xmm3 __asm movaps XMMWORD PTR [htmp+16], xmm1 /* h1ij */

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v2i */
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm movaps xmm2, XMMWORD PTR [edi + ecx] /* v2j */
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm movaps xmm5, XMMWORD PTR [edi + ecx] /* v3i */
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm movaps xmm3, XMMWORD PTR [edi + ecx] /* v3j */

		mulps xmm2, xmm7 __asm addps xmm2, xmm4 __asm movaps XMMWORD PTR [htmp+32], xmm2 /* h2ij */
		mulps xmm3, xmm7 __asm addps xmm3, xmm5 __asm movaps XMMWORD PTR [htmp+48], xmm3 /* h3ij */

		movaps xmm4, xmm0
		movaps xmm5, xmm1

		unpcklps xmm0, xmm2 /* a */
		unpckhps xmm4, xmm2 /* b */
		unpcklps xmm1, xmm3 /* c */
		unpckhps xmm5, xmm3 /* d */

		movaps xmm2, xmm0
		movaps xmm6, xmm4

		unpcklps xmm0, xmm1 /* hxij */
		unpckhps xmm2, xmm1 /* hyij */
		unpcklps xmm4, xmm5 /* hzij */
		unpckhps xmm6, xmm5 /* vwij */

		movaps xmm7, sEpsilon
		movaps xmm5, sOne
		movaps xmm3, stiffness
		movaps xmm1, XMMWORD PTR [edx] /* rij */

		mulps xmm0, xmm0 __asm addps xmm0, xmm7 /* e2ij */
		mulps xmm2, xmm2 __asm addps xmm0, xmm2
		mulps xmm4, xmm4 __asm addps xmm0, xmm4

		rsqrtps xmm0, xmm0 __asm mulps xmm0, xmm1 /* erij */
		cmpnleps xmm1, xmm7 /* mask */
		subps xmm5, xmm0 __asm andps xmm5, xmm1
		addps xmm6, xmm7 __asm rcpps xmm6, xmm6

		mulps xmm6, xmm3 __asm mulps xmm6, xmm5 /* exij */

		movaps xmm7, sMaskXY
		andps xmm7, xmm6 /* exlo */
		xorps xmm6, xmm7 /* exhi */

		movaps xmm0, XMMWORD PTR [htmp   ] /* h0ij */
		movaps xmm1, XMMWORD PTR [htmp+16] /* h1ij */
		movaps xmm2, XMMWORD PTR [htmp+32] /* h2ij */
		movaps xmm3, XMMWORD PTR [htmp+48] /* h3ij */

		pshufd xmm5, xmm7, 0xc0 __asm mulps xmm0, xmm5 /* f0ij */
		pshufd xmm7, xmm7, 0xd5 __asm mulps xmm1, xmm7 /* f1ij */
		pshufd xmm4, xmm6, 0x2a __asm mulps xmm2, xmm4 /* f2ij */
		pshufd xmm6, xmm6, 0x3f __asm mulps xmm3, xmm6 /* f3ij */

		mov edi, [ptmp   ] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v0i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm0 __asm subps xmm4, xmm5 /* u0i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+ 4] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v0j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm0 __asm addps xmm6, xmm7 /* u0j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+ 8] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v1i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm1 __asm subps xmm4, xmm5 /* u1i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+12] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v1j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm1 __asm addps xmm6, xmm7 /* u1j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+16] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v2i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm2 __asm subps xmm4, xmm5 /* u2i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+20] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v2j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm2 __asm addps xmm6, xmm7 /* u2j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+24] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v3i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm3 __asm subps xmm4, xmm5 /* u3i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+28] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v3j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm3 __asm addps xmm6, xmm7 /* u3j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		add eax, 16
		add edx, 16

		cmp edx, esi
		jb forBegin
forEnd:
	}
}

template <>
void solveConstraints<true>(float* __restrict posIt, const float* __restrict rIt, const float* __restrict rEnd, 
                            const uint16_t* __restrict iIt, __m128 stiffness)
{
	__m128 sOne = _mm_set1_ps(1.0f);
	__m128 sEpsilon = gSimd4fEpsilon;
	__m128 stretchLimit = _mm_shuffle_ps(stiffness, stiffness, 0xff);
	__m128 compressionLimit = _mm_shuffle_ps(stiffness, stiffness, 0xaa);
	__m128 multiplier = _mm_shuffle_ps(stiffness, stiffness, 0x55);
	stiffness = _mm_shuffle_ps(stiffness, stiffness, 0x00);

	__m128 htmp[4];
	float* ptmp[8];

	__asm 
	{
		mov edx, rIt
		mov esi, rEnd

		cmp edx, esi
		jae forEnd

		mov eax, iIt
		mov ecx, posIt

forBegin:
		movzx edi, WORD PTR [eax   ] __asm shl edi, 4 __asm mov [ptmp   ], edi __asm movaps xmm0, XMMWORD PTR [edi + ecx] /* v0i */
		movzx edi, WORD PTR [eax+ 2] __asm shl edi, 4 __asm mov [ptmp+ 4], edi __asm movaps xmm2, XMMWORD PTR [edi + ecx] /* v0j */
		movzx edi, WORD PTR [eax+ 4] __asm shl edi, 4 __asm mov [ptmp+ 8], edi __asm movaps xmm1, XMMWORD PTR [edi + ecx] /* v1i */
		movzx edi, WORD PTR [eax+ 6] __asm shl edi, 4 __asm mov [ptmp+12], edi __asm movaps xmm3, XMMWORD PTR [edi + ecx] /* v1j */

		movaps xmm7, sMinusOneXYZOneW
		mulps xmm2, xmm7 __asm addps xmm0, xmm2 __asm movaps XMMWORD PTR [htmp   ], xmm0 /* h0ij */
		mulps xmm3, xmm7 __asm addps xmm1, xmm3 __asm movaps XMMWORD PTR [htmp+16], xmm1 /* h1ij */

		movzx edi, WORD PTR [eax+ 8] __asm shl edi, 4 __asm mov [ptmp+16], edi __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v2i */
		movzx edi, WORD PTR [eax+10] __asm shl edi, 4 __asm mov [ptmp+20], edi __asm movaps xmm2, XMMWORD PTR [edi + ecx] /* v2j */
		movzx edi, WORD PTR [eax+12] __asm shl edi, 4 __asm mov [ptmp+24], edi __asm movaps xmm5, XMMWORD PTR [edi + ecx] /* v3i */
		movzx edi, WORD PTR [eax+14] __asm shl edi, 4 __asm mov [ptmp+28], edi __asm movaps xmm3, XMMWORD PTR [edi + ecx] /* v3j */

		mulps xmm2, xmm7 __asm addps xmm2, xmm4 __asm movaps XMMWORD PTR [htmp+32], xmm2 /* h2ij */
		mulps xmm3, xmm7 __asm addps xmm3, xmm5 __asm movaps XMMWORD PTR [htmp+48], xmm3 /* h3ij */

		movaps xmm4, xmm0
		movaps xmm5, xmm1

		unpcklps xmm0, xmm2 /* a */
		unpckhps xmm4, xmm2 /* b */
		unpcklps xmm1, xmm3 /* c */
		unpckhps xmm5, xmm3 /* d */

		movaps xmm2, xmm0
		movaps xmm6, xmm4

		unpcklps xmm0, xmm1 /* hxij */
		unpckhps xmm2, xmm1 /* hyij */
		unpcklps xmm4, xmm5 /* hzij */
		unpckhps xmm6, xmm5 /* vwij */

		movaps xmm7, sEpsilon
		movaps xmm5, sOne
		movaps xmm3, stiffness
		movaps xmm1, XMMWORD PTR [edx] /* rij */

		mulps xmm0, xmm0 __asm addps xmm0, xmm7 /* e2ij */
		mulps xmm2, xmm2 __asm addps xmm0, xmm2
		mulps xmm4, xmm4 __asm addps xmm0, xmm4

		rsqrtps xmm0, xmm0 __asm mulps xmm0, xmm1 /* erij */
		cmpnleps xmm1, xmm7 /* mask */
		subps xmm5, xmm0 __asm andps xmm5, xmm1
		addps xmm6, xmm7 __asm rcpps xmm6, xmm6

		movaps xmm0, stretchLimit /* multiplier block */
		movaps xmm1, compressionLimit
		movaps xmm2, multiplier
		minps xmm0, xmm5
		maxps xmm1, xmm0
		mulps xmm2, xmm1
		subps xmm5, xmm2

		mulps xmm6, xmm3 __asm mulps xmm6, xmm5 /* exij */

		movaps xmm7, sMaskXY
		andps xmm7, xmm6 /* exlo */
		xorps xmm6, xmm7 /* exhi */

		movaps xmm0, XMMWORD PTR [htmp   ] /* h0ij */
		movaps xmm1, XMMWORD PTR [htmp+16] /* h1ij */
		movaps xmm2, XMMWORD PTR [htmp+32] /* h2ij */
		movaps xmm3, XMMWORD PTR [htmp+48] /* h3ij */

		pshufd xmm5, xmm7, 0xc0 __asm mulps xmm0, xmm5 /* f0ij */
		pshufd xmm7, xmm7, 0xd5 __asm mulps xmm1, xmm7 /* f1ij */
		pshufd xmm4, xmm6, 0x2a __asm mulps xmm2, xmm4 /* f2ij */
		pshufd xmm6, xmm6, 0x3f __asm mulps xmm3, xmm6 /* f3ij */

		mov edi, [ptmp   ] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v0i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm0 __asm subps xmm4, xmm5 /* u0i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+ 4] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v0j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm0 __asm addps xmm6, xmm7 /* u0j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+ 8] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v1i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm1 __asm subps xmm4, xmm5 /* u1i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+12] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v1j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm1 __asm addps xmm6, xmm7 /* u1j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+16] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v2i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm2 __asm subps xmm4, xmm5 /* u2i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+20] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v2j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm2 __asm addps xmm6, xmm7 /* u2j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		mov edi, [ptmp+24] __asm movaps xmm4, XMMWORD PTR [edi + ecx] /* v3i */
		pshufd xmm5, xmm4, 0xff __asm mulps xmm5, xmm3 __asm subps xmm4, xmm5 /* u3i */
		movaps XMMWORD PTR [edi + ecx], xmm4

		mov edi, [ptmp+28] __asm movaps xmm6, XMMWORD PTR [edi + ecx] /* v3j */
		pshufd xmm7, xmm6, 0xff __asm mulps xmm7, xmm3 __asm addps xmm6, xmm7 /* u3j */
		movaps XMMWORD PTR [edi + ecx], xmm6

		add eax, 16
		add edx, 16

		cmp edx, esi
		jb forBegin
forEnd:
	}
}

// clang-format:enable

#endif
