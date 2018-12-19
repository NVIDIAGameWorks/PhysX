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
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.


/*!
\brief NvParameterized inline implementation
*/

namespace NvParameterized
{

#include "nvparameterized/NvParameterizedMacroses.h"

#if PX_VC && !PX_PS4
	#pragma warning(push)
	#pragma warning(disable: 4996)
#endif	//!PX_PS4

#define IS_ALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c) (IS_ALPHA(c) || IS_DIGIT(c))
#define IS_IDENTCHAR(c) (IS_ALPHANUM(c) || (c) == ' ' || (c) == '_')

/**
\brief Enum of tokenizer result types
*/
enum TokenizerResultType
{
   TOKENIZER_RESULT_NONE,
   TOKENIZER_RESULT_BUFFER_OVERFLOW,
   TOKENIZER_RESULT_SYNTAX_ERROR,
   TOKENIZER_RESULT_STRUCT_MEMBER,
   TOKENIZER_RESULT_ARRAY_INDEX
};

/**
\brief Get struct member token
*/
PX_INLINE TokenizerResultType getStructMemberToken(const char *long_name,
                                                char *token,
                                                uint32_t max_token_len,
                                                uint32_t &offset)
{
    PX_ASSERT(long_name != NULL);
    PX_ASSERT(token != NULL);
    PX_ASSERT(max_token_len > 1);
    PX_ASSERT(IS_IDENTCHAR(long_name[0]) || long_name[0] == '.');

    offset = 0;

    if(long_name[0] == '.')
      offset++;

    uint32_t tokenLen = 0;
    while(IS_IDENTCHAR(long_name[offset]))
    {
        if(offset == max_token_len - 1)
           return(TOKENIZER_RESULT_BUFFER_OVERFLOW);

        token[tokenLen++] = long_name[offset++];
    }

    token[tokenLen++] = 0;

    if(long_name[offset] != 0 && long_name[offset] != '.' && long_name[offset] != '[')
        return(TOKENIZER_RESULT_SYNTAX_ERROR);

    return(TOKENIZER_RESULT_STRUCT_MEMBER);
}

/**
\brief Get array member token
*/
PX_INLINE TokenizerResultType getArrayMemberToken(const char *long_name,
                                               char *token,
                                               uint32_t max_token_len,
                                               uint32_t &offset)
{
    PX_ASSERT(long_name != NULL);
    PX_ASSERT(token != NULL);
    PX_ASSERT(max_token_len > 1);
    PX_ASSERT(long_name[0] == '[');

    offset = 1;

    uint32_t tokenLen = 0;
    while(long_name[offset] && IS_DIGIT(long_name[offset]))
    {
        if(tokenLen == max_token_len - 1)
             return(TOKENIZER_RESULT_BUFFER_OVERFLOW);

        token[tokenLen++] = long_name[offset++];
    }

    token[tokenLen++] = 0;

    if(long_name[offset] != ']')
        return(TOKENIZER_RESULT_SYNTAX_ERROR);

    offset++;

    return(TOKENIZER_RESULT_ARRAY_INDEX);
}

/**
\brief Get next token
*/
PX_INLINE TokenizerResultType getNextToken(const char *long_name,
                                        char *token,
                                        uint32_t max_token_len,
                                        uint32_t &offset)
{
    PX_ASSERT(long_name != NULL);
    PX_ASSERT(token != NULL);
    PX_ASSERT(max_token_len > 1);

    if(long_name[0] == 0)
        return(TOKENIZER_RESULT_NONE);

    if(long_name[0] == '.' || IS_IDENTCHAR(long_name[0]))
        return(getStructMemberToken(long_name, token, max_token_len, offset));

    if(long_name[0] == '[')
        return(getArrayMemberToken(long_name, token, max_token_len, offset));

    return(TOKENIZER_RESULT_SYNTAX_ERROR);
}

#undef IS_ALPHA
#undef IS_DIGIT
#undef IS_ALPHANUM
#undef IS_IDENTCHAR

/*
 The local_strcat_s function appends strSource to strDestination and terminates the resulting string with a null character. 
 The initial character of strSource overwrites the terminating null character of  strDestination. The behavior of strcat_s is 
 undefined if the source and destination strings overlap. Note that the second parameter is the total size of the buffer, not 
 the remaining size
*/

/**
\brief The local_strcat_s function appends strSource to strDestination and terminates the resulting string with a null character.
*/
PX_INLINE int local_strcat_s(char* dest, size_t size, const char* src)
{
	size_t d, destStringLen, srcStringLen;

	if (	dest		== NULL ||
			src			== NULL ||
			size		== 0	)
	{
		return -1;
	}

	destStringLen	= strlen(dest);
	srcStringLen	= strlen(src);
	d				= srcStringLen + destStringLen;

	if ( size <= d )
	{
		return -1;
	}

	::memcpy( &dest[destStringLen], src, srcStringLen );
	dest[d] = '\0';

	return 0;
}

/**
\brief The local_sprintf_s function wraps the va_list functionality required for PxVxprintf
*/
int32_t local_sprintf_s( char * _DstBuf, size_t _DstSize, const char * _Format, ...);

PX_INLINE Handle::Handle(::NvParameterized::Interface *iface)
{
	reset();
	mInterface = iface;
	mIsConst = false;
	if (mInterface != NULL)
		mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(::NvParameterized::Interface &iface)
{
    reset();
	mInterface = &iface;
	mIsConst = false;
	mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(const ::NvParameterized::Interface &iface)
{
    reset();
	mInterface = const_cast< ::NvParameterized::Interface * >(&iface);
	mIsConst = true;
	mParameterDefinition = mInterface->rootParameterDefinition();
}

PX_INLINE Handle::Handle(const Handle &param_handle)
{
    reset();

    if(param_handle.isValid())
    {
        mNumIndexes = param_handle.mNumIndexes;
        memcpy(mIndexList, param_handle.mIndexList, sizeof(int32_t) * mNumIndexes);
        mParameterDefinition = param_handle.mParameterDefinition;
        mIsValid = param_handle.mIsValid;
		mIsConst = param_handle.mIsConst;
		mInterface = param_handle.mInterface;
    }
    else
        mIsConst = mIsValid = false;
}


PX_INLINE Handle::Handle(::NvParameterized::Interface &instance,const char *longName)
{
	mInterface = &instance;
	mIsConst = false;
    mInterface->getParameterHandle(longName, *this);
}

PX_INLINE Handle::Handle(const ::NvParameterized::Interface &instance,const char *longName)
{
	mInterface = const_cast< ::NvParameterized::Interface *>(&instance);
	mIsConst = true;
    mInterface->getParameterHandle(longName, *this);
}

PX_INLINE ErrorType Handle::getChildHandle(const ::NvParameterized::Interface *instance,const char *child_long_name, Handle &handle)
{
    handle = *this;
    handle.mUserData = NULL;
    return(handle.set(instance,child_long_name));
}

PX_INLINE ErrorType Handle::getParameter(const char *longName)
{
	if( !mInterface )
	{
		return ERROR_HANDLE_MISSING_INTERFACE_POINTER;
	}

	return mInterface->getParameterHandle(longName, *this);
}

PX_INLINE ErrorType  Handle::set(const ::NvParameterized::Interface *instance,const Definition *root,const char *child_long_name)
{
    PX_ASSERT(root->parent() == NULL);

    reset();
    mParameterDefinition = root;
    mIsValid = true;

    return(set(instance,child_long_name));
}

PX_INLINE ErrorType Handle::set(const ::NvParameterized::Interface *instance,const char *child_long_name)
{
    PX_ASSERT(mParameterDefinition != NULL);
    PX_ASSERT(child_long_name != NULL);

    if(!isValid())
        return(ERROR_INVALID_PARAMETER_HANDLE);

    mUserData = NULL;

    if(child_long_name[0] == 0)
    {
        return(ERROR_NONE);
    }

    int32_t indexLevel = 0;

    mIsValid = false;
    // while(1) causes C4127 warning
	for( ; ; )
    {
        char token[1024];
        uint32_t offset;

        TokenizerResultType Result = getNextToken(child_long_name, token, sizeof(token), offset);

        switch(Result)
        {
            case TOKENIZER_RESULT_NONE:
                if(indexLevel == 0)
                    return(ERROR_INVALID_PARAMETER_NAME);
                 else
                    goto no_error;

            case TOKENIZER_RESULT_BUFFER_OVERFLOW:
                return(ERROR_RESULT_BUFFER_OVERFLOW);

            case TOKENIZER_RESULT_SYNTAX_ERROR:
                return(ERROR_SYNTAX_ERROR_IN_NAME);

            case TOKENIZER_RESULT_STRUCT_MEMBER:
                {
                    if(mParameterDefinition->type() != TYPE_STRUCT)
                        return(ERROR_NAME_DOES_NOT_MATCH_DEFINITION);

                    int32_t index;
                    mParameterDefinition = mParameterDefinition->child(token, index);
                    if(mParameterDefinition == NULL)
                        return(ERROR_INVALID_PARAMETER_NAME);

                    pushIndex(index);
                }
                break;

            case TOKENIZER_RESULT_ARRAY_INDEX:
                {
                    if(mParameterDefinition->type() != TYPE_ARRAY)
                        return(ERROR_NAME_DOES_NOT_MATCH_DEFINITION);

                    int32_t index = atoi(token);
                    PX_ASSERT(index >= 0);

					int32_t arraySize=0;
					if ( instance )
					{
						Handle handle(*instance);
						ErrorType err = instance->getParameterHandle( mParameterDefinition->longName(), handle );
						if(err != ERROR_NONE)
							return(err);
						handle.getArraySize(arraySize);
					}
					else
					{
						arraySize = mParameterDefinition->arraySize();
					}

                    if(index >= arraySize )
                        return(ERROR_INDEX_OUT_OF_RANGE);

                    PX_ASSERT(mParameterDefinition->numChildren() == 1);
                    mParameterDefinition = mParameterDefinition->child(0);

                    pushIndex(index);
                }
                break;
        }

        child_long_name += offset;
        indexLevel++;
    }

no_error:

    mIsValid = true;
    return(ERROR_NONE);
}

PX_INLINE ErrorType Handle::set(int32_t child_index)
{
    PX_ASSERT(mParameterDefinition != NULL);
    PX_ASSERT(child_index >= 0);

    switch(parameterDefinition()->type())
    {
        case TYPE_STRUCT:
            if(child_index < 0 || child_index >= parameterDefinition()->numChildren())
                return(ERROR_INDEX_OUT_OF_RANGE);
            mParameterDefinition = mParameterDefinition->child(child_index);
            pushIndex(child_index);

            break;


        case TYPE_ARRAY:
            if(child_index < 0)
                return(ERROR_INDEX_OUT_OF_RANGE);

			// parameterDefinition()->arraySize() does not work on dynamic arrays...
            if( parameterDefinition()->arraySizeIsFixed() &&
               child_index >= parameterDefinition()->arraySize())
                return(ERROR_INDEX_OUT_OF_RANGE);

            mParameterDefinition = mParameterDefinition->child(0);
            pushIndex(child_index);
            break;

		NV_PARAMETRIZED_NO_AGGREGATE_DATATYPE_LABELS
		default:
		{
            return(ERROR_IS_LEAF_NODE);
		}
    }

    mIsValid = true;
    return(ERROR_NONE);
}

PX_INLINE ErrorType Handle::getChildHandle(int32_t index, Handle &handle)
{
    if(parameterDefinition()->type() != TYPE_ARRAY && parameterDefinition()->type() != TYPE_STRUCT)
        return(ERROR_IS_LEAF_NODE);

    if(!isValid())
        return(ERROR_INVALID_PARAMETER_HANDLE);

    handle = *this;
    handle.pushIndex(index);
    if(parameterDefinition()->type() == TYPE_STRUCT)
    {
        PX_ASSERT(parameterDefinition()->child(index) != NULL);
		handle.mParameterDefinition = parameterDefinition()->child(index);
    }
    else
    {
        PX_ASSERT(parameterDefinition()->child(0) != NULL);
        handle.mParameterDefinition = parameterDefinition()->child(0);
    }

    return(ERROR_NONE);
}

PX_INLINE bool Handle::getLongName(char *str, uint32_t max_str_len) const
{
    PX_ASSERT(parameterDefinition() != NULL);

    if(!isValid())
        return(false);

    if(numIndexes() < 1)
        return(false);

    const Definition *root = parameterDefinition()->root();

    *str = 0;
    const Definition *node = root->child(index(0));
    char tmpStr[32];
    for(int32_t i=1; i <= numIndexes(); ++i)
    {
        PX_ASSERT(node != NULL);
        PX_ASSERT(node->parent() != NULL);

        switch(node->parent()->type())
        {
            case TYPE_STRUCT:
			{
                if(node->parent()->parent() != NULL)
				{
					local_strcat_s(str, max_str_len, ".");
				}
                local_strcat_s(str, max_str_len, node->name());
				break;
			}

            case TYPE_ARRAY:
			{
                local_strcat_s(str, max_str_len, "[");

				local_sprintf_s(tmpStr, sizeof(tmpStr), "%d", index(i-1));

                local_strcat_s(str, max_str_len, tmpStr);
                local_strcat_s(str, max_str_len, "]");
                break;
			}

            NV_PARAMETRIZED_NO_AGGREGATE_DATATYPE_LABELS
			default:
			{
                local_strcat_s(str, max_str_len, node->name());
			}
        }

        switch(node->type())
        {
            case TYPE_STRUCT:
                node = node->child(index(i));
                break;

            case TYPE_ARRAY:
                node = node->child(0);
                break;

			NV_PARAMETRIZED_NO_AGGREGATE_DATATYPE_LABELS
			default:
                node = NULL;
        }
    }

    return(true);
}

PX_INLINE void Handle::reset(void)
{
    mNumIndexes = 0;
    mParameterDefinition = NULL;
    mUserData = NULL;
    mIsValid = false;
}

PX_INLINE void Handle::pushIndex(int32_t index)
{
    PX_ASSERT(mNumIndexes < MAX_DEPTH);
    PX_ASSERT(index >= 0);

    if(mNumIndexes < MAX_DEPTH)
        mIndexList[mNumIndexes++] = index;
}

PX_INLINE int32_t Handle::popIndex(int32_t levels)
{
    PX_ASSERT(levels > 0);
    PX_ASSERT(mNumIndexes >= levels);
    PX_ASSERT(mParameterDefinition != NULL);

    if(mNumIndexes >= levels )
    {
        mNumIndexes -= levels;

        for(; levels > 0; --levels)
            mParameterDefinition = mParameterDefinition->parent();

        return(mIndexList[mNumIndexes]);
    }

    return(-1);
}

PX_INLINE ErrorType Handle::initParamRef(const char *chosenRefStr, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	return mInterface->initParamRef(*this, chosenRefStr, doDestroyOld);
}

// These functions wrap the raw(Get|Set)XXXXX() methods.  They deal with
// error handling and casting.

#define CHECK_CONST_HANDLE if( mIsConst ) return ERROR_MODIFY_CONST_HANDLE;

PX_INLINE ErrorType Handle::getParamBool(bool &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBool(*this,val);
}

PX_INLINE ErrorType Handle::setParamBool(bool val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBool(*this,val);
}

PX_INLINE ErrorType Handle::getParamBoolArray(bool *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBoolArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamBoolArray(const bool *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBoolArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::getParamString(const char *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamString(*this,val);
}

PX_INLINE ErrorType Handle::setParamString(const char *val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamString(*this,val);
}

PX_INLINE ErrorType Handle::getParamStringArray(char **array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamStringArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamStringArray(const char **array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamStringArray(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamEnum(const char *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamEnum(*this,val);
}

PX_INLINE ErrorType Handle::setParamEnum(const char *val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamEnum(*this,val);
}

PX_INLINE ErrorType Handle::getParamEnumArray(char **array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamEnumArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamEnumArray(const char **array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamEnumArray(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamRef(::NvParameterized::Interface *&val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamRef(*this,val);
}

PX_INLINE ErrorType Handle::setParamRef(::NvParameterized::Interface *val, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamRef(*this, val, doDestroyOld);
}

PX_INLINE ErrorType Handle::getParamRefArray(::NvParameterized::Interface **array, int32_t n, int32_t offset) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamRefArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamRefArray(::NvParameterized::Interface **array, int32_t n, int32_t offset, bool doDestroyOld)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamRefArray(*this,array,n,offset,doDestroyOld);
}

PX_INLINE ErrorType Handle::getParamI8(int8_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI8(*this,val);
}

PX_INLINE ErrorType Handle::setParamI8(int8_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI8(*this,val);
}

PX_INLINE ErrorType Handle::getParamI8Array(int8_t *_array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI8Array(*this,_array,n,offset);
}

PX_INLINE ErrorType Handle::setParamI8Array(const int8_t *val, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI8Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI16(int16_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI16(*this,val);
}

PX_INLINE ErrorType Handle::setParamI16(int16_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI16(*this,val);
}

PX_INLINE ErrorType Handle::getParamI16Array(int16_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI16Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamI16Array(const int16_t *val, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI16Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI32(int32_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI32(*this,val);
}

PX_INLINE ErrorType Handle::setParamI32(int32_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI32(*this,val);
}

PX_INLINE ErrorType Handle::getParamI32Array(int32_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI32Array(*this,array,n,offset ) ;
}

PX_INLINE ErrorType Handle::setParamI32Array(const int32_t *val, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI32Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamI64(int64_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI64(*this,val) ;
}

PX_INLINE ErrorType Handle::setParamI64(int64_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI64(*this,val);
}

PX_INLINE ErrorType Handle::getParamI64Array(int64_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamI64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamI64Array(const int64_t *val, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamI64Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamU8(uint8_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU8(*this,val);
}

PX_INLINE ErrorType Handle::setParamU8(uint8_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU8(*this,val);
}

PX_INLINE ErrorType Handle::getParamU8Array(uint8_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU8Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU8Array(const uint8_t *val, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU8Array(*this,val,n,offset);
}


PX_INLINE ErrorType Handle::getParamU16(uint16_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU16(*this,val);
}

PX_INLINE ErrorType Handle::setParamU16(uint16_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU16(*this,val);
}

PX_INLINE ErrorType Handle::getParamU16Array(uint16_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU16Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU16Array(const uint16_t *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU16Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamU32(uint32_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU32(*this,val);
}

PX_INLINE ErrorType Handle::setParamU32(uint32_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU32(*this,val);
}

PX_INLINE ErrorType Handle::getParamU32Array(uint32_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU32Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU32Array(const uint32_t *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU32Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamU64(uint64_t &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU64(*this,val);
}

PX_INLINE ErrorType Handle::setParamU64(uint64_t val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU64(*this,val);
}

PX_INLINE ErrorType Handle::getParamU64Array(uint64_t *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamU64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamU64Array(const uint64_t *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamU64Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::getParamF32(float &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF32(*this,val);
}

PX_INLINE ErrorType Handle::setParamF32(float val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF32(*this,val);
}

PX_INLINE ErrorType Handle::getParamF32Array(float *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF32Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamF32Array(const float *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF32Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::getParamF64(double &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF64(*this,val);
}

PX_INLINE ErrorType Handle::setParamF64(double val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF64(*this,val);
}

PX_INLINE ErrorType Handle::getParamF64Array(double *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamF64Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamF64Array(const double *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamF64Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec2(const physx::PxVec2 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec2(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec2(physx::PxVec2 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec2(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec2Array(physx::PxVec2 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec2Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec2Array(const physx::PxVec2 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec2Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec3(const physx::PxVec3 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec3(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec3(physx::PxVec3 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec3(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec3Array(physx::PxVec3 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec3Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec3Array(const physx::PxVec3 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec3Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamVec4(const physx::PxVec4 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec4(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec4(physx::PxVec4 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec4(*this,val);
}

PX_INLINE ErrorType Handle::getParamVec4Array(physx::PxVec4 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamVec4Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamVec4Array(const physx::PxVec4 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamVec4Array(*this,array,n,offset);
}


PX_INLINE ErrorType Handle::setParamQuat(const physx::PxQuat &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamQuat(*this,val);
}

PX_INLINE ErrorType Handle::getParamQuat(physx::PxQuat &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamQuat(*this,val);
}

PX_INLINE ErrorType Handle::getParamQuatArray(physx::PxQuat *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamQuatArray(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamQuatArray(const physx::PxQuat *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamQuatArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamMat33(const physx::PxMat33 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat33(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat33(physx::PxMat33 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat33(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat33Array(physx::PxMat33 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat33Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamMat33Array(const physx::PxMat33 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat33Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamMat44(const physx::PxMat44 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat44(physx::PxMat44 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat44(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat44Array(physx::PxMat44 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat44Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamMat44Array(const physx::PxMat44 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat44Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamMat34Legacy(const float (&val)[12])
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat34Legacy(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat34Legacy(float (&val)[12]) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat34Legacy(*this,val);
}

PX_INLINE ErrorType Handle::getParamMat34LegacyArray(float *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamMat34LegacyArray(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamMat34LegacyArray(const float *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamMat34LegacyArray(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamBounds3(const physx::PxBounds3 &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBounds3(*this,val);
}

PX_INLINE ErrorType Handle::getParamBounds3(physx::PxBounds3 &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBounds3(*this,val);
}

PX_INLINE ErrorType Handle::getParamBounds3Array(physx::PxBounds3 *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamBounds3Array(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamBounds3Array(const physx::PxBounds3 *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->setParamBounds3Array(*this,array,n,offset);
}

PX_INLINE ErrorType Handle::setParamTransform(const physx::PxTransform &val)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
		return mInterface->setParamTransform(*this,val);
}

PX_INLINE ErrorType Handle::getParamTransform(physx::PxTransform &val) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamTransform(*this,val);
}

PX_INLINE ErrorType Handle::getParamTransformArray(physx::PxTransform *array, int32_t n, int32_t offset ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getParamTransformArray(*this,array,n,offset );
}

PX_INLINE ErrorType Handle::setParamTransformArray(const physx::PxTransform *array, int32_t n, int32_t offset)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
		return mInterface->setParamTransformArray(*this,array,n,offset);
}



#define NV_PARAMETERIZED_TYPES_NO_LEGACY_TYPES
#define NV_PARAMETERIZED_TYPES_ONLY_SIMPLE_TYPES
#define NV_PARAMETERIZED_TYPES_NO_STRING_TYPES
#define NV_PARAMETERIZED_TYPE(type_name, enum_name, c_type) \
	template <> PX_INLINE ::NvParameterized::ErrorType Handle::setParam<c_type>(const c_type &val) { return setParam##type_name(val); } \
	template <> PX_INLINE ::NvParameterized::ErrorType Handle::getParam<c_type>(c_type &val) const {return getParam##type_name(val); } \
	template <> PX_INLINE ::NvParameterized::ErrorType Handle::getParamArray<c_type>(c_type *array, int32_t n, int32_t offset) const { return getParam##type_name##Array(array, n, offset); } \
	template <> PX_INLINE ::NvParameterized::ErrorType Handle::setParamArray<c_type>(const c_type *array, int32_t n, int32_t offset) { return setParam##type_name##Array(array, n, offset); }
#include "NvParameterized_types.h"

PX_INLINE ErrorType Handle::valueToStr(char *buf, uint32_t bufSize, const char *&ret)
{
	PX_ASSERT(mInterface);
	return mInterface->valueToStr(*this, buf, bufSize, ret);
}

PX_INLINE ErrorType Handle::strToValue(const char *str, const char **endptr) // assigns this string to the valu
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->strToValue(*this,str, endptr); // assigns this string to the value
}


PX_INLINE ErrorType Handle::resizeArray(int32_t new_size)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->resizeArray(*this,new_size);
}

PX_INLINE ErrorType Handle::getArraySize(int32_t &size, int32_t dimension ) const
{
	PX_ASSERT(mInterface);
	return mInterface->getArraySize(*this,size,dimension );
}

PX_INLINE ErrorType Handle::swapArrayElements(uint32_t firstElement, uint32_t secondElement)
{
	PX_ASSERT(mInterface);
	CHECK_CONST_HANDLE
	return mInterface->swapArrayElements(*this, firstElement, secondElement);
}

#undef IS_ALPHA
#undef IS_DIGIT
#undef IS_ALPHANUM
#undef IS_IDENTCHAR
#undef CHECK_CONST_HANDLE

#if PX_VC && !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

} // end of namespace
