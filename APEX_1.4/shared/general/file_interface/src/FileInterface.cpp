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


#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "PxAssert.h"
#include "PsUserAllocated.h"
#include "PsString.h"
#include "ApexUsingNamespace.h"
#pragma warning(disable:4267)

/*
 * Copyright 2009-2011 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */


//********************************************************************************************
//***
//*** A wrapper interface for standard FILE IO services that provides support to read and
//** write 'files' to and from a buffer in memory.
//***
//********************************************************************************************


#include "FileInterface.h"

#pragma warning(disable:4996) // Disabling stupid .NET deprecated warning.

#define DEFAULT_BUFFER_SIZE 8192
#define BUFFER_GROW_SIZE    1000000 // grow in 1 MB chunks

namespace nvidia
{
	namespace general_file_interface2
	{

class MemoryBlock : public shdfnd::UserAllocated
{
public:
  MemoryBlock(size_t size)
  {
    mNextBlock = 0;
	mMemory    = (char *)PX_ALLOC(size, PX_DEBUG_EXP("FileInterface::MemoryBlock"));
    mSize      = size;
    mLen       = 0;
  }

  ~MemoryBlock(void)
  {
    PX_FREE(mMemory);
  }

  const char * write(const char *mem,size_t len,size_t &remaining)
  {
    const char *ret = 0;

    if ( (len+mLen) <= mSize )
    {
      char *dest = &mMemory[mLen];
      memcpy(dest,mem,len);
      mLen+=len;
	  remaining = 0;
    }
    else
    {
      uint32_t slen = mSize-mLen;
      if ( slen )
      {
        char *dest = &mMemory[mLen];
        memcpy(dest,mem,slen);
        mLen+=slen;
      }
      ret = mem+slen;
      remaining = len-slen;
      PX_ASSERT( remaining != 0 );
    }
    return ret;
  }

  char * getData(char *dest)
  {
    memcpy(dest,mMemory,mLen);
    dest+=mLen;
    return dest;
  }

  MemoryBlock    *mNextBlock;
  char           *mMemory;
  uint32_t    mLen;
  uint32_t    mSize;

};

class _FILE_INTERFACE : public shdfnd::UserAllocated
{
public:
	_FILE_INTERFACE(const char *fname,const char *spec,void *mem,size_t len)
	{
    mHeadBlock = 0;
    mTailBlock = 0;
		mMyAlloc   = false;
		mRead      = true; // default is read access.
		mFph       = 0;
		mData      = (char *) mem;
		mLen       = len;
		mLoc       = 0;

		if ( spec && shdfnd::stricmp(spec,"wmem") == 0 )
		{
			mRead = false;
			if ( mem == 0 || len == 0 )
			{
				mHeadBlock = PX_NEW(MemoryBlock)(DEFAULT_BUFFER_SIZE);
				mTailBlock = mHeadBlock;
				mData = 0;
				mLen  = 0;
				mMyAlloc = true;
			}
		}

		if ( mData == 0 && mHeadBlock == 0 )
		{
			mFph = fopen(fname,spec);
		}

  	strncpy(mName,fname,512);
	}

  ~_FILE_INTERFACE(void)
  {
  	if ( mMyAlloc )
  	{
  		PX_FREE(mData);

      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        MemoryBlock *next = mb->mNextBlock;
        delete mb;
        mb = next;
      }

  	}
  	if ( mFph )
  	{
  		fclose(mFph);
  	}
  }

  size_t read(char *data,size_t size)
  {
  	size_t ret = 0;
  	if ( (mLoc+size) <= mLen )
  	{
  		memcpy(data, &mData[mLoc], size );
  		mLoc+=size;
  		ret = 1;
  	}
    return ret;
  }

  void validateLen(void)
  {
    if ( mHeadBlock )
    {
      uint32_t slen = 0;

      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        slen+=mb->mLen;
        mb = mb->mNextBlock;
      }
      PX_ASSERT( slen == mLoc );
    }
  }

  size_t write(const char *data,size_t size)
  {
  	size_t ret = 0;

    if ( mMyAlloc )
    {
#ifdef _DEBUG
      validateLen();
#endif
      size_t remaining;
      data = mTailBlock->write(data,size,remaining);
      while ( data )
      {
        size_t _size = remaining;
        MemoryBlock *block = PX_NEW(MemoryBlock)(BUFFER_GROW_SIZE);
        mTailBlock->mNextBlock = block;
        mTailBlock = block;
        data = mTailBlock->write(data,_size,remaining);
      }
      mLoc+=size;
#ifdef _DEBUG
      validateLen();
#endif
      ret = 1;
    }
    else
    {
    	if ( (mLoc+size) <= mLen )
    	{
    		memcpy(&mData[mLoc],data,size);
    		mLoc+=size;
    		ret = 1;
    	}
    }
  	return ret;
  }

	size_t read(void *buffer,size_t size,size_t count)
	{
		size_t ret = 0;
		if ( mFph )
		{
			ret = fread(buffer,size,count,mFph);
		}
		else
		{
			char *data = (char *)buffer;
			for (size_t i=0; i<count; i++)
			{
				if ( (mLoc+size) <= mLen )
				{
					read(data,size);
					data+=size;
					ret++;
				}
				else
				{
					break;
				}
			}
		}
		return ret;
	}

  size_t write(const void *buffer,size_t size,size_t count)
  {
  	size_t ret = 0;

  	if ( mFph )
  	{
  		ret = fwrite(buffer,size,count,mFph);
  	}
  	else
  	{
  		const char *data = (const char *)buffer;
  		for (size_t i=0; i<count; i++)
  		{
    		if ( write(data,size) )
				{
    			data+=size;
    			ret++;
    		}
    		else
    		{
    			break;
    		}
  		}
  	}
  	return ret;
  }

  size_t writeString(const char *str)
  {
  	size_t ret = 0;
  	if ( str )
  	{
  		size_t len = strlen(str);
  		ret = write(str,len, 1 );
  	}
  	return ret;
  }


  size_t  flush(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fflush(mFph);
  	}
  	return ret;
  }


  size_t seek(size_t loc,size_t mode)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fseek(mFph,loc,mode);
  	}
  	else
  	{
  		if ( mode == SEEK_SET )
  		{
  			if ( loc <= mLen )
  			{
  				mLoc = loc;
  				ret = 1;
  			}
  		}
  		else if ( mode == SEEK_END )
  		{
  			mLoc = mLen;
  		}
  		else
  		{
  			PX_ALWAYS_ASSERT();
  		}
  	}
  	return ret;
  }

  size_t tell(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = ftell(mFph);
  	}
  	else
  	{
  		ret = mLoc;
  	}
  	return ret;
  }

  size_t myputc(char c)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = fputc(c,mFph);
  	}
  	else
  	{
  		ret = write(&c,1);
  	}
  	return ret;
  }

  size_t eof(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = feof(mFph);
  	}
  	else
  	{
  		if ( mLoc >= mLen )
  			ret = 1;
  	}
  	return ret;
  }

  size_t  error(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = ferror(mFph);
  	}
  	return ret;
  }

  void * getMemBuffer(size_t &outputLength)
  {
    outputLength = mLoc;

    if ( mHeadBlock && mLoc > 0 )
    {
      PX_ASSERT(mData==0);
      mData = (char *)PX_ALLOC(mLoc, PX_DEBUG_EXP("FileInterface::MemoryBuffer"));
      char *dest = mData;
      MemoryBlock *mb = mHeadBlock;
      while ( mb )
      {
        dest = mb->getData(dest);
        MemoryBlock *next = mb->mNextBlock;
        delete mb;
        mb = next;
      }

      mHeadBlock = 0;
      mTailBlock = 0;
    }
    return mData;
  }

  void  myclearerr(void)
  {
    if ( mFph )
    {
      clearerr(mFph);
    }
  }

  FILE 	            *mFph;
  char              *mData;
  size_t             mLen;
  size_t             mLoc;
  bool               mRead;
	char               mName[512];
	bool               mMyAlloc;
  MemoryBlock       *mHeadBlock;
  MemoryBlock       *mTailBlock;

};

FILE_INTERFACE * fi_fopen(const char *fname,const char *spec,void *mem,size_t len)
{
	_FILE_INTERFACE *ret = 0;

	ret = PX_NEW(_FILE_INTERFACE)(fname,spec,mem,len);

	if ( mem == 0 && ret->mData == 0 && ret->mHeadBlock == 0 )
  {
  	if ( ret->mFph == 0 )
  	{
      delete ret;
  		ret = 0;
  	}
  }

	return (FILE_INTERFACE *)ret;
}

size_t  fi_fclose(FILE_INTERFACE *_file)
{
  size_t ret = 0;

  if ( _file )
  {
    _FILE_INTERFACE *file = (_FILE_INTERFACE *)_file;
    delete file;
  }
  return ret;
}

void  fi_clearerr(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
  if ( fph )
  {
    fph->myclearerr();
  }
}

size_t        fi_fread(void *buffer,size_t size,size_t count,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->read(buffer,size,count);
	}
	return ret;
}

size_t        fi_fwrite(const void *buffer,size_t size,size_t count,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->write(buffer,size,count);
	}
	return ret;
}

size_t        fi_fprintf(FILE_INTERFACE *_fph,const char *fmt,...)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;

	char buffer[2048];
	buffer[2047] = 0;
	va_list arg;
	va_start( arg, fmt );
	shdfnd::vsnprintf(buffer,2047, fmt, arg);
	va_end(arg);

	if ( fph )
	{
		ret = fph->writeString(buffer);
	}

	return ret;
}


size_t        fi_fflush(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->flush();
	}
	return ret;
}


size_t        fi_fseek(FILE_INTERFACE *_fph,size_t loc,size_t mode)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->seek(loc,mode);
	}
	return ret;
}

size_t        fi_ftell(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->tell();
	}
	return ret;
}

size_t        fi_fputc(char c,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->myputc(c);
	}
	return ret;
}

size_t        fi_fputs(const char *str,FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->writeString(str);
	}
	return ret;
}

size_t        fi_feof(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->eof();
	}
	return ret;
}

size_t        fi_ferror(FILE_INTERFACE *_fph)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->error();
	}
	return ret;
}

void *     fi_getMemBuffer(FILE_INTERFACE *_fph,size_t *outputLength)
{
  _FILE_INTERFACE *fph = (_FILE_INTERFACE *)_fph;
	*outputLength = 0;
	void * ret = 0;
	if ( fph && outputLength )
	{
    ret = fph->getMemBuffer(*outputLength);
	}
	return ret;
}

}; // end of namespace
};

