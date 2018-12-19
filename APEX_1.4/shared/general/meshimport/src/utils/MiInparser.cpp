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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#pragma warning(disable:4996)

#include "MiInparser.h"

/** @file inparser.cpp
 * @brief        Parse ASCII text, in place, very quickly.
 *
 * This class provides for high speed in-place (destructive) parsing of an ASCII text file.
 * This class will either load an ASCII text file from disk, or can be constructed with a pointer to
 * a piece of ASCII text in memory.  It can only be called once, and the contents are destroyed.
 * To speed the process of parsing, it simply builds pointers to the original ascii data and replaces the
 * seperators with a zero byte to indicate end of string.  It performs callbacks to parse each line, in argc/argv format,
 * offering the option to cancel the parsing process at any time.
 *
 *
 * By default the only valid seperator is whitespace.  It will not treat commas or any other symbol as a separator.
 * However, you can specify up to 32 'hard' seperators, such as a comma, equal sign, etc. and these will act as valid
 * seperators and come back as part of the argc/argv data list.
 *
 * To use the parser simply inherit the pure virtual base class 'InPlaceParserInterface'.  Define the method 'ParseLine'.
 * When you invoke the Parse method on the InPlaceParser class, you will get an ARGC - ARGV style callback for each line
 * in the source file.  If you return 'false' at any time, it will abort parsing.  The entire thing is stack based, so you
 * can recursively call multiple parser instances.
 *
 * It is important to note.  Since this parser is 'in place' it writes 'zero bytes' (EOS marker) on top of the whitespace.
 * While it can handle text in quotes, it does not handle escape sequences.  This is a limitation which could be resolved.
 * There is a hard coded maximum limit of 512 arguments per line.
 *
 * Here is the full example usage:
 *
 *  InPlaceParser ipp("parse_me.txt");
 *
 *    ipp.Parse(this);
 *
 *  That's it, and you will receive an ARGC - ARGV callback for every line in the file.
 *
 *  If you want to parse some text in memory of your own. (It *MUST* be terminated by a zero byte, and lines seperated by carriage return
 *  or line-feed.  You will receive an assertion if it does not.  If you specify the source data than *you* are responsible for that memory
 *  and must de-allocate it yourself.  If the data was loaded from a file on disk, then it is automatically de-allocated by the InPlaceParser.
 *
 *  You can also construct the InPlaceParser without passing any data, so you can simply pass it a line of data at a time yourself.  The
 *  line of data should be zero-byte terminated.
*/

//==================================================================================

namespace mimp
{

void InPlaceParser::SetFile(const char *fname)
{
	if ( mMyAlloc )
	{
		MI_FREE(mData);
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
			mData = (char *) MI_ALLOC(sizeof(char)*(mLen+1));
			MiI32 read = (MiI32)fread(mData,(size_t)mLen,1,fph);
			if ( !read )
			{
				MI_FREE(mData);
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
		MI_FREE(mData);
	}
}

//==================================================================================
bool InPlaceParser::IsHard(char c)
{
	return mHard[(unsigned char)c] == ST_HARD;
}

//==================================================================================
char * InPlaceParser::AddHard(MiI32 &argc,const char **argv,char *foo)
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
	return mHard[(unsigned char)c] == ST_SOFT;
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
MiI32 InPlaceParser::ProcessLine(MiI32 lineno,char *line,InPlaceParserInterface *callback)
{
	MiI32 ret = 0;

	const char *argv[MAXARGS];
	MiI32 argc = 0;

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


MiI32  InPlaceParser::Parse(const char *str,InPlaceParserInterface *callback) // returns true if entire file was parsed, false if it aborted for some reason
{
  MiI32 ret = 0;

  mLen = (MiI32)strlen(str);
  if ( mLen )
  {
    mData = (char *)MI_ALLOC((MiU32)mLen+1);
    strcpy(mData,str);
    mMyAlloc = true;
    ret = Parse(callback);
  }
  return ret;
}

//==================================================================================
// returns true if entire file was parsed, false if it aborted for some reason
//==================================================================================
MiI32  InPlaceParser::Parse(InPlaceParserInterface *callback)
{
	MiI32 ret = 0;
	MI_ASSERT( callback );
	if ( mData )
	{
		MiI32 lineno = 0;

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
  					MiI32 v = ProcessLine(lineno,begin,callback);
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

		MiI32 v = ProcessLine(lineno,begin,callback);
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
const char ** InPlaceParser::GetArglist(char *line,MiI32 &count)
{
	const char **ret = 0;

	MiI32 argc = 0;

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


};