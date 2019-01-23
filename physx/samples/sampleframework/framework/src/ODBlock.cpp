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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.

/*-------------------------------------------------------------*\
|																|
|	ODBlock class - object description script					|
|																|
| Copyright (C) 1999 Adam Moravanszky							|
|																|
|                                                               |
\*-------------------------------------------------------------*/
/*-Revision----------------------\
| At: 10/3/99 5:51:37 PM
|
| Changed vector container from 
| storing val to refernce.
\-------------------------------*/
/*-Revision----------------------\
| At: 5/18/00 9:29:43 PM
| changed static sized indentifiers
| to arbitrary dynamic identifiers.
|
\-------------------------------*/
/*-Revision----------------------\
| At: 6/19/02 
| added multiline comments.
|
|
\-------------------------------*/
/*-Revision----------------------\
| At: 5/19/03 
| fixed commend bug with 
| file > 0xffff chars.
|
\-------------------------------*/
/*-Revision----------------------\
| At: 19/10/09 
| removed encryption
| converted to FILE * usage.
|
\-------------------------------*/
#include "ODBlock.h"
#include "PxTkFile.h"

const char * ODBlock::lastError = NULL;


const char * ODBlock::ODSyntaxError::asString()
{
	static const char unex1[] = "Quote unexpected.";
	static const char unex2[] = "Opening brace unexpected.";
	static const char unex3[] = "Closing brace unexpected.";
	static const char unex4[] = "Literal or semicolon unexpected.";
	static const char unex5[] = "Unexpected end of file found.";
	static const char encun[] = "Unknown encryption algo code.";
	switch (err)
	{
	case ODSE_UNEX_QUOTE:
		return unex1;
	case ODSE_UNEX_OBRACE:
		return unex2;
	case ODSE_UNEX_CBRACE:
		return unex3;
	case ODSE_UNEX_LITERAL:
		return unex4;
	case ODSE_UNEX_EOF:
		return unex5;
	case ODSE_ENC_UNKNOWN:
		return encun;
	default:
		return NULL;
	}
}

static inline bool isNewLine(char c)
{
	return c == '\n' || c == '\r';
}

bool ODBlock::loadScript(SampleFramework::File * fp)			//loads block from script, including subbocks
{
	int c;
	unsigned currindex = 0;
	bool bQuote = false;					//if we're inside a pair of quotes
	identifier[0] = '\0';
	identSize = 0;
	bTerminal = true;
	termiter = NULL;
	bool ignoreWhiteSpace = true;

	State state;

	state = WAIT_IDENT;


	//0) read possible comments starting with # to end of line
	//1) read our identifier. -->forget quotes for now.

	while(!feof(fp))
	{
		do
		{
			c = fgetc(fp);
		}
		while (ignoreWhiteSpace && ((c == ' ') || (c == '\t') || isNewLine((char)c)) && !feof(fp));

		if  (c == '"')
		{
			if (state == BLOCK)
			{
				bTerminal = false;
				ungetc(c, fp);
				ODBlock * child = new ODBlock();
				addStatement(*child);
				if (!child->loadScript(fp))
					return false;
			}
			else
			{
				if (state != WAIT_IDENT && state != IDENT) 
				{
					lastError = ODSyntaxError(ODSyntaxError::ODSE_UNEX_QUOTE).asString();
					return false;
				}
				ignoreWhiteSpace = bQuote;
				bQuote = !bQuote;
				state = IDENT;
			}
		}
		else
		{
			if (bQuote)
			{
				PX_ASSERT(state == IDENT);
				//if (currindex >= identSize)
				//	{
				//	identSize = 2*currindex + 4;
				//	char * newi = new char [identSize+1];
				//	if (identifier)
				//		strcpy(newi,identifier);
				//	delete [] identifier;
				//	identifier = newi;
				//	}
				identifier[currindex] = (char) c;
				identifier[currindex+1] = 0;
				currindex++;
			}
			else
			{
				switch (c)
				{
				case '#':
					while(!isNewLine((char)fgetc(fp))) ; //read and discard line
					break;
				case '/':
					while(fgetc(fp) != '/') ; //read and discard comment
					break;
				case '{': 
					if (state != WAIT_BLOCK && state != IDENT) 
					{
						lastError = ODSyntaxError(ODSyntaxError::ODSE_UNEX_OBRACE).asString();//identifier can be followed by block w/o whitespace inbetween
						return false;
					}
					state = BLOCK;
					break;
				case '}':
					if (state != BLOCK) 
					{
						lastError = ODSyntaxError(ODSyntaxError::ODSE_UNEX_CBRACE).asString();
						return false;
					}
					return true;
				default:
					//text
					//our identifier?
					if (state == BLOCK)
					{
						bTerminal = false;
						ungetc(c, fp);
						ODBlock * child = new ODBlock();
						addStatement(*child);
						if (!child->loadScript(fp))
							return false;
					}
					else
					{
						if (state != WAIT_IDENT && state != IDENT) 
						{
							lastError = ODSyntaxError(ODSyntaxError::ODSE_UNEX_LITERAL).asString();
							return false;
						}
						if (state == IDENT && c == ';')		//terminal ended
							return true;
						state = IDENT;
						//if (currindex >= identSize)
						//	{
						//	identSize = 2*currindex + 4;
						//	char * newi = new char [identSize+1];
						//	if (identifier)
						//		strcpy(newi,identifier);
						//	delete [] identifier;
						//	identifier = newi;
						//	}
						identifier[currindex] = (char)c;
						identifier[currindex+1] = 0;
						currindex++;
					}
				}
			}
		}
	}
	//eof and block didn't close
	lastError = ODSyntaxError(ODSyntaxError::ODSE_UNEX_EOF).asString();
	return false;
}

ODBlock::ODBlock()
{
	//create empty block
	identifier[0] = '\0';
	identSize = 0;
	bTerminal = true;
	termiter = NULL;
	subBlocks.reserve(32);
}

ODBlock::~ODBlock()
{
	//free the contents
	ODBlockList::Iterator i;
	for (i =  subBlocks.begin(); i != subBlocks.end(); ++i)
		delete (*i);
	//free the pointers
	subBlocks.clear();

	//delete [] identifier;
	identifier[0] = '\0';
	identSize = 0;
}


bool ODBlock::saveScript(SampleFramework::File * fp,bool bQuote)
{
	static int tablevel = 1;
	static int retVal = 0;
	int j;
	//save the script in said file  should be stream!!
	for (j=0; j<tablevel-1; j++)
		retVal = fprintf(fp,"\t"); 
	if (bQuote) 
		retVal = fprintf(fp,"\"");
	if (identifier)
		retVal = fprintf(fp, "%s", identifier);
	else
		retVal = fprintf(fp,"_noname");
	if (bQuote) 
		retVal = fprintf(fp,"\"");
	if (!bTerminal)
	{
		retVal =fprintf(fp,"\n");
		for (j=0; j<tablevel; j++) 
			retVal = fprintf(fp,"\t");	
		retVal = fprintf(fp,"{\n");
		tablevel ++;		
		for (physx::PxU32 i = 0; i < subBlocks.size(); ++i)
			(subBlocks[i])->saveScript(fp,bQuote);
		tablevel --;
		for (j=0; j<tablevel; j++) 
			retVal = fprintf(fp,"\t");
		retVal = fprintf(fp,"}\n");
	}
	else 
		retVal = fprintf(fp,";\n");

	PX_ASSERT( retVal >= 0 );
	// added this return to make this compile on snc & release
	return retVal >= 0;
}


const char * ODBlock::ident()
{
	static char noname[] = "_noname";
	if (identifier)
		return identifier;
	else
		return noname;
}


bool ODBlock::isTerminal()
{
	return bTerminal;
}


void ODBlock::ident(const char * i)
{
	if (!i) return;
	strcpy(identifier,i);
}


void ODBlock::addStatement(ODBlock &b)
{
	bTerminal = false;
	subBlocks.pushBack(&b);
}

void ODBlock::reset()								//prepares to get first immediate terminal child of current block
{
	termiter = subBlocks.begin();
}

bool ODBlock::moreTerminals()						//returns true if more terminals are available
{
	//skip any nonterminals
	while ( (termiter != subBlocks.end()) && !(*termiter)->bTerminal)
		++termiter;
	return  termiter != subBlocks.end();			//return true if not the end yet
}

char * ODBlock::nextTerminal()								//returns a pointer to the next terminal string.  
{
	char *  s = (*termiter)->identifier;
	++termiter;

	static char noname[] = "_noname";
	if (s)
		return s;
	else
		return noname;
}

bool ODBlock::moreSubBlocks()						//returns true if more sub blocks are available
{
	return  termiter != subBlocks.end();			//return true if not the end yet
}
ODBlock * ODBlock::nextSubBlock()					//returns a pointer to the next sub block.  
{
	ODBlock * b =  (*termiter);
	++termiter;
	return b;
}


ODBlock * ODBlock::getBlock(const char * ident,bool bRecursiveSearch)		//returns block with given identifier, or NULL.
{
	ODBlock * b;
	if (identifier && ident && strncmp(identifier,ident,OD_MAXID) == 0)
		return this;
	else
	{
		if (bTerminal) 
			return NULL;
		else
		{
			ODBlockList::Iterator i;
			for (i =  subBlocks.begin(); i != subBlocks.end(); ++i)
				if (bRecursiveSearch)
				{
					b = (*i)->getBlock(ident,true);
					if (b) 
						return b;
				}
				else
				{
					if ((*i)->identifier && ident && strncmp((*i)->identifier,ident,OD_MAXID) == 0)
						return (*i);
				}
				return NULL;
		}
	}

}

// hig level macro functs, return true on success:
bool ODBlock::getBlockInt(const char * ident, int* p, unsigned count)		//reads blocks of form:		ident{ 123; 123; ... }
{
	ODBlock* temp = getBlock(ident);
	if (temp)
	{
		temp->reset();
		for(; count && temp->moreTerminals(); --count)
			if(p)	
				sscanf(temp->nextTerminal(),"%d", p++);

		return !count;
	}
	return false;
}

// hig level macro functs, return true on success:
bool ODBlock::getBlockU32(const char * ident, physx::PxU32* p, unsigned count)		//reads blocks of form:		ident{ 123;}
{
	ODBlock* temp = getBlock(ident);
	if (temp)
	{
		temp->reset();
		for(; count && temp->moreTerminals(); --count)
			if(p)	
				sscanf(temp->nextTerminal(),"%u", p++);

		return !count;
	}
	return false;
}

bool ODBlock::getBlockString(const char *  ident, const char **  p)		//of form:				ident{abcdef;}
{
	ODBlock * temp = getBlock(ident);
	if (temp)
	{
		temp->reset(); 
		if (temp->moreTerminals())	
		{ 
			*p = temp->nextTerminal();
			return true; 
		}
	}
	return false;
}
bool ODBlock::getBlockStrings(const char *  ident, const char **  p, unsigned numStrings)		//of form:				ident{abcdef; abcdef;...}
{
	ODBlock * temp= getBlock(ident);
	if (temp)
	{
		temp->reset(); 
		for (unsigned int n=0; n<numStrings; n++)
		{
			if (temp->moreTerminals())	
			{ 
				p[n] = temp->nextTerminal();
			}
		}
	}
	return false;
}
bool ODBlock::getBlockFloat(const char *  ident, float *p)		//of form:				ident{123.456;}
{
	ODBlock * temp = getBlock(ident);
	if (temp)
	{
		temp->reset(); 
		if (temp->moreTerminals())	
		{ 
			if(p) sscanf(temp->nextTerminal(),"%f",p); 
			return true; 
		}
	}
	return false;
}
bool ODBlock::getBlockFloats(const char *  ident, float *p, unsigned numfloats)//form:		ident{12.3; 12.3; 12.3; ... };
{
	ODBlock * temp = getBlock(ident);
	if (temp)
	{
		temp->reset();
		for (unsigned int n=0; n<numfloats; n++)
			if (temp->moreTerminals())	sscanf(temp->nextTerminal(),"%f",&(p[n]));

		return true;
	}
	return false;
}

bool ODBlock::addBlockFloats(const char * ident, float *f, unsigned  numfloats)
{
	char buf[32];
	ODBlock & newBlock = *new ODBlock();

	addStatement(newBlock);
	newBlock.ident(ident);

	for (unsigned i = 0; i < numfloats; i++)
	{
		ODBlock & floatBlock = *new ODBlock(); 
		newBlock.addStatement(floatBlock);

		//_gcvt_s(buf, 32, f[i],5); //gcc doesn't support this
		sprintf(buf, "%.5f", f[i]);

		floatBlock.ident(buf);
	}
	return true;
}

bool ODBlock::addBlockInts(const char * ident, int *f, unsigned  numInts)
{
	char buf[32];
	ODBlock & newBlock = *new ODBlock();

	addStatement(newBlock);
	newBlock.ident(ident);

	for (unsigned i = 0; i < numInts; i++)
	{
		ODBlock & floatBlock = *new ODBlock(); 
		newBlock.addStatement(floatBlock);
		//_itoa_s(f[i], buf, 32, 10); //gcc doesn't support this
		sprintf(buf, "%d", f[i]);
		floatBlock.ident(buf);
	}
	return true;
}

