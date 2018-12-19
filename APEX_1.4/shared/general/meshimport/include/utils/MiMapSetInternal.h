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



#ifndef MI_MAP_SET_INTERNAL_H

#define MI_MAP_SET_INTERNAL_H

/*
Copyright (C) 2009-2010 Electronic Arts, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
3.  Neither the name of Electronic Arts, Inc. ("EA") nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ELECTRONIC ARTS AND ITS CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS OR ITS CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

///////////////////////////////////////////////////////////////////////////////
// Written by Paul Pedriana.
//
// Refactored to be compliant with the PhysX data types by John W. Ratcliff
// on March 23, 2011
//////////////////////////////////////////////////////////////////////////////



#include "MiPlatformConfig.h"
#include <limits.h>
#include <stddef.h>
#include <new>

#pragma warning(push)
#pragma warning(disable:4512)

///////////////////////////////////////////////////////////////////////////////
// EASTL namespace
//
// We define this so that users that #include this config file can reference 
// these namespaces without seeing any other files that happen to use them.
///////////////////////////////////////////////////////////////////////////////

/// EA Standard Template Library
namespace mimp
{

///////////////////////////////////////////////////////////////////////////////
// EASTL_ALIGN_OF
//
// Determines the alignment of a type.
//
// Example usage:
//    size_t alignment = EASTL_ALIGN_OF(int);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALIGN_OF
#if defined(__MWERKS__)
#define EASTL_ALIGN_OF(type) ((size_t)__alignof__(type))
#elif !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x doesn't do __alignof correctly all the time.
#define EASTL_ALIGN_OF __alignof
#else
#define EASTL_ALIGN_OF(type) ((size_t)offsetof(struct{ char c; type m; }, m))
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// EASTL_DEBUG
//
// Defined as an integer >= 0. Default is 1 for debug builds and 0 for 
// release builds. This define is also a master switch for the default value 
// of some other settings.
//
// Example usage:
//    #if EASTL_DEBUG
//       ...
//    #endif
//
///////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG)
        #define EASTL_DEBUG 1
#else
        #define EASTL_DEBUG 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DEBUGPARAMS_LEVEL
//
// EASTL_DEBUGPARAMS_LEVEL controls what debug information is passed through to
// the allocator by default.
// This value may be defined by the user ... if not it will default to 1 for
// EA_DEBUG builds, otherwise 0.
//
//  0 - no debug information is passed through to allocator calls.
//  1 - 'name' is passed through to allocator calls.
//  2 - 'name', __FILE__, and __LINE__ are passed through to allocator calls.
//
// This parameter mirrors the equivalent parameter in the CoreAllocator package.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_DEBUGPARAMS_LEVEL
    #if EASTL_DEBUG
        #define EASTL_DEBUGPARAMS_LEVEL 2
    #else
        #define EASTL_DEBUGPARAMS_LEVEL 0
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_NAME_ENABLED / EASTL_NAME / EASTL_NAME_VAL
//
// Used to wrap debug string names. In a release build, the definition 
// goes away. These are present to avoid release build compiler warnings 
// and to make code simpler.
//
// Example usage of EASTL_NAME:
//    // pName will defined away in a release build and thus prevent compiler warnings.
//    void allocator::set_name(const char* EASTL_NAME(pName))
//    {
//        #if EASTL_NAME_ENABLED
//            mpName = pName;
//        #endif
//    }
//
// Example usage of EASTL_NAME_VAL:
//    // "xxx" is defined to NULL in a release build.
//    vector<T, Allocator>::vector(const allocator_type& allocator = allocator_type(EASTL_NAME_VAL("xxx")));
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_NAME_ENABLED
    #define EASTL_NAME_ENABLED EASTL_DEBUG
#endif

#ifndef EASTL_NAME
    #if EASTL_NAME_ENABLED
        #define EASTL_NAME(x)      x
        #define EASTL_NAME_VAL(x)  x
    #else
        #define EASTL_NAME(x)
        #define EASTL_NAME_VAL(x) ((const char*)NULL)
    #endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_DEFAULT_NAME_PREFIX
//
// Defined as a string literal. Defaults to "EASTL".
// This define is used as the default name for EASTL where such a thing is
// referenced in EASTL. For example, if the user doesn't specify an allocator
// name for their deque, it is named "EASTL deque". However, you can override
// this to say "SuperBaseball deque" by changing EASTL_DEFAULT_NAME_PREFIX.
//
// Example usage (which is simply taken from how deque.h uses this define):
//     #ifndef EASTL_DEQUE_DEFAULT_NAME
//         #define EASTL_DEQUE_DEFAULT_NAME   EASTL_DEFAULT_NAME_PREFIX " deque"
//     #endif
//
#ifndef EASTL_DEFAULT_NAME_PREFIX
    #define EASTL_DEFAULT_NAME_PREFIX "EASTL"
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as EASTL_DEBUG.
// If EASTL_ASSERT_ENABLED is non-zero, then asserts will be executed via 
// the assertion mechanism.
//
// Example usage:
//     #if EASTL_ASSERT_ENABLED
//         EASTL_ASSERT(v.size() > 17);
//     #endif
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ASSERT_ENABLED
    #define EASTL_ASSERT_ENABLED EASTL_DEBUG
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
//
// Defined as 0 or non-zero. Default is same as EASTL_ASSERT_ENABLED.
// This is like EASTL_ASSERT_ENABLED, except it is for empty container 
// references. Sometime people like to be able to take a reference to 
// the front of the container, but not use it if the container is empty.
// In practice it's often easier and more efficient to do this than to write 
// extra code to check if the container is empty. 
//
// Example usage:
//     template <typename T, typename Allocator>
//     inline typename vector<T, Allocator>::reference
//     vector<T, Allocator>::front()
//     {
//         #if EASTL_ASSERT_ENABLED
//             EASTL_ASSERT(mpEnd > mpBegin);
//         #endif
// 
//         return *mpBegin;
//     }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_EMPTY_REFERENCE_ASSERT_ENABLED
    #define EASTL_EMPTY_REFERENCE_ASSERT_ENABLED EASTL_ASSERT_ENABLED
#endif



///////////////////////////////////////////////////////////////////////////////
// SetAssertionFailureFunction
//
// Allows the user to set a custom assertion failure mechanism.
//
// Example usage:
//     void Assert(const char* pExpression, void* pContext);
//     SetAssertionFailureFunction(Assert, this);
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ASSERTION_FAILURE_DEFINED
    #define EASTL_ASSERTION_FAILURE_DEFINED

        typedef void (*EASTL_AssertionFailureFunction)(const char* pExpression, void* pContext);
         void SetAssertionFailureFunction(EASTL_AssertionFailureFunction pFunction, void* pContext);

        // These are the internal default functions that implement asserts.
         void AssertionFailure(const char* pExpression);
         void AssertionFailureFunctionDefault(const char* pExpression, void* pContext);
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ASSERT
//
// Assertion macro. Can be overridden by user with a different value.
//
// Example usage:
//    EASTL_ASSERT(intVector.size() < 100);
//
///////////////////////////////////////////////////////////////////////////////

#define EASTL_ASSERT(expression)



///////////////////////////////////////////////////////////////////////////////
// EASTL_FAIL_MSG
//
// Failure macro. Can be overridden by user with a different value.
//
// Example usage:
//    EASTL_FAIL("detected error condition!");
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FAIL_MSG
    #if EASTL_ASSERT_ENABLED
        #define EASTL_FAIL_MSG(message) (mimp::AssertionFailure(message))
    #else
        #define EASTL_FAIL_MSG(message)
    #endif
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_CT_ASSERT / EASTL_CT_ASSERT_NAMED
//
// EASTL_CT_ASSERT is a macro for compile time assertion checks, useful for 
// validating *constant* expressions. The advantage over using EASTL_ASSERT 
// is that errors are caught at compile time instead of runtime.
//
// Example usage:
//     EASTL_CT_ASSERT(sizeof(mimp::MiU32 == 4));
//
///////////////////////////////////////////////////////////////////////////////

#if defined(EASTL_DEBUG) && !defined(EASTL_CT_ASSERT)
    template <bool>  struct EASTL_CT_ASSERTION_FAILURE;
    template <>      struct EASTL_CT_ASSERTION_FAILURE<true>{ enum { value = 1 }; }; // We create a specialization for true, but not for false.
    template <int x> struct EASTL_CT_ASSERTION_TEST{};

    #define EASTL_PREPROCESSOR_JOIN(a, b)  EASTL_PREPROCESSOR_JOIN1(a, b)
    #define EASTL_PREPROCESSOR_JOIN1(a, b) EASTL_PREPROCESSOR_JOIN2(a, b)
    #define EASTL_PREPROCESSOR_JOIN2(a, b) a##b

    #if defined(_MSC_VER)
        #define EASTL_CT_ASSERT(expression)  typedef EASTL_CT_ASSERTION_TEST< sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >)> EASTL_CT_ASSERT_FAILURE
    #elif defined(__ICL) || defined(__ICC)
        #define EASTL_CT_ASSERT(expression)  typedef char EASTL_PREPROCESSOR_JOIN(EASTL_CT_ASSERT_FAILURE_, __LINE__) [EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >::value]
    #elif defined(__MWERKS__)
        #define EASTL_CT_ASSERT(expression)  enum { EASTL_PREPROCESSOR_JOIN(EASTL_CT_ASSERT_FAILURE_, __LINE__) = sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >) }
    #else // GCC, etc.
        #define EASTL_CT_ASSERT(expression)  typedef EASTL_CT_ASSERTION_TEST< sizeof(EASTL_CT_ASSERTION_FAILURE< (bool)(expression) >)> EASTL_PREPROCESSOR_JOIN1(EASTL_CT_ASSERT_FAILURE_, __LINE__)
    #endif
#else
    #define EASTL_CT_ASSERT(expression)
#endif


///////////////////////////////////////////////////////////////////////////////
// EASTL_ALLOCATOR_COPY_ENABLED
//
// Defined as 0 or 1. Default is 0 (disabled) until some future date.
// If enabled (1) then container operator= copies the allocator from the 
// source container. It ideally should be set to enabled but for backwards
// compatibility with older versions of EASTL it is currently set to 0.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALLOCATOR_COPY_ENABLED
    #define EASTL_ALLOCATOR_COPY_ENABLED 0
#endif

	///////////////////////////////////////////////////////////////////////////////
	// EASTL_FIXED_SIZE_TRACKING_ENABLED
	//
	// Defined as an integer >= 0. Default is same as EASTL_DEBUG.
	// If EASTL_FIXED_SIZE_TRACKING_ENABLED is enabled, then fixed
	// containers in debug builds track the max count of objects 
	// that have been in the container. This allows for the tuning
	// of fixed container sizes to their minimum required size.
	//
	///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FIXED_SIZE_TRACKING_ENABLED
#define EASTL_FIXED_SIZE_TRACKING_ENABLED EASTL_DEBUG
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_STRING_OPT_XXXX
//
// Enables some options / optimizations options that cause the string class 
// to behave slightly different from the C++ standard basic_string. These are 
// options whereby you can improve performance by avoiding operations that 
// in practice may never occur for you.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_STRING_OPT_CHAR_INIT
// Defined as 0 or 1. Default is 1.
// Defines if newly created characters are initialized to 0 or left
// as random values.
// The C++ string standard is to initialize chars to 0.
#define EASTL_STRING_OPT_CHAR_INIT 1
#endif

#ifndef EASTL_STRING_OPT_EXPLICIT_CTORS
// Defined as 0 or 1. Default is 0.
// Defines if we should implement explicity in constructors where the C++
// standard string does not. The advantage of enabling explicit constructors
// is that you can do this: string s = "hello"; in addition to string s("hello");
// The disadvantage of enabling explicity constructors is that there can be 
// silent conversions done which impede performance if the user isn't paying
// attention.
// C++ standard string ctors are not explicit.
#define EASTL_STRING_OPT_EXPLICIT_CTORS 0
#endif

#ifndef EASTL_STRING_OPT_LENGTH_ERRORS
// Defined as 0 or 1. Default is equal to EASTL_EXCEPTIONS_ENABLED.
// Defines if we check for string values going beyond kMaxSize 
// (a very large value) and throw exections if so.
// C++ standard strings are expected to do such checks.
#define EASTL_STRING_OPT_LENGTH_ERRORS 0
#endif

#ifndef EASTL_STRING_OPT_RANGE_ERRORS
// Defined as 0 or 1. Default is equal to EASTL_EXCEPTIONS_ENABLED.
// Defines if we check for out-of-bounds references to string
// positions and throw exceptions if so. Well-behaved code shouldn't 
// refence out-of-bounds positions and so shouldn't need these checks.
// C++ standard strings are expected to do such range checks.
#define EASTL_STRING_OPT_RANGE_ERRORS 0
#endif

#ifndef EASTL_STRING_OPT_ARGUMENT_ERRORS
// Defined as 0 or 1. Default is 0.
// Defines if we check for NULL ptr arguments passed to string 
// functions by the user and throw exceptions if so. Well-behaved code 
// shouldn't pass bad arguments and so shouldn't need these checks.
// Also, some users believe that strings should check for NULL pointers 
// in all their arguments and do no-ops if so. This is very debatable.
// C++ standard strings are not required to check for such argument errors.
#define EASTL_STRING_OPT_ARGUMENT_ERRORS 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_ABSTRACT_STRING_ENABLED
//
// Defined as 0 or 1. Default is 0 until abstract string is fully tested.
// Defines whether the proposed replacement for the string module is enabled.
// See bonus/abstract_string.h for more information.
//
#ifndef EASTL_ABSTRACT_STRING_ENABLED
#define EASTL_ABSTRACT_STRING_ENABLED 0
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_BITSET_SIZE_T
//
// Defined as 0 or 1. Default is 1.
// Controls whether bitset uses size_t or size_t.
//
#ifndef EASTL_BITSET_SIZE_T
#define EASTL_BITSET_SIZE_T 1
#endif


///////////////////////////////////////////////////////////////////////////////
// EASTL_LIST_SIZE_CACHE
//
// Defined as 0 or 1. Default is 0.
// If defined as 1, the list and slist containers (and possibly any additional
// containers as well) keep a member mSize (or similar) variable which allows
// the size() member function to execute in constant time (a.k.a. O(1)). 
// There are debates on both sides as to whether it is better to have this 
// cached value or not, as having it entails some cost (memory and code).
// To consider: Make list size caching an optional template parameter.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_LIST_SIZE_CACHE
#define EASTL_LIST_SIZE_CACHE 0
#endif

#ifndef EASTL_SLIST_SIZE_CACHE
#define EASTL_SLIST_SIZE_CACHE 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MAX_STACK_USAGE
//
// Defined as an integer greater than zero. Default is 4000.
// There are some places in EASTL where temporary objects are put on the 
// stack. A common example of this is in the implementation of container
// swap functions whereby a temporary copy of the container is made.
// There is a problem, however, if the size of the item created on the stack 
// is very large. This can happen with fixed-size containers, for example.
// The EASTL_MAX_STACK_USAGE define specifies the maximum amount of memory
// (in bytes) that the given platform/compiler will safely allow on the stack.
// Platforms such as Windows will generally allow larger values than embedded
// systems or console machines, but it is usually a good idea to stick with
// a max usage value that is portable across all platforms, lest the user be
// surprised when something breaks as it is ported to another platform.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_MAX_STACK_USAGE
#define EASTL_MAX_STACK_USAGE 4000
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_VA_COPY_ENABLED
//
// Defined as 0 or 1. Default is 1 for compilers that need it, 0 for others.
// Some compilers on some platforms implement va_list whereby its contents  
// are destroyed upon usage, even if passed by value to another function. 
// With these compilers you can use va_copy to restore the a va_list.
// Known compiler/platforms that destroy va_list contents upon usage include:
//     CodeWarrior on PowerPC
//     GCC on x86-64
// However, va_copy is part of the C99 standard and not part of earlier C and
// C++ standards. So not all compilers support it. VC++ doesn't support va_copy,
// but it turns out that VC++ doesn't need it on the platforms it supports.
// For example usage, see the EASTL string.h file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VA_COPY_ENABLED
#if defined(__MWERKS__) || (defined(__GNUC__) && (__GNUC__ >= 3) && (!defined(__i386__) || defined(__x86_64__)) && !defined(__ppc__) && !defined(__PPC__) && !defined(__PPC64__))
#define EASTL_VA_COPY_ENABLED 1
#else
#define EASTL_VA_COPY_ENABLED 0
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_LIST_PROXY_ENABLED
//
#if !defined(EASTL_LIST_PROXY_ENABLED)
// GCC with -fstrict-aliasing has bugs (or undocumented functionality in their 
// __may_alias__ implementation. The compiler gets confused about function signatures.
// VC8 (1400) doesn't need the proxy because it has built-in smart debugging capabilities.
#if defined(EASTL_DEBUG) && (!defined(__GNUC__) || defined(__SNC__)) && (!defined(_MSC_VER) || (_MSC_VER < 1400))
#define EASTL_LIST_PROXY_ENABLED 1
#define EASTL_LIST_PROXY_MAY_ALIAS EASTL_MAY_ALIAS
#else
#define EASTL_LIST_PROXY_ENABLED 0
#define EASTL_LIST_PROXY_MAY_ALIAS
#endif
#endif

#define EASTL_ITC_NS mimp

///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATION_ENABLED
//
// Defined as an integer >= 0. Default is to be equal to EASTL_DEBUG.
// If nonzero, then a certain amount of automatic runtime validation is done.
// Runtime validation is not considered the same thing as asserting that user
// input values are valid. Validation refers to internal consistency checking
// of the validity of containers and their iterators. Validation checking is
// something that often involves significantly more than basic assertion 
// checking, and it may sometimes be desirable to disable it.
// This macro would generally be used internally by EASTL.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATION_ENABLED
#define EASTL_VALIDATION_ENABLED EASTL_DEBUG
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATE_COMPARE
//
// Defined as EASTL_ASSERT or defined away. Default is EASTL_ASSERT if EASTL_VALIDATION_ENABLED is enabled.
// This is used to validate user-supplied comparison functions, particularly for sorting purposes.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATE_COMPARE_ENABLED
#define EASTL_VALIDATE_COMPARE_ENABLED EASTL_VALIDATION_ENABLED
#endif

#if EASTL_VALIDATE_COMPARE_ENABLED
#define EASTL_VALIDATE_COMPARE EASTL_ASSERT
#else
#define EASTL_VALIDATE_COMPARE(expression)
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_VALIDATE_INTRUSIVE_LIST
//
// Defined as an integral value >= 0. Controls the amount of automatic validation
// done by intrusive_list. A value of 0 means no automatic validation is done.
// As of this writing, EASTL_VALIDATE_INTRUSIVE_LIST defaults to 0, as it makes
// the intrusive_list_node become a non-POD, which may be an issue for some code.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_VALIDATE_INTRUSIVE_LIST
#define EASTL_VALIDATE_INTRUSIVE_LIST 0
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_FORCE_INLINE
//
// Defined as a "force inline" expression or defined away.
// You generally don't need to use forced inlining with the Microsoft and 
// Metrowerks compilers, but you may need it with the GCC compiler (any version).
// 
// Example usage:
//     template <typename T, typename Allocator>
//     EASTL_FORCE_INLINE typename vector<T, Allocator>::size_type
//     vector<T, Allocator>::size() const
//        { return mpEnd - mpBegin; }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_FORCE_INLINE
#define EASTL_FORCE_INLINE MI_INLINE
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MAY_ALIAS
//
// Defined as a macro that wraps the GCC may_alias attribute. This attribute
// has no significance for VC++ because VC++ doesn't support the concept of 
// strict aliasing. Users should avoid writing code that breaks strict 
// aliasing rules; EASTL_MAY_ALIAS is for cases with no alternative.
//
// Example usage:
//    mimp::MiU32 value EASTL_MAY_ALIAS;
//
// Example usage:
//    typedef mimp::MiU32 EASTL_MAY_ALIAS value_type;
//    value_type value;
//
#if defined(__GNUC__) && (((__GNUC__ * 100) + __GNUC_MINOR__) >= 303)
#define EASTL_MAY_ALIAS __attribute__((__may_alias__))
#else
#define EASTL_MAY_ALIAS
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_LIKELY / EASTL_UNLIKELY
//
// Defined as a macro which gives a hint to the compiler for branch
// prediction. GCC gives you the ability to manually give a hint to 
// the compiler about the result of a comparison, though it's often
// best to compile shipping code with profiling feedback under both
// GCC (-fprofile-arcs) and VC++ (/LTCG:PGO, etc.). However, there 
// are times when you feel very sure that a boolean expression will
// usually evaluate to either true or false and can help the compiler
// by using an explicity directive...
//
// Example usage:
//    if(EASTL_LIKELY(a == 0)) // Tell the compiler that a will usually equal 0.
//       { ... }
//
// Example usage:
//    if(EASTL_UNLIKELY(a == 0)) // Tell the compiler that a will usually not equal 0.
//       { ... }
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_LIKELY
#if defined(__GNUC__) && (__GNUC__ >= 3)
#define EASTL_LIKELY(x)   __builtin_expect(!!(x), true)
#define EASTL_UNLIKELY(x) __builtin_expect(!!(x), false) 
#else
#define EASTL_LIKELY(x)   (x)
#define EASTL_UNLIKELY(x) (x)
#endif
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL_MINMAX_ENABLED
//
// Defined as 0 or 1; default is 1.
// Specifies whether the min and max algorithms are available. 
// It may be useful to disable the min and max algorithems because sometimes
// #defines for min and max exist which would collide with EASTL min and max.
// Note that there are already alternative versions of min and max in EASTL
// with the min_alt and max_alt functions. You can use these without colliding
// with min/max macros that may exist.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef EASTL_MINMAX_ENABLED
#define EASTL_MINMAX_ENABLED 1
#endif

///////////////////////////////////////////////////////////////////////////////
// EASTL_NOMINMAX
//
// Defined as 0 or 1; default is 1.
// MSVC++ has #defines for min/max which collide with the min/max algorithm
// declarations. If EASTL_NOMINMAX is defined as 1, then we undefine min and 
// max if they are #defined by an external library. This allows our min and 
// max definitions in algorithm.h to work as expected. An alternative to 
// the enabling of EASTL_NOMINMAX is to #define NOMINMAX in your project 
// settings if you are compiling for Windows.
// Note that this does not control the availability of the EASTL min and max 
// algorithms; the EASTL_MINMAX_ENABLED configuration parameter does that.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_NOMINMAX
#define EASTL_NOMINMAX 1
#endif



///////////////////////////////////////////////////////////////////////////////
// AddRef / Release
//
// AddRef and Release are used for "intrusive" reference counting. By the term
// "intrusive", we mean that the reference count is maintained by the object 
// and not by the user of the object. Given that an object implements referencing 
// counting, the user of the object needs to be able to increment and decrement
// that reference count. We do that via the venerable AddRef and Release functions
// which the object must supply. These defines here allow us to specify the name
// of the functions. They could just as well be defined to addref and delref or 
// IncRef and DecRef.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTLAddRef
#define EASTLAddRef AddRef
#endif

#ifndef EASTLRelease
#define EASTLRelease Release
#endif




///////////////////////////////////////////////////////////////////////////////
// EASTL_ALLOCATOR_EXPLICIT_ENABLED
//
// Defined as 0 or 1. Default is 0 for now but ideally would be changed to 
// 1 some day. It's 0 because setting it to 1 breaks some existing code.
// This option enables the allocator ctor to be explicit, which avoids
// some undesirable silent conversions, especially with the string class.
//
// Example usage:
//     class allocator
//     {
//     public:
//         EASTL_ALLOCATOR_EXPLICIT allocator(const char* pName);
//     };
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTL_ALLOCATOR_EXPLICIT_ENABLED
#define EASTL_ALLOCATOR_EXPLICIT_ENABLED 0
#endif

#if EASTL_ALLOCATOR_EXPLICIT_ENABLED
#define EASTL_ALLOCATOR_EXPLICIT explicit
#else
#define EASTL_ALLOCATOR_EXPLICIT 
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL allocator
//
// The EASTL allocator system allows you to redefine how memory is allocated
// via some defines that are set up here. In the container code, memory is 
// allocated via macros which expand to whatever the user has them set to 
// expand to. Given that there are multiple allocator systems available, 
// this system allows you to configure it to use whatever system you want,
// provided your system meets the requirements of this library. 
// The requirements are:
//
//     - Must be constructable via a const char* (name) parameter.
//       Some uses of allocators won't require this, however.
//     - Allocate a block of memory of size n and debug name string.
//     - Allocate a block of memory of size n, debug name string, 
//       alignment a, and offset o.
//     - Free memory allocated via either of the allocation functions above.
//     - Provide a default allocator instance which can be used if the user 
//       doesn't provide a specific one.
//
///////////////////////////////////////////////////////////////////////////////

// namespace mimp
// {
//     class allocator
//     {
//         allocator(const char* pName = NULL);
//
//         void* allocate(size_t n, int flags = 0);
//         void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
//         void  deallocate(void* p, size_t n);
//
//         const char* get_name() const;
//         void        set_name(const char* pName);
//     };
//
//     allocator* GetDefaultAllocator(); // This is used for anonymous allocations.
// }

#ifndef EASTLAlloc // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.
#define EASTLAlloc(allocator, n) (allocator).allocate(n);
#endif

#ifndef EASTLAllocFlags // To consider: Instead of calling through pAllocator, just go directly to operator new, since that's what allocator does.
#define EASTLAllocFlags(allocator, n, flags) (allocator).allocate(n, flags);
#endif

#ifndef EASTLAllocAligned
#define EASTLAllocAligned(allocator, n, alignment, offset) (allocator).allocate((n), (alignment), (offset))
#endif

#ifndef EASTLFree
#define EASTLFree(allocator, p, size) (allocator).deallocate((p), (size))
#endif

#ifndef EASTLAllocatorType
#define EASTLAllocatorType mimp::allocator
#endif

#ifndef EASTLAllocatorDefault
// EASTLAllocatorDefault returns the default allocator instance. This is not a global 
// allocator which implements all container allocations but is the allocator that is 
// used when EASTL needs to allocate memory internally. There are very few cases where 
// EASTL allocates memory internally, and in each of these it is for a sensible reason 
// that is documented to behave as such.
#define EASTLAllocatorDefault mimp::GetDefaultAllocator
#endif


	///////////////////////////////////////////////////////////////////////
	// integral_constant
	//
	// This is the base class for various type traits, as defined by the proposed
	// C++ standard. This is essentially a utility base class for defining properties
	// as both class constants (value) and as types (type).
	//
	template <typename T, T v>
	struct integral_constant
	{
		static const T value = v;
		typedef T value_type;
		typedef integral_constant<T, v> type;
	};


	///////////////////////////////////////////////////////////////////////
	// true_type / false_type
	//
	// These are commonly used types in the implementation of type_traits.
	// Other integral constant types can be defined, such as those based on int.
	//
	typedef integral_constant<bool, true>  true_type;
	typedef integral_constant<bool, false> false_type;



	///////////////////////////////////////////////////////////////////////
	// yes_type / no_type
	//
	// These are used as a utility to differentiate between two things.
	//
	typedef char yes_type;                      // sizeof(yes_type) == 1
	struct       no_type { char padding[8]; };  // sizeof(no_type)  != 1



	///////////////////////////////////////////////////////////////////////
	// type_select
	//
	// This is used to declare a type from one of two type options. 
	// The result is based on the condition type. This has certain uses
	// in template metaprogramming.
	//
	// Example usage:
	//    typedef ChosenType = type_select<is_integral<SomeType>::value, ChoiceAType, ChoiceBType>::type;
	//
	template <bool bCondition, class ConditionIsTrueType, class ConditionIsFalseType>
	struct type_select { typedef ConditionIsTrueType type; };

	template <typename ConditionIsTrueType, class ConditionIsFalseType>
	struct type_select<false, ConditionIsTrueType, ConditionIsFalseType> { typedef ConditionIsFalseType type; };



	///////////////////////////////////////////////////////////////////////
	// type_or
	//
	// This is a utility class for creating composite type traits.
	//
	template <bool b1, bool b2, bool b3 = false, bool b4 = false, bool b5 = false>
	struct type_or;

	template <bool b1, bool b2, bool b3, bool b4, bool b5>
	struct type_or { static const bool value = true; };

	template <> 
	struct type_or<false, false, false, false, false> { static const bool value = false; };



	///////////////////////////////////////////////////////////////////////
	// type_and
	//
	// This is a utility class for creating composite type traits.
	//
	template <bool b1, bool b2, bool b3 = true, bool b4 = true, bool b5 = true>
	struct type_and;

	template <bool b1, bool b2, bool b3, bool b4, bool b5>
	struct type_and{ static const bool value = false; };

	template <>
	struct type_and<true, true, true, true, true>{ static const bool value = true; };



	///////////////////////////////////////////////////////////////////////
	// type_equal
	//
	// This is a utility class for creating composite type traits.
	//
	template <int b1, int b2>
	struct type_equal{ static const bool value = (b1 == b2); };



	///////////////////////////////////////////////////////////////////////
	// type_not_equal
	//
	// This is a utility class for creating composite type traits.
	//
	template <int b1, int b2>
	struct type_not_equal{ static const bool value = (b1 != b2); };



	///////////////////////////////////////////////////////////////////////
	// type_not
	//
	// This is a utility class for creating composite type traits.
	//
	template <bool b>
	struct type_not{ static const bool value = true; };

	template <>
	struct type_not<true>{ static const bool value = false; };



	///////////////////////////////////////////////////////////////////////
	// empty
	//
	template <typename T>
	struct empty{ };



// The following files implement the type traits themselves.
#if defined(__GNUC__) && (__GNUC__ <= 2)
#else

///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_fundamental.h
//
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////



	// The following properties or relations are defined here. If the given 
	// item is missing then it simply hasn't been implemented, at least not yet.


	///////////////////////////////////////////////////////////////////////
	// is_void
	//
	// is_void<T>::value == true if and only if T is one of the following types:
	//    [const][volatile] void
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_void : public false_type{};

	template <> struct is_void<void> : public true_type{};
	template <> struct is_void<void const> : public true_type{};
	template <> struct is_void<void volatile> : public true_type{};
	template <> struct is_void<void const volatile> : public true_type{};


	///////////////////////////////////////////////////////////////////////
	// is_integral
	//
	// is_integral<T>::value == true if and only if T  is one of the following types:
	//    [const] [volatile] bool
	//    [const] [volatile] char
	//    [const] [volatile] signed char
	//    [const] [volatile] unsigned char
	//    [const] [volatile] wchar_t
	//    [const] [volatile] short
	//    [const] [volatile] int
	//    [const] [volatile] long
	//    [const] [volatile] long long
	//    [const] [volatile] unsigned short
	//    [const] [volatile] unsigned int
	//    [const] [volatile] unsigned long
	//    [const] [volatile] unsigned long long
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_integral : public false_type{};

	// To do: Need to define volatile and const volatile versions of these.
	template <> struct is_integral<unsigned char>            : public true_type{};
	template <> struct is_integral<const unsigned char>      : public true_type{};
	template <> struct is_integral<unsigned short>           : public true_type{};
	template <> struct is_integral<const unsigned short>     : public true_type{};
	template <> struct is_integral<unsigned int>             : public true_type{};
	template <> struct is_integral<const unsigned int>       : public true_type{};
	template <> struct is_integral<unsigned long>            : public true_type{};
	template <> struct is_integral<const unsigned long>      : public true_type{};
	template <> struct is_integral<unsigned long long>       : public true_type{};
	template <> struct is_integral<const unsigned long long> : public true_type{};

	template <> struct is_integral<signed char>              : public true_type{};
	template <> struct is_integral<const signed char>        : public true_type{};
	template <> struct is_integral<signed short>             : public true_type{};
	template <> struct is_integral<const signed short>       : public true_type{};
	template <> struct is_integral<signed int>               : public true_type{};
	template <> struct is_integral<const signed int>         : public true_type{};
	template <> struct is_integral<signed long>              : public true_type{};
	template <> struct is_integral<const signed long>        : public true_type{};
	template <> struct is_integral<signed long long>         : public true_type{};
	template <> struct is_integral<const signed long long>   : public true_type{};

	template <> struct is_integral<bool>            : public true_type{};
	template <> struct is_integral<const bool>      : public true_type{};
	template <> struct is_integral<char>            : public true_type{};
	template <> struct is_integral<const char>      : public true_type{};
#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
	template <> struct is_integral<wchar_t>         : public true_type{};
	template <> struct is_integral<const wchar_t>   : public true_type{};
#endif

	///////////////////////////////////////////////////////////////////////
	// is_floating_point
	//
	// is_floating_point<T>::value == true if and only if T is one of the following types:
	//    [const] [volatile] float
	//    [const] [volatile] double
	//    [const] [volatile] long double
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_floating_point : public false_type{};

	// To do: Need to define volatile and const volatile versions of these.
	template <> struct is_floating_point<float>             : public true_type{};
	template <> struct is_floating_point<const float>       : public true_type{};
	template <> struct is_floating_point<double>            : public true_type{};
	template <> struct is_floating_point<const double>      : public true_type{};
	template <> struct is_floating_point<long double>       : public true_type{};
	template <> struct is_floating_point<const long double> : public true_type{};



	///////////////////////////////////////////////////////////////////////
	// is_arithmetic
	//
	// is_arithmetic<T>::value == true if and only if:
	//    is_floating_point<T>::value == true, or
	//    is_integral<T>::value == true
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> 
	struct is_arithmetic : public integral_constant<bool,
		is_integral<T>::value || is_floating_point<T>::value
	>{};


	///////////////////////////////////////////////////////////////////////
	// is_fundamental
	//
	// is_fundamental<T>::value == true if and only if:
	//    is_floating_point<T>::value == true, or
	//    is_integral<T>::value == true, or
	//    is_void<T>::value == true
	///////////////////////////////////////////////////////////////////////
	template <typename T> 
	struct is_fundamental : public integral_constant<bool,
		is_void<T>::value || is_integral<T>::value || is_floating_point<T>::value
	>{};

#endif

///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_transformations.h
// Written and maintained by Paul Pedriana - 2005
///////////////////////////////////////////////////////////////////////////////



	// The following transformations are defined here. If the given item 
	// is missing then it simply hasn't been implemented, at least not yet.
	//    add_unsigned
	//    add_signed
	//    remove_const
	//    remove_volatile
	//    remove_cv
	//    add_const
	//    add_volatile
	//    add_cv
	//    remove_reference
	//    add_reference
	//    remove_extent
	//    remove_all_extents
	//    remove_pointer
	//    add_pointer
	//    aligned_storage


	///////////////////////////////////////////////////////////////////////
	// add_signed
	//
	// Adds signed-ness to the given type. 
	// Modifies only integral values; has no effect on others.
	// add_signed<int>::type is int
	// add_signed<unsigned int>::type is int
	//
	///////////////////////////////////////////////////////////////////////

	template<class T>
	struct add_signed
	{ typedef T type; };

	template<>
	struct add_signed<unsigned char>
	{ typedef signed char type; };

#if (defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)) // If char is unsigned (which is usually not the case)...
	template<>
	struct add_signed<char>
	{ typedef signed char type; };
#endif

	template<>
	struct add_signed<unsigned short>
	{ typedef short type; };

	template<>
	struct add_signed<unsigned int>
	{ typedef int type; };

	template<>
	struct add_signed<unsigned long>
	{ typedef long type; };

	template<>
	struct add_signed<unsigned long long>
	{ typedef long long type; };

#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
#if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 4294967295U)) // If wchar_t is a 32 bit unsigned value...
	template<>
	struct add_signed<wchar_t>
	{ typedef int32_t type; };
#elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 65535)) // If wchar_t is a 16 bit unsigned value...
	template<>
	struct add_signed<wchar_t>
	{ typedef int16_t type; };
#endif
#endif



	///////////////////////////////////////////////////////////////////////
	// add_unsigned
	//
	// Adds unsigned-ness to the given type. 
	// Modifies only integral values; has no effect on others.
	// add_unsigned<int>::type is unsigned int
	// add_unsigned<unsigned int>::type is unsigned int
	//
	///////////////////////////////////////////////////////////////////////

	template<class T>
	struct add_unsigned
	{ typedef T type; };

	template<>
	struct add_unsigned<signed char>
	{ typedef unsigned char type; };

#if (defined(CHAR_MAX) && defined(SCHAR_MAX) && (CHAR_MAX == SCHAR_MAX)) // If char is signed (which is usually so)...
	template<>
	struct add_unsigned<char>
	{ typedef unsigned char type; };
#endif

	template<>
	struct add_unsigned<short>
	{ typedef unsigned short type; };

	template<>
	struct add_unsigned<int>
	{ typedef unsigned int type; };

	template<>
	struct add_unsigned<long>
	{ typedef unsigned long type; };

	template<>
	struct add_unsigned<long long>
	{ typedef unsigned long long type; };

#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
#if (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 2147483647)) // If wchar_t is a 32 bit signed value...
	template<>
	struct add_unsigned<wchar_t>
	{ typedef mimp::MiU32 type; };
#elif (defined(__WCHAR_MAX__) && (__WCHAR_MAX__ == 32767)) // If wchar_t is a 16 bit signed value...
	template<>
	struct add_unsigned<wchar_t>
	{ typedef MiU16 type; };
#endif
#endif

	///////////////////////////////////////////////////////////////////////
	// remove_cv
	//
	// Remove const and volatile from a type.
	//
	// The remove_cv transformation trait removes top-level const and/or volatile 
	// qualification (if any) from the type to which it is applied. For a given type T, 
	// remove_cv<T const volatile>::type is equivalent to T. For example, 
	// remove_cv<char* volatile>::type is equivalent to char*, while remove_cv<const char*>::type 
	// is equivalent to const char*. In the latter case, the const qualifier modifies 
	// char, not *, and is therefore not at the top level.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct remove_cv_imp{};
	template <typename T> struct remove_cv_imp<T*>                { typedef T unqualified_type; };
	template <typename T> struct remove_cv_imp<const T*>          { typedef T unqualified_type; };
	template <typename T> struct remove_cv_imp<volatile T*>       { typedef T unqualified_type; };
	template <typename T> struct remove_cv_imp<const volatile T*> { typedef T unqualified_type; };

	template <typename T> struct remove_cv{ typedef typename remove_cv_imp<T*>::unqualified_type type; };
	template <typename T> struct remove_cv<T&>{ typedef T& type; }; // References are automatically not const nor volatile. See section 8.3.2p1 of the C++ standard.

	template <typename T, size_t N> struct remove_cv<T const[N]>         { typedef T type[N]; };
	template <typename T, size_t N> struct remove_cv<T volatile[N]>      { typedef T type[N]; };
	template <typename T, size_t N> struct remove_cv<T const volatile[N]>{ typedef T type[N]; };



	///////////////////////////////////////////////////////////////////////
	// add_reference
	//
	// Add reference to a type.
	//
	// The add_reference transformation trait adds a level of indirection 
	// by reference to the type to which it is applied. For a given type T, 
	// add_reference<T>::type is equivalent to T& if is_reference<T>::value == false, 
	// and T otherwise.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct add_reference_impl{ typedef T& type; };

	template <typename T>
	struct add_reference_impl<T&>{ typedef T& type; };

	template <>
	struct add_reference_impl<void>{ typedef void type; };

	template <typename T>
	struct add_reference { typedef typename add_reference_impl<T>::type type; };


///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_properties.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////





	// The following properties or relations are defined here. If the given 
	// item is missing then it simply hasn't been implemented, at least not yet.

	///////////////////////////////////////////////////////////////////////
	// is_const
	//
	// is_const<T>::value == true if and only if T has const-qualification.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_const_value                    : public false_type{};
	template <typename T> struct is_const_value<const T*>          : public true_type{};
	template <typename T> struct is_const_value<const volatile T*> : public true_type{};

	template <typename T> struct is_const : public is_const_value<T*>{};
	template <typename T> struct is_const<T&> : public false_type{}; // Note here that T is const, not the reference to T. So is_const is false. See section 8.3.2p1 of the C++ standard.



	///////////////////////////////////////////////////////////////////////
	// is_volatile
	//
	// is_volatile<T>::value == true  if and only if T has volatile-qualification.
	//
	///////////////////////////////////////////////////////////////////////

	template <typename T> struct is_volatile_value                    : public false_type{};
	template <typename T> struct is_volatile_value<volatile T*>       : public true_type{};
	template <typename T> struct is_volatile_value<const volatile T*> : public true_type{};

	template <typename T> struct is_volatile : public is_volatile_value<T*>{};
	template <typename T> struct is_volatile<T&> : public false_type{}; // Note here that T is volatile, not the reference to T. So is_const is false. See section 8.3.2p1 of the C++ standard.



	///////////////////////////////////////////////////////////////////////
	// is_abstract
	//
	// is_abstract<T>::value == true if and only if T is a class or struct 
	// that has at least one pure virtual function. is_abstract may only 
	// be applied to complete types.
	//
	///////////////////////////////////////////////////////////////////////

	// Not implemented yet.



	///////////////////////////////////////////////////////////////////////
	// is_signed
	//
	// is_signed<T>::value == true if and only if T is one of the following types:
	//    [const] [volatile] char (maybe)
	//    [const] [volatile] signed char
	//    [const] [volatile] short
	//    [const] [volatile] int
	//    [const] [volatile] long
	//    [const] [volatile] long long
	// 
	// Used to determine if a integral type is signed or unsigned.
	// Given that there are some user-made classes which emulate integral
	// types, we provide the EASTL_DECLARE_SIGNED macro to allow you to 
	// set a given class to be identified as a signed type.
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_signed : public false_type{};

	template <> struct is_signed<signed char>              : public true_type{};
	template <> struct is_signed<const signed char>        : public true_type{};
	template <> struct is_signed<signed short>             : public true_type{};
	template <> struct is_signed<const signed short>       : public true_type{};
	template <> struct is_signed<signed int>               : public true_type{};
	template <> struct is_signed<const signed int>         : public true_type{};
	template <> struct is_signed<signed long>              : public true_type{};
	template <> struct is_signed<const signed long>        : public true_type{};
	template <> struct is_signed<signed long long>         : public true_type{};
	template <> struct is_signed<const signed long long>   : public true_type{};

#if (CHAR_MAX == SCHAR_MAX)
	template <> struct is_signed<char>            : public true_type{};
	template <> struct is_signed<const char>      : public true_type{};
#endif
#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
#if defined(__WCHAR_MAX__) && ((__WCHAR_MAX__ == 2147483647) || (__WCHAR_MAX__ == 32767)) // GCC defines __WCHAR_MAX__ for most platforms.
	template <> struct is_signed<wchar_t>         : public true_type{};
	template <> struct is_signed<const wchar_t>   : public true_type{};
#endif
#endif

#define EASTL_DECLARE_SIGNED(T) namespace mimp{ template <> struct is_signed<T> : public true_type{}; template <> struct is_signed<const T> : public true_type{}; }



	///////////////////////////////////////////////////////////////////////
	// is_unsigned
	//
	// is_unsigned<T>::value == true if and only if T is one of the following types:
	//    [const] [volatile] char (maybe)
	//    [const] [volatile] unsigned char
	//    [const] [volatile] unsigned short
	//    [const] [volatile] unsigned int
	//    [const] [volatile] unsigned long
	//    [const] [volatile] unsigned long long
	//
	// Used to determine if a integral type is signed or unsigned. 
	// Given that there are some user-made classes which emulate integral
	// types, we provide the EASTL_DECLARE_UNSIGNED macro to allow you to 
	// set a given class to be identified as an unsigned type.
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_unsigned : public false_type{};

	template <> struct is_unsigned<unsigned char>              : public true_type{};
	template <> struct is_unsigned<const unsigned char>        : public true_type{};
	template <> struct is_unsigned<unsigned short>             : public true_type{};
	template <> struct is_unsigned<const unsigned short>       : public true_type{};
	template <> struct is_unsigned<unsigned int>               : public true_type{};
	template <> struct is_unsigned<const unsigned int>         : public true_type{};
	template <> struct is_unsigned<unsigned long>              : public true_type{};
	template <> struct is_unsigned<const unsigned long>        : public true_type{};
	template <> struct is_unsigned<unsigned long long>         : public true_type{};
	template <> struct is_unsigned<const unsigned long long>   : public true_type{};

#if (CHAR_MAX == UCHAR_MAX)
	template <> struct is_unsigned<char>            : public true_type{};
	template <> struct is_unsigned<const char>      : public true_type{};
#endif
#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
#if defined(_MSC_VER) || (defined(__WCHAR_MAX__) && ((__WCHAR_MAX__ == 4294967295U) || (__WCHAR_MAX__ == 65535))) // GCC defines __WCHAR_MAX__ for most platforms.
	template <> struct is_unsigned<wchar_t>         : public true_type{};
	template <> struct is_unsigned<const wchar_t>   : public true_type{};
#endif
#endif

#define EASTL_DECLARE_UNSIGNED(T) namespace mimp{ template <> struct is_unsigned<T> : public true_type{}; template <> struct is_unsigned<const T> : public true_type{}; }



	///////////////////////////////////////////////////////////////////////
	// alignment_of
	//
	// alignment_of<T>::value is an integral value representing, in bytes, 
	// the memory alignment of objects of type T.
	//
	// alignment_of may only be applied to complete types.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct alignment_of_value{ static const size_t value = EASTL_ALIGN_OF(T); };

	template <typename T>
	struct alignment_of : public integral_constant<size_t, alignment_of_value<T>::value>{};



	///////////////////////////////////////////////////////////////////////
	// is_aligned
	//
	// Defined as true if the type has alignment requirements greater 
	// than default alignment, which is taken to be 8. This allows for 
	// doing specialized object allocation and placement for such types.
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct is_aligned_value{ static const bool value = (EASTL_ALIGN_OF(T) > 8); };

	template <typename T> 
	struct is_aligned : public integral_constant<bool, is_aligned_value<T>::value>{};



	///////////////////////////////////////////////////////////////////////
	// rank
	//
	// rank<T>::value is an integral value representing the number of 
	// dimensions possessed by an array type. For example, given a 
	// multi-dimensional array type T[M][N], std::tr1::rank<T[M][N]>::value == 2. 
	// For a given non-array type T, std::tr1::rank<T>::value == 0.
	//
	///////////////////////////////////////////////////////////////////////

	// Not implemented yet.



	///////////////////////////////////////////////////////////////////////
	// extent
	//
	// extent<T, I>::value is an integral type representing the number of 
	// elements in the Ith dimension of array type T.
	// 
	// For a given array type T[N], std::tr1::extent<T[N]>::value == N.
	// For a given multi-dimensional array type T[M][N], std::tr1::extent<T[M][N], 0>::value == N.
	// For a given multi-dimensional array type T[M][N], std::tr1::extent<T[M][N], 1>::value == M.
	// For a given array type T and a given dimension I where I >= rank<T>::value, std::tr1::extent<T, I>::value == 0.
	// For a given array type of unknown extent T[], std::tr1::extent<T[], 0>::value == 0.
	// For a given non-array type T and an arbitrary dimension I, std::tr1::extent<T, I>::value == 0.
	// 
	///////////////////////////////////////////////////////////////////////

	// Not implemented yet.



	///////////////////////////////////////////////////////////////////////
	// is_base_of
	//
	// Given two (possibly identical) types Base and Derived, is_base_of<Base, Derived>::value == true 
	// if and only if Base is a direct or indirect base class of Derived, 
	// or Base and Derived are the same type.
	//
	// is_base_of may only be applied to complete types.
	//
	///////////////////////////////////////////////////////////////////////

	// Not implemented yet.



///////////////////////////////////////////////////////////////////////////////
// EASTL/internal/type_compound.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////



	// The following properties or relations are defined here. If the given 
	// item is missing then it simply hasn't been implemented, at least not yet.
	//   is_array
	//   is_pointer
	//   is_reference
	//   is_member_object_pointer
	//   is_member_function_pointer
	//   is_member_pointer
	//   is_enum
	//   is_union
	//   is_class
	//   is_polymorphic
	//   is_function
	//   is_object
	//   is_scalar
	//   is_compound
	//   is_same
	//   is_convertible

	///////////////////////////////////////////////////////////////////////
	// is_array
	//
	// is_array<T>::value == true if and only if T is an array type.
	// As of this writing, the SNC compiler (EDG-based) doesn't compile
	// the code below and says that returning an array is illegal.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	T (*is_array_tester1(empty<T>))(empty<T>);
	char is_array_tester1(...);     // May need to use __cdecl under VC++.

	template <typename T>
	no_type  is_array_tester2(T(*)(empty<T>));
	yes_type is_array_tester2(...); // May need to use __cdecl under VC++.

	template <typename T>
	struct is_array_helper {
		static empty<T> emptyInstance;
	};

	template <typename T> 
	struct is_array : public integral_constant<bool,
		sizeof(is_array_tester2(is_array_tester1(is_array_helper<T>::emptyInstance))) == 1
	>{};



	///////////////////////////////////////////////////////////////////////
	// is_reference
	//
	// is_reference<T>::value == true if and only if T is a reference type. 
	// This category includes reference to function types.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_reference : public false_type{};
	template <typename T> struct is_reference<T&> : public true_type{};



	///////////////////////////////////////////////////////////////////////
	// is_member_function_pointer
	//
	// is_member_function_pointer<T>::value == true if and only if T is a 
	// pointer to member function type.
	//
	///////////////////////////////////////////////////////////////////////
	// We detect member functions with 0 to N arguments. We can extend this
	// for additional arguments if necessary.
	// To do: Make volatile and const volatile versions of these in addition to non-const and const.
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_mem_fun_pointer_value : public false_type{};
	template <typename R, typename T> struct is_mem_fun_pointer_value<R (T::*)()> : public true_type{};
	template <typename R, typename T> struct is_mem_fun_pointer_value<R (T::*)() const> : public true_type{};
	template <typename R, typename T, typename Arg0> struct is_mem_fun_pointer_value<R (T::*)(Arg0)> : public true_type{};
	template <typename R, typename T, typename Arg0> struct is_mem_fun_pointer_value<R (T::*)(Arg0) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6) const> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)> : public true_type{};
	template <typename R, typename T, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct is_mem_fun_pointer_value<R (T::*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7) const> : public true_type{};

	template <typename T> 
	struct is_member_function_pointer : public integral_constant<bool, is_mem_fun_pointer_value<T>::value>{};


	///////////////////////////////////////////////////////////////////////
	// is_member_pointer
	//
	// is_member_pointer<T>::value == true if and only if:
	//    is_member_object_pointer<T>::value == true, or
	//    is_member_function_pointer<T>::value == true
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> 
	struct is_member_pointer : public integral_constant<bool, is_member_function_pointer<T>::value>{};

	template <typename T, typename U> struct is_member_pointer<U T::*> : public true_type{};




	///////////////////////////////////////////////////////////////////////
	// is_pointer
	//
	// is_pointer<T>::value == true if and only if T is a pointer type. 
	// This category includes function pointer types, but not pointer to 
	// member types.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_pointer_helper : public false_type{};

	template <typename T> struct is_pointer_helper<T*>                : public true_type{};
	template <typename T> struct is_pointer_helper<T* const>          : public true_type{};
	template <typename T> struct is_pointer_helper<T* volatile>       : public true_type{};
	template <typename T> struct is_pointer_helper<T* const volatile> : public true_type{};

	template <typename T>
	struct is_pointer_value : public type_and<is_pointer_helper<T>::value, type_not<is_member_pointer<T>::value>::value> {};

	template <typename T> 
	struct is_pointer : public integral_constant<bool, is_pointer_value<T>::value>{};



	///////////////////////////////////////////////////////////////////////
	// is_same
	//
	// Given two (possibly identical) types T and U, is_same<T, U>::value == true 
	// if and only if T and U are the same type.
	//
	///////////////////////////////////////////////////////////////////////
	template<typename T, typename U>
	struct is_same : public false_type { };

	template<typename T>
	struct is_same<T, T> : public true_type { };


	///////////////////////////////////////////////////////////////////////
	// is_convertible
	//
	// Given two (possible identical) types From and To, is_convertible<From, To>::value == true 
	// if and only if an lvalue of type From can be implicitly converted to type To, 
	// or is_void<To>::value == true
	//
	// is_convertible may only be applied to complete types.
	// Type To may not be an abstract type. 
	// If the conversion is ambiguous, the program is ill-formed. 
	// If either or both of From and To are class types, and the conversion would invoke 
	// non-public member functions of either From or To (such as a private constructor of To, 
	// or a private conversion operator of From), the program is ill-formed.
	//
	// Note that without compiler help, both is_convertible and is_base 
	// can produce compiler errors if the conversion is ambiguous. 
	// Example:
	//    struct A {};
	//    struct B : A {};
	//    struct C : A {};
	//    struct D : B, C {};
	//    is_convertible<D*, A*>::value; // Generates compiler error.
	///////////////////////////////////////////////////////////////////////
#if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x doesn't like the code below.
	template <typename From, typename To, bool is_from_void = false, bool is_to_void = false>
	struct is_convertible_helper {
		static yes_type Test(To);  // May need to use __cdecl under VC++.
		static no_type  Test(...); // May need to use __cdecl under VC++.
		static From from;
		typedef integral_constant<bool, sizeof(Test(from)) == sizeof(yes_type)> result;
	};

	// void is not convertible to non-void
	template <typename From, typename To>
	struct is_convertible_helper<From, To, true, false> { typedef false_type result; };

	// Anything is convertible to void
	template <typename From, typename To, bool is_from_void>
	struct is_convertible_helper<From, To, is_from_void, true> { typedef true_type result; };

	template <typename From, typename To>
	struct is_convertible : public is_convertible_helper<From, To, is_void<From>::value, is_void<To>::value>::result {};

#else
	template <typename From, typename To>
	struct is_convertible : public false_type{};
#endif


	///////////////////////////////////////////////////////////////////////
	// is_union
	//
	// is_union<T>::value == true if and only if T is a union type.
	//
	// There is no way to tell if a type is a union without compiler help.
	// As of this writing, only Metrowerks v8+ supports such functionality
	// via 'msl::is_union<T>::value'. The user can force something to be 
	// evaluated as a union via EASTL_DECLARE_UNION.
	///////////////////////////////////////////////////////////////////////
	template <typename T> struct is_union : public false_type{};

#define EASTL_DECLARE_UNION(T) namespace mimp{ template <> struct is_union<T> : public true_type{}; template <> struct is_union<const T> : public true_type{}; }




	///////////////////////////////////////////////////////////////////////
	// is_class
	//
	// is_class<T>::value == true if and only if T is a class or struct 
	// type (and not a union type).
	//
	// Without specific compiler help, it is not possible to 
	// distinguish between unions and classes. As a result, is_class
	// will erroneously evaluate to true for union types.
	///////////////////////////////////////////////////////////////////////
#if defined(__MWERKS__)
	// To do: Switch this to use msl_utility type traits.
	template <typename T> 
	struct is_class : public false_type{};
#elif !defined(__GNUC__) || (((__GNUC__ * 100) + __GNUC_MINOR__) >= 304) // Not GCC or GCC 3.4+
	template <typename U> static yes_type is_class_helper(void (U::*)());
	template <typename U> static no_type  is_class_helper(...);

	template <typename T> 
	struct is_class : public integral_constant<bool,
		sizeof(is_class_helper<T>(0)) == sizeof(yes_type) && !is_union<T>::value
	>{};
#else
	// GCC 2.x version, due to GCC being broken.
	template <typename T> 
	struct is_class : public false_type{};
#endif



	///////////////////////////////////////////////////////////////////////
	// is_enum
	//
	// is_enum<T>::value == true if and only if T is an enumeration type.
	//
	///////////////////////////////////////////////////////////////////////
	struct int_convertible{ int_convertible(int); };

	template <bool is_arithmetic_or_reference>
	struct is_enum_helper { template <typename T> struct nest : public is_convertible<T, int_convertible>{}; };

	template <>
	struct is_enum_helper<true> { template <typename T> struct nest : public false_type {}; };

	template <typename T>
	struct is_enum_helper2
	{
		typedef type_or<is_arithmetic<T>::value, is_reference<T>::value, is_class<T>::value> selector;
		typedef is_enum_helper<selector::value> helper_t;
		typedef typename add_reference<T>::type ref_t;
		typedef typename helper_t::template nest<ref_t> result;
	};

	template <typename T> 
	struct is_enum : public integral_constant<bool, is_enum_helper2<T>::result::value>{};

	template <> struct is_enum<void> : public false_type {};
	template <> struct is_enum<void const> : public false_type {};
	template <> struct is_enum<void volatile> : public false_type {};
	template <> struct is_enum<void const volatile> : public false_type {};

#define EASTL_DECLARE_ENUM(T) namespace mimp{ template <> struct is_enum<T> : public true_type{}; template <> struct is_enum<const T> : public true_type{}; }


	///////////////////////////////////////////////////////////////////////
	// is_polymorphic
	// 
	// is_polymorphic<T>::value == true if and only if T is a class or struct 
	// that declares or inherits a virtual function. is_polymorphic may only 
	// be applied to complete types.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct is_polymorphic_imp1
	{
		typedef typename remove_cv<T>::type t;

		struct helper_1 : public t
		{
			helper_1();
			~helper_1() throw();
			char pad[64];
		};

		struct helper_2 : public t
		{
			helper_2();
			virtual ~helper_2() throw();
#ifndef _MSC_VER
			virtual void foo();
#endif
			char pad[64];
		};

		static const bool value = (sizeof(helper_1) == sizeof(helper_2));
	};

	template <typename T>
	struct is_polymorphic_imp2{ static const bool value = false; };

	template <bool is_class>
	struct is_polymorphic_selector{ template <typename T> struct rebind{ typedef is_polymorphic_imp2<T> type; }; };

	template <>
	struct is_polymorphic_selector<true>{ template <typename T> struct rebind{ typedef is_polymorphic_imp1<T> type; }; };

	template <typename T>
	struct is_polymorphic_value{
		typedef is_polymorphic_selector<is_class<T>::value> selector;
		typedef typename selector::template rebind<T> binder;
		typedef typename binder::type imp_type;
		static const bool value = imp_type::value;
	};

	template <typename T> 
	struct is_polymorphic : public integral_constant<bool, is_polymorphic_value<T>::value>{};




	///////////////////////////////////////////////////////////////////////
	// is_function
	//
	// is_function<T>::value == true  if and only if T is a function type.
	//
	///////////////////////////////////////////////////////////////////////
	template <typename R> struct is_function_ptr_helper : public false_type{};
	template <typename R> struct is_function_ptr_helper<R (*)()> : public true_type{};
	template <typename R, typename Arg0> struct is_function_ptr_helper<R (*)(Arg0)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1> struct is_function_ptr_helper<R (*)(Arg0, Arg1)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2, typename Arg3> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2, Arg3)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2, Arg3, Arg4)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6)> : public true_type{};
	template <typename R, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct is_function_ptr_helper<R (*)(Arg0, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7)> : public true_type{};

	template <bool is_ref = true>
	struct is_function_chooser{ template <typename T> struct result_ : public false_type{}; };

	template <>
	struct is_function_chooser<false>{ template <typename T> struct result_ : public is_function_ptr_helper<T*>{}; };

	template <typename T>
	struct is_function_value : public is_function_chooser<is_reference<T>::value>::template result_<T>{};

	template <typename T> 
	struct is_function : public integral_constant<bool, is_function_value<T>::value>{};




	///////////////////////////////////////////////////////////////////////
	// is_object
	//
	// is_object<T>::value == true if and only if:
	//    is_reference<T>::value == false, and
	//    is_function<T>::value == false, and
	//    is_void<T>::value == false
	//
	// The C++ standard, section 3.9p9, states: "An object type is a
	// (possibly cv-qualified) type that is not a function type, not a 
	// reference type, and not incomplete (except for an incompletely
	// defined object type).
	///////////////////////////////////////////////////////////////////////

	template <typename T> 
	struct is_object : public integral_constant<bool,
		!is_reference<T>::value && !is_void<T>::value && !is_function<T>::value
	>{};



	///////////////////////////////////////////////////////////////////////
	// is_scalar
	//
	// is_scalar<T>::value == true if and only if:
	//    is_arithmetic<T>::value == true, or
	//    is_enum<T>::value == true, or
	//    is_pointer<T>::value == true, or
	//    is_member_pointer<T>::value
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct is_scalar : public integral_constant<bool, is_arithmetic<T>::value || is_enum<T>::value>{};

	template <typename T> struct is_scalar<T*> : public true_type {};
	template <typename T> struct is_scalar<T* const> : public true_type {};
	template <typename T> struct is_scalar<T* volatile> : public true_type {};
	template <typename T> struct is_scalar<T* const volatile> : public true_type {};



	///////////////////////////////////////////////////////////////////////
	// is_compound
	//
	// Compound means anything but fundamental. See C++ standard, section 3.9.2.
	//
	// is_compound<T>::value == true if and only if:
	//    is_fundamental<T>::value == false
	//
	// Thus, is_compound<T>::value == true if and only if:
	//    is_floating_point<T>::value == false, and
	//    is_integral<T>::value == false, and
	//    is_void<T>::value == false
	//
	///////////////////////////////////////////////////////////////////////
	template <typename T> 
	struct is_compound : public integral_constant<bool, !is_fundamental<T>::value>{};



	// The following properties or relations are defined here. If the given 
	// item is missing then it simply hasn't been implemented, at least not yet.
	//    is_empty
	//    is_pod
	//    has_trivial_constructor
	//    has_trivial_copy
	//    has_trivial_assign
	//    has_trivial_destructor
	//    has_trivial_relocate       -- EA extension to the C++ standard proposal.
	//    has_nothrow_constructor
	//    has_nothrow_copy
	//    has_nothrow_assign
	//    has_virtual_destructor




	///////////////////////////////////////////////////////////////////////
	// is_empty
	// 
	// is_empty<T>::value == true if and only if T is an empty class or struct. 
	// is_empty may only be applied to complete types.
	//
	// is_empty cannot be used with union types until is_union can be made to work.
	///////////////////////////////////////////////////////////////////////
	template <typename T>
	struct is_empty_helper_t1 : public T { char m[64]; };
	struct is_empty_helper_t2            { char m[64]; };

	// The inheritance in empty_helper_t1 will not work with non-class types
	template <typename T, bool is_a_class = false>
	struct is_empty_helper : public false_type{};

	template <typename T>
	struct is_empty_helper<T, true> : public integral_constant<bool,
		sizeof(is_empty_helper_t1<T>) == sizeof(is_empty_helper_t2)
	>{};

	template <typename T>
	struct is_empty_helper2
	{
		typedef typename remove_cv<T>::type _T;
		typedef is_empty_helper<_T, is_class<_T>::value> type;
	};

	template <typename T> 
	struct is_empty : public is_empty_helper2<T>::type {};


	///////////////////////////////////////////////////////////////////////
	// is_pod
	// 
	// is_pod<T>::value == true if and only if, for a given type T:
	//    - is_scalar<T>::value == true, or
	//    - T is a class or struct that has no user-defined copy 
	//      assignment operator or destructor, and T has no non-static 
	//      data members M for which is_pod<M>::value == false, and no 
	//      members of reference type, or
	//    - T is a class or struct that has no user-defined copy assignment 
	//      operator or destructor, and T has no non-static data members M for 
	//      which is_pod<M>::value == false, and no members of reference type, or
	//    - T is the type of an array of objects E for which is_pod<E>::value == true
	//
	// is_pod may only be applied to complete types.
	//
	// Without some help from the compiler or user, is_pod will not report 
	// that a struct or class is a POD, but will correctly report that 
	// built-in types such as int are PODs. The user can help the compiler
	// by using the EASTL_DECLARE_POD macro on a class.
	///////////////////////////////////////////////////////////////////////
	template <typename T> // There's not much we can do here without some compiler extension.
	struct is_pod : public integral_constant<bool, is_void<T>::value || is_scalar<T>::value>{};

	template <typename T, size_t N>
	struct is_pod<T[N]> : public is_pod<T>{};

	template <typename T>
	struct is_POD : public is_pod<T>{};

#define EASTL_DECLARE_POD(T) namespace mimp{ template <> struct is_pod<T> : public true_type{}; template <> struct is_pod<const T> : public true_type{}; }




	///////////////////////////////////////////////////////////////////////
	// has_trivial_constructor
	//
	// has_trivial_constructor<T>::value == true if and only if T is a class 
	// or struct that has a trivial constructor. A constructor is trivial if
	//    - it is implicitly defined by the compiler, and
	//    - is_polymorphic<T>::value == false, and
	//    - T has no virtual base classes, and
	//    - for every direct base class of T, has_trivial_constructor<B>::value == true, 
	//      where B is the type of the base class, and
	//    - for every nonstatic data member of T that has class type or array 
	//      of class type, has_trivial_constructor<M>::value == true, 
	//      where M is the type of the data member
	//
	// has_trivial_constructor may only be applied to complete types.
	//
	// Without from the compiler or user, has_trivial_constructor will not 
	// report that a class or struct has a trivial constructor. 
	// The user can use EASTL_DECLARE_TRIVIAL_CONSTRUCTOR to help the compiler.
	//
	// A default constructor for a class X is a constructor of class X that 
	// can be called without an argument.
	///////////////////////////////////////////////////////////////////////

	// With current compilers, this is all we can do.
	template <typename T> 
	struct has_trivial_constructor : public is_pod<T> {};

#define EASTL_DECLARE_TRIVIAL_CONSTRUCTOR(T) namespace mimp{ template <> struct has_trivial_constructor<T> : public true_type{}; template <> struct has_trivial_constructor<const T> : public true_type{}; }




	///////////////////////////////////////////////////////////////////////
	// has_trivial_copy
	//
	// has_trivial_copy<T>::value == true if and only if T is a class or 
	// struct that has a trivial copy constructor. A copy constructor is 
	// trivial if
	//   - it is implicitly defined by the compiler, and
	//   - is_polymorphic<T>::value == false, and
	//   - T has no virtual base classes, and
	//   - for every direct base class of T, has_trivial_copy<B>::value == true, 
	//     where B is the type of the base class, and
	//   - for every nonstatic data member of T that has class type or array 
	//     of class type, has_trivial_copy<M>::value == true, where M is the 
	//     type of the data member
	//
	// has_trivial_copy may only be applied to complete types.
	//
	// Another way of looking at this is:
	// A copy constructor for class X is trivial if it is implicitly 
	// declared and if all the following are true:
	//    - Class X has no virtual functions (10.3) and no virtual base classes (10.1).
	//    - Each direct base class of X has a trivial copy constructor.
	//    - For all the nonstatic data members of X that are of class type 
	//      (or array thereof), each such class type has a trivial copy constructor;
	//      otherwise the copy constructor is nontrivial.
	//
	// Without from the compiler or user, has_trivial_copy will not report 
	// that a class or struct has a trivial copy constructor. The user can 
	// use EASTL_DECLARE_TRIVIAL_COPY to help the compiler.
	///////////////////////////////////////////////////////////////////////

	template <typename T> 
	struct has_trivial_copy : public integral_constant<bool, is_pod<T>::value && !is_volatile<T>::value>{};

#define EASTL_DECLARE_TRIVIAL_COPY(T) namespace mimp{ template <> struct has_trivial_copy<T> : public true_type{}; template <> struct has_trivial_copy<const T> : public true_type{}; }


	///////////////////////////////////////////////////////////////////////
	// has_trivial_assign
	//
	// has_trivial_assign<T>::value == true if and only if T is a class or 
	// struct that has a trivial copy assignment operator. A copy assignment 
	// operator is trivial if:
	//    - it is implicitly defined by the compiler, and
	//    - is_polymorphic<T>::value == false, and
	//    - T has no virtual base classes, and
	//    - for every direct base class of T, has_trivial_assign<B>::value == true, 
	//      where B is the type of the base class, and
	//    - for every nonstatic data member of T that has class type or array 
	//      of class type, has_trivial_assign<M>::value == true, where M is 
	//      the type of the data member.
	//
	// has_trivial_assign may only be applied to complete types.
	//
	// Without  from the compiler or user, has_trivial_assign will not 
	// report that a class or struct has trivial assignment. The user 
	// can use EASTL_DECLARE_TRIVIAL_ASSIGN to help the compiler.
	///////////////////////////////////////////////////////////////////////

	template <typename T> 
	struct has_trivial_assign : public integral_constant<bool,
		is_pod<T>::value && !is_const<T>::value && !is_volatile<T>::value
	>{};

#define EASTL_DECLARE_TRIVIAL_ASSIGN(T) namespace mimp{ template <> struct has_trivial_assign<T> : public true_type{}; template <> struct has_trivial_assign<const T> : public true_type{}; }




	///////////////////////////////////////////////////////////////////////
	// has_trivial_destructor
	//
	// has_trivial_destructor<T>::value == true if and only if T is a class 
	// or struct that has a trivial destructor. A destructor is trivial if
	//    - it is implicitly defined by the compiler, and
	//    - for every direct base class of T, has_trivial_destructor<B>::value == true, 
	//      where B is the type of the base class, and
	//    - for every nonstatic data member of T that has class type or 
	//      array of class type, has_trivial_destructor<M>::value == true, 
	//      where M is the type of the data member
	//
	// has_trivial_destructor may only be applied to complete types.
	//
	// Without from the compiler or user, has_trivial_destructor will not 
	// report that a class or struct has a trivial destructor. 
	// The user can use EASTL_DECLARE_TRIVIAL_DESTRUCTOR to help the compiler.
	///////////////////////////////////////////////////////////////////////

	// With current compilers, this is all we can do.
	template <typename T> 
	struct has_trivial_destructor : public is_pod<T>{};

#define EASTL_DECLARE_TRIVIAL_DESTRUCTOR(T) namespace mimp{ template <> struct has_trivial_destructor<T> : public true_type{}; template <> struct has_trivial_destructor<const T> : public true_type{}; }


	///////////////////////////////////////////////////////////////////////
	// has_trivial_relocate
	//
	// This is an EA extension to the type traits standard.
	//
	// A trivially relocatable object is one that can be safely memmove'd 
	// to uninitialized memory. construction, assignment, and destruction 
	// properties are not addressed by this trait. A type that has the 
	// is_fundamental trait would always have the has_trivial_relocate trait. 
	// A type that has the has_trivial_constructor, has_trivial_copy or 
	// has_trivial_assign traits would usally have the has_trivial_relocate 
	// trait, but this is not strictly guaranteed.
	//
	// The user can use EASTL_DECLARE_TRIVIAL_RELOCATE to help the compiler.
	///////////////////////////////////////////////////////////////////////

	// With current compilers, this is all we can do.
	template <typename T> 
	struct has_trivial_relocate : public integral_constant<bool, is_pod<T>::value && !is_volatile<T>::value>{};

#define EASTL_DECLARE_TRIVIAL_RELOCATE(T) namespace mimp{ template <> struct has_trivial_relocate<T> : public true_type{}; template <> struct has_trivial_relocate<const T> : public true_type{}; }


///////////////////////////////////////////////////////////////////////////////
// EASTL/allocator.h
//
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////

	/// EASTL_ALLOCATOR_DEFAULT_NAME
	///
	/// Defines a default allocator name in the absence of a user-provided name.
	///
#ifndef EASTL_ALLOCATOR_DEFAULT_NAME
#define EASTL_ALLOCATOR_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX // Unless the user overrides something, this is "EASTL".
#endif


	/// alloc_flags
	///
	/// Defines allocation flags.
	///
	enum alloc_flags 
	{
		MEM_TEMP = 0, // Low memory, not necessarily actually temporary.
		MEM_PERM = 1  // High memory, for things that won't be unloaded.
	};


	/// allocator
	///
	/// In this allocator class, note that it is not templated on any type and
	/// instead it simply allocates blocks of memory much like the C malloc and
	/// free functions. It can be thought of as similar to C++ std::allocator<char>.
	/// The flags parameter has meaning that is specific to the allocation 
	///
	class  allocator
	{
	public:
		EASTL_ALLOCATOR_EXPLICIT allocator(const char* pName = EASTL_NAME_VAL(EASTL_ALLOCATOR_DEFAULT_NAME));
		allocator(const allocator& x);
		allocator(const allocator& x, const char* pName);

		allocator& operator=(const allocator& x);

		void* allocate(size_t n, int flags = 0);
		void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
		void  deallocate(void* p, size_t n);

		const char* get_name() const;
		void        set_name(const char* pName);

	protected:
#if EASTL_NAME_ENABLED
		const char* mpName; // Debug name, used to track memory.
#endif
	};

	bool operator==(const allocator& a, const allocator& b);
	bool operator!=(const allocator& a, const allocator& b);

	 allocator* GetDefaultAllocator();
	 allocator* SetDefaultAllocator(allocator* pAllocator);



	/// get_default_allocator
	///
	/// This templated function allows the user to implement a default allocator
	/// retrieval function that any part of EASTL can use. EASTL containers take
	/// an Allocator parameter which identifies an Allocator class to use. But 
	/// different kinds of allocators have different mechanisms for retrieving 
	/// a default allocator instance, and some don't even intrinsically support
	/// such functionality. The user can override this get_default_allocator 
	/// function in order to provide the glue between EASTL and whatever their
	/// system's default allocator happens to be.
	///
	/// Example usage:
	///     MyAllocatorType* gpSystemAllocator;
	///     
	///     MyAllocatorType* get_default_allocator(const MyAllocatorType*)
	///         { return gpSystemAllocator; }
	///
	template <typename Allocator>
	inline Allocator* get_default_allocator(const Allocator*)
	{
		return NULL; // By default we return NULL; the user must make specialization of this function in order to provide their own implementation.
	}

	inline EASTLAllocatorType* get_default_allocator(const EASTLAllocatorType*)
	{
		return EASTLAllocatorDefault(); // For the built-in allocator EASTLAllocatorType, we happen to already have a function for returning the default allocator instance, so we provide it.
	}


	/// default_allocfreemethod
	///
	/// Implements a default allocfreemethod which uses the default global allocator.
	/// This version supports only default alignment.
	///
	inline void* default_allocfreemethod(size_t n, void* pBuffer, void* /*pContext*/)
	{
		EASTLAllocatorType* const pAllocator = EASTLAllocatorDefault();

		if(pBuffer) // If freeing...
		{
			EASTLFree(*pAllocator, pBuffer, n);
			return NULL;  // The return value is meaningless for the free.
		}
		else // allocating
			return EASTLAlloc(*pAllocator, n);
	}


	/// allocate_memory
	///
	/// This is a memory allocation dispatching function.
	/// To do: Make aligned and unaligned specializations.
	///        Note that to do this we will need to use a class with a static
	///        function instead of a standalone function like below.
	///
	template <typename Allocator>
	void* allocate_memory(Allocator& a, size_t n, size_t alignment, size_t alignmentOffset)
	{
		if(alignment <= 8)
			return EASTLAlloc(a, n);
		return EASTLAllocAligned(a, n, alignment, alignmentOffset);
	}


#ifndef EASTL_USER_DEFINED_ALLOCATOR // If the user hasn't declared that he has defined a different allocator implementation elsewhere...

	inline allocator::allocator(const char* EASTL_NAME(pName))
	{
#if EASTL_NAME_ENABLED
		mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
	}


	inline allocator::allocator(const allocator& EASTL_NAME(alloc))
	{
#if EASTL_NAME_ENABLED
		mpName = alloc.mpName;
#endif
	}


	inline allocator::allocator(const allocator&, const char* EASTL_NAME(pName))
	{
#if EASTL_NAME_ENABLED
		mpName = pName ? pName : EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
	}


	inline allocator& allocator::operator=(const allocator& EASTL_NAME(alloc))
	{
#if EASTL_NAME_ENABLED
		mpName = alloc.mpName;
#endif
		return *this;
	}


	inline const char* allocator::get_name() const
	{
#if EASTL_NAME_ENABLED
		return mpName;
#else
		return EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
	}


	inline void allocator::set_name(const char* EASTL_NAME(pName))
	{
#if EASTL_NAME_ENABLED
		mpName = pName;
#endif
	}


	inline void* allocator::allocate(size_t n, int /*flags*/)
	{
#if EASTL_NAME_ENABLED
		return MI_ALLOC(n);
#else
		return MI_ALLOC(n);
#endif
	}


	inline void* allocator::allocate(size_t n, size_t /*alignment*/, size_t /*offset*/, int /*flags*/)
	{
#if EASTL_NAME_ENABLED
		return MI_ALLOC(n);
#else
		return MI_ALLOC(n);
#endif
	}


	inline void allocator::deallocate(void* p, size_t)
	{
		MI_FREE(p);
	}


	inline bool operator==(const allocator&, const allocator&)
	{
		return true; // All allocators are considered equal, as they merely use global new/delete.
	}


	inline bool operator!=(const allocator&, const allocator&)
	{
		return false; // All allocators are considered equal, as they merely use global new/delete.
	}



#endif

///////////////////////////////////////////////////////////////////////////////
// EASTL/iterator.h
//
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////



	/// iterator_status_flag
	/// 
	/// Defines the validity status of an iterator. This is primarily used for 
	/// iterator validation in debug builds. These are implemented as OR-able 
	/// flags (as opposed to mutually exclusive values) in order to deal with 
	/// the nature of iterator status. In particular, an iterator may be valid
	/// but not dereferencable, as in the case with an iterator to container end().
	/// An iterator may be valid but also dereferencable, as in the case with an
	/// iterator to container begin().
	///
	enum iterator_status_flag
	{
		isf_none            = 0x00, /// This is called none and not called invalid because it is not strictly the opposite of invalid.
		isf_valid           = 0x01, /// The iterator is valid, which means it is in the range of [begin, end].
		isf_current         = 0x02, /// The iterator is valid and points to the same element it did when created. For example, if an iterator points to vector::begin() but an element is inserted at the front, the iterator is valid but not current. Modification of elements in place do not make iterators non-current.
		isf_can_dereference = 0x04  /// The iterator is dereferencable, which means it is in the range of [begin, end). It may or may not be current.
	};



	// The following declarations are taken directly from the C++ standard document.
	//    input_iterator_tag, etc.
	//    iterator
	//    iterator_traits
	//    reverse_iterator

	// Iterator categories
	// Every iterator is defined as belonging to one of the iterator categories that
	// we define here. These categories come directly from the C++ standard.
	struct input_iterator_tag { };
	struct output_iterator_tag { };
	struct forward_iterator_tag       : public input_iterator_tag { };
	struct bidirectional_iterator_tag : public forward_iterator_tag { };
	struct random_access_iterator_tag : public bidirectional_iterator_tag { };
	struct contiguous_iterator_tag    : public random_access_iterator_tag { };  // Extension to the C++ standard. Contiguous ranges are more than random access, they are physically contiguous.

	// struct iterator
	template <typename Category, typename T, typename Distance = ptrdiff_t, 
		typename Pointer = T*, typename Reference = T&>
	struct iterator
	{
		typedef Category  iterator_category;
		typedef T         value_type;
		typedef Distance  difference_type;
		typedef Pointer   pointer;
		typedef Reference reference;
	};


	// struct iterator_traits
	template <typename Iterator>
	struct iterator_traits
	{
		typedef typename Iterator::iterator_category iterator_category;
		typedef typename Iterator::value_type        value_type;
		typedef typename Iterator::difference_type   difference_type;
		typedef typename Iterator::pointer           pointer;
		typedef typename Iterator::reference         reference;
	};

	template <typename T>
	struct iterator_traits<T*>
	{
		typedef EASTL_ITC_NS::random_access_iterator_tag iterator_category;     // To consider: Change this to contiguous_iterator_tag for the case that 
		typedef T                                        value_type;            //              EASTL_ITC_NS is "mimp" instead of "std".
		typedef ptrdiff_t                                difference_type;
		typedef T*                                       pointer;
		typedef T&                                       reference;
	};

	template <typename T>
	struct iterator_traits<const T*>
	{
		typedef EASTL_ITC_NS::random_access_iterator_tag iterator_category;
		typedef T                                        value_type;
		typedef ptrdiff_t                                difference_type;
		typedef const T*                                 pointer;
		typedef const T&                                 reference;
	};





	/// reverse_iterator
	///
	/// From the C++ standard:
	/// Bidirectional and random access iterators have corresponding reverse 
	/// iterator adaptors that iterate through the data structure in the 
	/// opposite direction. They have the same signatures as the corresponding 
	/// iterators. The fundamental relation between a reverse iterator and its 
	/// corresponding iterator i is established by the identity:
	///     &*(reverse_iterator(i)) == &*(i - 1).
	/// This mapping is dictated by the fact that while there is always a pointer 
	/// past the end of an array, there might not be a valid pointer before the
	/// beginning of an array.
	///
	template <typename Iterator>
	class reverse_iterator : public iterator<typename mimp::iterator_traits<Iterator>::iterator_category,
		typename mimp::iterator_traits<Iterator>::value_type,
		typename mimp::iterator_traits<Iterator>::difference_type,
		typename mimp::iterator_traits<Iterator>::pointer,
		typename mimp::iterator_traits<Iterator>::reference>
	{
	public:
		typedef Iterator                                                   iterator_type;
		typedef typename mimp::iterator_traits<Iterator>::pointer         pointer;
		typedef typename mimp::iterator_traits<Iterator>::reference       reference;
		typedef typename mimp::iterator_traits<Iterator>::difference_type difference_type;

	protected:
		Iterator mIterator;

	public:
		reverse_iterator()      // It's important that we construct mIterator, because if Iterator  
			: mIterator() { }   // is a pointer, there's a difference between doing it and not.

		explicit reverse_iterator(iterator_type i)
			: mIterator(i) { }

		reverse_iterator(const reverse_iterator& ri)
			: mIterator(ri.mIterator) { }

		template <typename U>
		reverse_iterator(const reverse_iterator<U>& ri)
			: mIterator(ri.base()) { }

		// This operator= isn't in the standard, but the the C++ 
		// library working group has tentatively approved it, as it
		// allows const and non-const reverse_iterators to interoperate.
		template <typename U>
		reverse_iterator<Iterator>& operator=(const reverse_iterator<U>& ri)
		{ mIterator = ri.base(); return *this; }

		iterator_type base() const
		{ return mIterator; }

		reference operator*() const
		{
			iterator_type i(mIterator);
			return *--i;
		}

		pointer operator->() const
		{ return &(operator*()); }

		reverse_iterator& operator++()
		{ --mIterator; return *this; }

		reverse_iterator operator++(int)
		{
			reverse_iterator ri(*this);
			--mIterator;
			return ri;
		}

		reverse_iterator& operator--()
		{ ++mIterator; return *this; }

		reverse_iterator operator--(int)
		{
			reverse_iterator ri(*this);
			++mIterator;
			return ri;
		}

		reverse_iterator operator+(difference_type n) const
		{ return reverse_iterator(mIterator - n); }

		reverse_iterator& operator+=(difference_type n)
		{ mIterator -= n; return *this; }

		reverse_iterator operator-(difference_type n) const
		{ return reverse_iterator(mIterator + n); }

		reverse_iterator& operator-=(difference_type n)
		{ mIterator += n; return *this; }

		reference operator[](difference_type n) const
		{ return mIterator[-n - 1]; } 
	};


	// The C++ library working group has tentatively approved the usage of two
	// template parameters (Iterator1 and Iterator2) in order to allow reverse_iterators
	// and const_reverse iterators to be comparable. This is a similar issue to the 
	// C++ defect report #179 regarding comparison of container iterators and const_iterators.
	template <typename Iterator1, typename Iterator2>
	inline bool
		operator==(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() == b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline bool
		operator<(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() > b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline bool
		operator!=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() != b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline bool
		operator>(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() < b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline bool
		operator<=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() >= b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline bool
		operator>=(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return a.base() <= b.base(); }


	template <typename Iterator1, typename Iterator2>
	inline typename reverse_iterator<Iterator1>::difference_type
		operator-(const reverse_iterator<Iterator1>& a, const reverse_iterator<Iterator2>& b)
	{ return b.base() - a.base(); }


	template <typename Iterator>
	inline reverse_iterator<Iterator>
		operator+(typename reverse_iterator<Iterator>::difference_type n, const reverse_iterator<Iterator>& a)
	{ return reverse_iterator<Iterator>(a.base() - n); }







	/// back_insert_iterator
	///
	/// A back_insert_iterator is simply a class that acts like an iterator but when you 
	/// assign a value to it, it calls push_back on the container with the value.
	///
	template <typename Container>
	class back_insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
	{
	public:
		typedef Container                           container_type;
		typedef typename Container::const_reference const_reference;

	protected:
		Container& container;

	public:
		explicit back_insert_iterator(Container& x)
			: container(x) { }

		back_insert_iterator& operator=(const_reference value)
		{ container.push_back(value); return *this; }

		back_insert_iterator& operator*()
		{ return *this; }

		back_insert_iterator& operator++()
		{ return *this; } // This is by design.

		back_insert_iterator operator++(int)
		{ return *this; } // This is by design.
	};


	/// back_inserter
	///
	/// Creates an instance of a back_insert_iterator.
	///
	template <typename Container>
	inline back_insert_iterator<Container>
		back_inserter(Container& x)
	{ return back_insert_iterator<Container>(x); }




	/// front_insert_iterator
	///
	/// A front_insert_iterator is simply a class that acts like an iterator but when you 
	/// assign a value to it, it calls push_front on the container with the value.
	///
	template <typename Container>
	class front_insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
	{
	public:
		typedef Container                           container_type;
		typedef typename Container::const_reference const_reference;

	protected:
		Container& container;

	public:
		explicit front_insert_iterator(Container& x)
			: container(x) { }

		front_insert_iterator& operator=(const_reference value)
		{ container.push_front(value); return *this; }

		front_insert_iterator& operator*()
		{ return *this; }

		front_insert_iterator& operator++()
		{ return *this; } // This is by design.

		front_insert_iterator operator++(int)
		{ return *this; } // This is by design.
	};


	/// front_inserter
	///
	/// Creates an instance of a front_insert_iterator.
	///
	template <typename Container>
	inline front_insert_iterator<Container>
		front_inserter(Container& x)
	{ return front_insert_iterator<Container>(x); }




	/// insert_iterator
	///
	/// An insert_iterator is like an iterator except that when you assign a value to it, 
	/// the insert_iterator inserts the value into the container and increments the iterator.
	///
	/// insert_iterator is an iterator adaptor that functions as an OutputIterator: 
	/// assignment through an insert_iterator inserts an object into a container. 
	/// Specifically, if ii is an insert_iterator, then ii keeps track of a container c and 
	/// an insertion point p; the expression *ii = x performs the insertion c.insert(p, x).
	///
	/// If you assign through an insert_iterator several times, then you will be inserting 
	/// several elements into the underlying container. In the case of a sequence, they will 
	/// appear at a particular location in the underlying sequence, in the order in which 
	/// they were inserted: one of the arguments to insert_iterator's constructor is an 
	/// iterator p, and the new range will be inserted immediately before p.
	///
	template <typename Container>
	class insert_iterator : public iterator<EASTL_ITC_NS::output_iterator_tag, void, void, void, void>
	{
	public:
		typedef Container                           container_type;
		typedef typename Container::iterator        iterator_type;
		typedef typename Container::const_reference const_reference;

	protected:
		Container&     container;
		iterator_type  it; 

	public:
		// This assignment operator is defined more to stop compiler warnings (e.g. VC++ C4512)
		// than to be useful. However, it does an insert_iterator to be assigned to another 
		// insert iterator provided that they point to the same container.
		insert_iterator& operator=(const insert_iterator& x)
		{
			EASTL_ASSERT(&x.container == &container);
			it = x.it;
			return *this;
		}

		insert_iterator(Container& x, iterator_type itNew)
			: container(x), it(itNew) {}

		insert_iterator& operator=(const_reference value)
		{
			it = container.insert(it, value);
			++it;
			return *this;
		}

		insert_iterator& operator*()
		{ return *this; }

		insert_iterator& operator++()
		{ return *this; } // This is by design.

		insert_iterator& operator++(int)
		{ return *this; } // This is by design.

	}; // insert_iterator 


	/// inserter
	///
	/// Creates an instance of an insert_iterator.
	///
	template <typename Container, typename Iterator>
	inline mimp::insert_iterator<Container>
		inserter(Container& x, Iterator i)
	{
		typedef typename Container::iterator iterator;
		return mimp::insert_iterator<Container>(x, iterator(i));
	}




	//////////////////////////////////////////////////////////////////////////////////
	/// distance
	///
	/// Implements the distance() function. There are two versions, one for
	/// random access iterators (e.g. with vector) and one for regular input
	/// iterators (e.g. with list). The former is more efficient.
	///
	template <typename InputIterator>
	inline typename mimp::iterator_traits<InputIterator>::difference_type
		distance_impl(InputIterator first, InputIterator last, EASTL_ITC_NS::input_iterator_tag)
	{
		typename mimp::iterator_traits<InputIterator>::difference_type n = 0;

		while(first != last)
		{
			++first;
			++n;
		}
		return n;
	}

	template <typename RandomAccessIterator>
	inline typename mimp::iterator_traits<RandomAccessIterator>::difference_type
		distance_impl(RandomAccessIterator first, RandomAccessIterator last, EASTL_ITC_NS::random_access_iterator_tag)
	{
		return last - first;
	}

	// Special version defined so that std C++ iterators can be recognized by 
	// this function. Unfortunately, this function treats all foreign iterators
	// as InputIterators and thus can seriously hamper performance in the case
	// of large ranges of bidirectional_iterator_tag iterators.
	//template <typename InputIterator>
	//inline typename mimp::iterator_traits<InputIterator>::difference_type
	//distance_impl(InputIterator first, InputIterator last, ...)
	//{
	//    typename mimp::iterator_traits<InputIterator>::difference_type n = 0;
	//
	//    while(first != last)
	//    {
	//        ++first;
	//        ++n;
	//    }
	//    return n;
	//}

	template <typename InputIterator>
	inline typename mimp::iterator_traits<InputIterator>::difference_type
		distance(InputIterator first, InputIterator last)
	{
		typedef typename mimp::iterator_traits<InputIterator>::iterator_category IC;

		return mimp::distance_impl(first, last, IC());
	}




	//////////////////////////////////////////////////////////////////////////////////
	/// advance
	///
	/// Implements the advance() function. There are three versions, one for
	/// random access iterators (e.g. with vector), one for bidirectional 
	/// iterators (list) and one for regular input iterators (e.g. with slist). 
	///
	template <typename InputIterator, typename Distance>
	inline void
		advance_impl(InputIterator& i, Distance n, EASTL_ITC_NS::input_iterator_tag)
	{
		while(n--)
			++i;
	}

	template <typename BidirectionalIterator, typename Distance>
	inline void
		advance_impl(BidirectionalIterator& i, Distance n, EASTL_ITC_NS::bidirectional_iterator_tag)
	{
		if(n > 0)
		{
			while(n--)
				++i;
		}
		else
		{
			while(n++)
				--i;
		}
	}

	template <typename RandomAccessIterator, typename Distance>
	inline void
		advance_impl(RandomAccessIterator& i, Distance n, EASTL_ITC_NS::random_access_iterator_tag)
	{
		i += n;
	}

	// Special version defined so that std C++ iterators can be recognized by 
	// this function. Unfortunately, this function treats all foreign iterators
	// as InputIterators and thus can seriously hamper performance in the case
	// of large ranges of bidirectional_iterator_tag iterators.
	//template <typename InputIterator, typename Distance>
	//inline void
	//advance_impl(InputIterator& i, Distance n, ...)
	//{
	//    while(n--)
	//        ++i;
	//}

	template <typename InputIterator, typename Distance>
	inline void
		advance(InputIterator& i, Distance n)
	{
		typedef typename mimp::iterator_traits<InputIterator>::iterator_category IC;

		mimp::advance_impl(i, n, IC());
	}



///////////////////////////////////////////////////////////////////////////////
// EASTL/utility.h
// Written and maintained by Paul Pedriana - 2005.
///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////
	/// rel_ops
	///
	/// rel_ops allow the automatic generation of operators !=, >, <=, >= from
	/// just operators == and <. These are intentionally in the rel_ops namespace
	/// so that they don't conflict with other similar operators. To use these 
	/// operators, add "using namespace std::rel_ops;" to an appropriate place in 
	/// your code, usually right in the function that you need them to work.
	/// In fact, you will very likely have collision problems if you put such
	/// using statements anywhere other than in the .cpp file like so and may 
	/// also have collisions when you do, as the using statement will affect all
	/// code in the module. You need to be careful about use of rel_ops.
	///
	namespace rel_ops
	{
		template <typename T>
		inline bool operator!=(const T& x, const T& y)
		{ return !(x == y); }

		template <typename T>
		inline bool operator>(const T& x, const T& y)
		{ return (y < x); }

		template <typename T>
		inline bool operator<=(const T& x, const T& y)
		{ return !(y < x); }

		template <typename T>
		inline bool operator>=(const T& x, const T& y)
		{ return !(x < y); }
	}



	///////////////////////////////////////////////////////////////////////
	/// pair
	///
	/// Implements a simple pair, just like the C++ std::pair.
	///
	template <typename T1, typename T2>
	struct pair
	{
		typedef T1 first_type;
		typedef T2 second_type;

		T1 first;
		T2 second;

		pair();
		pair(const T1& x);
		pair(const T1& x, const T2& y);

		template <typename U, typename V>
		pair(const pair<U, V>& p);

		// pair(const pair& p);              // Not necessary, as default version is OK.
		// pair& operator=(const pair& p);   // Not necessary, as default version is OK.
	};




	/// use_self
	///
	/// operator()(x) simply returns x. Used in sets, as opposed to maps.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_first template implementation.
	///
	/// The existance of use_self may seem odd, given that it does nothing,
	/// but these kinds of things are useful, virtually required, for optimal 
	/// generic programming.
	///
	template <typename T>
	struct use_self             // : public unary_function<T, T> // Perhaps we want to make it a subclass of unary_function.
	{
		typedef T result_type;

		const T& operator()(const T& x) const
		{ return x; }
	};

	/// use_first
	///
	/// operator()(x) simply returns x.first. Used in maps, as opposed to sets.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_self template implementation. This is the same thing as the
	/// SGI SGL select1st utility.
	///
	template <typename Pair>
	struct use_first            // : public unary_function<Pair, typename Pair::first_type> // Perhaps we want to make it a subclass of unary_function.
	{
		typedef typename Pair::first_type result_type;

		const result_type& operator()(const Pair& x) const
		{ return x.first; }
	};

	/// use_second
	///
	/// operator()(x) simply returns x.second. 
	/// This is the same thing as the SGI SGL select2nd utility
	///
	template <typename Pair>
	struct use_second           // : public unary_function<Pair, typename Pair::second_type> // Perhaps we want to make it a subclass of unary_function.
	{
		typedef typename Pair::second_type result_type;

		const result_type& operator()(const Pair& x) const
		{ return x.second; }
	};





	///////////////////////////////////////////////////////////////////////
	// pair
	///////////////////////////////////////////////////////////////////////

	template <typename T1, typename T2>
	inline pair<T1, T2>::pair()
		: first(), second()
	{
		// Empty
	}


	template <typename T1, typename T2>
	inline pair<T1, T2>::pair(const T1& x)
		: first(x), second()
	{
		// Empty
	}


	template <typename T1, typename T2>
	inline pair<T1, T2>::pair(const T1& x, const T2& y)
		: first(x), second(y)
	{
		// Empty
	}


	template <typename T1, typename T2>
	template <typename U, typename V>
	inline pair<T1, T2>::pair(const pair<U, V>& p)
		: first(p.first), second(p.second)
	{
		// Empty
	}




	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T1, typename T2>
	inline bool operator==(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return ((a.first == b.first) && (a.second == b.second));
	}


	template <typename T1, typename T2>
	inline bool operator<(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		// Note that we use only operator < in this expression. Otherwise we could
		// use the simpler: return (a.m1 == b.m1) ? (a.m2 < b.m2) : (a.m1 < b.m1);
		// The user can write a specialization for this operator to get around this
		// in cases where the highest performance is required.
		return ((a.first < b.first) || (!(b.first < a.first) && (a.second < b.second)));
	}


	template <typename T1, typename T2>
	inline bool operator!=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(a == b);
	}


	template <typename T1, typename T2>
	inline bool operator>(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return b < a;
	}


	template <typename T1, typename T2>
	inline bool operator>=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(a < b);
	}


	template <typename T1, typename T2>
	inline bool operator<=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(b < a);
	}




	///////////////////////////////////////////////////////////////////////
	/// make_pair / make_pair_ref
	///
	/// make_pair is the same as std::make_pair specified by the C++ standard.
	/// If you look at the C++ standard, you'll see that it specifies T& instead of T.
	/// However, it has been determined that the C++ standard is incorrect and has 
	/// flagged it as a defect (http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#181).
	/// In case you feel that you want a more efficient version that uses references,
	/// we provide the make_pair_ref function below.
	/// 
	/// Note: You don't need to use make_pair in order to make a pair. The following
	/// code is equivalent, and the latter avoids one more level of inlining:
	///     return make_pair(charPtr, charPtr);
	///     return pair<char*, char*>(charPtr, charPtr);
	///
	template <typename T1, typename T2>
	inline pair<T1, T2> make_pair(T1 a, T2 b)
	{
		return pair<T1, T2>(a, b);
	}


	template <typename T1, typename T2>
	inline pair<T1, T2> make_pair_ref(const T1& a, const T2& b)
	{
		return pair<T1, T2>(a, b);
	}



	/// swap
	///
	/// Assigns the contents of a to b and the contents of b to a. 
	/// A temporary instance of type T is created and destroyed
	/// in the process.
	///
	/// This function is used by numerous other algorithms, and as 
	/// such it may in some cases be feasible and useful for the user 
	/// to implement an override version of this function which is 
	/// more efficient in some way. 
	///
	template <typename T>
	inline void swap(T& a, T& b)
	{
		T temp(a);
		a = b;
		b = temp;
	}



	// iter_swap helper functions
	//
	template <bool bTypesAreEqual>
	struct iter_swap_impl
	{
		template <typename ForwardIterator1, typename ForwardIterator2>
		static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
		{
			typedef typename mimp::iterator_traits<ForwardIterator1>::value_type value_type_a;

			value_type_a temp(*a);
			*a = *b;
			*b = temp; 
		}
	};

	template <>
	struct iter_swap_impl<true>
	{
		template <typename ForwardIterator1, typename ForwardIterator2>
		static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
		{
			mimp::swap(*a, *b);
		}
	};

	/// iter_swap
	///
	/// Equivalent to swap(*a, *b), though the user can provide an override to
	/// iter_swap that is independent of an override which may exist for swap.
	///
	/// We provide a version of iter_swap which uses swap when the swapped types 
	/// are equal but a manual implementation otherwise. We do this because the 
	/// C++ standard defect report says that iter_swap(a, b) must be implemented 
	/// as swap(*a, *b) when possible.
	///
	template <typename ForwardIterator1, typename ForwardIterator2>
	inline void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
	{
		typedef typename mimp::iterator_traits<ForwardIterator1>::value_type value_type_a;
		typedef typename mimp::iterator_traits<ForwardIterator2>::value_type value_type_b;
		typedef typename mimp::iterator_traits<ForwardIterator1>::reference  reference_a;
		typedef typename mimp::iterator_traits<ForwardIterator2>::reference  reference_b;

		iter_swap_impl<type_and<is_same<value_type_a, value_type_b>::value, is_same<value_type_a&, reference_a>::value, is_same<value_type_b&, reference_b>::value >::value >::iter_swap(a, b);
	}




	/// EASTL_RBTREE_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
#ifndef EASTL_RBTREE_DEFAULT_NAME
#define EASTL_RBTREE_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " rbtree" // Unless the user overrides something, this is "EASTL rbtree".
#endif


	/// EASTL_RBTREE_DEFAULT_ALLOCATOR
	///
#ifndef EASTL_RBTREE_DEFAULT_ALLOCATOR
#define EASTL_RBTREE_DEFAULT_ALLOCATOR allocator_type(EASTL_RBTREE_DEFAULT_NAME)
#endif



	/// RBTreeColor
	///
	enum RBTreeColor
	{
		kRBTreeColorRed,
		kRBTreeColorBlack
	};



	/// RBTreeColor
	///
	enum RBTreeSide
	{
		kRBTreeSideLeft,
		kRBTreeSideRight
	};



	/// rbtree_node_base
	///
	/// We define a rbtree_node_base separately from rbtree_node (below), because it 
	/// allows us to have non-templated operations, and it makes it so that the 
	/// rbtree anchor node doesn't carry a T with it, which would waste space and 
	/// possibly lead to surprising the user due to extra Ts existing that the user 
	/// didn't explicitly create. The downside to all of this is that it makes debug 
	/// viewing of an rbtree harder, given that the node pointers are of type 
	/// rbtree_node_base and not rbtree_node.
	///
	struct rbtree_node_base
	{
		typedef rbtree_node_base this_type;

	public:
		this_type* mpNodeRight;  // Declared first because it is used most often.
		this_type* mpNodeLeft;
		this_type* mpNodeParent;
		char       mColor;       // We only need one bit here, would be nice if we could stuff that bit somewhere else.
	};


	/// rbtree_node
	///
	template <typename Value>
	struct rbtree_node : public rbtree_node_base
	{
		Value mValue; // For set and multiset, this is the user's value, for map and multimap, this is a pair of key/value.
	};




	// rbtree_node_base functions
	//
	// These are the fundamental functions that we use to maintain the 
	// tree. The bulk of the work of the tree maintenance is done in 
	// these functions.
	//
	 rbtree_node_base* RBTreeIncrement    (const rbtree_node_base* pNode);
	 rbtree_node_base* RBTreeDecrement    (const rbtree_node_base* pNode);
	 rbtree_node_base* RBTreeGetMinChild  (const rbtree_node_base* pNode);
	 rbtree_node_base* RBTreeGetMaxChild  (const rbtree_node_base* pNode);
	 size_t            RBTreeGetBlackCount(const rbtree_node_base* pNodeTop,
		const rbtree_node_base* pNodeBottom);
	 void              RBTreeInsert       (      rbtree_node_base* pNode,
		rbtree_node_base* pNodeParent, 
		rbtree_node_base* pNodeAnchor,
		RBTreeSide insertionSide);
	 void              RBTreeErase        (      rbtree_node_base* pNode,
		rbtree_node_base* pNodeAnchor); 







	/// rbtree_iterator
	///
	template <typename T, typename Pointer, typename Reference>
	struct rbtree_iterator
	{
		typedef rbtree_iterator<T, Pointer, Reference>      this_type;
		typedef rbtree_iterator<T, T*, T&>                  iterator;
		typedef rbtree_iterator<T, const T*, const T&>      const_iterator;
		typedef size_t                                size_type;     // See config.h for the definition of size_t, which defaults to mimp::MiU32.
		typedef ptrdiff_t                                   difference_type;
		typedef T                                           value_type;
		typedef rbtree_node_base                            base_node_type;
		typedef rbtree_node<T>                              node_type;
		typedef Pointer                                     pointer;
		typedef Reference                                   reference;
		typedef EASTL_ITC_NS::bidirectional_iterator_tag    iterator_category;

	public:
		node_type* mpNode;

	public:
		rbtree_iterator();
		explicit rbtree_iterator(const node_type* pNode);
		rbtree_iterator(const iterator& x);

		reference operator*() const;
		pointer   operator->() const;

		rbtree_iterator& operator++();
		rbtree_iterator  operator++(int);

		rbtree_iterator& operator--();
		rbtree_iterator  operator--(int);

	}; // rbtree_iterator





	///////////////////////////////////////////////////////////////////////////////
	// rb_base
	//
	// This class allows us to use a generic rbtree as the basis of map, multimap,
	// set, and multiset transparently. The vital template parameters for this are 
	// the ExtractKey and the bUniqueKeys parameters.
	//
	// If the rbtree has a value type of the form pair<T1, T2> (i.e. it is a map or
	// multimap and not a set or multiset) and a key extraction policy that returns 
	// the first part of the pair, the rbtree gets a mapped_type typedef. 
	// If it satisfies those criteria and also has unique keys, then it also gets an 
	// operator[] (which only map and set have and multimap and multiset don't have).
	//
	///////////////////////////////////////////////////////////////////////////////



	/// rb_base
	/// This specialization is used for 'set'. In this case, Key and Value 
	/// will be the same as each other and ExtractKey will be mimp::use_self.
	///
	template <typename Key, typename Value, typename Compare, typename ExtractKey, bool bUniqueKeys, typename RBTree>
	struct rb_base
	{
		typedef ExtractKey extract_key;

	public:
		Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

	public:
		rb_base() : mCompare() {}
		rb_base(const Compare& compare) : mCompare(compare) {}
	};


	/// rb_base
	/// This class is used for 'multiset'.
	/// In this case, Key and Value will be the same as each 
	/// other and ExtractKey will be mimp::use_self.
	///
	template <typename Key, typename Value, typename Compare, typename ExtractKey, typename RBTree>
	struct rb_base<Key, Value, Compare, ExtractKey, false, RBTree>
	{
		typedef ExtractKey extract_key;

	public:
		Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

	public:
		rb_base() : mCompare() {}
		rb_base(const Compare& compare) : mCompare(compare) {}
	};


	/// rb_base
	/// This specialization is used for 'map'.
	///
	template <typename Key, typename Pair, typename Compare, typename RBTree>
	struct rb_base<Key, Pair, Compare, mimp::use_first<Pair>, true, RBTree>
	{
		typedef mimp::use_first<Pair> extract_key;

	public:
		Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

	public:
		rb_base() : mCompare() {}
		rb_base(const Compare& compare) : mCompare(compare) {}
	};


	/// rb_base
	/// This specialization is used for 'multimap'.
	///
	template <typename Key, typename Pair, typename Compare, typename RBTree>
	struct rb_base<Key, Pair, Compare, mimp::use_first<Pair>, false, RBTree>
	{
		typedef mimp::use_first<Pair> extract_key;

	public:
		Compare mCompare; // To do: Make sure that empty Compare classes go away via empty base optimizations.

	public:
		rb_base() : mCompare() {}
		rb_base(const Compare& compare) : mCompare(compare) {}
	};





	/// rbtree
	///
	/// rbtree is the red-black tree basis for the map, multimap, set, and multiset 
	/// containers. Just about all the work of those containers is done here, and 
	/// they are merely a shell which sets template policies that govern the code
	/// generation for this rbtree.
	///
	/// This rbtree implementation is pretty much the same as all other modern 
	/// rbtree implementations, as the topic is well known and researched. We may
	/// choose to implement a "relaxed balancing" option at some point in the 
	/// future if it is deemed worthwhile. Most rbtree implementations don't do this.
	///
	/// The primary rbtree member variable is mAnchor, which is a node_type and 
	/// acts as the end node. However, like any other node, it has mpNodeLeft,
	/// mpNodeRight, and mpNodeParent members. We do the conventional trick of 
	/// assigning begin() (left-most rbtree node) to mpNodeLeft, assigning 
	/// 'end() - 1' (a.k.a. rbegin()) to mpNodeRight, and assigning the tree root
	/// node to mpNodeParent. 
	///
	/// Compare (functor): This is a comparison class which defaults to 'less'.
	/// It is a common STL thing which takes two arguments and returns true if  
	/// the first is less than the second.
	///
	/// ExtractKey (functor): This is a class which gets the key from a stored
	/// node. With map and set, the node is a pair, whereas with set and multiset
	/// the node is just the value. ExtractKey will be either mimp::use_first (map and multimap)
	/// or mimp::use_self (set and multiset).
	///
	/// bMutableIterators (bool): true if rbtree::iterator is a mutable
	/// iterator, false if iterator and const_iterator are both const iterators. 
	/// It will be true for map and multimap and false for set and multiset.
	///
	/// bUniqueKeys (bool): true if the keys are to be unique, and false if there
	/// can be multiple instances of a given key. It will be true for set and map 
	/// and false for multiset and multimap.
	///
	/// To consider: Add an option for relaxed tree balancing. This could result 
	/// in performance improvements but would require a more complicated implementation.
	///
	///////////////////////////////////////////////////////////////////////
	/// find_as
	/// In order to support the ability to have a tree of strings but
	/// be able to do efficiently lookups via char pointers (i.e. so they
	/// aren't converted to string objects), we provide the find_as
	/// function. This function allows you to do a find with a key of a
	/// type other than the tree's key type. See the find_as function
	/// for more documentation on this.
	///
	template <typename Key, typename Value, typename Compare, typename Allocator, 
		typename ExtractKey, bool bMutableIterators, bool bUniqueKeys>
	class rbtree
		: public rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, 
		rbtree<Key, Value, Compare, Allocator, ExtractKey, bMutableIterators, bUniqueKeys> >
	{
	public:
		typedef ptrdiff_t                                                                       difference_type;
		typedef size_t                                                                    size_type;     // See config.h for the definition of size_t, which defaults to mimp::MiU32.
		typedef Key                                                                             key_type;
		typedef Value                                                                           value_type;
		typedef rbtree_node<value_type>                                                         node_type;
		typedef value_type&                                                                     reference;
		typedef const value_type&                                                               const_reference;
		typedef typename type_select<bMutableIterators, 
			rbtree_iterator<value_type, value_type*, value_type&>, 
			rbtree_iterator<value_type, const value_type*, const value_type&> >::type   iterator;
		typedef rbtree_iterator<value_type, const value_type*, const value_type&>               const_iterator;
		typedef mimp::reverse_iterator<iterator>                                               reverse_iterator;
		typedef mimp::reverse_iterator<const_iterator>                                         const_reverse_iterator;

		typedef Allocator                                                                       allocator_type;
		typedef Compare                                                                         key_compare;
		typedef typename type_select<bUniqueKeys, mimp::pair<iterator, bool>, iterator>::type  insert_return_type;  // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
		typedef rbtree<Key, Value, Compare, Allocator, 
			ExtractKey, bMutableIterators, bUniqueKeys>                             this_type;
		typedef rb_base<Key, Value, Compare, ExtractKey, bUniqueKeys, this_type>                base_type;
		typedef integral_constant<bool, bUniqueKeys>                                            has_unique_keys_type;
		typedef typename base_type::extract_key                                                 extract_key;

		using base_type::mCompare;

		enum
		{
			kKeyAlignment         = EASTL_ALIGN_OF(key_type),
			kKeyAlignmentOffset   = 0,                          // To do: Make sure this really is zero for all uses of this template.
			kValueAlignment       = EASTL_ALIGN_OF(value_type),
			kValueAlignmentOffset = 0                           // To fix: This offset is zero for sets and >0 for maps. Need to fix this.
		};

	public:
		rbtree_node_base  mAnchor;      /// This node acts as end() and its mpLeft points to begin(), and mpRight points to rbegin() (the last node on the right).
		size_type         mnSize;       /// Stores the count of nodes in the tree (not counting the anchor node).
		allocator_type    mAllocator;   // To do: Use base class optimization to make this go away.

	public:
		// ctor/dtor
		rbtree();
		rbtree(const allocator_type& allocator);
		rbtree(const Compare& compare, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR);
		rbtree(const this_type& x);

		template <typename InputIterator>
		rbtree(InputIterator first, InputIterator last, const Compare& compare, const allocator_type& allocator = EASTL_RBTREE_DEFAULT_ALLOCATOR);

		~rbtree();

	public:
		// properties
		allocator_type& get_allocator();
		void            set_allocator(const allocator_type& allocator);

		const key_compare& key_comp() const { return mCompare; }
		key_compare&       key_comp()       { return mCompare; }

		this_type& operator=(const this_type& x);

		void swap(this_type& x);

	public: 
		// iterators
		iterator        begin();
		const_iterator  begin() const;
		iterator        end();
		const_iterator  end() const;

		reverse_iterator        rbegin();
		const_reverse_iterator  rbegin() const;
		reverse_iterator        rend();
		const_reverse_iterator  rend() const;

	public:
		bool      empty() const;
		size_type size() const;

		/// map::insert and set::insert return a pair, while multimap::insert and
		/// multiset::insert return an iterator.
		insert_return_type insert(const value_type& value);

		// C++ standard: inserts value if and only if there is no element with 
		// key equivalent to the key of t in containers with unique keys; always 
		// inserts value in containers with equivalent keys. Always returns the 
		// iterator pointing to the element with key equivalent to the key of value. 
		// iterator position is a hint pointing to where the insert should start
		// to search. However, there is a potential defect/improvement report on this behaviour:
		// LWG issue #233 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1780.html)
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		iterator insert(iterator position, const value_type& value);

		template <typename InputIterator>
		void insert(InputIterator first, InputIterator last);

		iterator erase(iterator position);
		iterator erase(iterator first, iterator last);

		reverse_iterator erase(reverse_iterator position);
		reverse_iterator erase(reverse_iterator first, reverse_iterator last);

		// For some reason, multiple STL versions make a specialization 
		// for erasing an array of key_types. I'm pretty sure we don't
		// need this, but just to be safe we will follow suit. 
		// The implementation is trivial. Returns void because the values
		// could well be randomly distributed throughout the tree and thus
		// a return value would be nearly meaningless.
		void erase(const key_type* first, const key_type* last);

		void clear();
		void reset();

		iterator       find(const key_type& key);
		const_iterator find(const key_type& key) const;

		/// Implements a find whereby the user supplies a comparison of a different type
		/// than the tree's value_type. A useful case of this is one whereby you have
		/// a container of string objects but want to do searches via passing in char pointers.
		/// The problem is that without this kind of find, you need to do the expensive operation
		/// of converting the char pointer to a string so it can be used as the argument to the
		/// find function.
		///
		/// Example usage (note that the compare uses string as first type and char* as second):
		///     set<string> strings;
		///     strings.find_as("hello", less_2<string, const char*>());
		///
		template <typename U, typename Compare2>
		iterator       find_as(const U& u, Compare2 compare2);

		template <typename U, typename Compare2>
		const_iterator find_as(const U& u, Compare2 compare2) const;

		iterator       lower_bound(const key_type& key);
		const_iterator lower_bound(const key_type& key) const;

		iterator       upper_bound(const key_type& key);
		const_iterator upper_bound(const key_type& key) const;

		bool validate() const;
		int  validate_iterator(const_iterator i) const;

	protected:
		node_type*  DoAllocateNode();
		void        DoFreeNode(node_type* pNode);

		node_type* DoCreateNodeFromKey(const key_type& key);
		node_type* DoCreateNode(const value_type& value);
		node_type* DoCreateNode(const node_type* pNodeSource, node_type* pNodeParent);

		node_type* DoCopySubtree(const node_type* pNodeSource, node_type* pNodeDest);
		void       DoNukeSubtree(node_type* pNode);

		// Intentionally return a pair and not an iterator for DoInsertValue(..., true_type)
		// This is because the C++ standard for map and set is to return a pair and not just an iterator.
		mimp::pair<iterator, bool> DoInsertValue(const value_type& value, true_type);  // true_type means keys are unique.
		iterator DoInsertValue(const value_type& value, false_type);                    // false_type means keys are not unique.

		mimp::pair<iterator, bool> DoInsertKey(const key_type& key, true_type);
		iterator DoInsertKey(const key_type& key, false_type);

		iterator DoInsertValue(iterator position, const value_type& value, true_type);
		iterator DoInsertValue(iterator position, const value_type& value, false_type);

		iterator DoInsertKey(iterator position, const key_type& key, true_type);
		iterator DoInsertKey(iterator position, const key_type& key, false_type);

		iterator DoInsertValueImpl(node_type* pNodeParent, const value_type& value, bool bForceToLeft);
		iterator DoInsertKeyImpl(node_type* pNodeParent, const key_type& key, bool bForceToLeft);

	}; // rbtree





	///////////////////////////////////////////////////////////////////////
	// rbtree_node_base functions
	///////////////////////////////////////////////////////////////////////

	 inline rbtree_node_base* RBTreeGetMinChild(const rbtree_node_base* pNodeBase)
	{
		while(pNodeBase->mpNodeLeft) 
			pNodeBase = pNodeBase->mpNodeLeft;
		return const_cast<rbtree_node_base*>(pNodeBase);
	}

	 inline rbtree_node_base* RBTreeGetMaxChild(const rbtree_node_base* pNodeBase)
	{
		while(pNodeBase->mpNodeRight) 
			pNodeBase = pNodeBase->mpNodeRight;
		return const_cast<rbtree_node_base*>(pNodeBase);
	}

	// The rest of the functions are non-trivial and are found in 
	// the corresponding .cpp file to this file.



	///////////////////////////////////////////////////////////////////////
	// rbtree_iterator functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator()
		: mpNode(NULL) { }


	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(const node_type* pNode)
		: mpNode(static_cast<node_type*>(const_cast<node_type*>(pNode))) { }


	template <typename T, typename Pointer, typename Reference>
	rbtree_iterator<T, Pointer, Reference>::rbtree_iterator(const iterator& x)
		: mpNode(x.mpNode) { }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::reference
		rbtree_iterator<T, Pointer, Reference>::operator*() const
	{ return mpNode->mValue; }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::pointer
		rbtree_iterator<T, Pointer, Reference>::operator->() const
	{ return &mpNode->mValue; }


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type&
		rbtree_iterator<T, Pointer, Reference>::operator++()
	{
		mpNode = static_cast<node_type*>(RBTreeIncrement(mpNode));
		return *this;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type
		rbtree_iterator<T, Pointer, Reference>::operator++(int)
	{
		this_type temp(*this);
		mpNode = static_cast<node_type*>(RBTreeIncrement(mpNode));
		return temp;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type&
		rbtree_iterator<T, Pointer, Reference>::operator--()
	{
		mpNode = static_cast<node_type*>(RBTreeDecrement(mpNode));
		return *this;
	}


	template <typename T, typename Pointer, typename Reference>
	typename rbtree_iterator<T, Pointer, Reference>::this_type
		rbtree_iterator<T, Pointer, Reference>::operator--(int)
	{
		this_type temp(*this);
		mpNode = static_cast<node_type*>(RBTreeDecrement(mpNode));
		return temp;
	}


	// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
	// Thus we provide additional template paremeters here to support this. The defect report does not
	// require us to support comparisons between reverse_iterators and const_reverse_iterators.
	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator==(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
		const rbtree_iterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode == b.mpNode;
	}


	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator!=(const rbtree_iterator<T, PointerA, ReferenceA>& a, 
		const rbtree_iterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode != b.mpNode;
	}


	// We provide a version of operator!= for the case where the iterators are of the 
	// same type. This helps prevent ambiguity errors in the presence of rel_ops.
	template <typename T, typename Pointer, typename Reference>
	inline bool operator!=(const rbtree_iterator<T, Pointer, Reference>& a, 
		const rbtree_iterator<T, Pointer, Reference>& b)
	{
		return a.mpNode != b.mpNode;
	}




	///////////////////////////////////////////////////////////////////////
	// rbtree functions
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree()
		: mAnchor(),
		mnSize(0),
		mAllocator(EASTL_RBTREE_DEFAULT_NAME)
	{
		reset();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const allocator_type& allocator)
		: mAnchor(),
		mnSize(0),
		mAllocator(allocator)
	{
		reset();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const C& compare, const allocator_type& allocator)
		: base_type(compare),
		mAnchor(),
		mnSize(0),
		mAllocator(allocator)
	{
		reset();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(const this_type& x)
		: base_type(x.mCompare),
		mAnchor(),
		mnSize(0),
		mAllocator(x.mAllocator)
	{
		reset();

		if(x.mAnchor.mpNodeParent) // mAnchor.mpNodeParent is the rb_tree root node.
		{
			mAnchor.mpNodeParent = DoCopySubtree((const node_type*)x.mAnchor.mpNodeParent, (node_type*)&mAnchor);
			mAnchor.mpNodeRight  = RBTreeGetMaxChild(mAnchor.mpNodeParent);
			mAnchor.mpNodeLeft   = RBTreeGetMinChild(mAnchor.mpNodeParent);
			mnSize               = x.mnSize;
		}
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename InputIterator>
	inline rbtree<K, V, C, A, E, bM, bU>::rbtree(InputIterator first, InputIterator last, const C& compare, const allocator_type& allocator)
		: base_type(compare),
		mAnchor(),
		mnSize(0),
		mAllocator(allocator)
	{
		reset();

			for(; first != last; ++first)
				insert(*first);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline rbtree<K, V, C, A, E, bM, bU>::~rbtree()
	{
		// Erase the entire tree. DoNukeSubtree is not a 
		// conventional erase function, as it does no rebalancing.
		DoNukeSubtree((node_type*)mAnchor.mpNodeParent);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::allocator_type&
		rbtree<K, V, C, A, E, bM, bU>::get_allocator()
	{
		return mAllocator;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::set_allocator(const allocator_type& allocator)
	{
		mAllocator = allocator;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::size_type
		rbtree<K, V, C, A, E, bM, bU>::size() const
	{ return mnSize; }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline bool rbtree<K, V, C, A, E, bM, bU>::empty() const
	{ return (mnSize == 0); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::begin()
	{ return iterator(static_cast<node_type*>(mAnchor.mpNodeLeft)); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::begin() const
	{ return const_iterator(static_cast<node_type*>(const_cast<rbtree_node_base*>(mAnchor.mpNodeLeft))); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::end()
	{ return iterator(static_cast<node_type*>(&mAnchor)); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::end() const
	{ return const_iterator(static_cast<node_type*>(const_cast<rbtree_node_base*>(&mAnchor))); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::rbegin()
	{ return reverse_iterator(end()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::rbegin() const
	{ return const_reverse_iterator(end()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::rend()
	{ return reverse_iterator(begin()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::rend() const
	{ return const_reverse_iterator(begin()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::this_type&
		rbtree<K, V, C, A, E, bM, bU>::operator=(const this_type& x)
	{
		if(this != &x)
		{
			clear();

#if EASTL_ALLOCATOR_COPY_ENABLED
			mAllocator = x.mAllocator;
#endif

			base_type::mCompare = x.mCompare;

			if(x.mAnchor.mpNodeParent) // mAnchor.mpNodeParent is the rb_tree root node.
			{
				mAnchor.mpNodeParent = DoCopySubtree((const node_type*)x.mAnchor.mpNodeParent, (node_type*)&mAnchor);
				mAnchor.mpNodeRight  = RBTreeGetMaxChild(mAnchor.mpNodeParent);
				mAnchor.mpNodeLeft   = RBTreeGetMinChild(mAnchor.mpNodeParent);
				mnSize               = x.mnSize;
			}
		}
		return *this;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::swap(this_type& x)
	{
		if(mAllocator == x.mAllocator) // If allocators are equivalent...
		{
			// Most of our members can be exchaged by a basic swap:
			// We leave mAllocator as-is.
			mimp::swap(mnSize,              x.mnSize);
			mimp::swap(base_type::mCompare, x.mCompare);

			// However, because our anchor node is a part of our class instance and not 
			// dynamically allocated, we can't do a swap of it but must do a more elaborate
			// procedure. This is the downside to having the mAnchor be like this, but 
			// otherwise we consider it a good idea to avoid allocating memory for a 
			// nominal container instance.

			// We optimize for the expected most common case: both pointers being non-null.
			if(mAnchor.mpNodeParent && x.mAnchor.mpNodeParent) // If both pointers are non-null...
			{
				mimp::swap(mAnchor.mpNodeRight,  x.mAnchor.mpNodeRight);
				mimp::swap(mAnchor.mpNodeLeft,   x.mAnchor.mpNodeLeft);
				mimp::swap(mAnchor.mpNodeParent, x.mAnchor.mpNodeParent);

				// We need to fix up the anchors to point to themselves (we can't just swap them).
				mAnchor.mpNodeParent->mpNodeParent   = &mAnchor;
				x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;
			}
			else if(mAnchor.mpNodeParent)
			{
				x.mAnchor.mpNodeRight  = mAnchor.mpNodeRight;
				x.mAnchor.mpNodeLeft   = mAnchor.mpNodeLeft;
				x.mAnchor.mpNodeParent = mAnchor.mpNodeParent;
				x.mAnchor.mpNodeParent->mpNodeParent = &x.mAnchor;

				// We need to fix up our anchor to point it itself (we can't have it swap with x).
				mAnchor.mpNodeRight  = &mAnchor;
				mAnchor.mpNodeLeft   = &mAnchor;
				mAnchor.mpNodeParent = NULL;
			}
			else if(x.mAnchor.mpNodeParent)
			{
				mAnchor.mpNodeRight  = x.mAnchor.mpNodeRight;
				mAnchor.mpNodeLeft   = x.mAnchor.mpNodeLeft;
				mAnchor.mpNodeParent = x.mAnchor.mpNodeParent;
				mAnchor.mpNodeParent->mpNodeParent = &mAnchor;

				// We need to fix up x's anchor to point it itself (we can't have it swap with us).
				x.mAnchor.mpNodeRight  = &x.mAnchor;
				x.mAnchor.mpNodeLeft   = &x.mAnchor;
				x.mAnchor.mpNodeParent = NULL;
			} // Else both are NULL and there is nothing to do.
		}
		else
		{
			const this_type temp(*this); // Can't call mimp::swap because that would
			*this = x;                   // itself call this member swap function.
			x     = temp;
		}
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::insert_return_type // map/set::insert return a pair, multimap/multiset::iterator return an iterator.
		rbtree<K, V, C, A, E, bM, bU>::insert(const value_type& value)
	{ return DoInsertValue(value, has_unique_keys_type()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::insert(iterator position, const value_type& value)
	{ return DoInsertValue(position, value, has_unique_keys_type()); }


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	mimp::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
		rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(const value_type& value, true_type) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		// Note that we return a pair and not an iterator. This is because the C++ standard for map
		// and set is to return a pair and not just an iterator.
		extract_key extractKey;

		node_type* pCurrent    = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pLowerBound = (node_type*)&mAnchor;             // Set it to the container end for now.
		node_type* pParent;                                        // This will be where we insert the new node.

		bool bValueLessThanNode = true; // If the tree is empty, this will result in an insertion at the front.

		// Find insertion position of the value. This will either be a position which 
		// already contains the value, a position which is greater than the value or
		// end(), which we treat like a position which is greater than the value.
		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			bValueLessThanNode = mCompare(extractKey(value), extractKey(pCurrent->mValue));
			pLowerBound        = pCurrent;

			if(bValueLessThanNode)
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), extractKey(value))); // Validate that the compare function is sane.
				pCurrent = (node_type*)pCurrent->mpNodeLeft;
			}
			else
				pCurrent = (node_type*)pCurrent->mpNodeRight;
		}

		pParent = pLowerBound; // pLowerBound is actually upper bound right now (i.e. it is > value instead of <=), but we will make it the lower bound below.

		if(bValueLessThanNode) // If we ended up on the left side of the last parent node...
		{
			if(EASTL_LIKELY(pLowerBound != (node_type*)mAnchor.mpNodeLeft)) // If the tree was empty or if we otherwise need to insert at the very front of the tree...
			{
				// At this point, pLowerBound points to a node which is > than value.
				// Move it back by one, so that it points to a node which is <= value.
				pLowerBound = (node_type*)RBTreeDecrement(pLowerBound);
			}
			else
			{
				const iterator itResult(DoInsertValueImpl(pLowerBound, value, false));
				return pair<iterator, bool>(itResult, true);
			}
		}

		// Since here we require values to be unique, we will do nothing if the value already exists.
		if(mCompare(extractKey(pLowerBound->mValue), extractKey(value))) // If the node is < the value (i.e. if value is >= the node)...
		{
			EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(pLowerBound->mValue))); // Validate that the compare function is sane.
			const iterator itResult(DoInsertValueImpl(pParent, value, false));
			return pair<iterator, bool>(itResult, true);
		}

		// The item already exists (as found by the compare directly above), so return false.
		return pair<iterator, bool>(iterator(pLowerBound), false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(const value_type& value, false_type) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.
		extract_key extractKey;

		while(pCurrent)
		{
			pRangeEnd = pCurrent;

			if(mCompare(extractKey(value), extractKey(pCurrent->mValue)))
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), extractKey(value))); // Validate that the compare function is sane.
				pCurrent = (node_type*)pCurrent->mpNodeLeft;
			}
			else
				pCurrent = (node_type*)pCurrent->mpNodeRight;
		}

		return DoInsertValueImpl(pRangeEnd, value, false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	mimp::pair<typename rbtree<K, V, C, A, E, bM, bU>::iterator, bool>
		rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(const key_type& key, true_type) // true_type means keys are unique.
	{
		// This code is essentially a slightly modified copy of the the rbtree::insert 
		// function whereby this version takes a key and not a full value_type.
		extract_key extractKey;

		node_type* pCurrent    = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pLowerBound = (node_type*)&mAnchor;             // Set it to the container end for now.
		node_type* pParent;                                        // This will be where we insert the new node.

		bool bValueLessThanNode = true; // If the tree is empty, this will result in an insertion at the front.

		// Find insertion position of the value. This will either be a position which 
		// already contains the value, a position which is greater than the value or
		// end(), which we treat like a position which is greater than the value.
		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			bValueLessThanNode = mCompare(key, extractKey(pCurrent->mValue));
			pLowerBound        = pCurrent;

			if(bValueLessThanNode)
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pCurrent = (node_type*)pCurrent->mpNodeLeft;
			}
			else
				pCurrent = (node_type*)pCurrent->mpNodeRight;
		}

		pParent = pLowerBound; // pLowerBound is actually upper bound right now (i.e. it is > value instead of <=), but we will make it the lower bound below.

		if(bValueLessThanNode) // If we ended up on the left side of the last parent node...
		{
			if(EASTL_LIKELY(pLowerBound != (node_type*)mAnchor.mpNodeLeft)) // If the tree was empty or if we otherwise need to insert at the very front of the tree...
			{
				// At this point, pLowerBound points to a node which is > than value.
				// Move it back by one, so that it points to a node which is <= value.
				pLowerBound = (node_type*)RBTreeDecrement(pLowerBound);
			}
			else
			{
				const iterator itResult(DoInsertKeyImpl(pLowerBound, key, false));
				return pair<iterator, bool>(itResult, true);
			}
		}

		// Since here we require values to be unique, we will do nothing if the value already exists.
		if(mCompare(extractKey(pLowerBound->mValue), key)) // If the node is < the value (i.e. if value is >= the node)...
		{
			EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pLowerBound->mValue))); // Validate that the compare function is sane.
			const iterator itResult(DoInsertKeyImpl(pParent, key, false));
			return pair<iterator, bool>(itResult, true);
		}

		// The item already exists (as found by the compare directly above), so return false.
		return pair<iterator, bool>(iterator(pLowerBound), false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(const key_type& key, false_type) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.
		extract_key extractKey;

		while(pCurrent)
		{
			pRangeEnd = pCurrent;

			if(mCompare(key, extractKey(pCurrent->mValue)))
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pCurrent = (node_type*)pCurrent->mpNodeLeft;
			}
			else
				pCurrent = (node_type*)pCurrent->mpNodeRight;
		}

		return DoInsertKeyImpl(pRangeEnd, key, false);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(iterator position, const value_type& value, true_type) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		extract_key extractKey;

		if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
		{
			iterator itNext(position);
			++itNext;

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.
			const bool bPositionLessThanValue = mCompare(extractKey(position.mpNode->mValue), extractKey(value));

			if(bPositionLessThanValue) // If (value > *position)...
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(position.mpNode->mValue))); // Validate that the compare function is sane.

				const bool bValueLessThanNext = mCompare(extractKey(value), extractKey(itNext.mpNode->mValue));

				if(bValueLessThanNext) // if (value < *itNext)...
				{
					EASTL_VALIDATE_COMPARE(!mCompare(extractKey(itNext.mpNode->mValue), extractKey(value))); // Validate that the compare function is sane.

					if(position.mpNode->mpNodeRight)
						return DoInsertValueImpl(itNext.mpNode, value, true);
					return DoInsertValueImpl(position.mpNode, value, false);
				}
			}

			return DoInsertValue(value, has_unique_keys_type()).first;
		}

		if(mnSize && mCompare(extractKey(((node_type*)mAnchor.mpNodeRight)->mValue), extractKey(value)))
		{
			EASTL_VALIDATE_COMPARE(!mCompare(extractKey(value), extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))); // Validate that the compare function is sane.
			return DoInsertValueImpl((node_type*)mAnchor.mpNodeRight, value, false);
		}

		return DoInsertValue(value, has_unique_keys_type()).first;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertValue(iterator position, const value_type& value, false_type) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		extract_key extractKey;

		if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
		{
			iterator itNext(position);
			++itNext;

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.

			if(!mCompare(extractKey(value), extractKey(position.mpNode->mValue)) && // If value >= *position && 
				!mCompare(extractKey(itNext.mpNode->mValue), extractKey(value)))     // if value <= *itNext...
			{
				if(position.mpNode->mpNodeRight) // If there are any nodes to the right... [this expression will always be true as long as we aren't at the end()]
					return DoInsertValueImpl(itNext.mpNode, value, true); // Specifically insert in front of (to the left of) itNext (and thus after 'position').
				return DoInsertValueImpl(position.mpNode, value, false);
			}

			return DoInsertValue(value, has_unique_keys_type()); // If the above specified hint was not useful, then we do a regular insertion.
		}

		// This pathway shouldn't be commonly executed, as the user shouldn't be calling 
		// this hinted version of insert if the user isn't providing a useful hint.

		if(mnSize && !mCompare(extractKey(value), extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))) // If we are non-empty and the value is >= the last node...
			return DoInsertValueImpl((node_type*)mAnchor.mpNodeRight, value, false); // Insert after the last node (doesn't matter if we force left or not).

		return DoInsertValue(value, has_unique_keys_type()); // We are empty or we are inserting at the end.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(iterator position, const key_type& key, true_type) // true_type means keys are unique.
	{
		// This is the pathway for insertion of unique keys (map and set, but not multimap and multiset).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		extract_key extractKey;

		if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
		{
			iterator itNext(position);
			++itNext;

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.
			const bool bPositionLessThanValue = mCompare(extractKey(position.mpNode->mValue), key);

			if(bPositionLessThanValue) // If (value > *position)...
			{
				EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(position.mpNode->mValue))); // Validate that the compare function is sane.

				const bool bValueLessThanNext = mCompare(key, extractKey(itNext.mpNode->mValue));

				if(bValueLessThanNext) // If value < *itNext...
				{
					EASTL_VALIDATE_COMPARE(!mCompare(extractKey(itNext.mpNode->mValue), key)); // Validate that the compare function is sane.

					if(position.mpNode->mpNodeRight)
						return DoInsertKeyImpl(itNext.mpNode, key, true);
					return DoInsertKeyImpl(position.mpNode, key, false);
				}
			}

			return DoInsertKey(key, has_unique_keys_type()).first;
		}

		if(mnSize && mCompare(extractKey(((node_type*)mAnchor.mpNodeRight)->mValue), key))
		{
			EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))); // Validate that the compare function is sane.
			return DoInsertKeyImpl((node_type*)mAnchor.mpNodeRight, key, false);
		}

		return DoInsertKey(key, has_unique_keys_type()).first;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertKey(iterator position, const key_type& key, false_type) // false_type means keys are not unique.
	{
		// This is the pathway for insertion of non-unique keys (multimap and multiset, but not map and set).
		//
		// We follow the same approach as SGI STL/STLPort and use the position as
		// a forced insertion position for the value when possible.
		extract_key extractKey;

		if((position.mpNode != mAnchor.mpNodeRight) && (position.mpNode != &mAnchor)) // If the user specified a specific insertion position...
		{
			iterator itNext(position);
			++itNext;

			// To consider: Change this so that 'position' specifies the position after 
			// where the insertion goes and not the position before where the insertion goes.
			// Doing so would make this more in line with user expectations and with LWG #233.
			if(!mCompare(key, extractKey(position.mpNode->mValue)) && // If value >= *position && 
				!mCompare(extractKey(itNext.mpNode->mValue), key))     // if value <= *itNext...
			{
				if(position.mpNode->mpNodeRight) // If there are any nodes to the right... [this expression will always be true as long as we aren't at the end()]
					return DoInsertKeyImpl(itNext.mpNode, key, true); // Specifically insert in front of (to the left of) itNext (and thus after 'position').
				return DoInsertKeyImpl(position.mpNode, key, false);
			}

			return DoInsertKey(key, has_unique_keys_type()); // If the above specified hint was not useful, then we do a regular insertion.
		}

		// This pathway shouldn't be commonly executed, as the user shouldn't be calling 
		// this hinted version of insert if the user isn't providing a useful hint.
		if(mnSize && !mCompare(key, extractKey(((node_type*)mAnchor.mpNodeRight)->mValue))) // If we are non-empty and the value is >= the last node...
			return DoInsertKeyImpl((node_type*)mAnchor.mpNodeRight, key, false); // Insert after the last node (doesn't matter if we force left or not).

		return DoInsertKey(key, has_unique_keys_type()); // We are empty or we are inserting at the end.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertValueImpl(node_type* pNodeParent, const value_type& value, bool bForceToLeft)
	{
		RBTreeSide side;
		extract_key extractKey;

		// The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
		// In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
		// suggests that we should use the insert hint position to force an ordering. So that's what we do.
		if(bForceToLeft || (pNodeParent == &mAnchor) || mCompare(extractKey(value), extractKey(pNodeParent->mValue)))
			side = kRBTreeSideLeft;
		else
			side = kRBTreeSideRight;

		node_type* const pNodeNew = DoCreateNode(value); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
		RBTreeInsert(pNodeNew, pNodeParent, &mAnchor, side);
		mnSize++;

		return iterator(pNodeNew);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::DoInsertKeyImpl(node_type* pNodeParent, const key_type& key, bool bForceToLeft)
	{
		RBTreeSide side;
		extract_key extractKey;

		// The reason we may want to have bForceToLeft == true is that pNodeParent->mValue and value may be equal.
		// In that case it doesn't matter what side we insert on, except that the C++ LWG #233 improvement report
		// suggests that we should use the insert hint position to force an ordering. So that's what we do.
		if(bForceToLeft || (pNodeParent == &mAnchor) || mCompare(key, extractKey(pNodeParent->mValue)))
			side = kRBTreeSideLeft;
		else
			side = kRBTreeSideRight;

		node_type* const pNodeNew = DoCreateNodeFromKey(key); // Note that pNodeNew->mpLeft, mpRight, mpParent, will be uninitialized.
		RBTreeInsert(pNodeNew, pNodeParent, &mAnchor, side);
		mnSize++;

		return iterator(pNodeNew);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename InputIterator>
	void rbtree<K, V, C, A, E, bM, bU>::insert(InputIterator first, InputIterator last)
	{
		for( ; first != last; ++first)
			DoInsertValue(*first, has_unique_keys_type()); // Or maybe we should call 'insert(end(), *first)' instead. If the first-last range was sorted then this might make some sense.
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::clear()
	{
		// Erase the entire tree. DoNukeSubtree is not a 
		// conventional erase function, as it does no rebalancing.
		DoNukeSubtree((node_type*)mAnchor.mpNodeParent);
		reset();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::reset()
	{
		// The reset function is a special extension function which unilaterally 
		// resets the container to an empty state without freeing the memory of 
		// the contained objects. This is useful for very quickly tearing down a 
		// container built into scratch memory.
		mAnchor.mpNodeRight  = &mAnchor;
		mAnchor.mpNodeLeft   = &mAnchor;
		mAnchor.mpNodeParent = NULL;
		mAnchor.mColor       = kRBTreeColorRed;
		mnSize               = 0;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::erase(iterator position)
	{
		const iterator iErase(position);
		--mnSize; // Interleave this between the two references to itNext. We expect no exceptions to occur during the code below.
		++position;
		RBTreeErase(iErase.mpNode, &mAnchor);
		DoFreeNode(iErase.mpNode);
		return position;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::erase(iterator first, iterator last)
	{
		// We expect that if the user means to clear the container, they will call clear.
		if(EASTL_LIKELY((first.mpNode != mAnchor.mpNodeLeft) || (last.mpNode != &mAnchor))) // If (first != begin or last != end) ...
		{
			// Basic implementation:
			while(first != last)
				first = erase(first);
			return first;

			// Inlined implementation:
			//size_type n = 0;
			//while(first != last)
			//{
			//    const iterator itErase(first);
			//    ++n;
			//    ++first;
			//    RBTreeErase(itErase.mpNode, &mAnchor);
			//    DoFreeNode(itErase.mpNode);
			//}
			//mnSize -= n;
			//return first;
		}

		clear();
		return iterator((node_type*)&mAnchor); // Same as: return end();
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::erase(reverse_iterator position)
	{
		return reverse_iterator(erase((++position).base()));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::reverse_iterator
		rbtree<K, V, C, A, E, bM, bU>::erase(reverse_iterator first, reverse_iterator last)
	{
		// Version which erases in order from first to last.
		// difference_type i(first.base() - last.base());
		// while(i--)
		//     first = erase(first);
		// return first;

		// Version which erases in order from last to first, but is slightly more efficient:
		return reverse_iterator(erase((++last).base(), (++first).base()));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::erase(const key_type* first, const key_type* last)
	{
		// We have no choice but to run a loop like this, as the first/last range could
		// have values that are discontiguously located in the tree. And some may not 
		// even be in the tree.
		while(first != last)
			erase(*first++);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key)
	{
		// To consider: Implement this instead via calling lower_bound and 
		// inspecting the result. The following is an implementation of this:
		//    const iterator it(lower_bound(key));
		//    return ((it.mpNode == &mAnchor) || mCompare(key, extractKey(it.mpNode->mValue))) ? iterator(&mAnchor) : it;
		// We don't currently implement the above because in practice people tend to call 
		// find a lot with trees, but very uncommonly call lower_bound.
		extract_key extractKey;

		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!mCompare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
			{
				pRangeEnd = pCurrent;
				pCurrent  = (node_type*)pCurrent->mpNodeLeft;
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = (node_type*)pCurrent->mpNodeRight;
			}
		}

		if(EASTL_LIKELY((pRangeEnd != &mAnchor) && !mCompare(key, extractKey(pRangeEnd->mValue))))
			return iterator(pRangeEnd);
		return iterator((node_type*)&mAnchor);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::find(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->find(key));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename U, typename Compare2>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2)
	{
		extract_key extractKey;

		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!compare2(extractKey(pCurrent->mValue), u))) // If pCurrent is >= u...
			{
				pRangeEnd = pCurrent;
				pCurrent  = (node_type*)pCurrent->mpNodeLeft;
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!compare2(u, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = (node_type*)pCurrent->mpNodeRight;
			}
		}

		if(EASTL_LIKELY((pRangeEnd != &mAnchor) && !compare2(u, extractKey(pRangeEnd->mValue))))
			return iterator(pRangeEnd);
		return iterator((node_type*)&mAnchor);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	template <typename U, typename Compare2>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::find_as(const U& u, Compare2 compare2) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->find_as(u, compare2));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key)
	{
		extract_key extractKey;

		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(!mCompare(extractKey(pCurrent->mValue), key))) // If pCurrent is >= key...
			{
				pRangeEnd = pCurrent;
				pCurrent  = (node_type*)pCurrent->mpNodeLeft;
			}
			else
			{
				EASTL_VALIDATE_COMPARE(!mCompare(key, extractKey(pCurrent->mValue))); // Validate that the compare function is sane.
				pCurrent  = (node_type*)pCurrent->mpNodeRight;
			}
		}

		return iterator(pRangeEnd);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::lower_bound(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->lower_bound(key));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::iterator
		rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key)
	{
		extract_key extractKey;

		node_type* pCurrent  = (node_type*)mAnchor.mpNodeParent; // Start with the root node.
		node_type* pRangeEnd = (node_type*)&mAnchor;             // Set it to the container end for now.

		while(EASTL_LIKELY(pCurrent)) // Do a walk down the tree.
		{
			if(EASTL_LIKELY(mCompare(key, extractKey(pCurrent->mValue)))) // If key is < pCurrent...
			{
				EASTL_VALIDATE_COMPARE(!mCompare(extractKey(pCurrent->mValue), key)); // Validate that the compare function is sane.
				pRangeEnd = pCurrent;
				pCurrent  = (node_type*)pCurrent->mpNodeLeft;
			}
			else
				pCurrent  = (node_type*)pCurrent->mpNodeRight;
		}

		return iterator(pRangeEnd);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::const_iterator
		rbtree<K, V, C, A, E, bM, bU>::upper_bound(const key_type& key) const
	{
		typedef rbtree<K, V, C, A, E, bM, bU> rbtree_type;
		return const_iterator(const_cast<rbtree_type*>(this)->upper_bound(key));
	}


	// To do: Move this validate function entirely to a template-less implementation.
	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	bool rbtree<K, V, C, A, E, bM, bU>::validate() const
	{
		// Red-black trees have the following canonical properties which we validate here:
		//   1 Every node is either red or black.
		//   2 Every leaf (NULL) is black by defintion. Any number of black nodes may appear in a sequence. 
		//   3 If a node is red, then both its children are black. Thus, on any path from 
		//     the root to a leaf, red nodes must not be adjacent.
		//   4 Every simple path from a node to a descendant leaf contains the same number of black nodes.
		//   5 The mnSize member of the tree must equal the number of nodes in the tree.
		//   6 The tree is sorted as per a conventional binary tree.
		//   7 The comparison function is sane; it obeys strict weak ordering. If mCompare(a,b) is true, then mCompare(b,a) must be false. Both cannot be true.

		extract_key extractKey;

		if(mnSize)
		{
			// Verify basic integrity.
			//if(!mAnchor.mpNodeParent || (mAnchor.mpNodeLeft == mAnchor.mpNodeRight))
			//    return false;             // Fix this for case of empty tree.

			if(mAnchor.mpNodeLeft != RBTreeGetMinChild(mAnchor.mpNodeParent))
				return false;

			if(mAnchor.mpNodeRight != RBTreeGetMaxChild(mAnchor.mpNodeParent))
				return false;

			const size_t nBlackCount   = RBTreeGetBlackCount(mAnchor.mpNodeParent, mAnchor.mpNodeLeft);
			size_type    nIteratedSize = 0;

			for(const_iterator it = begin(); it != end(); ++it, ++nIteratedSize)
			{
				const node_type* const pNode      = (const node_type*)it.mpNode;
				const node_type* const pNodeRight = (const node_type*)pNode->mpNodeRight;
				const node_type* const pNodeLeft  = (const node_type*)pNode->mpNodeLeft;

				// Verify #7 above.
				if(pNodeRight && mCompare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)) && mCompare(extractKey(pNode->mValue), extractKey(pNodeRight->mValue))) // Validate that the compare function is sane.
					return false;

				// Verify #7 above.
				if(pNodeLeft && mCompare(extractKey(pNodeLeft->mValue), extractKey(pNode->mValue)) && mCompare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue))) // Validate that the compare function is sane.
					return false;

				// Verify item #1 above.
				if((pNode->mColor != kRBTreeColorRed) && (pNode->mColor != kRBTreeColorBlack))
					return false;

				// Verify item #3 above.
				if(pNode->mColor == kRBTreeColorRed)
				{
					if((pNodeRight && (pNodeRight->mColor == kRBTreeColorRed)) ||
						(pNodeLeft  && (pNodeLeft->mColor  == kRBTreeColorRed)))
						return false;
				}

				// Verify item #6 above.
				if(pNodeRight && mCompare(extractKey(pNodeRight->mValue), extractKey(pNode->mValue)))
					return false;

				if(pNodeLeft && mCompare(extractKey(pNode->mValue), extractKey(pNodeLeft->mValue)))
					return false;

				if(!pNodeRight && !pNodeLeft) // If we are at a bottom node of the tree...
				{
					// Verify item #4 above.
					if(RBTreeGetBlackCount(mAnchor.mpNodeParent, pNode) != nBlackCount)
						return false;
				}
			}

			// Verify item #5 above.
			if(nIteratedSize != mnSize)
				return false;

			return true;
		}
		else
		{
			if((mAnchor.mpNodeLeft != &mAnchor) || (mAnchor.mpNodeRight != &mAnchor))
				return false;
		}

		return true;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline int rbtree<K, V, C, A, E, bM, bU>::validate_iterator(const_iterator i) const
	{
		// To do: Come up with a more efficient mechanism of doing this.

		for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
		{
			if(temp == i)
				return (isf_valid | isf_current | isf_can_dereference);
		}

		if(i == end())
			return (isf_valid | isf_current); 

		return isf_none;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline typename rbtree<K, V, C, A, E, bM, bU>::node_type*
		rbtree<K, V, C, A, E, bM, bU>::DoAllocateNode()
	{
		return (node_type*)allocate_memory(mAllocator, sizeof(node_type), kValueAlignment, kValueAlignmentOffset);
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	inline void rbtree<K, V, C, A, E, bM, bU>::DoFreeNode(node_type* pNode)
	{
		pNode->~node_type();
		EASTLFree(mAllocator, pNode, sizeof(node_type));
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::node_type*
		rbtree<K, V, C, A, E, bM, bU>::DoCreateNodeFromKey(const key_type& key)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
		node_type* const pNode = DoAllocateNode();

			::new(&pNode->mValue) value_type(key);

#if EASTL_DEBUG
		pNode->mpNodeRight  = NULL;
		pNode->mpNodeLeft   = NULL;
		pNode->mpNodeParent = NULL;
		pNode->mColor       = kRBTreeColorBlack;
#endif

		return pNode;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::node_type*
		rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const value_type& value)
	{
		// Note that this function intentionally leaves the node pointers uninitialized.
		// The caller would otherwise just turn right around and modify them, so there's
		// no point in us initializing them to anything (except in a debug build).
		node_type* const pNode = DoAllocateNode();

			::new(&pNode->mValue) value_type(value);
#if EASTL_DEBUG
		pNode->mpNodeRight  = NULL;
		pNode->mpNodeLeft   = NULL;
		pNode->mpNodeParent = NULL;
		pNode->mColor       = kRBTreeColorBlack;
#endif

		return pNode;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::node_type*
		rbtree<K, V, C, A, E, bM, bU>::DoCreateNode(const node_type* pNodeSource, node_type* pNodeParent)
	{
		node_type* const pNode = DoCreateNode(pNodeSource->mValue);

		pNode->mpNodeRight  = NULL;
		pNode->mpNodeLeft   = NULL;
		pNode->mpNodeParent = pNodeParent;
		pNode->mColor       = pNodeSource->mColor;

		return pNode;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	typename rbtree<K, V, C, A, E, bM, bU>::node_type*
		rbtree<K, V, C, A, E, bM, bU>::DoCopySubtree(const node_type* pNodeSource, node_type* pNodeDest)
	{
		node_type* const pNewNodeRoot = DoCreateNode(pNodeSource, pNodeDest);

			// Copy the right side of the tree recursively.
			if(pNodeSource->mpNodeRight)
				pNewNodeRoot->mpNodeRight = DoCopySubtree((const node_type*)pNodeSource->mpNodeRight, pNewNodeRoot);

			node_type* pNewNodeLeft;

			for(pNodeSource = (node_type*)pNodeSource->mpNodeLeft, pNodeDest = pNewNodeRoot; 
				pNodeSource;
				pNodeSource = (node_type*)pNodeSource->mpNodeLeft, pNodeDest = pNewNodeLeft)
			{
				pNewNodeLeft = DoCreateNode(pNodeSource, pNodeDest);

				pNodeDest->mpNodeLeft = pNewNodeLeft;

				// Copy the right side of the tree recursively.
				if(pNodeSource->mpNodeRight)
					pNewNodeLeft->mpNodeRight = DoCopySubtree((const node_type*)pNodeSource->mpNodeRight, pNewNodeLeft);
			}
		return pNewNodeRoot;
	}


	template <typename K, typename V, typename C, typename A, typename E, bool bM, bool bU>
	void rbtree<K, V, C, A, E, bM, bU>::DoNukeSubtree(node_type* pNode)
	{
		while(pNode) // Recursively traverse the tree and destroy items as we go.
		{
			DoNukeSubtree((node_type*)pNode->mpNodeRight);

			node_type* const pNodeLeft = (node_type*)pNode->mpNodeLeft;
			DoFreeNode(pNode);
			pNode = pNodeLeft;
		}
	}



	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator==(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return (a.size() == b.size()) && equal(a.begin(), a.end(), b.begin());
	}


	// Note that in operator< we do comparisons based on the tree value_type with operator<() of the
	// value_type instead of the tree's Compare function. For set/multiset, the value_type is T, while
	// for map/multimap the value_type is a pair<Key, T>. operator< for pair can be seen by looking
	// utility.h, but it basically is uses the operator< for pair.first and pair.second. The C++ standard
	// appears to require this behaviour, whether intentionally or not. If anything, a good reason to do
	// this is for consistency. A map and a vector that contain the same items should compare the same.
	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator<(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator!=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(a == b);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator>(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return b < a;
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator<=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(b < a);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline bool operator>=(const rbtree<K, V, C, A, E, bM, bU>& a, const rbtree<K, V, C, A, E, bM, bU>& b)
	{
		return !(a < b);
	}


	template <typename K, typename V, typename A, typename C, typename E, bool bM, bool bU>
	inline void swap(rbtree<K, V, C, A, E, bM, bU>& a, rbtree<K, V, C, A, E, bM, bU>& b)
	{
		a.swap(b);
	}


///////////////////////////////////////////////////////////////////////////////
// EASTL/functional.h
// Written and maintained by Paul Pedriana - 2005
///////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////
	// Primary C++ functions
	///////////////////////////////////////////////////////////////////////

	template <typename Argument, typename Result>
	struct unary_function
	{
		typedef Argument argument_type;
		typedef Result   result_type;
	};


	template <typename Argument1, typename Argument2, typename Result>
	struct binary_function
	{
		typedef Argument1 first_argument_type;
		typedef Argument2 second_argument_type;
		typedef Result    result_type;
	};


	template <typename T>
	struct plus : public binary_function<T, T, T>
	{
		T operator()(const T& a, const T& b) const
		{ return a + b; }
	};

	template <typename T>
	struct minus : public binary_function<T, T, T>
	{
		T operator()(const T& a, const T& b) const
		{ return a - b; }
	};

	template <typename T>
	struct multiplies : public binary_function<T, T, T>
	{
		T operator()(const T& a, const T& b) const
		{ return a * b; }
	};

	template <typename T>
	struct divides : public binary_function<T, T, T>
	{
		T operator()(const T& a, const T& b) const
		{ return a / b; }
	};

	template <typename T>
	struct modulus : public binary_function<T, T, T>
	{
		T operator()(const T& a, const T& b) const
		{ return a % b; }
	};

	template <typename T>
	struct negate : public unary_function<T, T>
	{
		T operator()(const T& a) const
		{ return -a; }
	};

	template <typename T>
	struct equal_to : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a == b; }
	};

	template <typename T, typename Compare>
	bool validate_equal_to(const T& a, const T& b, Compare compare)
	{
		return compare(a, b) == compare(b, a);
	}

	template <typename T>
	struct not_equal_to : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a != b; }
	};

	template <typename T, typename Compare>
	bool validate_not_equal_to(const T& a, const T& b, Compare compare)
	{
		return compare(a, b) == compare(b, a); // We want the not equal comparison results to be equal.
	}

	/// str_equal_to
	///
	/// Compares two 0-terminated string types.
	/// The T types are expected to be iterators or act like iterators.
	///
	/// Example usage:
	///     hash_set<const char*, eastl_hash<const char*>, str_equal_to<const char*> > stringHashSet;
	///
	/// Note:
	/// You couldn't use str_equal_to like this:
	///     bool result = equal("hi", "hi" + 2, "ho", str_equal_to<const char*>());
	/// This is because equal tests an array of something, with each element by
	/// the comparison function. But str_equal_to tests an array of something itself.
	///
	template <typename T>
	struct str_equal_to : public binary_function<T, T, bool>
	{
		bool operator()(T a, T b) const
		{
			while(*a && (*a == *b))
			{
				++a;
				++b;
			}
			return (*a == *b);
		}
	};

	template <typename T>
	struct greater : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a > b; }
	};

	template <typename T, typename Compare>
	bool validate_greater(const T& a, const T& b, Compare compare)
	{
		return !compare(a, b) || !compare(b, a); // If (a > b), then !(b > a)
	}

	template <typename T>
	struct less : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a < b; }
	};

	template <typename T, typename Compare>
	bool validate_less(const T& a, const T& b, Compare compare)
	{
		return !compare(a, b) || !compare(b, a); // If (a < b), then !(b < a)
	}

	template <typename T>
	struct greater_equal : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a >= b; }
	};

	template <typename T, typename Compare>
	bool validate_greater_equal(const T& a, const T& b, Compare compare)
	{
		return !compare(a, b) || !compare(b, a); // If (a >= b), then !(b >= a)
	}

	template <typename T>
	struct less_equal : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a <= b; }
	};

	template <typename T, typename Compare>
	bool validate_less_equal(const T& a, const T& b, Compare compare)
	{
		return !compare(a, b) || !compare(b, a); // If (a <= b), then !(b <= a)
	}

	template <typename T>
	struct logical_and : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a && b; }
	};

	template <typename T>
	struct logical_or : public binary_function<T, T, bool>
	{
		bool operator()(const T& a, const T& b) const
		{ return a || b; }
	};

	template <typename T>
	struct logical_not : public unary_function<T, bool>
	{
		bool operator()(const T& a) const
		{ return !a; }
	};



	///////////////////////////////////////////////////////////////////////
	// Dual type functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename U>
	struct equal_to_2 : public binary_function<T, U, bool>
	{
		bool operator()(const T& a, const U& b) const
		{ return a == b; }
	};

	template <typename T, typename U>
	struct not_equal_to_2 : public binary_function<T, U, bool>
	{
		bool operator()(const T& a, const U& b) const
		{ return a != b; }
	};

	template <typename T, typename U>
	struct less_2 : public binary_function<T, U, bool>
	{
		bool operator()(const T& a, const U& b) const
		{ return a < b; }
	};




	/// unary_negate
	///
	template <typename Predicate>
	class unary_negate : public unary_function<typename Predicate::argument_type, bool>
	{
	protected:
		Predicate mPredicate;
	public:
		explicit unary_negate(const Predicate& a)
			: mPredicate(a) {}
		bool operator()(const typename Predicate::argument_type& a) const
		{ return !mPredicate(a); }
	};

	template <typename Predicate>
	inline unary_negate<Predicate> not1(const Predicate& predicate)
	{ return unary_negate<Predicate>(predicate); }



	/// binary_negate
	///
	template <typename Predicate>
	class binary_negate : public binary_function<typename Predicate::first_argument_type, typename Predicate::second_argument_type, bool>
	{
	protected:
		Predicate mPredicate;
	public:
		explicit binary_negate(const Predicate& a)
			: mPredicate(a) { }
		bool operator()(const typename Predicate::first_argument_type& a, const typename Predicate::second_argument_type& b) const
		{ return !mPredicate(a, b); }
	};

	template <typename Predicate>
	inline binary_negate<Predicate> not2(const Predicate& predicate)
	{ return binary_negate<Predicate>(predicate); }




	///////////////////////////////////////////////////////////////////////
	// bind
	///////////////////////////////////////////////////////////////////////

	/// bind1st
	///
	template <typename Operation>
	class binder1st : public unary_function<typename Operation::second_argument_type, typename Operation::result_type>
	{
	protected:
		typename Operation::first_argument_type value;
		Operation op;

	public:
		binder1st(const Operation& x, const typename Operation::first_argument_type& y)
			: value(y), op(x) { }

		typename Operation::result_type operator()(const typename Operation::second_argument_type& x) const
		{ return op(value, x); }

		typename Operation::result_type operator()(typename Operation::second_argument_type& x) const
		{ return op(value, x); }
	};


	template <typename Operation, typename T>
	inline binder1st<Operation> bind1st(const Operation& op, const T& x)
	{
		typedef typename Operation::first_argument_type value;
		return binder1st<Operation>(op, value(x));
	}


	/// bind2nd
	///
	template <typename Operation>
	class binder2nd : public unary_function<typename Operation::first_argument_type, typename Operation::result_type>
	{
	protected:
		Operation op;
		typename Operation::second_argument_type value;

	public:
		binder2nd(const Operation& x, const typename Operation::second_argument_type& y)
			: op(x), value(y) { }

		typename Operation::result_type operator()(const typename Operation::first_argument_type& x) const
		{ return op(x, value); }

		typename Operation::result_type operator()(typename Operation::first_argument_type& x) const
		{ return op(x, value); }
	};


	template <typename Operation, typename T>
	inline binder2nd<Operation> bind2nd(const Operation& op, const T& x)
	{
		typedef typename Operation::second_argument_type value;
		return binder2nd<Operation>(op, value(x));
	}




	///////////////////////////////////////////////////////////////////////
	// pointer_to_unary_function
	///////////////////////////////////////////////////////////////////////

	/// pointer_to_unary_function
	///
	/// This is an adapter template which converts a pointer to a standalone
	/// function to a function object. This allows standalone functions to 
	/// work in many cases where the system requires a function object.
	///
	/// Example usage:
	///     ptrdiff_t Rand(ptrdiff_t n) { return rand() % n; } // Note: The C rand function is poor and slow.
	///     pointer_to_unary_function<ptrdiff_t, ptrdiff_t> randInstance(Rand);
	///     random_shuffle(pArrayBegin, pArrayEnd, randInstance);
	///
	template <typename Arg, typename Result>
	class pointer_to_unary_function : public unary_function<Arg, Result>
	{
	protected:
		Result (*mpFunction)(Arg);

	public:
		pointer_to_unary_function()
		{ }

		explicit pointer_to_unary_function(Result (*pFunction)(Arg))
			: mpFunction(pFunction) { }

		Result operator()(Arg x) const
		{ return mpFunction(x); } 
	};


	/// ptr_fun
	///
	/// This ptr_fun is simply shorthand for usage of pointer_to_unary_function.
	///
	/// Example usage (actually, you don't need to use ptr_fun here, but it works anyway):
	///    int factorial(int x) { return (x > 1) ? (x * factorial(x - 1)) : x; }
	///    transform(pIntArrayBegin, pIntArrayEnd, pIntArrayBegin, ptr_fun(factorial));
	///
	template <typename Arg, typename Result>
	inline pointer_to_unary_function<Arg, Result> ptr_fun(Result (*pFunction)(Arg))
	{ return pointer_to_unary_function<Arg, Result>(pFunction); }





	///////////////////////////////////////////////////////////////////////
	// pointer_to_binary_function
	///////////////////////////////////////////////////////////////////////

	/// pointer_to_binary_function
	///
	/// This is an adapter template which converts a pointer to a standalone
	/// function to a function object. This allows standalone functions to 
	/// work in many cases where the system requires a function object.
	///
	template <typename Arg1, typename Arg2, typename Result>
	class pointer_to_binary_function : public binary_function<Arg1, Arg2, Result>
	{
	protected:
		Result (*mpFunction)(Arg1, Arg2);

	public:
		pointer_to_binary_function()
		{ }

		explicit pointer_to_binary_function(Result (*pFunction)(Arg1, Arg2))
			: mpFunction(pFunction) {}

		Result operator()(Arg1 x, Arg2 y) const
		{ return mpFunction(x, y); }
	};


	/// This ptr_fun is simply shorthand for usage of pointer_to_binary_function.
	///
	/// Example usage (actually, you don't need to use ptr_fun here, but it works anyway):
	///    int multiply(int x, int y) { return x * y; }
	///    transform(pIntArray1Begin, pIntArray1End, pIntArray2Begin, pIntArray1Begin, ptr_fun(multiply));
	///
	template <typename Arg1, typename Arg2, typename Result>
	inline pointer_to_binary_function<Arg1, Arg2, Result> ptr_fun(Result (*pFunction)(Arg1, Arg2))
	{ return pointer_to_binary_function<Arg1, Arg2, Result>(pFunction); }






	///////////////////////////////////////////////////////////////////////
	// mem_fun
	// mem_fun1
	//
	// Note that mem_fun calls member functions via *pointers* to classes 
	// and not instances of classes. mem_fun_ref is for calling functions
	// via instances of classes or references to classes.
	//
	///////////////////////////////////////////////////////////////////////

	/// mem_fun_t
	///
	/// Member function with no arguments.
	///
	template <typename Result, typename T> 
	class mem_fun_t : public unary_function<T*, Result>
	{
	public:
		typedef Result (T::*MemberFunction)();

		MI_INLINE explicit mem_fun_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(T* pT) const
		{
			return (pT->*mpMemberFunction)();
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// mem_fun1_t
	///
	/// Member function with one argument.
	///
	template <typename Result, typename T, typename Argument>
	class mem_fun1_t : public binary_function<T*, Argument, Result>
	{
	public:
		typedef Result (T::*MemberFunction)(Argument);

		MI_INLINE explicit mem_fun1_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(T* pT, Argument arg) const
		{
			return (pT->*mpMemberFunction)(arg);
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// const_mem_fun_t
	///
	/// Const member function with no arguments.
	/// Note that we inherit from unary_function<const T*, Result>
	/// instead of what the C++ standard specifies: unary_function<T*, Result>.
	/// The C++ standard is in error and this has been recognized by the defect group.
	///
	template <typename Result, typename T>
	class const_mem_fun_t : public unary_function<const T*, Result>
	{
	public:
		typedef Result (T::*MemberFunction)() const;

		MI_INLINE explicit const_mem_fun_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(const T* pT) const
		{
			return (pT->*mpMemberFunction)();
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// const_mem_fun1_t
	///
	/// Const member function with one argument.
	/// Note that we inherit from unary_function<const T*, Result>
	/// instead of what the C++ standard specifies: unary_function<T*, Result>.
	/// The C++ standard is in error and this has been recognized by the defect group.
	///
	template <typename Result, typename T, typename Argument>
	class const_mem_fun1_t : public binary_function<const T*, Argument, Result>
	{
	public:
		typedef Result (T::*MemberFunction)(Argument) const;

		MI_INLINE explicit const_mem_fun1_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(const T* pT, Argument arg) const
		{
			return (pT->*mpMemberFunction)(arg);
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// mem_fun
	///
	/// This is the high level interface to the mem_fun_t family.
	///
	/// Example usage:
	///    struct TestClass { void print() { puts("hello"); } }
	///    TestClass* pTestClassArray[3] = { ... };
	///    for_each(pTestClassArray, pTestClassArray + 3, &TestClass::print);
	///
	template <typename Result, typename T>
	MI_INLINE mem_fun_t<Result, T>
		mem_fun(Result (T::*MemberFunction)())
	{
		return mimp::mem_fun_t<Result, T>(MemberFunction);
	}

	template <typename Result, typename T, typename Argument>
	MI_INLINE mem_fun1_t<Result, T, Argument>
		mem_fun(Result (T::*MemberFunction)(Argument))
	{
		return mimp::mem_fun1_t<Result, T, Argument>(MemberFunction);
	}

	template <typename Result, typename T>
	MI_INLINE const_mem_fun_t<Result, T>
		mem_fun(Result (T::*MemberFunction)() const)
	{
		return mimp::const_mem_fun_t<Result, T>(MemberFunction);
	}

	template <typename Result, typename T, typename Argument>
	MI_INLINE const_mem_fun1_t<Result, T, Argument>
		mem_fun(Result (T::*MemberFunction)(Argument) const)
	{
		return mimp::const_mem_fun1_t<Result, T, Argument>(MemberFunction);
	}





	///////////////////////////////////////////////////////////////////////
	// mem_fun_ref
	// mem_fun1_ref
	//
	///////////////////////////////////////////////////////////////////////

	/// mem_fun_ref_t
	///
	template <typename Result, typename T>
	class mem_fun_ref_t : public unary_function<T, Result>
	{
	public:
		typedef Result (T::*MemberFunction)();

		MI_INLINE explicit mem_fun_ref_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(T& t) const
		{
			return (t.*mpMemberFunction)();
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// mem_fun1_ref_t
	///
	template <typename Result, typename T, typename Argument>
	class mem_fun1_ref_t : public binary_function<T, Argument, Result>
	{
	public:
		typedef Result (T::*MemberFunction)(Argument);

		MI_INLINE explicit mem_fun1_ref_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(T& t, Argument arg) const
		{
			return (t.*mpMemberFunction)(arg);
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// const_mem_fun_ref_t
	///
	template <typename Result, typename T>
	class const_mem_fun_ref_t : public unary_function<T, Result>
	{
	public:
		typedef Result (T::*MemberFunction)() const;

		MI_INLINE explicit const_mem_fun_ref_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(const T& t) const
		{
			return (t.*mpMemberFunction)();
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// const_mem_fun1_ref_t
	///
	template <typename Result, typename T, typename Argument>
	class const_mem_fun1_ref_t : public binary_function<T, Argument, Result>
	{
	public:
		typedef Result (T::*MemberFunction)(Argument) const;

		MI_INLINE explicit const_mem_fun1_ref_t(MemberFunction pMemberFunction)
			: mpMemberFunction(pMemberFunction)
		{
			// Empty
		}

		MI_INLINE Result operator()(const T& t, Argument arg) const
		{
			return (t.*mpMemberFunction)(arg);
		}

	protected:
		MemberFunction mpMemberFunction;
	};


	/// mem_fun_ref
	/// Example usage:
	///    struct TestClass { void print() { puts("hello"); } }
	///    TestClass testClassArray[3];
	///    for_each(testClassArray, testClassArray + 3, &TestClass::print);
	///
	template <typename Result, typename T>
	MI_INLINE mem_fun_ref_t<Result, T>
		mem_fun_ref(Result (T::*MemberFunction)())
	{
		return mimp::mem_fun_ref_t<Result, T>(MemberFunction);
	}

	template <typename Result, typename T, typename Argument>
	MI_INLINE mem_fun1_ref_t<Result, T, Argument>
		mem_fun_ref(Result (T::*MemberFunction)(Argument))
	{
		return mimp::mem_fun1_ref_t<Result, T, Argument>(MemberFunction);
	}

	template <typename Result, typename T>
	MI_INLINE const_mem_fun_ref_t<Result, T>
		mem_fun_ref(Result (T::*MemberFunction)() const)
	{
		return mimp::const_mem_fun_ref_t<Result, T>(MemberFunction);
	}

	template <typename Result, typename T, typename Argument>
	MI_INLINE const_mem_fun1_ref_t<Result, T, Argument>
		mem_fun_ref(Result (T::*MemberFunction)(Argument) const)
	{
		return mimp::const_mem_fun1_ref_t<Result, T, Argument>(MemberFunction);
	}




	///////////////////////////////////////////////////////////////////////
	// eastl_hash
	///////////////////////////////////////////////////////////////////////

	template <typename T> struct eastl_hash;

	template <typename T> struct eastl_hash<T*> // Note that we use the pointer as-is and don't divide by sizeof(T*). This is because the table is of a prime size and this division doesn't benefit distribution.
	{ size_t operator()(T* p) const { return size_t(uintptr_t(p)); } };

	template <> struct eastl_hash<bool>
	{ size_t operator()(bool val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<char>
	{ size_t operator()(char val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<signed char>
	{ size_t operator()(signed char val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<unsigned char>
	{ size_t operator()(unsigned char val) const { return static_cast<size_t>(val); } };

#ifndef EA_WCHAR_T_NON_NATIVE // If wchar_t is a native type instead of simply a define to an existing type...
	template <> struct eastl_hash<wchar_t>
	{ size_t operator()(wchar_t val) const { return static_cast<size_t>(val); } };
#endif

	template <> struct eastl_hash<signed short>
	{ size_t operator()(short val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<unsigned short>
	{ size_t operator()(unsigned short val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<signed int>
	{ size_t operator()(signed int val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<unsigned int>
	{ size_t operator()(unsigned int val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<signed long>
	{ size_t operator()(signed long val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<unsigned long>
	{ size_t operator()(unsigned long val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<signed long long>
	{ size_t operator()(signed long long val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<unsigned long long>
	{ size_t operator()(unsigned long long val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<float>
	{ size_t operator()(float val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<double>
	{ size_t operator()(double val) const { return static_cast<size_t>(val); } };

	template <> struct eastl_hash<long double>
	{ size_t operator()(long double val) const { return static_cast<size_t>(val); } };


	///////////////////////////////////////////////////////////////////////////
	// string hashes
	//
	// Note that our string hashes here intentionally are slow for long strings.
	// The reasoning for this is so:
	//    - The large majority of hashed strings are only a few bytes long.
	//    - The eastl_hash function is significantly more efficient if it can make this assumption.
	//    - The user is welcome to make a custom eastl_hash for those uncommon cases where
	//      long strings need to be hashed. Indeed, the user can probably make a 
	//      special eastl_hash customized for such strings that's better than what we provide.
	///////////////////////////////////////////////////////////////////////////

	template <> struct eastl_hash<mimp::MiI8*>
	{
		size_t operator()(const mimp::MiI8* p) const
		{
			size_t c, result = 2166136261U;  // FNV1 eastl_hash. Perhaps the best string eastl_hash.
			while((c = (mimp::MiU8)*p++) != 0)  // Using '!=' disables compiler warnings.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	template <> struct eastl_hash<const mimp::MiI8*>
	{
		size_t operator()(const mimp::MiI8* p) const
		{
			size_t c, result = 2166136261U;
			while((c = (mimp::MiU8)*p++) != 0) // cast to unsigned 8 bit.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	template <> struct eastl_hash<mimp::MiI16*>
	{
		size_t operator()(const mimp::MiI16* p) const
		{
			size_t c, result = 2166136261U;
			while((c = (MiU16)*p++) != 0) // cast to unsigned 16 bit.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	template <> struct eastl_hash<const mimp::MiI16*>
	{
		size_t operator()(const mimp::MiI16* p) const
		{
			size_t c, result = 2166136261U;
			while((c = (MiU16)*p++) != 0) // cast to unsigned 16 bit.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	template <> struct eastl_hash<mimp::MiU32*>
	{
		size_t operator()(const mimp::MiU32* p) const
		{
			size_t c, result = 2166136261U;
			while((c = (mimp::MiU32)*p++) != 0) // cast to unsigned 32 bit.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	template <> struct eastl_hash<const mimp::MiU32*>
	{
		size_t operator()(const mimp::MiU32* p) const
		{
			size_t c, result = 2166136261U;
			while((c = (mimp::MiU32)*p++) != 0) // cast to unsigned 32 bit.
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};

	/// string_hash
	///
	/// Defines a generic string eastl_hash for an arbitrary EASTL basic_string container.
	///
	/// Example usage:
	///    mimp::hash_set<MyString, mimp::string_hash<MyString> > hashSet;
	///
	template <typename String>
	struct string_hash
	{
		typedef String                                         string_type;
		typedef typename String::value_type                    value_type;
		typedef typename mimp::add_unsigned<value_type>::type unsigned_value_type;

		size_t operator()(const string_type& s) const
		{
			const unsigned_value_type* p = (const unsigned_value_type*)s.c_str();
			size_t c, result = 2166136261U;
			while((c = *p++) != 0)
				result = (result * 16777619) ^ c;
			return (size_t)result;
		}
	};


} // namespace mimp

#pragma warning(pop)

#endif // include guard
