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

#ifndef SAMPLE_VIEWER_GAMEPAD_H
#define SAMPLE_VIEWER_GAMEPAD_H

#include <WTypes.h>
#include <XInput.h>

class SampleViewerScene;

class SampleViewerGamepad
{
public:
	enum GamepadTrigger
	{
		GAMEPAD_RIGHT_SHOULDER_TRIGGER = 0,
		GAMEPAD_LEFT_SHOULDER_TRIGGER = 1
	};

	enum GamepadButtons
	{
		GAMEPAD_DPAD_UP = 0,
		GAMEPAD_DPAD_DOWN,
		GAMEPAD_DPAD_LEFT,
		GAMEPAD_DPAD_RIGHT,
		GAMEPAD_START,
		GAMEPAD_BACK,
		GAMEPAD_LEFT_THUMB,
		GAMEPAD_RIGHT_THUMB,
		GAMEPAD_Y,
		GAMEPAD_A,
		GAMEPAD_X,
		GAMEPAD_B,
		GAMEPAD_LEFT_SHOULDER,
		GAMEPAD_RIGHT_SHOULDER
	};

	enum GamepadAxis
	{
		GAMEPAD_RIGHT_STICK_X = 0,
		GAMEPAD_RIGHT_STICK_Y,
		GAMEPAD_LEFT_STICK_X,
		GAMEPAD_LEFT_STICK_Y
	};

	SampleViewerGamepad();
	~SampleViewerGamepad();

	void							init();
	void							release();

	void							processGamepads(SampleViewerScene& viewerScene);

private:
	bool							hasXInput() const { return mpXInputGetState && mpXInputGetCapabilities; }

private:

	bool							mGamePadConnected;
	unsigned int					mConnectedPad;

	typedef DWORD(WINAPI *LPXINPUTGETSTATE)(DWORD, XINPUT_STATE*);
	typedef DWORD(WINAPI *LPXINPUTGETCAPABILITIES)(DWORD, DWORD, XINPUT_CAPABILITIES*);

	HMODULE							mXInputLibrary;
	LPXINPUTGETSTATE				mpXInputGetState;
	LPXINPUTGETCAPABILITIES			mpXInputGetCapabilities;

};

#endif // SAMPLE_VIEWER_GAMEPAD_H
