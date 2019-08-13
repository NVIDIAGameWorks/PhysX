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

#include <linux/LinuxSampleUserInput.h>
#include <X11/keysym.h>
#include <stdio.h>

static bool gTimeInit=false;

using namespace SampleFramework;
using namespace physx;

LinuxSampleUserInput::LinuxSampleUserInput()
{	
	// register all user inputs for linux platform
	registerUserInput(LINUXKEY_1,"KEY_1", "1");
	registerUserInput(LINUXKEY_2,"KEY_2", "2");	
	registerUserInput(LINUXKEY_3,"KEY_3", "3");	
	registerUserInput(LINUXKEY_4,"KEY_4", "4");	
	registerUserInput(LINUXKEY_5,"KEY_5", "5");	
	registerUserInput(LINUXKEY_6,"KEY_6", "6");	
	registerUserInput(LINUXKEY_7,"KEY_7", "7");	
	registerUserInput(LINUXKEY_8,"KEY_8", "8");	
	registerUserInput(LINUXKEY_9,"KEY_9", "9");	
	registerUserInput(LINUXKEY_0,"KEY_0", "0");	

	registerUserInput(LINUXKEY_A,"KEY_A", "A");	
	registerUserInput(LINUXKEY_B,"KEY_B", "B");	
	registerUserInput(LINUXKEY_C,"KEY_C", "C");	
	registerUserInput(LINUXKEY_D,"KEY_D", "D");	
	registerUserInput(LINUXKEY_E,"KEY_E", "E");	
	registerUserInput(LINUXKEY_F,"KEY_F", "F");	
	registerUserInput(LINUXKEY_G,"KEY_G", "G");	
	registerUserInput(LINUXKEY_H,"KEY_H", "H");	
	registerUserInput(LINUXKEY_I,"KEY_I", "I");	
	registerUserInput(LINUXKEY_J,"KEY_J", "J");	
	registerUserInput(LINUXKEY_K,"KEY_K", "K");	
	registerUserInput(LINUXKEY_L,"KEY_L", "L");	
	registerUserInput(LINUXKEY_M,"KEY_M", "M");	
	registerUserInput(LINUXKEY_N,"KEY_N", "N");	
	registerUserInput(LINUXKEY_O,"KEY_O", "O");	
	registerUserInput(LINUXKEY_P,"KEY_P", "P");	
	registerUserInput(LINUXKEY_Q,"KEY_Q", "Q");	
	registerUserInput(LINUXKEY_R,"KEY_R", "R");	
	registerUserInput(LINUXKEY_S,"KEY_S", "S");	
	registerUserInput(LINUXKEY_T,"KEY_T", "T");	
	registerUserInput(LINUXKEY_U,"KEY_U", "U");	
	registerUserInput(LINUXKEY_V,"KEY_V", "V");	
	registerUserInput(LINUXKEY_W,"KEY_W", "W");	
	registerUserInput(LINUXKEY_X,"KEY_X", "X");	
	registerUserInput(LINUXKEY_Y,"KEY_Y", "Y");	
	registerUserInput(LINUXKEY_Z,"KEY_Z", "Z");	
	
	registerUserInput(LINUXKEY_SPACE  ,"KEY_SPACE","Space");
	registerUserInput(LINUXKEY_RETURN ,"KEY_RETURN","Enter");
	registerUserInput(LINUXKEY_SHIFT ,"KEY_SHIFT","Shift");
	registerUserInput(LINUXKEY_CONTROL ,"KEY_CONTROL","Control");
	registerUserInput(LINUXKEY_ESCAPE ,"KEY_ESCAPE","Escape");
	registerUserInput(LINUXKEY_COMMA ,"KEY_COMMA",",");
	registerUserInput(LINUXKEY_NUMPAD0 ,"KEY_NUMPAD0","Numpad0");
	registerUserInput(LINUXKEY_NUMPAD1 ,"KEY_NUMPAD1","Numpad1");
	registerUserInput(LINUXKEY_NUMPAD2 ,"KEY_NUMPAD2","Numpad2");
	registerUserInput(LINUXKEY_NUMPAD3 ,"KEY_NUMPAD3","Numpad3");
	registerUserInput(LINUXKEY_NUMPAD4 ,"KEY_NUMPAD4","Numpad4");
	registerUserInput(LINUXKEY_NUMPAD5 ,"KEY_NUMPAD5","Numpad5");
	registerUserInput(LINUXKEY_NUMPAD6 ,"KEY_NUMPAD6","Numpad6");
	registerUserInput(LINUXKEY_NUMPAD7 ,"KEY_NUMPAD7","Numpad7");
	registerUserInput(LINUXKEY_NUMPAD8 ,"KEY_NUMPAD8","Numpad8");
	registerUserInput(LINUXKEY_NUMPAD9 ,"KEY_NUMPAD9","Numpad9");
	registerUserInput(LINUXKEY_ADD ,"KEY_ADD","+");
	registerUserInput(LINUXKEY_SUBTRACT ,"KEY_SUBTRACT","-");
	registerUserInput(LINUXKEY_COMMA ,"KEY_COMMM",", on keypad");
	registerUserInput(LINUXKEY_DIVIDE ,"KEY_DIVIDE","/");

	registerUserInput(LINUXKEY_F1 ,"KEY_F1","F1");
	registerUserInput(LINUXKEY_F2 ,"KEY_F2","F2");
	registerUserInput(LINUXKEY_F3 ,"KEY_F3","F3");
	registerUserInput(LINUXKEY_F4 ,"KEY_F4","F4");
	registerUserInput(LINUXKEY_F5 ,"KEY_F5","F5");
	registerUserInput(LINUXKEY_F6 ,"KEY_F6","F6");
	registerUserInput(LINUXKEY_F7 ,"KEY_F7","F7");
	registerUserInput(LINUXKEY_F8 ,"KEY_F8","F8");
	registerUserInput(LINUXKEY_F9 ,"KEY_F9","F9");
	registerUserInput(LINUXKEY_F10 ,"KEY_F10","F10");
	registerUserInput(LINUXKEY_F11 ,"KEY_F11","F11");
	registerUserInput(LINUXKEY_F12 ,"KEY_F12","F12");

	registerUserInput(LINUXKEY_TAB ,"KEY_TAB","Tab");
	registerUserInput(LINUXKEY_BACKSPACE ,"KEY_BACKSPACE","Backspace");
	registerUserInput(LINUXKEY_PRIOR ,"KEY_PRIOR","PgUp");
	registerUserInput(LINUXKEY_NEXT ,"KEY_NEXT","PgDn");
	registerUserInput(LINUXKEY_UP ,"KEY_UP","Up Arrow");
	registerUserInput(LINUXKEY_DOWN ,"KEY_DOWN","Down Arrow");
	registerUserInput(LINUXKEY_LEFT ,"KEY_LEFT","Left Arrow");
	registerUserInput(LINUXKEY_RIGHT ,"KEY_RIGHT","Right Arrow");
					  
	// mouse
	registerUserInput(MOUSE_BUTTON_LEFT ,"MOUSE_BUTTON_LEFT","Left Mouse Button");
	registerUserInput(MOUSE_BUTTON_RIGHT ,"MOUSE_BUTTON_RIGHT","Right Mouse Button");
	registerUserInput(MOUSE_BUTTON_CENTER ,"MOUSE_BUTTON_CENTER","Middle Mouse Button");
	registerUserInput(MOUSE_MOVE,"MOUSE_MOVE", "Mouse Move");	
	
	// scan codes
	registerScanCode(SCAN_CODE_UP, 26, LINUXKEY_E, "SCAN_CODE_E");
	registerScanCode(SCAN_CODE_DOWN, 54, LINUXKEY_C, "SCAN_CODE_C");
	registerScanCode(SCAN_CODE_LEFT, 38, LINUXKEY_A, "SCAN_CODE_A");
	registerScanCode(SCAN_CODE_RIGHT, 40, LINUXKEY_D, "SCAN_CODE_D");
	registerScanCode(SCAN_CODE_FORWARD, 25, LINUXKEY_W, "SCAN_CODE_W");
	registerScanCode(SCAN_CODE_BACKWARD, 39, LINUXKEY_S, "SCAN_CODE_S");
	registerScanCode(SCAN_CODE_L, 46, LINUXKEY_L, "SCAN_CODE_L");
	registerScanCode(SCAN_CODE_9, 18, LINUXKEY_9, "SCAN_CODE_9");
	registerScanCode(SCAN_CODE_0, 19, LINUXKEY_0, "SCAN_CODE_0");
}

void LinuxSampleUserInput::registerScanCode(LinuxSampleUserInputIds scanCodeId, physx::PxU16 scanCode, LinuxSampleUserInputIds nameId, const char* name)
{
	const UserInput* ui = getUserInputFromId(nameId);
	if(ui)
	{
		registerUserInput(scanCodeId, name, ui->m_Name);
		m_ScanCodesMap[scanCode] = scanCodeId;
	}
}

const UserInput* LinuxSampleUserInput::getUserInputFromId(LinuxSampleUserInputIds id) const
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

LinuxSampleUserInput::~LinuxSampleUserInput()
{
	m_ScanCodesMap.clear();
	m_AnalogStates.clear();
	m_DigitalStates.clear();
}

LinuxSampleUserInputIds LinuxSampleUserInput::getInputIdFromMouseButton(const physx::PxU16 b) const
{
	if 		(b == Button1)	return MOUSE_BUTTON_LEFT;
	else if (b == Button2)	return MOUSE_BUTTON_CENTER;
	else if (b == Button3)	return MOUSE_BUTTON_RIGHT;
	else					return LINUXKEY_UNKNOWN;
}

LinuxSampleUserInputIds LinuxSampleUserInput::getInputIdFromKeySym(const KeySym keySym) const
{
	LinuxSampleUserInputIds id = LINUXKEY_UNKNOWN;

	if(keySym >= XK_A && keySym <= XK_Z)						id = (LinuxSampleUserInputIds)((keySym - XK_A)+LINUXKEY_A);
	else if(keySym >= XK_a && keySym <= XK_z)					id = (LinuxSampleUserInputIds)((keySym - XK_a)+LINUXKEY_A);
	else if(keySym >= XK_0 && keySym <= XK_9)					id = (LinuxSampleUserInputIds)((keySym - XK_0)+LINUXKEY_0);
	else if(keySym >= XK_KP_0 && keySym <= XK_KP_9)				id = (LinuxSampleUserInputIds)((keySym - XK_KP_0)+LINUXKEY_NUMPAD0);	
	else if(keySym == XK_Shift_L || keySym == XK_Shift_R)		id = LINUXKEY_SHIFT;
	else if(keySym == XK_Control_L || keySym == XK_Control_R)	id = LINUXKEY_CONTROL;
	else if(keySym == XK_space)									id = LINUXKEY_SPACE;
	else if(keySym == XK_Return)								id = LINUXKEY_RETURN;
	else if(keySym == XK_Escape)								id = LINUXKEY_ESCAPE;
	else if(keySym == XK_KP_Separator)							id = LINUXKEY_COMMA;
	else if(keySym == XK_KP_Divide)								id = LINUXKEY_DIVIDE;
	else if(keySym == XK_KP_Subtract)							id = LINUXKEY_SUBTRACT;
	else if(keySym == XK_KP_Add)								id = LINUXKEY_ADD;
	//
	else if(keySym == XK_F1)									id = LINUXKEY_F1;
	else if(keySym == XK_F2)									id = LINUXKEY_F2;
	else if(keySym == XK_F3)									id = LINUXKEY_F3;
	else if(keySym == XK_F4)									id = LINUXKEY_F4;
	else if(keySym == XK_F5)									id = LINUXKEY_F5;
	else if(keySym == XK_F6)									id = LINUXKEY_F6;
	else if(keySym == XK_F7)									id = LINUXKEY_F7;
	else if(keySym == XK_F8)									id = LINUXKEY_F8;
	else if(keySym == XK_F9)									id = LINUXKEY_F9;
	else if(keySym == XK_F10)									id = LINUXKEY_F10;
	else if(keySym == XK_F11)									id = LINUXKEY_F11;
	else if(keySym == XK_F12)									id = LINUXKEY_F12;
	//
	else if(keySym == XK_Tab)									id = LINUXKEY_TAB;
	//
	else if(keySym == XK_BackSpace)								id = LINUXKEY_BACKSPACE;
	else if(keySym == XK_Prior)									id = LINUXKEY_PRIOR;
	else if(keySym == XK_Next)									id = LINUXKEY_NEXT;
	//
	else if(keySym == XK_Up)									id = LINUXKEY_UP;
	else if(keySym == XK_Down)									id = LINUXKEY_DOWN;
	else if(keySym == XK_Left)									id = LINUXKEY_LEFT;
	else if(keySym == XK_Right)									id = LINUXKEY_RIGHT;

	return id;
}

void LinuxSampleUserInput::doOnMouseMove(physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, physx::PxU16 button)
{	
	const std::vector<size_t>* events = getInputEvents(button);

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


void LinuxSampleUserInput::doOnMouseDown(physx::PxU32 x, physx::PxU32 y, physx::PxU16 button)
{	
	const std::vector<size_t>* events = getInputEvents(getInputIdFromMouseButton(button));

	if(events)
	{
		for (size_t i = events->size(); i--;)
		{
			const InputEvent& ie = mInputEvents[(*events)[i]];
			m_DigitalStates[ie.m_Id] = true;

			if(getInputEventListener())
			{		
				getInputEventListener()->onPointerInputEvent(ie, x, y, 0, 0, true);
			}
		}
	}

}

void LinuxSampleUserInput::doOnMouseUp(physx::PxU32 x, physx::PxU32 y, physx::PxU16 button)
{
	const std::vector<size_t>* events = getInputEvents(getInputIdFromMouseButton(button));
	if(events)
	{
		for (size_t i = events->size(); i--;)
		{
			const InputEvent& ie = mInputEvents[(*events)[i]];
			m_DigitalStates[ie.m_Id] = false;

			if(getInputEventListener())
			{		
				getInputEventListener()->onPointerInputEvent(ie, x, y, 0, 0, false);
			}
		}
	}

}

void LinuxSampleUserInput::doOnKeyDown(KeySym keySym, physx::PxU16 keyCode, physx::PxU8 ascii)
{
	const std::vector<size_t>* events = NULL;
	
	if(getInputEventListener())
	{
		//raw ASCII printable characters get sent to the console
		if (ascii >= 'a' && ascii <= 'z')
		{
			getInputEventListener()->onKeyDownEx(static_cast<KeyCode>(ascii - 'a' + KEY_A), ascii);
		}
		else if (ascii >= 'A' && ascii <= 'Z')
		{
			getInputEventListener()->onKeyDownEx(static_cast<KeyCode>(ascii - 'A' + KEY_A), ascii);
		}
		else if (ascii >= '0' && ascii <= '9')
		{
			getInputEventListener()->onKeyDownEx(static_cast<KeyCode>(ascii - 'A' + KEY_A), ascii);
		}
		else if (ascii == ' ')
		{
			getInputEventListener()->onKeyDownEx(static_cast<KeyCode>(ascii - ' ' + KEY_SPACE), ascii);
		}
		else if (ascii == '.')
		{
			getInputEventListener()->onKeyDownEx(static_cast<KeyCode>(ascii - '.' + KEY_DECIMAL), ascii);
		}
	
	
		std::map<physx::PxU16, physx::PxU16>::iterator fit = m_ScanCodesMap.find(keyCode);
		if(fit != m_ScanCodesMap.end())
		{
			events = getInputEvents(fit->second);
		}
		
		if(!events)
		{
			LinuxSampleUserInputIds id = getInputIdFromKeySym(keySym);
			events = getInputEvents(id);
		}
		
		if(!events || !getInputEventListener())
			return;
	
		for (size_t i = events->size(); i--;)
		{
			const InputEvent& ie = mInputEvents[(*events)[i]];
			m_DigitalStates[ie.m_Id] = true;
			getInputEventListener()->onDigitalInputEvent(ie, true);
		}	
	}
}

void LinuxSampleUserInput::doOnKeyUp( KeySym keySym, physx::PxU16 keyCode, physx::PxU8 ascii)
{
	const std::vector<size_t>* events = NULL;
	std::map<physx::PxU16, physx::PxU16>::iterator fit = m_ScanCodesMap.find(keyCode);
	if(fit != m_ScanCodesMap.end())
	{
		events = getInputEvents(fit->second);
	}
	if(!events)
	{
		LinuxSampleUserInputIds id = getInputIdFromKeySym(keySym);
		events = getInputEvents(id);
	}

	if(!events || !getInputEventListener())
		return;

	for (size_t i = events->size(); i--;)
	{
		const InputEvent& ie = mInputEvents[(*events)[i]];
		m_DigitalStates[ie.m_Id] = false;
		getInputEventListener()->onDigitalInputEvent(ie, false);
	}		
}

bool LinuxSampleUserInput::getDigitalInputEventState(physx::PxU16 inputEventId ) const
{
	std::map<physx::PxU16,bool>::const_iterator fit = m_DigitalStates.find(inputEventId);
	if(fit != m_DigitalStates.end())
	{
		return fit->second;
	}
	else
	{
		return false;
	}
}
float LinuxSampleUserInput::getAnalogInputEventState(physx::PxU16 inputEventId ) const
{
	std::map<physx::PxU16,float>::const_iterator fit = m_AnalogStates.find(inputEventId);
	if(fit != m_AnalogStates.end())
	{
		return fit->second;
	}
	else
	{
		return 0.0f;
	}
}

void LinuxSampleUserInput::shutdown()
{	
	m_AnalogStates.clear();
	m_DigitalStates.clear();

	SampleUserInput::shutdown();
}

void LinuxSampleUserInput::updateInput()
{
	SampleUserInput::updateInput();

	processGamepads();
}


