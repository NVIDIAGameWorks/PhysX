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



#ifndef HEADER_DebugColorParamsListener_h
#define HEADER_DebugColorParamsListener_h

#include "NvParameters.h"
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParameterizedTraits.h"

#include "DebugColorParams.h"
#include "ApexSDKIntl.h"

namespace nvidia
{
namespace apex
{

#define MAX_COLOR_NAME_LENGTH 32

class DebugColorParamsEx : public DebugColorParams
{
public:
	DebugColorParamsEx(NvParameterized::Traits* traits, ApexSDKIntl* mSdk) :
		DebugColorParams(traits),
		mApexSdk(mSdk) {}

	~DebugColorParamsEx()
	{
	}

	void destroy()
	{
		this->~DebugColorParamsEx();
		this->DebugColorParams::destroy();
	}

	NvParameterized::ErrorType setParamU32(const NvParameterized::Handle& handle, uint32_t val)
	{
		NvParameterized::ErrorType err = NvParameterized::NvParameters::setParamU32(handle, val);

		NvParameterized::Handle& h = const_cast<NvParameterized::Handle&>(handle);
		char color[MAX_COLOR_NAME_LENGTH];
		h.getLongName(color, MAX_COLOR_NAME_LENGTH);
		mApexSdk->updateDebugColorParams(color, val);

		return err;
	}

private:
	ApexSDKIntl* mApexSdk;
};

}
} // namespace nvidia::apex::

#endif