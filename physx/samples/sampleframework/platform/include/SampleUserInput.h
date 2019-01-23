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

#ifndef SAMPLE_USER_INPUT_H
#define SAMPLE_USER_INPUT_H

#include <foundation/PxSimpleTypes.h>
#include <vector>
#include "PsString.h"

#if PX_VC
#pragma warning(push)
#pragma warning(disable:4702)
#include <map>
#pragma warning(pop)
#else
#include <map>
#endif

namespace SampleRenderer
{
	class Renderer;
}

namespace SampleFramework
{
	class InputEventListener;

	struct UserInput 
	{
		physx::PxU16	m_Id;
		char			m_IdName[256];	// this name is used for mapping (enum name)
		char			m_Name[256];	// this name is used for help
	};

	struct InputEvent
	{
		InputEvent(physx::PxU16 id, bool analog = false, float sens = 1.0f)
			:m_Id(id), m_Analog(analog), m_Sensitivity(sens)
		{
		}

		InputEvent(const InputEvent& e)
			:m_Id(e.m_Id), m_Analog(e.m_Analog), m_Sensitivity(e.m_Sensitivity)
		{
		}

		InputEvent()
			: m_Analog(false), m_Sensitivity(1.0f)
		{
		}

		physx::PxU16	m_Id;
		bool			m_Analog;		
		float			m_Sensitivity;
	};

	struct InputEventName
	{
		char			m_Name[256];
	};

	struct SampleInputData
	{
		char			m_InputEventName[256];
		char			m_UserInputName[256];
	};

	struct SampleInputMapping
	{
		physx::PxU16	m_InputEventId;
		size_t			m_InputEventIndex;
		physx::PxU16	m_UserInputId;
		size_t			m_UserInputIndex;
	};

	typedef std::vector<SampleInputData>		T_SampleInputData;

	enum InputType
	{
		UNDEFINED_INPUT =		0,
		KEYBOARD_INPUT =		(1 << 0),
		GAMEPAD_INPUT =			(1 << 1),
		TOUCH_BUTTON_INPUT =	(1 << 2),
		TOUCH_PAD_INPUT =		(1 << 3),
		MOUSE_INPUT =			(1 << 4),
	};

	enum InputDataReadState
	{
		STATE_INPUT_EVENT_ID,
		STATE_USER_INPUT_ID,
		STATE_DIGITAL,
		STATE_INPUT_EVENT_NAME,
	};

	class SampleUserInput
	{
	public:
		// key codes for console and raw key info
		enum KeyCode
		{
			KEY_UNKNOWN = 0,

			KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
			KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
			KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
			KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

			KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, 
			KEY_7, KEY_8, KEY_9,

			KEY_SPACE, KEY_RETURN, KEY_SHIFT, KEY_CONTROL, KEY_ESCAPE, KEY_COMMA, 
			KEY_NUMPAD0, KEY_NUMPAD1, KEY_NUMPAD2, KEY_NUMPAD3, KEY_NUMPAD4, KEY_NUMPAD5, KEY_NUMPAD6, KEY_NUMPAD7, KEY_NUMPAD8, KEY_NUMPAD9,
			KEY_MULTIPLY, KEY_ADD, KEY_SEPARATOR, KEY_SUBTRACT, KEY_DECIMAL, KEY_DIVIDE,

			KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
			KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

			KEY_TAB, KEY_PRIOR, KEY_NEXT,
			KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

			NUM_KEY_CODES,
		};

		SampleUserInput();

		virtual ~SampleUserInput();

		void						registerUserInput(physx::PxU16 id, const char* idName, const char* name);
		virtual const InputEvent*	registerInputEvent(const InputEvent& inputEvent, physx::PxU16 userInputId, const char* name);
        virtual const InputEvent*   registerTouchInputEvent(const InputEvent& inputEvent, physx::PxU16 userInputId, const char* caption, const char* name) 
		{ 
			PX_UNUSED(inputEvent);
			PX_UNUSED(userInputId);
			PX_UNUSED(caption);
			return NULL; 
		}
		virtual void				unregisterInputEvent(physx::PxU16 inputEventId);
		virtual void				registerInputEvent(const SampleInputMapping& mapping);

		virtual bool				keyboardSupported() const { return false; }
		virtual bool				gamepadSupported() const { return false; }
		virtual bool 				mouseSupported() const { return false; }

		virtual InputType			getInputType(const UserInput& ) const { return UNDEFINED_INPUT; }

		void						registerInputEventListerner(InputEventListener* listener) { mListener = listener; }
		InputEventListener*			getInputEventListener() const { return mListener; }

		virtual void				updateInput();
		virtual void				shutdown();

		virtual void				setRenderer(SampleRenderer::Renderer* ) {}

		virtual bool				getDigitalInputEventState(physx::PxU16 inputEventId ) const = 0;
		virtual float				getAnalogInputEventState(physx::PxU16 inputEventId ) const = 0;		

		const std::vector<size_t>*	getUserInputs(physx::PxI32 inputEventId) const;
		const std::vector<size_t>*	getInputEvents(physx::PxU16 userInputId) const;

		const std::vector<InputEvent>& getInputEventList() const { return mInputEvents; }
		const std::vector<InputEventName>& getInputEventNameList() const { return mInputEventNames; }
		const std::vector<UserInput>& getUserInputList() const { return mUserInputs; }
		const std::map<physx::PxU16, std::vector<size_t> >& getInputEventUserInputMap() const { return mInputEventUserInputMap; }

		physx::PxU16				getUserInputKeys(physx::PxU16 inputEventId, const char* names[], physx::PxU16 maxNames, physx::PxU32 inputTypeMask) const;

		physx::PxI32				translateUserInputNameToId(const char* name, size_t& index) const;
		physx::PxI32				translateInputEventNameToId(const char* name, size_t& index) const;
		const char*					translateInputEventIdToName(physx::PxI32 id) const;

		const InputEvent*			getInputEventSlow(physx::PxU16 inputEventId) const;		

	protected:
		virtual void				processGamepads();

		std::vector<UserInput>	mUserInputs;
		std::vector<InputEvent>	mInputEvents;
		std::vector<InputEventName>	mInputEventNames;

	private:		
		
		InputEventListener*	mListener;
		std::map<physx::PxU16, std::vector<size_t> >	mInputEventUserInputMap;
		std::map<physx::PxU16, std::vector<size_t> >	mUserInputInputEventMap;
	};

	class InputEventListener
	{
	public:
		InputEventListener() {}

		virtual ~InputEventListener() {}

		// special case for text console
		virtual void onKeyDownEx(SampleUserInput::KeyCode, physx::PxU32) {}

		virtual void onPointerInputEvent(const InputEvent&, physx::PxU32, physx::PxU32, physx::PxReal, physx::PxReal, bool val) {}
		virtual void onAnalogInputEvent(const InputEvent& , float val) = 0;
		virtual void onDigitalInputEvent(const InputEvent& , bool val) = 0;		
	};
}

#endif
