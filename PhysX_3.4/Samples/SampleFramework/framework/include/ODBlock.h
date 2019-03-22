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
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  
#ifndef ODBLOCK_H
#define ODBLOCK_H
/*----------------------------------------------------------------------------*\
|
|						     Ageia PhysX Technology
|
|							     www.ageia.com
|
\*----------------------------------------------------------------------------*/

/*
ObjectDescription Scripts
--------------------------
ODScript = Block
Statement = Block | Terminal
Block = indent '{' {Statement} '}'  
Terminal = ident ';'

Comments:
	# = line comment
	/ *  * / = multiline comment.  The / character cannot be used in identifiers.

idents may be enclosed in quotes, and should be unique to facilitate searching.

In a typical application, program would look for known Blocks, and read out its user set terminal(s).
Particular users define semantics:

SHIPFILE 
	{
	Shipname
		{
		Client
			{
			ShipModel
				{
				MeshFile 
					{
					Filename;
					lodlevels;
					}
				Texturefile 
					{
					Filename;
					}
				}
			CockpitModel
				{
				...
				}
			}
		Server
			{
			...
			}
		}
	}
*/
#include <stdio.h>
#include "FrameworkFoundation.h"
#include "SampleArray.h"

#define OD_MAXID 30								//max identifier length
class ODBlock; 
typedef SampleFramework::Array<ODBlock *> ODBlockList;


class ODBlock
/*-------------------------\
| Block = indent '{' {Statement} '}'  
| Terminals are simply empty blocks
|
|
\-------------------------*/
	{
	class ODSyntaxError 
		{
		public:
		enum Error { ODSE_UNEX_QUOTE, ODSE_UNEX_OBRACE, ODSE_UNEX_CBRACE, ODSE_UNEX_LITERAL,ODSE_UNEX_EOF,ODSE_ENC_UNKNOWN };
		private:
		Error err;
		public:
		ODSyntaxError(Error e) {err = e;}
		const char * asString();
		};
	enum State {WAIT_IDENT,IDENT,WAIT_BLOCK,BLOCK};
	char identifier[128];
	unsigned identSize;							//size of above array.
	bool bTerminal;
	ODBlockList subBlocks;	
	ODBlockList::Iterator termiter;				//iterator for reading terminals


	public:
	ODBlock();									//create a new one
	~ODBlock();
	bool loadScript(SampleFramework::File* fp);
	bool saveScript(SampleFramework::File* writeP, bool bQuote);//saves this block to scipt file.  set bQuote == true if you want to machine parse output.

	//reading:
	const char * ident();
	inline unsigned numSubBlocks() {return subBlocks.size(); }//returns number of sub blocks
	bool isTerminal();							//resets to first statement returns false if its a terminal == no contained Blocks
	
	//writing:	
	void ident(const char *);						//identifier of the block
	void addStatement(ODBlock &);

	//queries:  return true in success
	ODBlock * getBlock(const char * identifier,bool bRecursiveSearch=false);	//returns block with given identifier, or NULL.

	void reset();								//prepares to get first terminal or sub block of current block

	//getting terminals:
	bool moreTerminals();						//returns true if more terminals are available
	char * nextTerminal();					//returns a pointer to the next immediate terminal child of current block's identifier string.  

	//getting terminals:
	bool moreSubBlocks();						//returns true if more sub blocks (including terminals) are available
	ODBlock * nextSubBlock();					//returns a pointer to the next sub block.  

	// hig level macro functs, return true on success: (call for obj containing:)
	bool getBlockInt(const char * ident, int* p = 0, unsigned count = 1);	//reads blocks of form:		ident{ 123;}
	bool getBlockU32(const char * ident, physx::PxU32* p = 0, unsigned count = 1);		//reads blocks of form:		ident{ 123;}

	bool getBlockString(const char * ident, const char **);		//of form:				ident{abcdef;}
	bool getBlockStrings(const char * ident, const char **, unsigned  count);		//of form:				ident{abcdef; abcdef; ...}

	bool getBlockFloat(const char * ident, float * p = 0);		//of form:				ident{123.456;}
	bool getBlockFloats(const char * ident, float *, unsigned  count);//form:		ident{12.3; 12.3; 12.3; ... };

	bool addBlockFloats(const char * ident, float *, unsigned  count);
	bool addBlockInts(const char * ident, int *, unsigned  count);

	//errors
	static const char * lastError;
	};


#endif //ODBLOCK_H

