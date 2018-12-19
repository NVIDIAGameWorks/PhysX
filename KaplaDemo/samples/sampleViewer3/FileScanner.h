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

#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <stdio.h>

#define EOL 10
#define MAX_BONDS 5

#ifdef __BCPLUSPLUS__
#define STRCMPI(s, t) (strcmpi(s, t))
#elif defined (__WXWINDOWS25__) && defined (__WXMAC__) && defined (__WXMAC_XCODE__)
#define STRCMPI(s, t) (strcasecmp(s, t))
#else
#define STRCMPI(s, t) (_stricmp(s, t))
#endif

#ifdef __BCPLUSPLUS__
#define STRNCMPI(s, t, m) (strncmpi(s, t, m))
#elif defined (__WXWINDOWS25__) && defined (__WXMAC__) && defined (__WXMAC_XCODE__)
#define STRNCMPI(s, t, m) (strncasecmp(s, t, m))
#else
#define STRNCMPI(s, t, m) (_strnicmp(s, t, m))
#endif


// ------------------------------------------------------------------------------------
class FileScanner {
// ------------------------------------------------------------------------------------
public:
	FileScanner(); 
	~FileScanner();

	bool open(const char *filename);
	void close();

	void getSymbol(char *sym);
	bool checkSymbol(const char *expectedSym, bool caseSensitive = false);
	bool getIntSymbol(int &i);
	bool getFloatSymbol(float &r);

	bool getBinaryData(void *buffer, int size);  // must start after newline

	int  getLineNr() const { return lineNr; }
	int  getPositionNr() const { return (int)(pos - line); }

	bool endReached() const { if (feof(f)) return true; else return false; }

	void setErrorMessage(char *msg);

	char errorMessage[256];

	// ------------------------------------------------------
	static bool equalSymbol(const char *sym, const char *expectedSym, bool caseSensitive = false);
	static const int maxLineLen = 1024;

private:
	void getNextLine();
	FILE *f;
	int  lineNr;
	char line[maxLineLen+1];
	char *pos;
};

#endif