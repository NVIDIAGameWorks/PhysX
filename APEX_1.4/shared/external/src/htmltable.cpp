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



#include "htmltable.h"

#if PX_WINDOWS_FAMILY // only compile this source code for windows!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string>
#include <vector>

#include <direct.h>

#pragma warning(disable:4267 4127)

#define USE_CPP 0
#define USE_EXCEL 0

#pragma warning(disable:4996 4702) // Disable Microsof'ts freaking idiotic 'warnings' not to use standard ANSI C stdlib and string functions!

#include "htmltable.h"

#if PX_LINUX
#define stricmp(a,b) strcasecmp(a,b)
#define _vsnprintf vsnprintf
#define _mkdir(a) mkdir(a, 0)
#endif

namespace HTMLTABLE_NVSHARE
{

using namespace nvidia;

#pragma warning(disable:4100)

class MemHeader
{
public:
  MemHeader  *mNext;
  MemHeader  *mPrevious;
  size_t      mLength;
  const char *mTag;
  const char *mFile;
  int         mLineno;
};

class MemTracker
{
public:
  MemTracker(void)
  {
    mCount = 0;
    mTotal = 0;
    mRoot  = 0;
  }

  ~MemTracker(void)
  {
  }

  void * memAlloc(size_t len,const char *tag,const char *file,int lineno)
  {
    MemHeader *mh = (MemHeader *)::malloc(len+sizeof(MemHeader));
    mh->mNext = mRoot;
    mh->mPrevious = 0;
    if ( mRoot ) mRoot->mPrevious = mh;
    mRoot = mh;
    mh->mLength = len;
    mh->mTag    = tag;
    mh->mFile   = file;
    mh->mLineno = lineno;
    mCount++;
    mTotal+=len;
    mh++;
    return mh;
  }

  void memFree(void *mem)
  {
    MemHeader *mh = (MemHeader *)mem;
    mh--;

    MemHeader *prev = mh->mPrevious;
    MemHeader *next = mh->mNext;

    if ( prev )
    {
      prev->mNext = next;
    }
    else
    {
      assert( mRoot == mh );
      mRoot = next;
    }
    if ( next )
    {
      next->mPrevious = prev;
    }
    mCount--;
    mTotal-=mh->mLength;

    assert(mCount>=0);
    assert(mTotal>=0);

    ::free(mh);
  }

  int getMemoryUsage(void)
  {
    int c = 0;
    int t = 0;

    MemHeader *mh = mRoot;
    MemHeader *prev = 0;
    while ( mh )
    {
      c++;
      t+=mh->mLength;
      assert( mh->mPrevious == prev );
      prev = mh;
      mh = mh->mNext;
    }

    assert( c == mCount );
    assert( t == mTotal );

    return mTotal;
  }

private:
  int        mCount;
  int        mTotal;
  MemHeader *mRoot;
};

static MemTracker gMemTracker;

#define HTML_NEW(x) new ( gMemTracker.memAlloc(sizeof(x),#x,__FILE__,__LINE__) )x
#define HTML_DELETE(y,x) if ( x ) { x->~y(); gMemTracker.memFree(x); }

#define HTML_MALLOC(x) gMemTracker.memAlloc(x,__FILE__,__FILE__,__LINE__)
#define HTML_FREE(x) gMemTracker.memFree(x)


static char *         lastDot(char *src)
{
  char *ret = 0;

  char *dot = strchr(src,'.');
  while ( dot )
  {
    ret = dot;
    dot = strchr(dot+1,'.');
  }
  return ret;
}


static char *  lastSlash(char *src) // last forward or backward slash character, null if none found.
{
  char *ret = 0;

  char *dot = strchr(src,'\\');
  if  ( dot == 0 )
    dot = strchr(src,'/');
  while ( dot )
  {
    ret = dot;
    dot = strchr(ret+1,'\\');
    if ( dot == 0 )
      dot = strchr(ret+1,'/');
  }
  return ret;
}

static inline const char * tf(bool v)
{
  const char *ret = "false";
  if ( v ) ret = "true";
  return ret;
}

class QuickSortPointers
{
public:
	void qsort(void **base,int num); // perform the qsort.
protected:
  // -1 less, 0 equal, +1 greater.
	virtual int compare(void **p1,void **p2) = 0;
private:
	void inline swap(char **a,char **b);
};


void QuickSortPointers::swap(char **a,char **b)
{
	char *tmp;

	if ( a != b )
	{
		tmp = *a;
		*a++ = *b;
		*b++ = tmp;
	}
}


void QuickSortPointers::qsort(void **b,int num)
{
	char *lo,*hi;
	char *mid;
	char *bottom, *top;
	int size;
	char *lostk[30], *histk[30];
	int stkptr;
	char **base = (char **)b;

	if (num < 2 ) return;

	stkptr = 0;

	lo = (char *)base;
	hi = (char *)base + sizeof(char **) * (num-1);

nextone:

	size = (int)(hi - lo) / (int)sizeof(char**) + 1;

	mid = lo + (size / 2) * sizeof(char **);
	swap((char **)mid,(char **)lo);
	bottom = lo;
	top = hi + sizeof(char **);

	for (;;)
	{
		do
		{
			bottom += sizeof(char **);
		} while (bottom <= hi && compare((void **)bottom,(void **)lo) <= 0);

		do
		{
			top -= sizeof(char **);
		} while (top > lo && compare((void **)top,(void **)lo) >= 0);

		if (top < bottom) break;

		swap((char **)bottom,(char **)top);

	}

	swap((char **)lo,(char **)top);

	if ( top - 1 - lo >= hi - bottom )
	{
		if (lo + sizeof(char **) < top)
		{
			lostk[stkptr] = lo;
			histk[stkptr] = top - sizeof(char **);
			stkptr++;
		}
		if (bottom < hi)
		{
			lo = bottom;
			goto nextone;
		}
	}
	else
	{
		if ( bottom < hi )
		{
			lostk[stkptr] = bottom;
			histk[stkptr] = hi;
			stkptr++;
		}
		if (lo + sizeof(char **) < top)
		{
			hi = top - sizeof(char **);
			goto nextone; 					/* do small recursion */
		}
	}

	stkptr--;

	if (stkptr >= 0)
	{
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto nextone;
	}
	return;
}


//*** Including my 'FILE_INTERFACE' wrapper that supposed an STDIO style interface to read and write buffers.
class FILE_INTERFACE;


FILE_INTERFACE * fi_fopen(const char *fname,const char *spec,void *mem=0,size_t len=0);
void             fi_fclose(FILE_INTERFACE *file);
size_t           fi_fread(void *buffer,size_t size,size_t count,FILE_INTERFACE *fph);
size_t           fi_fwrite(const void *buffer,size_t size,size_t count,FILE_INTERFACE *fph);
size_t           fi_fprintf(FILE_INTERFACE *fph,const char *fmt,...);
size_t           fi_fflush(FILE_INTERFACE *fph);
size_t           fi_fseek(FILE_INTERFACE *fph,size_t loc,size_t mode);
size_t           fi_ftell(FILE_INTERFACE *fph);
size_t           fi_fputc(char c,FILE_INTERFACE *fph);
size_t           fi_fputs(const char *str,FILE_INTERFACE *fph);
size_t           fi_feof(FILE_INTERFACE *fph);
size_t           fi_ferror(FILE_INTERFACE *fph);
void *           fi_getMemBuffer(FILE_INTERFACE *fph,size_t &outputLength);  // return the buffer and length of the file.


#define DEFAULT_BUFFER_SIZE 8192

#if defined(LINUX)
#   define _stricmp(a,b) strcasecmp((a),(b))                                                   
#endif

class FILE_INTERFACE
{
public:
	FILE_INTERFACE(const char *fname,const char *spec,void *mem,size_t len)
	{
		mMyAlloc = false;
		mRead = true; // default is read access.
		mFph = 0;
		mData = (char *) mem;
		mLen  = len;
		mLoc  = 0;

		if ( spec && _stricmp(spec,"wmem") == 0 )
		{
			mRead = false;
			if ( mem == 0 || len == 0 )
			{
				mData = (char *)HTML_MALLOC(DEFAULT_BUFFER_SIZE);
				mLen  = DEFAULT_BUFFER_SIZE;
				mMyAlloc = true;
			}
		}

		if ( mData == 0 )
		{
			mFph = fopen(fname,spec);
		}

  	strncpy(mName,fname,512);
	}

  ~FILE_INTERFACE(void)
  {
  	if ( mMyAlloc )
  	{
  		HTML_FREE(mData);
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

  size_t write(const char *data,size_t size)
  {
  	size_t ret = 0;

		if ( (mLoc+size) >= mLen && mMyAlloc ) // grow it
		{
			size_t newLen = mLen*2;
			if ( size > newLen ) newLen = size+mLen;

			char *data = (char *)HTML_MALLOC(newLen);
			memcpy(data,mData,mLoc);
      HTML_FREE(mData);
			mData = data;
			mLen  = newLen;
		}

  	if ( (mLoc+size) <= mLen )
  	{
  		memcpy(&mData[mLoc],data,size);
  		mLoc+=size;
  		ret = 1;
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
  		ret = (size_t)fflush(mFph);
  	}
  	return ret;
  }


  size_t seek(size_t loc,size_t mode)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = (size_t)fseek(mFph,(int)loc,(int)mode);
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
  			assert(0);
  		}
  	}
  	return ret;
  }

  size_t tell(void)
  {
  	size_t ret = 0;
  	if ( mFph )
  	{
  		ret = (size_t)ftell(mFph);
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
  		ret = (size_t)fputc(c,mFph);
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
  		ret = (size_t)feof(mFph);
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
  		ret = (size_t)ferror(mFph);
  	}
  	return ret;
  }


  FILE 	*mFph;
  char  *mData;
  size_t    mLen;
  size_t    mLoc;
  bool   mRead;
	char   mName[512];
	bool   mMyAlloc;

};

FILE_INTERFACE * fi_fopen(const char *fname,const char *spec,void *mem,size_t len)
{
	FILE_INTERFACE *ret = 0;

	ret = HTML_NEW(FILE_INTERFACE)(fname,spec,mem,len);

	if ( mem == 0 && ret->mData == 0)
  {
  	if ( ret->mFph == 0 )
  	{
      HTML_DELETE(FILE_INTERFACE,ret);
  		ret = 0;
  	}
  }

	return ret;
}

void       fi_fclose(FILE_INTERFACE *file)
{
  HTML_DELETE(FILE_INTERFACE,file);
}

size_t        fi_fread(void *buffer,size_t size,size_t count,FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->read(buffer,size,count);
	}
	return ret;
}

size_t        fi_fwrite(const void *buffer,size_t size,size_t count,FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->write(buffer,size,count);
	}
	return ret;
}

size_t        fi_fprintf(FILE_INTERFACE *fph,const char *fmt,...)
{
	size_t ret = 0;

	char buffer[2048];
  buffer[2047] = 0;
	_vsnprintf(buffer,2047, fmt, (char *)(&fmt+1));

	if ( fph )
	{
		ret = fph->writeString(buffer);
	}

	return ret;
}


size_t        fi_fflush(FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->flush();
	}
	return ret;
}


size_t        fi_fseek(FILE_INTERFACE *fph,size_t loc,size_t mode)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->seek(loc,mode);
	}
	return ret;
}

size_t        fi_ftell(FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->tell();
	}
	return ret;
}

size_t        fi_fputc(char c,FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->myputc(c);
	}
	return ret;
}

size_t        fi_fputs(const char *str,FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->writeString(str);
	}
	return ret;
}

size_t        fi_feof(FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->eof();
	}
	return ret;
}

size_t        fi_ferror(FILE_INTERFACE *fph)
{
	size_t ret = 0;
	if ( fph )
	{
		ret = fph->error();
	}
	return ret;
}

void *     fi_getMemBuffer(FILE_INTERFACE *fph,size_t &outputLength)
{
	outputLength = 0;
	void * ret = 0;
	if ( fph )
	{
		ret = fph->mData;
		outputLength = fph->mLoc;
	}
	return ret;
}


//**** Probably a little bit overkill, but I have just copy pasted my 'InPlaceParser' to handle the comma seperated value lines, since it handles quotated strings and whitspace automatically.

class InPlaceParserInterface
{
public:
	virtual int ParseLine(int lineno,int argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
  virtual bool preParseLine(int /*lineno*/,const char * /*line */)  { return false; }; // optional chance to pre-parse the line as raw data.  If you return 'true' the line will be skipped assuming you snarfed it.
};

enum SeparatorType
{
	ST_DATA,        // is data
	ST_HARD,        // is a hard separator
	ST_SOFT,        // is a soft separator
	ST_EOS,          // is a comment symbol, and everything past this character should be ignored
  ST_LINE_FEED
};

class InPlaceParser
{
public:
	InPlaceParser(void)
	{
		Init();
	}

	InPlaceParser(char *data,int len)
	{
		Init();
		SetSourceData(data,len);
	}

	InPlaceParser(const char *fname)
	{
		Init();
		SetFile(fname);
	}

	~InPlaceParser(void);

	void Init(void)
	{
		mQuoteChar = 34;
		mData = 0;
		mLen  = 0;
		mMyAlloc = false;
		for (int i=0; i<256; i++)
		{
			mHard[i] = ST_DATA;
			mHardString[i*2] = (char)i;
			mHardString[i*2+1] = 0;
		}
		mHard[0]  = ST_EOS;
		mHard[32] = ST_SOFT;
		mHard[9]  = ST_SOFT;
		mHard[13] = ST_LINE_FEED;
		mHard[10] = ST_LINE_FEED;
	}

	void SetFile(const char *fname);

	void SetSourceData(char *data,int len)
	{
		mData = data;
		mLen  = len;
		mMyAlloc = false;
	};

	int  Parse(const char *str,InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason
	int  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

	int ProcessLine(int lineno,char *line,InPlaceParserInterface *callback);

	const char ** GetArglist(char *source,int &count); // convert source string into an arg list, this is a destructive parse.

	void SetHardSeparator(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}

	void SetHard(char c) // add a hard separator
	{
		mHard[c] = ST_HARD;
	}

	void SetSoft(char c) // add a hard separator
	{
		mHard[c] = ST_SOFT;
	}


	void SetCommentSymbol(char c) // comment character, treated as 'end of string'
	{
		mHard[c] = ST_EOS;
	}

	void ClearHardSeparator(char c)
	{
		mHard[c] = ST_DATA;
	}


	void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

	bool EOS(char c)
	{
		if ( mHard[c] == ST_EOS )
		{
			return true;
		}
		return false;
	}

	void SetQuoteChar(char c)
	{
		mQuoteChar = c;
	}

	bool HasData( void ) const
	{
		return ( mData != 0 );
	}

  void setLineFeed(char c)
  {
    mHard[c] = ST_LINE_FEED;
  }

  bool isLineFeed(char c)
  {
    if ( mHard[c] == ST_LINE_FEED ) return true;
    return false;
  }

private:

	inline char * AddHard(int &argc,const char **argv,char *foo);
	inline bool   IsHard(char c);
	inline char * SkipSpaces(char *foo);
	inline bool   IsWhiteSpace(char c);
	inline bool   IsNonSeparator(char c); // non seperator,neither hard nor soft

	bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
	char  *mData;  // ascii data to parse.
	int    mLen;   // length of data
	SeparatorType  mHard[256];
	char   mHardString[256*2];
	char           mQuoteChar;
};

//==================================================================================
void InPlaceParser::SetFile(const char *fname)
{
	if ( mMyAlloc )
	{
		HTML_FREE(mData);
	}
	mData = 0;
	mLen  = 0;
	mMyAlloc = false;

	FILE *fph = fopen(fname,"rb");
	if ( fph )
	{
		fseek(fph,0L,SEEK_END);
		mLen = ftell(fph);
		fseek(fph,0L,SEEK_SET);

		if ( mLen )
		{
			mData = (char *) HTML_MALLOC(sizeof(char)*(mLen+1));
			int read = (int)fread(mData,(size_t)mLen,1,fph);
			if ( !read )
			{
				HTML_FREE(mData);
				mData = 0;
			}
			else
			{
				mData[mLen] = 0; // zero byte terminate end of file marker.
				mMyAlloc = true;
			}
		}
		fclose(fph);
	}
}

//==================================================================================
InPlaceParser::~InPlaceParser(void)
{
	if ( mMyAlloc )
	{
		HTML_FREE(mData);
	}
}

#define MAXARGS 512

//==================================================================================
bool InPlaceParser::IsHard(char c)
{
	return mHard[c] == ST_HARD;
}

//==================================================================================
char * InPlaceParser::AddHard(int &argc,const char **argv,char *foo)
{
	while ( IsHard(*foo) )
	{
		const char *hard = &mHardString[*foo*2];
		if ( argc < MAXARGS )
		{
			argv[argc++] = hard;
		}
		++foo;
	}
	return foo;
}

//==================================================================================
bool   InPlaceParser::IsWhiteSpace(char c)
{
	return mHard[c] == ST_SOFT;
}

//==================================================================================
char * InPlaceParser::SkipSpaces(char *foo)
{
	while ( !EOS(*foo) && IsWhiteSpace(*foo) ) 
		++foo;
	return foo;
}

//==================================================================================
bool InPlaceParser::IsNonSeparator(char c)
{
	return ( !IsHard(c) && !IsWhiteSpace(c) && c != 0 );
}

//==================================================================================
int InPlaceParser::ProcessLine(int lineno,char *line,InPlaceParserInterface *callback)
{
	int ret = 0;

	const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{
		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) ) 
			break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			++foo;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) 
				++foo;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				++foo;
			}
		}
		else
		{
			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					++foo;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar )
						++foo;
					if ( *foo )
						*foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						++foo;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						++foo;
						break;
					}
					++foo;
				} // end of while loop...
			}
		}
	}

	if ( argc )
	{
		ret = callback->ParseLine(lineno, argc, argv );
	}

	return ret;
}


int  InPlaceParser::Parse(const char *str,InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
{
  int ret = 0;

  mLen = (int)strlen(str);
  if ( mLen )
  {
    mData = (char *)HTML_MALLOC((size_t)mLen+1);
    strcpy(mData,str);
    mMyAlloc = true;
    ret = Parse(callback);
  }
  return ret;
}

//==================================================================================
// returns true if entire file was parsed, false if it aborted for some reason
//==================================================================================
int  InPlaceParser::Parse(InPlaceParserInterface *callback)
{
	int ret = 0;
	assert( callback );
	if ( mData )
	{
		int lineno = 0;

		char *foo   = mData;
		char *begin = foo;

		while ( *foo )
		{
			if ( isLineFeed(*foo) )
			{
				++lineno;
				*foo = 0;
				if ( *begin ) // if there is any data to parse at all...
				{
          bool snarfed = callback->preParseLine(lineno,begin);
          if ( !snarfed )
          {
  					int v = ProcessLine(lineno,begin,callback);
  					if ( v )
  						ret = v;
          }
				}

				++foo;
				if ( *foo == 10 )
					++foo; // skip line feed, if it is in the carraige-return line-feed format...
				begin = foo;
			}
			else
			{
				++foo;
			}
		}

		lineno++; // lasst line.

		int v = ProcessLine(lineno,begin,callback);
		if ( v )
			ret = v;
	}
	return ret;
}

//==================================================================================
void InPlaceParser::DefaultSymbols(void)
{
	SetHardSeparator(',');
	SetHardSeparator('(');
	SetHardSeparator(')');
	SetHardSeparator('=');
	SetHardSeparator('[');
	SetHardSeparator(']');
	SetHardSeparator('{');
	SetHardSeparator('}');
	SetCommentSymbol('#');
}

//==================================================================================
// convert source string into an arg list, this is a destructive parse.
//==================================================================================
const char ** InPlaceParser::GetArglist(char *line,int &count)
{
	const char **ret = 0;

	static const char *argv[MAXARGS];
	int argc = 0;

	char *foo = line;

	while ( !EOS(*foo) && argc < MAXARGS )
	{
		foo = SkipSpaces(foo); // skip any leading spaces

		if ( EOS(*foo) )
			break;

		if ( *foo == mQuoteChar ) // if it is an open quote
		{
			++foo;
			if ( argc < MAXARGS )
			{
				argv[argc++] = foo;
			}
			while ( !EOS(*foo) && *foo != mQuoteChar ) 
				++foo;
			if ( !EOS(*foo) )
			{
				*foo = 0; // replace close quote with zero byte EOS
				++foo;
			}
		}
		else
		{
			foo = AddHard(argc,argv,foo); // add any hard separators, skip any spaces

			if ( IsNonSeparator(*foo) )  // add non-hard argument.
			{
				bool quote  = false;
				if ( *foo == mQuoteChar )
				{
					++foo;
					quote = true;
				}

				if ( argc < MAXARGS )
				{
					argv[argc++] = foo;
				}

				if ( quote )
				{
					while (*foo && *foo != mQuoteChar ) 
						++foo;
					if ( *foo ) 
						*foo = 32;
				}

				// continue..until we hit an eos ..
				while ( !EOS(*foo) ) // until we hit EOS
				{
					if ( IsWhiteSpace(*foo) ) // if we hit a space, stomp a zero byte, and exit
					{
						*foo = 0;
						++foo;
						break;
					}
					else if ( IsHard(*foo) ) // if we hit a hard separator, stomp a zero byte and store the hard separator argument
					{
						const char *hard = &mHardString[*foo*2];
						*foo = 0;
						if ( argc < MAXARGS )
						{
							argv[argc++] = hard;
						}
						++foo;
						break;
					}
					++foo;
				} // end of while loop...
			}
		}
	}

	count = argc;
	if ( argc )
	{
		ret = argv;
	}

	return ret;
}

static bool numeric(char c)
{
  bool ret = false;

  if ( (c >= '0' && c <= '9' ) || c == ',' || c == '.' || c == 32)
  {
    ret = true;
  }
  return ret;
}

static bool isNumeric(const std::string &str)
{
  bool ret = true;

  if ( str.size() == 0 )
  {
    ret = false;
  }
  else
  {
    const char *scan = str.c_str();
	if ( *scan == '-' ) scan++;
    while ( *scan )
    {
      if ( !numeric(*scan) )
      {
        ret = false;
        break;
      }
      scan++;
    }
  }
  return ret;
}

#define MAXNUMERIC 32  // JWR  support up to 16 32 character long numeric formated strings
#define MAXFNUM    16

static	char  gFormat[MAXNUMERIC*MAXFNUM];
static int    gIndex=0;

const char * formatNumber(int number) // JWR  format this integer into a fancy comma delimited string
{
	char * dest = &gFormat[gIndex*MAXNUMERIC];
	gIndex++;
	if ( gIndex == MAXFNUM ) gIndex = 0;

	char scratch[512];

#if defined (LINUX_GENERIC) || defined(LINUX) || defined (__CELLOS_LV2__)
	snprintf(scratch, 10, "%d", number);
#else
	itoa(number,scratch,10);
#endif

	char *source = scratch;
	char *str = dest;
	unsigned int len = (unsigned int)strlen(scratch);
	if ( scratch[0] == '-' )
	{
		*str++ = '-';
		source++;
		len--;
	}
	for (unsigned int i=0; i<len; i++)
	{
		int place = ((int)len-1)-(int)i;
		*str++ = source[i];
		if ( place && (place%3) == 0 ) *str++ = ',';
	}
	*str = 0;

	return dest;
}

void stripFraction(char *fraction)
{
  size_t len = strlen(fraction);
  if ( len > 0 )
  {
    len--;
    while ( len )
    {
      if ( fraction[len] == '0' )
      {
        fraction[len] = 0;
        len--;
      }
      else
      {
        break;
      }
    }
  }
}

float getFloatValue(const char *data)
{
  char temp[512];
  char *dest = temp;

  while ( *data )
  {
    char c = *data++;
    if ( c != ',' )
    {
      *dest++ = c;
    }
  }
  *dest = 0;
  float v = (float)atof(temp);
  return v;
}

void getFloat(float v,std::string &ret)
{
  int ivalue = (int)v;

  if ( v == 0 )
  {
    ret = "0";
  }
  else if ( v == 1 )
  {
    ret = "1";
  }
  else if ( v == -1 )
  {
    ret = "-1";
  }
  else if ( ivalue == 0 )
  {
    char fraction[512];
    sprintf(fraction,"%0.9f", v );
    stripFraction(fraction);
    ret = fraction;
  }
  else
  {
    v-=(float)ivalue;
    v = fabsf(v);
    if (v < 0.00001f ) 
      v = 0;


    const char *temp = formatNumber(ivalue);
    if ( v != 0 )
    {
      char fraction[512];
      sprintf(fraction,"%0.9f", v );
      assert( fraction[0] == '0' );
      assert( fraction[1] == '.' );
      stripFraction(fraction);
      char scratch[512];
      sprintf(scratch,"%s%s", temp, &fraction[1] );
      ret = scratch;
    }
    else
    {
      ret = temp;
    }
  }
}

class SortRequest
{
public:
  SortRequest(void)
  {

  }
  SortRequest(const char *sort_name,unsigned int primary_key,bool primary_ascending,unsigned int secondary_key,bool secondary_ascending)
  {
    if ( sort_name )
      mSortName = sort_name;

    mPrimaryKey          = primary_key;
    mPrimaryAscending    = primary_ascending;
    mSecondaryKey        = secondary_key;
    mSecondaryAscending  = secondary_ascending;

  }


  std::string           mSortName;
  unsigned int          mPrimaryKey;
  unsigned int          mSecondaryKey;
  bool                  mPrimaryAscending:1;
  bool                  mSecondaryAscending:1;
};

typedef std::vector< SortRequest > SortRequestVector;

typedef std::vector< std::string >  StringVector;
typedef std::vector< size_t > SizetVector;

class HtmlRow
{
public:
  HtmlRow(void)
  {
    mHeader = false;
    mFooter = false;
  }
  ~HtmlRow(void)
  {
  }

  void setFooter(bool state)
  {
    mFooter = state;
  }

  bool isFooter(void) const { return mFooter; };

  void setHeader(bool state)
  {
    mHeader = state;
  }

  bool isHeader(void) const { return mHeader; };

  void clear(void)
  {
    mRow.clear();
  }

  void addCSV(const char *data,InPlaceParser &parser)
  {
    if ( data )
    {
      size_t len = strlen(data);
      if ( len )
      {
        char *temp = (char *)HTML_MALLOC(sizeof(char)*(len+1));
        memcpy(temp,data,len+1);
        int count;
        const char **args = parser.GetArglist(temp,count);
        if ( args )
        {
          for (int i=0; i<count; i++)
          {
            const char *arg = args[i];
            if ( arg[0] != ',' )
            {
              addColumn(arg);
            }
          }
        }
        HTML_FREE(temp);
      }
    }
  }

  void addColumn(const char *data)
  {
    if ( data )
    {
		std::string str = data;
      if ( isNumeric(data) )
      {
        float v = getFloatValue(data);
        getFloat(v,str);
      }
      mRow.push_back(str);
    }
  }

  void columnSizes(SizetVector &csizes)
  {
    size_t ccount = csizes.size();
    size_t count  = mRow.size();
    for (size_t i=ccount; i<count; i++)
    {
      csizes.push_back(0);
    }
    for (size_t i=0; i<count; i++)
    {
      if ( mRow[i].size() > csizes[i] )
      {
        csizes[i] = mRow[i].size();
      }
    }
  }

  void getString(size_t index,std::string &str) const
  {
    if ( index < mRow.size() )
    {
      str = mRow[index];
    }
    else
    {
      str.clear();
    }
  }

  void htmlRow(FILE_INTERFACE *fph,HtmlTable *table)
  {
    {
      fi_fprintf(fph,"<TR>");

      unsigned int column = 1;

      StringVector::iterator i;
      for (i=mRow.begin(); i!=mRow.end(); ++i)
      {

        unsigned int color = table->getColor(column,mHeader,mFooter);

        const char *str = (*i).c_str();

        if ( mHeader )
        {
          fi_fprintf(fph,"<TH bgcolor=\"#%06X\"> %s </TH>", color, str );
        }
        else if ( mFooter )
        {
          if ( isNumeric(str))
          {
            fi_fprintf(fph,"<TH bgcolor=\"#%06X\" align=\"right\"> %s</TH>", color, str );
          }
          else
          {
            fi_fprintf(fph,"<TH bgcolor=\"#%06X\" align=\"left\">%s </TH>", color, str );
          }
        }
        else
        {
          if ( isNumeric(str))
          {
            fi_fprintf(fph,"<TD bgcolor=\"#%06X\" align=\"right\"> %s</TD>", color, str );
          }
          else
          {
            fi_fprintf(fph,"<TD bgcolor=\"#%06X\" align=\"left\">%s </TD>", color, str );
          }
        }

        column++;
      }

      fi_fprintf(fph,"</TR>\r\n");
    }
  }

  void saveExcel(FILE *fph,HtmlTable *table)
  {
    {
      fprintf(fph,"<TR>");

      unsigned int column = 1;

      StringVector::iterator i;
      for (i=mRow.begin(); i!=mRow.end(); ++i)
      {

        unsigned int color = table->getColor(column,mHeader,mFooter);

        const char *str = (*i).c_str();

        if ( mHeader )
        {
          fprintf(fph,"<TH bgcolor=\"#%06X\"> %s </TH>", color, str );
        }
        else if ( mFooter )
        {
          if ( isNumeric(str))
          {
            fprintf(fph,"<TH bgcolor=\"#%06X\" align=\"right\"> %s</TH>", color, str );
          }
          else
          {
            fprintf(fph,"<TH bgcolor=\"#%06X\" align=\"left\">%s </TH>", color, str );
          }
        }
        else
        {
          if ( isNumeric(str))
          {
            fprintf(fph,"<TD bgcolor=\"#%06X\" align=\"right\"> %s</TD>", color, str );
          }
          else
          {
            fprintf(fph,"<TD bgcolor=\"#%06X\" align=\"left\">%s </TD>", color, str );
          }
        }

        column++;
      }

      fprintf(fph,"</TR>\r\n");
    }
  }

  void saveCSV(FILE_INTERFACE *fph)
  {
    size_t count = mRow.size();
    for (size_t i=0; i<count; i++)
    {
      const char *data = mRow[i].c_str();
      fi_fprintf(fph,"\"%s\"", data );
      if ( (i+1) < count )
      {
        fi_fprintf(fph,",");
      }
    }
    fi_fprintf(fph,"\r\n");
  }

  void saveCPP(FILE_INTERFACE *fph)
  {
    if ( mHeader )
    {
      fi_fprintf(fph,"    table->addHeader(%c%c%c%c,%c", 34, '%', 's', 34, 34, 34);
    }
    else
    {
      fi_fprintf(fph,"    table->addCSV(%c%c%c%c,%c", 34, '%', 's', 34, 34, 34 );
    }

    size_t count = mRow.size();
    for (size_t i=0; i<count; i++)
    {
      const char *data = mRow[i].c_str();

      bool needQuote = false;
      bool isNumeric = true;

	  std::string str;
      while ( *data )
      {
        char c = *data++;

        if ( !numeric(c) )
        {
          isNumeric = false;
          if ( c == ',' )
            needQuote = true;
        }


        if ( c == 34 )
        {
          str.push_back('\\');
          str.push_back(34);
        }
        else if ( c == '\\' )
        {
          str.push_back('\\');
          str.push_back('\\');
        }
        else
        {
          str.push_back(c);
        }
      }

      if ( isNumeric )
      {
        const char *data = mRow[i].c_str();
        str.clear();
        while ( *data )
        {
          char c = *data++;
          if ( c != ',' )
          {
            str.push_back(c);
          }
        }
      }
      if ( needQuote )
      {
        fi_fprintf(fph,"%c%c%s%c%c", '\\', 34, str.c_str(), '\\', 34 );
      }
      else
      {
        fi_fprintf(fph,"%s", str.c_str() );
      }
      if ( (i+1) < count )
      {
        fi_fprintf(fph,",");
      }
    }
    fi_fprintf(fph,"%c);\r\n",34);
  }

  int compare(const HtmlRow &r,const SortRequest &s)
  {
    int ret = 0;

	std::string p1;  // primary 1
	std::string p2;  // primary 2


    getString(s.mPrimaryKey-1,p1);

    r.getString(s.mPrimaryKey-1,p2);

    if (isNumeric(p1) && isNumeric(p2) )
    {
      float v1 = getFloatValue(p1.c_str());
      float v2 = getFloatValue(p2.c_str());
      if ( v1 < v2 )
        ret = -1;
      else if ( v1 > v2 )
        ret = 1;
    }
    else
    {
      ret = stricmp(p1.c_str(),p2.c_str());

      if ( ret < 0 )
        ret = -1;
      else if ( ret > 0 )
        ret = 1;

    }

    if ( !s.mPrimaryAscending )
    {
      ret*=-1;
    }

    if ( ret == 0 )
    {
		std::string p1;  // secondary 1
		std::string p2;  // secondary 2
      getString(s.mSecondaryKey-1,p1);
      r.getString(s.mSecondaryKey-1,p2);
      if (isNumeric(p1) && isNumeric(p2) )
      {
        float v1 = getFloatValue(p1.c_str());
        float v2 = getFloatValue(p2.c_str());
        if ( v1 < v2 )
          ret = -1;
        else if ( v1 > v2 )
          ret = 1;
      }
      else
      {
        ret = stricmp(p1.c_str(),p2.c_str());

        if ( ret < 0 )
          ret = -1;
        else if ( ret > 0 )
          ret = 1;

      }

      if ( !s.mSecondaryAscending )
      {
        ret*=-1;
      }

    }

    return ret;
  }

private:
  bool          mHeader:1;
  bool          mFooter:1;
  StringVector  mRow;
};

typedef std::vector< HtmlRow * > HtmlRowVector;


static int gTableCount=0;

class _HtmlTable : public HtmlTable, public QuickSortPointers
{
public:
  _HtmlTable(const char *heading,HtmlDocument *parent);


  virtual ~_HtmlTable(void)
  {
//    gTableCount--;
//    printf("Destructed _HtmlTable(%08X) Count:%d\r\n", this, gTableCount );
    reset();
  }

  unsigned int getDisplayOrder(void) const { return mDisplayOrder; };


	int compare(void **p1,void **p2)
  {
    HtmlRow **r1 = (HtmlRow **)p1;
    HtmlRow **r2 = (HtmlRow **)p2;

    HtmlRow *row1 = r1[0];
    HtmlRow *row2 = r2[0];

    assert( !row1->isHeader() );
    assert( !row1->isFooter() );

    assert( !row2->isHeader() );
    assert( !row2->isFooter() );

    return row1->compare(*row2,mSortRequest);
  }

  void BorderASCII(void)
  {
    UPPER_LEFT_BORDER = '/';
    UPPER_RIGHT_BORDER = '\\';

    LOWER_LEFT_BORDER = '\\';
    LOWER_RIGHT_BORDER = '/';

    TOP_SEPARATOR = '-';
    BOTTOM_SEPARATOR = '-';
    TOP_BORDER = '-';
    BOTTOM_BORDER = '-';
    LEFT_BORDER = '|';
    RIGHT_BORDER = '|';
    VERTICAL_SEPARATOR = '|';
    LEFT_SIDE_BORDER = '|';
    RIGHT_SIDE_BORDER = '|';
    CROSS_BORDER = '-';
  }

  void BorderDOS(void)
  {
    UPPER_LEFT_BORDER = 201;
    UPPER_RIGHT_BORDER = 187;
    LOWER_LEFT_BORDER = 200;
    LOWER_RIGHT_BORDER = 188;
    TOP_SEPARATOR = 203;
    BOTTOM_SEPARATOR = 202;
    TOP_BORDER = 205;
    BOTTOM_BORDER = 205;
    LEFT_BORDER = 186;
    RIGHT_BORDER = 186;
    VERTICAL_SEPARATOR = 186;
    LEFT_SIDE_BORDER = 204;
    RIGHT_SIDE_BORDER = 185;
    CROSS_BORDER = 206;
  }

  void reset(void)
  {
    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      HTML_DELETE(HtmlRow,row);
    }
    mBody.clear();
    mExcludeTotals.clear();
    mCurrent = 0;
  }

  void addString(std::string &str,const char *data)
  {
    str.push_back(34);
    while ( *data )
    {
      str.push_back(*data);
      data++;
    }
    str.push_back(34);
  }

  void addHeader(const char *fmt,...)
  {
    char data[8192];
    data[8191] = 0;
  	_vsnprintf(data,8191, fmt, (char *)(&fmt+1));

    mParser.ClearHardSeparator(32);
    mParser.ClearHardSeparator(9);

    if ( strstr(data,"/") )
    {
		std::string sdata = data;

      while ( true )
      {
        size_t len = sdata.size();


		std::string header1;
		std::string header2;

        char *temp = (char *)HTML_MALLOC(sizeof(char)*(len+1));
        memcpy(temp,sdata.c_str(),len+1);
        int count;
        const char **args = mParser.GetArglist(temp,count);

        if ( args )
        {
          for (int i=0; i<count; i++)
          {
            const char *arg = args[i];
            if ( arg[0] == ',' )
            {
              header1.push_back(',');
              header2.push_back(',');
            }
            else
            {
              if ( strstr(arg,"/") )
              {
                header1.push_back(34);
                while ( *arg && *arg != '/' )
                {
                  header1.push_back(*arg);
                  arg++;
                }
                header1.push_back(34);

                header2.push_back(34);
                if ( *arg == '/'  )
                {
                  arg++;
                  while ( *arg )
                  {
                    header2.push_back(*arg);
                    arg++;
                  }
                }
                header2.push_back(34);
              }
              else
              {
                addString(header1,arg);
              }
            }
          }
        }
        HTML_FREE(temp);

        getCurrent();
        mCurrent->setHeader(true);
        mCurrent->addCSV(header1.c_str(),mParser);
        nextRow();

        if ( strstr(header2.c_str(),"/") )
        {
          sdata = header2; // now process header2...
        }
        else
        {
          getCurrent();
          mCurrent->setHeader(true);
          mCurrent->addCSV(header2.c_str(),mParser);
          nextRow();
          break;
        }
      }

    }
    else
    {
      getCurrent();
      mCurrent->setHeader(true);
      mCurrent->addCSV(data,mParser);
      nextRow();
    }
  }

  void addColumn(const char *data)
  {
    getCurrent();
    mCurrent->addColumn(data);
  }

  void addColumn(float v)
  {
	  std::string str;
    getFloat(v,str);
    addColumn(str.c_str());
  }

  void addColumnHex(unsigned int v)
  {
	  char scratch[512];
	  sprintf(scratch,"$%08X",v);
	  addColumn(scratch);
  }


  void addColumn(int v)
  {
    const char *temp = formatNumber(v);
    addColumn(temp);
  }

  void addColumn(unsigned int v)
  {
    const char *temp = formatNumber((int)v);
    addColumn(temp);
  }

  
  void addCSV(bool newRow,const char *fmt,...)
  {
    char data[8192];
    data[8191] = 0;
  	_vsnprintf(data,8191, fmt, (char *)(&fmt+1));

    getCurrent();
    mCurrent->addCSV(data,mParser);
    if ( newRow )
    {
      mCurrent = 0;
    }
  }

  void nextRow(void)
  {
    mCurrent = 0;
  }

  void getCurrent(void)
  {
    if ( mCurrent == 0 )
    {
      mCurrent = HTML_NEW(HtmlRow);
      mBody.push_back(mCurrent);
    }
  }


  void printLeft(FILE_INTERFACE *fph,const std::string &str,size_t width)
  {
    size_t swid = str.size();
    assert( swid <= width );

    size_t justify = (width-swid)-1;
    fi_fprintf(fph,"%c", 32 );
    fi_fprintf(fph,"%s", str.c_str() );
    for (size_t i=0; i<justify; i++)
    {
      fi_fprintf(fph,"%c", 32 );
    }
  }

  void printRight(FILE_INTERFACE *fph,const std::string &str,size_t width)
  {
    size_t swid = str.size();
    assert( swid <= width );
    size_t justify = (width-swid)-1;

    for (size_t i=0; i<justify; i++)
    {
      fi_fprintf(fph,"%c", 32 );
    }

    fi_fprintf(fph,"%s", str.c_str() );
    fi_fprintf(fph,"%c", 32 );

  }

  void printCenter(FILE_INTERFACE *fph,const std::string &str,size_t width)
  {
    size_t swid = str.size();
    if ( swid > width )
    {
      width = swid;
    }

    size_t count = 0;

    size_t center = (width-swid)/2;
    for (size_t i=0; i<center; i++)
    {
      fi_fprintf(fph,"%c", 32 );
      count++;
    }
    count+=str.size();
    fi_fprintf(fph,"%s", str.c_str() );
    for (size_t i=0; i<center; i++)
    {
      fi_fprintf(fph,"%c", 32 );
      count++;
    }
    if ( count < width )
    {
      assert( (count+1) == width );
      fi_fprintf(fph,"%c", 32 );
    }
  }

  void saveExcel(FILE *fph)
  {
    fprintf(fph,"<TABLE BORDER=\"1\">\r\n");
    fprintf(fph," <caption><EM>%s</EM></caption>\r\n", mHeading.c_str() );


    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      row->saveExcel(fph,this);
    }

    fprintf(fph,"</TABLE>\r\n");
    fprintf(fph,"<p></p>\r\n");
    fprintf(fph,"<p></p>\r\n");
    fprintf(fph,"<p></p>\r\n");
  }


  void saveSimpleHTML(FILE_INTERFACE *fph)
  {
    fi_fprintf(fph,"<TABLE BORDER=\"1\">\r\n");
    fi_fprintf(fph," <caption><EM>%s</EM></caption>\r\n", mHeading.c_str() );


    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      row->htmlRow(fph,this);
    }

    fi_fprintf(fph,"</TABLE>\r\n");
    fi_fprintf(fph,"<p></p>\r\n");
    fi_fprintf(fph,"<p></p>\r\n");
    fi_fprintf(fph,"<p></p>\r\n");
  }

  void saveCSV(FILE_INTERFACE *fph)
  {
    fi_fprintf(fph,"%s\r\n", mHeading.c_str() );
    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      row->saveCSV(fph);
    }
    fi_fprintf(fph,"\r\n");
  }

  void saveCPP(FILE_INTERFACE *fph)
  {
    fi_fprintf(fph,"  if ( 1 )\r\n");
    fi_fprintf(fph,"  {\r\n");
    fi_fprintf(fph,"    nvidia::HtmlTable *table = document->createHtmlTable(\"%s\");\r\n", mHeading.c_str() );
    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      row->saveCPP(fph);
    }

    if ( mComputeTotals )
    {
      for (size_t i=0; i<mExcludeTotals.size(); i++)
      {
        fi_fprintf(fph,"    table->excludeTotals(%d);\r\n", mExcludeTotals[i] );
      }
      fi_fprintf(fph,"    table->computeTotals();\r\n");
    }


    if ( !mSortRequests.empty() )
    {
      SortRequestVector::iterator i;
      for (i=mSortRequests.begin(); i!=mSortRequests.end(); ++i)
      {
        SortRequest &sr = (*i);
        fi_fprintf(fph,"    table->addSort(%c%s%c,%d,%s,%d,%s);\r\n", 34, sr.mSortName.c_str(), 34, sr.mPrimaryKey, tf(sr.mPrimaryAscending), sr.mSecondaryKey, tf(sr.mSecondaryAscending) );
      }
    }

    fi_fprintf(fph,"  }\r\n");
    fi_fprintf(fph,"\r\n");
  }

  void sortBody(const SortRequest &sr)
  {
    mSortRequest = sr;

    size_t rcount = mBody.size();
    int index  = 0;

    HtmlRow **rows = (HtmlRow **) HTML_MALLOC(sizeof(HtmlRow *)*rcount);
    size_t   *indices = (size_t *) HTML_MALLOC(sizeof(size_t)*rcount);


    for (size_t i=0; i<rcount; i++)
    {
      HtmlRow *row = mBody[i];
      if ( !row->isHeader() )
      {
        rows[index]    = row;
        indices[index] = i;
        index++;
      }
    }

    qsort( (void **)rows,index);

    for (int i=0; i<index; i++)
    {
      HtmlRow *row = rows[i];
      size_t   dest = indices[i];
      mBody[dest] = row;
    }

    HTML_FREE(rows);
    HTML_FREE(indices);

  }

  void save(FILE_INTERFACE *fph,HtmlSaveType type)
  {

    if ( mBody.size() >= 2 ) // must have at least one header row and one data row
    {

      if ( mSortRequests.size() && type != HST_CPP )
      {
        SortRequestVector::iterator i;
        for (i=mSortRequests.begin(); i!=mSortRequests.end(); i++)
        {
          sortBody( (*i) );
          saveInternal(fph,type,(*i).mSortName.c_str());
        }
      }
      else
      {
        saveInternal(fph,type,"");
      }
    }
  }

  void saveInternal(FILE_INTERFACE *fph,HtmlSaveType type,const char *secondary_caption)
  {
    bool totals = false;

    if ( mComputeTotals )
    {
      totals = addTotalsRow();
    }

    switch ( type )
    {
      case HST_SIMPLE_HTML:
        saveSimpleHTML(fph);
        break;
      case HST_CSV:
        saveCSV(fph);
        break;
      case HST_TEXT:
        BorderASCII();
        saveText(fph,secondary_caption);
        break;
      case HST_TEXT_EXTENDED:
        BorderDOS();
        saveText(fph,secondary_caption);
        break;
      case HST_CPP:
        saveCPP(fph);
        break;
      case HST_XML:
        break;
    }

    if ( totals )
    {
      removeTotalsRow();
    }
  }

  bool excluded(size_t c)
  {
    bool ret = false;

    for (size_t i=0; i<mExcludeTotals.size(); i++)
    {
      if ( c == mExcludeTotals[i] )
      {
        ret = true;
        break;
      }
    }

    return ret;
  }

  float computeTotal(size_t column)
  {
    float ret = 0;
    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      if ( !row->isHeader() )
      {
		  std::string str;
        row->getString(column,str);
        if ( isNumeric(str) )
        {
          float v = getFloatValue(str.c_str());
          ret+=v;
        }
      }
    }
    return ret;
  }

  bool addTotalsRow(void)
  {
    bool ret = false;

    if ( mBody.size() >= 2 )
    {
      HtmlRow *first_row = 0;

      SizetVector csize;
      HtmlRowVector::iterator i;
      for (i=mBody.begin(); i!=mBody.end(); i++)
      {
        HtmlRow *row = (*i);
        if ( !row->isHeader() && first_row == 0 )
        {
          first_row = row;
        }
        row->columnSizes(csize);
      }
      if ( first_row )
      {
        HtmlRow *totals = HTML_NEW(HtmlRow);
        totals->setFooter(true);
        size_t count = csize.size();
        for (size_t i=0; i<count; i++)
        {
          if ( !excluded(i+1) )
          {
			  std::string str;
            first_row->getString(i,str);
            if ( isNumeric(str) )
            {
              float v = computeTotal(i);
              std::string str;
              getFloat(v,str);
              totals->addColumn(str.c_str());
            }
            else
            {
              if ( i == 0 )
              {
                totals->addColumn("Totals");
              }
              else
              {
                totals->addColumn("");
              }
            }
            ret = true;
          }
          else
          {
            totals->addColumn("");
          }
        }
        if ( ret )
        {
          mBody.push_back(totals);
        }
        else
        {
          HTML_DELETE(HtmlRow,totals);
        }
      }
    }

    return ret;
  }

  void removeTotalsRow(void)
  {
    if ( mBody.size() )
    {
      size_t index = mBody.size()-1;
      HtmlRow *row = mBody[index];
      HTML_DELETE(HtmlRow,row);
      HtmlRowVector::iterator i = mBody.end();
      i--;
      mBody.erase(i);
    }
  }

  void saveText(FILE_INTERFACE *fph,const char *secondary_caption)
  {
    SizetVector csize;
    HtmlRowVector::iterator i;
    for (i=mBody.begin(); i!=mBody.end(); i++)
    {
      HtmlRow *row = (*i);
      row->columnSizes(csize);
    }

    size_t column_count = csize.size();
    size_t column_size  = 0;

    for (size_t i=0; i<column_count; i++)
    {
      csize[i]+=2;
      column_size+=csize[i];
    }


    fi_fprintf(fph," \r\n" );
    printCenter(fph,mHeading,column_size);
    fi_fprintf(fph," \r\n" );

    if ( secondary_caption && strlen(secondary_caption) > 0 )
    {
      printCenter(fph,secondary_caption,column_size);
      fi_fprintf(fph," \r\n" );
    }



    //*****************************************
    // Print the top border
    //*****************************************

    fi_fprintf(fph,"%c",UPPER_LEFT_BORDER );

    for (size_t i=0; i<column_count; i++)
    {
      size_t c = csize[i];
      for (size_t j=0; j<c; j++)
      {
        fi_fprintf(fph,"%c", TOP_BORDER );
      }
      if ( (i+1) < column_count )
      {
        fi_fprintf(fph,"%c", TOP_SEPARATOR );
      }
    }

    fi_fprintf(fph,"%c",UPPER_RIGHT_BORDER );
    fi_fprintf(fph," \r\n");


    bool lastHeader = true; // assume heading by default..


    //*****************************************
    // Print the body data
    //*****************************************
    HtmlRowVector::iterator j;
    for (j=mBody.begin(); j!=mBody.end(); j++)
    {
      HtmlRow &row = *(*j);

      if ( (!row.isHeader() && lastHeader) || row.isFooter() )
      {
        //*****************************************
        // Print the separator border
        //*****************************************


        fi_fprintf(fph,"%c",LEFT_SIDE_BORDER );

        for (size_t i=0; i<column_count; i++)
        {
          size_t c = csize[i];
          for (size_t j=0; j<c; j++)
          {
            fi_fprintf(fph,"%c", TOP_BORDER );
          }
          if ( (i+1) < column_count )
          {
            fi_fprintf(fph,"%c", CROSS_BORDER );
          }
        }

        fi_fprintf(fph,"%c",RIGHT_SIDE_BORDER );
        fi_fprintf(fph," \r\n");
      }

      lastHeader = row.isHeader();

      fi_fprintf(fph,"%c", LEFT_BORDER );

      for (size_t i=0; i<column_count; i++)
      {
        size_t c = csize[i];


        std::string str;
        row.getString(i,str);

        assert( str.size() < csize[i] );

        if ( lastHeader )
        {
          printCenter(fph,str,c);
        }
        else if ( isNumeric(str) )
        {
          printRight(fph,str,c);
        }
        else
        {
          printLeft(fph,str,c);
        }

        if ( (i+1) < column_count )
        {
          fi_fprintf(fph,"%c", VERTICAL_SEPARATOR );
        }
      }

      fi_fprintf(fph,"%c",RIGHT_BORDER );
      fi_fprintf(fph," \r\n");


    }

    //*****************************************
    // Print the separator border
    //*****************************************


    fi_fprintf(fph,"%c",LOWER_LEFT_BORDER );

    for (size_t i=0; i<column_count; i++)
    {
      size_t c = csize[i];
      for (size_t j=0; j<c; j++)
      {
        fi_fprintf(fph,"%c", TOP_BORDER );
      }
      if ( (i+1) < column_count )
      {
        fi_fprintf(fph,"%c", CROSS_BORDER );
      }
    }

    fi_fprintf(fph,"%c",LOWER_RIGHT_BORDER );
    fi_fprintf(fph," \r\n");
    fi_fprintf(fph," \r\n");
    fi_fprintf(fph," \r\n");




  }

  HtmlDocument * getDocument(void) { return mParent; };

  HtmlTableInterface * getHtmlTableInterface(void)
  {
    HtmlTableInterface *ret = 0;

    ret = mParent->getHtmlTableInterface();

    return ret;
  }

  void computeTotals(void) // compute and display totals of numeric columns when displaying this table.
  {
    mComputeTotals = true;
  }

  void excludeTotals(unsigned int column)
  {
    mExcludeTotals.push_back(column);
  }

  void addSort(const char *sort_name,unsigned int primary_key,bool primary_ascending,unsigned int secondary_key,bool secondary_ascending) // adds a sorted result.  You can set up mulitple sort requests for a single table.
  {
    SortRequest sr(sort_name,primary_key,primary_ascending,secondary_key,secondary_ascending);
    mSortRequests.push_back(sr);
  }

  void  setColumnColor(unsigned int column,unsigned int color) // set a color for a specific column.
  {
    mColumnColors.push_back(column);
    mColumnColors.push_back(color);
  }

  void  setHeaderColor(unsigned int color)               // color for header lines
  {
    mHeaderColor = color;
  }

  void setFooterColor(unsigned int color)              // color for footer lines
  {
    mFooterColor = color;
  }

  void setBodyColor(unsigned int color)
  {
    mBodyColor = color;
  }

  unsigned int getColor(unsigned int column,bool isHeader,bool isFooter)
  {

    unsigned int ret = mBodyColor;

    if ( isHeader )
    {
      ret = mHeaderColor;
    }
    else if ( isFooter )
    {
      ret = mFooterColor;
    }
    else
    {
      unsigned int count = unsigned int(mColumnColors.size())/2;
      for (unsigned int i=0; i<count; i++)
      {
        unsigned int c = unsigned int(mColumnColors[i*2+0]);
        unsigned int color = unsigned int(mColumnColors[i*2+1]);
        if ( column == c )
        {
          ret = color;
          break;
        }
      }
    }
    return ret;
  }

  virtual void                setOrder(unsigned int order)
  {
    mDisplayOrder = order;
  }


  const std::string & getHeading(void) const { return mHeading; };

private:
  unsigned int                        mDisplayOrder;
  unsigned int                        mHeaderColor;
  unsigned int                        mFooterColor;
  unsigned int                        mBodyColor;
  std::vector< unsigned int > mColumnColors;
  HtmlDocument        *mParent;
  std::string          mHeading;
  HtmlRow             *mCurrent;
  HtmlRowVector        mBody;
  InPlaceParser        mParser;
  SortRequest          mSortRequest;      // the current sort request...
  SortRequestVector    mSortRequests;

  bool                        mComputeTotals;
  std::vector< unsigned int > mExcludeTotals;

  unsigned char UPPER_LEFT_BORDER;
  unsigned char UPPER_RIGHT_BORDER;
  unsigned char LOWER_LEFT_BORDER;
  unsigned char LOWER_RIGHT_BORDER;
  unsigned char TOP_SEPARATOR;
  unsigned char BOTTOM_SEPARATOR;
  unsigned char TOP_BORDER;
  unsigned char BOTTOM_BORDER;
  unsigned char LEFT_BORDER;
  unsigned char RIGHT_BORDER;
  unsigned char VERTICAL_SEPARATOR;
  unsigned char LEFT_SIDE_BORDER;
  unsigned char RIGHT_SIDE_BORDER;
  unsigned char CROSS_BORDER;


};

typedef std::vector< _HtmlTable * > HtmlTableVector;


class SortTables : public QuickSortPointers
{
public:
  SortTables(unsigned int tcount,_HtmlTable **tables)
  {
    qsort((void **)tables,(int)tcount);
  }

  virtual int compare(void **p1,void **p2)
  {
    _HtmlTable **tp1 = (_HtmlTable **)p1;
    _HtmlTable **tp2 = (_HtmlTable **)p2;
    _HtmlTable *t1   = *tp1;
    _HtmlTable *t2   = *tp2;
    return (int)t1->getDisplayOrder() - (int)t2->getDisplayOrder();
  }

};

class _HtmlDocument : public HtmlDocument
{
public:
  _HtmlDocument(const char *document_name,HtmlTableInterface *iface)
  {
    mDisplayOrder = 0;
    mInterface = iface;
    if ( document_name )
    {
      mDocumentName = document_name;
    }
  }

  virtual ~_HtmlDocument(void)
  {
    reset();
  }

  unsigned int getDisplayOrder(void)
  {
    mDisplayOrder++;
    return mDisplayOrder;
  }

  void reset(void)
  {
    HtmlTableVector::iterator i;
    for (i=mTables.begin(); i!=mTables.end(); i++)
    {
      HtmlTable *t = (*i);
      _HtmlTable *tt = static_cast< _HtmlTable *>(t);
      HTML_DELETE(_HtmlTable,tt);
    }
    mTables.clear();
  }


  const char * saveDocument(size_t &len,HtmlSaveType type)
  {
    const char *ret = 0;

    FILE_INTERFACE *fph = fi_fopen("temp", "wmem");
    if ( fph )
    {

      switch ( type )
      {
        case HST_SIMPLE_HTML:
          fi_fprintf(fph,"<HTML>\r\n");
          fi_fprintf(fph,"<HEAD>\r\n");
          fi_fprintf(fph,"<TITLE>%s</TITLE>\r\n", mDocumentName.c_str() );
          fi_fprintf(fph,"<BODY>\r\n");
          break;
        case HST_CSV:
          break;
        case HST_CPP:
#if USE_CPP
          fi_fprintf(fph,"#include <stdlib.h>\r\n");
          fi_fprintf(fph,"#include <stdio.h>\r\n");
          fi_fprintf(fph,"#include <string.h>\r\n");
          fi_fprintf(fph,"#include <assert.h>\r\n");
          fi_fprintf(fph,"\r\n");
          fi_fprintf(fph,"#pragma warning(disable:4996)\r\n");
          fi_fprintf(fph,"\r\n");
          fi_fprintf(fph,"#include \"HtmlTable.h\"\r\n");
          fi_fprintf(fph,"\r\n");


          fi_fprintf(fph,"%s\r\n","void testSave(nvidia::HtmlDocument *document,const char *fname,nvidia::HtmlSaveType type)");
          fi_fprintf(fph,"%s\r\n","{");
          fi_fprintf(fph,"%s\r\n","  size_t len;");
          fi_fprintf(fph,"%s\r\n","  const char *data = document->saveDocument(len,type);");
          fi_fprintf(fph,"%s\r\n","  if ( data )");
          fi_fprintf(fph,"%s\r\n","  {");
          fi_fprintf(fph,"%s\r\n","    printf(\"Saving document '%s' which is %d bytes long.\\r\\n\", fname, len );");
          fi_fprintf(fph,"%s\r\n","    FILE *fph = fopen(fname,\"wb\");");
          fi_fprintf(fph,"%s\r\n","    if ( fph )");
          fi_fprintf(fph,"%s\r\n","    {");
          fi_fprintf(fph,"%s\r\n","      fwrite(data,len,1,fph);");
          fi_fprintf(fph,"%s\r\n","      fclose(fph);");
          fi_fprintf(fph,"%s\r\n","    }");
          fi_fprintf(fph,"%s\r\n","    else");
          fi_fprintf(fph,"%s\r\n","    {");
          fi_fprintf(fph,"%s\r\n","      printf(\"Failed to open file for write access.\\r\\n\");");
          fi_fprintf(fph,"%s\r\n","    }");
          fi_fprintf(fph,"%s\r\n","    document->releaseDocumentMemory(data);");
          fi_fprintf(fph,"%s\r\n","  }");
          fi_fprintf(fph,"%s\r\n","  else");
          fi_fprintf(fph,"%s\r\n","  {");
          fi_fprintf(fph,"%s\r\n","    printf(\"Failed to save document %s.\\r\\n\", fname );");
          fi_fprintf(fph,"%s\r\n","  }");
          fi_fprintf(fph,"%s\r\n","}");

          fi_fprintf(fph,"void html_test(void)\r\n");
          fi_fprintf(fph,"{\r\n");
          fi_fprintf(fph,"  nvidia::HtmlTableInterface *iface = nvidia::getHtmlTableInterface();\r\n");
          fi_fprintf(fph,"  nvidia::HtmlDocument *document    = iface->createHtmlDocument(\"%s\");\r\n", mDocumentName.c_str() );
#endif
          break;
        case HST_TEXT:
        case HST_TEXT_EXTENDED:
          fi_fprintf(fph,"[%s]\r\n", mDocumentName.c_str() );
          fi_fprintf(fph,"\r\n" );
          break;
		default:
			assert(0);
      }

	  if ( !mTables.empty() )
	  {
		  unsigned int tcount = mTables.size();
		  _HtmlTable **tables = &mTables[0];
          SortTables st( tcount, tables );
		  HtmlTableVector::iterator i;
		  for (i=mTables.begin(); i!=mTables.end(); ++i)
		  {
			(*i)->save(fph,type);
		  }
	  }

      switch ( type )
      {
        case HST_SIMPLE_HTML:
          fi_fprintf(fph,"</BODY>\r\n");
          fi_fprintf(fph,"</HEAD>\r\n");
          fi_fprintf(fph,"</HTML>\r\n");
          break;
        case HST_CPP:
#if USE_CPP
          fi_fprintf(fph,"%s\r\n","  testSave(document,\"table.txt\",  nvidia::HST_TEXT );");
          fi_fprintf(fph,"%s\r\n","  testSave(document,\"table.html\", nvidia::HST_SIMPLE_HTML );");
          fi_fprintf(fph,"%s\r\n","  testSave(document,\"table.cpp\",  nvidia::HST_CPP );");
          fi_fprintf(fph,"%s\r\n","");
          fi_fprintf(fph,"%s\r\n","");
          fi_fprintf(fph,"%s\r\n","");
          fi_fprintf(fph,"%s\r\n","  iface->releaseHtmlDocument(document);");
          fi_fprintf(fph,"%s\r\n","");
          fi_fprintf(fph,"%s\r\n","}");
#endif
          break;
        case HST_CSV:
          break;
        case HST_TEXT:
        case HST_TEXT_EXTENDED:
          fi_fprintf(fph,"\r\n" );
          break;
		default:
			assert(0);
      }
      void *data = fi_getMemBuffer(fph,len);
      if ( data )
      {
        char *temp = (char *)HTML_MALLOC(sizeof(char)*(len+1));
        temp[len] = 0;
        memcpy(temp,data,len);
        ret = temp;
      }
      fi_fclose(fph);
    }
    return ret;
  }

  HtmlTable * createHtmlTable(const char *heading)
  {
    _HtmlTable *ret = HTML_NEW(_HtmlTable)(heading,this);
    mTables.push_back(ret);
    return ret;
  }

  void           releaseDocumentMemory(const char *mem)           // release memory previously allocated for a document save.
  {
    HTML_FREE((void *)mem);
  }


  HtmlTableInterface *getHtmlTableInterface(void)
  {
    return mInterface;
  }

  bool saveExcel(const char *fname) // excel format can only be saved directly to files on disk, as it needs to create a sub-directory for the intermediate files.
  {
    bool ret = false;

    std::string dest_name;
    std::string base_name;
    std::string root_dir;


    char scratch[512];
    strcpy(scratch,fname);
    char *dot = lastDot(scratch);
    if ( dot )
    {
      strcpy(scratch,fname);
      char *slash = lastSlash(scratch);
      if ( slash )
      {
        slash++;
        char bname[512];
        strcpy(bname,slash);
        dest_name = bname;
        *slash = 0;
        dot = lastDot(bname);
        assert(dot);
        if ( dot )
        {
          char temp[512];
          *dot = 0;
          base_name = bname;
          root_dir = scratch;
          sprintf(temp,"%s%s_files",scratch,bname);
          _mkdir(temp);
        }
      }
      else
      {
        dest_name = fname;
        *dot = 0;
        base_name = scratch;
        char temp[512];
        sprintf(temp,"%s_files", scratch );
        _mkdir(temp);
      }
    }

    saveExcel(dest_name,base_name,root_dir);

    return ret;
  }

  void saveExcel(const std::string &dest_name,const std::string &base_name,std::string &root_dir)
  {
#if USE_EXCEL

    char scratch[512];
    sprintf(scratch,"%s%s", root_dir.c_str(), dest_name.c_str() );

    unsigned int tableCount = unsigned int(mTables.size());

    FILE *fph = fopen(scratch,"wb");

    if ( fph )
    {
      fprintf(fph,"%s\r\n","<html xmlns:o=\"urn:schemas-microsoft-com:office:office\"");
      fprintf(fph,"%s\r\n","xmlns:x=\"urn:schemas-microsoft-com:office:excel\"");
      fprintf(fph,"%s\r\n","xmlns=\"http://www.w3.org/TR/REC-html40\">");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","<head>");
      fprintf(fph,"%s\r\n","<meta name=\"Excel Workbook Frameset\">");
      fprintf(fph,"%s\r\n","<meta http-equiv=Content-Type content=\"text/html; charset=windows-1252\">");
      fprintf(fph,"%s\r\n","<meta name=ProgId content=Excel.Sheet>");
      fprintf(fph,"%s\r\n","<meta name=Generator content=\"Microsoft Excel 11\">");
      fprintf(fph,"<link rel=File-List href=\"%s_files/filelist.xml\">\r\n", base_name.c_str());
      fprintf(fph,"<link rel=Edit-Time-Data href=\"%s_files/editdata.mso\">\r\n", base_name.c_str());
      fprintf(fph,"<link rel=OLE-Object-Data href=\"%s_files/oledata.mso\">\r\n", base_name.c_str());
      fprintf(fph,"%s\r\n","<!--[if gte mso 9]><xml>");
      fprintf(fph,"%s\r\n"," <o:DocumentProperties>");
      fprintf(fph,"%s\r\n","  <o:Author>John Ratcliff</o:Author>");
      fprintf(fph,"%s\r\n","  <o:LastAuthor>John Ratcliff</o:LastAuthor>");
      fprintf(fph,"%s\r\n","  <o:Created>2008-04-17T20:09:59Z</o:Created>");
      fprintf(fph,"%s\r\n","  <o:LastSaved>2008-04-17T20:10:32Z</o:LastSaved>");
      fprintf(fph,"%s\r\n","  <o:Company>Simutronics Corporation</o:Company>");
      fprintf(fph,"%s\r\n","  <o:Version>11.8132</o:Version>");
      fprintf(fph,"%s\r\n"," </o:DocumentProperties>");
      fprintf(fph,"%s\r\n"," <o:OfficeDocumentSettings>");
      fprintf(fph,"%s\r\n","  <o:DownloadComponents/>");
      fprintf(fph,"%s\r\n","  <o:LocationOfComponents HRef=\"file:///D:\\ENGLISH\\OFFICE_SYSTEM\\OFFICE2003\\PROFESSIONAL\\\"/>");
      fprintf(fph,"%s\r\n"," </o:OfficeDocumentSettings>");
      fprintf(fph,"%s\r\n","</xml><![endif]--><![if !supportTabStrip]>");

      {
        unsigned int index = 0;
        HtmlTableVector::iterator i;
        for (i=mTables.begin(); i!=mTables.end(); ++i)
        {
          fprintf(fph,"<link id=\"shLink\" href=\"%s_files/sheet%03d.htm\">\r\n", base_name.c_str(), index+1 );
          index++;
        }
      }


      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","<link id=\"shLink\">");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","<script language=\"JavaScript\">");
      fprintf(fph,"%s\r\n","<!--");

      {
        fprintf(fph," var c_lTabs=%d;\r\n", mTables.size());
      }


      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var c_rgszSh=new Array(c_lTabs);");

      {
        unsigned int index = 0;
        HtmlTableVector::iterator i;
        for (i=mTables.begin(); i!=mTables.end(); ++i)
        {
          fprintf(fph," c_rgszSh[%d] = \"Sheet%d\";\r\n",index,index+1);
          index++;
        }
      }

      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var c_rgszClr=new Array(8);");
      fprintf(fph,"%s\r\n"," c_rgszClr[0]=\"window\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[1]=\"buttonface\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[2]=\"windowframe\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[3]=\"windowtext\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[4]=\"threedlightshadow\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[5]=\"threedhighlight\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[6]=\"threeddarkshadow\";");
      fprintf(fph,"%s\r\n"," c_rgszClr[7]=\"threedshadow\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var g_iShCur;");
      fprintf(fph,"%s\r\n"," var g_rglTabX=new Array(c_lTabs);");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnGetIEVer()");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var ua=window.navigator.userAgent");
      fprintf(fph,"%s\r\n"," var msie=ua.indexOf(\"MSIE\")");
      fprintf(fph,"%s\r\n"," if (msie>0 && window.navigator.platform==\"Win32\")");
      fprintf(fph,"%s\r\n","  return parseInt(ua.substring(msie+5,ua.indexOf(\".\", msie)));");
      fprintf(fph,"%s\r\n"," else");
      fprintf(fph,"%s\r\n","  return 0;");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnBuildFrameset()");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var szHTML=\"<frameset rows=\\\"*,18\\\" border=0 width=0 frameborder=no framespacing=0>\"+");
      fprintf(fph,"%s\r\n","  \"<frame src=\\\"\"+document.all.item(\"shLink\")[1].href+\"\\\" name=\\\"frSheet\\\" noresize>\"+");
      fprintf(fph,"%s\r\n","  \"<frameset cols=\\\"54,*\\\" border=0 width=0 frameborder=no framespacing=0>\"+");
      fprintf(fph,"%s\r\n","  \"<frame src=\\\"\\\" name=\\\"frScroll\\\" marginwidth=0 marginheight=0 scrolling=no>\"+");
      fprintf(fph,"%s\r\n","  \"<frame src=\\\"\\\" name=\\\"frTabs\\\" marginwidth=0 marginheight=0 scrolling=no>\"+");
      fprintf(fph,"%s\r\n","  \"</frameset></frameset><plaintext>\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," with (document) {");
      fprintf(fph,"%s\r\n","  open(\"text/html\",\"replace\");");
      fprintf(fph,"%s\r\n","  write(szHTML);");
      fprintf(fph,"%s\r\n","  close();");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," fnBuildTabStrip();");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnBuildTabStrip()");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var szHTML=");
      fprintf(fph,"%s\r\n","  \"<html><head><style>.clScroll {font:8pt Courier New;color:\"+c_rgszClr[6]+\";cursor:default;line-height:10pt;}\"+");
      fprintf(fph,"%s\r\n","  \".clScroll2 {font:10pt Arial;color:\"+c_rgszClr[6]+\";cursor:default;line-height:11pt;}</style></head>\"+");
      fprintf(fph,"%s\r\n","  \"<body onclick=\\\"event.returnValue=false;\\\" ondragstart=\\\"event.returnValue=false;\\\" onselectstart=\\\"event.returnValue=false;\\\" bgcolor=\"+c_rgszClr[4]+\" topmargin=0 leftmargin=0><table cellpadding=0 cellspacing=0 width=100%>\"+");
      fprintf(fph,"%s\r\n","  \"<tr><td colspan=6 height=1 bgcolor=\"+c_rgszClr[2]+\"></td></tr>\"+");
      fprintf(fph,"%s\r\n","  \"<tr><td style=\\\"font:1pt\\\">&nbsp;<td>\"+");
      fprintf(fph,"%s\r\n","  \"<td valign=top id=tdScroll class=\\\"clScroll\\\" onclick=\\\"parent.fnFastScrollTabs(0);\\\" onmouseover=\\\"parent.fnMouseOverScroll(0);\\\" onmouseout=\\\"parent.fnMouseOutScroll(0);\\\"><a>&#171;</a></td>\"+");
      fprintf(fph,"%s\r\n","  \"<td valign=top id=tdScroll class=\\\"clScroll2\\\" onclick=\\\"parent.fnScrollTabs(0);\\\" ondblclick=\\\"parent.fnScrollTabs(0);\\\" onmouseover=\\\"parent.fnMouseOverScroll(1);\\\" onmouseout=\\\"parent.fnMouseOutScroll(1);\\\"><a>&lt</a></td>\"+");
      fprintf(fph,"%s\r\n","  \"<td valign=top id=tdScroll class=\\\"clScroll2\\\" onclick=\\\"parent.fnScrollTabs(1);\\\" ondblclick=\\\"parent.fnScrollTabs(1);\\\" onmouseover=\\\"parent.fnMouseOverScroll(2);\\\" onmouseout=\\\"parent.fnMouseOutScroll(2);\\\"><a>&gt</a></td>\"+");
      fprintf(fph,"%s\r\n","  \"<td valign=top id=tdScroll class=\\\"clScroll\\\" onclick=\\\"parent.fnFastScrollTabs(1);\\\" onmouseover=\\\"parent.fnMouseOverScroll(3);\\\" onmouseout=\\\"parent.fnMouseOutScroll(3);\\\"><a>&#187;</a></td>\"+");
      fprintf(fph,"%s\r\n","  \"<td style=\\\"font:1pt\\\">&nbsp;<td></tr></table></body></html>\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," with (frames['frScroll'].document) {");
      fprintf(fph,"%s\r\n","  open(\"text/html\",\"replace\");");
      fprintf(fph,"%s\r\n","  write(szHTML);");
      fprintf(fph,"%s\r\n","  close();");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," szHTML =");
      fprintf(fph,"%s\r\n","  \"<html><head>\"+");
      fprintf(fph,"%s\r\n","  \"<style>A:link,A:visited,A:active {text-decoration:none;\"+\"color:\"+c_rgszClr[3]+\";}\"+");
      fprintf(fph,"%s\r\n","  \".clTab {cursor:hand;background:\"+c_rgszClr[1]+\";font:9pt Arial;padding-left:3px;padding-right:3px;text-align:center;}\"+");
      fprintf(fph,"%s\r\n","  \".clBorder {background:\"+c_rgszClr[2]+\";font:1pt;}\"+");
      fprintf(fph,"%s\r\n","  \"</style></head><body onload=\\\"parent.fnInit();\\\" onselectstart=\\\"event.returnValue=false;\\\" ondragstart=\\\"event.returnValue=false;\\\" bgcolor=\"+c_rgszClr[4]+");
      fprintf(fph,"%s\r\n","  \" topmargin=0 leftmargin=0><table id=tbTabs cellpadding=0 cellspacing=0>\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var iCellCount=(c_lTabs+1)*2;");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var i;");
      fprintf(fph,"%s\r\n"," for (i=0;i<iCellCount;i+=2)");
      fprintf(fph,"%s\r\n","  szHTML+=\"<col width=1><col>\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," var iRow;");
      fprintf(fph,"%s\r\n"," for (iRow=0;iRow<6;iRow++) {");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","  szHTML+=\"<tr>\";");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","  if (iRow==5)");
      fprintf(fph,"%s\r\n","   szHTML+=\"<td colspan=\"+iCellCount+\"></td>\";");
      fprintf(fph,"%s\r\n","  else {");
      fprintf(fph,"%s\r\n","   if (iRow==0) {");
      fprintf(fph,"%s\r\n","    for(i=0;i<iCellCount;i++)");
      fprintf(fph,"%s\r\n","     szHTML+=\"<td height=1 class=\\\"clBorder\\\"></td>\";");
      fprintf(fph,"%s\r\n","   } else if (iRow==1) {");
      fprintf(fph,"%s\r\n","    for(i=0;i<c_lTabs;i++) {");
      fprintf(fph,"%s\r\n","     szHTML+=\"<td height=1 nowrap class=\\\"clBorder\\\">&nbsp;</td>\";");
      fprintf(fph,"%s\r\n","     szHTML+=");
      fprintf(fph,"%s\r\n","      \"<td id=tdTab height=1 nowrap class=\\\"clTab\\\" onmouseover=\\\"parent.fnMouseOverTab(\"+i+\");\\\" onmouseout=\\\"parent.fnMouseOutTab(\"+i+\");\\\">\"+");
      fprintf(fph,"%s\r\n","      \"<a href=\\\"\"+document.all.item(\"shLink\")[i].href+\"\\\" target=\\\"frSheet\\\" id=aTab>&nbsp;\"+c_rgszSh[i]+\"&nbsp;</a></td>\";");
      fprintf(fph,"%s\r\n","    }");
      fprintf(fph,"%s\r\n","    szHTML+=\"<td id=tdTab height=1 nowrap class=\\\"clBorder\\\"><a id=aTab>&nbsp;</a></td><td width=100%></td>\";");
      fprintf(fph,"%s\r\n","   } else if (iRow==2) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<c_lTabs;i++)");
      fprintf(fph,"%s\r\n","     szHTML+=\"<td height=1></td><td height=1 class=\\\"clBorder\\\"></td>\";");
      fprintf(fph,"%s\r\n","    szHTML+=\"<td height=1></td><td height=1></td>\";");
      fprintf(fph,"%s\r\n","   } else if (iRow==3) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<iCellCount;i++)");
      fprintf(fph,"%s\r\n","     szHTML+=\"<td height=1></td>\";");
      fprintf(fph,"%s\r\n","   } else if (iRow==4) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<c_lTabs;i++)");
      fprintf(fph,"%s\r\n","     szHTML+=\"<td height=1 width=1></td><td height=1></td>\";");
      fprintf(fph,"%s\r\n","    szHTML+=\"<td height=1 width=1></td><td></td>\";");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","  }");
      fprintf(fph,"%s\r\n","  szHTML+=\"</tr>\";");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," szHTML+=\"</table></body></html>\";");
      fprintf(fph,"%s\r\n"," with (frames['frTabs'].document) {");
      fprintf(fph,"%s\r\n","  open(\"text/html\",\"replace\");");
      fprintf(fph,"%s\r\n","  charset=document.charset;");
      fprintf(fph,"%s\r\n","  write(szHTML);");
      fprintf(fph,"%s\r\n","  close();");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnInit()");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," g_rglTabX[0]=0;");
      fprintf(fph,"%s\r\n"," var i;");
      fprintf(fph,"%s\r\n"," for (i=1;i<=c_lTabs;i++)");
      fprintf(fph,"%s\r\n","  with (frames['frTabs'].document.all.tbTabs.rows[1].cells[fnTabToCol(i-1)])");
      fprintf(fph,"%s\r\n","   g_rglTabX[i]=offsetLeft+offsetWidth-6;");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnTabToCol(iTab)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," return 2*iTab+1;");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnNextTab(fDir)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var iNextTab=-1;");
      fprintf(fph,"%s\r\n"," var i;");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," with (frames['frTabs'].document.body) {");
      fprintf(fph,"%s\r\n","  if (fDir==0) {");
      fprintf(fph,"%s\r\n","   if (scrollLeft>0) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<c_lTabs&&g_rglTabX[i]<scrollLeft;i++);");
      fprintf(fph,"%s\r\n","    if (i<c_lTabs)");
      fprintf(fph,"%s\r\n","     iNextTab=i-1;");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","  } else {");
      fprintf(fph,"%s\r\n","   if (g_rglTabX[c_lTabs]+6>offsetWidth+scrollLeft) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<c_lTabs&&g_rglTabX[i]<=scrollLeft;i++);");
      fprintf(fph,"%s\r\n","    if (i<c_lTabs)");
      fprintf(fph,"%s\r\n","     iNextTab=i;");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","  }");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n"," return iNextTab;");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnScrollTabs(fDir)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var iNextTab=fnNextTab(fDir);");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," if (iNextTab>=0) {");
      fprintf(fph,"%s\r\n","  frames['frTabs'].scroll(g_rglTabX[iNextTab],0);");
      fprintf(fph,"%s\r\n","  return true;");
      fprintf(fph,"%s\r\n"," } else");
      fprintf(fph,"%s\r\n","  return false;");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnFastScrollTabs(fDir)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," if (c_lTabs>16)");
      fprintf(fph,"%s\r\n","  frames['frTabs'].scroll(g_rglTabX[fDir?c_lTabs-1:0],0);");
      fprintf(fph,"%s\r\n"," else");
      fprintf(fph,"%s\r\n","  if (fnScrollTabs(fDir)>0) window.setTimeout(\"fnFastScrollTabs(\"+fDir+\");\",5);");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnSetTabProps(iTab,fActive)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," var iCol=fnTabToCol(iTab);");
      fprintf(fph,"%s\r\n"," var i;");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," if (iTab>=0) {");
      fprintf(fph,"%s\r\n","  with (frames['frTabs'].document.all) {");
      fprintf(fph,"%s\r\n","   with (tbTabs) {");
      fprintf(fph,"%s\r\n","    for (i=0;i<=4;i++) {");
      fprintf(fph,"%s\r\n","     with (rows[i]) {");
      fprintf(fph,"%s\r\n","      if (i==0)");
      fprintf(fph,"%s\r\n","       cells[iCol].style.background=c_rgszClr[fActive?0:2];");
      fprintf(fph,"%s\r\n","      else if (i>0 && i<4) {");
      fprintf(fph,"%s\r\n","       if (fActive) {");
      fprintf(fph,"%s\r\n","        cells[iCol-1].style.background=c_rgszClr[2];");
      fprintf(fph,"%s\r\n","        cells[iCol].style.background=c_rgszClr[0];");
      fprintf(fph,"%s\r\n","        cells[iCol+1].style.background=c_rgszClr[2];");
      fprintf(fph,"%s\r\n","       } else {");
      fprintf(fph,"%s\r\n","        if (i==1) {");
      fprintf(fph,"%s\r\n","         cells[iCol-1].style.background=c_rgszClr[2];");
      fprintf(fph,"%s\r\n","         cells[iCol].style.background=c_rgszClr[1];");
      fprintf(fph,"%s\r\n","         cells[iCol+1].style.background=c_rgszClr[2];");
      fprintf(fph,"%s\r\n","        } else {");
      fprintf(fph,"%s\r\n","         cells[iCol-1].style.background=c_rgszClr[4];");
      fprintf(fph,"%s\r\n","         cells[iCol].style.background=c_rgszClr[(i==2)?2:4];");
      fprintf(fph,"%s\r\n","         cells[iCol+1].style.background=c_rgszClr[4];");
      fprintf(fph,"%s\r\n","        }");
      fprintf(fph,"%s\r\n","       }");
      fprintf(fph,"%s\r\n","      } else");
      fprintf(fph,"%s\r\n","       cells[iCol].style.background=c_rgszClr[fActive?2:4];");
      fprintf(fph,"%s\r\n","     }");
      fprintf(fph,"%s\r\n","    }");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","   with (aTab[iTab].style) {");
      fprintf(fph,"%s\r\n","    cursor=(fActive?\"default\":\"hand\");");
      fprintf(fph,"%s\r\n","    color=c_rgszClr[3];");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","  }");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnMouseOverScroll(iCtl)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," frames['frScroll'].document.all.tdScroll[iCtl].style.color=c_rgszClr[7];");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnMouseOutScroll(iCtl)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," frames['frScroll'].document.all.tdScroll[iCtl].style.color=c_rgszClr[6];");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnMouseOverTab(iTab)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," if (iTab!=g_iShCur) {");
      fprintf(fph,"%s\r\n","  var iCol=fnTabToCol(iTab);");
      fprintf(fph,"%s\r\n","  with (frames['frTabs'].document.all) {");
      fprintf(fph,"%s\r\n","   tdTab[iTab].style.background=c_rgszClr[5];");
      fprintf(fph,"%s\r\n","  }");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnMouseOutTab(iTab)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," if (iTab>=0) {");
      fprintf(fph,"%s\r\n","  var elFrom=frames['frTabs'].event.srcElement;");
      fprintf(fph,"%s\r\n","  var elTo=frames['frTabs'].event.toElement;");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","  if ((!elTo) ||");
      fprintf(fph,"%s\r\n","   (elFrom.tagName==elTo.tagName) ||");
      fprintf(fph,"%s\r\n","   (elTo.tagName==\"A\" && elTo.parentElement!=elFrom) ||");
      fprintf(fph,"%s\r\n","   (elFrom.tagName==\"A\" && elFrom.parentElement!=elTo)) {");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","   if (iTab!=g_iShCur) {");
      fprintf(fph,"%s\r\n","    with (frames['frTabs'].document.all) {");
      fprintf(fph,"%s\r\n","     tdTab[iTab].style.background=c_rgszClr[1];");
      fprintf(fph,"%s\r\n","    }");
      fprintf(fph,"%s\r\n","   }");
      fprintf(fph,"%s\r\n","  }");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","function fnSetActiveSheet(iSh)");
      fprintf(fph,"%s\r\n","{");
      fprintf(fph,"%s\r\n"," if (iSh!=g_iShCur) {");
      fprintf(fph,"%s\r\n","  fnSetTabProps(g_iShCur,false);");
      fprintf(fph,"%s\r\n","  fnSetTabProps(iSh,true);");
      fprintf(fph,"%s\r\n","  g_iShCur=iSh;");
      fprintf(fph,"%s\r\n"," }");
      fprintf(fph,"%s\r\n","}");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n"," window.g_iIEVer=fnGetIEVer();");
      fprintf(fph,"%s\r\n"," if (window.g_iIEVer>=4)");
      fprintf(fph,"%s\r\n","  fnBuildFrameset();");
      fprintf(fph,"%s\r\n","//-->");
      fprintf(fph,"%s\r\n","</script>");
      fprintf(fph,"%s\r\n","<![endif]><!--[if gte mso 9]><xml>");
      fprintf(fph,"%s\r\n"," <x:ExcelWorkbook>");
      fprintf(fph,"%s\r\n","  <x:ExcelWorksheets>");

      {
        unsigned int index = 0;
        HtmlTableVector::iterator i;
        for (i=mTables.begin(); i!=mTables.end(); ++i)
        {
          fprintf(fph,"%s\r\n","   <x:ExcelWorksheet>");
          fprintf(fph,"    <x:Name>Sheet%d</x:Name>\r\n", index+1 );
          fprintf(fph,"    <x:WorksheetSource HRef=\"%s_files/sheet%03d.htm\"/>\r\n", base_name.c_str(), index+1 );
          fprintf(fph,"%s\r\n","   </x:ExcelWorksheet>");
          index++;
        }
      }


      fprintf(fph,"%s\r\n","  </x:ExcelWorksheets>");
      fprintf(fph,"  <x:Stylesheet HRef=\"%s_files/stylesheet.css\"/>\r\n", base_name.c_str());
      fprintf(fph,"%s\r\n","  <x:WindowHeight>15585</x:WindowHeight>");
      fprintf(fph,"%s\r\n","  <x:WindowWidth>24795</x:WindowWidth>");
      fprintf(fph,"%s\r\n","  <x:WindowTopX>480</x:WindowTopX>");
      fprintf(fph,"%s\r\n","  <x:WindowTopY>105</x:WindowTopY>");
      fprintf(fph,"%s\r\n","  <x:ActiveSheet>1</x:ActiveSheet>");
      fprintf(fph,"%s\r\n","  <x:ProtectStructure>False</x:ProtectStructure>");
      fprintf(fph,"%s\r\n","  <x:ProtectWindows>False</x:ProtectWindows>");
      fprintf(fph,"%s\r\n"," </x:ExcelWorkbook>");
      fprintf(fph,"%s\r\n","</xml><![endif]-->");
      fprintf(fph,"%s\r\n","</head>");
      fprintf(fph,"%s\r\n","");
      fprintf(fph,"%s\r\n","<frameset rows=\"*,39\" border=0 width=0 frameborder=no framespacing=0>");
      fprintf(fph," <frame src=\"%s_files/sheet001.htm\" name=\"frSheet\">\r\n", base_name.c_str());
      fprintf(fph," <frame src=\"%s_files/tabstrip.htm\" name=\"frTabs\" marginwidth=0 marginheight=0>\r\n", base_name.c_str());
      fprintf(fph,"%s\r\n"," <noframes>");
      fprintf(fph,"%s\r\n","  <body>");
      fprintf(fph,"%s\r\n","   <p>This page uses frames, but your browser doesn't support them.</p>");
      fprintf(fph,"%s\r\n","  </body>");
      fprintf(fph,"%s\r\n"," </noframes>");
      fprintf(fph,"%s\r\n","</frameset>");
      fprintf(fph,"%s\r\n","</html>");
      fclose(fph);
    }

    sprintf(scratch,"%s%s_files\\filelist.xml", root_dir.c_str(), base_name.c_str() );
    fph = fopen(scratch,"wb");
    if ( fph )
    {

      fprintf(fph,"<xml xmlns:o=\"urn:schemas-microsoft-com:office:office\">\r\n");
      fprintf(fph," <o:MainFile HRef=\"../%s\"/>\r\n", dest_name.c_str());
      fprintf(fph," <o:File HRef=\"stylesheet.css\"/>\r\n");
      fprintf(fph," <o:File HRef=\"tabstrip.htm\"/>\r\n");

      for (unsigned int i=0; i<tableCount; i++)
      {
        fprintf(fph," <o:File HRef=\"sheet%03d.htm\"/>\r\n", i+1);
      }

      fprintf(fph," <o:File HRef=\"filelist.xml\"/>\r\n");
      fprintf(fph,"</xml>\r\n");
      fclose(fph);
    }


    sprintf(scratch,"%s%s_files\\stylesheet.css", root_dir.c_str(), base_name.c_str() );
    fph = fopen(scratch,"wb");
    if ( fph )
    {
      fprintf(fph,"%s\r\n", "tr");
      fprintf(fph,"%s\r\n", "	{mso-height-source:auto;}");
      fprintf(fph,"%s\r\n", "col");
      fprintf(fph,"%s\r\n", "	{mso-width-source:auto;}");
      fprintf(fph,"%s\r\n", "br");
      fprintf(fph,"%s\r\n", "	{mso-data-placement:same-cell;}");
      fprintf(fph,"%s\r\n", ".style0");
      fprintf(fph,"%s\r\n", "	{mso-number-format:General;");
      fprintf(fph,"%s\r\n", "	text-align:general;");
      fprintf(fph,"%s\r\n", "	vertical-align:bottom;");
      fprintf(fph,"%s\r\n", "	white-space:nowrap;");
      fprintf(fph,"%s\r\n", "	mso-rotate:0;");
      fprintf(fph,"%s\r\n", "	mso-background-source:auto;");
      fprintf(fph,"%s\r\n", "	mso-pattern:auto;");
      fprintf(fph,"%s\r\n", "	color:windowtext;");
      fprintf(fph,"%s\r\n", "	font-size:10.0pt;");
      fprintf(fph,"%s\r\n", "	font-weight:400;");
      fprintf(fph,"%s\r\n", "	font-style:normal;");
      fprintf(fph,"%s\r\n", "	text-decoration:none;");
      fprintf(fph,"%s\r\n", "	font-family:Arial;");
      fprintf(fph,"%s\r\n", "	mso-generic-font-family:auto;");
      fprintf(fph,"%s\r\n", "	mso-font-charset:0;");
      fprintf(fph,"%s\r\n", "	border:none;");
      fprintf(fph,"%s\r\n", "	mso-protection:locked visible;");
      fprintf(fph,"%s\r\n", "	mso-style-name:Normal;");
      fprintf(fph,"%s\r\n", "	mso-style-id:0;}");
      fprintf(fph,"%s\r\n", "td");
      fprintf(fph,"%s\r\n", "	{mso-style-parent:style0;");
      fprintf(fph,"%s\r\n", "	padding-top:1px;");
      fprintf(fph,"%s\r\n", "	padding-right:1px;");
      fprintf(fph,"%s\r\n", "	padding-left:1px;");
      fprintf(fph,"%s\r\n", "	mso-ignore:padding;");
      fprintf(fph,"%s\r\n", "	color:windowtext;");
      fprintf(fph,"%s\r\n", "	font-size:10.0pt;");
      fprintf(fph,"%s\r\n", "	font-weight:400;");
      fprintf(fph,"%s\r\n", "	font-style:normal;");
      fprintf(fph,"%s\r\n", "	text-decoration:none;");
      fprintf(fph,"%s\r\n", "	font-family:Arial;");
      fprintf(fph,"%s\r\n", "	mso-generic-font-family:auto;");
      fprintf(fph,"%s\r\n", "	mso-font-charset:0;");
      fprintf(fph,"%s\r\n", "	mso-number-format:General;");
      fprintf(fph,"%s\r\n", "	text-align:general;");
      fprintf(fph,"%s\r\n", "	vertical-align:bottom;");
      fprintf(fph,"%s\r\n", "	border:none;");
      fprintf(fph,"%s\r\n", "	mso-background-source:auto;");
      fprintf(fph,"%s\r\n", "	mso-pattern:auto;");
      fprintf(fph,"%s\r\n", "	mso-protection:locked visible;");
      fprintf(fph,"%s\r\n", "	white-space:nowrap;");
      fprintf(fph,"%s\r\n", "	mso-rotate:0;}");
      fclose(fph);
    }

    sprintf(scratch,"%s%s_files\\tabstrip.htm", root_dir.c_str(), base_name.c_str() );
    fph = fopen(scratch,"wb");
    if ( fph )
    {
      fprintf(fph,"<html>\r\n");
      fprintf(fph,"<head>\r\n");
      fprintf(fph,"<meta http-equiv=Content-Type content=\"text/html; charset=windows-1252\">\r\n");
      fprintf(fph,"<meta name=ProgId content=Excel.Sheet>\r\n");
      fprintf(fph,"<meta name=Generator content=\"Microsoft Excel 11\">\r\n");
      fprintf(fph,"<link id=Main-File rel=Main-File href=\"../%s\">\r\n", dest_name.c_str());
      fprintf(fph,"\r\n");
      fprintf(fph,"<script language=\"JavaScript\">\r\n");
      fprintf(fph,"<!--\r\n");
      fprintf(fph,"if (window.name!=\"frTabs\")\r\n");
      fprintf(fph," window.location.replace(document.all.item(\"Main-File\").href);\r\n");
      fprintf(fph,"//-->\r\n");
      fprintf(fph,"</script>\r\n");
      fprintf(fph,"<style>\r\n");
      fprintf(fph,"<!--\r\n");
      fprintf(fph,"A {\r\n");
      fprintf(fph,"    text-decoration:none;\r\n");
      fprintf(fph,"    color:#000000;\r\n");
      fprintf(fph,"    font-size:9pt;\r\n");
      fprintf(fph,"}\r\n");
      fprintf(fph,"-->\r\n");
      fprintf(fph,"</style>\r\n");
      fprintf(fph,"</head>\r\n");
      fprintf(fph,"<body topmargin=0 leftmargin=0 bgcolor=\"#808080\">\r\n");
      fprintf(fph,"<table border=0 cellspacing=1>\r\n");
      fprintf(fph," <tr>\r\n");
      for (unsigned int i=0; i<tableCount; i++)
      {
        fprintf(fph," <td bgcolor=\"#FFFFFF\" nowrap><b><small><small>&nbsp;<a href=\"sheet%03d.htm\" target=\"frSheet\"><font face=\"Arial\" color=\"#000000\">Sheet%d</font></a>&nbsp;</small></small></b></td>\r\n", i+1, i+1);
      }
      fprintf(fph,"\r\n");
      fprintf(fph," </tr>\r\n");
      fprintf(fph,"</table>\r\n");
      fprintf(fph,"</body>\r\n");
      fprintf(fph,"</html>\r\n");
      fclose(fph);
    }


    {
      unsigned int index = 0;
      HtmlTableVector::iterator i;
      for (i=mTables.begin(); i!=mTables.end(); ++i)
      {
        _HtmlTable *table = (*i);
        sprintf(scratch,"%s%s_files\\sheet%03d.htm", root_dir.c_str(), base_name.c_str(), index+1 );
        fph = fopen(scratch,"wb");
        if ( fph )
        {

          fprintf(fph,"<html xmlns:o=\"urn:schemas-microsoft-com:office:office\"\r\n");
          fprintf(fph,"xmlns:x=\"urn:schemas-microsoft-com:office:excel\"\r\n");
          fprintf(fph,"xmlns=\"http://www.w3.org/TR/REC-html40\">\r\n");
          fprintf(fph,"\r\n");
          fprintf(fph,"<head>\r\n");
          fprintf(fph,"<meta http-equiv=Content-Type content=\"text/html; charset=windows-1252\">\r\n");
          fprintf(fph,"<meta name=ProgId content=Excel.Sheet>\r\n");
          fprintf(fph,"<meta name=Generator content=\"Microsoft Excel 11\">\r\n");
          fprintf(fph,"<link id=Main-File rel=Main-File href=\"../%s\">\r\n", dest_name.c_str());
          fprintf(fph,"<link rel=File-List href=filelist.xml>\r\n");
          fprintf(fph,"<link rel=Edit-Time-Data href=editdata.mso>\r\n");
          fprintf(fph,"<link rel=Stylesheet href=stylesheet.css>\r\n");
          fprintf(fph,"<style>\r\n");
          fprintf(fph,"<!--table\r\n");
          fprintf(fph,"	{mso-displayed-decimal-separator:\"\\.\";\r\n");
          fprintf(fph,"	mso-displayed-thousand-separator:\"\\,\";}\r\n");
          fprintf(fph,"@page\r\n");
          fprintf(fph,"	{margin:1.0in .75in 1.0in .75in;\r\n");
          fprintf(fph,"	mso-header-margin:.5in;\r\n");
          fprintf(fph,"	mso-footer-margin:.5in;}\r\n");
          fprintf(fph,"-->\r\n");
          fprintf(fph,"</style>\r\n");
          fprintf(fph,"<![if !supportTabStrip]><script language=\"JavaScript\">\r\n");
          fprintf(fph,"<!--\r\n");
          fprintf(fph,"function fnUpdateTabs()\r\n");
          fprintf(fph," {\r\n");
          fprintf(fph,"  if (parent.window.g_iIEVer>=4) {\r\n");
          fprintf(fph,"   if (parent.document.readyState==\"complete\"\r\n");
          fprintf(fph,"    && parent.frames['frTabs'].document.readyState==\"complete\")\r\n");
          fprintf(fph,"   parent.fnSetActiveSheet(0);\r\n");
          fprintf(fph,"  else\r\n");
          fprintf(fph,"   window.setTimeout(\"fnUpdateTabs();\",150);\r\n");
          fprintf(fph," }\r\n");
          fprintf(fph,"}\r\n");
          fprintf(fph,"\r\n");
          fprintf(fph,"if (window.name!=\"frSheet\")\r\n");
          fprintf(fph," window.location.replace(\"../%s\");\r\n", dest_name.c_str());
          fprintf(fph,"else\r\n");
          fprintf(fph," fnUpdateTabs();\r\n");
          fprintf(fph,"//-->\r\n");
          fprintf(fph,"</script>\r\n");
          fprintf(fph,"<![endif]><!--[if gte mso 9]><xml>\r\n");
          fprintf(fph," <x:WorksheetOptions>\r\n");
          fprintf(fph,"  <x:Panes>\r\n");
          fprintf(fph,"   <x:Pane>\r\n");
          fprintf(fph,"    <x:Number>3</x:Number>\r\n");
          fprintf(fph,"    <x:ActiveRow>3</x:ActiveRow>\r\n");
          fprintf(fph,"    <x:ActiveCol>1</x:ActiveCol>\r\n");
          fprintf(fph,"   </x:Pane>\r\n");
          fprintf(fph,"  </x:Panes>\r\n");
          fprintf(fph,"  <x:ProtectContents>False</x:ProtectContents>\r\n");
          fprintf(fph,"  <x:ProtectObjects>False</x:ProtectObjects>\r\n");
          fprintf(fph,"  <x:ProtectScenarios>False</x:ProtectScenarios>\r\n");
          fprintf(fph," </x:WorksheetOptions>\r\n");
          fprintf(fph,"</xml><![endif]-->\r\n");
          fprintf(fph,"</head>\r\n");
          fprintf(fph,"\r\n");
          fprintf(fph,"<body link=blue vlink=purple>\r\n");
          fprintf(fph,"\r\n");
          table->saveExcel(fph);
/******
          fprintf(fph,"<table x:str border=0 cellpadding=0 cellspacing=0 width=128 style='border-collapse:\r\n");
          fprintf(fph," collapse;table-layout:fixed;width:96pt'>\r\n");
          fprintf(fph," <col width=64 span=2 style='width:48pt'>\r\n");
          fprintf(fph," <tr height=17 style='height:12.75pt'>\r\n");
          fprintf(fph,"  <td height=17 width=64 style='height:12.75pt;width:48pt'>a</td>\r\n");
          fprintf(fph,"  <td width=64 style='width:48pt'>b</td>\r\n");
          fprintf(fph," </tr>\r\n");
          fprintf(fph," <tr height=17 style='height:12.75pt'>\r\n");
          fprintf(fph,"  <td height=17 align=right style='height:12.75pt' x:num>1</td>\r\n");
          fprintf(fph,"  <td align=right x:num>2</td>\r\n");
          fprintf(fph," </tr>\r\n");
          fprintf(fph," <tr height=17 style='height:12.75pt'>\r\n");
          fprintf(fph,"  <td height=17 align=right style='height:12.75pt' x:num>3</td>\r\n");
          fprintf(fph,"  <td align=right x:num>4</td>\r\n");
          fprintf(fph," </tr>\r\n");
          fprintf(fph," <![if supportMisalignedColumns]>\r\n");
          fprintf(fph," <tr height=0 style='display:none'>\r\n");
          fprintf(fph,"  <td width=64 style='width:48pt'></td>\r\n");
          fprintf(fph,"  <td width=64 style='width:48pt'></td>\r\n");
          fprintf(fph," </tr>\r\n");
          fprintf(fph," <![endif]>\r\n");
          fprintf(fph,"</table>\r\n");
***/
          fprintf(fph,"\r\n");
          fprintf(fph,"</body>\r\n");
          fprintf(fph,"\r\n");
          fprintf(fph,"</html>\r\n");
          fclose(fph);
        }
        index++;
      }
    }



#endif
  }


private:
	unsigned int mDisplayOrder;
  std::string      mDocumentName;
  HtmlTableVector  mTables;
  HtmlTableInterface *mInterface;
};

  _HtmlTable::_HtmlTable(const char *heading,HtmlDocument *parent)
  {
//    gTableCount++;
//    printf("Constructed _HtmlTable(%08X) Count:%d\r\n", this,gTableCount );

    _HtmlDocument *hd = static_cast< _HtmlDocument *>(parent);
    mDisplayOrder = hd->getDisplayOrder();

    mHeaderColor = 0x00FFFF;
    mFooterColor = 0xCCFFFF;
    mBodyColor   = 0xCCFFCC;

    mParent = parent;
    if ( heading )
    {
      mHeading = heading;
    }
    mComputeTotals = false;

    mCurrent = 0;
    mParser.ClearHardSeparator(32);
    mParser.ClearHardSeparator(9);
    mParser.SetHard(',');
  }


class MyHtmlTableInterface : public HtmlTableInterface
{
public:

  HtmlDocument * createHtmlDocument(const char *document_name)
  {
    HtmlDocument *ret = 0;

    ret = HTML_NEW(_HtmlDocument)(document_name,this);

    return ret;
  }

  void           releaseHtmlDocument(HtmlDocument *document)      // release a previously created HTML document
  {
    assert(document);
    _HtmlDocument *doc = static_cast< _HtmlDocument *>(document);
    HTML_DELETE(_HtmlDocument,doc);
  }


}; //

}; // end of namespace

namespace nvidia
{
using namespace HTMLTABLE_NVSHARE;

static MyHtmlTableInterface *gInterface=NULL;

HtmlTableInterface *getHtmlTableInterface(void)
{
	if ( gInterface == NULL )
	{
		gInterface = new MyHtmlTableInterface;
	}
  return gInterface;
}

int                 getHtmlMemoryUsage(void)
{
  return gMemTracker.getMemoryUsage();
}

};

#endif