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

#include "XMLParser.h"

#include <string.h>
#include <float.h>

#define EOL 10

#ifdef __CELLOS_LV2__
#include <safeStdio.h>
#endif

// ------ singleton pattern -----------------------------------------------------------

static XMLParser *gXMLParser = NULL;

XMLParser* XMLParser::getInstance() 
{
	if (gXMLParser == NULL) {
		gXMLParser = new XMLParser();
	}
	return gXMLParser;
}

void XMLParser::destroyInstance() 
{
	if (gXMLParser != NULL) {
		delete gXMLParser;
	}
	gXMLParser = NULL;
}

//---------------------------------------------------------------------
XMLParser::XMLParser()
{
	clearTag();
}

//---------------------------------------------------------------------
XMLParser::~XMLParser()
{
}

//---------------------------------------------------------------------
void XMLParser::clearTag()
{
	tagName.clear();
	vars.clear();
	closed = false;
	endTag = false;
}

//---------------------------------------------------------------------
bool XMLParser::open(std::string filename)
{
	return fs.open(filename.c_str());
}

//---------------------------------------------------------------------
void XMLParser::close()
{
	fs.close();
}

//---------------------------------------------------------------------
bool XMLParser::endOfFile()
{
	return fs.endReached();
}

//---------------------------------------------------------------------
bool XMLParser::readNextTag()
{
	clearTag();

	char sym[1024];

	while (!fs.endReached()) {
		fs.getSymbol(sym);
		if (strlen(sym) > 0 && sym[0] == '<')
			break;
	}
	if (fs.endReached()) return false;
	fs.getSymbol(sym);
	if (strlen(sym) == 0) return false;
	if (sym[0] == '/') {
		endTag = true;
		fs.getSymbol(sym);
	}
	tagName = sym;

	while (!fs.endReached()) {
		fs.getSymbol(sym);
		if (strlen(sym) == 0) return false;
		if (sym[0] == '/') {
			closed = true;
			fs.getSymbol(sym);
			if (strlen(sym) == 0) return false;
		}
		if (sym[0] == '>') break;

		XMLVariable var;
		var.name = sym;
		fs.getSymbol(sym);
		if (strlen(sym) == 0 || sym[0] != '=') return false;
		fs.getSymbol(sym);
		var.value = sym;
		vars.push_back(var);
	}
	return true;
}

// ezm

//---------------------------------------------------------------------
void XMLParser::ezmParseSemantic(int &count)
{
	count = 0;
	ezmTypes.clear();
	ezmAttrs.clear();

	std::string ctype = "";
	std::string semantic = "";

	for (int i = 0; i < (int)vars.size(); i++) {
		if (vars[i].name == "count")
			sscanf_s(vars[i].value.c_str(), "%i", &count);
		else if (vars[i].name == "ctype")
			ctype = vars[i].value;
		else if (vars[i].name == "semantic")
			semantic = vars[i].value;
	}

	std::string s;
	int i = 0;
	while (i < (int)ctype.size()) {
		while (i < (int)ctype.size() && ctype[i] == ' ')
			i++;
		s = "";
		while (i < (int)ctype.size() && ctype[i] != ' ') {
			s += ctype[i]; i++;
		}
		if (!s.empty())
			ezmTypes.push_back(s);
	}
	i = 0;
	while (i < (int)semantic.size()) {
		while (i < (int)semantic.size() && semantic[i] == ' ')
			i++;
		s = "";
		while (i < (int)semantic.size() && semantic[i] != ' ') {
			s += semantic[i]; i++;
		}
		if (!s.empty())
			ezmAttrs.push_back(s);
	}

	// should not happen
	if (ezmTypes.size() > ezmAttrs.size())
		ezmTypes.resize(ezmAttrs.size());
	if (ezmAttrs.size() > ezmTypes.size())
		ezmAttrs.resize(ezmTypes.size());
}

//---------------------------------------------------------------------
void XMLParser::ezmReadSemanticEntry(int nr)
{
	ezmFloats.clear();
	ezmInts.clear();

	if (nr < 0 || nr >= (int)ezmTypes.size())
		return;

	std::string &type = ezmTypes[nr];

	if (type == "x1" || type == "x2" || type == "x4") {
		char sym[32];
		fs.getSymbol(sym);
		int x4;
		sscanf(sym, "%X", &x4);
		ezmInts.push_back(x4);
	}
	else if (type.size() > 0) {
		if (type[0] == 'f') {
			for (int k = 0; k < (int)type.size(); k++) {
				float fsym;
				fs.getFloatSymbol(fsym);
				ezmFloats.push_back(fsym);
			}
		}
		if (type[0] == 'h' || type[0] == 'd' || type[0] == 'c' || type[0] == 'b') {
			for (int k = 0; k < (int)type.size(); k++) {
				int isym;
				fs.getIntSymbol(isym);
				ezmInts.push_back(isym);
			}
		}
	}
}
