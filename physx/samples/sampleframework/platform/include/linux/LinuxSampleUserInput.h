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

#ifndef LINUX_SAMPLE_USER_INPUT_H
#define LINUX_SAMPLE_USER_INPUT_H

#include <X11/Xlib.h>
#include <SampleUserInput.h>
#include <linux/LinuxSampleUserInputIds.h>
#include <set>


namespace SampleFramework
{
	class LinuxSampleUserInput: public SampleUserInput
	{
	public:

		LinuxSampleUserInput();
		~LinuxSampleUserInput();
		
		void doOnMouseMove( physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, physx::PxU16 button);
		void doOnMouseDown( physx::PxU32 x, physx::PxU32 y, physx::PxU16 button);
		void doOnMouseUp( physx::PxU32 x, physx::PxU32 y, physx::PxU16 button);
		void doOnKeyDown( KeySym keySym, physx::PxU16 keyCode, physx::PxU8 ascii);
		void doOnKeyUp( KeySym keySym, physx::PxU16 keyCode, physx::PxU8 ascii);
		
		virtual void updateInput();

		virtual void shutdown();

		virtual bool keyboardSupported() const { return true; }
		virtual bool gamepadSupported() const { return false; }
		virtual bool mouseSupported() const { return true; }

		virtual bool	getDigitalInputEventState(physx::PxU16 inputEventId) const;
		virtual float	getAnalogInputEventState(physx::PxU16 inputEventId) const;

	protected:

		void registerScanCode(LinuxSampleUserInputIds scanCodeId, physx::PxU16 scanCode, LinuxSampleUserInputIds nameId, const char* name);
		LinuxSampleUserInputIds getInputIdFromKeySym(const KeySym keySym) const;
		LinuxSampleUserInputIds getInputIdFromMouseButton(const physx::PxU16 button) const;
		const UserInput* getUserInputFromId(LinuxSampleUserInputIds id) const;		

		std::map<physx::PxU16, physx::PxU16>	m_ScanCodesMap;
		std::map<physx::PxU16,float>			m_AnalogStates;
		std::map<physx::PxU16,bool>				m_DigitalStates;
	};
}

#endif
