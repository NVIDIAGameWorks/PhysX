// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.

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
