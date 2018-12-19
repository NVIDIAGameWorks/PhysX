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



#ifndef MI_STRING_TABLE_H

#define MI_STRING_TABLE_H

#pragma warning( push )
#pragma warning( disable: 4555 ) // expression has no effect; expected expression with side-effect
#pragma warning( pop )

#include <assert.h>
#include "MiPlatformConfig.h"

//********** String Table
#pragma warning(push)
#pragma warning(disable:4996)

namespace mimp
{

class StringSort
{
public:
	bool operator()(const char *str1,const char *str2) const
	{
		return MESH_IMPORT_STRING::stricmp(str1,str2) < 0;
	}
};

typedef STDNAME::map< const char *, const char *, StringSort> StringMap;

class StringTable : public mimp::MeshImportAllocated
{
public:

	StringTable(void)
	{
	};

	~StringTable(void)
	{
		release();
	}

	void release(void)
	{
		for (StringMap::iterator i=mStrings.begin(); i!=mStrings.end(); ++i)
        {
            const char *string = (*i).second;
			MI_FREE( (void *)string );
        }
        mStrings.clear();
	}

	const char * Get(const char *str,bool &first)
	{
		str = str ? str : "";
		const char *ret;
		StringMap::iterator found = mStrings.find(str);
		if ( found == mStrings.end() )
		{
			MiU32 slen = (MiU32)strlen(str);
			char *string = (char *)MI_ALLOC(slen+1);
			strcpy(string,str);
			mStrings[string] = string;
			ret = string;
			first = true;
		}
		else
		{
			first = false;
			ret = (*found).second;
		}
        return ret;
	};


private:
    StringMap       mStrings;
//	char				mScratch[512];
};

class StringTableInt
{
public:
	typedef STDNAME::map< size_t, MiU32 > StringIntMap;

	bool Get(const char *str,MiU32 &v)
	{
		bool ret = false;
		bool first;
		str = mStringTable.Get(str,first);
		size_t index = (size_t)str;

		StringIntMap::iterator found = mStringInt.find(index);
		if ( found != mStringInt.end() )
		{
			v = (*found).second;
			ret = true;
		}
		return ret;
	}

	MiU32 Get(const char *str)
	{
		MiU32 ret=0;
		Get(str,ret);
		return ret;
	}

	void Add(const char *str,MiU32 v)
	{
		bool first;
		str = mStringTable.Get(str,first);
		size_t index = (size_t)str;
		StringIntMap::iterator found = mStringInt.find(index);
		if ( found != mStringInt.end() )
		{
			assert(0);
		}
		else
		{
			mStringInt[index] = v;
		}
	}

private:
	StringTable		mStringTable;
	StringIntMap	mStringInt;
};

}; // end of namespace

#pragma warning(pop)

#endif
