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



#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/filter.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>

#pragma warning(push)
#pragma warning(disable: 4355)

#define _PP_GET_FIELD_TYPE(n) BOOST_PP_ARRAY_ELEM(0, BOOST_PP_SEQ_ELEM(n, INPLACE_TYPE_STRUCT_FIELDS))
#define _PP_GET_FIELD_NAME(n) BOOST_PP_ARRAY_ELEM(1, BOOST_PP_SEQ_ELEM(n, INPLACE_TYPE_STRUCT_FIELDS))

#define _PP_HAS_FIELD_SIZE(n) BOOST_PP_EQUAL(BOOST_PP_ARRAY_SIZE(BOOST_PP_SEQ_ELEM(n, INPLACE_TYPE_STRUCT_FIELDS)), 3)
#define _PP_GET_FIELD_SIZE(n) BOOST_PP_ARRAY_ELEM(2, BOOST_PP_SEQ_ELEM(n, INPLACE_TYPE_STRUCT_FIELDS))


#ifdef INPLACE_TYPE_STRUCT_NAME

struct INPLACE_TYPE_STRUCT_NAME
#ifdef INPLACE_TYPE_STRUCT_BASE
	: INPLACE_TYPE_STRUCT_BASE
#endif
{
//fields
#ifdef INPLACE_TYPE_STRUCT_FIELDS
#define BOOST_PP_LOCAL_LIMITS (0, BOOST_PP_SEQ_SIZE(INPLACE_TYPE_STRUCT_FIELDS) - 1)
#define BOOST_PP_LOCAL_MACRO(n) \
	_PP_GET_FIELD_TYPE(n) _PP_GET_FIELD_NAME(n) BOOST_PP_IF(_PP_HAS_FIELD_SIZE(n), [##_PP_GET_FIELD_SIZE(n)##], BOOST_PP_EMPTY());
#include BOOST_PP_LOCAL_ITERATE()
#endif

//reflectSelf
	template <int _inplace_offset_, typename R, typename RA>
#if defined(INPLACE_TYPE_STRUCT_BASE) | defined(INPLACE_TYPE_STRUCT_FIELDS)
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& r, RA ra)
	{
#ifdef INPLACE_TYPE_STRUCT_BASE
		INPLACE_TYPE_STRUCT_BASE::reflectSelf<_inplace_offset_>(r, ra);
#endif
#ifdef INPLACE_TYPE_STRUCT_FIELDS
#define BOOST_PP_LOCAL_LIMITS (0, BOOST_PP_SEQ_SIZE(INPLACE_TYPE_STRUCT_FIELDS) - 1)
#define BOOST_PP_LOCAL_MACRO(n) \
		InplaceTypeHelper::reflectType<APEX_OFFSETOF( INPLACE_TYPE_STRUCT_NAME, _PP_GET_FIELD_NAME(n) ) + _inplace_offset_>(r, ra, _PP_GET_FIELD_NAME(n), InplaceTypeMemberDefaultTraits());
#include BOOST_PP_LOCAL_ITERATE()
#endif
	}
#else
	APEX_CUDA_CALLABLE PX_INLINE void reflectSelf(R& , RA )
	{
	}
#endif

#ifndef INPLACE_TYPE_STRUCT_LEAVE_OPEN
};
#endif

#endif

#undef _PP_GET_FIELD_TYPE
#undef _PP_GET_FIELD_NAME
#undef _PP_HAS_FIELD_SIZE
#undef _PP_GET_FIELD_SIZE

#undef INPLACE_TYPE_STRUCT_NAME
#undef INPLACE_TYPE_STRUCT_BASE
#undef INPLACE_TYPE_STRUCT_FIELDS
#undef INPLACE_TYPE_STRUCT_LEAVE_OPEN

#pragma warning(pop)
