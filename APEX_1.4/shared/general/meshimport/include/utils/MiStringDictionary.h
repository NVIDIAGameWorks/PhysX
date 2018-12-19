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



#ifndef MI_STRING_DICTIONARY_H
#define MI_STRING_DICTIONARY_H


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "MiPlatformConfig.h"
#include "MiStringTable.h"


#define __LINE ( __LINE__ )
#define __GETLINE(x) "(" #x ")"
#define GETLINE __GETLINE __LINE
#define IDE_MESSAGE(y) message ( __FILE__ GETLINE " : " #y " " )

#pragma warning(push)
#pragma warning(disable:4996)

namespace mimp
{

extern const char *nullstring;
extern const char *emptystring;


class StringRef 
{
	friend class StringRefHash;
	friend class StringRefEqual;
public:

	static const StringRef Null;
	static const StringRef Empty;
	static const StringRef EmptyInitializer(); // use this for static initializers -ENS

	StringRef(void)
	{
		mString = nullstring;
	}

	StringRef(size_t index)
	{
		mString = (const char *)index;
	}

	StringRef(const char *str);

	~StringRef(void)
	{
	}

	inline StringRef(const StringRef &str);

	operator const char *() const
	{
		return mString;
	}

	size_t operator()(const StringRef& key_value) const 
	{
		return (size_t)(key_value.mString);
	}

	bool operator()(const StringRef &s1, const StringRef &s2) const
	{
		return s1.mString == s2.mString == 0;
	}

	const char * Get(void) const { return mString; };

	size_t GetSizeT(void) const { return (size_t)mString; };

	void Set(const char *str)
	{
		mString = str;
	}

	const StringRef &operator= (const StringRef& rhs )
	{
		mString = rhs.Get();
		return *this;
	}

	bool operator== ( const StringRef& rhs ) const
	{
		return rhs.mString == mString;
	}

	bool operator< ( const StringRef& rhs ) const
	{
		return rhs.mString < mString;
	}

	bool operator!= ( const StringRef& rhs ) const
	{
		return rhs.mString != mString;
	}

	bool operator> ( const StringRef& rhs ) const
	{
		return rhs.mString > mString;
	}

	bool operator<= ( const StringRef& rhs ) const
	{
		return rhs.mString <= mString;
	}

	bool operator>= ( const StringRef& rhs ) const
	{
		return rhs.mString >= mString;
	}

	bool SamePrefix(const char *prefix) const
	{
		unsigned int len = (unsigned int)strlen(prefix);
		if ( len && strncmp(mString,prefix,len) == 0 ) return true;
		return false;
	}

	bool SameSuffix(const StringRef &suf) const
	{
		const char *source = mString;
		const char *suffix = suf.mString;
		unsigned int len1 = (unsigned int)strlen(source);
		unsigned int len2 = (unsigned int)strlen(suffix);
		if ( len1 < len2 ) return false;
		const char *compare = &source[(len1-len2)];
		if ( strcmp(compare,suffix) == 0 ) return true;
		return false;
	}

private:
	const char *mString; // the actual char ptr
};

class StringDict : public mimp::MeshImportAllocated
{
public:
	StringDict(void)
	{
		mLogging = false;
	}

	~StringDict(void)
	{
	}

	StringRef Get(const char *text)
	{
		StringRef ref;
		if ( text )
		{
			if ( strcmp(text,nullstring) == 0 )
			{
				ref.Set(nullstring);
			}
			else
			{
				if ( strcmp(text,emptystring) == 0 )
				{
					ref.Set(emptystring);
				}
				else
				{
					bool first;
					const char *foo = mStringTable.Get(text,first);
					ref.Set(foo);
				}
			}
		}
		return ref;
	}

	StringRef Get(const char *text,bool &first)
	{
		StringRef ref;
		const char *foo = mStringTable.Get(text,first);
		ref.Set(foo);
		return ref;
	}

	void SetLogging(bool state)
	{
		mLogging = state;
	}

	bool GetLogging(void) const
	{
		return mLogging;
	}
private:
	bool	mLogging;
	StringTable mStringTable;
};

inline StringRef::StringRef(const StringRef &str)
{
	mString = str.Get();
}


extern StringDict *gStringDict;

class StringSortRef
{
public:

	bool operator()(const StringRef &a,const StringRef &b) const
	{
		const char *str1 = a.Get();
		const char *str2 = b.Get();
		int r = MESH_IMPORT_STRING::stricmp(str1,str2);
		return r < 0;
	}
};



static inline StringDict * getGlobalStringDict(void)
{
	if ( gStringDict == 0 )
	{
		gStringDict = MI_NEW(StringDict);
	}
	return gStringDict;
}

#define SGET(x) getGlobalStringDict()->Get(x)

inline StringRef::StringRef(const char *str)
{
	StringRef ref = SGET(str);
	mString = ref.mString;
}


}; // end of namespace

#pragma warning(pop)

#endif
