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

#include <ctype.h>
#include "SamplePreprocessor.h"
#include "SampleConsole.h"
#include "RendererMemoryMacros.h"
#include "Renderer.h"
#include "PsString.h"
#include "PsUtilities.h"
#include "SampleBaseInputEventIds.h"
#include <SampleUserInputIds.h>

#include "SampleUserInputDefines.h"

using namespace SampleRenderer;
using namespace SampleFramework;

#define NB_LINES	14

// EXIT: a basic command to hide the console
// Usage: exit
void Console::BasicCmdexit(Console* console, const char* text, void* user_data)
{
	console->setActive(false);
}

// CLS: a basic command to clear the console
// Usage: cls
void Console::BasicCmdcls(Console* console, const char* text, void* user_data)
{
	console->clear();
}

// PROMPT: a basic command to set the prompt
// Usage: prompt [text]
void Console::BasicCmdSetPrompt(Console* console, const char* text, void* user_data)
{
	console->setPrompt(text);
}

// CMDLIST: a basic command to display a list of all possible commands
// Usage:	cmdlist				<= display all possible commands
//			cmdlist [command]	<= check whether a command exists or not
void Console::BasicCmdcmdlist(Console* console, const char* text, void* user_data)
{
	ConsoleCommand* pcmd;
	if(!text)
	{
		for(int i=0;i<256;i++)
		{
			pcmd = console->mCmds[i];
			while(pcmd)
			{
				console->out(pcmd->fullcmd);
				pcmd = pcmd->next;
			}
		}
	}
	else
	{
		int i = text[0];
		if( (i >= 'A') && (i<='Z') )
			i -= 'A' - 'a';
		pcmd = console->mCmds[i];
		while(pcmd)
		{
			if(Ps::strncmp(pcmd->fullcmd, text, strlen(text)) == 0)
				console->out(pcmd->fullcmd);
			pcmd = pcmd->next;
		}
	}
}

// CMDHIST: a basic command to display command history
// Usage: cmdhist
void Console::BasicCmdcmdhist(Console* console, const char* text, void* user_data)
{
	long index = console->mNewcmd - console->mNumcmdhist;
	for(long i=0;i<console->mNumcmdhist;i++)
	{
		if( index > CONSOLE_MAX_HIST )
			index -= CONSOLE_MAX_HIST;
		console->out(console->mCmdhist[index] );
		index++;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Constructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Console::Console(SampleFramework::SamplePlatform* plt) :
	mViewBottom	(0),
	mNewline	(0),
	mIsActive	(false)
{
	strcpy(mLastChar, "-");
	strcpy(mPrompt, ">");
	resetCol();

	mNbCmds			= 0;
	mNewcmd			= 0;
	mNumcmdhist		= 0;
	mCurcmd			= 0;
	mUserData		= 0;

	for(PxU32 i=0;i<CONSOLE_MAX_COMMAND_NB;i++)
		mCmds[i] = NULL;

	// Create console
	addCmd("exit",		BasicCmdexit);
	addCmd("cmdlist",	BasicCmdcmdlist);
	addCmd("cls",		BasicCmdcls);
	addCmd("cmdhist",	BasicCmdcmdhist);
	addCmd("prompt",	BasicCmdSetPrompt);

	clear();
	cmdClear();

	out("PhysX Samples console");
	out("");
	out("Type cmdlist to display all possible commands.");
	out("Use PageUp / PageDown to scroll the window.");
	out("Use arrow keys to recall old commands.");
	out("Use ESC to exit.");
	out("");

	advance();
	strcpy(mBuffer[mNewline].mText, mPrompt);
	strcat(mBuffer[mNewline].mText, mLastChar);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Destructor.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Console::~Console()
{
	destroy();
}

void Console::resetCol()
{
	mCol = (PxI32)strlen(mPrompt);
}

void Console::setPrompt(const char* text)
{
	if(!text)
		return;
	const PxU32 Length = (PxU32)strlen(text);
	if(Length>255)
		return;
	strcpy(mPrompt, text);
}

//  Advance the console one line
void Console::advance()
{
	mNewline--;
	if(mNewline<0)
		mNewline += CONSOLE_MAX_ROW;

	mBuffer[mNewline].mText[0] = '\0';
	mViewBottom = mNewline;
}

// Clear the console text buffer
void Console::clear()
{
	for(PxU32 i=0;i<CONSOLE_MAX_ROW;i++)
	{
		for(PxU32 j=0;j<CONSOLE_MAX_COL;j++)
			mBuffer[i].mText[j] = '\0';

		mBuffer[i].mColor = RendererColor(255,255,255);
	}
	mNewline = 0;
	mViewBottom = 0;
}

// Clear the console text buffer
void Console::cmdClear()
{
	for(PxU32 i=0;i<CONSOLE_MAX_HIST;i++)
		for(PxU32 j=0;j<CONSOLE_MAX_COL;j++)
			mCmdhist[i][j] = '\0';
	mNewcmd = 0;
	mNumcmdhist = 0;	
	mCurcmd = 0;
}

//  Write a string to the console
void Console::out(const char* string)
{
	advance();

	if(string)
	{
		size_t Length = strlen(string);
		if(Length>=CONSOLE_MAX_COL-1)
		{
			PX_ASSERT(0);
			strcpy(mBuffer[mNewline].mText, "CONSOLE LINE TOO LONG!");
		}
		else
			strcpy(mBuffer[mNewline].mText, string);
	}
}

//  Process an instruction
//		called after the user hits enter
void Console::process()
{
	// Discard prompt
	char cmd[1024];
	long Index = (long)strlen(mPrompt);
	strcpy(cmd, &mBuffer[mNewline].mText[Index]);

	// Keep track of command in history buffer
	strcpy(mCmdhist[mNewcmd], cmd);
	mNewcmd = mNewcmd % CONSOLE_MAX_HIST;
	mNewcmd++;
	mCurcmd = 0;
	mNumcmdhist++;
	if(mNumcmdhist>CONSOLE_MAX_HIST)	mNumcmdhist = CONSOLE_MAX_HIST;

	mBuffer[mNewline].mColor = 0xffeeeeee;

	// Extract param and execute command
	char* cmdparam;
	cmdparam = strchr(cmd, ' ');
	if(cmdparam)
	{
		*cmdparam=0;
		cmdparam++;
	}

	if(!execCmd(cmd, cmdparam))
		out("Invalid command");
}



//		up and down arrow
//		for command history
void Console::cmdHistory()
{
	if( mCurcmd != -1 )
	{
		char buf[256];
		long cmdnum;
		strcpy(buf, mPrompt);
		cmdnum = mNewcmd - mCurcmd;
		if( cmdnum < 0 )
			cmdnum += CONSOLE_MAX_HIST;
		strcat(buf, mCmdhist[cmdnum]);
		strcat(buf, mLastChar);
		strcpy(mBuffer[mNewline].mText, buf);
		mCol = (PxI32)strlen(buf)-1;
	}
}

bool Console::findBestCommand(char* best_command, const char* text, PxU32& tabIndex) const
{
	if(!text || !best_command)
		return false;

	const size_t length = strlen(text);
	if(length>1023)
		return false;

	char tmp[1024];
	strcpy(tmp, text);
	Ps::strlwr(tmp);

	const unsigned char i = tmp[0];

	ConsoleCommand* FirstCommand = NULL;
	ConsoleCommand* BestCommand = NULL;
	ConsoleCommand* pcmd = mCmds[i];
	PxU32 currentIndex = 0;
	while(pcmd && !BestCommand)
	{
		char tmp2[1024];
		strcpy(tmp2, pcmd->fullcmd);
		Ps::strlwr(tmp2);
		if(Ps::strncmp(tmp, tmp2, length)== 0)
		{
			if(!currentIndex)
				FirstCommand = pcmd;

			if(currentIndex>=tabIndex)
				BestCommand = pcmd;
			currentIndex++;
		}
		pcmd = pcmd->next;
	}

	if(BestCommand)
	{
		tabIndex++;
		strcpy(best_command, BestCommand->fullcmd);
		return true;
	}

	tabIndex = 0;
	if(currentIndex && FirstCommand)
	{
		tabIndex++;
		strcpy(best_command, FirstCommand->fullcmd);
		return true;
	}
	return false;
}

// Try to execute a command
bool Console::execCmd(const char* cmd, const char* param)
{
	if(!cmd)
		return false;

	int HashIndex = cmd[0];
	HashIndex = tolower(HashIndex);

	ConsoleCommand* pcmd = mCmds[HashIndex];

	while(pcmd)
	{
		if(Ps::stricmp(cmd, pcmd->fullcmd) == 0)
		{
			pcmd->function(this, param, mUserData);
			return true;
		}
		else pcmd = pcmd->next;
	}
	return false;
}

// Destroy the console
void Console::destroy()
{
	// clean up command list
	for(PxU32 i=0;i<256;i++)
	{
		if(mCmds[i])
		{
			ConsoleCommand* pcmd = mCmds[i];
			while(pcmd)
			{
				ConsoleCommand* next = pcmd->next;
				DELETESINGLE(pcmd);
				pcmd = next;
			}
		}
	}
}

// Add a command
void Console::addCmd(const char* full_cmd, void (*function)(Console*, const char *, void*))
{
	if(!full_cmd)										return;		// Command must be defined
	if(!function)										return;		// Function must be defines
	if(strlen(full_cmd)>=CONSOLE_MAX_COMMAND_LENGTH)	return;
	if(mNbCmds==CONSOLE_MAX_COMMAND_NB)					return;
	mNbCmds++;

	int HashIndex = full_cmd[0];
	HashIndex = tolower(HashIndex);

	ConsoleCommand* pcmd = mCmds[HashIndex];
	if(!pcmd)
	{
		mCmds[HashIndex] = SAMPLE_NEW(ConsoleCommand);
		pcmd = mCmds[HashIndex];
	}
	else
	{
		while(pcmd->next)
		{
			pcmd = pcmd->next;
		}
		pcmd->next = SAMPLE_NEW(ConsoleCommand);
		pcmd = pcmd->next;
	}
	strcpy(pcmd->fullcmd, full_cmd);
	pcmd->function = function;
	pcmd->next = NULL;
}

bool Console::render(Renderer* rnd)
{
	if(!rnd)
		return false;
	if(!mIsActive)
		return true;

	const PxU32 NbLines = NB_LINES;
	const PxU32 FntHeight = 14;

	PxU32 width, height;
	rnd->getWindowSize(width, height);

	const RendererColor backColor(3, 3, 39);

	ScreenQuad sq;
	sq.mX0				= 0.0f;
	sq.mY0				= 20.0f/float(height);
	sq.mX1				= 1.0f;
	sq.mY1				= (20.0f + float((NbLines+2)*FntHeight))/float(height);
	sq.mLeftUpColor		= backColor;
	sq.mRightUpColor	= backColor;
	sq.mLeftDownColor	= backColor;
	sq.mRightDownColor	= backColor;
	sq.mAlpha			= 0.8f;

	rnd->drawScreenQuad(sq);

	PxU32 TextY = 24 + NbLines*FntHeight;

	const PxReal scale = 0.4f;
	const PxReal shadowOffset = 0.0f;

	long temp = mViewBottom;
	for(PxU32 i=0;i<NbLines;i++)
	{
		rnd->print(10, TextY, mBuffer[temp].mText, scale, shadowOffset, mBuffer[temp].mColor);

		temp = (temp + 1) % CONSOLE_MAX_ROW;
		TextY -= FntHeight;		//size should come from renderer
	}
	return true;
}


// Process a single character
static bool gTabMode = false;
static PxU32 gTabIndex = 0;
static char gTabCmd[1024];
void Console::in(PxU32 wparam)
{
	if(!mIsActive)
		return;

	if ((wparam >= 'a' && wparam <= 'z') || (wparam >= 'A' && wparam <= 'Z') || (wparam >= '0' && wparam <= '9') || wparam == ' ' || wparam == '.' || wparam == '-' || wparam == '_')
	{
		gTabMode = false;
		if(mCol >= CONSOLE_MAX_COL-2)	// We need 2 extra characters for the cursor and the final 0
			return;
		mBuffer[mNewline].mText[mCol++] = (char)wparam;	// Append new character
		mBuffer[mNewline].mText[mCol] = mLastChar[0];	// Append cursor
		mBuffer[mNewline].mText[mCol+1] = '\0';
	}
}

void Console::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	//digital keyboard events
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_OPEN,				WKEY_TAB,		OSXKEY_TAB,			LINUXKEY_TAB		);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_ESCAPE,				WKEY_ESCAPE,	OSXKEY_ESCAPE,		LINUXKEY_ESCAPE		);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_ENTER,				WKEY_RETURN,	OSXKEY_RETURN,		LINUXKEY_RETURN		);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_BACKSPACE,			WKEY_BACKSPACE,	OSXKEY_BACKSPACE,	LINUXKEY_BACKSPACE	);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_LIST_COMMAND_UP,	WKEY_UP,		OSXKEY_UP,			LINUXKEY_UP			);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_LIST_COMMAND_DOWN,	WKEY_DOWN,		OSXKEY_DOWN,		LINUXKEY_DOWN		);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_SCROLL_UP,			WKEY_PRIOR,		OSXKEY_PRIOR,		LINUXKEY_PRIOR		);
	DIGITAL_INPUT_EVENT_DEF(CONSOLE_SCROLL_DOWN,		WKEY_NEXT,		OSXKEY_NEXT,		LINUXKEY_NEXT		);
}

//return true if we processed the key
void Console::onDigitalInputEvent(const SampleFramework::InputEvent& ie, bool val)
{
	//if (val)
	{

		if (!mIsActive)
		{

			if (ie.m_Id == CONSOLE_OPEN)
			{
				mIsActive = true;
				return;
			}
		}
		else 
		{
			if (!val)
			{
				switch (ie.m_Id)
				{
				case CONSOLE_OPEN:
					if(!gTabMode)
					{
						gTabMode = true;

						// Discard last character
						mBuffer[mNewline].mText[mCol] = '\0';

						// Discard prompt
						long Index = (long)strlen(mPrompt);
						strcpy(gTabCmd, &mBuffer[mNewline].mText[Index]);
					}
					char BestCmd[1024];
					if(findBestCommand(BestCmd, gTabCmd, gTabIndex))
					{
						strcpy(mBuffer[mNewline].mText, mPrompt);
						strcat(mBuffer[mNewline].mText, BestCmd);
						strcat(mBuffer[mNewline].mText, mLastChar);
						mCol = PxI32(strlen(mPrompt) + strlen(BestCmd));
					}
					else
					{
						gTabMode = false;
						mBuffer[mNewline].mText[mCol] = mLastChar[0];	// Append cursor
						mBuffer[mNewline].mText[mCol+1] = '\0';
					}
					break;
				case CONSOLE_BACKSPACE:
					gTabMode = false;
					if(mCol>(long)strlen(mPrompt))
					{
						mBuffer[mNewline].mText[mCol] = '\0';
						mBuffer[mNewline].mText[mCol-1] = mLastChar[0];
						mCol--;
					}
					break;					
				case CONSOLE_ENTER:
					gTabMode = false;
					mBuffer[mNewline].mText[mCol] = '\0';
					process();
					advance();
					strcpy(mBuffer[mNewline].mText, mPrompt);
					strcat(mBuffer[mNewline].mText, mLastChar);
					resetCol();
					break;
				case CONSOLE_ESCAPE:
					mIsActive = false;
					gTabMode = false;
					break;
				case CONSOLE_LIST_COMMAND_UP:
					mCurcmd++;
					if( mCurcmd > mNumcmdhist )
						mCurcmd = mNumcmdhist;		
					cmdHistory();
					break;
				case CONSOLE_LIST_COMMAND_DOWN:
					mCurcmd--;
					if( mCurcmd <= 0 )
						mCurcmd = 0;
					cmdHistory();
					break;
				case CONSOLE_SCROLL_UP:
					mViewBottom++;
					if(mViewBottom >= CONSOLE_MAX_ROW)		mViewBottom -= CONSOLE_MAX_ROW;
					if(mViewBottom == mNewline - NB_LINES)	mViewBottom--;
					break;
				case CONSOLE_SCROLL_DOWN:
					mViewBottom--;
					if(mViewBottom < 0)				mViewBottom += CONSOLE_MAX_ROW;
					if(mViewBottom == mNewline-1)	mViewBottom = mNewline;
					break;
				}
			}
		}
	}
}

void Console::onKeyDown(SampleFramework::SampleUserInput::KeyCode keyCode, PxU32 param)
{
	//sschirm doesn't compile on snc
	//const int keyparam = (int)param;


	if (mIsActive)
	{
		if(param)
			in(param);
	}

	
}
