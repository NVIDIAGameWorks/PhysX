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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.

#ifndef PSFOUNDATION_PSUNIXLSXINLINEAOS_H
#define PSFOUNDATION_PSUNIXLSXINLINEAOS_H

#if !COMPILE_VECTOR_INTRINSICS
#error Vector intrinsics should not be included when using scalar implementation.
#endif

namespace physx
{
namespace shdfnd
{
namespace aos
{

#define PX_FPCLASS_SNAN 0x0001 /* signaling NaN */
#define PX_FPCLASS_QNAN 0x0002 /* quiet NaN */
#define PX_FPCLASS_NINF 0x0004 /* negative infinity */
#define PX_FPCLASS_PINF 0x0200 /* positive infinity */

PX_FORCE_INLINE __m128 m128_I2F(__m128i n)
{
	return (__m128)n;
}
PX_FORCE_INLINE __m128i m128_F2I(__m128 n)
{
	return (__m128i)n;
}

//////////////////////////////////////////////////////////////////////
//Test that Vec3V and FloatV are legal
//////////////////////////////////////////////////////////////////////

#define FLOAT_COMPONENTS_EQUAL_THRESHOLD 0.01f
PX_FORCE_INLINE static bool isValidFloatV(const FloatV a)
{
	const PxF32 x = V4ReadX(a);
	const PxF32 y = V4ReadY(a);
	const PxF32 z = V4ReadZ(a);
	const PxF32 w = V4ReadW(a);

	if (
		(PxAbs(x - y) < FLOAT_COMPONENTS_EQUAL_THRESHOLD) &&
		(PxAbs(x - z) < FLOAT_COMPONENTS_EQUAL_THRESHOLD) &&
		(PxAbs(x - w) < FLOAT_COMPONENTS_EQUAL_THRESHOLD)
		)
	{
		return true;
	}

	if (
		(PxAbs((x - y) / x) < FLOAT_COMPONENTS_EQUAL_THRESHOLD) &&
		(PxAbs((x - z) / x) < FLOAT_COMPONENTS_EQUAL_THRESHOLD) &&
		(PxAbs((x - w) / x) < FLOAT_COMPONENTS_EQUAL_THRESHOLD)
		)
	{
		return true;
	}

	return false;
}

PX_FORCE_INLINE bool isValidVec3V(const Vec3V a)
{
	PX_ALIGN(16, PxF32 f[4]);
	V4StoreA(a, f);
	return (f[3] == 0.0f);
}

PX_FORCE_INLINE bool isFiniteLength(const Vec3V a)
{
	return !FAllEq(V4LengthSq(a), FZero());
}

PX_FORCE_INLINE bool isAligned16(void* a)
{
	return(0 == (size_t(a) & 0x0f));
}

//ASSERT_FINITELENGTH is deactivated because there is a lot of code that calls a simd normalisation function with zero length but then ignores the result.

#if PX_DEBUG
#define ASSERT_ISVALIDVEC3V(a) PX_ASSERT(isValidVec3V(a))
#define ASSERT_ISVALIDFLOATV(a) PX_ASSERT(isValidFloatV(a))
#define ASSERT_ISALIGNED16(a) PX_ASSERT(isAligned16(reinterpret_cast<void*>(a)))
#define ASSERT_ISFINITELENGTH(a) //PX_ASSERT(isFiniteLength(a))
#else
#define ASSERT_ISVALIDVEC3V(a)
#define ASSERT_ISVALIDFLOATV(a)
#define ASSERT_ISALIGNED16(a)
#define ASSERT_ISFINITELENGTH(a)
#endif


namespace internalUnitLSXSimd
{
PX_FORCE_INLINE PxU32 BAllTrue4_R(const BoolV a)
{
	__m128i mask = __lsx_vmskltz_w(a);
	return PxU32(__lsx_vpickve2gr_w(mask, 0) == 0xf);
}

PX_FORCE_INLINE PxU32 BAllTrue3_R(const BoolV a)
{
	__m128i mask = __lsx_vmskltz_w(a);
	return PxU32((__lsx_vpickve2gr_w(mask, 0) & 0x7) == 0x7);
}

PX_FORCE_INLINE PxU32 BAnyTrue4_R(const BoolV a)
{
	__m128i mask = __lsx_vmskltz_w(a);
	return PxU32(__lsx_vpickve2gr_w(mask, 0) != 0x0);
}

PX_FORCE_INLINE PxU32 BAnyTrue3_R(const BoolV a)
{
	__m128i mask = __lsx_vmskltz_w(a);
	return PxU32((__lsx_vpickve2gr_w(mask, 0) & 0x7) != 0x0);
}
}

namespace _VecMathTests
{
// PT: this function returns an invalid Vec3V (W!=0.0f) just for unit-testing 'isValidVec3V'
PX_FORCE_INLINE Vec3V getInvalidVec3V()
{
	const float f = 1.0f;
	return __lsx_vldrepl_w(&f, 0);
}

PX_FORCE_INLINE bool allElementsEqualFloatV(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	__m128i m0 = __lsx_vfcmp_ceq_s(a, b);
	return __lsx_vpickve2gr_w(m0, 0) != 0;
}

PX_FORCE_INLINE bool allElementsEqualVec3V(const Vec3V a, const Vec3V b)
{
	return V3AllEq(a, b) != 0;
}

PX_FORCE_INLINE bool allElementsEqualVec4V(const Vec4V a, const Vec4V b)
{
	return V4AllEq(a, b) != 0;
}

PX_FORCE_INLINE bool allElementsEqualBoolV(const BoolV a, const BoolV b)
{
	return internalUnitLSXSimd::BAllTrue4_R(VecI32V_IsEq(m128_F2I(a), m128_F2I(b))) != 0;
}

PX_FORCE_INLINE bool allElementsEqualVecU32V(const VecU32V a, const VecU32V b)
{
	return internalUnitLSXSimd::BAllTrue4_R(V4IsEqU32(a, b)) != 0;
}

PX_FORCE_INLINE bool allElementsEqualVecI32V(const VecI32V a, const VecI32V b)
{
	BoolV c = m128_I2F(__lsx_vseq_w(a, b));
	return internalUnitLSXSimd::BAllTrue4_R(c) != 0;
}

#define VECMATH_AOS_EPSILON (1e-3f)

PX_FORCE_INLINE bool allElementsNearEqualFloatV(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	const FloatV c = FSub(a, b);
	const FloatV error = FLoad(VECMATH_AOS_EPSILON);
	__m128i mask = __lsx_vfcmp_cult_s(FAbs(c), error); //abs(c) < error
	return __lsx_vpickve2gr_w(mask, 0) != 0;
}

PX_FORCE_INLINE bool allElementsNearEqualVec3V(const Vec3V a, const Vec3V b)
{
	const Vec3V c = V3Sub(a, b);
	const Vec3V error = V3Load(VECMATH_AOS_EPSILON);
	__m128i mask = __lsx_vfcmp_cult_s(FAbs(c), error);
	return internalUnitLSXSimd::BAllTrue3_R(mask) != 0;
}

PX_FORCE_INLINE bool allElementsNearEqualVec4V(const Vec4V a, const Vec4V b)
{
	const Vec4V c = V4Sub(a, b);
	const Vec4V error = V4Load(VECMATH_AOS_EPSILON);
	__m128i mask = __lsx_vfcmp_cult_s(FAbs(c), error);
	return internalUnitLSXSimd::BAllTrue4_R(mask) != 0;
}
}

/////////////////////////////////////////////////////////////////////
////FUNCTIONS USED ONLY FOR ASSERTS IN VECTORISED IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE bool isFiniteFloatV(const FloatV a)
{
	return PxIsFinite(a[0]) && PxIsFinite(a[1]);
}

PX_FORCE_INLINE bool isFiniteVec3V(const Vec3V a)
{
	return PxIsFinite(a[0]) && PxIsFinite(a[1]) && PxIsFinite(a[2]);
}

PX_FORCE_INLINE bool isFiniteVec4V(const Vec4V a)
{
	return PxIsFinite(a[0]) && PxIsFinite(a[1]) && PxIsFinite(a[2]) && PxIsFinite(a[3]);
}

PX_FORCE_INLINE bool hasZeroElementinFloatV(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return (__lsx_vpickve2gr_w(a, 0) == 0);
}

PX_FORCE_INLINE bool hasZeroElementInVec3V(const Vec3V a)
{
	return (__lsx_vpickve2gr_w(a, 0) == 0 ||
			__lsx_vpickve2gr_w(a, 1) == 0 ||
			__lsx_vpickve2gr_w(a, 2) == 0);
}

PX_FORCE_INLINE bool hasZeroElementInVec4V(const Vec4V a)
{
	return (__lsx_vpickve2gr_w(a, 0) == 0 ||
			__lsx_vpickve2gr_w(a, 1) == 0 ||
			__lsx_vpickve2gr_w(a, 2) == 0 ||
			__lsx_vpickve2gr_w(a, 3) == 0);
}

/////////////////////////////////////////////////////////////////////
////VECTORISED FUNCTION IMPLEMENTATIONS
/////////////////////////////////////////////////////////////////////

PX_FORCE_INLINE FloatV FLoad(const PxF32 f)
{
	return __lsx_vldrepl_w(&f, 0);
}

PX_FORCE_INLINE Vec3V V3Load(const PxF32 f)
{
	PX_ALIGN(16, PxF32) data[4] = { f, f, f, 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec4V V4Load(const PxF32 f)
{
	return __lsx_vldrepl_w(&f, 0);
}

PX_FORCE_INLINE BoolV BLoad(const bool f)
{
	const PxU32 i = -PxI32(f);
	return __lsx_vldrepl_w(&i, 0);;
}

PX_FORCE_INLINE Vec3V V3LoadA(const PxVec3& f)
{
	PX_ALIGN(16, PxF32) data[4] = { f.x, f.y, f.z, 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec3V V3LoadU(const PxVec3& f)
{
	PX_ALIGN(16, PxF32) data[4] = { f.x, f.y, f.z, 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec3V V3LoadUnsafeA(const PxVec3& f)
{
	ASSERT_ISALIGNED16(const_cast<PxVec3*>(&f));
	PX_ALIGN(16, PxF32) data[4] = { f.x, f.y, f.z, 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec3V V3LoadA(const PxF32* const f)
{
	ASSERT_ISALIGNED16(f);
	PX_ALIGN(16, PxF32) data[4] = { f[0], f[1], f[2], 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec3V V3LoadU(const PxF32* const i)
{
	PX_ALIGN(16, PxF32) data[4] = { i[0], i[1], i[2], 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec3V Vec3V_From_Vec4V(Vec4V v)
{
	return V4ClearW(v);
}

PX_FORCE_INLINE Vec3V Vec3V_From_Vec4V_WUndefined(const Vec4V v)
{
	return v;
}

PX_FORCE_INLINE Vec4V Vec4V_From_Vec3V(Vec3V f)
{
	ASSERT_ISVALIDVEC3V(f);
	return f; // ok if it is implemented as the same type.
}

PX_FORCE_INLINE Vec4V Vec4V_From_PxVec3_WUndefined(const PxVec3& f)
{
	PX_ALIGN(16, PxF32) data[4] = { f.x, f.y, f.z, 0.0f };
	return V4LoadA(data);
}

PX_FORCE_INLINE Vec4V Vec4V_From_FloatV(FloatV f)
{
	return __lsx_vpermi_w(f, f, 0x00);
}

PX_FORCE_INLINE Vec3V Vec3V_From_FloatV(FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return Vec3V_From_Vec4V(Vec4V_From_FloatV(f));
}

PX_FORCE_INLINE Vec3V Vec3V_From_FloatV_WUndefined(FloatV f)
{
	ASSERT_ISVALIDVEC3V(f);
	return Vec3V_From_Vec4V_WUndefined(Vec4V_From_FloatV(f));
}

PX_FORCE_INLINE Mat33V Mat33V_From_PxMat33(const PxMat33& m)
{
	return Mat33V(V3LoadU(m.column0), V3LoadU(m.column1), V3LoadU(m.column2));
}

PX_FORCE_INLINE void PxMat33_From_Mat33V(const Mat33V& m, PxMat33& out)
{
	V3StoreU(m.col0, out.column0);
	V3StoreU(m.col1, out.column1);
	V3StoreU(m.col2, out.column2);
}

PX_FORCE_INLINE Vec4V V4LoadA(const PxF32* const f)
{
	ASSERT_ISALIGNED16(const_cast<PxF32*>(f));
	return __lsx_vld(f, 0);
}

PX_FORCE_INLINE void V4StoreA(Vec4V a, PxF32* f)
{
	ASSERT_ISALIGNED16(f);
	__lsx_vst(a, f, 0);
}

PX_FORCE_INLINE void V4StoreU(const Vec4V a, PxF32* f)
{
	__lsx_vst(a, f, 0);
}

PX_FORCE_INLINE void BStoreA(const BoolV a, PxU32* f)
{
	ASSERT_ISALIGNED16(f);
	__lsx_vst(a, f, 0);
}

PX_FORCE_INLINE void U4StoreA(const VecU32V uv, PxU32* u)
{
	ASSERT_ISALIGNED16(u);
	__lsx_vst(uv, u, 0);
}

PX_FORCE_INLINE void I4StoreA(const VecI32V iv, PxI32* i)
{
	ASSERT_ISALIGNED16(i);
	__lsx_vst(iv, i, 0);
}

PX_FORCE_INLINE Vec4V V4LoadU(const PxF32* const f)
{
	return __lsx_vld(f, 0);
}

PX_FORCE_INLINE BoolV BLoad(const bool* const f)
{
	const PX_ALIGN(16, PxI32) b[4] = { -PxI32(f[0]), -PxI32(f[1]), -PxI32(f[2]), -PxI32(f[3]) };
	return __lsx_vld(b, 0);
}

PX_FORCE_INLINE void FStore(const FloatV a, PxF32* PX_RESTRICT f)
{
	ASSERT_ISVALIDFLOATV(a);
	__lsx_vstelm_w(a, f, 0, 0);
}

PX_FORCE_INLINE void V3StoreA(const Vec3V a, PxVec3& f)
{
	ASSERT_ISALIGNED16(&f);
	f = PxVec3(a[0], a[1], a[2]);
}

PX_FORCE_INLINE void V3StoreU(const Vec3V a, PxVec3& f)
{
	f = PxVec3(a[0], a[1], a[2]);
}

PX_FORCE_INLINE void Store_From_BoolV(const BoolV b, PxU32* b2)
{
	__lsx_vstelm_w(b, b2, 0, 0);
}

PX_FORCE_INLINE VecU32V U4Load(const PxU32 i)
{
	return __lsx_vldrepl_w(&i, 0);
}

PX_FORCE_INLINE VecU32V U4LoadU(const PxU32* i)
{
	return __lsx_vldrepl_w(i, 0);
}

PX_FORCE_INLINE VecU32V U4LoadA(const PxU32* i)
{
	ASSERT_ISALIGNED16(const_cast<PxU32*>(i));
	return __lsx_vldrepl_w(i, 0);
}

//////////////////////////////////
// FLOATV
//////////////////////////////////

PX_FORCE_INLINE FloatV FZero()
{
	return FLoad(0.0f);
}

PX_FORCE_INLINE FloatV FOne()
{
	return FLoad(1.0f);
}

PX_FORCE_INLINE FloatV FHalf()
{
	return FLoad(0.5f);
}

PX_FORCE_INLINE FloatV FEps()
{
	return FLoad(PX_EPS_REAL);
}

PX_FORCE_INLINE FloatV FEps6()
{
	return FLoad(1e-6f);
}

PX_FORCE_INLINE FloatV FMax()
{
	return FLoad(PX_MAX_REAL);
}

PX_FORCE_INLINE FloatV FNegMax()
{
	return FLoad(-PX_MAX_REAL);
}

PX_FORCE_INLINE FloatV IZero()
{
	return __lsx_vldi(0);
}

PX_FORCE_INLINE FloatV IOne()
{
	const PxU32 one = 1;
	return __lsx_vreplgr2vr_w(one);
}

PX_FORCE_INLINE FloatV ITwo()
{
	const PxU32 two = 2;
	return __lsx_vreplgr2vr_w(two);
}

PX_FORCE_INLINE FloatV IThree()
{
	const PxU32 three = 3;
	return __lsx_vreplgr2vr_w(three);
}

PX_FORCE_INLINE FloatV IFour()
{
	const PxU32 four = 4;
	return __lsx_vreplgr2vr_w(four);
}

PX_FORCE_INLINE FloatV FNeg(const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return (__m128)__lsx_vbitrevi_w((__m128i)f, 31);
}

PX_FORCE_INLINE FloatV FAdd(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfadd_s(a, b);
}

PX_FORCE_INLINE FloatV FSub(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfsub_s(a, b);
}

PX_FORCE_INLINE FloatV FMul(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmul_s(a, b);
}

PX_FORCE_INLINE FloatV FDiv(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfdiv_s(a, b);
}

PX_FORCE_INLINE FloatV FDivFast(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmul_s(a, __lsx_vfrecip_s(b));
}

PX_FORCE_INLINE FloatV FRecip(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return __lsx_vfdiv_s(FOne(), a);
}

PX_FORCE_INLINE FloatV FRecipFast(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return __lsx_vfrecip_s(a);
}

PX_FORCE_INLINE FloatV FRsqrt(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return __lsx_vfdiv_s(FOne(), __lsx_vfsqrt_s(a));
}

PX_FORCE_INLINE FloatV FSqrt(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return __lsx_vfsqrt_s(a);
}

PX_FORCE_INLINE FloatV FRsqrtFast(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return __lsx_vfrsqrt_s(a);
}

PX_FORCE_INLINE FloatV FScaleAdd(const FloatV a, const FloatV b, const FloatV c)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	ASSERT_ISVALIDFLOATV(c);
	return FAdd(FMul(a, b), c);
}

PX_FORCE_INLINE FloatV FNegScaleSub(const FloatV a, const FloatV b, const FloatV c)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	ASSERT_ISVALIDFLOATV(c);
	return FSub(c, FMul(a, b));
}

PX_FORCE_INLINE FloatV FAbs(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	return (__m128)__lsx_vbitclri_w((__m128i)a, 31);
}

PX_FORCE_INLINE FloatV FSel(const BoolV c, const FloatV a, const FloatV b)
{
	PX_ASSERT(_VecMathTests::allElementsEqualBoolV(c,BTTTT()) ||
			  _VecMathTests::allElementsEqualBoolV(c,BFFFF()));
	ASSERT_ISVALIDFLOATV(__lsx_vbitsel_v(b, a, c));
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE BoolV FIsGrtr(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfcmp_clt_s(b, a);
}

PX_FORCE_INLINE BoolV FIsGrtrOrEq(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfcmp_cle_s(b, a);
}

PX_FORCE_INLINE BoolV FIsEq(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfcmp_cueq_s(a, b);
}

PX_FORCE_INLINE FloatV FMax(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmax_s(a, b);
}

PX_FORCE_INLINE FloatV FMin(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmin_s(a, b);
}

PX_FORCE_INLINE FloatV FClamp(const FloatV a, const FloatV minV, const FloatV maxV)
{
	ASSERT_ISVALIDFLOATV(minV);
	ASSERT_ISVALIDFLOATV(maxV);
	return __lsx_vfmax_s(__lsx_vfmin_s(a, maxV), minV);
}

PX_FORCE_INLINE PxU32 FAllGrtr(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vpickve2gr_w(__lsx_vfcmp_cult_s(b, a), 0);
}

PX_FORCE_INLINE PxU32 FAllGrtrOrEq(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vpickve2gr_w(__lsx_vfcmp_cule_s(b, a), 0);
}

PX_FORCE_INLINE PxU32 FAllEq(const FloatV a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vpickve2gr_w(__lsx_vfcmp_cueq_s(a, b), 0);
}

PX_FORCE_INLINE FloatV FRound(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);
	const FloatV half = FLoad(0.5f);
	const __m128 singBit = __lsx_vffint_s_w(__lsx_vsrli_w(__lsx_vftint_w_s(a), 31));
	const FloatV aRound = FSub(FAdd(a, half), singBit);
	__m128i tmp = __lsx_vftint_w_s(aRound);
	return __lsx_vffint_s_w(tmp);
}

PX_FORCE_INLINE FloatV FSin(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);

	// Modulo the range of the given angles such that -XM_2PI <= Angles < XM_2PI
	const FloatV recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const FloatV twoPi = V4LoadA(g_PXTwoPi.f);
	const FloatV tmp = FMul(a, recipTwoPi);
	const FloatV b = FRound(tmp);
	const FloatV V1 = FNegScaleSub(twoPi, b, a);

	// sin(V) ~= V - V^3 / 3! + V^5 / 5! - V^7 / 7! + V^9 / 9! - V^11 / 11! + V^13 / 13! -
	//			 V^15 / 15! + V^17 / 17! - V^19 / 19! + V^21 / 21! - V^23 / 23! (for -PI <= V < PI)
	const FloatV V2 = FMul(V1, V1);
	const FloatV V3 = FMul(V2, V1);
	const FloatV V5 = FMul(V3, V2);
	const FloatV V7 = FMul(V5, V2);
	const FloatV V9 = FMul(V7, V2);
	const FloatV V11 = FMul(V9, V2);
	const FloatV V13 = FMul(V11, V2);
	const FloatV V15 = FMul(V13, V2);
	const FloatV V17 = FMul(V15, V2);
	const FloatV V19 = FMul(V17, V2);
	const FloatV V21 = FMul(V19, V2);
	const FloatV V23 = FMul(V21, V2);

	const Vec4V sinCoefficients0 = V4LoadA(g_PXSinCoefficients0.f);
	const Vec4V sinCoefficients1 = V4LoadA(g_PXSinCoefficients1.f);
	const Vec4V sinCoefficients2 = V4LoadA(g_PXSinCoefficients2.f);

	const FloatV S1 = V4GetY(sinCoefficients0);
	const FloatV S2 = V4GetZ(sinCoefficients0);
	const FloatV S3 = V4GetW(sinCoefficients0);
	const FloatV S4 = V4GetX(sinCoefficients1);
	const FloatV S5 = V4GetY(sinCoefficients1);
	const FloatV S6 = V4GetZ(sinCoefficients1);
	const FloatV S7 = V4GetW(sinCoefficients1);
	const FloatV S8 = V4GetX(sinCoefficients2);
	const FloatV S9 = V4GetY(sinCoefficients2);
	const FloatV S10 = V4GetZ(sinCoefficients2);
	const FloatV S11 = V4GetW(sinCoefficients2);

	FloatV Result;
	Result = FScaleAdd(S1, V3, V1);
	Result = FScaleAdd(S2, V5, Result);
	Result = FScaleAdd(S3, V7, Result);
	Result = FScaleAdd(S4, V9, Result);
	Result = FScaleAdd(S5, V11, Result);
	Result = FScaleAdd(S6, V13, Result);
	Result = FScaleAdd(S7, V15, Result);
	Result = FScaleAdd(S8, V17, Result);
	Result = FScaleAdd(S9, V19, Result);
	Result = FScaleAdd(S10, V21, Result);
	Result = FScaleAdd(S11, V23, Result);

	return Result;
}

PX_FORCE_INLINE FloatV FCos(const FloatV a)
{
	ASSERT_ISVALIDFLOATV(a);

	// Modulo the range of the given angles such that -XM_2PI <= Angles < XM_2PI
	const FloatV recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const FloatV twoPi = V4LoadA(g_PXTwoPi.f);
	const FloatV tmp = FMul(a, recipTwoPi);
	const FloatV b = FRound(tmp);
	const FloatV V1 = FNegScaleSub(twoPi, b, a);

	// cos(V) ~= 1 - V^2 / 2! + V^4 / 4! - V^6 / 6! + V^8 / 8! - V^10 / 10! + V^12 / 12! -
	//           V^14 / 14! + V^16 / 16! - V^18 / 18! + V^20 / 20! - V^22 / 22! (for -PI <= V < PI)
	const FloatV V2 = FMul(V1, V1);
	const FloatV V4 = FMul(V2, V2);
	const FloatV V6 = FMul(V4, V2);
	const FloatV V8 = FMul(V4, V4);
	const FloatV V10 = FMul(V6, V4);
	const FloatV V12 = FMul(V6, V6);
	const FloatV V14 = FMul(V8, V6);
	const FloatV V16 = FMul(V8, V8);
	const FloatV V18 = FMul(V10, V8);
	const FloatV V20 = FMul(V10, V10);
	const FloatV V22 = FMul(V12, V10);

	const Vec4V cosCoefficients0 = V4LoadA(g_PXCosCoefficients0.f);
	const Vec4V cosCoefficients1 = V4LoadA(g_PXCosCoefficients1.f);
	const Vec4V cosCoefficients2 = V4LoadA(g_PXCosCoefficients2.f);

	const FloatV C1 = V4GetY(cosCoefficients0);
	const FloatV C2 = V4GetZ(cosCoefficients0);
	const FloatV C3 = V4GetW(cosCoefficients0);
	const FloatV C4 = V4GetX(cosCoefficients1);
	const FloatV C5 = V4GetY(cosCoefficients1);
	const FloatV C6 = V4GetZ(cosCoefficients1);
	const FloatV C7 = V4GetW(cosCoefficients1);
	const FloatV C8 = V4GetX(cosCoefficients2);
	const FloatV C9 = V4GetY(cosCoefficients2);
	const FloatV C10 = V4GetZ(cosCoefficients2);
	const FloatV C11 = V4GetW(cosCoefficients2);

	FloatV Result;
	Result = FScaleAdd(C1, V2, V4One());
	Result = FScaleAdd(C2, V4, Result);
	Result = FScaleAdd(C3, V6, Result);
	Result = FScaleAdd(C4, V8, Result);
	Result = FScaleAdd(C5, V10, Result);
	Result = FScaleAdd(C6, V12, Result);
	Result = FScaleAdd(C7, V14, Result);
	Result = FScaleAdd(C8, V16, Result);
	Result = FScaleAdd(C9, V18, Result);
	Result = FScaleAdd(C10, V20, Result);
	Result = FScaleAdd(C11, V22, Result);

	return Result;
}

PX_FORCE_INLINE PxU32 FOutOfBounds(const FloatV a, const FloatV min, const FloatV max)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(min);
	ASSERT_ISVALIDFLOATV(max);
	const BoolV c = BOr(FIsGrtr(a, max), FIsGrtr(min, a));
	return !BAllEqFFFF(c);
}

PX_FORCE_INLINE PxU32 FInBounds(const FloatV a, const FloatV min, const FloatV max)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(min);
	ASSERT_ISVALIDFLOATV(max)
	const BoolV c = BAnd(FIsGrtrOrEq(a, min), FIsGrtrOrEq(max, a));
	return BAllEqTTTT(c);
}

PX_FORCE_INLINE PxU32 FOutOfBounds(const FloatV a, const FloatV bounds)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(bounds);
	return FOutOfBounds(a, FNeg(bounds), bounds);
}

PX_FORCE_INLINE PxU32 FInBounds(const FloatV a, const FloatV bounds)
{
	ASSERT_ISVALIDFLOATV(a);
	ASSERT_ISVALIDFLOATV(bounds);
	return FInBounds(a, FNeg(bounds), bounds);
}

//////////////////////////////////
// VEC3V
//////////////////////////////////

PX_FORCE_INLINE Vec3V V3Splat(const FloatV f) // 1.0, 0, 0, 0
{
	ASSERT_ISVALIDFLOATV(f);
	v4i32 mask = {-1, -1, -1, 0};
	__m128i tmp = __lsx_vpermi_w(f, f, 0x00);
	return (__m128)__lsx_vand_v(tmp, (__m128i)mask);
}

PX_FORCE_INLINE Vec3V V3Merge(const FloatVArg x, const FloatVArg y, const FloatVArg z)
{
	ASSERT_ISVALIDFLOATV(x);//1
	ASSERT_ISVALIDFLOATV(y);//2
	ASSERT_ISVALIDFLOATV(z);//3
	__m128i zero = {0};
	__m128i xy = __lsx_vilvl_w((__m128i)y, (__m128i)x);
	__m128i z0 = __lsx_vilvl_w(zero, (__m128i)z);
	return (__m128)__lsx_vilvl_d(z0, xy);
}

PX_FORCE_INLINE Vec3V V3UnitX()
{
	__m128 x = { 1.0f, 0.0f, 0.0f, 0.0f };
	return x;
}

PX_FORCE_INLINE Vec3V V3UnitY()
{
	__m128 y = { 0.0f, 1.0f, 0.0f, 0.0f };
	return y;
}

PX_FORCE_INLINE Vec3V V3UnitZ()
{
	__m128 z = { 0.0f, 0.0f, 1.0f, 0.0f };
	return z;
}

PX_FORCE_INLINE FloatV V3GetX(const Vec3V f)
{
	ASSERT_ISVALIDVEC3V(f);
	return f;
}

PX_FORCE_INLINE FloatV V3GetY(const Vec3V f)
{
	ASSERT_ISVALIDVEC3V(f)
	return (__m128)__lsx_vreplvei_w((__m128i)f, 1);
}

PX_FORCE_INLINE FloatV V3GetZ(const Vec3V f)
{
	ASSERT_ISVALIDVEC3V(f);
	return (__m128)__lsx_vreplvei_w((__m128i)f, 2);
}

PX_FORCE_INLINE Vec3V V3SetX(const Vec3V v, const FloatV f)
{
	ASSERT_ISVALIDVEC3V(v);
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BFTTT(), v, f);
}

PX_FORCE_INLINE Vec3V V3SetY(const Vec3V v, const FloatV f)
{
	ASSERT_ISVALIDVEC3V(v);
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BTFTT(), v, f);
}

PX_FORCE_INLINE Vec3V V3SetZ(const Vec3V v, const FloatV f)
{
	ASSERT_ISVALIDVEC3V(v);
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BTTFT(), v, f);
}

PX_FORCE_INLINE Vec3V V3ColX(const Vec3V a, const Vec3V b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	ASSERT_ISVALIDVEC3V(c);
	Vec3V r = __lsx_vpermi_w(c, a, 0xcc);
	return V3SetY(r, V3GetX(b));
}

PX_FORCE_INLINE Vec3V V3ColY(const Vec3V a, const Vec3V b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	ASSERT_ISVALIDVEC3V(c)
	Vec3V r = __lsx_vpermi_w(c, a, 0xdd);
	return V3SetY(r, V3GetY(b));
}

PX_FORCE_INLINE Vec3V V3ColZ(const Vec3V a, const Vec3V b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	ASSERT_ISVALIDVEC3V(c);
	Vec3V r = __lsx_vpermi_w(c, a, 0xee);
	return V3SetY(r, V3GetZ(b));
}

PX_FORCE_INLINE Vec3V V3Zero()
{
	return V3Load(0.0f);
}

PX_FORCE_INLINE Vec3V V3Eps()
{
	return V3Load(PX_EPS_REAL);
}
PX_FORCE_INLINE Vec3V V3One()
{
	return V3Load(1.0f);
}

PX_FORCE_INLINE Vec3V V3Neg(const Vec3V f)
{
	ASSERT_ISVALIDVEC3V(f);
	return (__m128)__lsx_vbitrevi_w((__m128i)f, 31);
}

PX_FORCE_INLINE Vec3V V3Add(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfadd_s(a, b);
}

PX_FORCE_INLINE Vec3V V3Sub(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfsub_s(a, b);
}

PX_FORCE_INLINE Vec3V V3Scale(const Vec3V a, const FloatV b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmul_s(a, b);
}

PX_FORCE_INLINE Vec3V V3Mul(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfmul_s(a, b);
}

PX_FORCE_INLINE Vec3V V3ScaleInv(const Vec3V a, const FloatV b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfdiv_s(a, b);
}

PX_FORCE_INLINE Vec3V V3Div(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return V4ClearW(__lsx_vfdiv_s(a, b));
}

PX_FORCE_INLINE Vec3V V3ScaleInvFast(const Vec3V a, const FloatV b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmul_s(a, __lsx_vfrecip_s(b));
}

PX_FORCE_INLINE Vec3V V3DivFast(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return V4ClearW(__lsx_vfmul_s(a, __lsx_vfrecip_s(b)));
}

PX_FORCE_INLINE Vec3V V3Recip(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V4ClearW(__lsx_vfrecip_s(a));
}

PX_FORCE_INLINE Vec3V V3RecipFast(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V4ClearW(__lsx_vfrecip_s(a));
}

PX_FORCE_INLINE Vec3V V3Rsqrt(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V4ClearW(__lsx_vfrsqrt_s(a));
}

PX_FORCE_INLINE Vec3V V3RsqrtFast(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V4ClearW(__lsx_vfrsqrt_s(a));
}

PX_FORCE_INLINE Vec3V V3ScaleAdd(const Vec3V a, const FloatV b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDFLOATV(b);
	ASSERT_ISVALIDVEC3V(c);
	return V3Add(V3Scale(a, b), c);
}

PX_FORCE_INLINE Vec3V V3NegScaleSub(const Vec3V a, const FloatV b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDFLOATV(b);
	ASSERT_ISVALIDVEC3V(c);
	return V3Sub(c, V3Scale(a, b));
}

PX_FORCE_INLINE Vec3V V3MulAdd(const Vec3V a, const Vec3V b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	ASSERT_ISVALIDVEC3V(c);
	return V3Add(V3Mul(a, b), c);
}

PX_FORCE_INLINE Vec3V V3NegMulSub(const Vec3V a, const Vec3V b, const Vec3V c)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	ASSERT_ISVALIDVEC3V(c);
	return V3Sub(c, V3Mul(a, b));
}

PX_FORCE_INLINE Vec3V V3Abs(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V3Max(a, V3Neg(a));
}

PX_FORCE_INLINE FloatV V3Dot(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	const __m128 t0 = __lsx_vfmul_s(a, b);										//	aw*bw | az*bz | ay*by | ax*bx
	const __m128 t1 = (__m128)__lsx_vpermi_w((__m128i)t0, (__m128i)t0, 0x4e);
	const __m128 t2 = __lsx_vfadd_s(t0, t1);									//	ay*by + aw*bw | ax*bx + az*bz | aw*bw + ay*by | az*bz + ax*bx
	const __m128 t3 = (__m128)__lsx_vpermi_w((__m128i)t2, (__m128i)t2, 0xb1);	//	ax*bx + az*bz | ay*by + aw*bw | az*bz + ax*bx | aw*bw + ay*by
	return __lsx_vfadd_s(t3, t2);												//	ax*bx + az*bz + ay*by + aw*bw
}

PX_FORCE_INLINE Vec3V V3Cross(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	const __m128 r1 = __lsx_vpermi_w(a, a, 0xd2); // z,x,y,w
	const __m128 r2 = __lsx_vpermi_w(b, b, 0xc9); // y,z,x,w
	const __m128 l1 = __lsx_vpermi_w(a, a, 0xc9); // y,z,x,w
	const __m128 l2 = __lsx_vpermi_w(b, b, 0xd2); // z,x,y,w
	return __lsx_vfsub_s(__lsx_vfmul_s(l1, l2), __lsx_vfmul_s(r1, r2));
}

PX_FORCE_INLINE VecCrossV V3PrepareCross(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	VecCrossV v;
	v.mR1 = __lsx_vpermi_w(a, a, 0xd2); // z,x,y,w
	v.mL1 = __lsx_vpermi_w(a, a, 0xc9); // y,z,x,w
	return v;
}

PX_FORCE_INLINE Vec3V V3Cross(const VecCrossV& a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(b);
	const __m128 r2 = __lsx_vpermi_w(b, b, 0xc9); // y,z,x,w
	const __m128 l2 = __lsx_vpermi_w(b, b, 0xd2); // z,x,y,w
	return __lsx_vfsub_s(__lsx_vfmul_s(a.mL1, l2), __lsx_vfmul_s(a.mR1, r2));
}

PX_FORCE_INLINE Vec3V V3Cross(const Vec3V a, const VecCrossV& b)
{
	ASSERT_ISVALIDVEC3V(a);
	const __m128 r2 = __lsx_vpermi_w(a, a, 0xc9); // y,z,x,w
	const __m128 l2 = __lsx_vpermi_w(a, a, 0xd2); // z,x,y,w
	return __lsx_vfsub_s(__lsx_vfmul_s(b.mR1, r2), __lsx_vfmul_s(b.mL1, l2));
}

PX_FORCE_INLINE Vec3V V3Cross(const VecCrossV& a, const VecCrossV& b)
{
	return __lsx_vfsub_s(__lsx_vfmul_s(a.mL1, b.mR1), __lsx_vfmul_s(a.mR1, b.mL1));
}

PX_FORCE_INLINE FloatV V3Length(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vfsqrt_s(V3Dot(a, a));
}

PX_FORCE_INLINE FloatV V3LengthSq(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return V3Dot(a, a);
}

PX_FORCE_INLINE Vec3V V3Normalize(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISFINITELENGTH(a);
	return V3ScaleInv(a, __lsx_vfsqrt_s(V3Dot(a, a)));
}

PX_FORCE_INLINE Vec3V V3NormalizeFast(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISFINITELENGTH(a);
	return V3Scale(a, __lsx_vfrsqrt_s(V3Dot(a, a)));
}

PX_FORCE_INLINE Vec3V V3NormalizeSafe(const Vec3V a, const Vec3V unsafeReturnValue)
{
	ASSERT_ISVALIDVEC3V(a);
	const __m128 eps = V3Eps();
	const __m128 length = V3Length(a);
	const __m128 isGreaterThanZero = FIsGrtr(length, eps);
	return V3Sel(isGreaterThanZero, V3ScaleInv(a, length), unsafeReturnValue);
}

PX_FORCE_INLINE Vec3V V3Sel(const BoolV c, const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(__lsx_vbitsel_v(b, a, c));
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE BoolV V3IsGrtr(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfcmp_cult_s(b, a);
}

PX_FORCE_INLINE BoolV V3IsGrtrOrEq(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfcmp_cule_s(b, a);
}

PX_FORCE_INLINE BoolV V3IsEq(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfcmp_cueq_s(b, a);
}

PX_FORCE_INLINE Vec3V V3Max(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfmax_s(a, b);
}

PX_FORCE_INLINE Vec3V V3Min(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return __lsx_vfmin_s(a, b);
}

PX_FORCE_INLINE FloatV V3ExtractMax(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	const __m128 shuf1 = __lsx_vpermi_w(a, a, 0x55);
	const __m128 shuf2 = __lsx_vpermi_w(a, a, 0xaa);
	return __lsx_vfmax_s(__lsx_vfmax_s(shuf1, shuf2), a);
}

PX_FORCE_INLINE FloatV V3ExtractMin(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);

	const __m128 shuf1 = __lsx_vpermi_w(a, a, 0x55);
	const __m128 shuf2 = __lsx_vpermi_w(a, a, 0xaa);
	return __lsx_vfmin_s(__lsx_vfmin_s(shuf1, shuf2), a);
}

// return (a >= 0.0f) ? 1.0f : -1.0f;
PX_FORCE_INLINE Vec3V V3Sign(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	const __m128 zero = V3Zero();
	const __m128 one = V3One();
	const __m128 none = V3Neg(one);
	return V3Sel(V3IsGrtrOrEq(a, zero), one, none);
}

PX_FORCE_INLINE Vec3V V3Clamp(const Vec3V a, const Vec3V minV, const Vec3V maxV)
{
	ASSERT_ISVALIDVEC3V(maxV);
	ASSERT_ISVALIDVEC3V(minV);
	return V3Max(V3Min(a, maxV), minV);
}

PX_FORCE_INLINE PxU32 V3AllGrtr(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return internalUnitLSXSimd::BAllTrue3_R(V4IsGrtr(a, b));
}

PX_FORCE_INLINE PxU32 V3AllGrtrOrEq(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return internalUnitLSXSimd::BAllTrue3_R(V4IsGrtrOrEq(a, b));
}

PX_FORCE_INLINE PxU32 V3AllEq(const Vec3V a, const Vec3V b)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(b);
	return internalUnitLSXSimd::BAllTrue3_R(V4IsEq(a, b));
}

PX_FORCE_INLINE Vec3V V3Round(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	const Vec3V half = V3Load(0.5f);
	const __m128 signBit = __lsx_vffint_s_w(__lsx_vsrli_w(__lsx_vftint_w_s(a), 31));
	const Vec3V aRound = V3Sub(V3Add(a, half), signBit);
	__m128i tmp = __lsx_vftint_w_s(aRound);
	return __lsx_vffint_s_w(tmp);
}

PX_FORCE_INLINE Vec3V V3Sin(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	// Modulo the range of the given angles such that -XM_2PI <= Angles < XM_2PI
	const Vec4V recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const Vec4V twoPi = V4LoadA(g_PXTwoPi.f);
	const Vec3V tmp = V3Scale(a, recipTwoPi);
	const Vec3V b = V3Round(tmp);
	const Vec3V V1 = V3NegScaleSub(b, twoPi, a);

	// sin(V) ~= V - V^3 / 3! + V^5 / 5! - V^7 / 7! + V^9 / 9! - V^11 / 11! + V^13 / 13! -
	//           V^15 / 15! + V^17 / 17! - V^19 / 19! + V^21 / 21! - V^23 / 23! (for -PI <= V < PI)
	const Vec3V V2 = V3Mul(V1, V1);
	const Vec3V V3 = V3Mul(V2, V1);
	const Vec3V V5 = V3Mul(V3, V2);
	const Vec3V V7 = V3Mul(V5, V2);
	const Vec3V V9 = V3Mul(V7, V2);
	const Vec3V V11 = V3Mul(V9, V2);
	const Vec3V V13 = V3Mul(V11, V2);
	const Vec3V V15 = V3Mul(V13, V2);
	const Vec3V V17 = V3Mul(V15, V2);
	const Vec3V V19 = V3Mul(V17, V2);
	const Vec3V V21 = V3Mul(V19, V2);
	const Vec3V V23 = V3Mul(V21, V2);

	const Vec4V sinCoefficients0 = V4LoadA(g_PXSinCoefficients0.f);
	const Vec4V sinCoefficients1 = V4LoadA(g_PXSinCoefficients1.f);
	const Vec4V sinCoefficients2 = V4LoadA(g_PXSinCoefficients2.f);

	const FloatV S1 = V4GetY(sinCoefficients0);
	const FloatV S2 = V4GetZ(sinCoefficients0);
	const FloatV S3 = V4GetW(sinCoefficients0);
	const FloatV S4 = V4GetX(sinCoefficients1);
	const FloatV S5 = V4GetY(sinCoefficients1);
	const FloatV S6 = V4GetZ(sinCoefficients1);
	const FloatV S7 = V4GetW(sinCoefficients1);
	const FloatV S8 = V4GetX(sinCoefficients2);
	const FloatV S9 = V4GetY(sinCoefficients2);
	const FloatV S10 = V4GetZ(sinCoefficients2);
	const FloatV S11 = V4GetW(sinCoefficients2);

	Vec3V Result;
	Result = V3ScaleAdd(V3, S1, V1);
	Result = V3ScaleAdd(V5, S2, Result);
	Result = V3ScaleAdd(V7, S3, Result);
	Result = V3ScaleAdd(V9, S4, Result);
	Result = V3ScaleAdd(V11, S5, Result);
	Result = V3ScaleAdd(V13, S6, Result);
	Result = V3ScaleAdd(V15, S7, Result);
	Result = V3ScaleAdd(V17, S8, Result);
	Result = V3ScaleAdd(V19, S9, Result);
	Result = V3ScaleAdd(V21, S10, Result);
	Result = V3ScaleAdd(V23, S11, Result);

	ASSERT_ISVALIDVEC3V(Result);
	return Result;
}

PX_FORCE_INLINE Vec3V V3Cos(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);

	// Modulo the range of the given angles such that -XM_2PI <= Angles < XM_2PI
	const Vec4V recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const Vec4V twoPi = V4LoadA(g_PXTwoPi.f);
	const Vec3V tmp = V3Scale(a, recipTwoPi);
	const Vec3V b = V3Round(tmp);
	const Vec3V V1 = V3NegScaleSub(b, twoPi, a);

	// cos(V) ~= 1 - V^2 / 2! + V^4 / 4! - V^6 / 6! + V^8 / 8! - V^10 / 10! + V^12 / 12! -
	//           V^14 / 14! + V^16 / 16! - V^18 / 18! + V^20 / 20! - V^22 / 22! (for -PI <= V < PI)
	const Vec3V V2 = V3Mul(V1, V1);
	const Vec3V V4 = V3Mul(V2, V2);
	const Vec3V V6 = V3Mul(V4, V2);
	const Vec3V V8 = V3Mul(V4, V4);
	const Vec3V V10 = V3Mul(V6, V4);
	const Vec3V V12 = V3Mul(V6, V6);
	const Vec3V V14 = V3Mul(V8, V6);
	const Vec3V V16 = V3Mul(V8, V8);
	const Vec3V V18 = V3Mul(V10, V8);
	const Vec3V V20 = V3Mul(V10, V10);
	const Vec3V V22 = V3Mul(V12, V10);

	const Vec4V cosCoefficients0 = V4LoadA(g_PXCosCoefficients0.f);
	const Vec4V cosCoefficients1 = V4LoadA(g_PXCosCoefficients1.f);
	const Vec4V cosCoefficients2 = V4LoadA(g_PXCosCoefficients2.f);

	const FloatV C1 = V4GetY(cosCoefficients0);
	const FloatV C2 = V4GetZ(cosCoefficients0);
	const FloatV C3 = V4GetW(cosCoefficients0);
	const FloatV C4 = V4GetX(cosCoefficients1);
	const FloatV C5 = V4GetY(cosCoefficients1);
	const FloatV C6 = V4GetZ(cosCoefficients1);
	const FloatV C7 = V4GetW(cosCoefficients1);
	const FloatV C8 = V4GetX(cosCoefficients2);
	const FloatV C9 = V4GetY(cosCoefficients2);
	const FloatV C10 = V4GetZ(cosCoefficients2);
	const FloatV C11 = V4GetW(cosCoefficients2);

	Vec3V Result;
	Result = V3ScaleAdd(V2, C1, V3One());
	Result = V3ScaleAdd(V4, C2, Result);
	Result = V3ScaleAdd(V6, C3, Result);
	Result = V3ScaleAdd(V8, C4, Result);
	Result = V3ScaleAdd(V10, C5, Result);
	Result = V3ScaleAdd(V12, C6, Result);
	Result = V3ScaleAdd(V14, C7, Result);
	Result = V3ScaleAdd(V16, C8, Result);
	Result = V3ScaleAdd(V18, C9, Result);
	Result = V3ScaleAdd(V20, C10, Result);
	Result = V3ScaleAdd(V22, C11, Result);

	ASSERT_ISVALIDVEC3V(Result);
	return Result;
}

PX_FORCE_INLINE Vec3V V3PermYZZ(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xe9);
}

PX_FORCE_INLINE Vec3V V3PermXYX(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xc4);
}

PX_FORCE_INLINE Vec3V V3PermYZX(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xc9);
}

PX_FORCE_INLINE Vec3V V3PermZXY(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xd2);
}

PX_FORCE_INLINE Vec3V V3PermZZY(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xda);
}

PX_FORCE_INLINE Vec3V V3PermYXX(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	return __lsx_vpermi_w(a, a, 0xc1);
}

PX_FORCE_INLINE Vec3V V3Perm_Zero_1Z_0Y(const Vec3V v0, const Vec3V v1)
{
	ASSERT_ISVALIDVEC3V(v0);
	ASSERT_ISVALIDVEC3V(v1);
	return __lsx_vpermi_w(v0, v1, 0xdb);
}

PX_FORCE_INLINE Vec3V V3Perm_0Z_Zero_1X(const Vec3V v0, const Vec3V v1)
{
	ASSERT_ISVALIDVEC3V(v0);
	ASSERT_ISVALIDVEC3V(v1);
	return __lsx_vpermi_w(v0, v1, 0xce);
}

PX_FORCE_INLINE Vec3V V3Perm_1Y_0X_Zero(const Vec3V v0, const Vec3V v1)
{
	ASSERT_ISVALIDVEC3V(v0);
	ASSERT_ISVALIDVEC3V(v1);
	__m128i zero = {0};
	__m128 tmp = __lsx_vilvl_d(v1, v0); //y1x1y0x0
	return __lsx_vpermi_w(zero, tmp, 0x03); //0 0 x0 y1
}

PX_FORCE_INLINE FloatV V3SumElems(const Vec3V a)
{
	ASSERT_ISVALIDVEC3V(a);
	__m128 y = __lsx_vbsrl_v(a, 4);
	__m128 z = __lsx_vbsrl_v(a, 8);
	return __lsx_vfadd_s(__lsx_vfadd_s(y, z), a);
}

PX_FORCE_INLINE PxU32 V3OutOfBounds(const Vec3V a, const Vec3V min, const Vec3V max)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(min);
	ASSERT_ISVALIDVEC3V(max);
	const BoolV c = BOr(V3IsGrtr(a, max), V3IsGrtr(min, a));
	return !BAllEqFFFF(c);
}

PX_FORCE_INLINE PxU32 V3InBounds(const Vec3V a, const Vec3V min, const Vec3V max)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(min);
	ASSERT_ISVALIDVEC3V(max);
	const BoolV c = BAnd(V3IsGrtrOrEq(a, min), V3IsGrtrOrEq(max, a));
	return BAllEqTTTT(c);
}

PX_FORCE_INLINE PxU32 V3OutOfBounds(const Vec3V a, const Vec3V bounds)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(bounds);
	return V3OutOfBounds(a, V3Neg(bounds), bounds);
}

PX_FORCE_INLINE PxU32 V3InBounds(const Vec3V a, const Vec3V bounds)
{
	ASSERT_ISVALIDVEC3V(a);
	ASSERT_ISVALIDVEC3V(bounds)
	return V3InBounds(a, V3Neg(bounds), bounds);
}

PX_FORCE_INLINE void V3Transpose(Vec3V& col0, Vec3V& col1, Vec3V& col2)
{
	ASSERT_ISVALIDVEC3V(col0);
	ASSERT_ISVALIDVEC3V(col1);
	ASSERT_ISVALIDVEC3V(col2);

	const Vec3V col3 = {0.0f, 0.0f, 0.0f, 0.0f};
	Vec3V tmp0 = __lsx_vilvl_w(col1, col0);
	Vec3V tmp2 = __lsx_vilvl_w(col3, col2);
	Vec3V tmp1 = __lsx_vilvh_w(col1, col0);
	Vec3V tmp3 = __lsx_vilvh_w(col3, col2);
	col0 = __lsx_vilvl_d(tmp2, tmp0);
	col1 = __lsx_vilvh_d(tmp2, tmp0);
	col2 = __lsx_vilvl_d(tmp3, tmp1);
}

//////////////////////////////////
// VEC4V
//////////////////////////////////

PX_FORCE_INLINE Vec4V V4Splat(const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return f;
}

PX_FORCE_INLINE Vec4V V4Merge(const FloatV* const floatVArray)
{
	ASSERT_ISVALIDFLOATV(floatVArray[0]);
	ASSERT_ISVALIDFLOATV(floatVArray[1]);
	ASSERT_ISVALIDFLOATV(floatVArray[2]);
	ASSERT_ISVALIDFLOATV(floatVArray[3]);
	const __m128i xy = __lsx_vilvl_w(floatVArray[1], floatVArray[0]);
	const __m128i zw = __lsx_vilvl_w(floatVArray[3], floatVArray[2]);
	return __lsx_vilvl_d(zw, xy);
}

PX_FORCE_INLINE Vec4V V4Merge(const FloatVArg x, const FloatVArg y, const FloatVArg z, const FloatVArg w)
{
	ASSERT_ISVALIDFLOATV(x);
	ASSERT_ISVALIDFLOATV(y);
	ASSERT_ISVALIDFLOATV(z);
	ASSERT_ISVALIDFLOATV(w);
	const __m128i xy = __lsx_vilvl_w(y, x);
	const __m128i zw = __lsx_vilvl_w(w, z);
	return __lsx_vilvl_d(zw, xy);
}

PX_FORCE_INLINE Vec4V V4MergeW(const Vec4VArg x, const Vec4VArg y, const Vec4VArg z, const Vec4VArg w)
{
	const __m128i xyW = __lsx_vilvh_w(y, x);
	const __m128i zwW = __lsx_vilvh_w(w, z);
	return __lsx_vilvh_d(zwW, xyW);
}

PX_FORCE_INLINE Vec4V V4MergeZ(const Vec4VArg x, const Vec4VArg y, const Vec4VArg z, const Vec4VArg w)
{
	const __m128i xyZ = __lsx_vilvh_w(y, x);
	const __m128i zwZ = __lsx_vilvh_w(w, z);
	return __lsx_vilvl_d(zwZ, xyZ);
}

PX_FORCE_INLINE Vec4V V4MergeY(const Vec4VArg x, const Vec4VArg y, const Vec4VArg z, const Vec4VArg w)
{
	const __m128i xyY = __lsx_vilvl_w(y, x);
	const __m128i zwY = __lsx_vilvl_w(w, z);
	return __lsx_vilvh_d(zwY, xyY);
}

PX_FORCE_INLINE Vec4V V4MergeX(const Vec4VArg x, const Vec4VArg y, const Vec4VArg z, const Vec4VArg w)
{
	const __m128i xyX = __lsx_vilvl_w(y, x);
	const __m128i zwX = __lsx_vilvl_w(w, z);
	return __lsx_vilvl_d(zwX, xyX);
}

PX_FORCE_INLINE Vec4V V4UnpackXY(const Vec4VArg a, const Vec4VArg b)
{
	return __lsx_vilvl_w(b, a);
}

PX_FORCE_INLINE Vec4V V4UnpackZW(const Vec4VArg a, const Vec4VArg b)
{
	return __lsx_vilvh_w(b ,a);
}

PX_FORCE_INLINE Vec4V V4UnitW()
{
	__m128 w = { 0.0f, 0.0f, 0.0f, 1.0f };
	return w;
}

PX_FORCE_INLINE Vec4V V4UnitX()
{
	__m128 x = { 1.0f, 0.0f, 0.0f, 0.0f };
	return x;
}

PX_FORCE_INLINE Vec4V V4UnitY()
{
	__m128 y = { 0.0f, 1.0f, 0.0f, 0.0f };
	return y;
}

PX_FORCE_INLINE Vec4V V4UnitZ()
{
	__m128 z = { 0.0f, 0.0f, 1.0f, 0.0f };
	return z;
}

PX_FORCE_INLINE FloatV V4GetW(const Vec4V f)
{
	return __lsx_vreplvei_w(f, 3);
}

PX_FORCE_INLINE FloatV V4GetX(const Vec4V f)
{
	return __lsx_vreplvei_w(f, 0);
}

PX_FORCE_INLINE FloatV V4GetY(const Vec4V f)
{
	return __lsx_vreplvei_w(f, 1);
}

PX_FORCE_INLINE FloatV V4GetZ(const Vec4V f)
{
	return __lsx_vreplvei_w(f, 2);
}

PX_FORCE_INLINE Vec4V V4SetW(const Vec4V v, const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BTTTF(), v, f);
}

PX_FORCE_INLINE Vec4V V4SetX(const Vec4V v, const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BFTTT(), v, f);
}

PX_FORCE_INLINE Vec4V V4SetY(const Vec4V v, const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BTFTT(), v, f);
}

PX_FORCE_INLINE Vec4V V4SetZ(const Vec4V v, const FloatV f)
{
	ASSERT_ISVALIDFLOATV(f);
	return V4Sel(BTTFT(), v, f);
}

PX_FORCE_INLINE Vec4V V4ClearW(const Vec4V v)
{
	v4i32 mask = {-1, -1, -1, 0};
	return (__m128)__lsx_vand_v((__m128i)v, (__m128i)mask);
}

PX_FORCE_INLINE Vec4V V4PermYXWZ(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, 0xb1);
}

PX_FORCE_INLINE Vec4V V4PermXZXZ(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, 0x88);
}

PX_FORCE_INLINE Vec4V V4PermYWYW(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, 0xdd);
}

PX_FORCE_INLINE Vec4V V4PermYZXW(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, 0xc9);
}

PX_FORCE_INLINE Vec4V V4PermZWXY(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, 0x8e);
}

template <PxU8 x, PxU8 y, PxU8 z, PxU8 w>
PX_FORCE_INLINE Vec4V V4Perm(const Vec4V a)
{
	return __lsx_vpermi_w(a, a, (w << 6 | z << 4 | y << 2 | x));
}

PX_FORCE_INLINE Vec4V V4Zero()
{
	return V4Load(0.0f);
}

PX_FORCE_INLINE Vec4V V4One()
{
	return V4Load(1.0f);
}

PX_FORCE_INLINE Vec4V V4Eps()
{
	return V4Load(PX_EPS_REAL);
}

PX_FORCE_INLINE Vec4V V4Neg(const Vec4V f)
{
	return __lsx_vbitrevi_w(f, 31);
}

PX_FORCE_INLINE Vec4V V4Add(const Vec4V a, const Vec4V b)
{
	return __lsx_vfadd_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Sub(const Vec4V a, const Vec4V b)
{
	return __lsx_vfsub_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Scale(const Vec4V a, const FloatV b)
{
	return __lsx_vfmul_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Mul(const Vec4V a, const Vec4V b)
{
	return __lsx_vfmul_s(a, b);
}

PX_FORCE_INLINE Vec4V V4ScaleInv(const Vec4V a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfdiv_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Div(const Vec4V a, const Vec4V b)
{
	return __lsx_vfdiv_s(a, b);
}

PX_FORCE_INLINE Vec4V V4ScaleInvFast(const Vec4V a, const FloatV b)
{
	ASSERT_ISVALIDFLOATV(b);
	return __lsx_vfmul_s(a, __lsx_vfrecip_s(b));
}

PX_FORCE_INLINE Vec4V V4DivFast(const Vec4V a, const Vec4V b)
{
	return __lsx_vfmul_s(a, __lsx_vfrecip_s(b));
}

PX_FORCE_INLINE Vec4V V4Recip(const Vec4V a)
{
	return __lsx_vfdiv_s(V4One(), a);
}

PX_FORCE_INLINE Vec4V V4RecipFast(const Vec4V a)
{
	return __lsx_vfrecip_s(a);
}

PX_FORCE_INLINE Vec4V V4Rsqrt(const Vec4V a)
{
	return __lsx_vfdiv_s(V4One(), __lsx_vfsqrt_s(a));
}

PX_FORCE_INLINE Vec4V V4RsqrtFast(const Vec4V a)
{
	return __lsx_vfrsqrt_s(a);
}

PX_FORCE_INLINE Vec4V V4Sqrt(const Vec4V a)
{
	return __lsx_vfsqrt_s(a);
}

PX_FORCE_INLINE Vec4V V4ScaleAdd(const Vec4V a, const FloatV b, const Vec4V c)
{
	ASSERT_ISVALIDFLOATV(b);
	return V4Add(V4Scale(a, b), c);
}

PX_FORCE_INLINE Vec4V V4NegScaleSub(const Vec4V a, const FloatV b, const Vec4V c)
{
	ASSERT_ISVALIDFLOATV(b);
	return V4Sub(c, V4Scale(a, b));
}

PX_FORCE_INLINE Vec4V V4MulAdd(const Vec4V a, const Vec4V b, const Vec4V c)
{
	return V4Add(V4Mul(a, b), c);
}

PX_FORCE_INLINE Vec4V V4NegMulSub(const Vec4V a, const Vec4V b, const Vec4V c)
{
	return V4Sub(c, V4Mul(a, b));
}

PX_FORCE_INLINE Vec4V V4Abs(const Vec4V a)
{
	return V4Max(a, V4Neg(a));
}

PX_FORCE_INLINE FloatV V4SumElements(const Vec4V a)
{
	const Vec4V xy = V4UnpackXY(a, a); // x,x,y,y
	const Vec4V zw = V4UnpackZW(a, a); // z,z,w,w
	const Vec4V xz_yw = V4Add(xy, zw); // x+z,x+z,y+w,y+w
	const FloatV xz = V4GetX(xz_yw);   // x+z
	const FloatV yw = V4GetZ(xz_yw);   // y+w
	return FAdd(xz, yw);               // sum
}

PX_FORCE_INLINE FloatV V4Dot(const Vec4V a, const Vec4V b)
{
	__m128 dot1 = __lsx_vfmul_s(a, b);
	__m128 dot2 = __lsx_vbsrl_v(dot1, 8);
	dot1 = __lsx_vfadd_s(dot1, dot2);
	dot2 = __lsx_vbsrl_v(dot1, 4);
	return __lsx_vfadd_s(dot1, dot2);
}

PX_FORCE_INLINE FloatV V4Dot3(const Vec4V a, const Vec4V b)
{
	const __m128 dot1 = __lsx_vfmul_s(a, b);
	const __m128 dot2 = __lsx_vbsrl_v(dot1, 4);
	const __m128 dot3 = __lsx_vbsrl_v(dot1, 8);
	return __lsx_vfadd_s(__lsx_vfadd_s(dot1, dot2), dot3);
}

PX_FORCE_INLINE Vec4V V4Cross(const Vec4V a, const Vec4V b)
{
	const __m128 r1 = __lsx_vpermi_w(a, a, 0xd2); // z,x,y,w
	const __m128 r2 = __lsx_vpermi_w(b, b, 0xc9); // y,z,x,w
	const __m128 l1 = __lsx_vpermi_w(a, a, 0xc9); // y,z,x,w
	const __m128 l2 = __lsx_vpermi_w(b, b, 0xd2); // z,x,y,w
	return __lsx_vfsub_s(__lsx_vfmul_s(l1, l2), __lsx_vfmul_s(r1, r2));
}

PX_FORCE_INLINE FloatV V4Length(const Vec4V a)
{
	return __lsx_vfsqrt_s(V4Dot(a, a));
}

PX_FORCE_INLINE FloatV V4LengthSq(const Vec4V a)
{
	return V4Dot(a, a);
}

PX_FORCE_INLINE Vec4V V4Normalize(const Vec4V a)
{
	ASSERT_ISFINITELENGTH(a);
	return V4ScaleInv(a, __lsx_vfsqrt_s(V4Dot(a, a)));
}

PX_FORCE_INLINE Vec4V V4NormalizeFast(const Vec4V a)
{
	ASSERT_ISFINITELENGTH(a);
	return V4ScaleInvFast(a, __lsx_vfsqrt_s(V4Dot(a, a)));
}

PX_FORCE_INLINE Vec4V V4NormalizeSafe(const Vec4V a, const Vec3V unsafeReturnValue)
{
	const __m128 eps = V3Eps();
	const __m128 length = V4Length(a);
	const __m128 isGreaterThanZero = V4IsGrtr(length, eps);
	return V4Sel(isGreaterThanZero, V4ScaleInv(a, length), unsafeReturnValue);
}

PX_FORCE_INLINE BoolV V4IsEqU32(const VecU32V a, const VecU32V b)
{
	return __lsx_vfcmp_cueq_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Sel(const BoolV c, const Vec4V a, const Vec4V b)
{
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE BoolV V4IsGrtr(const Vec4V a, const Vec4V b)
{
	return __lsx_vfcmp_cult_s(b, a);
}

PX_FORCE_INLINE BoolV V4IsGrtrOrEq(const Vec4V a, const Vec4V b)
{
	return __lsx_vfcmp_cule_s(b, a);
}

PX_FORCE_INLINE BoolV V4IsEq(const Vec4V a, const Vec4V b)
{
	return __lsx_vfcmp_cueq_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Max(const Vec4V a, const Vec4V b)
{
	return __lsx_vfmax_s(a, b);
}

PX_FORCE_INLINE Vec4V V4Min(const Vec4V a, const Vec4V b)
{
	return __lsx_vfmin_s(a, b);
}

PX_FORCE_INLINE FloatV V4ExtractMax(const Vec4V a)
{
	const __m128 shuf1 = __lsx_vbsrl_v(a, 4);
	const __m128 shuf2 = __lsx_vbsrl_v(a, 8);
	const __m128 shuf3 = __lsx_vbsrl_v(a, 12);

	return __lsx_vfmax_s(__lsx_vfmax_s(a, shuf1), __lsx_vfmax_s(shuf2, shuf3));
}

PX_FORCE_INLINE FloatV V4ExtractMin(const Vec4V a)
{
	const __m128 shuf1 = __lsx_vbsrl_v(a, 4);
	const __m128 shuf2 = __lsx_vbsrl_v(a, 8);
	const __m128 shuf3 = __lsx_vbsrl_v(a, 12);

	return __lsx_vfmin_s(__lsx_vfmin_s(a, shuf1), __lsx_vfmin_s(shuf2, shuf3));
}

PX_FORCE_INLINE Vec4V V4Clamp(const Vec4V a, const Vec4V minV, const Vec4V maxV)
{
	return V4Max(V4Min(a, maxV), minV);
}

PX_FORCE_INLINE PxU32 V4AllGrtr(const Vec4V a, const Vec4V b)
{
	return internalUnitLSXSimd::BAllTrue4_R(V4IsGrtr(a, b));
}

PX_FORCE_INLINE PxU32 V4AllGrtrOrEq(const Vec4V a, const Vec4V b)
{
	return internalUnitLSXSimd::BAllTrue4_R(V4IsGrtrOrEq(a, b));
}

PX_FORCE_INLINE PxU32 V4AllGrtrOrEq3(const Vec4V a, const Vec4V b)
{
	return internalUnitLSXSimd::BAllTrue3_R(V4IsGrtrOrEq(a, b));
}

PX_FORCE_INLINE PxU32 V4AllEq(const Vec4V a, const Vec4V b)
{
	return internalUnitLSXSimd::BAllTrue4_R(V4IsEq(a, b));
}

PX_FORCE_INLINE PxU32 V4AnyGrtr3(const Vec4V a, const Vec4V b)
{
	return internalUnitLSXSimd::BAnyTrue3_R(V4IsGrtr(a, b));
}

PX_FORCE_INLINE Vec4V V4Round(const Vec4V a)
{
	const Vec4V half = V4Load(0.5f);
	const __m128 signBit = __lsx_vffint_s_w(__lsx_vsrli_w(__lsx_vftint_w_s(a), 31));
	const Vec4V aRound = V4Sub(V4Add(a, half), signBit);
	__m128i tmp = __lsx_vftint_w_s(aRound);
	return __lsx_vffint_s_w(tmp);
}

PX_FORCE_INLINE Vec4V V4Sin(const Vec4V a)
{
	const Vec4V recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const Vec4V twoPi = V4LoadA(g_PXTwoPi.f);
	const Vec4V tmp = V4Mul(a, recipTwoPi);
	const Vec4V b = V4Round(tmp);
	const Vec4V V1 = V4NegMulSub(twoPi, b, a);

	// sin(V) ~= V - V^3 / 3! + V^5 / 5! - V^7 / 7! + V^9 / 9! - V^11 / 11! + V^13 / 13! -
	//           V^15 / 15! + V^17 / 17! - V^19 / 19! + V^21 / 21! - V^23 / 23! (for -PI <= V < PI)
	const Vec4V V2 = V4Mul(V1, V1);
	const Vec4V V3 = V4Mul(V2, V1);
	const Vec4V V5 = V4Mul(V3, V2);
	const Vec4V V7 = V4Mul(V5, V2);
	const Vec4V V9 = V4Mul(V7, V2);
	const Vec4V V11 = V4Mul(V9, V2);
	const Vec4V V13 = V4Mul(V11, V2);
	const Vec4V V15 = V4Mul(V13, V2);
	const Vec4V V17 = V4Mul(V15, V2);
	const Vec4V V19 = V4Mul(V17, V2);
	const Vec4V V21 = V4Mul(V19, V2);
	const Vec4V V23 = V4Mul(V21, V2);

	const Vec4V sinCoefficients0 = V4LoadA(g_PXSinCoefficients0.f);
	const Vec4V sinCoefficients1 = V4LoadA(g_PXSinCoefficients1.f);
	const Vec4V sinCoefficients2 = V4LoadA(g_PXSinCoefficients2.f);

	const FloatV S1 = V4GetY(sinCoefficients0);
	const FloatV S2 = V4GetZ(sinCoefficients0);
	const FloatV S3 = V4GetW(sinCoefficients0);
	const FloatV S4 = V4GetX(sinCoefficients1);
	const FloatV S5 = V4GetY(sinCoefficients1);
	const FloatV S6 = V4GetZ(sinCoefficients1);
	const FloatV S7 = V4GetW(sinCoefficients1);
	const FloatV S8 = V4GetX(sinCoefficients2);
	const FloatV S9 = V4GetY(sinCoefficients2);
	const FloatV S10 = V4GetZ(sinCoefficients2);
	const FloatV S11 = V4GetW(sinCoefficients2);

	Vec4V Result;
	Result = V4MulAdd(S1, V3, V1);
	Result = V4MulAdd(S2, V5, Result);
	Result = V4MulAdd(S3, V7, Result);
	Result = V4MulAdd(S4, V9, Result);
	Result = V4MulAdd(S5, V11, Result);
	Result = V4MulAdd(S6, V13, Result);
	Result = V4MulAdd(S7, V15, Result);
	Result = V4MulAdd(S8, V17, Result);
	Result = V4MulAdd(S9, V19, Result);
	Result = V4MulAdd(S10, V21, Result);
	Result = V4MulAdd(S11, V23, Result);

	return Result;
}

PX_FORCE_INLINE Vec4V V4Cos(const Vec4V a)
{
	const Vec4V recipTwoPi = V4LoadA(g_PXReciprocalTwoPi.f);
	const Vec4V twoPi = V4LoadA(g_PXTwoPi.f);
	const Vec4V tmp = V4Mul(a, recipTwoPi);
	const Vec4V b = V4Round(tmp);
	const Vec4V V1 = V4NegMulSub(twoPi, b, a);

	// cos(V) ~= 1 - V^2 / 2! + V^4 / 4! - V^6 / 6! + V^8 / 8! - V^10 / 10! + V^12 / 12! -
	//           V^14 / 14! + V^16 / 16! - V^18 / 18! + V^20 / 20! - V^22 / 22! (for -PI <= V < PI)
	const Vec4V V2 = V4Mul(V1, V1);
	const Vec4V V4 = V4Mul(V2, V2);
	const Vec4V V6 = V4Mul(V4, V2);
	const Vec4V V8 = V4Mul(V4, V4);
	const Vec4V V10 = V4Mul(V6, V4);
	const Vec4V V12 = V4Mul(V6, V6);
	const Vec4V V14 = V4Mul(V8, V6);
	const Vec4V V16 = V4Mul(V8, V8);
	const Vec4V V18 = V4Mul(V10, V8);
	const Vec4V V20 = V4Mul(V10, V10);
	const Vec4V V22 = V4Mul(V12, V10);

	const Vec4V cosCoefficients0 = V4LoadA(g_PXCosCoefficients0.f);
	const Vec4V cosCoefficients1 = V4LoadA(g_PXCosCoefficients1.f);
	const Vec4V cosCoefficients2 = V4LoadA(g_PXCosCoefficients2.f);

	const FloatV C1 = V4GetY(cosCoefficients0);
	const FloatV C2 = V4GetZ(cosCoefficients0);
	const FloatV C3 = V4GetW(cosCoefficients0);
	const FloatV C4 = V4GetX(cosCoefficients1);
	const FloatV C5 = V4GetY(cosCoefficients1);
	const FloatV C6 = V4GetZ(cosCoefficients1);
	const FloatV C7 = V4GetW(cosCoefficients1);
	const FloatV C8 = V4GetX(cosCoefficients2);
	const FloatV C9 = V4GetY(cosCoefficients2);
	const FloatV C10 = V4GetZ(cosCoefficients2);
	const FloatV C11 = V4GetW(cosCoefficients2);

	Vec4V Result;
	Result = V4MulAdd(C1, V2, V4One());
	Result = V4MulAdd(C2, V4, Result);
	Result = V4MulAdd(C3, V6, Result);
	Result = V4MulAdd(C4, V8, Result);
	Result = V4MulAdd(C5, V10, Result);
	Result = V4MulAdd(C6, V12, Result);
	Result = V4MulAdd(C7, V14, Result);
	Result = V4MulAdd(C8, V16, Result);
	Result = V4MulAdd(C9, V18, Result);
	Result = V4MulAdd(C10, V20, Result);
	Result = V4MulAdd(C11, V22, Result);

	return Result;
}

PX_FORCE_INLINE void V4Transpose(Vec4V& col0, Vec4V& col1, Vec4V& col2, Vec4V& col3)
{
	Vec4V tmp0 = __lsx_vilvl_w(col1, col0);
	Vec4V tmp2 = __lsx_vilvl_w(col3, col2);
	Vec4V tmp1 = __lsx_vilvh_w(col1, col0);
	Vec4V tmp3 = __lsx_vilvh_w(col3, col2);
	col0 = __lsx_vilvl_d(tmp2, tmp0);
	col1 = __lsx_vilvh_d(tmp2, tmp0);
	col2 = __lsx_vilvl_d(tmp3, tmp1);
	col3 = __lsx_vilvh_d(tmp3, tmp1);
}

//////////////////////////////////
// BoolV
//////////////////////////////////

PX_FORCE_INLINE BoolV BFFFF()
{
	__m128i zero = {0};
	return zero;
}

PX_FORCE_INLINE BoolV BFFFT()
{
	v4i32 ffft = {0, 0, 0, -1};
	return (__m128i)ffft;
}

PX_FORCE_INLINE BoolV BFFTF()
{
	v4i32 fftf = {0, 0, -1, 0};
	return (__m128i)fftf;
}

PX_FORCE_INLINE BoolV BFFTT()
{
	v4i32 fftt = {0, 0, -1, -1};
	return (__m128i)fftt;
}

PX_FORCE_INLINE BoolV BFTFF()
{
	v4i32 ftff = {0, -1, 0, 0};
	return (__m128i)ftff;
}

PX_FORCE_INLINE BoolV BFTFT()
{
	v4i32 ftft = {0, -1, 0, -1};
	return (__m128i)ftft;
}

PX_FORCE_INLINE BoolV BFTTF()
{
	v4i32 fttf = {0, -1, -1, 0};
	return (__m128i)fttf;
}

PX_FORCE_INLINE BoolV BFTTT()
{
	v4i32 fttt = {0, -1, -1, -1};
	return (__m128i)fttt;
}

PX_FORCE_INLINE BoolV BTFFF()
{
	v4i32 tfff = {-1, 0, 0, 0};
	return (__m128i)tfff;
}

PX_FORCE_INLINE BoolV BTFFT()
{
	v4i32 tfft = {-1, 0, 0, -1};
	return (__m128i)tfft;
}

PX_FORCE_INLINE BoolV BTFTF()
{
	v4i32 tftf = {-1, 0, -1, 0};
	return (__m128i)tftf;
}

PX_FORCE_INLINE BoolV BTFTT()
{
	v4i32 tftt = {-1, 0, -1, -1};
	return (__m128i)tftt;
}

PX_FORCE_INLINE BoolV BTTFF()
{
	v4i32 ttff = {-1, -1, 0, 0};
	return (__m128i)ttff;
}

PX_FORCE_INLINE BoolV BTTFT()
{
	v4i32 ttft = {-1, -1, 0, -1};
	return (__m128i)ttft;
}

PX_FORCE_INLINE BoolV BTTTF()
{
	v4i32 tttf = {-1, -1, -1, 0};
	return (__m128i)tttf;
}

PX_FORCE_INLINE BoolV BTTTT()
{
	v4i32 tttt = {-1, -1, -1, -1};
	return (__m128i)tttt;
}

PX_FORCE_INLINE BoolV BXMask()
{
	v4i32 bxmask = {-1, 0, 0, 0};
	return (__m128i)bxmask;
}

PX_FORCE_INLINE BoolV BYMask()
{
	v4i32 bymask = {0, -1, 0, 0};
	return (__m128i)bymask;
}

PX_FORCE_INLINE BoolV BZMask()
{
	v4i32 bzmask = {0, 0, -1, 0};
	return (__m128i)bzmask;
}

PX_FORCE_INLINE BoolV BWMask()
{
	v4i32 bwmask = {0, 0, 0, -1};
	return (__m128i)bwmask;
}

PX_FORCE_INLINE BoolV BGetX(const BoolV f)
{
	return __lsx_vreplvei_w(f, 0);
}

PX_FORCE_INLINE BoolV BGetY(const BoolV f)
{
	return __lsx_vreplvei_w(f, 1);
}

PX_FORCE_INLINE BoolV BGetZ(const BoolV f)
{
	return __lsx_vreplvei_w(f, 2);
}

PX_FORCE_INLINE BoolV BGetW(const BoolV f)
{
	return __lsx_vreplvei_w(f, 3);
}

PX_FORCE_INLINE BoolV BSetX(const BoolV v, const BoolV f)
{
	return V4Sel(BFTTT(), v, f);
}

PX_FORCE_INLINE BoolV BSetY(const BoolV v, const BoolV f)
{
	return V4Sel(BTFTT(), v, f);
}

PX_FORCE_INLINE BoolV BSetZ(const BoolV v, const BoolV f)
{
	return V4Sel(BTTFT(), v, f);
}

PX_FORCE_INLINE BoolV BSetW(const BoolV v, const BoolV f)
{
	return V4Sel(BTTTF(), v, f);
}

PX_FORCE_INLINE BoolV BAnd(const BoolV a, const BoolV b)
{
	return __lsx_vand_v(a, b);
}

PX_FORCE_INLINE BoolV BNot(const BoolV a)
{
	const BoolV bAllTrue(BTTTT());
	return __lsx_vxor_v(a, bAllTrue);
}

PX_FORCE_INLINE BoolV BAndNot(const BoolV a, const BoolV b)
{
	return __lsx_vandn_v(b, a);
}

PX_FORCE_INLINE BoolV BOr(const BoolV a, const BoolV b)
{
	return __lsx_vor_v(a, b);
}

PX_FORCE_INLINE BoolV BAllTrue4(const BoolV a)
{
	const BoolV bTmp = __lsx_vand_v(__lsx_vpermi_w(a, a, 0x11), __lsx_vpermi_w(a, a, 0xbb));
	return __lsx_vand_v(__lsx_vpermi_w(bTmp, bTmp, 0x00), __lsx_vpermi_w(bTmp, bTmp, 0x99));
}

PX_FORCE_INLINE BoolV BAnyTrue4(const BoolV a)
{
	const BoolV bTmp = __lsx_vor_v(__lsx_vpermi_w(a, a, 0x11), __lsx_vpermi_w(a, a, 0xbb));
	return __lsx_vor_v(__lsx_vpermi_w(bTmp, bTmp, 0x00), __lsx_vpermi_w(bTmp, bTmp, 0x99));
}

PX_FORCE_INLINE BoolV BAllTrue3(const BoolV a)
{
	const BoolV bTmp = __lsx_vand_v(__lsx_vpermi_w(a, a, 0x11), __lsx_vpermi_w(a, a, 0xaa));
	return __lsx_vand_v(__lsx_vpermi_w(bTmp, bTmp, 0x00), __lsx_vpermi_w(bTmp, bTmp, 0x99));
}

PX_FORCE_INLINE BoolV BAnyTrue3(const BoolV a)
{
	const BoolV bTmp = __lsx_vor_v(__lsx_vpermi_w(a, a, 0x11), __lsx_vpermi_w(a, a, 0xaa));
	return __lsx_vor_v(__lsx_vpermi_w(bTmp, bTmp, 0x00), __lsx_vpermi_w(bTmp, bTmp, 0x99));
}

PX_FORCE_INLINE PxU32 BAllEq(const BoolV a, const BoolV b)
{
	const BoolV bTest = m128_I2F(__lsx_vfcmp_cueq_s(a, b));
	return internalUnitLSXSimd::BAllTrue4_R(bTest);
}

PX_FORCE_INLINE PxU32 BAllEqTTTT(const BoolV a)
{
	return BAllEq(a, BTTTT());
}

PX_FORCE_INLINE PxU32 BAllEqFFFF(const BoolV a)
{
	return BAllEq(a, BFFFF());
}

PX_FORCE_INLINE PxU32 BGetBitMask(const BoolV a)
{
	v4u32 mask = {1, 2, 4, 8};
	__m128i t0 = __lsx_vand_v((__m128i)a, (__m128i)mask);
	__m128i t1 = __lsx_vbsrl_v(t0, 8);
	t0 = __lsx_vor_v(t0, t1);
	t1 = __lsx_vbsrl_v(t0, 4);
	t0 = __lsx_vor_v(t0, t1);
	return (PxU32)__lsx_vpickve2gr_w(t0, 0);
}

//////////////////////////////////
// MAT33V
//////////////////////////////////

PX_FORCE_INLINE Vec3V M33MulV3(const Mat33V& a, const Vec3V b)
{
	const FloatV x = V3GetX(b);
	const FloatV y = V3GetY(b);
	const FloatV z = V3GetZ(b);
	const Vec3V v0 = V3Scale(a.col0, x);
	const Vec3V v1 = V3Scale(a.col1, y);
	const Vec3V v2 = V3Scale(a.col2, z);
	const Vec3V v0PlusV1 = V3Add(v0, v1);
	return V3Add(v0PlusV1, v2);
}

PX_FORCE_INLINE Vec3V M33TrnspsMulV3(const Mat33V& a, const Vec3V b)
{
	const FloatV x = V3Dot(a.col0, b);
	const FloatV y = V3Dot(a.col1, b);
	const FloatV z = V3Dot(a.col2, b);
	return V3Merge(x, y, z);
}

PX_FORCE_INLINE Vec3V M33MulV3AddV3(const Mat33V& A, const Vec3V b, const Vec3V c)
{
	const FloatV x = V3GetX(b);
	const FloatV y = V3GetY(b);
	const FloatV z = V3GetZ(b);
	Vec3V result = V3ScaleAdd(A.col0, x, c);
	result = V3ScaleAdd(A.col1, y, result);
	return V3ScaleAdd(A.col2, z, result);
}

PX_FORCE_INLINE Mat33V M33MulM33(const Mat33V& a, const Mat33V& b)
{
	return Mat33V(M33MulV3(a, b.col0), M33MulV3(a, b.col1), M33MulV3(a, b.col2));
}

PX_FORCE_INLINE Mat33V M33Add(const Mat33V& a, const Mat33V& b)
{
	return Mat33V(V3Add(a.col0, b.col0), V3Add(a.col1, b.col1), V3Add(a.col2, b.col2));
}

PX_FORCE_INLINE Mat33V M33Scale(const Mat33V& a, const FloatV& b)
{
	return Mat33V(V3Scale(a.col0, b), V3Scale(a.col1, b), V3Scale(a.col2, b));
}

PX_FORCE_INLINE Mat33V M33Inverse(const Mat33V& a)
{
	const BoolV tfft = BTFFT();
	const BoolV tttf = BTTTF();
	const FloatV zero = FZero();
	const Vec3V cross01 = V3Cross(a.col0, a.col1);
	const Vec3V cross12 = V3Cross(a.col1, a.col2);
	const Vec3V cross20 = V3Cross(a.col2, a.col0);
	const FloatV dot = V3Dot(cross01, a.col2);
	const FloatV invDet = __lsx_vfrecip_s(dot);
	const Vec3V mergeh = __lsx_vilvl_w(cross01, cross12);
	const Vec3V mergel = __lsx_vilvh_w(cross01, cross12);
	Vec3V colInv0 = __lsx_vilvl_w(cross20, mergeh);
	colInv0 = __lsx_vor_v(__lsx_vandn_v(tttf, zero), __lsx_vand_v(tttf, colInv0));
	const Vec3V zppd = __lsx_vpermi_w(cross20, mergeh, 0xc2);
	const Vec3V pbwp = __lsx_vpermi_w(mergeh, cross20, 0xf4);
	const Vec3V colInv1 = __lsx_vor_v(__lsx_vandn_v(BTFFT(), pbwp), __lsx_vand_v(BTFFT(), zppd));
	const Vec3V xppd = __lsx_vpermi_w(cross20, mergel, 0xc0);
	const Vec3V pcyp = __lsx_vpermi_w(mergel, cross20, 0xd8);
	const Vec3V colInv2 = __lsx_vor_v(__lsx_vandn_v(tfft, pcyp), __lsx_vand_v(tfft, xppd));

	return Mat33V(__lsx_vfmul_s(colInv0, invDet), __lsx_vfmul_s(colInv1, invDet), __lsx_vfmul_s(colInv2, invDet));
}

PX_FORCE_INLINE Mat33V M33Trnsps(const Mat33V& a)
{
	return Mat33V(V3Merge(V3GetX(a.col0), V3GetX(a.col1), V3GetX(a.col2)),
	              V3Merge(V3GetY(a.col0), V3GetY(a.col1), V3GetY(a.col2)),
	              V3Merge(V3GetZ(a.col0), V3GetZ(a.col1), V3GetZ(a.col2)));
}

PX_FORCE_INLINE Mat33V M33Identity()
{
	return Mat33V(V3UnitX(), V3UnitY(), V3UnitZ());
}

PX_FORCE_INLINE Mat33V M33Sub(const Mat33V& a, const Mat33V& b)
{
	return Mat33V(V3Sub(a.col0, b.col0), V3Sub(a.col1, b.col1), V3Sub(a.col2, b.col2));
}

PX_FORCE_INLINE Mat33V M33Neg(const Mat33V& a)
{
	return Mat33V(V3Neg(a.col0), V3Neg(a.col1), V3Neg(a.col2));
}

PX_FORCE_INLINE Mat33V M33Abs(const Mat33V& a)
{
	return Mat33V(V3Abs(a.col0), V3Abs(a.col1), V3Abs(a.col2));
}

PX_FORCE_INLINE Mat33V PromoteVec3V(const Vec3V v)
{
	const BoolV bTFFF = BTFFF();
	const BoolV bFTFF = BFTFF();
	const BoolV bFFTF = BTFTF();

	const Vec3V zero = V3Zero();

	return Mat33V(V3Sel(bTFFF, v, zero), V3Sel(bFTFF, v, zero), V3Sel(bFFTF, v, zero));
}

PX_FORCE_INLINE Mat33V M33Diagonal(const Vec3VArg d)
{
	const FloatV x = V3Mul(V3UnitX(), d);
	const FloatV y = V3Mul(V3UnitY(), d);
	const FloatV z = V3Mul(V3UnitZ(), d);
	return Mat33V(x, y, z);
}

//////////////////////////////////
// MAT34V
//////////////////////////////////

PX_FORCE_INLINE Vec3V M34MulV3(const Mat34V& a, const Vec3V b)
{
	const FloatV x = V3GetX(b);
	const FloatV y = V3GetY(b);
	const FloatV z = V3GetZ(b);
	const Vec3V v0 = V3Scale(a.col0, x);
	const Vec3V v1 = V3Scale(a.col1, y);
	const Vec3V v2 = V3Scale(a.col2, z);
	const Vec3V v0PlusV1 = V3Add(v0, v1);
	const Vec3V v0PlusV1Plusv2 = V3Add(v0PlusV1, v2);
	return V3Add(v0PlusV1Plusv2, a.col3);
}

PX_FORCE_INLINE Vec3V M34Mul33V3(const Mat34V& a, const Vec3V b)
{
	const FloatV x = V3GetX(b);
	const FloatV y = V3GetY(b);
	const FloatV z = V3GetZ(b);
	const Vec3V v0 = V3Scale(a.col0, x);
	const Vec3V v1 = V3Scale(a.col1, y);
	const Vec3V v2 = V3Scale(a.col2, z);
	const Vec3V v0PlusV1 = V3Add(v0, v1);
	return V3Add(v0PlusV1, v2);
}

PX_FORCE_INLINE Vec3V M34TrnspsMul33V3(const Mat34V& a, const Vec3V b)
{
	const FloatV x = V3Dot(a.col0, b);
	const FloatV y = V3Dot(a.col1, b);
	const FloatV z = V3Dot(a.col2, b);
	return V3Merge(x, y, z);
}

PX_FORCE_INLINE Mat34V M34MulM34(const Mat34V& a, const Mat34V& b)
{
	return Mat34V(M34Mul33V3(a, b.col0), M34Mul33V3(a, b.col1), M34Mul33V3(a, b.col2), M34MulV3(a, b.col3));
}

PX_FORCE_INLINE Mat33V M34MulM33(const Mat34V& a, const Mat33V& b)
{
	return Mat33V(M34Mul33V3(a, b.col0), M34Mul33V3(a, b.col1), M34Mul33V3(a, b.col2));
}

PX_FORCE_INLINE Mat33V M34Mul33MM34(const Mat34V& a, const Mat34V& b)
{
	return Mat33V(M34Mul33V3(a, b.col0), M34Mul33V3(a, b.col1), M34Mul33V3(a, b.col2));
}

PX_FORCE_INLINE Mat34V M34Add(const Mat34V& a, const Mat34V& b)
{
	return Mat34V(V3Add(a.col0, b.col0), V3Add(a.col1, b.col1), V3Add(a.col2, b.col2), V3Add(a.col3, b.col3));
}

PX_FORCE_INLINE Mat33V M34Trnsps33(const Mat34V& a)
{
	return Mat33V(V3Merge(V3GetX(a.col0), V3GetX(a.col1), V3GetX(a.col2)),
	              V3Merge(V3GetY(a.col0), V3GetY(a.col1), V3GetY(a.col2)),
	              V3Merge(V3GetZ(a.col0), V3GetZ(a.col1), V3GetZ(a.col2)));
}

//////////////////////////////////
// MAT44V
//////////////////////////////////

PX_FORCE_INLINE Vec4V M44MulV4(const Mat44V& a, const Vec4V b)
{
	const FloatV x = V4GetX(b);
	const FloatV y = V4GetY(b);
	const FloatV z = V4GetZ(b);
	const FloatV w = V4GetW(b);

	const Vec4V v0 = V4Scale(a.col0, x);
	const Vec4V v1 = V4Scale(a.col1, y);
	const Vec4V v2 = V4Scale(a.col2, z);
	const Vec4V v3 = V4Scale(a.col3, w);
	const Vec4V v0PlusV1 = V4Add(v0, v1);
	const Vec4V v0PlusV1Plusv2 = V4Add(v0PlusV1, v2);
	return V4Add(v0PlusV1Plusv2, v3);
}

PX_FORCE_INLINE Vec4V M44TrnspsMulV4(const Mat44V& a, const Vec4V b)
{
	PX_ALIGN(16, FloatV) dotProdArray[4] = { V4Dot(a.col0, b), V4Dot(a.col1, b), V4Dot(a.col2, b), V4Dot(a.col3, b) };
	return V4Merge(dotProdArray);
}

PX_FORCE_INLINE Mat44V M44MulM44(const Mat44V& a, const Mat44V& b)
{
	return Mat44V(M44MulV4(a, b.col0), M44MulV4(a, b.col1), M44MulV4(a, b.col2), M44MulV4(a, b.col3));
}

PX_FORCE_INLINE Mat44V M44Add(const Mat44V& a, const Mat44V& b)
{
	return Mat44V(V4Add(a.col0, b.col0), V4Add(a.col1, b.col1), V4Add(a.col2, b.col2), V4Add(a.col3, b.col3));
}

PX_FORCE_INLINE Mat44V M44Trnsps(const Mat44V& a)
{
	const Vec4V v0 = __lsx_vilvl_w(a.col2, a.col0);
	const Vec4V v1 = __lsx_vilvh_w(a.col2, a.col0);
	const Vec4V v2 = __lsx_vilvl_w(a.col3, a.col1);
	const Vec4V v3 = __lsx_vilvh_w(a.col3, a.col1);
	return Mat44V(__lsx_vilvl_w(v2, v0), __lsx_vilvh_w(v2, v0), __lsx_vilvl_w(v3, v1), __lsx_vilvh_w(v3, v1));
}

PX_FORCE_INLINE Mat44V M44Inverse(const Mat44V& a)
{
	__m128 minor0, minor1, minor2, minor3;
	__m128 row0, row1, row2, row3;
	__m128 det, tmp1;

	tmp1 = V4Zero();
	row1 = V4Zero();
	row3 = V4Zero();

	row0 = a.col0;
	row1 = __lsx_vpermi_w(a.col1, a.col1, 0x8e);
	row2 = a.col2;
	row3 = __lsx_vpermi_w(a.col3, a.col3, 0x08e);

	tmp1 = __lsx_vfmul_s(row2, row3);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	minor0 = __lsx_vfmul_s(row1, tmp1);
	minor1 = __lsx_vfmul_s(row0, tmp1);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor0 = __lsx_vfsub_s(__lsx_vfmul_s(row1, tmp1), minor0);
	minor1 = __lsx_vfsub_s(__lsx_vfmul_s(row0, tmp1), minor1);
	minor1 = __lsx_vpermi_w(minor1, minor1, 0x4E);

	tmp1 = __lsx_vfmul_s(row1, row2);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	minor0 = __lsx_vfadd_s(__lsx_vfmul_s(row3, tmp1), minor0);
	minor3 = __lsx_vfmul_s(row0, tmp1);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor0 = __lsx_vfsub_s(minor0, __lsx_vfmul_s(row3, tmp1));
	minor3 = __lsx_vfsub_s(__lsx_vfmul_s(row0, tmp1), minor3);
	minor3 = __lsx_vpermi_w(minor3, minor3, 0x4E);

	tmp1 = __lsx_vfmul_s(__lsx_vpermi_w(row1, row1, 0x4E), row3);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	row2 = __lsx_vpermi_w(row2, row2, 0x4E);
	minor0 = __lsx_vfadd_s(__lsx_vfmul_s(row2, tmp1), minor0);
	minor2 = __lsx_vfmul_s(row0, tmp1);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor0 = __lsx_vfsub_s(minor0, __lsx_vfmul_s(row2, tmp1));
	minor2 = __lsx_vfsub_s(__lsx_vfmul_s(row0, tmp1), minor2);
	minor2 = __lsx_vpermi_w(minor2, minor2, 0x4E);

	tmp1 = __lsx_vfmul_s(row0, row1);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	minor2 = __lsx_vfadd_s(__lsx_vfmul_s(row3, tmp1), minor2);
	minor3 = __lsx_vfsub_s(__lsx_vfmul_s(row2, tmp1), minor3);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor2 = __lsx_vfsub_s(__lsx_vfmul_s(row3, tmp1), minor2);
	minor3 = __lsx_vfsub_s(minor3, __lsx_vfmul_s(row2, tmp1));

	tmp1 = __lsx_vfmul_s(row0, row3);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	minor1 = __lsx_vfsub_s(minor1, __lsx_vfmul_s(row2, tmp1));
	minor2 = __lsx_vfadd_s(__lsx_vfmul_s(row1, tmp1), minor2);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor1 = __lsx_vfadd_s(__lsx_vfmul_s(row2, tmp1), minor1);
	minor2 = __lsx_vfsub_s(minor2, __lsx_vfmul_s(row1, tmp1));

	tmp1 = __lsx_vfmul_s(row0, row2);
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0xB1);
	minor1 = __lsx_vfadd_s(__lsx_vfmul_s(row3, tmp1), minor1);
	minor3 = __lsx_vfsub_s(minor3, __lsx_vfmul_s(row1, tmp1));
	tmp1 = __lsx_vpermi_w(tmp1, tmp1, 0x4E);
	minor1 = __lsx_vfsub_s(minor1, __lsx_vfmul_s(row3, tmp1));
	minor3 = __lsx_vfadd_s(__lsx_vfmul_s(row1, tmp1), minor3);

	det = __lsx_vfmul_s(row0, minor0);
	det = __lsx_vfadd_s(__lsx_vpermi_w(det, det, 0x4E), det);
	det = __lsx_vfadd_s(__lsx_vpermi_w(det, det, 0xB1), det);
	tmp1 = __lsx_vfrecip_s(det);
	det = __lsx_vpermi_w(tmp1, tmp1, 0x00);

	minor0 = __lsx_vfmul_s(det, minor0);
	minor1 = __lsx_vfmul_s(det, minor1);
	minor2 = __lsx_vfmul_s(det, minor2);
	minor3 = __lsx_vfmul_s(det, minor3);
	Mat44V invTrans(minor0, minor1, minor2, minor3);
	return M44Trnsps(invTrans);
}

PX_FORCE_INLINE Vec4V V4LoadXYZW(const PxF32& x, const PxF32& y, const PxF32& z, const PxF32& w)
{
	__m128 xyzw = {x, y, z, w};
	return xyzw;
}

/*
PX_FORCE_INLINE VecU16V V4U32PK(VecU32V a, VecU32V b)
{
    VecU16V result = {
    PxU16(PxClamp<PxU32>(a[0], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(a[1], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(a[2], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(a[3], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(b[0], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(b[1], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(b[2], 0, 0xFFFF)),
    PxU16(PxClamp<PxU32>(b[3], 0, 0xFFFF))};
    return result;
}
*/

PX_FORCE_INLINE VecU32V V4U32Sel(const BoolV c, const VecU32V a, const VecU32V b)
{
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE VecU32V V4U32or(VecU32V a, VecU32V b)
{
	return __lsx_vor_v(a, b);
}

PX_FORCE_INLINE VecU32V V4U32xor(VecU32V a, VecU32V b)
{
	return __lsx_vxor_v(a, b);
}

PX_FORCE_INLINE VecU32V V4U32and(VecU32V a, VecU32V b)
{
	return __lsx_vand_v(a, b);
}

PX_FORCE_INLINE VecU32V V4U32Andc(VecU32V a, VecU32V b)
{
	return __lsx_vandn_v(b, a);
}

/*
PX_FORCE_INLINE VecU16V V4U16Or(VecU16V a, VecU16V b)
{
    return __lsx_vor_v(a, b);
}
*/

/*
PX_FORCE_INLINE VecU16V V4U16And(VecU16V a, VecU16V b)
{
    return __lsx_vand_v(a, b);
}
*/

/*
PX_FORCE_INLINE VecU16V V4U16Andc(VecU16V a, VecU16V b)
{
    return __lsx_vandn_v(b, a);
}
*/

PX_FORCE_INLINE VecI32V I4Load(const PxI32 i)
{
	return __lsx_vreplgr2vr_w(i);
}

PX_FORCE_INLINE VecI32V I4LoadU(const PxI32* i)
{
	return __lsx_vldrepl_w(i, 0);
}

PX_FORCE_INLINE VecI32V I4LoadA(const PxI32* i)
{
	return __lsx_vldrepl_w(i, 0);
}

PX_FORCE_INLINE VecI32V VecI32V_Add(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vadd_w(a, b);
}

PX_FORCE_INLINE VecI32V VecI32V_Sub(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vsub_w(a, b);
}

PX_FORCE_INLINE BoolV VecI32V_IsGrtr(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vslt_w(b, a);
}

PX_FORCE_INLINE BoolV VecI32V_IsEq(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vseq_w(a, b);
}

PX_FORCE_INLINE VecI32V V4I32Sel(const BoolV c, const VecI32V a, const VecI32V b)
{
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE VecI32V VecI32V_Zero()
{
	__m128i zero = {0};
	return zero;
}

PX_FORCE_INLINE VecI32V VecI32V_One()
{
	return I4Load(1);
}

PX_FORCE_INLINE VecI32V VecI32V_Two()
{
	return I4Load(2);
}

PX_FORCE_INLINE VecI32V VecI32V_MinusOne()
{
	return I4Load(-1);
}

PX_FORCE_INLINE VecU32V U4Zero()
{
	return U4Load(0);
}

PX_FORCE_INLINE VecU32V U4One()
{
	return U4Load(1);
}

PX_FORCE_INLINE VecU32V U4Two()
{
	return U4Load(2);
}

PX_FORCE_INLINE VecI32V VecI32V_Sel(const BoolV c, const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vbitsel_v(b, a, c);
}

PX_FORCE_INLINE VecShiftV VecI32V_PrepareShift(const VecI32VArg shift)
{
	return shift;
}

PX_FORCE_INLINE VecI32V VecI32V_LeftShift(const VecI32VArg a, const VecShiftVArg count)
{
	return __lsx_vsll_w(a, count);
}

PX_FORCE_INLINE VecI32V VecI32V_RightShift(const VecI32VArg a, const VecShiftVArg count)
{
	return __lsx_vsrl_w(a, count);
}

PX_FORCE_INLINE VecI32V VecI32V_And(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vand_v(a, b);
}

PX_FORCE_INLINE VecI32V VecI32V_Or(const VecI32VArg a, const VecI32VArg b)
{
	return __lsx_vor_v(a, b);
}

PX_FORCE_INLINE VecI32V VecI32V_GetX(const VecI32VArg a)
{
	return __lsx_vreplvei_w(a, 0);
}

PX_FORCE_INLINE VecI32V VecI32V_GetY(const VecI32VArg a)
{
	return __lsx_vreplvei_w(a, 1);
}

PX_FORCE_INLINE VecI32V VecI32V_GetZ(const VecI32VArg a)
{
	return __lsx_vreplvei_w(a, 2);
}

PX_FORCE_INLINE VecI32V VecI32V_GetW(const VecI32VArg a)
{
	return __lsx_vreplvei_w(a, 3);
}

PX_FORCE_INLINE void PxI32_From_VecI32V(const VecI32VArg a, PxI32* i)
{
	__lsx_vstelm_w(a, i, 0, 0);
}

PX_FORCE_INLINE VecI32V VecI32V_Merge(const VecI32VArg x, const VecI32VArg y, const VecI32VArg z, const VecI32VArg w)
{
	const __m128i xy = __lsx_vilvl_w(y, x);
	const __m128i zw = __lsx_vilvl_w(w, z);
	return __lsx_vilvl_d(zw, xy);
}

PX_FORCE_INLINE VecI32V VecI32V_From_BoolV(const BoolVArg a)
{
	return a;
}

PX_FORCE_INLINE VecU32V VecU32V_From_BoolV(const BoolVArg a)
{
	return a;
}

/*
template<int a> PX_FORCE_INLINE VecI32V V4ISplat()
{
    VecI32V result = {a, a, a, a};
    return result;
}

template<PxU32 a> PX_FORCE_INLINE VecU32V V4USplat()
{
    VecU32V result = {a, a, a, a};
    return result;
}
*/

/*
PX_FORCE_INLINE void V4U16StoreAligned(VecU16V val, VecU16V* address)
{
    *address = val;
}
*/

PX_FORCE_INLINE void V4U32StoreAligned(VecU32V val, VecU32V* address)
{
	*address = val;
}

PX_FORCE_INLINE Vec4V V4LoadAligned(Vec4V* addr)
{
	return *addr;
}

PX_FORCE_INLINE Vec4V V4LoadUnaligned(Vec4V* addr)
{
	return V4LoadU(reinterpret_cast<float*>(addr));
}

PX_FORCE_INLINE Vec4V V4Andc(const Vec4V a, const VecU32V b)
{
	VecU32V result32(a);
	result32 = V4U32Andc(result32, b);
	return Vec4V(result32);
}

PX_FORCE_INLINE VecU32V V4IsGrtrV32u(const Vec4V a, const Vec4V b)
{
	return V4IsGrtr(a, b);
}

PX_FORCE_INLINE VecU16V V4U16LoadAligned(VecU16V* addr)
{
	return *addr;
}

PX_FORCE_INLINE VecU16V V4U16LoadUnaligned(VecU16V* addr)
{
	return *addr;
}

PX_FORCE_INLINE VecU16V V4U16CompareGt(VecU16V a, VecU16V b)
{
	return __lsx_vslt_hu(b, a);
}

PX_FORCE_INLINE VecU16V V4I16CompareGt(VecU16V a, VecU16V b)
{
	return __lsx_vslt_h(b, a);
}

PX_FORCE_INLINE Vec4V Vec4V_From_VecU32V(VecU32V a)
{
	return __lsx_vffint_s_wu(a);
}

PX_FORCE_INLINE Vec4V Vec4V_From_VecI32V(VecI32V in)
{
	return __lsx_vffint_s_w(in);
}

PX_FORCE_INLINE VecI32V VecI32V_From_Vec4V(Vec4V a)
{
	return __lsx_vftint_w_s(a);
}

PX_FORCE_INLINE Vec4V Vec4V_ReinterpretFrom_VecU32V(VecU32V a)
{
	return Vec4V(a);
}

PX_FORCE_INLINE Vec4V Vec4V_ReinterpretFrom_VecI32V(VecI32V a)
{
	return m128_I2F(a);
}

PX_FORCE_INLINE VecU32V VecU32V_ReinterpretFrom_Vec4V(Vec4V a)
{
	return VecU32V(a);
}

PX_FORCE_INLINE VecI32V VecI32V_ReinterpretFrom_Vec4V(Vec4V a)
{
	return m128_F2I(a);
}

/*
template<int index> PX_FORCE_INLINE BoolV BSplatElement(BoolV a)
{
    return __lsx_vreplve_w(a, index);
}
*/

template <int index>
BoolV BSplatElement(BoolV a)
{
	return __lsx_vreplve_w(a, index);
}

template <int index>
PX_FORCE_INLINE VecU32V V4U32SplatElement(VecU32V a)
{
	return __lsx_vreplve_w(a, index);
}

template <int index>
PX_FORCE_INLINE Vec4V V4SplatElement(Vec4V a)
{
	return __lsx_vreplve_w(a, index);
}

PX_FORCE_INLINE VecU32V U4LoadXYZW(PxU32 x, PxU32 y, PxU32 z, PxU32 w)
{
	VecU32V result = {x, y, z, w};
	return result;
}

PX_FORCE_INLINE Vec4V V4Ceil(const Vec4V in)
{
	return V4LoadXYZW(PxCeil(in[0]), PxCeil(in[1]), PxCeil(in[2]), PxCeil(in[3]));
}

PX_FORCE_INLINE Vec4V V4Floor(const Vec4V in)
{
	return V4LoadXYZW(PxFloor(in[0]), PxFloor(in[1]), PxFloor(in[2]), PxFloor(in[3]));
}

PX_FORCE_INLINE VecU32V V4ConvertToU32VSaturate(const Vec4V in, PxU32 power)
{
	PX_ASSERT(power == 0 && "Non-zero power not supported in convertToU32VSaturate");
	PX_UNUSED(power); // prevent warning in release builds
	return __lsx_vftint_w_s(in);
}

PX_FORCE_INLINE void QuatGetMat33V(const QuatVArg q, Vec3V& column0, Vec3V& column1, Vec3V& column2)
{
    const FloatV one = FOne();
    const FloatV x = V4GetX(q);
    const FloatV y = V4GetY(q);
    const FloatV z = V4GetZ(q);
    const FloatV w = V4GetW(q);

    const FloatV x2 = FAdd(x, x);
    const FloatV y2 = FAdd(y, y);
    const FloatV z2 = FAdd(z, z);

    const FloatV xx = FMul(x2, x);
    const FloatV yy = FMul(y2, y);
    const FloatV zz = FMul(z2, z);

    const FloatV xy = FMul(x2, y);
    const FloatV xz = FMul(x2, z);
    const FloatV xw = FMul(x2, w);

    const FloatV yz = FMul(y2, z);
    const FloatV yw = FMul(y2, w);
    const FloatV zw = FMul(z2, w);

    const FloatV v = FSub(one, xx);

    column0 = V3Merge(FSub(FSub(one, yy), zz), FAdd(xy, zw), FSub(xz, yw));
    column1 = V3Merge(FSub(xy, zw), FSub(v, zz), FAdd(yz, xw));
    column2 = V3Merge(FAdd(xz, yw), FSub(yz, xw), FSub(v, yy));
}

} // namespace aos
} // namespace shdfnd
} // namespace physx

#endif // PSFOUNDATION_PSUNIXSSE2INLINEAOS_H
