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



#ifndef NV_PARAMETERIZED_CONVERSION_TEMPLATE_H
#define NV_PARAMETERIZED_CONVERSION_TEMPLATE_H

#include <NvTraitsInternal.h>
#include <nvparameterized/NvParameterizedTraits.h>

namespace NvParameterized
{
/**
\brief Class to handle all the redundant part of version upgrades.

It verifies class names and versions, and it runs the default converter. The user may overload
convert(), getPreferredVersions() and release() methods.
*/
template<typename Told, typename Tnew, uint32_t oldVersion, uint32_t newVersion>
class ParamConversionTemplate : public NvParameterized::Conversion
{
public:
	typedef Told TOldClass;
	typedef Tnew TNewClass;

	bool operator()(NvParameterized::Interface& legacyObj, NvParameterized::Interface& obj)
	{
		if (!mDefaultConversion)
		{
			mDefaultConversion = NvParameterized::internalCreateDefaultConversion(mTraits, getPreferredVersions());
		}

		// verify class names
		if (physx::shdfnd::strcmp(legacyObj.className(), Told::staticClassName()) != 0)
		{
			return false;
		}
		if (physx::shdfnd::strcmp(obj.className(), Tnew::staticClassName()) != 0)
		{
			return false;
		}

		// verify version
		if (legacyObj.version() != oldVersion)
		{
			return false;
		}
		if (obj.version() != newVersion)
		{
			return false;
		}

		//Copy unchanged fields
		if (!(*mDefaultConversion)(legacyObj, obj))
		{
			return false;
		}

		mLegacyData = static_cast<Told*>(&legacyObj);
		mNewData = static_cast<Tnew*>(&obj);

		if (!convert())
		{
			return false;
		}

		NvParameterized::Handle invalidHandle(mNewData);
		if (!mNewData->areParamsOK(&invalidHandle, 1))
		{
			if (invalidHandle.isValid())
			{
				char buf[256];				
				physx::shdfnd::strlcpy(buf, 256, "First invalid item: ");
				invalidHandle.getLongName(buf + strlen("First invalid item: "), 256UL - static_cast<uint32_t>(strlen("First invalid item: ")));
				mTraits->traitsWarn(buf);
			}
			return false;
		}

		return true;
	}

	/// User code, frees itself with the traits, and also calls destroy() on the ParamConversionTemplate object
	virtual void release()
	{
		destroy();
		mTraits->free(this);
	}

protected:
	ParamConversionTemplate(NvParameterized::Traits* traits)
		: mTraits(traits), mDefaultConversion(0), mLegacyData(0), mNewData(0)
	{
		// Virtual method getPreferredVersions() can not be called in constructors
		// so we defer construction of mDefaultConversion
	}

	~ParamConversionTemplate()
	{
		destroy();
	}

	/// User code, return list of preferred versions.
	virtual const NvParameterized::PrefVer* getPreferredVersions() const
	{
		return 0;
	}

	/// User code, return true if conversion is successful.
	virtual bool convert()
	{
		return true;
	}

	void destroy()
	{
		if (mDefaultConversion)
		{
			mDefaultConversion->release();
		}
	}


	NvParameterized::Traits*			mTraits;
	NvParameterized::Conversion*		mDefaultConversion;

	Told*							mLegacyData;
	Tnew*							mNewData;
};
}

// Force inclusion of files with initParamRef before we redefine it below
#include "nvparameterized/NvParameterized.h"
#include "nvparameterized/NvParamUtils.h"
#include "NvParameters.h"
// Do not call initParamRef in converter - prefer Traits::createNvParameterized with explicit version
// (see wiki for more details)
#define initParamRef DO_NOT_USE_ME

#endif // PARAM_CONVERSION_TEMPLATE_H
