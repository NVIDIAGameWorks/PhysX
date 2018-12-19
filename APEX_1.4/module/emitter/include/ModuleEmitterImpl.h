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



#ifndef __MODULE_EMITTER_IMPL_H__
#define __MODULE_EMITTER_IMPL_H__

#include "Apex.h"
#include "ModuleEmitter.h"
#include "ApexSDKIntl.h"
#include "ModuleBase.h"
#include "ModuleIntl.h"
#include "EmitterAssetImpl.h"
#include "GroundEmitterAssetImpl.h"
#include "ModulePerfScope.h"
#include "ImpactObjectEvent.h"

#include "ModuleEmitterRegistration.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace emitter
{

class EmitterScene;

class ModuleEmitterDesc : public ApexDesc
{
public:

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE ModuleEmitterDesc() : ApexDesc()
	{
		init();
	}

	/**
	\brief sets members to default values.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();
		init();
	}

	/**
	\brief checks if this is a valid descriptor.
	*/
	PX_INLINE bool isValid() const
	{
		bool retVal = ApexDesc::isValid();
		return retVal;
	}


private:

	PX_INLINE void init()
	{
	}
};

class ModuleEmitterImpl : public ModuleEmitter, public ModuleIntl, public ModuleBase, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ModuleEmitterImpl(ApexSDKIntl* inSdk);
	~ModuleEmitterImpl();

	void						init(const ModuleEmitterDesc& moduleEmitterDesc);

	// base class methods
	void						init(NvParameterized::Interface&) {}
	NvParameterized::Interface* getDefaultModuleDesc();
	void						release()
	{
		ModuleBase::release();
	}
	void						destroy();
	const char*					getName() const
	{
		READ_ZONE();
		return ModuleBase::getName();
	}

	RenderableIterator* 	createRenderableIterator(const Scene&);
	ModuleSceneIntl* 				createInternalModuleScene(SceneIntl&, RenderDebugInterface*);
	void						releaseModuleSceneIntl(ModuleSceneIntl&);
	uint32_t				forceLoadAssets();

	AuthObjTypeID				getModuleID() const;
	AuthObjTypeID             getEmitterAssetTypeID() const;

	ApexActor* 					getApexActor(Actor*, AuthObjTypeID) const;

	float getRateScale() const
	{
		return mRateScale;
	}
	float getDensityScale() const
	{
		return mDensityScale;
	}
	float getGroundDensityScale() const
	{
		return mGroundDensityScale;
	}

	void setRateScale(float rateScale)
	{
		mRateScale = rateScale;
	}
	void setDensityScale(float densityScale)
	{
		mDensityScale = densityScale;
	}
	void setGroundDensityScale(float groundDensityScale)
	{
		mGroundDensityScale = groundDensityScale;
	}

private:
	EmitterScene* 				getEmitterScene(const Scene& apexScene);
	ResourceList				mAuthorableObjects;
	ResourceList				mEmitterScenes;

private:

	EmitterModuleParameters* 			mModuleParams;

	float		mRateScale;
	float		mDensityScale;
	float		mGroundDensityScale;

	friend class EmitterAssetImpl;
	friend class GroundEmitterAssetImpl;
	friend class GroundEmitterActorImpl;
	friend class ImpactEmitterAssetImpl;
	friend class ImpactEmitterActorImpl;
};

}
} // namespace nvidia::apex

#endif // __MODULE_EMITTER_IMPL_H__
