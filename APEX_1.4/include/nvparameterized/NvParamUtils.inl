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
#include "NvParameterized.h"
#include "NvParameterizedTraits.h"

namespace NvParameterized
{

#if PX_VC && !PX_PS4
	#pragma warning(push)
	#pragma warning(disable: 4996)
#endif	//!PX_PS4

#define MAX_SEARCH_NAME 1024
#define MAX_SEARCH_NAMES 32

/// local_strcmp
int32_t local_strcmp(const char* s1, const char* s2);
/// local_stricmp
int32_t local_stricmp(const char* s1, const char* s2);

/// Finds parameter in NvParameterized
struct ParameterFind
{
	/// Contructor
	ParameterFind(const NvParameterized::Interface &iface) : mNameIndex(0), mNameCount(0), mResult(iface), mInterface(NULL), mError(false)
	{

	}
	
	/// isDigit
	bool isDigit(char c) const
	{
		return (c>='0' && c <='9');
	}

	/// setSearchName
	bool setSearchName(const char *str)
	{
		bool ret = true;

		mNameIndex = 0;
		mNameCount = 0;
		mInterface = 0;
		mError = false;
		char *dest = mSearchName;
		char *stop = &mSearchName[MAX_SEARCH_NAME-1];

		char *head = mSearchName;
		mArrayIndex[0] = -1;

		while ( *str && dest < stop )
		{
			if ( *str == '[' )
			{
				*dest++ = 0;
				str++;
				if ( isDigit(*str) )
				{
					int32_t v = 0;
					while ( isDigit(*str) )
					{
						int32_t iv = *str-'0';
						v = v*10+iv;
						str++;
					}
					if ( *str == ']' )
					{
						mArrayIndex[mNameCount] = v;
					}
					else
					{
						ret = false;
						break;
					}
				}
				else
				{
					ret = false;
					break;
				}
			}
			else
			{
				if ( *str == '.' )
				{
					if ( mNameCount < MAX_SEARCH_NAMES )
					{
						mSearchNames[mNameCount] = head;
						mNameCount++;
						if ( mNameCount < MAX_SEARCH_NAMES )
						{
							mArrayIndex[mNameCount] = -1;
						}
						*dest++ = 0;
						str++;
						head = dest;
					}
					else
					{
						ret = false;
						break;
					}
				}
				else
				{
					*dest++ = *str++;
				}
			}
		}

		*dest = 0;

		if ( head && *head )
		{
			if ( mNameCount < MAX_SEARCH_NAMES )
			{
				mSearchNames[mNameCount] = head;
				mNameCount++;
				*dest++ = 0;
				str++;
				head = dest;
			}
			else
			{
				ret = false;
			}
		}

		return ret;
	}

	/// done
	bool done(void) const
	{
		bool ret = false;
		if ( mInterface || mError ) ret = true;
		return ret;
	}

	/// getArrayIndex
	int32_t getArrayIndex(void)
	{
		return mArrayIndex[mNameIndex];
	}

	/// nameMatch
	bool nameMatch(const char *longName) const
	{
		bool ret = true;

		if (longName && strlen(longName))
		{
#if PX_GCC || PX_LINUX || PX_PS4 || PX_ANDROID || PX_OSX || PX_CLANG
			if ( local_stricmp(longName,mSearchNames[mNameIndex]) == 0 )
#else
#pragma warning(push)
#pragma warning(disable: 4996)

			if ( _stricmp(longName,mSearchNames[mNameIndex]) == 0 )

#pragma warning(pop)
#endif	
			{
				ret = true;
			}
			else
			{
				ret = false;
			}
		}

		return ret;
	}

	/// isCompleete
	bool isComplete(void) const
	{
		return (mNameIndex+1) == mNameCount;
	}

	/// pushNameMatch
	bool pushNameMatch(void)
	{
		bool ret = false;

		if ( (mNameIndex+1) < mNameCount )
		{
			mNameIndex++;
			ret = true;
		}
		return ret;
	}

	/// popNameMatch
	void popNameMatch(void)
	{
		if ( mNameIndex )
		{
			mNameIndex--;
		}
	}

	uint32_t							mNameIndex;	///< mNameIndex
	uint32_t				    		mNameCount; ///< mNameCount
	Handle 								mResult; ///< mResult
	const NvParameterized::Interface	*mInterface; ///< mInterface
	bool								mError; ///< mError;
	char								mSearchName[MAX_SEARCH_NAME]; ///< mSearchName
	char				  				*mSearchNames[MAX_SEARCH_NAMES]; ///< mSearchNames
	int32_t								mArrayIndex[MAX_SEARCH_NAMES]; ///< mArrayIndex
};

/// findParameter
PX_INLINE void findParameter(const NvParameterized::Interface &obj,
							 Handle &handle,
							 ParameterFind &pf,
							 bool fromArray=false)
{
	if ( pf.done() ) return;

	if ( handle.numIndexes() < 1 )
	{
		obj.getParameterHandle("",handle);
	}

	const Definition *pd = handle.parameterDefinition();
	const char *name = pd->name();
	DataType t = pd->type();

	if ( fromArray || pf.nameMatch(name) )
	{
		NvParameterized::Interface *paramPtr = 0;
		if ( t == TYPE_REF )
		{
			handle.getParamRef(paramPtr);
			if ( paramPtr  )
			{
				if ( pf.pushNameMatch() )
				{
					Handle newHandle(*paramPtr, "");
					findParameter(*paramPtr,newHandle,pf);
					pf.popNameMatch();
				}
			}
			if ( pf.isComplete() )
			{
				pf.mInterface = &obj;
				pf.mResult    = handle;
			}
		}
		if ( t == TYPE_STRUCT )
		{
			bool pushed = false;
			if ( strlen(name) )
			{
				pf.pushNameMatch();
				pushed = true;
			}
			int32_t count = pd->numChildren();
			for (int32_t i=0; i<count; i++)
			{
				handle.set(i);
				findParameter(obj,handle,pf);
				handle.popIndex();
				if ( pf.done() ) break;
			}
			if ( pushed )
			{
				pf.popNameMatch();
			}
		}
		else if ( t == TYPE_ARRAY )
		{
			int32_t arraySize;
			handle.getArraySize(arraySize);
			int32_t arrayIndex = pf.getArrayIndex();
			if ( arrayIndex == -1 && pf.isComplete() )
			{
				pf.mInterface = &obj;
				pf.mResult    = handle;
			}
			else if ( arrayIndex >= 0 && arrayIndex < arraySize )
			{
				handle.set(arrayIndex);
				if ( pf.isComplete() )
				{
					pf.mInterface = &obj;
					pf.mResult    = handle;
				}
				else
				{
					findParameter(obj,handle,pf,true);
				}
				handle.popIndex();
			}
			else
			{

				pf.mError = true;
			}
		}
		else if ( pf.isComplete() )
		{
			pf.mInterface = &obj;
			pf.mResult    = handle;
		}
	}
}

PX_INLINE const Interface *findParam(const Interface &i,const char *long_name, Handle &result)
{
	const Interface *ret = 0;
	result.setInterface((const NvParameterized::Interface *)0);

	ParameterFind pf(i);
	if ( pf.setSearchName(long_name) )
	{
		Handle handle(i);
		findParameter(i,handle,pf);
		result 	= pf.mResult;
		ret 	= pf.mInterface;
	}
	return ret;
}

PX_INLINE Interface *findParam(Interface &i,const char *long_name, Handle &result)
{
	Interface *ret = const_cast<Interface *>(
		findParam(const_cast<const Interface &>(i),long_name,result));
	result.setInterface(ret); // Give write access to handle
	return ret;
}

/// Parameter list
struct ParameterList
{
	/// Constructor
	ParameterList(const NvParameterized::Interface &iface,const char *className,const char *paramName,bool recursive,bool classesOnly,NvParameterized::Traits *traits) 
		: mNameIndex(0), 
		mResult(iface), 
		mInterface(NULL), 
		mClassName(className), 
		mParamName(paramName), 
		mRecursive(recursive), 
		mClassesOnly(classesOnly), 
		mTraits(traits), 
		mResultCount(0), 
		mMaxResults(0), 
		mResults(NULL)
	{

	}

	~ParameterList(void)
	{
	}

	/// nameMatch
	bool nameMatch(const Interface *iface,const char * name,int32_t arrayIndex,DataType type,Handle &handle)
	{
		size_t slen = strlen(name);

		if ( mClassesOnly )
		{
			if ( slen > 0 ) return true;
		}
		else
		{
			if ( slen == 0 ) return true;
		}

		bool match = true;
		if ( mClassName )
		{
			const char *cname = iface->className();
			if ( local_strcmp(cname,mClassName) != 0 || strlen(name) )
			{
				match = false;
			}
		}
		if ( mParamName ) // if we specified a parameter name, than only include exact matches of this parameter name.
		{
			if (local_strcmp(mParamName,name) != 0 )
			{
				match = false;
			}

		}
		if ( match )
		{
			// ok..let's build the long name...
			const char *longName = NULL;
			char scratch[1024];
			scratch[0] = 0;

			if ( slen > 0 )
			{

				for (uint32_t i=0; i<mNameIndex; i++)
				{
					local_strcat_s(scratch, sizeof(scratch), mSearchName[i]);
					if ( mArrayIndex[i] > 0 )
					{
						char arrayIndexStr[32];
						arrayIndexStr[0] = 0;
						local_strcat_s(arrayIndexStr, sizeof(arrayIndexStr), "[0]");
						//sprintf_s(arrayIndexStr, sizeof(arrayIndexStr), "[%d]", 0); //mArrayIndex[i]);
						local_strcat_s(scratch, sizeof(scratch), arrayIndexStr);
					}
					local_strcat_s(scratch, sizeof(scratch),".");
				}

				local_strcat_s(scratch, sizeof(scratch),name);
				uint32_t len = (uint32_t)strlen(scratch);
				char *temp = (char *)mTraits->alloc(len+1);
				temp[0] = 0;
				local_strcat_s(temp, len+1, scratch);
				longName = temp;
				if ( type == TYPE_ARRAY )
				{
					handle.getArraySize(arrayIndex);
				}
			}

			ParamResult pr(name,longName,iface->className(),iface->name(), handle, arrayIndex, type );

			if ( mResultCount >= mMaxResults )
			{
				mMaxResults = mMaxResults ? mMaxResults*2 : 32;
				ParamResult *results = (ParamResult *)mTraits->alloc(sizeof(ParamResult)*mMaxResults);
				if ( mResults )
				{
					for (uint32_t i=0; i<mResultCount; i++)
					{
						results[i] = mResults[i];
					}
				}
				mResults = results;
			}
			mResults[mResultCount] = pr;
			mResultCount++;
		}

		return true; // always matches....
	}

	/// pushName
	bool pushName(const char *name,int32_t arrayIndex)
	{
		mSearchName[mNameIndex] = name;
		mArrayIndex[mNameIndex] = arrayIndex;
		mNameIndex++;
		return true;
	}

	/// popNameMatch
	void popNameMatch(void)
	{
		if ( mNameIndex )
		{
			mNameIndex--;
		}
	}

	/// isRecursive
	bool isRecursive(void) const { return mRecursive; }

	/// getResultCount
	uint32_t getResultCount(void) const { return mResultCount; }
	
	/// getResults
	ParamResult * getResults(void) const { return mResults; }

	uint32_t					mNameIndex; ///< mNameIndex
	Handle 						mResult; ///< mResult
	const NvParameterized::Interface *mInterface; ///< mInterface
	const char *				mClassName; ///< mClassName
	const char *				mParamName; ///< mParamName
	bool						mRecursive; ///< mRecursive
	bool						mClassesOnly; ///< mClassesOnly
	NvParameterized::Traits		*mTraits; ///< mTraits
	uint32_t					mResultCount; ///< mResultCount
	uint32_t					mMaxResults; ///< mMaxResults
	ParamResult					*mResults; ///< mResults
	const char					*mSearchName[MAX_SEARCH_NAME]; ///< mSearchName
	int32_t						mArrayIndex[MAX_SEARCH_NAME]; ///< mArrayIndex

};

/// listParameters
PX_INLINE void listParameters(const NvParameterized::Interface &obj,
			   		Handle &handle,
			   		ParameterList &pf,
					int32_t parentArraySize)
{
	if ( handle.numIndexes() < 1 )
	{
		obj.getParameterHandle("",handle);
	}

	const Definition *pd = handle.parameterDefinition();
	const char *name = pd->name();
	DataType t = pd->type();

	if ( pf.nameMatch(&obj,name,parentArraySize,t,handle) )
	{
		NvParameterized::Interface *paramPtr = 0;
		if ( t == TYPE_REF )
		{
			handle.getParamRef(paramPtr);
		}
		if ( t == TYPE_STRUCT )
		{
			bool pushed=false;
			if ( strlen(name) )
			{
				pf.pushName(name,parentArraySize);
				pushed = true;
			}
       		int32_t count = pd->numChildren();
       		for (int32_t i=0; i<count; i++)
       		{
       			handle.set(i);
       			listParameters(obj,handle,pf,0);
       			handle.popIndex();
			}
			if ( pushed )
			{
				pf.popNameMatch();
			}
		}
		else if ( t == TYPE_ARRAY )
		{
   			int32_t arraySize;
   			handle.getArraySize(arraySize);
			if ( arraySize > 0 )
			{
				for (int32_t i=0; i<arraySize; i++)
				{
					handle.set(i);
					listParameters(obj,handle,pf,arraySize);
					const Definition *elemPd = handle.parameterDefinition();
					DataType elemType = elemPd->type();
					handle.popIndex();
					if ( !(elemType == TYPE_STRUCT || elemType == TYPE_ARRAY || elemType == TYPE_REF) )
					{
						break;
					}
				}
			}
		}
		else if ( t == TYPE_REF && pf.isRecursive() )
		{
			if ( paramPtr && pd->isIncludedRef() )
			{
				if ( pf.pushName(name,parentArraySize) )
				{
     				Handle newHandle(*paramPtr, "");
     				listParameters(*paramPtr,newHandle,pf,0);
     				pf.popNameMatch();
     			}
			}
		}
	}
}


/**
\brief Gets every parameter in an NvParameterized Interface class
\note The return pointer is allocated by the NvParameterized Traits class and should be freed by calling releaseParamList
*/
PX_INLINE const ParamResult *	getParamList(const Interface &i,const char *className,const char *paramName,uint32_t &count,bool recursive,bool classesOnly,NvParameterized::Traits *traits)
{

	PX_UNUSED(className);

	ParameterList pl(i,className,paramName,recursive,classesOnly,traits);

   	Handle handle(i);
	listParameters(i,handle,pl,0);

	count = pl.getResultCount();

	return pl.getResults();
}

PX_INLINE void	releaseParamList(uint32_t resultCount,const ParamResult *results,NvParameterized::Traits *traits)
{
	if ( results )
	{
		for (uint32_t i=0; i<resultCount; i++)
		{
			const ParamResult &r = results[i];
			if ( r.mLongName )
			{
				traits->free( (void *)r.mLongName );
			}
		}
		traits->free((void *)results);
	}
}

/// Calls back for every reference.
PX_INLINE void getReferences(const Interface &iface,
										  Handle &handle,
										  ReferenceInterface &cb,
										  bool named,
										  bool included,
										  bool recursive)
{
	if ( handle.numIndexes() < 1 )
		iface.getParameterHandle("",handle);

	NvParameterized::Interface *paramPtr = 0;

	const Definition *pd = handle.parameterDefinition();
	switch( pd->type() )
	{
	case TYPE_REF:
		handle.getParamRef(paramPtr);
		if ( !paramPtr )
			break;

		if ( !pd->isIncludedRef() )
		{
			if( named )
				cb.referenceCallback(handle);
		}
		else
		{
			if( included )
				cb.referenceCallback(handle);

			if ( recursive )
			{
				Handle newHandle(*paramPtr, "");
				getReferences(*paramPtr,newHandle,cb,named,included,recursive);
			}
		}
		break;

	case TYPE_STRUCT:
		{
			int32_t count = pd->numChildren();
			for (int32_t i=0; i<count; i++)
			{
				handle.set(i);
				getReferences(iface,handle,cb,named,included,recursive);
				handle.popIndex();
			}

			break;
		}

	case TYPE_ARRAY:
		{
			int32_t arraySize;
			handle.getArraySize(arraySize);
			if ( arraySize <= 0 )
				break;

			const Definition *elemPd = pd->child(0);
			bool scan = elemPd->type() == TYPE_ARRAY || elemPd->type() == TYPE_REF || elemPd->type() == TYPE_STRUCT;

			if ( scan )
			{
				for (int32_t i=0; i<arraySize; i++)
				{
					handle.set(i);
					getReferences(iface,handle,cb,named,included,recursive);
					handle.popIndex();
				}
			}

			break;
		}
NV_PARAMETRIZED_NO_AGGREGATE_AND_REF_DATATYPE_LABELS
	default:
		break;
	}
}

PX_INLINE void getReferences(const Interface &i,
									 ReferenceInterface &cb,
									 bool named,
									 bool included,
									 bool recursive)
{
	Handle handle(i);
	getReferences(i,handle,cb,named,included,recursive);
}

/// WrappedNamedReference
class WrappedNamedReference: public ReferenceInterface
{
	NamedReferenceInterface &wrappedReference;

	///Silence warnings on unable to generate assignment operator
	template<typename T> void operator =(T) {}
	void operator =(WrappedNamedReference) {}
public:

	/// refCount
	uint32_t refCount;

	virtual ~WrappedNamedReference() {}
	
	/// Constructor
	WrappedNamedReference(NamedReferenceInterface &wrappedReference_): wrappedReference(wrappedReference_), refCount(0) {}

	/// referenceCallback
	void referenceCallback(Handle &handle)
	{
		Interface *iface;
		handle.getParamRef(iface);
		const char *name = wrappedReference.namedReferenceCallback(iface->className(), iface->name(), handle);
		if( name )
		{
			iface->setName(name);
			++refCount;
		}
	}
};

PX_INLINE uint32_t getNamedReferences(const Interface &i,
										  NamedReferenceInterface &namedReference,
										  bool recursive)
{
	WrappedNamedReference reference(namedReference);
	getReferences(i, reference, true, false, recursive);
	return reference.refCount;
}


PX_INLINE bool getParamBool(const Interface &pm, const char *name, bool &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamBool(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamBool(Interface &pm, const char *name, bool value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamBool(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// string
PX_INLINE bool getParamString(const Interface &pm, const char *name, const char *&value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamString(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamString(Interface &pm, const char *name, const char *value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamString(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// enum
PX_INLINE bool getParamEnum(const Interface &pm, const char *name,  const char *&value)
{																   
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;   
	NvParameterized::Handle handle(pm);							   
	const NvParameterized::Interface *iface = findParam(pm,name,handle);   
	if ( iface )												   
	{															   
		ret = handle.getParamEnum(value);						   
	}															   
	return (ret == NvParameterized::ERROR_NONE);													   
}																   
																   
PX_INLINE bool setParamEnum(Interface &pm, const char *name,  const char *value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamEnum(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// reference
PX_INLINE bool getParamRef(const Interface &pm, const char *name, NvParameterized::Interface *&value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamRef(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamRef(Interface &pm, const char *name,  NvParameterized::Interface *value, bool doDestroyOld)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamRef(value, doDestroyOld);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool initParamRef(Interface &pm, const char *name, const char *className, bool doDestroyOld)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.initParamRef(className, doDestroyOld);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool initParamRef(Interface &pm, const char *name, const char *className, const char *objName, bool doDestroyOld)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.initParamRef(className, doDestroyOld);
		NvParameterized::Interface *ref;
		handle.getParamRef(ref);
		if (ref)
			ref->setName(objName);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// I8
PX_INLINE bool getParamI8(const Interface &pm, const char *name, int8_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamI8(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamI8(Interface &pm, const char *name, int8_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamI8(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// I16
PX_INLINE bool getParamI16(const Interface &pm, const char *name, int16_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamI16(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamI16(Interface &pm, const char *name, int16_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamI16(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// I32
PX_INLINE bool getParamI32(const Interface &pm, const char *name, int32_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamI32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamI32(Interface &pm, const char *name, int32_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamI32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// I64
PX_INLINE bool getParamI64(const Interface &pm, const char *name, int64_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamI64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamI64(Interface &pm, const char *name, int64_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamI64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// U8
PX_INLINE bool getParamU8(const Interface &pm, const char *name, uint8_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamU8(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamU8(Interface &pm, const char *name, uint8_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamU8(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// U16
PX_INLINE bool getParamU16(const Interface &pm, const char *name, uint16_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamU16(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamU16(Interface &pm, const char *name, uint16_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamU16(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// U32
PX_INLINE bool getParamU32(const Interface &pm, const char *name, uint32_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamU32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamU32(Interface &pm, const char *name, uint32_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamU32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// U64
PX_INLINE bool getParamU64(const Interface &pm, const char *name, uint64_t &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamU64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamU64(Interface &pm, const char *name, uint64_t value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamU64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// F32
PX_INLINE bool getParamF32(const Interface &pm, const char *name, float &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamF32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamF32(Interface &pm, const char *name, float value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamF32(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// F64
PX_INLINE bool getParamF64(const Interface &pm, const char *name, double &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamF64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamF64(Interface &pm, const char *name, double value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamF64(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Vec2
PX_INLINE bool getParamVec2(const Interface &pm, const char *name, physx::PxVec2 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamVec2(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamVec2(Interface &pm, const char *name, const physx::PxVec2 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamVec2(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Vec3
PX_INLINE bool getParamVec3(const Interface &pm, const char *name, physx::PxVec3 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamVec3(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamVec3(Interface &pm, const char *name, const physx::PxVec3 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamVec3(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Vec4
PX_INLINE bool getParamVec4(const Interface &pm, const char *name, physx::PxVec4 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamVec4(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamVec4(Interface &pm, const char *name, const physx::PxVec4 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamVec4(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Quat
PX_INLINE bool getParamQuat(const Interface &pm, const char *name, physx::PxQuat &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamQuat(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamQuat(Interface &pm, const char *name, const physx::PxQuat &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamQuat(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Bounds3
PX_INLINE bool getParamBounds3(const Interface &pm, const char *name, physx::PxBounds3 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamBounds3(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamBounds3(Interface &pm, const char *name, const physx::PxBounds3 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamBounds3(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Mat33
PX_INLINE bool getParamMat33(const Interface &pm, const char *name, physx::PxMat33 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamMat33(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamMat33(Interface &pm, const char *name, const physx::PxMat33 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamMat33(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Mat44
PX_INLINE bool getParamMat44(const Interface &pm, const char *name, physx::PxMat44 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamMat44(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamMat44(Interface &pm, const char *name, const physx::PxMat44 &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.setParamMat44(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

// Transform
PX_INLINE bool getParamTransform(const Interface &pm, const char *name, physx::PxTransform &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getParamTransform(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool setParamTransform(Interface &pm, const char *name, const physx::PxTransform &value)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		handle.setInterface( iface );	// set mIsConst to false
		ret = handle.setParamTransform(value);
	}
	return (ret == NvParameterized::ERROR_NONE);
}


PX_INLINE bool getParamArraySize(const Interface &pm, const char *name, int32_t &arraySize)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	const NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.getArraySize(arraySize);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

PX_INLINE bool resizeParamArray(Interface &pm, const char *name, int32_t newSize)
{
	ErrorType ret = NvParameterized::ERROR_INVALID_PARAMETER_NAME;
	NvParameterized::Handle handle(pm);
	NvParameterized::Interface *iface = findParam(pm,name,handle);
	if ( iface )
	{
		ret = handle.resizeArray(newSize);
	}
	return (ret == NvParameterized::ERROR_NONE);
}

#if PX_VC && !PX_PS4
	#pragma warning(pop)
#endif	//!PX_PS4

}
