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

#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <stdio.h>
#include <string>
#include <vector>
#include "FileScanner.h"

#define XML_LINE_MAX 1024


// -------------------------------------------------------------------------
struct XMLVariable {
	std::string name;
	std::string value;
};

// -------------------------------------------------------------------------
class XMLParser {
public: 
	// singleton pattern
	static XMLParser* getInstance();
	static void destroyInstance();

	bool open(std::string filename);
	void close();
	bool endOfFile();
	bool readNextTag();

	std::string tagName;
	std::vector<XMLVariable> vars;
	bool closed;
	bool endTag;

	FileScanner &getFileScanner() { return fs; }

	// ezm
	void ezmParseSemantic(int &count);
	void ezmReadSemanticEntry(int nr);

	std::vector<std::string> ezmAttrs;
	std::vector<std::string> ezmTypes;
	std::vector<float> ezmFloats;
	std::vector<int> ezmInts;

private:
	XMLParser();
	~XMLParser();

	void clearTag();
	FileScanner fs;
};

#include <stdio.h>

#define FILE_SCANNER_LINE_MAX 1024



#endif