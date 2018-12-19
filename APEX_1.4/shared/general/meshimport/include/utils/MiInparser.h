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



#ifndef INPARSER_H
#define INPARSER_H

#include <assert.h>

#define MAXARGS 512


/** @file inparser.h
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
 * You can specify any character to be  a 'hard' seperator, such as an '=' for example and that will come back as a
 * distinct argument string.
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

#include "MiPlatformConfig.h"

namespace mimp
{
class InPlaceParserInterface
{
public:
	virtual MiI32 ParseLine(MiI32 lineno,MiI32 argc,const char **argv) =0;  // return TRUE to continue parsing, return FALSE to abort parsing process
  virtual bool preParseLine(MiI32 /* lineno */,const char * /* line */)  { return false; }; // optional chance to pre-parse the line as raw data.  If you return 'true' the line will be skipped assuming you snarfed it.
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

	InPlaceParser(char *data,MiI32 len)
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
		for (MiI32 i=0; i<256; i++)
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

	void SetSourceData(char *data,MiI32 len)
	{
		mData = data;
		mLen  = len;
		mMyAlloc = false;
	};

	MiI32  Parse(const char *str,InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason
	MiI32  Parse(InPlaceParserInterface *callback); // returns true if entire file was parsed, false if it aborted for some reason

	MiI32 ProcessLine(MiI32 lineno,char *line,InPlaceParserInterface *callback);

	const char ** GetArglist(char *source,MiI32 &count); // convert source string into an arg list, this is a destructive parse.

	void SetHardSeparator(char c) // add a hard separator
	{
		mHard[(unsigned char)c] = ST_HARD;
	}

	void SetHard(char c) // add a hard separator
	{
		mHard[(unsigned char)c] = ST_HARD;
	}

	void SetSoft(char c) // add a hard separator
	{
		mHard[(unsigned char)c] = ST_SOFT;
	}


	void SetCommentSymbol(char c) // comment character, treated as 'end of string'
	{
		mHard[(unsigned char)c] = ST_EOS;
	}

	void ClearHardSeparator(char c)
	{
		mHard[(unsigned char)c] = ST_DATA;
	}


	void DefaultSymbols(void); // set up default symbols for hard seperator and comment symbol of the '#' character.

	bool EOS(char c)
	{
		if ( mHard[(unsigned char)c] == ST_EOS )
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
    mHard[(unsigned char)c] = ST_LINE_FEED;
  }

  bool isLineFeed(char c)
  {
    if ( mHard[(unsigned char)c] == ST_LINE_FEED ) return true;
    return false;
  }

private:

	inline char * AddHard(MiI32 &argc,const char **argv,char *foo);
	inline bool   IsHard(char c);
	inline char * SkipSpaces(char *foo);
	inline bool   IsWhiteSpace(char c);
	inline bool   IsNonSeparator(char c); // non seperator,neither hard nor soft

	bool   mMyAlloc; // whether or not *I* allocated the buffer and am responsible for deleting it.
	char  *mData;  // ascii data to parse.
	MiI32    mLen;   // length of data
	SeparatorType  mHard[256];
	char   mHardString[256*2];
	char           mQuoteChar;
	const char *argv[MAXARGS];
};

};

#endif
