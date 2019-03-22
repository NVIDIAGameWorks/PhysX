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

#include <windows/WindowsSampleUserInput.h>

#include <foundation/PxAssert.h>

#include <stdio.h>
#include <direct.h>

static bool gTimeInit=false;

// PT: TODO: share this with PxApplication
static const unsigned int MAX_GAMEPADS = 4;
static const unsigned int MAX_GAMEPAD_AXES = 4;
static XINPUT_STATE m_lastInputState[MAX_GAMEPADS];
static int m_lastAxisData[MAX_GAMEPADS][MAX_GAMEPAD_AXES];

using namespace SampleFramework;
using namespace physx;

WindowsSampleUserInput::WindowsSampleUserInput()
{	
	mXInputLibrary          = 0;
	mpXInputGetState        = 0;
	mpXInputGetCapabilities = 0;

	static const unsigned int xInputLibCount = 4;
	static const char* xInputLibs[xInputLibCount] = { "xinput1_4.dll" ,
													  "xinput1_3.dll",
	                                                  "xinput1_2.dll",
	                                                  "xinput1_1.dll" };
	for (unsigned int i = 0; i < xInputLibCount; ++i)
	{
		mXInputLibrary = LoadLibraryA(xInputLibs[i]);
		if(mXInputLibrary)
			break;
	}
	PX_ASSERT(mXInputLibrary && "Could not load XInput library.");
	if (mXInputLibrary)
	{
		mpXInputGetState        = (LPXINPUTGETSTATE)GetProcAddress(mXInputLibrary, "XInputGetState");
		mpXInputGetCapabilities = (LPXINPUTGETCAPABILITIES)GetProcAddress(mXInputLibrary, "XInputGetCapabilities");
		PX_ASSERT(mpXInputGetState && "Error loading XInputGetState function.");
		PX_ASSERT(mpXInputGetCapabilities && "Error loading XInputGetCapabilities function.");
	}

	mGamePadConnected = false;
	mConnectedPad = 0;

	// register all user inputs for windows platform
	registerUserInput(WKEY_1,"KEY_1", "1");
	registerUserInput(WKEY_2,"KEY_2", "2");	
	registerUserInput(WKEY_3,"KEY_3", "3");	
	registerUserInput(WKEY_4,"KEY_4", "4");	
	registerUserInput(WKEY_5,"KEY_5", "5");	
	registerUserInput(WKEY_6,"KEY_6", "6");	
	registerUserInput(WKEY_7,"KEY_7", "7");	
	registerUserInput(WKEY_8,"KEY_8", "8");	
	registerUserInput(WKEY_9,"KEY_9", "9");	
	registerUserInput(WKEY_0,"KEY_0", "0");	

	registerUserInput(WKEY_A,"KEY_A", "A");	
	registerUserInput(WKEY_B,"KEY_B", "B");	
	registerUserInput(WKEY_C,"KEY_C", "C");	
	registerUserInput(WKEY_D,"KEY_D", "D");	
	registerUserInput(WKEY_E,"KEY_E", "E");	
	registerUserInput(WKEY_F,"KEY_F", "F");	
	registerUserInput(WKEY_G,"KEY_G", "G");	
	registerUserInput(WKEY_H,"KEY_H", "H");	
	registerUserInput(WKEY_I,"KEY_I", "I");	
	registerUserInput(WKEY_J,"KEY_J", "J");	
	registerUserInput(WKEY_K,"KEY_K", "K");	
	registerUserInput(WKEY_L,"KEY_L", "L");	
	registerUserInput(WKEY_M,"KEY_M", "M");	
	registerUserInput(WKEY_N,"KEY_N", "N");	
	registerUserInput(WKEY_O,"KEY_O", "O");	
	registerUserInput(WKEY_P,"KEY_P", "P");	
	registerUserInput(WKEY_Q,"KEY_Q", "Q");	
	registerUserInput(WKEY_R,"KEY_R", "R");	
	registerUserInput(WKEY_S,"KEY_S", "S");	
	registerUserInput(WKEY_T,"KEY_T", "T");	
	registerUserInput(WKEY_U,"KEY_U", "U");	
	registerUserInput(WKEY_V,"KEY_V", "V");	
	registerUserInput(WKEY_W,"KEY_W", "W");	
	registerUserInput(WKEY_X,"KEY_X", "X");	
	registerUserInput(WKEY_Y,"KEY_Y", "Y");	
	registerUserInput(WKEY_Z,"KEY_Z", "Z");	

	registerUserInput(WKEY_SPACE  ,"KEY_SPACE","Space");
	registerUserInput(WKEY_RETURN ,"KEY_RETURN","Enter");
	registerUserInput(WKEY_SHIFT ,"KEY_SHIFT","Shift");
	registerUserInput(WKEY_CONTROL ,"KEY_CONTROL","Control");
	registerUserInput(WKEY_ESCAPE ,"KEY_ESCAPE","Escape");
	registerUserInput(WKEY_COMMA ,"KEY_COMMA",",");
	registerUserInput(WKEY_NUMPAD0 ,"KEY_NUMPAD0","Numpad 0");
	registerUserInput(WKEY_NUMPAD1 ,"KEY_NUMPAD1","Numpad 1");
	registerUserInput(WKEY_NUMPAD2 ,"KEY_NUMPAD2","Numpad 2");
	registerUserInput(WKEY_NUMPAD3 ,"KEY_NUMPAD3","Numpad 3");
	registerUserInput(WKEY_NUMPAD4 ,"KEY_NUMPAD4","Numpad 4");
	registerUserInput(WKEY_NUMPAD5 ,"KEY_NUMPAD5","Numpad 5");
	registerUserInput(WKEY_NUMPAD6 ,"KEY_NUMPAD6","Numpad 6");
	registerUserInput(WKEY_NUMPAD7 ,"KEY_NUMPAD7","Numpad 7");
	registerUserInput(WKEY_NUMPAD8 ,"KEY_NUMPAD8","Numpad 8");
	registerUserInput(WKEY_NUMPAD9 ,"KEY_NUMPAD9","Numpad 9");
	registerUserInput(WKEY_MULTIPLY ,"KEY_MULTIPLY","*");
	registerUserInput(WKEY_ADD ,"KEY_ADD","+");
	registerUserInput(WKEY_SEPARATOR ,"KEY_SEPARATOR","Separator");
	registerUserInput(WKEY_SUBTRACT ,"KEY_SUBTRACT","-");
	registerUserInput(WKEY_DECIMAL ,"KEY_DECIMAL",".");
	registerUserInput(WKEY_DIVIDE ,"KEY_DIVIDE","/");

	registerUserInput(WKEY_F1 ,"KEY_F1","F1");
	registerUserInput(WKEY_F2 ,"KEY_F2","F2");
	registerUserInput(WKEY_F3 ,"KEY_F3","F3");
	registerUserInput(WKEY_F4 ,"KEY_F4","F4");
	registerUserInput(WKEY_F5 ,"KEY_F5","F5");
	registerUserInput(WKEY_F6 ,"KEY_F6","F6");
	registerUserInput(WKEY_F7 ,"KEY_F7","F7");
	registerUserInput(WKEY_F8 ,"KEY_F8","F8");
	registerUserInput(WKEY_F9 ,"KEY_F9","F9");
	registerUserInput(WKEY_F10 ,"KEY_F10","F10");
	registerUserInput(WKEY_F11 ,"KEY_F11","F11");
	registerUserInput(WKEY_F12 ,"KEY_F12","F12");

	registerUserInput(WKEY_TAB ,"KEY_TAB","Tab");
	registerUserInput(WKEY_BACKSPACE,"KEY_BACKSPACE","Backspace");
	registerUserInput(WKEY_PRIOR ,"KEY_PRIOR","PgUp");
	registerUserInput(WKEY_NEXT ,"KEY_NEXT","PgDn");
	registerUserInput(WKEY_UP ,"KEY_UP","Up Arrow");
	registerUserInput(WKEY_DOWN ,"KEY_DOWN","Down Arrow");
	registerUserInput(WKEY_LEFT ,"KEY_LEFT","Left Arrow");
	registerUserInput(WKEY_RIGHT ,"KEY_RIGHT","Right Arrow");

	// mouse
	registerUserInput(MOUSE_BUTTON_LEFT ,"MOUSE_BUTTON_LEFT","Left Mouse Button");
	registerUserInput(MOUSE_BUTTON_RIGHT ,"MOUSE_BUTTON_RIGHT","Right Mouse Button");
	registerUserInput(MOUSE_BUTTON_CENTER ,"MOUSE_BUTTON_CENTER","Middle Mouse Button");

	registerUserInput(MOUSE_MOVE,"MOUSE_MOVE", "Mouse Move");	

	//assumes the pad naming conventions of xbox
	registerUserInput(GAMEPAD_DIGI_UP,"GAMEPAD_DIGI_UP", "gpad UP");
	registerUserInput(GAMEPAD_DIGI_DOWN,"GAMEPAD_DIGI_DOWN", "gpad DOWN");
	registerUserInput(GAMEPAD_DIGI_LEFT,"GAMEPAD_DIGI_LEFT", "gpad LEFT");
	registerUserInput(GAMEPAD_DIGI_RIGHT,"GAMEPAD_DIGI_RIGHT", "gpad RIGHT");
	registerUserInput(GAMEPAD_START ,"GAMEPAD_START", "gpad START");
	registerUserInput(GAMEPAD_SELECT ,"GAMEPAD_SELECT", "gpad BACK");
	registerUserInput(GAMEPAD_LEFT_STICK ,"GAMEPAD_LEFT_STICK", "gpad LSTICK");
	registerUserInput(GAMEPAD_RIGHT_STICK ,"GAMEPAD_RIGHT_STICK", "gpad RSTICK");
	registerUserInput(GAMEPAD_NORTH ,"GAMEPAD_NORTH", "gpad Y");
	registerUserInput(GAMEPAD_SOUTH ,"GAMEPAD_SOUTH", "gpad A");
	registerUserInput(GAMEPAD_WEST ,"GAMEPAD_WEST", "gpad X");
	registerUserInput(GAMEPAD_EAST ,"GAMEPAD_EAST", "gpad B");
	registerUserInput(GAMEPAD_LEFT_SHOULDER_TOP ,"GAMEPAD_LEFT_SHOULDER_TOP", "gpad LB");
	registerUserInput(GAMEPAD_RIGHT_SHOULDER_TOP ,"GAMEPAD_RIGHT_SHOULDER_TOP", "gpad RB");
	registerUserInput(GAMEPAD_LEFT_SHOULDER_BOT ,"GAMEPAD_LEFT_SHOULDER_BOT", "gpad LT");
	registerUserInput(GAMEPAD_RIGHT_SHOULDER_BOT ,"GAMEPAD_RIGHT_SHOULDER_BOT", "gpad RT");

	registerUserInput(GAMEPAD_RIGHT_STICK_X,"GAMEPAD_RIGHT_STICK_X", "gpad RSTICK");
	registerUserInput(GAMEPAD_RIGHT_STICK_Y,"GAMEPAD_RIGHT_STICK_Y", "gpad RSTICK");
	registerUserInput(GAMEPAD_LEFT_STICK_X,"GAMEPAD_LEFT_STICK_X", "gpad LSTICK");
	registerUserInput(GAMEPAD_LEFT_STICK_Y,"GAMEPAD_LEFT_STICK_Y", "gpad LSTICK");

	// use scan codes for movement
	PxU16 vkey = (PxU16) MapVirtualKey(18, MAPVK_VSC_TO_VK);
	WindowsSampleUserInputIds getId = getKeyCode(vkey);
	const UserInput* ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_UP,"SCAN_CODE_E", ui->m_Name);
		mScanCodesMap[18] = SCAN_CODE_UP;
	}

	vkey = (PxU16)MapVirtualKey(46, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_DOWN,"SCAN_CODE_C", ui->m_Name);
		mScanCodesMap[46] = SCAN_CODE_DOWN;
	}

	vkey = (PxU16) MapVirtualKey(30, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_LEFT,"SCAN_CODE_A", ui->m_Name);
		mScanCodesMap[30] = SCAN_CODE_LEFT;
	}

	vkey = (PxU16) MapVirtualKey(32, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_RIGHT,"SCAN_CODE_D", ui->m_Name);
		mScanCodesMap[32] = SCAN_CODE_RIGHT;
	}
	

	vkey = (PxU16) MapVirtualKey(17, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_FORWARD,"SCAN_CODE_W", ui->m_Name);
		mScanCodesMap[17] = SCAN_CODE_FORWARD;
	}

	vkey = (PxU16) MapVirtualKey(31, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_BACKWARD,"SCAN_CODE_S", ui->m_Name);
		mScanCodesMap[31] = SCAN_CODE_BACKWARD;
	}

	vkey = (PxU16) MapVirtualKey(42, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_LEFT_SHIFT,"SCAN_CODE_LEFT_SHIFT", ui->m_Name);
		mScanCodesMap[42] = SCAN_CODE_LEFT_SHIFT;
	}

	vkey = (PxU16) MapVirtualKey(57, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_SPACE,"SCAN_CODE_SPACE", ui->m_Name);
		mScanCodesMap[57] = SCAN_CODE_SPACE;
	}

	vkey =(PxU16) MapVirtualKey(38, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_L,"SCAN_CODE_L", ui->m_Name);
		mScanCodesMap[38] = SCAN_CODE_L;
	}

	vkey = (PxU16)MapVirtualKey(10, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_9,"SCAN_CODE_9", ui->m_Name);
		mScanCodesMap[10] = SCAN_CODE_9;
	}

	vkey = (PxU16)MapVirtualKey(11, MAPVK_VSC_TO_VK);
	getId = getKeyCode(vkey);
	ui = getUserInputFromId(getId);
	if(ui)
	{
		registerUserInput(SCAN_CODE_0,"SCAN_CODE_0", ui->m_Name);
		mScanCodesMap[11] = SCAN_CODE_0;
	}

}

const UserInput* WindowsSampleUserInput::getUserInputFromId(WindowsSampleUserInputIds id) const
{
	for (size_t i = mUserInputs.size(); i--;)
	{
		if(mUserInputs[i].m_Id == id)
		{
			return &mUserInputs[i];
		}
	}

	return NULL;
}

WindowsSampleUserInput::~WindowsSampleUserInput()
{
	mScanCodesMap.clear();
	mAnalogStates.clear();
	mDigitalStates.clear();
	if (mXInputLibrary) 
	{
		FreeLibrary(mXInputLibrary);
		mXInputLibrary = 0;
	}
}

WindowsSampleUserInputIds WindowsSampleUserInput::getKeyCode(WPARAM wParam) const
{
	WindowsSampleUserInputIds keyCode = WKEY_UNKNOWN;
	const int keyparam = (int)wParam;

	if(keyparam >= 'A' && keyparam <= 'Z')						keyCode = (WindowsSampleUserInputIds)((keyparam - 'A')+WKEY_A);
	else if(keyparam >= '0' && keyparam <= '9')					keyCode = (WindowsSampleUserInputIds)((keyparam - '0')+WKEY_0);
	else if(keyparam >= VK_NUMPAD0 && keyparam <= VK_DIVIDE)	keyCode = (WindowsSampleUserInputIds)((keyparam - VK_NUMPAD0)+WKEY_NUMPAD0);
	else if(keyparam == VK_SHIFT)								keyCode = WKEY_SHIFT;
	else if(keyparam == VK_CONTROL)								keyCode = WKEY_CONTROL;
	else if(keyparam == VK_SPACE)								keyCode = WKEY_SPACE;
	else if(keyparam == VK_RETURN)								keyCode = WKEY_RETURN;
	else if(keyparam == VK_ESCAPE)								keyCode = WKEY_ESCAPE;
	else if(keyparam == VK_OEM_COMMA)							keyCode = WKEY_COMMA;
	else if(keyparam == VK_OEM_2)								keyCode = WKEY_DIVIDE;
	else if(keyparam == VK_OEM_MINUS)							keyCode = WKEY_SUBTRACT;
	else if(keyparam == VK_OEM_PLUS)							keyCode = WKEY_ADD;
	//
	else if(keyparam == VK_F1)									keyCode = WKEY_F1;
	else if(keyparam == VK_F2)									keyCode = WKEY_F2;
	else if(keyparam == VK_F3)									keyCode = WKEY_F3;
	else if(keyparam == VK_F4)									keyCode = WKEY_F4;
	else if(keyparam == VK_F5)									keyCode = WKEY_F5;
	else if(keyparam == VK_F6)									keyCode = WKEY_F6;
	else if(keyparam == VK_F7)									keyCode = WKEY_F7;
	else if(keyparam == VK_F8)									keyCode = WKEY_F8;
	else if(keyparam == VK_F9)									keyCode = WKEY_F9;
	else if(keyparam == VK_F10)									keyCode = WKEY_F10;
	else if(keyparam == VK_F11)									keyCode = WKEY_F11;
	else if(keyparam == VK_F12)									keyCode = WKEY_F12;
	//
	else if(keyparam == VK_TAB)									keyCode = WKEY_TAB;
	else if(keyparam == VK_BACK)								keyCode = WKEY_BACKSPACE;
	//
	else if(keyparam == VK_PRIOR)								keyCode = WKEY_PRIOR;
	else if(keyparam == VK_NEXT)								keyCode = WKEY_NEXT;
	//
	else if(keyparam == VK_UP)									keyCode = WKEY_UP;
	else if(keyparam == VK_DOWN)								keyCode = WKEY_DOWN;
	else if(keyparam == VK_LEFT)								keyCode = WKEY_LEFT;
	else if(keyparam == VK_RIGHT)								keyCode = WKEY_RIGHT;

	return keyCode;
}

SampleUserInput::KeyCode WindowsSampleUserInput::getSampleUserInputKeyCode(WPARAM wParam) const
{
	SampleUserInput::KeyCode keyCode = KEY_UNKNOWN;
	const int keyparam = (int)wParam;
	// PT: TODO: the comment below is of course wrong. You just need to use the proper event code (WM_CHAR)
	//there are no lowercase virtual key codes!!
	//if(     keyparam >= 'a' && keyparam <= 'z') keyCode = (RendererWindow::KeyCode)((keyparam - 'a')+KEY_A);		else 

	if(keyparam >= 'A' && keyparam <= 'Z')						keyCode = (SampleUserInput::KeyCode)((keyparam - 'A')+KEY_A);
	else if(keyparam >= '0' && keyparam <= '9')					keyCode = (SampleUserInput::KeyCode)((keyparam - '0')+KEY_0);
	else if(keyparam >= VK_NUMPAD0 && keyparam <= VK_DIVIDE)	keyCode = (SampleUserInput::KeyCode)((keyparam - VK_NUMPAD0)+KEY_NUMPAD0);
	else if(keyparam == VK_SHIFT)								keyCode = KEY_SHIFT;
	else if(keyparam == VK_CONTROL)								keyCode = KEY_CONTROL;
	else if(keyparam == VK_SPACE)								keyCode = KEY_SPACE;
	else if(keyparam == VK_RETURN)								keyCode = KEY_RETURN;
	else if(keyparam == VK_ESCAPE)								keyCode = KEY_ESCAPE;
	else if(keyparam == VK_OEM_COMMA)							keyCode = KEY_COMMA;
	else if(keyparam == VK_OEM_2)								keyCode = KEY_DIVIDE;
	else if(keyparam == VK_OEM_MINUS)							keyCode = KEY_SUBTRACT;
	else if(keyparam == VK_OEM_PLUS)							keyCode = KEY_ADD;
	//
	else if(keyparam == VK_F1)									keyCode = KEY_F1;
	else if(keyparam == VK_F2)									keyCode = KEY_F2;
	else if(keyparam == VK_F3)									keyCode = KEY_F3;
	else if(keyparam == VK_F4)									keyCode = KEY_F4;
	else if(keyparam == VK_F5)									keyCode = KEY_F5;
	else if(keyparam == VK_F6)									keyCode = KEY_F6;
	else if(keyparam == VK_F7)									keyCode = KEY_F7;
	else if(keyparam == VK_F8)									keyCode = KEY_F8;
	else if(keyparam == VK_F9)									keyCode = KEY_F9;
	else if(keyparam == VK_F10)									keyCode = KEY_F10;
	else if(keyparam == VK_F11)									keyCode = KEY_F11;
	else if(keyparam == VK_F12)									keyCode = KEY_F12;
	//
	else if(keyparam == VK_TAB)									keyCode = KEY_TAB;
	//
	else if(keyparam == VK_PRIOR)								keyCode = KEY_PRIOR;
	else if(keyparam == VK_NEXT)								keyCode = KEY_NEXT;
	//
	else if(keyparam == VK_UP)									keyCode = KEY_UP;
	else if(keyparam == VK_DOWN)								keyCode = KEY_DOWN;
	else if(keyparam == VK_LEFT)								keyCode = KEY_LEFT;
	else if(keyparam == VK_RIGHT)								keyCode = KEY_RIGHT;

	return keyCode;
}

void WindowsSampleUserInput::doOnMouseMove(physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, WindowsSampleUserInputIds button)
{	
	const std::vector<size_t>* events = getInputEvents((PxU16)button);

	if(events)
	{
		for (size_t i = events->size(); i--;)
		{
			const InputEvent& ie = mInputEvents[(*events)[i]];
			if(getInputEventListener())
			{		
				getInputEventListener()->onPointerInputEvent(ie, x, y, dx, dy, false);
			}
		}
	}

}

void WindowsSampleUserInput::doOnMouseButton(physx::PxU32 x, physx::PxU32 y , MouseButtons button, bool down)
{
	WindowsSampleUserInputIds buttonId = WKEY_UNKNOWN;
	WindowsSampleUserInputIds buttonIdOpposite = WKEY_UNKNOWN;
	switch (button)
	{
	case LEFT_MOUSE_BUTTON:
		buttonId = MOUSE_BUTTON_LEFT;
		break;
	case RIGHT_MOUSE_BUTTON:
		buttonId = MOUSE_BUTTON_RIGHT;
		break;
	case CENTER_MOUSE_BUTTON:
		buttonId = MOUSE_BUTTON_CENTER;
		break;
	}

	const std::vector<size_t>* events = getInputEvents((PxU16)buttonId);
	if(events)
	{
		for (size_t i = events->size(); i--;)
		{
			const InputEvent& ie = mInputEvents[(*events)[i]];
			mDigitalStates[ie.m_Id] = down;

			if(getInputEventListener())
			{		
				getInputEventListener()->onPointerInputEvent(ie, x, y, 0, 0, down);
			}
		}
	}
}

void WindowsSampleUserInput::onKeyDownEx(WPARAM wParam)
{	
	if(getInputEventListener())
	{
		KeyCode keyCode = getSampleUserInputKeyCode(wParam);

		getInputEventListener()->onKeyDownEx(keyCode, (PxU32)wParam);
	}
}

void WindowsSampleUserInput::onKeyDown(WPARAM wParam, LPARAM lParam)
{
	PxU16 scanCode = (lParam >> 16) & 0xFF;
	const std::vector<size_t>* events = NULL;
	std::map<physx::PxU16, physx::PxU16>::iterator fit = mScanCodesMap.find(scanCode);
	if(fit != mScanCodesMap.end())
	{
		events = getInputEvents(fit->second);
	}

	if(!events)
	{
		WindowsSampleUserInputIds keyCode = getKeyCode(wParam);
		events = getInputEvents((PxU16)keyCode);
	}

	if(!events || !getInputEventListener())
		return;

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		mDigitalStates[ie.m_Id] = true;
		getInputEventListener()->onDigitalInputEvent(ie, true);
	}		
}

void WindowsSampleUserInput::onKeyUp(WPARAM wParam, LPARAM lParam)
{
	PxU16 scanCode = (lParam >> 16) & 0xFF;
	const std::vector<size_t>* events = NULL;
	std::map<physx::PxU16, physx::PxU16>::iterator fit = mScanCodesMap.find(scanCode);
	if(fit != mScanCodesMap.end())
	{
		events = getInputEvents(fit->second);
	}
	if(!events)
	{
		WindowsSampleUserInputIds keyCode = getKeyCode(wParam);
		events = getInputEvents((PxU16)keyCode);
	}

	if(!events || !getInputEventListener())
		return;

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		mDigitalStates[ie.m_Id] = false;
		getInputEventListener()->onDigitalInputEvent(ie, false);
	}		
}

void WindowsSampleUserInput::onKeyEvent(const KeyEvent& keyEvent)
{
	WindowsSampleUserInputIds keyCode = getKeyCode(keyEvent.m_Param);
	const std::vector<size_t>* events = getInputEvents((PxU16)keyCode);

	if(!events || !getInputEventListener())
		return;

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];

		if(keyEvent.m_Flags == KEY_EVENT_UP)
		{
			mDigitalStates[ie.m_Id] = false;
			getInputEventListener()->onDigitalInputEvent(ie, false);
		}

		if(keyEvent.m_Flags == KEY_EVENT_DOWN)
		{
			mDigitalStates[ie.m_Id] = true;
			getInputEventListener()->onDigitalInputEvent(ie, true);
		}
	}
}

bool WindowsSampleUserInput::getDigitalInputEventState(physx::PxU16 inputEventId ) const
{
	std::map<physx::PxU16,bool>::const_iterator fit = mDigitalStates.find(inputEventId);
	if(fit != mDigitalStates.end())
	{
		return fit->second;
	}
	else
	{
		return false;
	}
}
float WindowsSampleUserInput::getAnalogInputEventState(physx::PxU16 inputEventId ) const
{
	std::map<physx::PxU16,float>::const_iterator fit = mAnalogStates.find(inputEventId);
	if(fit != mAnalogStates.end())
	{
		return fit->second;
	}
	else
	{
		return 0.0f;
	}
}

void WindowsSampleUserInput::onGamepadAnalogButton(physx::PxU32 buttonIndex,const BYTE oldValue,const BYTE newValue)
{
	const std::vector<size_t>* events = getInputEvents((PxU16)buttonIndex);

	if(!events || !getInputEventListener())
		return;

	bool newUp = false;
	bool newDown = false;
	if( !oldValue && newValue )
	{
		newUp = true;
	}
	else if( oldValue && !newValue )
	{
		newDown = true;
	}

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		if(ie.m_Analog)
		{
			mAnalogStates[ie.m_Id] = newValue/255.0f;
			getInputEventListener()->onAnalogInputEvent(ie, newValue/255.0f);
		}
		else
		{
			if(newUp)
			{
				mDigitalStates[ie.m_Id] = true;
				getInputEventListener()->onDigitalInputEvent(ie, true);
			}
			if(newDown)
			{
				mDigitalStates[ie.m_Id] = false;
				getInputEventListener()->onDigitalInputEvent(ie, false);
			}
		}
	}



}

void WindowsSampleUserInput::onGamepadButton(physx::PxU32 buttonIndex, bool buttonDown)
{
	PxU16 userInput = (PxU16) (GAMEPAD_DIGI_UP + buttonIndex);

	const std::vector<size_t>* events = getInputEvents(userInput);

	if(!events || !getInputEventListener())
		return;

	for (size_t i = 0; i < events->size(); i++)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		mDigitalStates[ie.m_Id] = buttonDown;
		getInputEventListener()->onDigitalInputEvent(ie, buttonDown);
	}
}

void WindowsSampleUserInput::onGamepadAxis(physx::PxU32 axis, physx::PxReal val)
{
	PxU16 userInput = (PxU16)(GAMEPAD_RIGHT_STICK_X + axis);

	const std::vector<size_t>* events = getInputEvents(userInput);

	if(!events || !getInputEventListener())
		return;

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		mAnalogStates[ie.m_Id] = val;
		getInputEventListener()->onAnalogInputEvent(ie, val);		
	}

}

void WindowsSampleUserInput::shutdown()
{	
	mAnalogStates.clear();
	mDigitalStates.clear();

	SampleUserInput::shutdown();
}

void WindowsSampleUserInput::updateInput()
{
	SampleUserInput::updateInput();

	processGamepads();
}

void WindowsSampleUserInput::processGamepads()
{
	SampleUserInput::processGamepads();

	if(!gTimeInit)
	{
		gTimeInit=true;

		for(PxU32 p=0;p<MAX_GAMEPADS;p++)
		{
			memset(&m_lastInputState[p], 0, sizeof(XINPUT_STATE));
			for(PxU32 i=0;i<MAX_GAMEPAD_AXES;i++)
			{
				m_lastAxisData[p][i]=0;
			}
		}
	}

	static PxI32 disConnected[4] = {1, 2, 3, 4};

	if (!hasXInput())
		return;

	// PT: TODO: share this with PxApplication
	for(PxU32 p=0;p<MAX_GAMEPADS;p++)
	{
		if((--disConnected[p]) == 0)
		{
			XINPUT_STATE inputState;
			DWORD state = mpXInputGetState(p, &inputState);
			if(state == ERROR_DEVICE_NOT_CONNECTED)
			{
				disConnected[p] = 4;
				if(mGamePadConnected && (mConnectedPad == p))
				{
					mGamePadConnected = false;
					mConnectedPad = 0;
					for (PxU32 k=0;k<MAX_GAMEPADS;k++)
					{
						XINPUT_STATE inputStateDisc;
						DWORD stateDisc = mpXInputGetState(k, &inputStateDisc);
						if(stateDisc == ERROR_SUCCESS)
						{
							mConnectedPad = k;
							mGamePadConnected = true;
							break;
						}
					}
				}
			}
			else if(state == ERROR_SUCCESS)
			{
				if(!mGamePadConnected)
				{
					mGamePadConnected = true;
					mConnectedPad = p;
				}

				disConnected[p] = 1; //force to test next time
				XINPUT_CAPABILITIES caps;
				mpXInputGetCapabilities(p, XINPUT_FLAG_GAMEPAD, &caps);

				//gamepad
				{
					// Process buttons
					const WORD lastWButtons	= m_lastInputState[p].Gamepad.wButtons;
					const WORD currWButtons	= inputState.Gamepad.wButtons;

					const WORD buttonsDown	= currWButtons & ~lastWButtons;
					const WORD buttonsUp	=  ~currWButtons & lastWButtons;
					//				const WORD buttonsHeld	= currWButtons & lastWButtons;

					for(int i=0;i<14;i++)
					{
						// order has to match struct GamepadControls
						static const WORD buttonMasks[]={
							XINPUT_GAMEPAD_DPAD_UP,
							XINPUT_GAMEPAD_DPAD_DOWN,
							XINPUT_GAMEPAD_DPAD_LEFT,
							XINPUT_GAMEPAD_DPAD_RIGHT,
							XINPUT_GAMEPAD_START,
							XINPUT_GAMEPAD_BACK,
							XINPUT_GAMEPAD_LEFT_THUMB,
							XINPUT_GAMEPAD_RIGHT_THUMB, 
							XINPUT_GAMEPAD_Y,
							XINPUT_GAMEPAD_A,
							XINPUT_GAMEPAD_X,
							XINPUT_GAMEPAD_B,
							XINPUT_GAMEPAD_LEFT_SHOULDER,
							XINPUT_GAMEPAD_RIGHT_SHOULDER,
						};

						if (buttonsDown & buttonMasks[i])
							onGamepadButton(i, true);
						else if(buttonsUp & buttonMasks[i])
							onGamepadButton(i, false);
					}

					// PT: I think we do the 2 last ones separately because they're listed in GamepadControls but not in buttonMasks...
					{
						const BYTE oldTriggerVal = m_lastInputState[p].Gamepad.bRightTrigger;
						const BYTE newTriggerVal = inputState.Gamepad.bRightTrigger;
						onGamepadAnalogButton(GAMEPAD_RIGHT_SHOULDER_BOT, oldTriggerVal, newTriggerVal);
					}
					{
						const BYTE oldTriggerVal = m_lastInputState[p].Gamepad.bLeftTrigger;
						const BYTE newTriggerVal = inputState.Gamepad.bLeftTrigger;
						onGamepadAnalogButton(GAMEPAD_LEFT_SHOULDER_BOT, oldTriggerVal, newTriggerVal);
					}
				}		

				// Gamepad
				const int axisData[] = {inputState.Gamepad.sThumbRX, inputState.Gamepad.sThumbRY, inputState.Gamepad.sThumbLX, inputState.Gamepad.sThumbLY };
				for(PxU32 i=0;i<MAX_GAMEPAD_AXES;i++)
				{
					if(axisData[i] != m_lastAxisData[p][i])
					{
						int data = axisData[i];
						if(abs(data) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
						{
							data = 0;
						}
						onGamepadAxis( i, ((float)data)/SHRT_MAX);
					}
					m_lastAxisData[p][i] = axisData[i];
				}
				m_lastInputState[p] = inputState;
			}
		}
	}
}

bool WindowsSampleUserInput::gamepadSupported() const
{
	return mGamePadConnected;
}

InputType WindowsSampleUserInput::getInputType(const UserInput& ui) const
{
	if(ui.m_Id > WKEY_DEFINITION_START && ui.m_Id < WKEY_DEFINITION_END)
	{
		return KEYBOARD_INPUT;
	}

	if(ui.m_Id > GAMEPAD_DEFINITION_START && ui.m_Id < GAMEPAD_DEFINITION_END)
	{
		return GAMEPAD_INPUT;
	}

	if(ui.m_Id > MOUSE_DEFINITION_START && ui.m_Id < MOUSE_DEFINITION_END)
	{
		return MOUSE_INPUT;
	}

	return UNDEFINED_INPUT;
}

