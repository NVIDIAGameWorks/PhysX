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

#include "SampleViewerGamepad.h"
#include "SampleViewerScene.h"

#include <stdio.h>
#include <direct.h>

#include "PsString.h"

//////////////////////////////////////////////////////////////////////////

static bool gTimeInit = false;
static const unsigned int MAX_GAMEPADS = 4;
static const unsigned int MAX_GAMEPAD_AXES = 4;
static XINPUT_STATE m_lastInputState[MAX_GAMEPADS];
static int m_lastAxisData[MAX_GAMEPADS][MAX_GAMEPAD_AXES];

//////////////////////////////////////////////////////////////////////////

SampleViewerGamepad::SampleViewerGamepad()
	: mGamePadConnected(false), mConnectedPad(0), mXInputLibrary(), mpXInputGetState(0), mpXInputGetCapabilities(0)
{
}

//////////////////////////////////////////////////////////////////////////
SampleViewerGamepad::~SampleViewerGamepad()
{
	release();
}

//////////////////////////////////////////////////////////////////////////

void SampleViewerGamepad::init()
{
	static const unsigned int xInputLibCount = 4;
	static const char* xInputLibs[xInputLibCount] = { "xinput1_4.dll",
		"xinput1_3.dll",
		"xinput1_2.dll",
		"xinput1_1.dll" };
	for (unsigned int i = 0; i < xInputLibCount; ++i)
	{
		mXInputLibrary = LoadLibraryA(xInputLibs[i]);
		if (mXInputLibrary)
			break;
	}

	if(!mXInputLibrary)
	{
		physx::shdfnd::printString("Could not load XInput library.");
	}
	
	if (mXInputLibrary)
	{
		mpXInputGetState = (LPXINPUTGETSTATE)GetProcAddress(mXInputLibrary, "XInputGetState");
		mpXInputGetCapabilities = (LPXINPUTGETCAPABILITIES)GetProcAddress(mXInputLibrary, "XInputGetCapabilities");
		if(!mpXInputGetState)
		{
			physx::shdfnd::printString("Error loading XInputGetState function.");
		}

		if(!mpXInputGetCapabilities)
		{
			physx::shdfnd::printString("Error loading XInputGetCapabilities function.");
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void SampleViewerGamepad::release()
{
	if (mXInputLibrary)
	{
		FreeLibrary(mXInputLibrary);
		mXInputLibrary = 0;
	}

	mpXInputGetState = 0;
	mpXInputGetCapabilities = 0;
	mGamePadConnected = false;
	mConnectedPad = 0;
}

//////////////////////////////////////////////////////////////////////////

void SampleViewerGamepad::processGamepads(SampleViewerScene& viewerScene)
{
	if (!gTimeInit)
	{
		gTimeInit = true;

		for (unsigned int p = 0; p < MAX_GAMEPADS; p++)
		{
			memset(&m_lastInputState[p], 0, sizeof(XINPUT_STATE));
			for (unsigned int i = 0; i < MAX_GAMEPAD_AXES; i++)
			{
				m_lastAxisData[p][i] = 0;
			}
		}
	}

	static int32_t disConnected[4] = { 1, 2, 3, 4 };

	if (!hasXInput())
		return;
	
	for (uint32_t p = 0; p < MAX_GAMEPADS; p++)
	{
		if ((--disConnected[p]) == 0)
		{
			XINPUT_STATE inputState;
			DWORD state = mpXInputGetState(p, &inputState);
			if (state == ERROR_DEVICE_NOT_CONNECTED)
			{
				disConnected[p] = 4;
				if (mGamePadConnected && (mConnectedPad == p))
				{
					mGamePadConnected = false;
					mConnectedPad = 0;
					for (uint32_t k = 0; k < MAX_GAMEPADS; k++)
					{
						XINPUT_STATE inputStateDisc;
						DWORD stateDisc = mpXInputGetState(k, &inputStateDisc);
						if (stateDisc == ERROR_SUCCESS)
						{
							mConnectedPad = k;
							mGamePadConnected = true;
							break;
						}
					}
				}
			}
			else if (state == ERROR_SUCCESS)
			{
				if (!mGamePadConnected)
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
					const WORD lastWButtons = m_lastInputState[p].Gamepad.wButtons;
					const WORD currWButtons = inputState.Gamepad.wButtons;

					const WORD buttonsDown = currWButtons & ~lastWButtons;
					const WORD buttonsUp = ~currWButtons & lastWButtons;
					//				const WORD buttonsHeld	= currWButtons & lastWButtons;

					for (int i = 0; i < 14; i++)
					{
						// order has to match struct GamepadControls
						static const WORD buttonMasks[] = {
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
							viewerScene.handleGamepadButton(i, true);
						else if (buttonsUp & buttonMasks[i])
							viewerScene.handleGamepadButton(i, false);
					}
					
					{						
						const BYTE newTriggerVal = inputState.Gamepad.bRightTrigger;
						viewerScene.handleGamepadTrigger(GamepadTrigger::GAMEPAD_RIGHT_SHOULDER_TRIGGER, ((float)newTriggerVal) / 255);
					}
					{						
						const BYTE newTriggerVal = inputState.Gamepad.bLeftTrigger;
						viewerScene.handleGamepadTrigger(GamepadTrigger::GAMEPAD_LEFT_SHOULDER_TRIGGER, ((float)newTriggerVal) / 255);
					}
				}

				// Gamepad
				const int axisData[] = { inputState.Gamepad.sThumbRX, inputState.Gamepad.sThumbRY, inputState.Gamepad.sThumbLX, inputState.Gamepad.sThumbLY };
				for (uint32_t i = 0; i < MAX_GAMEPAD_AXES; i++)
				{
					if (axisData[i] != m_lastAxisData[p][i])
					{
						int data = axisData[i];
						if (abs(data) < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
						{
							data = 0;
						}
						viewerScene.handleGamepadAxis(i, ((float)data) / SHRT_MAX);
					}
					m_lastAxisData[p][i] = axisData[i];
				}
				m_lastInputState[p] = inputState;
			}
		}
	}

}

