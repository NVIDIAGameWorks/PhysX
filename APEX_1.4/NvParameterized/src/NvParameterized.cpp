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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <PxSimpleTypes.h>
#include <PsIntrinsics.h>
#include "ApexDefs.h"

#if PX_VC && !PX_PS4
#pragma warning(disable:4996 4310)
#endif

#include "PsAsciiConversion.h"

#include "nvparameterized/NvParameterized.h"

#include "NvParameters.h"
#include "NvTraitsInternal.h"
#include "PsString.h"


namespace NvParameterized
{

#define NV_ERR_CHECK_RETURN(x) if(x!=ERROR_NONE) return(x);
#define NV_BOOL_RETURN(x) if((x)!=ERROR_NONE) return(false);

#define DEBUG_ASSERT(x)
//#define DEBUG_ASSERT(x) PX_ASSERT(x)

char Spog[] = "test";

#define COND_DUP(str) (mStaticAllocation) ? (char *)(str) : local_strdup(str)

template <typename T>
void swap(T& a, T&b)
{
	T tmp = b;
	b = a;
	a = tmp;
}


int32_t local_strcmp(const char* s1, const char* s2)
{
	return physx::shdfnd::strcmp(s1, s2);
}

int32_t local_stricmp(const char* s1, const char* s2)
{
	return physx::shdfnd::stricmp(s1, s2);
}

int32_t local_sprintf_s( char * _DstBuf, size_t _DstSize, const char * _Format, ...)
{
	if ( _DstBuf == NULL || _Format == NULL )
	{
		return -1;
	}

	va_list arg;
	va_start( arg, _Format );
	int32_t r = physx::shdfnd::vsnprintf( _DstBuf, _DstSize, _Format, arg );
	va_end(arg);

	return r;
}


static PX_INLINE void* allocAligned(NvParameterized::Traits* t, uint32_t size, uint32_t align)
{
	void* buf = t->alloc(size, align);
	if( (size_t)buf & (align - 1) )
	{
		t->free(buf);
		return 0;
	}

	return buf;
}

static PX_INLINE double RandomF64()
{
	return (double)rand() / RAND_MAX;
}

static PX_INLINE float RandomF32()
{
	return (float)RandomF64();
}

static PX_INLINE uint32_t RandomIdx(uint32_t m, uint32_t M)
{
	return uint32_t(m + RandomF64() * (M - m) + 0.99); // FIXME: round
}

static PX_INLINE physx::PxVec2 RandomVec2()
{
	return physx::PxVec2(RandomF32(), RandomF32());
}

static PX_INLINE physx::PxVec3 RandomVec3()
{
	return physx::PxVec3(RandomF32(), RandomF32(), RandomF32());
}

static PX_INLINE physx::PxVec4 RandomVec4()
{
	return physx::PxVec4(RandomF32(), RandomF32(), RandomF32(), RandomF32());
}

static PX_INLINE physx::PxQuat RandomQuat()
{
	return physx::PxQuat(RandomF32(), RandomF32(), RandomF32(), RandomF32());
}

static PX_INLINE uint32_t RandomU32()
{
	return (uint32_t)rand();
}

static PX_INLINE uint64_t RandomU64()
{
	uint32_t u32s[2];
	u32s[0] = RandomU32();
	u32s[1] = RandomU32();

	return *(uint64_t*)&u32s[0];
}

static PX_INLINE bool notEqual(const char *a, const char *b)
{
	if(a == NULL && b == NULL)
		return(false);

	return (a == NULL && b != NULL)
		|| (a != NULL && b == NULL)
		|| 0 != strcmp(a, b);
}

static PX_INLINE bool notEqual(uint8_t a,uint8_t b)
{
	return a!=b;
}
static PX_INLINE bool notEqual(uint16_t a,uint16_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(uint32_t a,uint32_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(uint64_t a,uint64_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(int8_t a,int8_t b)
{
	return a!=b;
}
static PX_INLINE bool notEqual(int16_t a,int16_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(int32_t a,int32_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(int64_t a,int64_t b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(float a,float b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(float (&a)[12],float (&b)[12])
{
	return a[0] != b[0] || a[1] != b[1] || a[2] != b[2] || 
			a[3] != b[3] || a[4] != b[4] || a[5] != b[5] || 
			a[6] != b[6] || a[7] != b[7] || a[8] != b[8] || 
			a[9] != b[9] || a[10] != b[10] || a[11] != b[11];
}

static PX_INLINE bool notEqual(double a,double b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(bool a,bool b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(const physx::PxVec2 &a,const physx::PxVec2 &b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(const physx::PxVec3 &a,const physx::PxVec3 &b)
{
	return a!=b;
}

static PX_INLINE bool notEqual(const physx::PxVec4 &a,const physx::PxVec4 &b)
{
	return a!=b;
}

// This was formerly in the NvQuat.h file but was removed due to other code bases requiring it to be gone
static PX_INLINE bool operator!=( const physx::PxQuat& a, const physx::PxQuat& b )
{
	return a.x != b.x
		|| a.y != b.y
		|| a.z != b.z
		|| a.w != b.w;
}

static PX_INLINE bool notEqual(const physx::PxQuat &a,physx::PxQuat &b)
{
	return a != b;
}

static PX_INLINE bool notEqual(const physx::PxMat33 &a,physx::PxMat33 &b)
{
	const float *qa = a.front();
	const float *qb = b.front();
	return qa[0] != qb[0] || qa[1] != qb[1] || qa[2] != qb[2] || qa[3] != qb[3] ||
		   qa[4] != qb[4] || qa[5] != qb[5] || qa[6] != qb[6] || qa[7] != qb[7] ||
		   qa[8] != qb[8];
}

static PX_INLINE bool notEqual(const physx::PxMat44 &a,physx::PxMat44 &b)
{
	return a.column0.w != b.column0.w || a.column0.x != b.column0.x || a.column0.y != b.column0.y || a.column0.z != b.column0.z ||
		a.column1.w != b.column1.w || a.column1.x != b.column1.x || a.column1.y != b.column1.y || a.column1.z != b.column1.z ||
		a.column2.w != b.column2.w || a.column2.x != b.column2.x || a.column2.y != b.column2.y || a.column2.z != b.column2.z ||
		a.column3.w != b.column3.w || a.column3.x != b.column3.x || a.column3.y != b.column3.y || a.column3.z != b.column3.z;
}

static PX_INLINE bool notEqual(const physx::PxBounds3 &a,const physx::PxBounds3 &b)
{
	return a.minimum != b.minimum || a.maximum != b.maximum;
}

static PX_INLINE bool notEqual(const physx::PxTransform &a,const physx::PxTransform &b)
{
	return a.p != b.p || a.q != b.q;
}

#define CHECK_FINITE(t) { \
	const t* _fs = (const t*)&val; \
	for(size_t _j = 0; _j < sizeof(val) / sizeof(t); ++_j) \
	{ \
		if( physx::PxIsFinite(_fs[_j]) ) continue; \
		char _longName[256]; \
		handle.getLongName(_longName, sizeof(_longName)); \
		NV_PARAM_TRAITS_WARNING(mParameterizedTraits, "%s: setting non-finite floating point value", _longName); \
		break; \
	} \
}

#define CHECK_FINITE_ARRAY(t) { \
	for(int32_t _i = 0; _i < int32_t(n); ++_i) \
	{ \
		const t* _fs = (const t*)(&array[_i]); \
		for(size_t _j = 0; _j < sizeof(array[0]) / sizeof(t); ++_j) \
		{ \
			if( physx::PxIsFinite(_fs[_j]) ) continue; \
			char _longName[256]; \
			handle.getLongName(_longName, sizeof(_longName)); \
			NV_PARAM_TRAITS_WARNING(mParameterizedTraits, "%s[%d]: setting non-finite floating point value", _longName, (int)_i); \
			break; \
		} \
	} \
}

#define CHECK_F32_FINITE \
	CHECK_FINITE(float)

#define CHECK_F32_FINITE_ARRAY \
	CHECK_FINITE_ARRAY(float)

#define CHECK_F64_FINITE \
	CHECK_FINITE(double)

#define CHECK_F64_FINITE_ARRAY \
	CHECK_FINITE_ARRAY(double)

//******************************************************************************
//*** Local functions
//******************************************************************************

#if 0
static void *local_malloc(uint32_t bytes)
{
	return(NV_ALLOC(bytes, NV_DEBUG_EXP("NvParameterized::local_malloc")));
}

static void *local_realloc(void *buf, uint32_t bytes)
{
    return(GetApexAllocator()->realloc(buf, bytes));
}

static void local_free(void *buf)
{
    NV_FREE(buf);
}
#else

static void *local_malloc(uint32_t bytes)
{
    return(malloc(bytes));
}

static void *local_realloc(void *buf, uint32_t bytes)
{
    return(realloc(buf, bytes));
}

static void local_free(void *buf)
{
    free(buf);
}

#endif

static char *local_strdup(const char *str)
{
	if(str == NULL)
		return NULL;

	uint32_t len = (uint32_t)strlen(str);

	char *result = (char *)local_malloc(sizeof(char) * (len + 1));
	physx::shdfnd::strlcpy(result, len+1, str);
	return result;
}

static void local_strncpy(char *dst, const char *src, uint32_t n)
{
	physx::shdfnd::strlcpy(dst, n, src);
}


static void local_strncat(char *dst, const char *src, uint32_t n)
{
	PX_UNUSED(n);
	physx::shdfnd::strlcat(dst, strlen(dst) + strlen(src) + 1,src);
}

static int32_t safe_strcmp(const char *str1, const char *str2)
{
	if( str1 != NULL && str2 != NULL )
		return strcmp(str1, str2);
	else if( str1 == NULL && str2 == NULL )
		return 0;
	else
		return -1;
}

void *dupBuf(void *buf, uint32_t n)
{
    PX_ASSERT(buf != NULL);
    PX_ASSERT(n > 0);

    void *Ret = local_malloc(n);
    physx::intrinsics::memCopy(Ret, buf, n);

    return(Ret);
}

DataType strToType(const char *str)
{
#   define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
        if(!strcmp(str, #type_name)) \
            return(TYPE_##enum_name);

#   include "nvparameterized/NvParameterized_types.h"

	if(!strcmp(str, "Pointer"))
		return(TYPE_POINTER);

	if(!strcmp(str, "Mat34"))
		return(TYPE_MAT34);

    return(TYPE_UNDEFINED);
}

const char *typeToStr(DataType type)
{
    switch(type)
    {

#   define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
        case TYPE_##enum_name : \
            return(#type_name);
#       include "nvparameterized/NvParameterized_types.h"
		
		case TYPE_MAT34:
			return "Mat34";

		case TYPE_POINTER:
			return "Pointer";

NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
		default:
			return NULL;
    }
}

//******************************************************************************
//*** class HintImpl
//******************************************************************************

HintImpl::HintImpl()
{
    mStaticAllocation = true;
}

HintImpl::HintImpl(const char *name, uint64_t value)
{
    PX_ASSERT(name != NULL);

    init(name, value);
}

HintImpl::HintImpl(const char *name, double value)
{
    PX_ASSERT(name != NULL);

    init(name, value);
}

HintImpl::HintImpl(const char *name, const char *value)
{
    PX_ASSERT(name != NULL);
    PX_ASSERT(value != NULL);

    init(name, value);
}

HintImpl::~HintImpl()
{
    cleanup();
}

void HintImpl::init(const char *name, uint64_t value, bool static_allocation)
{
    PX_ASSERT(name != NULL);

    mStaticAllocation = static_allocation;
    mType = TYPE_U64;
    mName = COND_DUP(name);
    mUIntValue = value;
}

void HintImpl::init(const char *name, double value, bool static_allocation)
{
    PX_ASSERT(name != NULL);

    mStaticAllocation = static_allocation;
    mType = TYPE_F64;
    mName = COND_DUP(name);
    mFloatValue = value;
}

void HintImpl::init(const char *name, const char *value, bool static_allocation)
{
    PX_ASSERT(name != NULL);
    PX_ASSERT(value != NULL);

    mStaticAllocation = static_allocation;
    mType = TYPE_STRING;
    mName = COND_DUP(name);
    mStringValue = COND_DUP(value);
}

void HintImpl::cleanup(void)
{
    PX_ASSERT(mName != NULL);

    if(!mStaticAllocation)
    {
        local_free(mName);

        if(type() == TYPE_STRING)
        {
            PX_ASSERT(mStringValue != NULL);
            local_free(mStringValue);
        }
    }
}

bool HintImpl::setAsUInt(uint64_t v) 
{
	bool ret = false;
  PX_ASSERT(type() == TYPE_U64);
  if ( type() == TYPE_U64 )
  {
	mUIntValue = v;
	ret = true;
  }
  return ret;
}

uint64_t HintImpl::asUInt(void) const
{
	PX_ASSERT(type() == TYPE_U64);
	return(mUIntValue);
}

double HintImpl::asFloat(void) const
{
  PX_ASSERT(type() == TYPE_U64 || type() == TYPE_F64);
  return(type() == TYPE_U64 ? static_cast<int64_t>(mUIntValue) : mFloatValue);
}

const char *HintImpl::asString(void)const
{
  PX_ASSERT(type() == TYPE_STRING);
  return(mStringValue);
}


//******************************************************************************
//*** class Handle
//******************************************************************************



//******************************************************************************
//*** class DefinitionImpl
//******************************************************************************

void DefinitionImpl::setDefaults(void)
{
    mName = NULL;

	mLongName = NULL;
	mLongNameAllocated = false;

	mStructName = NULL;

    mType = TYPE_UNDEFINED;

    mArraySize = 0;

    mParent = NULL;

    mNumChildren = 0;
    mChildren = NULL;

    mNumHints = 0;
    mHints = NULL;

    mEnumVals = NULL;
    mNumEnumVals = 0;

	mNumRefVariants = 0;
	mRefVariantVals = NULL;

	mNumDynamicHandleIndices = 0;
	mDynamicHandleIndices = NULL;

	mAlign = mPad = 0;
}

DefinitionImpl::DefinitionImpl(Traits &traits, bool staticAlloc)
{
    mStaticAllocation = staticAlloc;
	mTraits = &traits;
    setDefaults();
}

DefinitionImpl::DefinitionImpl(const char *name, DataType t, const char *structName, Traits &traits, bool staticAlloc)
{
    mStaticAllocation = staticAlloc;
	mTraits = &traits;
	mLongNameAllocated = false;
    init(name, t, structName, false);
}


DefinitionImpl::~DefinitionImpl()
{
    cleanup();
}

void DefinitionImpl::setAlignment(uint32_t align)
{
	mAlign = align;
}

uint32_t DefinitionImpl::alignment(void) const
{
	return mAlign;
}

void DefinitionImpl::setPadding(uint32_t pad)
{
	mPad = pad;
}

uint32_t DefinitionImpl::padding(void) const
{
	return mPad;
}

void DefinitionImpl::init(const char *name, DataType t, const char *structName, bool static_allocation)
{
	PX_UNUSED( static_allocation );

    cleanup();

	mName = name;
    mLongName = name;
	mStructName = structName;
	
    PX_ASSERT(t != TYPE_UNDEFINED);
    mType = t;
}

void DefinitionImpl::destroy(void)
{
	PX_ASSERT( !mStaticAllocation );

	if( !mStaticAllocation )
	{
		this->~DefinitionImpl();
		mTraits->free(this);
	}
}

void DefinitionImpl::cleanup(void)
{
    if(!mStaticAllocation)
    {
		if( mStructName )
			mTraits->free((void *)mStructName);

		mTraits->free((void *)mName);

        if(mChildren != NULL)
        {
           for(int32_t i=0; i < mNumChildren; ++i)
			   mChildren[i]->destroy();

		   local_free(mChildren);
		}

		if(mHints != NULL)
		{
			for(int32_t i=0; i < mNumHints; ++i)
			{
				mHints[i]->cleanup();
				mTraits->free(mHints[i]);
			}

            local_free(mHints);
		}

		if(mEnumVals != NULL)
		{
			for(int32_t i = 0; i < mNumEnumVals; ++i)
				local_free(mEnumVals[i]);

            local_free(mEnumVals);
		}

		if(mRefVariantVals != NULL)
		{
			for(int32_t i = 0; i < mNumRefVariants; ++i)
				local_free(mRefVariantVals[i]);

            local_free(mRefVariantVals);
        }
    }

	if(mLongNameAllocated && mLongName)
	{
		mTraits->strfree( (char *)mLongName );
		mLongName = NULL;
		mLongNameAllocated = false;
	}
    
    setDefaults();
}

const Definition *DefinitionImpl::root(void) const
{
    const Definition *root = this;
    while(root->parent() != NULL)
        root = root->parent();

    return(root);
}

int32_t DefinitionImpl::arrayDimension(void) const
{
    PX_ASSERT(type() == TYPE_ARRAY);

    int32_t Dim = 0;
    const Definition *Cur = this;
    for(;Cur->type() == TYPE_ARRAY; Cur = Cur->child(0))
    {
      PX_ASSERT(Cur != NULL);
      Dim++;
    }

    return(Dim);
}

int32_t DefinitionImpl::arraySize(int32_t dimension) const
{
    PX_ASSERT(type() == TYPE_ARRAY);
    PX_ASSERT(dimension >= 0);
    PX_ASSERT(dimension < arrayDimension());

    const Definition *Cur = this;
    for(int32_t i=0; i < dimension; ++i)
        Cur = Cur->child(0);

    if(Cur->type() != TYPE_ARRAY)
        return(-1);

	const DefinitionImpl *pd = static_cast<const DefinitionImpl *>(Cur);

    return( pd->mArraySize);
}

bool DefinitionImpl::arraySizeIsFixed(void) const
{
    PX_ASSERT(type() == TYPE_ARRAY);
    return(mArraySize > 0);
}

bool DefinitionImpl::setArraySize(int32_t size)
{
    PX_ASSERT(size >= -1);

    if(size < 0)
        return(false);

    mArraySize = size;

    return(true);
}

bool DefinitionImpl::isIncludedRef(void) const
{
	const Hint *h = hint("INCLUDED");
   	return h && h->type() == TYPE_U64 && h->asUInt();
}

int32_t DefinitionImpl::numChildren(void) const
{
    return(mNumChildren);
}

const Definition * DefinitionImpl::child(int32_t index) const
{
    PX_ASSERT(index >= 0);
    PX_ASSERT(index < numChildren());
    PX_ASSERT(type() == TYPE_STRUCT || type() == TYPE_ARRAY);

    if(index < 0 || index >= numChildren())
        return(NULL);

    return(mChildren[index]);
}

const Definition *  DefinitionImpl::child(const char *name, int32_t &index) const
{
    PX_ASSERT(name);
    PX_ASSERT(type() == TYPE_STRUCT);

    int32_t i;
    for(i=0; i < numChildren(); ++i)
        if(!strcmp(mChildren[i]->name(), name))
        {
            index = i;
            return(mChildren[i]);
        }

    return(NULL);
}


#define PUSH_TO_ARRAY(val, array, obj_type, num_var) \
    num_var++; \
    if(array == NULL) \
        array = (obj_type *)local_malloc(sizeof(obj_type)); \
    else \
        array = (obj_type *)local_realloc(array, num_var * sizeof(obj_type)); \
    PX_ASSERT(array != NULL); \
    array[num_var-1] = val;


static char *GenLongName(char *dst, 
                         uint32_t n,
                         const char *parent_long_name,
                         DataType parent_type, 
                         const char *child_name)
{
    local_strncpy(dst, parent_long_name, n);

    switch(parent_type)
    {
        case TYPE_STRUCT:
            if(parent_long_name[0])
                local_strncat(dst, ".", n);
            local_strncat(dst, child_name, n);
            break;

        case TYPE_ARRAY:
            local_strncat(dst, "[]", n);
            break;

		NV_PARAMETRIZED_NO_AGGREGATE_DATATYPE_LABELS
        default:
            PX_ASSERT((void *)"Shouldn't be here!" == NULL);
            break;
    }

    return(dst);
}

#define SET_ARRAY(type, src_array_var, dst_array_var, num_var) \
    PX_ASSERT(src_array_var != NULL); \
    PX_ASSERT(n > 0); \
    if(mStaticAllocation) \
    { \
        dst_array_var = (type *)src_array_var; \
        num_var = n; \
    } \
    else \
    { \
        PX_ASSERT(dst_array_var == NULL); \
        dst_array_var = (type *)dupBuf(src_array_var, sizeof(type) * n); \
        num_var = n; \
    }

void DefinitionImpl::setChildren(Definition **children, int32_t n)
{
    SET_ARRAY(Definition *, children, mChildren, mNumChildren);

	char tmpStr[MAX_NAME_LEN];

	for(int32_t i=0; i < n; ++i)
    {
        Definition *_child = children[i];
		DefinitionImpl *child = static_cast< DefinitionImpl *>(_child);

        PX_ASSERT(child->parent() == NULL); // Only one parent allowed

        GenLongName(tmpStr, 
                    MAX_NAME_LEN, 
                    mLongName, 
                    type(),
                    child->mName);

		child->mLongName = mTraits->strdup( tmpStr );
		child->mLongNameAllocated = true;

		PX_ASSERT( child != this );
        child->mParent = this;
    }
}

void DefinitionImpl::addChild(Definition *_child)
{
    PX_ASSERT(_child != NULL);
    PX_ASSERT(!mStaticAllocation);

	DefinitionImpl *child = static_cast< DefinitionImpl *>(_child);
	PX_ASSERT(child->mParent == NULL); // Only one parent allowed
	
	char tmpStr[MAX_NAME_LEN];

    GenLongName(tmpStr, 
                MAX_NAME_LEN, 
                mLongName, 
                type(), 
                child->mName);

	child->mLongName = mTraits->strdup( tmpStr );
	child->mLongNameAllocated = true;

	PX_ASSERT( child != this );
    child->mParent = this;
    PUSH_TO_ARRAY(_child, mChildren, Definition *, mNumChildren)
}


int32_t DefinitionImpl::numHints(void) const
{
    return(mNumHints);   
}

const Hint *DefinitionImpl::hint(int32_t index) const
{
    PX_ASSERT(index >= 0);
    PX_ASSERT(index < numHints());

	if( index >= numHints() )
	{
		return(NULL);
	}

    return(mHints[index]);
}

const Hint *DefinitionImpl::hint(const char *name) const
{
    PX_ASSERT(name != NULL);

    for(int32_t i=0; i < numHints(); ++i)
        if(!strcmp(mHints[i]->name(), name))
            return(mHints[i]);

    return(NULL);
}

void DefinitionImpl::setHints(const Hint **hints, int32_t n)
{
    SET_ARRAY(HintImpl *, hints, mHints, mNumHints);
}

void DefinitionImpl::addHint(Hint *_hint)
{
    PX_ASSERT(_hint != NULL);
	HintImpl *hint = static_cast< HintImpl *>(_hint);
    PUSH_TO_ARRAY(hint, mHints, HintImpl *, mNumHints)
}

int32_t DefinitionImpl::numEnumVals(void) const
{
   return(mNumEnumVals);
}

int32_t DefinitionImpl::enumValIndex( const char * enum_val ) const
{
	if(!enum_val)
		return(-1);

	for(int32_t i=0; i < numEnumVals(); ++i)
	{
        if( !strcmp( enumVal(i), enum_val ) )
		{
            return(i);
		}
	}

	return(-1);
}

const char *DefinitionImpl::enumVal(int32_t index) const
{
    PX_ASSERT(index >= 0);
    PX_ASSERT(index < numEnumVals());

    return(mEnumVals[index]);
}

void DefinitionImpl::setEnumVals(const char **enum_vals, int32_t n)
{
    SET_ARRAY(char *, enum_vals, mEnumVals, mNumEnumVals);
}

void DefinitionImpl::addEnumVal(const char *enum_val)
{
    PX_ASSERT(enum_val != NULL);

    char *NewEnumVal = COND_DUP(enum_val);
    PUSH_TO_ARRAY(NewEnumVal, mEnumVals, char *, mNumEnumVals)
}

int32_t DefinitionImpl::refVariantValIndex( const char * ref_val ) const
{
	if(!ref_val)
		return(-1);

	for(int32_t i=0; i < numRefVariants(); ++i)
	{
        if( !strcmp( refVariantVal(i), ref_val ) )
		{
            return(i);
		}
	}

	return(-1);
}

int32_t DefinitionImpl::numRefVariants(void) const
{
	return(mNumRefVariants);
}

const char *DefinitionImpl::refVariantVal(int32_t index) const
{
    PX_ASSERT(index >= 0);
    PX_ASSERT(index < numRefVariants());

    return(mRefVariantVals[index]);
}

void DefinitionImpl::setRefVariantVals(const char **ref_vals, int32_t n)
{
    SET_ARRAY(char *, ref_vals, mRefVariantVals, mNumRefVariants);
}

void DefinitionImpl::addRefVariantVal(const char *ref_val)
{
    PX_ASSERT(ref_val != NULL);

    char *NewEnumVal = COND_DUP(ref_val);
    PUSH_TO_ARRAY(NewEnumVal, mRefVariantVals, char *, mNumRefVariants)
}

void DefinitionImpl::setDynamicHandleIndicesMap(const uint8_t *indices, uint32_t numIndices)
{
	mNumDynamicHandleIndices = numIndices;
	mDynamicHandleIndices = indices;
}

const uint8_t * DefinitionImpl::getDynamicHandleIndicesMap(uint32_t &outNumIndices) const
{
	outNumIndices = mNumDynamicHandleIndices;
	return mDynamicHandleIndices;
}

bool DefinitionImpl::isSimpleType(bool simpleStructs, bool simpleStrings) const
{
	switch(mType)
	{
	case TYPE_STRUCT:
		if( !simpleStructs )
			return false;

		for(int32_t i = 0; i < mNumChildren; ++i)
		{
			if( !mChildren[i]->isSimpleType(simpleStructs, simpleStrings) )
				return false;
		}

		return true;

	case TYPE_ARRAY:
	case TYPE_REF:
		return false;

	NV_PARAMETRIZED_NO_AGGREGATE_AND_REF_DATATYPE_LABELS
	default:
		PX_ASSERT( mNumChildren == 0 );
		return true;
	}
}

//******************************************************************************
//*** class NvParameterized
//******************************************************************************

NvParameters::NvParameters(Traits *traits, void *buf, int32_t *refCount)
{
	mParameterizedTraits = traits;

	if( buf )
	{
		mBuffer = buf;
		mRefCount = refCount;

		//Values of other fields are already deserialized
	}
	else
	{
		mName = mClassName = NULL;
		mDoDeallocateName = mDoDeallocateClassName = mDoDeallocateSelf = true;

		mSerializationCb = NULL;
		mCbUserData = NULL;

		mBuffer = NULL;
		mRefCount = NULL;

		//Other fields are used only for inplace objects => skip them
	}
}

NvParameters::~NvParameters()
{
	if( mClassName && mDoDeallocateClassName )
	{
		mParameterizedTraits->strfree( const_cast<char*>(mClassName) );
		mClassName = NULL;
	}

	if( mName && mDoDeallocateName )
	{
		mParameterizedTraits->strfree( const_cast<char*>(mName) );
		mName = NULL;
	}
}

// placement delete
void NvParameters::destroy()
{
	// We cache these fields here to avoid overwrite in destructor
	bool doDeallocateSelf = mDoDeallocateSelf;
	void *buf = mBuffer;
	int32_t *refCount = mRefCount;
	NvParameterized::Traits *traits = mParameterizedTraits;

	this->~NvParameters();

	destroy(this, traits, doDeallocateSelf, refCount, buf);
}

void NvParameters::destroy(NvParameters *obj, NvParameterized::Traits *traits, bool doDeallocateSelf, int32_t *refCount, void *buf)
{
	if( !doDeallocateSelf )
		return;

	if( !refCount ) //Ordinary object?
		{
		traits->free(obj);
			return;
		}

		//Inplace object => callback client

	traits->onInplaceObjectDestroyed(buf, obj);
	if( !traits->decRefCount(refCount) )
		traits->onAllInplaceObjectsDestroyed(buf);
}


uint16_t NvParameters::getMajorVersion(void) const
{
	uint16_t major = version() >> 16;
	return major;
}

uint16_t NvParameters::getMinorVersion(void) const
{
	uint16_t minor = version() & 0xffff;
	return minor;
}

void NvParameters::initRandom(void)
{
	Handle handle(*this, "");
	initRandom(handle);
}

void NvParameters::initRandom(NvParameterized::Handle& handle)
{
	NvParameterized::ErrorType error;

	const Definition* pd = handle.parameterDefinition();
	switch( pd->type() )
	{
	case TYPE_STRUCT:
		{
			for(int32_t i = 0; i < pd->numChildren(); ++i)
			{
				handle.set(i);
				initRandom(handle);
				handle.popIndex();
			}

			break;
		}

	case TYPE_ARRAY:
		{
			if (!pd->arraySizeIsFixed())
			{
				error = handle.resizeArray(int32_t(10 * (double)rand() / RAND_MAX));
				DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
			}

			int32_t size;
			error = handle.getArraySize(size);
			PX_ASSERT(error == ERROR_NONE);

			DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );

			for(int32_t i = 0; i < size; ++i)
			{
				handle.set(i);
				initRandom(handle);
				handle.popIndex();
			}

			break;
		}

	case TYPE_REF:
		{
			if (!pd->numRefVariants())
				break; // Can't do anything without refVariants-hint

			PX_ASSERT(pd->numRefVariants() > 0);
			uint32_t refIdx = RandomIdx(0U, pd->numRefVariants() - 1U);
			const char* className = pd->refVariantVal(static_cast<int32_t>(refIdx));
			if( mParameterizedTraits->doesFactoryExist(className) ) {
				error = initParamRef(handle, className, true);
				DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );

				NvParameterized::Interface* obj = NULL;
				error = handle.getParamRef(obj);
				DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
			}

			break;
		}

	case TYPE_BOOL:
		error = handle.setParamBool( 0 == RandomU32() % 2 );
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_STRING:
		{
			char s[10];
			int32_t len = (int32_t)( (size_t)rand() % sizeof(s) );
			for(int32_t i = 0; i < len; ++i)
				s[i] = 'a' + rand() % ('z' - 'a');
			s[len] = 0;

			error = handle.setParamString(s);
			DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
			break;
		}

	case TYPE_ENUM:
		{
			uint32_t enumIdx = RandomIdx(0U, static_cast<uint32_t>(pd->numEnumVals()-1));
			error = handle.setParamEnum(pd->enumVal(static_cast<int32_t>(enumIdx)));
			DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
			break;
		}

	case TYPE_I8:
		error = handle.setParamI8((int8_t)(RandomU32() & 0xff));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_U8:
		error = handle.setParamU8((uint8_t)(RandomU32() & 0xff));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_I16:
		error = handle.setParamI16((int16_t)(RandomU32() & 0xffff));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_U16:
		error = handle.setParamU16((uint16_t)(RandomU32() & 0xffff));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_I32:
		error = handle.setParamI32((int32_t)RandomU32());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_U32:
		error = handle.setParamU32((uint32_t)RandomU32());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_I64:
		error = handle.setParamI64((int64_t)RandomU64());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_U64:
		error = handle.setParamU64(RandomU64());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_F32:
		error = handle.setParamF32(RandomF32());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_F64:
		error = handle.setParamF64(RandomF64());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_VEC2:
		error = handle.setParamVec2(RandomVec2());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_VEC3:
		error = handle.setParamVec3(RandomVec3());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_VEC4:
		error = handle.setParamVec4(RandomVec4());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_QUAT:
		error = handle.setParamQuat(RandomQuat());
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_MAT33:
		error = handle.setParamMat33(physx::PxMat33(RandomVec3(), RandomVec3(), RandomVec3()));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_BOUNDS3:
		error = handle.setParamBounds3(physx::PxBounds3(RandomVec3(), RandomVec3()));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_MAT44:
		error = handle.setParamMat44(physx::PxMat44(RandomVec4(), RandomVec4(), RandomVec4(), RandomVec4()));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;

	case TYPE_MAT34:
		{
			float f[12] = { RandomF32(), RandomF32(), RandomF32(), RandomF32(), 
							RandomF32(), RandomF32(), RandomF32(), RandomF32(), 
							RandomF32(), RandomF32(), RandomF32(), RandomF32() };
			error = handle.setParamMat34Legacy(f);
			DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
			break;
		}

	case TYPE_POINTER:
		// We can't init pointers :(
		break;

	case TYPE_TRANSFORM:
	{
		// PxTransform asserts if the quat isn't "sane"
		physx::PxQuat q;
		do { 
			q = RandomQuat();
		} while (!q.isSane());

		error = handle.setParamTransform(physx::PxTransform(RandomVec3(),q));
		DEBUG_ASSERT( NvParameterized::ERROR_NONE == error );
		break;
	}
	NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
	default:
		PX_ALWAYS_ASSERT();
	}

	PX_UNUSED(error); // Make compiler happy
}

void NvParameters::setClassName(const char *name) 
{ 
	if(mParameterizedTraits)
	{
		if( mClassName )
		{
			if( !strcmp( mClassName, name ) )
				return;

			if( mDoDeallocateClassName )
				mParameterizedTraits->strfree( const_cast<char*>(mClassName) );
		}

		mClassName = mParameterizedTraits->strdup(name);
	}
	else
	{
		mClassName = name;
	}

	mDoDeallocateClassName = true;
}

void NvParameters::setName(const char *name) 
{ 
	if(mParameterizedTraits)
	{
		if( mName )
		{
			if( !strcmp( mName, name ) )
				return;

			if( mDoDeallocateName )
				mParameterizedTraits->strfree( const_cast<char*>(mName) );
		}

		mName = mParameterizedTraits->strdup(name);
	}
	else
	{
		mName = name; 
	}

	mDoDeallocateName = true;
}

void NvParameters::setSerializationCallback(SerializationCallback *cb, void *userData)
{
	mSerializationCb = cb;
	mCbUserData = userData;
}

ErrorType NvParameters::callPreSerializeCallback() const
{
	if(mSerializationCb)
	{
		mSerializationCb->preSerialize(mCbUserData);
	}

	Handle handle(*this);

	NV_ERR_CHECK_RETURN( getParameterHandle("", handle) );

	return callPreSerializeCallback(handle);
}

int32_t NvParameters::numParameters(void) 
{
    return(rootParameterDefinition()->numChildren());
}

const Definition *NvParameters::parameterDefinition(int32_t index) 
{
    return(rootParameterDefinition()->child(index));
}

const Definition *NvParameters::rootParameterDefinition(void)
{
    return(getParameterDefinitionTree());
}

const Definition *NvParameters::rootParameterDefinition(void) const
{
    return(getParameterDefinitionTree());
}

ErrorType NvParameters::getParameterHandle(const char *long_name, Handle &handle) const
{
	ErrorType result = ERROR_NONE;

	PX_ASSERT( handle.getConstInterface() == this );
	
	if( rootParameterDefinition() == NULL )
	{
		handle.reset();
		result = (ERROR_INVALID_CALL_ON_NAMED_REFERENCE);
	}
	else
	{
		result = handle.set(this,rootParameterDefinition(), long_name);
	}

	PX_ASSERT(result == ERROR_NONE);

    return result;
}

ErrorType NvParameters::getParameterHandle(const char *long_name, Handle &handle)
{
	ErrorType result = ERROR_NONE;

	PX_ASSERT( handle.getConstInterface() == this );

	if( rootParameterDefinition() == NULL )
	{
		handle.reset();
		result = (ERROR_INVALID_CALL_ON_NAMED_REFERENCE);
	}
	else
	{
		result = handle.set(this,rootParameterDefinition(), long_name);
	}

	PX_ASSERT(result == ERROR_NONE);

	return result;
}

#ifndef NV_CHECKED
#	define CHECK_HANDLE
#	define CHECK_IS_SIMPLE_ARRAY(type_enum_name)
#else
#	define CHECK_HANDLE \
		{ \
	        ErrorType result = checkParameterHandle(handle); \
			PX_ASSERT(result == ERROR_NONE); \
		    if(result != ERROR_NONE) \
			    return(result); \
		}
#	define CHECK_IS_SIMPLE_ARRAY(type_enum_name) \
		{ \
			PX_ASSERT(offset >= 0);\
	        PX_ASSERT(n >= 0);\
		    if(handle.parameterDefinition()->type() != TYPE_ARRAY) \
			{\
				PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_NOT_AN_ARRAY");\
			    return(ERROR_NOT_AN_ARRAY); \
			}\
	        if(handle.parameterDefinition()->child(0)->type() != TYPE_##type_enum_name) \
			{\
  			    PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");\
		        return(ERROR_CAST_FAILED); \
			}\
			int32_t arraySize; \
	        ErrorType error; \
		    if((error = getArraySize(handle, arraySize)) != ERROR_NONE) \
			{\
				PX_ASSERT(error == ERROR_NONE); \
			    return(error); \
			}\
	        if( offset + n > arraySize) \
			{\
				PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_ARRAY_IS_TOO_SMALL");\
				return(ERROR_ARRAY_IS_TOO_SMALL); \
			}\
		}
#endif

template <class Type > ErrorType rawGetParam(const Handle &handle,Type &val,const NvParameters *parent)
{
	size_t offset;
	void *ptr=NULL;
	parent->getVarPtr(handle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}
	Type *var = (Type *)((char *)ptr);
	val = *var;
	return(ERROR_NONE);
}

template <class Type > ErrorType rawSetParam(const Handle &handle,const Type &val,NvParameters *parent)
{
	size_t offset;
	void *ptr=NULL;
	parent->getVarPtr(handle, ptr, offset);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}
	Type *Var = (Type *)((char *)ptr);
	*Var = val;
	return(ERROR_NONE);
}

template <class Type >ErrorType rawGetParamArray(const Handle &handle,Type *array, int32_t n, int32_t offset,const NvParameters *parent) 
{
	int32_t size;
	NV_ERR_CHECK_RETURN(handle.getArraySize(size));
	if( size < offset + n )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	if( n )
	{
		Handle memberHandle(handle);
		NV_ERR_CHECK_RETURN(memberHandle.set(offset));

		size_t tmp;
		void *ptr=NULL;
		parent->getVarPtr(memberHandle, ptr, tmp);
		if(ptr == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
			return(ERROR_INDEX_OUT_OF_RANGE);
		}

		physx::intrinsics::memCopy(array, ptr, n * sizeof(Type));
	}

	return(ERROR_NONE);
}

template <class Type> ErrorType rawSetParamArray(const Handle &handle, const Type *array, int32_t n, int32_t offset,NvParameters *parent)
{
	int32_t size;
	NV_ERR_CHECK_RETURN(handle.getArraySize(size));
	if( size < offset + n )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	if( n )
	{
		Handle memberHandle(handle);
		NV_ERR_CHECK_RETURN(memberHandle.set(offset));

		size_t tmp;
		void *ptr=NULL;
		parent->getVarPtr(memberHandle, ptr, tmp);
		if(ptr == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
			return(ERROR_INDEX_OUT_OF_RANGE);
		}

		physx::intrinsics::memCopy(ptr, array, n * sizeof(Type));
	}

	return(ERROR_NONE);
}

//******************************************************************************
//*** Bool
//******************************************************************************

ErrorType NvParameters::getParamBool(const Handle &handle, bool &val) const
{
    CHECK_HANDLE

    if(handle.parameterDefinition()->type() == TYPE_BOOL)
    {
		return rawGetParamBool(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamBool(const Handle &handle, bool val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_BOOL)
    {
        return rawSetParamBool(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamBoolArray(const Handle &handle, bool *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(BOOL)
    return(rawGetParamBoolArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamBoolArray(const Handle &handle, const bool *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(BOOL)
    return(rawSetParamBoolArray(handle, array, n, offset));
}

//******************************************************************************
//*** String
//******************************************************************************

ErrorType NvParameters::getParamString(const Handle &handle, const char *&val) const
{
    CHECK_HANDLE

    if(handle.parameterDefinition()->type() == TYPE_ENUM)
    {
        return rawGetParamEnum(handle, val);
    }
    
	if(handle.parameterDefinition()->type() == TYPE_STRING)
    {
        return rawGetParamString(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamString(const Handle &handle, const char *val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_STRING)
    {
        return rawSetParamString(handle, val);
    }

	if(handle.parameterDefinition()->type() == TYPE_ENUM)
    {
        return rawSetParamEnum(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::rawGetParamStringArray(const Handle &handle, char **array, int32_t n, int32_t offset) const
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i)
	{
		ErrorType error;
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
		{
			PX_ASSERT(error == ERROR_NONE);
			return(error);
		}

		const char * tmp;
		if((error = rawGetParamString(memberHandle, tmp)) != ERROR_NONE)
		{
			PX_ASSERT(error == ERROR_NONE);
			return(error);
		}

		array[i] = const_cast<char*>(tmp);
		memberHandle.popIndex();
	}
	return(ERROR_NONE);
}


ErrorType NvParameters::getParamStringArray(const Handle &handle, char **array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(STRING)
    return(rawGetParamStringArray(handle, array, n, offset));
}

ErrorType NvParameters::rawSetParamStringArray(const Handle &handle, const char **array, int32_t n, int32_t offset)
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i)
	{
		ErrorType error;
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
		{
			PX_ASSERT(error == ERROR_NONE);
			return(error);
		}

		if((error = rawSetParamString(memberHandle, array[i] )) != ERROR_NONE)
		{
			PX_ASSERT(error == ERROR_NONE);
			return(error);
		}

		memberHandle.popIndex();
	}
	return(ERROR_NONE);
}

ErrorType NvParameters::setParamStringArray(const Handle &handle, const char **array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(STRING)
    return(rawSetParamStringArray(handle, array, n, offset));
}

//******************************************************************************
//*** Enum
//******************************************************************************

ErrorType NvParameters::getParamEnum(const Handle &handle, const char *&val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_STRING)
    {
        return rawGetParamString(handle, val);
    }

	if(handle.parameterDefinition()->type() == TYPE_ENUM)
    {
        return rawGetParamEnum(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");    
    return(ERROR_CAST_FAILED);
}

// returns the actual enum string from the ParameterDefintion object, which should be good for the liftetime
// of the DefinitionImpl object.  Returns NULL if no such enum exists.
static const char* getEnumString(const Handle &handle, const char* str)
{
    PX_ASSERT(str != NULL);

    const Definition* paramDef = handle.parameterDefinition();
    PX_ASSERT(paramDef != NULL);
    PX_ASSERT(paramDef->type() == TYPE_ENUM);

    for (int32_t i = 0; i < paramDef->numEnumVals(); ++i)
        if(!strcmp(paramDef->enumVal(i), str))
            return(paramDef->enumVal(i));

    return(NULL);    
}

ErrorType NvParameters::setParamEnum(const Handle &handle, const char *val)
{
    CHECK_HANDLE

    val = getEnumString(handle, val);
    if(val == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_ENUM_VAL");
        return(ERROR_INVALID_ENUM_VAL);
	}

	if(handle.parameterDefinition()->type() == TYPE_ENUM)
    {
        return rawSetParamEnum(handle, val);
    }

	if(handle.parameterDefinition()->type() == TYPE_STRING)
    {
        return rawSetParamString(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
	return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::rawGetParamEnumArray(const Handle &handle, char **array, int32_t n, int32_t offset) const
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i)
	{
		ErrorType error; 
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
			return(error);
		const char * tmp;
		if((error = rawGetParamEnum(memberHandle, tmp)) != ERROR_NONE)
			return(error);
		array[i] = const_cast<char*>(tmp);
		memberHandle.popIndex();
	}
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamEnumArray(const Handle &handle, const char **array, int32_t n, int32_t offset)
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i)
	{
		ErrorType error;
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
			return(error);
		if((error = rawSetParamEnum(memberHandle, array[i])) != ERROR_NONE)
			return(error);
		memberHandle.popIndex();
	}
	return(ERROR_NONE);
}

ErrorType NvParameters::getParamEnumArray(const Handle &handle, char **array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(ENUM)
    return(rawGetParamEnumArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamEnumArray(const Handle &handle, const char **array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(ENUM)
    return(rawSetParamEnumArray(handle, array, n, offset));
}

//******************************************************************************
//*** Ref
//******************************************************************************

ErrorType NvParameters::initParamRef(const Handle &handle, const char *inChosenRefStr, bool doDestroyOld)
{
	CHECK_HANDLE

	NvParameterized::Interface *param = NULL;
	const char *chosenRefStr = inChosenRefStr;

	// create NvParam object (depends on if it's included or not)
	const Hint *hint = handle.parameterDefinition()->hint("INCLUDED");
	if (hint && hint->type() != TYPE_U64)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_REFERENCE_INCLUDE_HINT");
		return(ERROR_INVALID_REFERENCE_INCLUDE_HINT);
	}

	if (hint != NULL && hint->asUInt() == 1)
	{
		// included

		if (chosenRefStr == 0 && handle.parameterDefinition()->numRefVariants() > 1)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_REFERENCE_VALUE");
			return(ERROR_INVALID_REFERENCE_VALUE);
		}

		for (int32_t i = 0; i < handle.parameterDefinition()->numRefVariants(); i++)
		{
			if (!strcmp(handle.parameterDefinition()->refVariantVal(i), chosenRefStr))
			{
				// create an object of type chosenRefStr, somehow
				param = mParameterizedTraits->createNvParameterized(chosenRefStr);
				if (!param)
				{
					PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_OBJECT_CONSTRUCTION_FAILED");
					return ERROR_OBJECT_CONSTRUCTION_FAILED;
				}

				return setParamRef(handle, param, doDestroyOld);
			}
		}
		// PH: debug hint
		// If you land here, you should compare chosenRefStr and handle.mParameterDefinition.mRefVariantVals
		// to see why it couldn't find anything, and then fix the .pl
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_REFERENCE_VALUE");
		return(ERROR_INVALID_REFERENCE_VALUE);
	}
	else
	{
		// not included, just create generic NvParameterized
		param = NV_PARAM_PLACEMENT_NEW(mParameterizedTraits->alloc(sizeof(NvParameters)), NvParameters)(mParameterizedTraits);
		if (!param)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_OBJECT_CONSTRUCTION_FAILED");
			return ERROR_OBJECT_CONSTRUCTION_FAILED;
		}

		if (chosenRefStr == 0)
		{
			param->setClassName(handle.parameterDefinition()->refVariantVal(0));
		}
		else
		{
			bool found = false;
			for (int32_t i = 0; i < handle.parameterDefinition()->numRefVariants(); i++)
			{
				if (!strcmp(handle.parameterDefinition()->refVariantVal(i), chosenRefStr))
				{
					param->setClassName(handle.parameterDefinition()->refVariantVal(i));
					found = true;
					break;
				}
			}
			if ( !found )
			{
				// ensure that we free this memory that we've allocated
				mParameterizedTraits->free(param);
				PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_REFERENCE_VALUE");
				return(ERROR_INVALID_REFERENCE_VALUE);
			}
		}
		return setParamRef(handle, param, doDestroyOld);
	}
}

ErrorType NvParameters::getParamRef(const Handle &handle, NvParameterized::Interface *&val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_REF)
    {
        return rawGetParamRef(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamRef(const Handle &handle, NvParameterized::Interface *val, bool doDestroyOld)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_REF)
    {
		if (doDestroyOld)
		{
			NvParameterized::Interface *param = NULL;

			getParamRef(handle, param);
			if(param)
			{
				param->destroy();
				param = NULL;
			}
		}
        return rawSetParamRef(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::rawGetParamRefArray(const Handle &handle, NvParameterized::Interface **array, int32_t n, int32_t offset) const
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i)
	{
		ErrorType error; 
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
			return(error);
		NvParameterized::Interface * tmp;
		if((error = rawGetParamRef(memberHandle, tmp)) != ERROR_NONE)
			return(error);
		array[i] = tmp;
		memberHandle.popIndex();
	}
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamRefArray(const Handle &handle,NvParameterized::Interface **array, int32_t n, int32_t offset)
{
	Handle memberHandle(handle);
	for(int32_t i=0; i < n; ++i) 
	{ 
		ErrorType error;
		if((error = memberHandle.set(i + offset)) != ERROR_NONE) 
			return(error); 
		if((error = rawSetParamRef(memberHandle, array[i])) != ERROR_NONE)
			return(error); 
		memberHandle.popIndex(); 
	} 

	return(ERROR_NONE);
}

ErrorType NvParameters::getParamRefArray(const Handle &handle, NvParameterized::Interface * *array, int32_t n, int32_t offset) const
{ 
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(REF)
    return(rawGetParamRefArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamRefArray(const Handle &handle, /*const*/ NvParameterized::Interface **array, int32_t n, int32_t offset, bool doDestroyOld)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(REF)

	Handle memberHandle(handle);
	for(int32_t i = 0; i < n; ++i) 
	{ 
		ErrorType error;
		if((error = memberHandle.set(i + offset)) != ERROR_NONE)
		{
			return(error); 
		}

		if((error = setParamRef(memberHandle, array[i]), doDestroyOld) != ERROR_NONE)
		{
			return(error); 
		}
		memberHandle.popIndex(); 
	}
	return(ERROR_NONE);
}

//******************************************************************************
//*** I8
//******************************************************************************

ErrorType NvParameters::getParamI8(const Handle &handle, int8_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        return rawGetParamI8(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamI8(const Handle &handle, int8_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        return rawSetParamI8(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamI8Array(const Handle &handle, int8_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I8)
    return(rawGetParamI8Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamI8Array(const Handle &handle, const int8_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I8)
    return(rawSetParamI8Array(handle, array, n, offset));
}

//******************************************************************************
//*** I16
//******************************************************************************

ErrorType NvParameters::getParamI16(const Handle &handle, int16_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        int8_t tmp;
        ErrorType result = rawGetParamI8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int16_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        return rawGetParamI16(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamI16(const Handle &handle, int16_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        return rawSetParamI16(handle, val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        return rawSetParamI8(handle, (int8_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t)val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamI16Array(const Handle &handle, int16_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I16)
    return(rawGetParamI16Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamI16Array(const Handle &handle, const int16_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I16)
    return(rawSetParamI16Array(handle, array, n, offset));
}

//******************************************************************************
//*** I32
//******************************************************************************

ErrorType NvParameters::getParamI32(const Handle &handle, int32_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        int8_t tmp;
        ErrorType result = rawGetParamI8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int32_t)tmp;
        return(ERROR_NONE);
    }
	
	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        int16_t tmp;
        ErrorType result = rawGetParamI16(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int32_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_I32)
    {
        int32_t tmp;
        ErrorType result = rawGetParamI32(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int32_t)tmp;
        return(ERROR_NONE);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamI32(const Handle &handle, int32_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I32)
    {
        return rawSetParamI32(handle, val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        return rawSetParamI16(handle, (int16_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        return rawSetParamU16(handle, (uint16_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        return rawSetParamI8(handle, (int8_t) val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t) val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamI32Array(const Handle &handle, int32_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I32)
    return(rawGetParamI32Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamI32Array(const Handle &handle, const int32_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I32)
    return(rawSetParamI32Array(handle, array, n, offset));
}

//******************************************************************************
//*** I64
//******************************************************************************

ErrorType NvParameters::getParamI64(const Handle &handle, int64_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        int8_t tmp;
        ErrorType result = rawGetParamI8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int64_t)tmp;
        return(ERROR_NONE);
    }
	
	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        int16_t tmp;
        ErrorType result = rawGetParamI16(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int64_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_I32)
    {
        int32_t tmp;
        ErrorType result = rawGetParamI32(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int64_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_I64)
    {
        int64_t tmp;
        ErrorType result = rawGetParamI64(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (int64_t)tmp;
        return(ERROR_NONE);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamI64(const Handle &handle, int64_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_I64)
    {
        return rawSetParamI64(handle, val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U32)
    {
        return rawSetParamU32(handle, (uint32_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I32)
    {
        return rawSetParamI32(handle, (int32_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I16)
    {
        return rawSetParamI16(handle, (int16_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        return rawSetParamU16(handle, (uint16_t) val);
    }
	if(handle.parameterDefinition()->type() == TYPE_I8)
    {
        return rawSetParamI8(handle, (int8_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t)val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamI64Array(const Handle &handle, int64_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I64)
    return(rawGetParamI64Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamI64Array(const Handle &handle, const int64_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(I64)
    return(rawSetParamI64Array(handle, array, n, offset));
}

//******************************************************************************
//*** U8
//******************************************************************************

ErrorType NvParameters::getParamU8(const Handle &handle, uint8_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawGetParamU8(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamU8(const Handle &handle, uint8_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamU8Array(const Handle &handle, const uint8_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U8)
    return(rawSetParamU8Array(handle, array, n, offset));
}


//******************************************************************************
//*** U16
//******************************************************************************

ErrorType NvParameters::getParamU16(const Handle &handle, uint16_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        uint8_t tmp;
        ErrorType result = rawGetParamU8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint16_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        uint16_t tmp;
        ErrorType result = rawGetParamU16(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint16_t)tmp;
        return(ERROR_NONE);
    }
	
	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
	return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamU16(const Handle &handle, uint16_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        return rawSetParamU16(handle, val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t)val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamU16Array(const Handle &handle, uint16_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U16)
    return(rawGetParamU16Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamU16Array(const Handle &handle, const uint16_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U16)
    return(rawSetParamU16Array(handle, array, n, offset));
}

//******************************************************************************
//*** U32
//******************************************************************************

ErrorType NvParameters::getParamU32(const Handle &handle, uint32_t &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        uint8_t tmp;
        ErrorType result = rawGetParamU8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint32_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        uint16_t tmp;
        ErrorType result = rawGetParamU16(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint32_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U32)
    {
        uint32_t tmp;
        ErrorType result = rawGetParamU32(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint32_t)tmp;
        return(ERROR_NONE);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamU32(const Handle &handle, uint32_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U32)
    {
        return rawSetParamU32(handle, val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        return rawSetParamU16(handle, (uint16_t)val);
    }
	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t)val);
    }
	
	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamU32Array(const Handle &handle, uint32_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U32)
    return(rawGetParamU32Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamU32Array(const Handle &handle, const uint32_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U32)
    return(rawSetParamU32Array(handle, array, n, offset));
}

//******************************************************************************
//*** U64
//******************************************************************************

ErrorType NvParameters::getParamU64(const Handle &handle, uint64_t &val) const
{
    CHECK_HANDLE

	if (handle.parameterDefinition()->type() == TYPE_BOOL)
	{
		bool tmp;
		ErrorType result = rawGetParamBool(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

		if (result != ERROR_NONE)
			return(result);
		val = (uint64_t)tmp;
		return(ERROR_NONE);
	}

	if (handle.parameterDefinition()->type() == TYPE_U8)
    {
        uint8_t tmp;
        ErrorType result = rawGetParamU8(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint64_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        uint16_t tmp;
        ErrorType result = rawGetParamU16(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint64_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U32)
    {
        uint32_t tmp;
        ErrorType result = rawGetParamU32(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint64_t)tmp;
        return(ERROR_NONE);
    }

	if(handle.parameterDefinition()->type() == TYPE_U64)
    {
        uint64_t tmp;
        ErrorType result = rawGetParamU64(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (uint64_t)tmp;
        return(ERROR_NONE);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamU64(const Handle &handle, uint64_t val)
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_U64)
    {
        return rawSetParamU64(handle, val);
    }

	if(handle.parameterDefinition()->type() == TYPE_U32)
    {
        return rawSetParamU32(handle, (uint32_t)val);
    }

	if(handle.parameterDefinition()->type() == TYPE_U16)
    {
        return rawSetParamU16(handle, (uint16_t)val);
    }

	if(handle.parameterDefinition()->type() == TYPE_U8)
    {
        return rawSetParamU8(handle, (uint8_t)val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamU64Array(const Handle &handle, uint64_t *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U64)
    return(rawGetParamU64Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamU64Array(const Handle &handle, const uint64_t *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(U64)
    return(rawSetParamU64Array(handle, array, n, offset));
}

//******************************************************************************
//*** F32
//******************************************************************************

ErrorType NvParameters::getParamF32(const Handle &handle, float &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_F32)
    {
        return rawGetParamF32(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamF32(const Handle &handle, float val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_F32)
    {
        return rawSetParamF32(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamF32Array(const Handle &handle, float *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(F32)
    return(rawGetParamF32Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamF32Array(const Handle &handle, const float *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(F32)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamF32Array(handle, array, n, offset));
}

//******************************************************************************
//*** F64
//******************************************************************************

ErrorType NvParameters::getParamF64(const Handle &handle, double &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_F32)
    {
        float tmp;
        ErrorType result = rawGetParamF32(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (double)tmp;
        return(ERROR_NONE);
    }
	
	if(handle.parameterDefinition()->type() == TYPE_F64)
    {
        double tmp;
        ErrorType result = rawGetParamF64(handle, tmp);
		PX_ASSERT(result == ERROR_NONE);

        if(result != ERROR_NONE)
            return(result);
        val = (double)tmp;
        return(ERROR_NONE);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamF64(const Handle &handle, double val)
{
    CHECK_HANDLE
    CHECK_F64_FINITE

	if(handle.parameterDefinition()->type() == TYPE_F64)
    {
        return rawSetParamF64(handle, val);
    }
	else if(handle.parameterDefinition()->type() == TYPE_F32)
    {
		return rawSetParamF32(handle, (float)val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::setParamF64Array(const Handle &handle, const double *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(F64)
    CHECK_F64_FINITE_ARRAY
    return(rawSetParamF64Array(handle, array, n, offset));
}

/**
# When shrinking a dynamic array, the array may contain dynamically allocated Refs or Strings.
# It may also contain Structs that contain these items, we'll call them all "dynamic parameters".
#
# To handle this, we generate, for every dynamic array with dynamic parameters, handle indices in 
# the Parameter Definition that help the NvParameters::rawResizeArray() method find these parameters
# and destroy them quickly (without having to recursively traverse the parameters).
#
# The handle indices are layed out like this for the following struct:
# [ handleDepth0, 3, handleDepth1, 5 ] - handleDepth0 = handleDepth1 = 1 
# struct myStruct {
#   float a;
#   float b;
#   float c;
#   string myString;
#   float a;
#   ref myRef;
# }
#
# You can see that myString and myRef are the only two dynamically allocated members that need
# to be destroyed, so only their indices appear in the list.
# 
# Note: Currently, we only support 1D arrays with dynamic parameters in the top most struct.  
# array[i].myString is supported
# array[i].structa.myString is supported
# array[i].structa.structb.myString is NOT supported
*/
ErrorType NvParameters::releaseDownsizedParameters(const Handle &handle, int newSize, int oldSize)
{
	// if downsizing array, release dynamic parameters
	// for now, just do 1D arrays
	if( newSize >= oldSize )
	{
		return ERROR_NONE;
	}

	uint32_t numIndices=0;
	Handle tmpHandle(handle);

	const uint8_t *hIndices = tmpHandle.parameterDefinition()->getDynamicHandleIndicesMap(numIndices);

	if( numIndices )
	{
		// from array[new_size] to array[old_size]
		for( int i=newSize; i<oldSize; i++ )
		{
			tmpHandle.set( i );

			for( uint32_t j=0; j<numIndices; )
			{
				// set the handle up to point to the right dynamic parameter
				uint8_t indexDepth = hIndices[j];
				int k=0;
				for( ; k<indexDepth; k++ )
				{
					tmpHandle.set( hIndices[j+k+1] );
				}
				j += k + 1;
				
				// now we have a handle that's pointing to the dynamic parameter, release it
				DataType type = tmpHandle.parameterDefinition()->type();
				if( type == TYPE_STRING )
				{
					tmpHandle.setParamString( NULL );
				}
				else if( type == TYPE_REF )
				{
					NvParameterized::Interface * paramPtr = 0;
					tmpHandle.getParamRef( paramPtr );
					if( paramPtr )
					{
						paramPtr->destroy();
						tmpHandle.setParamRef( NULL );
					}
				}
				else if( type == TYPE_ENUM )
				{
					// nothing to do on a downsize
				}
				else
				{
					PX_ASSERT( 0 && "Invalid dynamically allocated type defined in Definition handle list" );
				}
				
				// reset the handle
				if( indexDepth > 0 )
				{
					tmpHandle.popIndex( indexDepth );
				}
			}

			tmpHandle.popIndex();
		}
	}

	return ERROR_NONE;
}



/**
# When growing a dynamic array, the array may contain Enums or Structs that contain enums.
#
# To handle this, we generate, for every dynamic array with enums, handle indices in 
# the Parameter Definition that help the NvParameters::rawResizeArray() method find these parameters
# and initialized them quickly (without having to recursively traverse the parameters).
#
# Note: Currently, we only support 1D arrays with enums in the top most struct.  
# array[i].enumA is supported
# array[i].structa.enumA is supported
# array[i].structa.structb.enumA is NOT supported
*/
ErrorType NvParameters::initNewResizedParameters(const Handle &handle, int newSize, int oldSize)
{
	// if downsizing array, release dynamic parameters
	// for now, just do 1D arrays
	if( newSize <= oldSize )
	{
		return ERROR_NONE;
	}

	uint32_t numIndices=0;
	Handle tmpHandle(handle);

	const uint8_t *hIndices = tmpHandle.parameterDefinition()->getDynamicHandleIndicesMap(numIndices);

	if( numIndices )
	{
		// from array[new_size] to array[old_size]
		for( int i=oldSize; i<newSize; i++ )
		{
			tmpHandle.set( i );

			for( uint32_t j=0; j<numIndices; )
			{
				// set the handle up to point to the right dynamic parameter
				uint8_t indexDepth = hIndices[j];
				int k=0;
				for( ; k<indexDepth; k++ )
				{
					tmpHandle.set( hIndices[j+k+1] );
				}
				j += k + 1;
				
				// now we have a handle that's pointing to the dynamic parameter, release it
				DataType type = tmpHandle.parameterDefinition()->type();
				if( type == TYPE_STRING || type == TYPE_REF )
				{
					// nothing to do on an array growth
				}
				else if( type == TYPE_ENUM )
				{
					// this is not the default value, but that's not available from here
					// we could possibly store the enum default values, or just always make them first
					tmpHandle.setParamEnum( tmpHandle.parameterDefinition()->enumVal(0) );
				}
				else if( type == TYPE_ARRAY )
				{
					// FIXME: we do not fully support arrays here, this is just a brain-dead stub

					const Definition *pd = 0;
					for(pd = tmpHandle.parameterDefinition(); pd->type() == TYPE_ARRAY; pd = pd->child(0))
					{
						PX_ASSERT( pd->numChildren() == 1 );
					}

					if( pd->type() != TYPE_STRING && pd->type() != TYPE_REF )
						PX_ASSERT( 0 && "Invalid dynamically allocated type defined in Definition handle list" );
				}
				else
				{
					PX_ASSERT( 0 && "Invalid dynamically allocated type defined in Definition handle list" );
				}
				
				// reset the handle
				if( indexDepth > 0 )
				{
					tmpHandle.popIndex( indexDepth );
				}
			}

			tmpHandle.popIndex();
		}
	}

	return ERROR_NONE;
}

ErrorType NvParameters::rawResizeArray(const Handle &handle, int new_size)
{
	size_t offset;
	ErrorType ret;
	void *ptr=NULL;

	Handle arrayRootHandle(handle);

	// We should consider storing alignment in dynamic array struct at some point
	uint32_t align = arrayRootHandle.parameterDefinition()->alignment();
	if( 0 == align )
		align = 8; // Default alignment

	int dimension = 0;

	while(arrayRootHandle.parameterDefinition()->parent() &&
		arrayRootHandle.parameterDefinition()->parent()->type() == TYPE_ARRAY)
	{
		arrayRootHandle.popIndex();
		dimension++;
	}

	getVarPtr(arrayRootHandle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}

	DummyDynamicArrayStruct *dynArray = (DummyDynamicArrayStruct *)ptr;
	int old_size = dynArray->arraySizes[0];

	releaseDownsizedParameters(handle, new_size, old_size);

	ret = resizeArray(mParameterizedTraits,
					  dynArray->buf, 
					  dynArray->arraySizes, 
					  arrayRootHandle.parameterDefinition()->arrayDimension(), 
					  dimension, 
					  new_size,
					  dynArray->isAllocated,
					  dynArray->elementSize,
					  align,
					  dynArray->isAllocated); 

	initNewResizedParameters(handle, new_size, old_size);

	return ret;
}

ErrorType NvParameters::rawGetArraySize(const Handle &array_handle, int &size, int dimension) const
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(array_handle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}

	DummyDynamicArrayStruct *dynArray = (DummyDynamicArrayStruct *)ptr;
	size = dynArray->arraySizes[dimension];
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSwapArrayElements(const Handle &array_handle, unsigned int firstElement, unsigned int secondElement)
{
	size_t offset = 0;
	void* ptr = NULL;
	Handle arrayRootHandle(array_handle);

	getVarPtr(arrayRootHandle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}

	DummyDynamicArrayStruct* dynArray = reinterpret_cast<DummyDynamicArrayStruct*>(ptr);

	const int elementSize = dynArray->elementSize;

	char tempDataStack[64];
	void* tempData = tempDataStack;

	void* tempDataHeap = NULL;
	PX_ASSERT(elementSize > 0);
	PX_ASSERT(sizeof(elementSize) <= sizeof(uint32_t));
	if (elementSize > 64)
	{
		tempDataHeap = mParameterizedTraits->alloc(static_cast<uint32_t>(elementSize));
		tempData = tempDataHeap;
	}

	char* firstPtr = (char*)dynArray->buf + elementSize * firstElement;
	char* secondPtr = (char*)dynArray->buf + elementSize * secondElement;

	physx::intrinsics::memCopy(tempData, firstPtr, static_cast<uint32_t>(elementSize));
	physx::intrinsics::memCopy(firstPtr, secondPtr, static_cast<uint32_t>(elementSize));
	physx::intrinsics::memCopy(secondPtr, tempData, static_cast<uint32_t>(elementSize));

	if (tempDataHeap != NULL)
	{
		mParameterizedTraits->free(tempDataHeap);
	}

	return(ERROR_NONE);
}

#define ARRAY_HANDLE_CHECKS \
    if(!array_handle.isValid()) \
	{ \
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_PARAMETER_HANDLE");\
        return(ERROR_INVALID_PARAMETER_HANDLE); \
	}\
    if(array_handle.parameterDefinition()->type() != TYPE_ARRAY) \
	{\
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_NOT_AN_ARRAY"); \
        return(ERROR_NOT_AN_ARRAY); \
	}


ErrorType NvParameters::resizeArray(const Handle &array_handle, int32_t new_size)
{
    ARRAY_HANDLE_CHECKS

    if(new_size < 0)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_ARRAY_SIZE");
        return(ERROR_INVALID_ARRAY_SIZE);
	}

    if(array_handle.parameterDefinition()->arraySizeIsFixed())
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_ARRAY_SIZE_IS_FIXED");
        return(ERROR_ARRAY_SIZE_IS_FIXED);
	}

    return(rawResizeArray(array_handle, new_size));
}

ErrorType NvParameters::swapArrayElements(const Handle &array_handle, uint32_t firstElement, uint32_t secondElement)
{
	ARRAY_HANDLE_CHECKS;

	if (firstElement == secondElement)
		return(ERROR_NONE);

	int array_size = 0;
	rawGetArraySize(array_handle, array_size, 0);

	if (array_size < 1)
		return(ERROR_NONE);

	if (firstElement >= (unsigned int)array_size)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	if (secondElement >= (unsigned int)array_size)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	// well, maybe swapping will work on fixed size arrays as well...
	if (array_handle.parameterDefinition()->arraySizeIsFixed())
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_ARRAY_SIZE_IS_FIXED");
		return(ERROR_ARRAY_SIZE_IS_FIXED);
	}

	return rawSwapArrayElements(array_handle, firstElement, secondElement);
}

ErrorType NvParameters::
    getArraySize(const Handle &array_handle, int32_t &size, int32_t dimension) const
{
    ARRAY_HANDLE_CHECKS

    if(dimension < 0 || dimension >= array_handle.parameterDefinition()->arrayDimension())
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_ARRAY_DIMENSION");
        return(ERROR_INVALID_ARRAY_DIMENSION);
	}

    if(array_handle.parameterDefinition()->arraySizeIsFixed())
    {
        size = array_handle.parameterDefinition()->arraySize(dimension);
        return(ERROR_NONE);
    }

    return(rawGetArraySize(array_handle, size, dimension));
}

ErrorType NvParameters::callPreSerializeCallback(Handle& handle) const
{
	const Definition* def = handle.parameterDefinition();
	if (def->type() == TYPE_REF)
	{
		// don't preSerialize Named References
		const Hint *hint = def->hint("INCLUDED");
		if( hint && 
			hint->type() == TYPE_U64 && 
			hint->asUInt() == 1 )
		{
			// included
			NvParameterized::Interface* ref = NULL;
			NV_ERR_CHECK_RETURN( getParamRef(handle, ref) );
			if( ref )
			{
				NV_ERR_CHECK_RETURN( ref->callPreSerializeCallback() );
			}
		}
	}
	else if (def->type() == TYPE_ARRAY)
	{
		int32_t arraySize = 0;
		getArraySize(handle, arraySize);

		for (int32_t i = 0; i < arraySize; i++)
		{
			handle.set(i);

			NV_ERR_CHECK_RETURN( callPreSerializeCallback(handle) );

			handle.popIndex();
		}
	}
	else if (def->type() == TYPE_STRUCT)
	{
		const int32_t numChildren = def->numChildren();

		for (int32_t childIndex = 0; childIndex < numChildren; childIndex++)
		{
			handle.set(childIndex);

			NV_ERR_CHECK_RETURN( callPreSerializeCallback(handle) );

			handle.popIndex();
		}
	}

	return ERROR_NONE;
}

ErrorType NvParameters::checkParameterHandle(const Handle &handle) const
{
	PX_ASSERT( handle.getConstInterface() );
	PX_ASSERT( handle.getConstInterface() == this );
	
	if (!handle.isValid()) 
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_PARAMETER_HANDLE");
		return(ERROR_INVALID_PARAMETER_HANDLE);
	}

	if (handle.getConstInterface() == NULL ) 
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_HANDLE_MISSING_INTERFACE_POINTER");
		return(ERROR_HANDLE_MISSING_INTERFACE_POINTER);
	}

	if (handle.getConstInterface() != this ) 
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_HANDLE_INVALID_INTERFACE_POINTER");
		return(ERROR_HANDLE_INVALID_INTERFACE_POINTER); 
	}

    if(handle.parameterDefinition()->root() != rootParameterDefinition()) 
	{
        return(ERROR_PARAMETER_HANDLE_DOES_NOT_MATCH_CLASS);
	}


    const Definition *ptr = rootParameterDefinition();
    for(int32_t i=0; i < handle.numIndexes(); ++i)
    {
        PX_ASSERT(ptr != NULL);

        switch(ptr->type())
        {
            case TYPE_STRUCT:
                if(handle.index(i) >= ptr->numChildren())
				{
					PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
                    return(ERROR_INDEX_OUT_OF_RANGE);
				}
                ptr = ptr->child(handle.index(i));
                break;

            case TYPE_ARRAY:
                {
                    int32_t size = ptr->arraySize();
                    Handle tmpHandle(handle);
					tmpHandle.popIndex(handle.numIndexes() - i);

                    if(size <= 0)
                        if(getArraySize(tmpHandle, size) != ERROR_NONE)
						{
							PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
                            return(ERROR_INDEX_OUT_OF_RANGE);
						}

                    if(handle.index(i) >= size)
					{
						PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
						return(ERROR_INDEX_OUT_OF_RANGE);
					}

                    ptr = ptr->child(0);
                }
                break;

			NV_PARAMETRIZED_NO_AGGREGATE_DATATYPE_LABELS
            default:
                PX_ALWAYS_ASSERT();
                ptr = NULL;
        }
    }

    return(ERROR_NONE);
}

ErrorType NvParameters::clone(NvParameterized::Interface *&nullDestObject) const
{
	PX_ASSERT(nullDestObject == NULL );
	nullDestObject = mParameterizedTraits->createNvParameterized(className(), version());
	if( !nullDestObject )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_OBJECT_CONSTRUCTION_FAILED");
		return ERROR_OBJECT_CONSTRUCTION_FAILED;
	}

	ErrorType err = nullDestObject->copy(*this);
	if( ERROR_NONE != err )
	{
		nullDestObject->destroy();
		nullDestObject = NULL;
	}

	return err;
}

ErrorType NvParameters::copy(const NvParameterized::Interface &other,
                             Handle &thisHandle)
{
   const Definition *paramDef = thisHandle.parameterDefinition();
   ErrorType error = ERROR_NONE;

    if( paramDef->type() == TYPE_STRUCT )
	{
        for(int32_t i=0; i < paramDef->numChildren(); ++i)
        {
            thisHandle.set(i);
			error = copy(other, thisHandle);
            NV_ERR_CHECK_RETURN(error);

            thisHandle.popIndex();
        }
        return(error);
	}
	else if( paramDef->type() == TYPE_ARRAY )
    {
        int32_t thisSize, otherSize;

		error = thisHandle.getArraySize(thisSize);
        NV_ERR_CHECK_RETURN(error);

		Handle otherHandle = thisHandle;
		otherHandle.setInterface(&other);
        error = otherHandle.getArraySize(otherSize);
        NV_ERR_CHECK_RETURN(error);

		thisHandle.setInterface(this);

		if(thisSize != otherSize)
		{
			error = thisHandle.resizeArray(otherSize);
			NV_ERR_CHECK_RETURN(error);
			thisSize = otherSize;
		}

        for(int32_t i=0; i < otherSize; ++i)
        {
            thisHandle.set(i);

			error = this->copy(other, thisHandle);
			NV_ERR_CHECK_RETURN(error);

            thisHandle.popIndex();
        }
        return(error);
    }

	Handle otherHandle = thisHandle; 
	otherHandle.setInterface(&other);
    switch(paramDef->type()) 
    {
		case TYPE_BOOL: 
			{ 
				bool otherVal; 
				error = otherHandle.getParamBool(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamBool(otherVal) );
			}

		case TYPE_I8: 
			{ 
				int8_t otherVal; 
				error = otherHandle.getParamI8(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamI8(otherVal) );
			}

		case TYPE_I16 : 
			{ 
				int16_t otherVal; 
				error = otherHandle.getParamI16(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamI16(otherVal) );
			}
		case TYPE_I32 : 
			{
				int32_t otherVal;
				error = otherHandle.getParamI32(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamI32(otherVal) );
			}
		case TYPE_I64 : 
			{
				int64_t otherVal;
				error = otherHandle.getParamI64(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamI64(otherVal) );
			}

		case TYPE_U8 : 
			{
				uint8_t otherVal;
				error = otherHandle.getParamU8(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamU8(otherVal) );
			}
		case TYPE_U16 : 
			{
				uint16_t otherVal;
				error = otherHandle.getParamU16(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamU16(otherVal) );
			}
		case TYPE_U32 : 
			{
				uint32_t otherVal;
				error = otherHandle.getParamU32(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamU32(otherVal) );
			}
		case TYPE_U64 : 
			{
				uint64_t otherVal;
				error = otherHandle.getParamU64(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamU64(otherVal) );
			}

		case TYPE_F32 : 
			{
				float otherVal;
				error = otherHandle.getParamF32(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamF32(otherVal) );
			}
		case TYPE_F64 : 
			{
				double otherVal;
				error = otherHandle.getParamF64(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamF64(otherVal) );
			}

		case TYPE_VEC2 : 
			{
				physx::PxVec2 otherVal;
				error = otherHandle.getParamVec2(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamVec2(otherVal) );
			}
		case TYPE_VEC3 : 
			{
				physx::PxVec3 otherVal;
				error = otherHandle.getParamVec3(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamVec3(otherVal) );
			}
		case TYPE_VEC4 : 
			{
				physx::PxVec4 otherVal;
				error = otherHandle.getParamVec4(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamVec4(otherVal) );
			}
		case TYPE_QUAT : 
			{
				physx::PxQuat otherVal;
				error = otherHandle.getParamQuat(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamQuat(otherVal) );
			}
		case TYPE_MAT33 : 
			{
				physx::PxMat33 otherVal;
				error = otherHandle.getParamMat33(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamMat33(otherVal) );
			}
		case TYPE_MAT44 : 
			{ 
				physx::PxMat44 otherVal;
				error = otherHandle.getParamMat44(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamMat44(otherVal) );
			}
		case TYPE_MAT34 : 
			{ 
				float otherVal[12];
				error = otherHandle.getParamMat34Legacy(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamMat34Legacy(otherVal) );
			}
		case TYPE_BOUNDS3 : 
			{ 
				physx::PxBounds3 otherVal;
				error = otherHandle.getParamBounds3(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamBounds3(otherVal) );
			}
		case TYPE_TRANSFORM : 
			{ 
				physx::PxTransform otherVal;
				error = otherHandle.getParamTransform(otherVal);
				NV_ERR_CHECK_RETURN( error );
				return( thisHandle.setParamTransform(otherVal) );
			}
        case TYPE_STRING:
            {
                const char *val1;
				error = otherHandle.getParamString(val1);
                NV_ERR_CHECK_RETURN(error);
				return(thisHandle.setParamString(val1));
            }

        case TYPE_ENUM:
            {
                const char *val1;
                error = otherHandle.getParamEnum(val1);
                NV_ERR_CHECK_RETURN(error);
				return val1 ? thisHandle.setParamEnum(val1) : ERROR_NONE;
            }

		case TYPE_REF:
			{
				NvParameterized::Interface *thisRef, *otherRef;
				error = thisHandle.getParamRef(thisRef);
				NV_ERR_CHECK_RETURN(error);
				
				error = otherHandle.getParamRef(otherRef);
				NV_ERR_CHECK_RETURN(error);

				if(thisRef)
				{
					thisRef->destroy();
					thisHandle.setParamRef(NULL);
				}

				if(otherRef)
				{
					error = thisHandle.initParamRef(otherRef->className(), true);
					NV_ERR_CHECK_RETURN(error);

					error = thisHandle.getParamRef(thisRef);
					NV_ERR_CHECK_RETURN(error);

					if(thisRef == NULL)
					{
						PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_PARAMETER_HANDLE_NOT_INITIALIZED");
						return(ERROR_PARAMETER_HANDLE_NOT_INITIALIZED);
					}

					return(thisRef->copy(*otherRef));
				}
			}
			break;

		case TYPE_POINTER:
			//Just don't do anything with pointers
			break;

		NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
		NV_PARAMETRIZED_AGGREGATE_DATATYPE_LABELS
		default:
            PX_ALWAYS_ASSERT();
            break;
    }

	return ERROR_NONE;
}

bool NvParameters::areParamsOK(Handle &handle, Handle *invalidHandles, uint32_t numInvalidHandles, uint32_t &numRemainingHandles)
{
	class Constraints
	{
		const Hint *min, *max, *multOf, *powOf;

		static int64_t abs(int64_t x)
		{
			return x >= 0 ? x : -x;
		}

	public:
		Constraints(const Hint *min_, const Hint *max_, const Hint *multOf_, const Hint *powOf_):
			min(min_), max(max_), multOf(multOf_), powOf(powOf_) {}

		bool isOK(uint64_t val)
		{
			bool res = true;

			if( min )
				res = res && val >= min->asUInt();

			if( max )
				res = res && val <= max->asUInt();

			if( multOf )
				res = res && 0 == val % multOf->asUInt();

			if( powOf  )
			{
				//TODO: this is too slow
				uint64_t base = powOf->asUInt(), acc = 1;
				while( acc < val )
					acc *= base;
				res = res && acc == base;
			}

			return res;
		}

		bool isOK(int64_t val)
		{
			bool res = true;

			if( min )
				res = res && val >= int64_t(min->asUInt());

			if( max )
				res = res && val <= int64_t(max->asUInt());

			if( multOf )
				res = res && 0 == val % int64_t(multOf->asUInt());

			if( powOf  )
			{
				//TODO: this is too slow
				int64_t base = int64_t(powOf->asUInt()), acc = 1;
				while( abs(acc) < val )
					acc *= base;
				res = res && abs(acc) == base;
			}

			return res;
		}

		bool isOK(float val)
		{
			bool res = true;

			if( min )
				res = res && val >= float(min->asFloat());

			if( max )
				res = res && val <= float(max->asFloat());

			if( multOf )
				res = res && 0.0f == (float)fmod(val, float(multOf->asFloat()));

			//TODO
			if( powOf)
			{
				PX_ALWAYS_ASSERT();
				return false;
			}

			return res;
		}

		bool isOK(double val)
		{
			bool res = true;

			if( min )
				res = res && val >= min->asFloat();

			if( max )
				res = res && val <= max->asFloat();

			if( multOf )
				res = res && 0.0f == (float)fmod((float)val, (float)multOf->asFloat());

			//TODO
			if( powOf )
			{
				PX_ALWAYS_ASSERT();
				return false;
			}

			return res;
		}
	};
	
	bool res = true;

    const Definition *pd = handle.parameterDefinition();

	Constraints con(pd->hint("min"), pd->hint("max"), pd->hint("multipleOf"), pd->hint("powerOf"));

	switch( pd->type() ) 
    {
        case TYPE_STRUCT:
            {
                for(int32_t i = 0; i < pd->numChildren(); ++i)
                {
                    handle.set(i);
                    res &= areParamsOK(handle, invalidHandles, numInvalidHandles, numRemainingHandles);
                    handle.popIndex();
                }
	            return res;
            }

        case TYPE_ARRAY:
            {
                int32_t arraySize = -1;
                NV_BOOL_RETURN( handle.getArraySize(arraySize) );

                for(int32_t i=0; i < arraySize; ++i)
                {
                    handle.set(i);
                    res &= areParamsOK(handle, invalidHandles, numInvalidHandles, numRemainingHandles);
                    handle.popIndex();
                }
	            return res;
            }

		case TYPE_BOOL:
		case TYPE_U8:
		case TYPE_U16:
		case TYPE_U32:
		case TYPE_U64:
			{
				uint64_t val;
				NV_BOOL_RETURN( handle.getParamU64(val) );
				res = con.isOK(val);
				if( !res && numRemainingHandles > 0 )
					invalidHandles[numRemainingHandles++ - 1] = handle;
				return res;
			}

		case TYPE_I8:
		case TYPE_I16:
		case TYPE_I32:
		case TYPE_I64:
			{
				int64_t val;
				NV_BOOL_RETURN( handle.getParamI64(val) );
				res = con.isOK(val);
				if( !res && numRemainingHandles > 0 )
					invalidHandles[numRemainingHandles++ - 1] = handle;
				return res;
			}

		case TYPE_F32:
			{
				float val = -1;
				NV_BOOL_RETURN( handle.getParamF32(val) );
				res = con.isOK(val);
				if( !res && numRemainingHandles > 0 )
					invalidHandles[numRemainingHandles++ - 1] = handle;
				return res;
			}

		case TYPE_F64:
			{
				double val = -1;
				NV_BOOL_RETURN( handle.getParamF64(val) );
				res = con.isOK(val);
				if( !res && numRemainingHandles > 0 )
					invalidHandles[numRemainingHandles++ - 1] = handle;
				return res;
			}

		NV_PARAMETRIZED_NO_AGGREGATE_AND_ARITHMETIC_DATATYPE_LABELS
		default:
			return true;
	}
}

bool NvParameters::equals(const NvParameterized::Interface &obj,
						  Handle &param_handle,
						  Handle *handlesOfInequality,
						  uint32_t numHandlesOfInequality,
						  bool doCompareNotSerialized) const
{
    const Definition *paramDef = param_handle.parameterDefinition();

	if (!doCompareNotSerialized && paramDef->hint("DONOTSERIALIZE") )
		return true;

	DataType type = paramDef->type();
    switch(type) 
    {
#define NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
	case TYPE_ ## enum_name: \
	{ \
		c_type a; \
		c_type b; \
		param_handle.setInterface(this); \
		NV_BOOL_RETURN( param_handle.getParam ## type_name(a) ); \
		param_handle.setInterface(&obj); \
		NV_BOOL_RETURN( param_handle.getParam ## type_name(b) ); \
		return !notEqual(a, b); \
	}
#include "nvparameterized/NvParameterized_types.h"

		case TYPE_MAT34:
		{
			float a[12];
			float b[12];
			param_handle.setInterface(this);
			NV_BOOL_RETURN( param_handle.getParamMat34Legacy(a) );
			param_handle.setInterface(&obj);
			NV_BOOL_RETURN( param_handle.getParamMat34Legacy(b) );
			return !notEqual(a, b);
		}

		case TYPE_REF:
			{
				NvParameterized::Interface *val1, *val2;

				param_handle.setInterface( this );
				if(param_handle.getParamRef(val1) != ERROR_NONE)
					return(false);

				param_handle.setInterface( &obj );
				if(param_handle.getParamRef(val2) != ERROR_NONE)
					return(false);

				if(val1 == NULL && val2 == NULL)
					return(true);
				else if(val1 == NULL || val2 == NULL)
					return(false);

				return val2->equals(
					*val1,
					handlesOfInequality != NULL ? handlesOfInequality+1 : NULL,
					numHandlesOfInequality > 0 ? numHandlesOfInequality-1 : 0,
					doCompareNotSerialized );
			}

		case TYPE_STRUCT:
	        for(int32_t i = 0; i < paramDef->numChildren(); ++i)
		    {
			    param_handle.set(i);
				if (!equals(obj, param_handle, handlesOfInequality, numHandlesOfInequality, doCompareNotSerialized))
					return(false);
	            param_handle.popIndex();
		    }
	        return(true);

		case TYPE_ARRAY:
	    {
		    int32_t arraySize1, arraySize2;

			param_handle.setInterface (this);
		    if (param_handle.getArraySize(arraySize1) != ERROR_NONE)
			    return(false);

			param_handle.setInterface (&obj);
		    if (param_handle.getArraySize(arraySize2) != ERROR_NONE)
			    return(false);

	        if(arraySize1 != arraySize2)
		        return(false);

			if( arraySize1 > 100 && paramDef->isSimpleType(false, false) )
			{
				// Large array of simple types, fast path

				switch( type)
				{
				NV_PARAMETRIZED_NO_ARITHMETIC_AND_LINAL_DATATYPE_LABELS
				NV_PARAMETRIZED_LEGACY_DATATYPE_LABELS
				default:
					// Fall to slow path, including TYPE_MAT34 case
					break;

#define NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#define NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
	case TYPE_ ## enum_name: { \
		uint32_t byteSize = static_cast<uint32_t>(sizeof(c_type)) * arraySize1; \
		c_type *data1 = (c_type *)mParameterizedTraits->alloc(byteSize), \
			*data2 = (c_type *)mParameterizedTraits->alloc(byteSize); \
		\
		param_handle.setInterface(this); \
		NV_BOOL_RETURN(param_handle.getParam ## type_name ## Array(data1, arraySize1)); \
		\
		param_handle.setInterface(&obj); \
		NV_BOOL_RETURN(param_handle.getParam ## type_name ## Array(data2, arraySize2)); \
		\
		int ret = memcmp(data1, data2, byteSize); \
		mParameterizedTraits->free(data1); \
		mParameterizedTraits->free(data2); \
		\
		return ret == 0; \
	}
#include "nvparameterized/NvParameterized_types.h"
				}
			}

			// Array of aggregates, slow path

			param_handle.setInterface(this);

			for (int32_t i = 0; i < arraySize1; ++i)
		    {
			    param_handle.set(i);
				if (!equals(obj, param_handle, handlesOfInequality, numHandlesOfInequality, doCompareNotSerialized))
					return(false);
				param_handle.popIndex();
			}

			return(true);
		}

		case TYPE_POINTER:
			return true;

NV_PARAMETRIZED_UNDEFINED_AND_LAST_DATATYPE_LABELS
        default:
			PX_ALWAYS_ASSERT();
            break;
    }

    return(false);
}
                   
ErrorType NvParameters::copy(const NvParameterized::Interface &other)
{
	if (this == &other)
	{
		return(ERROR_NONE);
	}

    if (rootParameterDefinition() != other.rootParameterDefinition())
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_PARAMETER_DEFINITIONS_DO_NOT_MATCH");
		return(ERROR_PARAMETER_DEFINITIONS_DO_NOT_MATCH);
	}

	// support empty, named references
	if (rootParameterDefinition() == NULL)
	{
		// the name or className could be NULL, strcmp doesn't like NULL strings...
		setClassName(other.className());
		setName(other.name());
		return(ERROR_NONE);
	}
	else
	{
		Handle handle (*this);
		if (getParameterHandle("", handle) != ERROR_NONE)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_PARAMETER_HANDLE");
			return(ERROR_INVALID_PARAMETER_HANDLE);
		}

		return(copy(other, handle));
	}
}

bool NvParameters::areParamsOK(Handle *invalidHandles, uint32_t numInvalidHandles)
{
    Handle handle(*this);
	NV_BOOL_RETURN( getParameterHandle("", handle) );

	uint32_t numRemainingHandles = numInvalidHandles;
	return areParamsOK(handle, invalidHandles, numInvalidHandles, numRemainingHandles);
}

bool NvParameters::equals(const NvParameterized::Interface &obj, Handle* handleOfInequality, uint32_t numHandlesOfInequality, bool doCompareNotSerialized) const
{
	if( this == &obj )
		return(true);

    if(rootParameterDefinition() != obj.rootParameterDefinition())
        return(false);

	// support empty, named references
	if(rootParameterDefinition() == NULL)
	{
		// the name or className could be NULL, strcmp doesn't like NULL strings...
		return 0 == safe_strcmp(name(), obj.name()) && 0 == safe_strcmp(className(), obj.className());
	}

	// This should be a handle that can not set any values!
    Handle constHandle(*this);
	NV_BOOL_RETURN( getParameterHandle("", constHandle) );

	bool theSame = equals(obj, constHandle, handleOfInequality, numHandlesOfInequality, doCompareNotSerialized);

	if (!theSame && numHandlesOfInequality > 0)
		*handleOfInequality = constHandle;

	return theSame;
}

ErrorType NvParameters::valueToStr(const Handle &handle,
                                   char *buf,
                                   uint32_t n,
                                   const char *&ret)
{
    PX_ASSERT(buf != NULL);
    PX_ASSERT(n > 0);

    if(!handle.isValid())
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_PARAMETER_HANDLE");
        return(ERROR_INVALID_PARAMETER_HANDLE);
	}

    const Definition *paramDef = handle.parameterDefinition();

    ErrorType error = ERROR_TYPE_NOT_SUPPORTED;

    switch(paramDef->type())
    {
    	case TYPE_VEC2:
    		{
        		physx::PxVec2 val;
                if ((error = getParamVec2(handle, val)) == ERROR_NONE)
                {
					char f[2][physx::PxAsc::PxF32StrLen];
					physx::shdfnd::snprintf(buf, n,"%s %s",
						 physx::PxAsc::valueToStr(val.x, f[0], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.y, f[1], physx::PxAsc::PxF32StrLen));
    				ret = buf;
    			}
			}
    		break;
    	case TYPE_VEC3:
    		{
        		physx::PxVec3 val;
                if ((error = getParamVec3(handle, val)) == ERROR_NONE)
                {
					char f[3][physx::PxAsc::PxF32StrLen];
					physx::shdfnd::snprintf(buf, n,"%s %s %s",
						 physx::PxAsc::valueToStr(val.x, f[0], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.y, f[1], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.z, f[2], physx::PxAsc::PxF32StrLen));
    				ret = buf;
    			}
			}
    		break;
    	case TYPE_VEC4:
    		{
        		physx::PxVec4 val;
                if ((error = getParamVec4(handle, val)) == ERROR_NONE)
                {
					char f[4][physx::PxAsc::PxF32StrLen];
					physx::shdfnd::snprintf(buf, n,"%s %s %s %s",
						 physx::PxAsc::valueToStr(val.x, f[0], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.y, f[1], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.z, f[2], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(val.w, f[3], physx::PxAsc::PxF32StrLen));
    				ret = buf;
    			}
			}
    		break;
    	case TYPE_QUAT:
			{
				physx::PxQuat val;
				if ((error = getParamQuat(handle, val)) == ERROR_NONE)
				{
					float quat[4];
					//val.getXYZW(quat);
					quat[0] = val.x; quat[1] = val.y; quat[2] = val.z; quat[3] = val.w;
					char f[4][physx::PxAsc::PxF32StrLen];
					physx::shdfnd::snprintf(buf, n,"%s %s %s %s",
						 physx::PxAsc::valueToStr(quat[0], f[0], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(quat[1], f[1], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(quat[2], f[2], physx::PxAsc::PxF32StrLen),
						 physx::PxAsc::valueToStr(quat[3], f[3], physx::PxAsc::PxF32StrLen));

					ret = buf;
				}
			}
    		break;
    	case TYPE_MAT33:
			{
				physx::PxMat33 val;
				if ((error = getParamMat33(handle, val)) == ERROR_NONE)
				{
					char f[9][physx::PxAsc::PxF32StrLen];
					const float *vals = val.front();
					physx::shdfnd::snprintf(buf, n,"%s %s %s  %s %s %s  %s %s %s",
						physx::PxAsc::valueToStr(vals[0], f[0], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[1], f[1], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[2], f[2], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[3], f[3], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[4], f[4], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[5], f[5], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[6], f[6], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[7], f[7], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[8], f[8], physx::PxAsc::PxF32StrLen));
					ret = buf;
				}
			}
    		break;
    	case TYPE_MAT44:
			{
				physx::PxMat44 val;
				if ((error = getParamMat44(handle, val)) == ERROR_NONE)
				{
					char f[16][physx::PxAsc::PxF32StrLen];
					const float *vals = val.front();
					physx::shdfnd::snprintf(buf, n,"%s %s %s %s  %s %s %s %s  %s %s %s %s  %s %s %s %s",
						physx::PxAsc::valueToStr(vals[0], f[0], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[1], f[1], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[2], f[2], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[3], f[3], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[4], f[4], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[5], f[5], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[6], f[6], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[7], f[7], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[8], f[8], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[9], f[9], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[10], f[10], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[11], f[11], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[12], f[12], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[13], f[13], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[14], f[14], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[15], f[15], physx::PxAsc::PxF32StrLen));
					ret = buf;
				}
			}
    		break;
    	case TYPE_MAT34:
			{
				float vals[12];
				if ((error = getParamMat34Legacy(handle, vals)) == ERROR_NONE)
				{
					char f[16][physx::PxAsc::PxF32StrLen];
					physx::shdfnd::snprintf(buf, n,"%s %s %s  %s %s %s  %s %s %s  %s %s %s",
						physx::PxAsc::valueToStr(vals[0], f[0], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[1], f[1], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[2], f[2], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[3], f[3], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[4], f[4], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[5], f[5], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[6], f[6], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[7], f[7], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[8], f[8], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[9], f[9], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[10], f[10], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(vals[11], f[11], physx::PxAsc::PxF32StrLen));
					ret = buf;
				}
			}
    		break;
		case TYPE_BOUNDS3:
			{
				char f[6][physx::PxAsc::PxF32StrLen];
				physx::PxBounds3 val;
				if ((error = getParamBounds3(handle, val)) == ERROR_NONE)
				{
					physx::shdfnd::snprintf(buf, n,"%s %s %s  %s %s %s ",
						physx::PxAsc::valueToStr(val.minimum.x, f[0], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.minimum.y, f[1], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.minimum.z, f[2], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.maximum.x, f[3], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.maximum.y, f[4], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.maximum.z, f[5], physx::PxAsc::PxF32StrLen));
					ret = buf;
				}
			}
    		break;
    	case TYPE_TRANSFORM:
			{
				char f[7][physx::PxAsc::PxF32StrLen];
				physx::PxTransform val;
				if ((error = getParamTransform(handle, val)) == ERROR_NONE)
				{
					physx::shdfnd::snprintf(buf, n,"%s %s %s %s  %s %s %s ",
						physx::PxAsc::valueToStr(val.q.x, f[0], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.q.y, f[1], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.q.z, f[2], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.q.w, f[3], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.p.x, f[4], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.p.y, f[5], physx::PxAsc::PxF32StrLen),
						physx::PxAsc::valueToStr(val.p.z, f[6], physx::PxAsc::PxF32StrLen));
					ret = buf;
				}
			}
    		break;
        case TYPE_UNDEFINED:
        case TYPE_ARRAY:
        case TYPE_STRUCT:
			break;
        case TYPE_STRING:
            error = getParamString(handle, ret);
			break;
        case TYPE_ENUM:
            error = getParamEnum(handle, ret);
			break;
		case TYPE_REF:
		{
			const Hint *hint = paramDef->hint("INCLUDED");
			if( hint && hint->type() != TYPE_U64 )
			{
				PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INVALID_REFERENCE_INCLUDE_HINT");
				return(ERROR_INVALID_REFERENCE_INCLUDE_HINT);
			}

			if( hint != NULL && hint->asUInt() == 1 )
			{
				// included, output the entire struct
			}
			else
			{
				//not included, get the "name" from the NvParameterized pointer
				NvParameterized::Interface *paramPtr = 0;
				ErrorType err = getParamRef(handle, paramPtr);
				PX_ASSERT(err == ERROR_NONE);

				if(err != ERROR_NONE)
				{
					return err;
				}

				ret = NULL;
				if(paramPtr)
				{
					ret = paramPtr->name();
				}
			}
			error = ERROR_NONE;
		}
		break;
		case TYPE_BOOL:
    		{
        		bool val;
                if ((error = getParamBool(handle, val)) == ERROR_NONE)
                {
					ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_I8:
    		{
        		int8_t val;
                if ((error = getParamI8(handle, val)) == ERROR_NONE)
                {
					ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_I16:
    		{
        		int16_t val;
                if ((error = getParamI16(handle, val)) == ERROR_NONE)
                {
                	ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_I32:
    		{
        		int32_t val;
                if ((error = getParamI32(handle, val)) == ERROR_NONE)
                {
                	ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_I64:
    		{
        		int64_t val;
                if ((error = getParamI64(handle, val)) == ERROR_NONE)
                {
           			ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_U8:
    		{
        		uint8_t val;
                if ((error = getParamU8(handle, val)) == ERROR_NONE)
                {
					ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_U16:
    		{
        		uint16_t val;
                if ((error = getParamU16(handle, val)) == ERROR_NONE)
                {
                	ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_U32:
    		{
        		uint32_t val;
                if ((error = getParamU32(handle, val)) == ERROR_NONE)
                {
                	ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_U64:
    		{
        		uint64_t val;
                if ((error = getParamU64(handle, val)) == ERROR_NONE)
                {
              		ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_F32:
    		{
        		float val;
                if ((error = getParamF32(handle, val)) == ERROR_NONE)
                {
					ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;
		case TYPE_F64:
    		{
        		double val;
                if ((error = getParamF64(handle, val)) == ERROR_NONE)
                {
					ret = physx::PxAsc::valueToStr(val, buf, n);
    			}
			}
			break;

		// Make compiler happy
		case TYPE_POINTER:
		case TYPE_LAST:
			break;
    }

	PX_ASSERT(error == ERROR_NONE);

    return error;
}

ErrorType NvParameters::strToValue(Handle &handle, const char *str, const char **endptr) // assigns this string to the value
{
	ErrorType ret = ERROR_NONE;

	const Definition *pd = handle.parameterDefinition();

	switch ( pd->type() )
	{
        case TYPE_UNDEFINED:
        case TYPE_ARRAY:
		case TYPE_STRUCT:    ret = ERROR_TYPE_NOT_SUPPORTED;                                  break;

		case TYPE_STRING:    ret = setParamString(handle,str);                                break;
        case TYPE_ENUM:      ret = setParamEnum(handle,str);                                  break;
		case TYPE_REF:       ret = ERROR_TYPE_NOT_SUPPORTED;                                  break;
		case TYPE_BOOL:	     ret = setParamBool(handle,physx::PxAsc::strToBool(str, endptr)); break;
        case TYPE_I8:  	     ret = setParamI8(handle,physx::PxAsc::strToI8(str, endptr));     break;
        case TYPE_I16:       ret = setParamI16(handle,physx::PxAsc::strToI16(str, endptr));   break;
        case TYPE_I32:       ret = setParamI32(handle,physx::PxAsc::strToI32(str, endptr));   break;
        case TYPE_I64:       ret = setParamI64(handle,physx::PxAsc::strToI64(str, endptr));   break;
        case TYPE_U8:        ret = setParamU8(handle,physx::PxAsc::strToU8(str, endptr));     break;
        case TYPE_U16:       ret = setParamU16(handle,physx::PxAsc::strToU16(str, endptr));   break;
        case TYPE_U32:       ret = setParamU32(handle,physx::PxAsc::strToU32(str, endptr));   break;
        case TYPE_U64:       ret = setParamU64(handle,physx::PxAsc::strToU64(str, endptr));   break;
        case TYPE_F32:       ret = setParamF32(handle,physx::PxAsc::strToF32(str, endptr));   break;
        case TYPE_F64:       ret = setParamF64(handle,physx::PxAsc::strToF64(str, endptr));   break;
        case TYPE_VEC2:      ret = setParamVec2(handle,getVec2(str, endptr));                 break;
        case TYPE_VEC3:      ret = setParamVec3(handle,getVec3(str, endptr));                 break;
        case TYPE_VEC4:      ret = setParamVec4(handle,getVec4(str, endptr));                 break;
        case TYPE_QUAT:      ret = setParamQuat(handle,getQuat(str, endptr));                 break;
        case TYPE_MAT33:     ret = setParamMat33(handle,getMat33(str, endptr));               break;
        case TYPE_MAT34:     
			{
				float f[12];
				getMat34Legacy(str, endptr, f);
				ret = setParamMat34Legacy(handle,f);   
				break;
			}
        case TYPE_MAT44:     ret = setParamMat44(handle,getMat44(str, endptr));               break;
        case TYPE_BOUNDS3:   ret = setParamBounds3(handle,getBounds3(str, endptr));           break;
        case TYPE_TRANSFORM: ret = setParamTransform(handle,getTransform(str, endptr));       break;

NV_PARAMETRIZED_SERVICE_AND_LAST_DATATYPE_LABELS
		default:             ret = ERROR_TYPE_NOT_SUPPORTED;                                  break;
    }

	PX_ASSERT(ret == ERROR_NONE);

    return ret;
}

physx::PxVec2 		NvParameters::getVec2(const char *str, const char **endptr)
{
	physx::PxVec2 ret(0,0);
	physx::PxAsc::strToF32s(&ret.x,2,str,endptr);

	return ret;
}

physx::PxVec3 		NvParameters::getVec3(const char *str, const char **endptr)
{
	physx::PxVec3 ret(0,0,0);
	physx::PxAsc::strToF32s(&ret.x,3,str,endptr);

	return ret;
}

physx::PxVec4 		NvParameters::getVec4(const char *str, const char **endptr)
{
	physx::PxVec4 ret(0,0,0,0);
	physx::PxAsc::strToF32s(&ret.x,4,str,endptr);

	return ret;
}

physx::PxQuat 		NvParameters::getQuat(const char *str, const char **endptr)
{
	physx::PxQuat ret;
	//ret.identity();
	float quat[4];
	//ret.getXYZW(quat);
	physx::PxAsc::strToF32s(quat,4,str,endptr);
	//ret.setXYZW(quat);
	ret = physx::PxQuat(quat[0], quat[1], quat[2], quat[3]);

	return ret;
}

physx::PxMat33 		NvParameters::getMat33(const char *str, const char **endptr)
{
	physx::PxMat33 ret;
	float *vals = const_cast<float *>(ret.front());
	physx::PxAsc::strToF32s(vals,9,str,endptr);
	return ret;
}

physx::PxMat44 		NvParameters::getMat44(const char *str, const char **endptr)
{
	physx::PxMat44 ret;
	float *vals = const_cast<float *>(ret.front());
	physx::PxAsc::strToF32s(vals,16,str,endptr);
	return ret;
}

void NvParameters::getMat34Legacy(const char *str, const char **endptr, float (&val)[12])
{
	physx::PxAsc::strToF32s(val,12,str,endptr);
}

physx::PxBounds3    NvParameters::getBounds3(const char *str, const char **endptr)
{
	physx::PxBounds3 ret;
	ret.setEmpty();
	physx::PxAsc::strToF32s(&ret.minimum.x,6,str,endptr);
	return ret;
}

//******************************************************************************
//*** Vec2
//******************************************************************************

ErrorType NvParameters::setParamVec2(const Handle &handle, const physx::PxVec2 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_VEC2)
    {
        return rawSetParamVec2(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec2(const Handle &handle, physx::PxVec2 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_VEC2)
    {
        return rawGetParamVec2(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec2Array(const Handle &handle, physx::PxVec2 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC2)
    return(rawGetParamVec2Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamVec2Array(const Handle &handle, const physx::PxVec2 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC2)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamVec2Array(handle, array, n, offset));
}

//******************************************************************************
//*** Vec3
//******************************************************************************

ErrorType NvParameters::setParamVec3(const Handle &handle, const physx::PxVec3 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_VEC3)
    {
        return rawSetParamVec3(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec3(const Handle &handle, physx::PxVec3 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_VEC3)
    {
        return rawGetParamVec3(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec3Array(const Handle &handle, physx::PxVec3 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC3)
    return(rawGetParamVec3Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamVec3Array(const Handle &handle, const physx::PxVec3 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC3)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamVec3Array(handle, array, n, offset));
}

//******************************************************************************
//*** Vec4
//******************************************************************************

ErrorType NvParameters::setParamVec4(const Handle &handle, const physx::PxVec4 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_VEC4)
    {
        return rawSetParamVec4(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec4(const Handle &handle, physx::PxVec4 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_VEC4)
    {
        return rawGetParamVec4(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamVec4Array(const Handle &handle, physx::PxVec4 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC4)
    return(rawGetParamVec4Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamVec4Array(const Handle &handle, const physx::PxVec4 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(VEC4)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamVec4Array(handle, array, n, offset));
}

//******************************************************************************
//*** Quat
//******************************************************************************

ErrorType NvParameters::setParamQuat(const Handle &handle, const physx::PxQuat &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_QUAT)
    {
        return rawSetParamQuat(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamQuat(const Handle &handle, physx::PxQuat &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_QUAT)
    {
        return rawGetParamQuat(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamQuatArray(const Handle &handle, physx::PxQuat *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(QUAT)
    return(rawGetParamQuatArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamQuatArray(const Handle &handle, const physx::PxQuat *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(QUAT)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamQuatArray(handle, array, n, offset));
}

//******************************************************************************
//*** Mat33
//******************************************************************************

ErrorType NvParameters::setParamMat33(const Handle &handle, const physx::PxMat33 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

    	if(handle.parameterDefinition()->type() == TYPE_MAT33)
    {
        return rawSetParamMat33(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat33(const Handle &handle, physx::PxMat33 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_MAT33)
    {
        return rawGetParamMat33(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat33Array(const Handle &handle, physx::PxMat33 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT33)
    return(rawGetParamMat33Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamMat33Array(const Handle &handle, const physx::PxMat33 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT33)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamMat33Array(handle, array, n, offset));
}

//******************************************************************************
//*** Mat34Legacy
//******************************************************************************

ErrorType NvParameters::setParamMat34Legacy(const Handle &handle, const float (&val)[12])
{
    CHECK_HANDLE
    CHECK_F32_FINITE

   	if(handle.parameterDefinition()->type() == TYPE_MAT34)
    {
        return rawSetParamMat34Legacy(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat34Legacy(const Handle &handle, float (&val)[12]) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_MAT34)
    {
        return rawGetParamMat34Legacy(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat34LegacyArray(const Handle &handle, float *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT34)
    return(rawGetParamMat34LegacyArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamMat34LegacyArray(const Handle &handle, const float *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT34)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamMat34LegacyArray(handle, array, n, offset));
}

//******************************************************************************
//*** Mat44
//******************************************************************************

ErrorType NvParameters::setParamMat44(const Handle &handle, const physx::PxMat44 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

   	if(handle.parameterDefinition()->type() == TYPE_MAT44)
    {
        return rawSetParamMat44(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat44(const Handle &handle, physx::PxMat44 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_MAT44)
    {
        return rawGetParamMat44(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamMat44Array(const Handle &handle, physx::PxMat44 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT44)
    return(rawGetParamMat44Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamMat44Array(const Handle &handle, const physx::PxMat44 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(MAT44)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamMat44Array(handle, array, n, offset));
}

//******************************************************************************
//*** Bounds3
//******************************************************************************

ErrorType NvParameters::setParamBounds3(const Handle &handle, const physx::PxBounds3 &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

	if(handle.parameterDefinition()->type() == TYPE_BOUNDS3)
    {
        return rawSetParamBounds3(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamBounds3(const Handle &handle, physx::PxBounds3 &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_BOUNDS3)
    {
        return rawGetParamBounds3(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamBounds3Array(const Handle &handle, physx::PxBounds3 *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(BOUNDS3)
    return(rawGetParamBounds3Array(handle, array, n, offset));
}

ErrorType NvParameters::setParamBounds3Array(const Handle &handle, const physx::PxBounds3 *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(BOUNDS3)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamBounds3Array(handle, array, n, offset));
}

//******************************************************************************
//*** Transform
//******************************************************************************

ErrorType NvParameters::setParamTransform(const Handle &handle, const physx::PxTransform &val)
{
    CHECK_HANDLE
    CHECK_F32_FINITE

   	if(handle.parameterDefinition()->type() == TYPE_TRANSFORM)
    {
        return rawSetParamTransform(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
	return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamTransform(const Handle &handle, physx::PxTransform &val) const
{
    CHECK_HANDLE

	if(handle.parameterDefinition()->type() == TYPE_TRANSFORM)
    {
        return rawGetParamTransform(handle, val);
    }

	PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_CAST_FAILED");
    return(ERROR_CAST_FAILED);
}

ErrorType NvParameters::getParamTransformArray(const Handle &handle, physx::PxTransform *array, int32_t n, int32_t offset) const
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(TRANSFORM)
    return(rawGetParamTransformArray(handle, array, n, offset));
}

ErrorType NvParameters::setParamTransformArray(const Handle &handle, const physx::PxTransform *array, int32_t n, int32_t offset)
{
    CHECK_HANDLE
    CHECK_IS_SIMPLE_ARRAY(TRANSFORM)
    CHECK_F32_FINITE_ARRAY
    return(rawSetParamTransformArray(handle, array, n, offset));
}

physx::PxTransform    NvParameters::getTransform(const char *str, const char **endptr)
{
	physx::PxTransform ret;
	physx::PxAsc::strToF32s((float*)&ret,7,str,endptr);
	return ret;
}


//***********************************************
physx::PxVec2 NvParameters::init(float x,float y)
{
	physx::PxVec2 ret(x,y);
	return ret;
}

physx::PxVec3 NvParameters::init(float x,float y,float z)
{
	physx::PxVec3 ret(x,y,z);
	return ret;
}

physx::PxVec4 NvParameters::initVec4(float x,float y,float z,float w)
{
	physx::PxVec4 ret(x,y,z,w);
	return ret;
}

physx::PxQuat NvParameters::init(float x,float y,float z,float w)
{
	physx::PxQuat ret;
	//ret.setXYZW(x,y,z,w);
	ret = physx::PxQuat(x,y,z,w);
	return ret;
}

physx::PxMat33 NvParameters::init(float _11,float _12,float _13,float _21,float _22,float _23,float _31,float _32,float _33)
{
	physx::PxMat33 ret;

	ret.column0.x = _11;
	ret.column0.y = _21;
	ret.column0.z = _31;

	ret.column1.x = _12;
	ret.column1.y = _22;
	ret.column1.z = _32;

	ret.column2.x = _13;
	ret.column2.y = _23;
	ret.column2.z = _33;

	return ret;
}

physx::PxMat44 NvParameters::init(float _11,float _12,float _13,float _14,float _21,float _22,float _23,float _24,float _31,float _32,float _33,float _34,float _41,float _42,float _43,float _44)
{
	physx::PxMat44 ret;

	ret.column0.x = _11;
	ret.column0.y = _21;
	ret.column0.z = _31;
	ret.column0.w = _41;

	ret.column1.x = _12;
	ret.column1.y = _22;
	ret.column1.z = _32;
	ret.column1.w = _42;

	ret.column2.x = _13;
	ret.column2.y = _23;
	ret.column2.z = _33;
	ret.column2.w = _43;

	ret.column3.x = _14;
	ret.column3.y = _24;
	ret.column3.z = _34;
	ret.column3.w = _44;

	return ret;
}

physx::PxTransform NvParameters::init(float x,float y,float z,float qx,float qy,float qz,float qw)
{
	return physx::PxTransform(physx::PxVec3(x,y,z), physx::PxQuat(qx,qy,qz,qw));
}


physx::PxBounds3 NvParameters::init(float minx,float miny,float minz,float maxx,float maxy,float maxz)
{
	physx::PxBounds3 ret;
	ret.minimum = physx::PxVec3(minx,miny,minz);
	ret.maximum = physx::PxVec3(maxx,maxy,maxz);
	return ret;
}



int32_t NvParameters::MultIntArray(const int32_t *array, int32_t n)
{
    PX_ASSERT(array != NULL);
    PX_ASSERT(n > 0);

    int32_t ret = array[0];

    for(int32_t i=1; i < n; ++i)
        ret *= array[i];

    return(ret);
}


ErrorType NvParameters::resizeArray(Traits *parameterizedTraits,
													 void *&buf, 
													 int32_t *array_sizes, 
													 int32_t dimension, 
													 int32_t resize_dim,
													 int32_t new_size,
													 bool doFree,
													 int32_t element_size,
													 uint32_t element_align,
													 bool &isMemAllocated)
{
    PX_ASSERT(array_sizes != NULL);
    PX_ASSERT(dimension > 0);
    PX_ASSERT(resize_dim >= 0 && resize_dim < dimension);
    PX_ASSERT(new_size >= 0);
	PX_ASSERT(element_size > 0 );

    if(array_sizes[resize_dim] == new_size)
        return(ERROR_NONE); //isMemAllocated is unchanged

    int32_t newSizes[Handle::MAX_DEPTH];
    physx::intrinsics::memCopy(newSizes, array_sizes, dimension * sizeof(int32_t));

    newSizes[resize_dim] = new_size;

    int32_t currentNumElems = MultIntArray(array_sizes, dimension);
    int32_t newNumElems = MultIntArray(newSizes, dimension);

    if(newNumElems <= 0)
    {
        if(buf != NULL && doFree) 
            parameterizedTraits->free(buf);

        buf = NULL;
        goto no_error;
    }

    if(buf == NULL) 
    {
		PX_ASSERT(element_size * newNumElems >= 0);
        if((buf = allocAligned(parameterizedTraits, static_cast<uint32_t>(element_size * newNumElems), element_align)) == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_MEMORY_ALLOCATION_FAILURE");
            return(ERROR_MEMORY_ALLOCATION_FAILURE);
		}

		isMemAllocated = true;
        
        //initialize the array to 0's (for strings)
		physx::intrinsics::memZero(buf, uint32_t(element_size * newNumElems));

        goto no_error;
    }

    if(resize_dim == 0)
    {
        void *newBuf;
        char *newBufDataStart;
        int32_t newBufDataSize;
        
		PX_ASSERT(element_size * newNumElems >= 0);
        // alloc new buffer
        if((newBuf = allocAligned(parameterizedTraits, static_cast<uint32_t>(element_size * newNumElems), element_align)) == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_MEMORY_ALLOCATION_FAILURE");
            return(ERROR_MEMORY_ALLOCATION_FAILURE);
		}

		isMemAllocated = true;
        
        // copy existing data to new buffer
		if(newNumElems < currentNumElems)
			physx::intrinsics::memCopy(newBuf, buf, uint32_t(element_size * newNumElems));
		else
			physx::intrinsics::memCopy(newBuf, buf, uint32_t(element_size * currentNumElems));
        
        // zero the new part of the array
        if(newNumElems > currentNumElems)
        {
            newBufDataStart = (char *)newBuf + (currentNumElems * element_size);
            newBufDataSize = (newNumElems - currentNumElems) * element_size;

			PX_ASSERT(newBufDataSize >= 0);
            memset(newBufDataStart, 0, static_cast<uint32_t>(newBufDataSize));
        }
        
        if( doFree )
        parameterizedTraits->free(buf);
        buf = newBuf;
    }
    else
    {
		PX_ASSERT(element_size * newNumElems >= 0);
        void *newBuf = allocAligned(parameterizedTraits, static_cast<uint32_t>(element_size * newNumElems), element_align);
        if(newBuf == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_MEMORY_ALLOCATION_FAILURE");
            return(ERROR_MEMORY_ALLOCATION_FAILURE);
		}

		isMemAllocated = true;

        //initialize the array to 0's (for strings)
		physx::intrinsics::memSet(newBuf, 0U, uint32_t(element_size * newNumElems));
        recursiveCopy(buf, array_sizes, newBuf, newSizes, dimension, element_size);

		if( doFree )
        parameterizedTraits->free(buf);
        buf = newBuf;
    }
      

no_error:

    array_sizes[resize_dim] = new_size;
    return(ERROR_NONE);
}

void NvParameters::recursiveCopy(const void *src, 
								 const int32_t *src_sizes, 
								 void *dst, 
								 const int32_t *dst_sizes, 
								 int32_t dimension, 
								 int32_t element_size,
								 int32_t *indexes, 
								 int32_t level)
{
    int32_t srcSize = src_sizes[level];
    int32_t dstSize = dst_sizes[level];

    int32_t size = physx::PxMin(srcSize, dstSize);

    int32_t indexStore[Handle::MAX_DEPTH];

    if(indexes == NULL)
    {
        indexes = indexStore;
        memset(indexes, 0, Handle::MAX_DEPTH * sizeof(int32_t));
    }

    if(level == dimension - 1)
    {
        int32_t srcIndex = indexes[0];
        int32_t dstIndex = indexes[0];

        for(int32_t i=1; i < dimension; ++i)
        {
            srcIndex = src_sizes[i] * (srcIndex) + indexes[i];
            dstIndex = dst_sizes[i] * (dstIndex) + indexes[i];
        }

		char *copy_dst = (char *)dst + dstIndex * element_size;
		char *copy_src = (char *)src + srcIndex * element_size;
		PX_ASSERT(element_size * size > 0);
		physx::intrinsics::memCopy(copy_dst, copy_src, uint32_t(element_size * size));
        return;
    }

    for(int32_t i=0; i < size; ++i)
    {
        indexes[level] = i;
        recursiveCopy(src, src_sizes, dst, dst_sizes, dimension, element_size, indexes, level + 1);
    }
}

ErrorType NvParameters::getParamU8Array(const Handle &handle, uint8_t *array, int32_t n, int32_t offset) const
{
	CHECK_HANDLE
	CHECK_IS_SIMPLE_ARRAY(U8)
	return(rawGetParamArray<uint8_t>(handle, array, n, offset,this));
}

ErrorType NvParameters::getParamF64Array(const Handle &handle, double *array, int32_t n, int32_t offset) const
{
	CHECK_HANDLE
	CHECK_IS_SIMPLE_ARRAY(F64)
	return(rawGetParamArray<double>(handle, array, n, offset,this));
}




ErrorType NvParameters::rawSetParamBool(const Handle &handle, bool val)
{
	return rawSetParam<bool>(handle,val,this);
}

ErrorType NvParameters::rawGetParamBool(const Handle &handle, bool &val) const
{
	return rawGetParam<bool>(handle,val,this);
}

ErrorType NvParameters::rawGetParamBoolArray(const Handle &handle, bool *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<bool>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamBoolArray(const Handle &handle, const bool *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<bool>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamString(const Handle &handle, const char *&val) const
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}

	DummyStringStruct *var = (DummyStringStruct *)(char *)ptr;
	val = var->buf;
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamString(const Handle &handle, const char *val)
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	DummyStringStruct *var = (DummyStringStruct *)(char *)ptr;
	if( var->isAllocated )
	{
		getTraits()->strfree((char *)var->buf);
	}

	var->buf = getTraits()->strdup(val);
	var->isAllocated = true;

	return(ERROR_NONE);
}

ErrorType NvParameters::rawGetParamEnum(const Handle &handle, const char *&val) const
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}
	const char * *var = (const char * *)((char *)ptr);
	val = *var;
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamEnum(const Handle &handle, const char *val)
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}
	const char * *Var = (const char * *)((char *)ptr);
	*Var = val;
	return(ERROR_NONE);
}

ErrorType NvParameters::rawGetParamRef(const Handle &handle, NvParameterized::Interface *&val) const
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if ( ptr == NULL )
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_TYPE_NOT_SUPPORTED");
		return ERROR_TYPE_NOT_SUPPORTED;
	}

	NvParameterized::Interface * *var = (NvParameterized::Interface * *)((char *)ptr);
	val = *var;
	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamRef(const Handle &handle, NvParameterized::Interface * val)
{
	size_t offset;
	void *ptr=NULL;
	getVarPtr(handle, ptr, offset);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	NvParameterized::Interface * *Var = (NvParameterized::Interface * *)((char *)ptr);
	*Var = val;
	return(ERROR_NONE);
}

ErrorType NvParameters::rawGetParamI8(const Handle &handle, int8_t &val) const
{
	return rawGetParam<int8_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamI8(const Handle &handle, int8_t val)
{
	return rawSetParam<int8_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamI8Array(const Handle &handle, int8_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<int8_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamI8Array(const Handle &handle, const int8_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<int8_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamI16(const Handle &handle, int16_t &val) const
{
	return rawGetParam<int16_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamI16(const Handle &handle, int16_t val)
{
	return rawSetParam<int16_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamI16Array(const Handle &handle, int16_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<int16_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamI16Array(const Handle &handle, const int16_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<int16_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamI32(const Handle &handle, int32_t &val) const
{
	return rawGetParam<int32_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamI32(const Handle &handle, int32_t val)
{
	return rawSetParam<int32_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamI32Array(const Handle &handle, int32_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<int32_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamI32Array(const Handle &handle, const int32_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<int32_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamI64(const Handle &handle, int64_t &val) const
{
	return rawGetParam<int64_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamI64(const Handle &handle, int64_t val)
{
	return rawSetParam<int64_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamI64Array(const Handle &handle, int64_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<int64_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamI64Array(const Handle &handle, const int64_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<int64_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawGetParamU8(const Handle &handle, uint8_t &val) const
{
	return rawGetParam<uint8_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamU8(const Handle &handle, uint8_t val)
{
	return rawSetParam<uint8_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamU8Array(const Handle &handle, uint8_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<uint8_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamU8Array(const Handle &handle, const uint8_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<uint8_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamU16(const Handle &handle, uint16_t &val) const
{
	return rawGetParam<uint16_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamU16(const Handle &handle, uint16_t val)
{
	return rawSetParam<uint16_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamU16Array(const Handle &handle, uint16_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<uint16_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamU16Array(const Handle &handle, const uint16_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<uint16_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamU32(const Handle &handle, uint32_t &val) const
{
	return rawGetParam<uint32_t>(handle,val,this);
}

ErrorType NvParameters::rawSetParamU32(const Handle &handle, uint32_t val)
{
	return rawSetParam<uint32_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamU32Array(const Handle &handle, uint32_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<uint32_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamU32Array(const Handle &handle, const uint32_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<uint32_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamU64(const Handle &handle, uint64_t val)
{
	return rawSetParam<uint64_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamU64(const Handle &handle, uint64_t &val) const
{
	return rawGetParam<uint64_t>(handle,val,this);
}

ErrorType NvParameters::rawGetParamU64Array(const Handle &handle, uint64_t *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<uint64_t>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamU64Array(const Handle &handle, const uint64_t *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<uint64_t>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamF32(const Handle &handle, float &val) const
{
	return rawGetParam<float>(handle,val,this);
}

ErrorType NvParameters::rawSetParamF32(const Handle &handle, float val)
{
	return rawSetParam<float>(handle,val,this);
}

ErrorType NvParameters::rawGetParamF32Array(const Handle &handle, float *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<float>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamF32Array(const Handle &handle, const float *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<float>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawGetParamF64(const Handle &handle, double &val) const
{
	return rawGetParam<double>(handle,val,this);
}

ErrorType NvParameters::rawSetParamF64(const Handle &handle, double val)
{
	return rawSetParam<double>(handle,val,this);
}

ErrorType NvParameters::rawGetParamF64Array(const Handle &handle, double *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<double>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamF64Array(const Handle &handle, const double *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<double>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamVec2(const Handle &handle,physx::PxVec2 val)
{
	return rawSetParam<physx::PxVec2>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec2(const Handle &handle,physx::PxVec2 &val) const
{
	return rawGetParam<physx::PxVec2>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec2Array(const Handle &handle,physx::PxVec2 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxVec2>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamVec2Array(const Handle &handle, const physx::PxVec2 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxVec2>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamVec3(const Handle &handle,physx::PxVec3 val)
{
	return rawSetParam<physx::PxVec3>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec3(const Handle &handle,physx::PxVec3 &val) const
{
	return rawGetParam<physx::PxVec3>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec3Array(const Handle &handle,physx::PxVec3 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxVec3>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamVec3Array(const Handle &handle, const physx::PxVec3 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxVec3>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamVec4(const Handle &handle,physx::PxVec4 val)
{
	return rawSetParam<physx::PxVec4>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec4(const Handle &handle,physx::PxVec4 &val) const
{
	return rawGetParam<physx::PxVec4>(handle,val,this);
}

ErrorType NvParameters::rawGetParamVec4Array(const Handle &handle,physx::PxVec4 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxVec4>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamVec4Array(const Handle &handle, const physx::PxVec4 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxVec4>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamQuat(const Handle &handle,physx::PxQuat val)
{
	return rawSetParam<physx::PxQuat>(handle,val,this);
}

ErrorType NvParameters::rawGetParamQuat(const Handle &handle,physx::PxQuat &val) const
{
	return rawGetParam<physx::PxQuat>(handle,val,this);
}

ErrorType NvParameters::rawGetParamQuatArray(const Handle &handle,physx::PxQuat *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxQuat>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamQuatArray(const Handle &handle, const physx::PxQuat *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxQuat>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamMat33(const Handle &handle,physx::PxMat33 val)
{
	return rawSetParam<physx::PxMat33>(handle,val,this);
}

ErrorType NvParameters::rawGetParamMat33(const Handle &handle,physx::PxMat33 &val) const
{
	return rawGetParam<physx::PxMat33>(handle,val,this);
}

ErrorType NvParameters::rawGetParamMat33Array(const Handle &handle,physx::PxMat33 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxMat33>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamMat33Array(const Handle &handle, const physx::PxMat33 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxMat33>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamMat34Legacy(const Handle &handle, const float (&val)[12])
{
	Handle memberHandle(handle);

	size_t tmp;
	void *ptr = NULL;
	this->getVarPtr(memberHandle, ptr, tmp);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	physx::intrinsics::memCopy(ptr, val, 12 * sizeof(float));

#if APEX_UE4
	// swap raw-column major
	swap(static_cast<float*>(ptr)[1], static_cast<float*>(ptr)[3]);
	swap(static_cast<float*>(ptr)[2], static_cast<float*>(ptr)[6]);
	swap(static_cast<float*>(ptr)[5], static_cast<float*>(ptr)[7]);
#endif
	return(ERROR_NONE);
}

ErrorType NvParameters::rawGetParamMat34Legacy(const Handle &handle, float (&val)[12]) const
{
	Handle memberHandle(handle);

	size_t tmp;
	void *ptr = NULL;
	this->getVarPtr(memberHandle, ptr, tmp);
	if(ptr == NULL)
	{
		PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
		return(ERROR_INDEX_OUT_OF_RANGE);
	}

	physx::intrinsics::memCopy(val, ptr, 12 * sizeof(float));

	return(ERROR_NONE);
}

ErrorType NvParameters::rawGetParamMat34LegacyArray(const Handle &handle, float *array, int32_t n, int32_t offset) const
{
	if( n )
	{
		Handle memberHandle(handle);
		NV_ERR_CHECK_RETURN(memberHandle.set(offset * 12));

		size_t tmp;
		void *ptr = NULL;
		this->getVarPtr(memberHandle, ptr, tmp);
		if(ptr == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
			return(ERROR_INDEX_OUT_OF_RANGE);
		}

		physx::intrinsics::memCopy(array, ptr, 12 * n * sizeof(float));
	}

	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamMat34LegacyArray(const Handle &handle, const float *array, int32_t n, int32_t offset)
{
	if( n )
	{
		Handle memberHandle(handle);
		NV_ERR_CHECK_RETURN(memberHandle.set(offset * 12));

		size_t tmp;
		void *ptr = NULL;
		this->getVarPtr(memberHandle, ptr, tmp);
		if(ptr == NULL)
		{
			PX_ASSERT_WITH_MESSAGE(0, "NVPARAMETERIZED.ERROR_INDEX_OUT_OF_RANGE");
			return(ERROR_INDEX_OUT_OF_RANGE);
		}

		for (int32_t i = 0; i < n; ++i)
		{
			float* dst = static_cast<float*>(ptr) + 12 * sizeof(float) * i;
			const float* src = array + 12 * sizeof(float) * i;

			physx::intrinsics::memCopy(dst, src, 12 * sizeof(float));
#if APEX_UE4
			swap(dst[1], dst[3]);
			swap(dst[2], dst[6]);
			swap(dst[5], dst[7]);
#endif
		}
	}

	return(ERROR_NONE);
}

ErrorType NvParameters::rawSetParamMat44(const Handle &handle,physx::PxMat44 val)
{
	return rawSetParam<physx::PxMat44>(handle,val,this);
}

ErrorType NvParameters::rawGetParamMat44(const Handle &handle,physx::PxMat44 &val) const
{
	return rawGetParam<physx::PxMat44>(handle,val,this);
}

ErrorType NvParameters::rawGetParamMat44Array(const Handle &handle,physx::PxMat44 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxMat44>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamMat44Array(const Handle &handle, const physx::PxMat44 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxMat44>(handle,array,n,offset,this);
}


ErrorType NvParameters::rawSetParamBounds3(const Handle &handle,physx::PxBounds3 val)
{
	return rawSetParam<physx::PxBounds3>(handle,val,this);
}

ErrorType NvParameters::rawGetParamBounds3(const Handle &handle,physx::PxBounds3 &val) const
{
	return rawGetParam<physx::PxBounds3>(handle,val,this);
}

ErrorType NvParameters::rawGetParamBounds3Array(const Handle &handle,physx::PxBounds3 *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxBounds3>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamBounds3Array(const Handle &handle, const physx::PxBounds3 *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxBounds3>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamTransform(const Handle &handle,physx::PxTransform val)
{
	return rawSetParam<physx::PxTransform>(handle,val,this);
}

ErrorType NvParameters::rawGetParamTransform(const Handle &handle,physx::PxTransform  &val) const
{
	return rawGetParam<physx::PxTransform>(handle,val,this);
}

ErrorType NvParameters::rawGetParamTransformArray(const Handle &handle,physx::PxTransform *array, int32_t n, int32_t offset) const
{
	return rawGetParamArray<physx::PxTransform>(handle,array,n,offset,this);
}

ErrorType NvParameters::rawSetParamTransformArray(const Handle &handle, const physx::PxTransform *array, int32_t n, int32_t offset)
{
	return rawSetParamArray<physx::PxTransform>(handle,array,n,offset,this);
}

void *NvParameters::getVarPtrHelper(const ParamLookupNode *rootNode, void *paramStruct, const Handle &handle, size_t &offset) const
{
	const ParamLookupNode* curNode = rootNode;

	bool hasDynamicArray = false;
	offset = curNode->offset;

	void *ptr = const_cast<void *>(paramStruct);
	for(int32_t i = 0; i < handle.numIndexes(); ++i)
	{
		int index = handle.index(i);

		if (curNode->type == TYPE_ARRAY)
		{
			PX_ASSERT(curNode->numChildren);

			if (curNode->isDynamicArrayRoot)
			{
				ptr = ((DummyDynamicArrayStruct*)ptr)->buf;
				hasDynamicArray = true;
				if (ptr == NULL)
				{
					offset = 0;
					return 0;
				}
			}

			// don't get the next curNode until after we've checked that the "parent" is dynamic
			curNode = &rootNode[curNode->children[0]];

			size_t localOffset = index * curNode->offset;
			offset += localOffset;
			ptr = (char*)ptr + localOffset;
		}
		else
		{
			PX_ASSERT(index >= 0 && index < curNode->numChildren);
			curNode = &rootNode[curNode->children[index]];
			offset += curNode->offset;
			ptr = (char*)ptr + curNode->offset;
		}
	}

	if (hasDynamicArray)
	{
		offset = 0;
	}

	return ptr;
}

bool NvParameters::checkAlignments() const
{
	// support empty, named references
	if(rootParameterDefinition() == NULL)
	{
		return IsAligned(this, 8);
	}

    Handle constHandle(*this, "");
	if( !constHandle.isValid() )
	{
		return false;
	}

	return checkAlignments(constHandle);
}

bool NvParameters::checkAlignments(Handle &param_handle) const
{
    const Definition *paramDef = param_handle.parameterDefinition();

	uint32_t align = paramDef->alignment();

	bool isDynamicArray = TYPE_ARRAY == paramDef->type() && !paramDef->arraySizeIsFixed();

	// For dynamic array alignment means alignment of it's first element
	if( !isDynamicArray )
	{
		size_t offset;
		void *ptr;
		getVarPtr(param_handle, ptr, offset);
		if( align > 0 && !IsAligned(ptr, align) )
		{
			return false;
		}
	}

	switch( paramDef->type() )
	{
	case TYPE_STRUCT:
		{
			for(int32_t i = 0; i < paramDef->numChildren(); ++i)
			{
				param_handle.set(i);
				if( !checkAlignments(param_handle) )
				{
					return false;
				}
				param_handle.popIndex();
			}
			break;
		}

	case TYPE_REF:
		{
			Interface *refObj;
			if( ERROR_NONE != param_handle.getParamRef(refObj) )
			{
				return false;
			}

			return 0 == refObj || refObj->checkAlignments();
		}

	case TYPE_ARRAY:
		{
			int32_t size;
			if( ERROR_NONE != param_handle.getArraySize(size) )
			{
				return false;
			}

			// See comment above
			if( isDynamicArray && align > 0 && size > 0 )
			{
				param_handle.set(0);

				size_t offset;
				void *ptr;
				getVarPtr(param_handle, ptr, offset);
				if( !IsAligned(ptr, align) )
				{
					return false;
				}

				param_handle.popIndex();
			}

			bool isSimpleType = paramDef->child(0)->isSimpleType();

			// Only check for first 10 elements if simple type
			size = physx::PxMin(size, isSimpleType ? 10 : INT32_MAX);

			for(int32_t i = 0; i < size; ++i)
			{
				param_handle.set(i);
				if( !checkAlignments(param_handle) )
				{
					return false;
				}
				param_handle.popIndex();
			}

			break;
		}
	NV_PARAMETRIZED_NO_AGGREGATE_AND_REF_DATATYPE_LABELS
	default:
		break;
    }

    return(true);
}
} // namespace NvParameterized
