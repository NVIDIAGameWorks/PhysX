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

#include <SampleUserInput.h>
#include <SamplePlatform.h>

#if defined(RENDERER_LINUX)
#include <stdio.h>
#endif

using namespace SampleFramework;
namespace Ps = physx::shdfnd;

// this will prepare the input events vector, its not resize-able
static const size_t NUM_INPUT_EVENTS = 256;

SampleUserInput::SampleUserInput()
	: mListener(NULL)
{

	mUserInputs.reserve(128);
	mInputEvents.reserve(NUM_INPUT_EVENTS);
	mInputEventNames.reserve(NUM_INPUT_EVENTS);
}

SampleUserInput::~SampleUserInput()
{

	mUserInputs.clear();
}

void SampleUserInput::registerUserInput(physx::PxU16 id,const char* idName ,const char* name)
{
	UserInput ui;
	ui.m_Id = id;
	strcpy(ui.m_Name , name);
	strcpy(ui.m_IdName, idName);

	mUserInputs.push_back(ui);
}

const InputEvent* SampleUserInput::registerInputEvent(const InputEvent& inputEvent, physx::PxU16 userInputId, const char* name)
{
	size_t ieId = 0;
	const InputEvent* retIe = NULL;
	std::map<physx::PxU16, std::vector<size_t> >::iterator fit = mInputEventUserInputMap.find(inputEvent.m_Id);
	if(fit != mInputEventUserInputMap.end())
	{
		for (size_t i = mInputEvents.size(); i--;)
		{
			if(mInputEvents[i].m_Id == inputEvent.m_Id)
			{
				// the analog has a priority
				if(inputEvent.m_Analog == true)
				{
					mInputEvents[i].m_Analog = true;
				}
				ieId = i;
				break;
			}
		}
	}
	else
	{
		ieId = mInputEvents.size();
		if(ieId == NUM_INPUT_EVENTS - 1)
		{
			// increase the num input events please
			PX_ASSERT(0);
			return NULL;
		}
		else
		{
			mInputEvents.push_back(inputEvent);
			mInputEventNames.resize(mInputEventNames.size()+1);
			Ps::strlcpy(mInputEventNames.back().m_Name, sizeof(mInputEventNames.back().m_Name), name);
			retIe = &mInputEvents[ieId];
			PX_ASSERT(mInputEventNames.size() == mInputEvents.size());
		}
	}

	bool found = false;
	size_t uiId = 0;
	for (size_t i = mUserInputs.size(); i--; )
	{
		const UserInput& ui = mUserInputs[i];
		if(ui.m_Id == userInputId)
		{
			uiId = i;
			found = true;
			break;
		}
	}

	if(found)
	{
		mInputEventUserInputMap[inputEvent.m_Id].push_back(uiId);
		mUserInputInputEventMap[userInputId].push_back(ieId);
		//if(mUserInputInputEventMap[userInputId].size() > 1)
		//{
		//	char msg[256];
		//	sprintf(msg,"Key %s used multiple times",mUserInputs[uiId].m_Name);
		//	SampleFramework::SamplePlatform::platform()->showMessage("SampleUserInput warning: ",msg);
		//}
	}

	return retIe;
}

void SampleUserInput::registerInputEvent(const SampleInputMapping& mapping)
{
	std::map<physx::PxU16, std::vector<size_t> >::iterator fit = mInputEventUserInputMap.find(mapping.m_InputEventId);
	if(fit != mInputEventUserInputMap.end())
	{
		std::vector<size_t>& userInputVector = fit->second;
		bool found = false;

		for (size_t i = userInputVector.size(); i--;)
		{
			if(userInputVector[i] == mapping.m_UserInputIndex)
			{
				found = true;
			}
		}

		if(!found)
		{
			userInputVector.push_back(mapping.m_UserInputIndex);
		}
	}
	else
	{
		mInputEventUserInputMap[mapping.m_InputEventId].push_back(mapping.m_UserInputIndex);
	}

	fit = mUserInputInputEventMap.find(mapping.m_UserInputId);
	if(fit != mUserInputInputEventMap.end())
	{
		std::vector<size_t>& inputEventVector = fit->second;
		bool found = false;

		for (size_t i = inputEventVector.size(); i--;)
		{
			if(inputEventVector[i] == mapping.m_InputEventIndex)
			{
				found = true;
			}
		}

		if(!found)
		{
			inputEventVector.push_back(mapping.m_InputEventIndex);
		}
	}
	else
	{
		mUserInputInputEventMap[mapping.m_UserInputId].push_back(mapping.m_InputEventIndex);
	}
}

void SampleUserInput::unregisterInputEvent(physx::PxU16 inputEventId)
{
	std::map<physx::PxU16, std::vector<size_t> >::iterator fit = mInputEventUserInputMap.find(inputEventId);
	if(fit != mInputEventUserInputMap.end())
	{
		const std::vector<size_t>& userInputs = fit->second;
		for (size_t j = userInputs.size(); j--;)
		{
			const UserInput& ui = mUserInputs[userInputs[j]];
			std::map<physx::PxU16, std::vector<size_t> >::iterator uifit = mUserInputInputEventMap.find(ui.m_Id);
			if(uifit != mUserInputInputEventMap.end())
			{
				std::vector<size_t>& inputEvents = uifit->second;
				for (size_t u = inputEvents.size(); u--;)
				{
					if(mInputEvents[inputEvents[u]].m_Id == inputEventId)
					{
						inputEvents[u] = inputEvents.back();
						inputEvents.pop_back();
					}
				}
			}
		}

		mInputEventUserInputMap.erase(fit);
	}
}

void SampleUserInput::updateInput()
{

}

void SampleUserInput::processGamepads()
{
}

const InputEvent* SampleUserInput::getInputEventSlow(physx::PxU16 inputEventId) const
{
	for (size_t i = mInputEvents.size(); i--;)
	{
		if(mInputEvents[i].m_Id == inputEventId)
		{
			return &mInputEvents[i];
		}
	}

	return NULL;
}

const std::vector<size_t>* SampleUserInput::getUserInputs(physx::PxI32 inputEventId) const
{
	if(inputEventId < 0)
		return NULL;

	std::map<physx::PxU16, std::vector<size_t> >::const_iterator fit = mInputEventUserInputMap.find((physx::PxU16)inputEventId);
	if(fit != mInputEventUserInputMap.end())
	{
		return &fit->second;
	}
	else
	{
		return NULL;
	}
}

const std::vector<size_t>* SampleUserInput::getInputEvents(physx::PxU16 userInputId) const
{
	std::map<physx::PxU16, std::vector<size_t> >::const_iterator fit = mUserInputInputEventMap.find(userInputId);
	if(fit != mUserInputInputEventMap.end())
	{
		return &fit->second;
	}
	else
	{
		return NULL;
	}
}

physx::PxI32 SampleUserInput::translateInputEventNameToId(const char* name, size_t& index) const
{
	PX_ASSERT(mInputEvents.size() == mInputEventNames.size());
	for (size_t i = mInputEventNames.size(); i--;)
	{
		if(!strcmp(mInputEventNames[i].m_Name, name))
		{
			index = i;
			return mInputEvents[i].m_Id;
		}
	}
	return -1;
}

const char* SampleUserInput::translateInputEventIdToName(physx::PxI32 id) const
{
	PX_ASSERT(mInputEvents.size() == mInputEventNames.size());
	for (size_t i = mInputEvents.size(); i--;)
	{
		if(mInputEvents[i].m_Id == id)
		{
			return mInputEventNames[i].m_Name;
		}
	}
	return NULL;
}

physx::PxI32 SampleUserInput::translateUserInputNameToId(const char* name, size_t& index) const
{
	for (size_t i = mUserInputs.size(); i--;)
	{
		if(!strcmp(mUserInputs[i].m_IdName, name))
		{
			index = i;
			return mUserInputs[i].m_Id;
		}
	}
	return -1;
}

void SampleUserInput::shutdown()
{
	mInputEvents.clear();
	mInputEventNames.clear();

	mUserInputInputEventMap.clear();
	mInputEventUserInputMap.clear();
}

physx::PxU16 SampleUserInput::getUserInputKeys(physx::PxU16 inputEventId, const char* names[], physx::PxU16 maxNames, physx::PxU32 inputTypeMask) const
{
	physx::PxU16 retVal = 0;

	const std::vector<size_t>* uis = getUserInputs(inputEventId);
	if(uis)
	{
		for (size_t i = uis->size(); i--;)
		{
			if(retVal < maxNames)
			{
				const UserInput& ie = mUserInputs[(*uis)[i]];
				InputType it = getInputType(ie);
				if(it)
				{
					if(it & inputTypeMask)
					{
						names[retVal] = ie.m_Name;
						retVal++;
					}
				}
				else
				{
					names[retVal] = ie.m_Name;
					retVal++;
				}
			}
		}
	}

	return retVal;
}
