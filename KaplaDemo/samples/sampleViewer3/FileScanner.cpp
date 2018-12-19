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

#include <string.h>
#include <float.h>
#include "FileScanner.h"

#pragma warning(disable : 4996)		//deprecated warnings for string ops not necessary
#define _CRT_SECURE_NO_DEPRECATE

#define EOL 10

// ------------------------------------------------------------------------------------
FileScanner::FileScanner()
// ------------------------------------------------------------------------------------
{
	f = NULL; 
	lineNr = 0; 
	pos = line; 
	strcpy(errorMessage, "");
}

// ------------------------------------------------------------------------------------
FileScanner::~FileScanner()
// ------------------------------------------------------------------------------------
{
	if (f) 
		fclose(f);
}

// ------------------------------------------------------------------------------------
bool FileScanner::equalSymbol(const char *sym, const char *expectedSym, bool caseSensitive)
// ------------------------------------------------------------------------------------
{
	if (caseSensitive) 
		return strcmp(expectedSym, sym) == 0;
	else
#if defined(__BCPLUSPLUS__)
		return strcmpi(expectedSym, sym) == 0;
#elif defined(__WXMAC__)
		return strcasecmp(expectedSym, sym) == 0;
#else
		return _stricmp(expectedSym, sym) == 0;
#endif
}


// ------------------------------------------------------------------------------------
bool FileScanner::open(const char *filename)
// ------------------------------------------------------------------------------------
{
	f = fopen(filename, "rb");
	if (!f) {
		strcpy(errorMessage, "file open error");
		return false;
	}
	fgets(line, maxLineLen, f);
	pos = line;
	lineNr = 1;
	return true;
}

// ------------------------------------------------------------------------------------
void FileScanner::close()
// ------------------------------------------------------------------------------------
{
	if (f) fclose(f);
	f = NULL;
}

// ------------------------------------------------------------------------------------
void FileScanner::getNextLine()
// ------------------------------------------------------------------------------------
{
	fgets(line, maxLineLen, f);
	pos = line;
	lineNr++;
}

bool singleCharSymbol(char ch)
{
	if (ch == '<' || ch == '>') return true;
	if (ch == '(' || ch == ')') return true;
	if (ch == '{' || ch == '}') return true;
	if (ch == '[' || ch == ']') return true;
	if (ch == '/' || ch == '=') return true;
	return false;
}

bool separatorChar(char ch)
{
	if ((unsigned char)ch <= ' ') return true;
	if (ch == ',') return true;
	if (ch == ';') return true;
	return false;
}

// ------------------------------------------------------------------------------------
void FileScanner::getSymbol(char *sym)
// ------------------------------------------------------------------------------------
{
	while (!feof(f)) {
		while (!feof(f) && separatorChar(*pos)) {	// separators
			if (*pos == EOL) 
				getNextLine();
			else pos++;
		}
		if (!feof(f) && *pos == '/' && *(pos+1) == '/') 	// line comment
			getNextLine();
		else if (!feof(f) && *pos == '/' && *(pos+1) == '*') {	// long comment
			pos += 2;
			while (!feof(f) && (*pos != '*' || *(pos+1) != '/')) {
				if (*pos == EOL) 
					getNextLine();
				else pos++;
			}
			if (!feof(f)) pos += 2;
		}
		else break;
	}

	char *cp = sym;
	if (*pos == '\"' && !feof(f)) {		// read string 
		pos++;
		while (!feof(f) && *pos != '\"' && *pos != EOL) {
			*cp++ = *pos++;
		}
	    if (!feof(f) && *pos == '\"') pos++;	// override <"> 
	}
	else {					/* read other symbol */
		if (!feof(f)) {
			if (singleCharSymbol(*pos))		// single char symbol
				*cp++ = *pos++;
			else {							// multiple char symbol
				while (!separatorChar(*pos) && !singleCharSymbol(*pos) && !feof(f)) {
					*cp++ = *pos++;
				}
			}
		}
	}  
	*cp = 0;       
}


// ------------------------------------------------------------------------------------
bool FileScanner::checkSymbol(const char *expectedSym, bool caseSensitive)
// ------------------------------------------------------------------------------------
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

	if (!equalSymbol(sym, expectedSym, caseSensitive)) {
		sprintf(errorMessage, "Parse error line %i, position %i: %s expected, %s read\n", 
			lineNr, (int)(pos - line), expectedSym, sym);
		return false;
	}
	return true;
}

/* ------------------------------------------------------------------- */
bool FileScanner::getIntSymbol(int &i)
/* ------------------------------------------------------------------- */
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

	int n = 0;
	for (int k = 0; k < (int)strlen(sym); k++) {
		if ((sym[k] >= '0') && (sym[k] <= '9')) n++;
		if ((sym[k] == '+') || (sym[k] == '-')) n++;
	}
	if (n < (int)strlen(sym)) { 
		setErrorMessage("integer expected");
		return false;
	}
	sscanf(sym,"%i",&i);
	return true;
}


/* ------------------------------------------------------------------- */
bool FileScanner::getFloatSymbol(float &r)
/* ------------------------------------------------------------------- */
{
	static char sym[maxLineLen+1];
	getSymbol(sym);

#if defined(__BCPLUSPLUS__)
	if (strcmpi(sym, "infinity") == 0) {
#elif defined(__WXMAC__)
  if (strcasecmp(sym, "infinity") == 0) {
#else
	if (_stricmp(sym, "infinity") == 0) {
#endif
		r = FLT_MAX;
		return true;
	}
#if defined(__BCPLUSPLUS__)
	if (strcmpi(sym, "-infinity") == 0) {
#elif defined(__WXMAC__)
  if (strcasecmp(sym, "-�infinity") == 0) {
#else
	if (_stricmp(sym, "-infinity") == 0) {
#endif
		r = -FLT_MAX;
		return true;
	}

	int n = 0;
	for (int k = 0; k < (int)strlen(sym); k++) {
		if ((sym[k] >= '0') && (sym[k] <= '9')) n++;
		if ((sym[k] == '+') || (sym[k] == '-')) n++;
		if ((sym[k] == '.') || (sym[k] == 'e')) n++;
	}
	if (n < (int)strlen(sym)) {
		setErrorMessage("float expected");
		return false;
	}
	sscanf(sym,"%f",&r);
	return true;
}

/* ------------------------------------------------------------------- */
bool FileScanner::getBinaryData(void *buffer, int size)
{
	size_t num = fread(buffer, 1, size, f);
	getNextLine();
	return num == size;

}

/* ------------------------------------------------------------------- */
void FileScanner::setErrorMessage(char *msg)
/* ------------------------------------------------------------------- */
{
	sprintf(errorMessage, "parse error line %i, position %i: %s", 
			lineNr, (int)(pos - line), msg);
}

