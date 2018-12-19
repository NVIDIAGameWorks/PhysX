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



#ifndef EMITTER_ASSET_IMPL_H
#define EMITTER_ASSET_IMPL_H

#include "Apex.h"
#include "EmitterAsset.h"
#include "EmitterGeomBase.h"
#include "ApexSDKHelpers.h"
#include "ApexAssetAuthoring.h"
#include "ApexAssetTracker.h"
#include "ApexString.h"
#include "ResourceProviderIntl.h"
#include "InstancedObjectSimulationIntl.h"
#include "EmitterLodParamDesc.h"
#include "ApexAuthorableObject.h"
#include "ApexEmitterAssetParameters.h"
#include "ApexEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"
#include "ApexUsingNamespace.h"
#include "CurveImpl.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"
#include "ApexAuthorableObject.h"

namespace nvidia
{
namespace iofx
{
class IofxAsset;
}

namespace emitter
{

class EmitterActorImpl;
class ModuleEmitterImpl;

/**
	Descriptor used to create an ApexEmitter preview.
*/
class EmitterPreviewDesc : public ApexDesc
{
public:
	PxTransform			mPose;						///< pose of the preview renderer
	float                mScale;                     ///< scaling factor of renderable

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE EmitterPreviewDesc() : ApexDesc()
	{
		setToDefault();
	}

	/**
	\brief Sets to default values.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();

		mPose = PxTransform(PxIdentity);
		mScale = 1.0f;
	}

	/**
	\brief checks the valididty of parameters.
	*/
	PX_INLINE bool isValid() const
	{
		return ApexDesc::isValid();
	}
};


class EmitterActorDesc : public ApexDesc
{
public:
	PxTransform		initialPose; ///< the pose of the emitter immediately after addding to the scene.

	PxActor* 				attachActor; ///< the actor to which the emitter will be attatched. May be NULL to keep the emitter unattatched.

	PxTransform		attachRelativePose; ///< the pose of the emitter in the space of the actor, to which it is attatched. Overrides the initial pose.

	uint32_t			overlapTestCollisionGroups; ///< collision groups used to reject particles overlapping with the geometry

	bool			emitAssetParticles; ///< indicates whether authored asset particle list will be emitted, defaults to true

	float	emitterDuration;

	float	initialScale;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE EmitterActorDesc() : ApexDesc()
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
		if (!ApexDesc::isValid())
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		initialPose = PxTransform(PxIdentity);
		attachActor = NULL;
		attachRelativePose = PxTransform(PxIdentity);
		overlapTestCollisionGroups = 0;
		emitAssetParticles = true;
		emitterDuration = PX_MAX_F32;
		initialScale = 1.0f;
	}
};

class EmitterAssetImpl : public EmitterAsset,
	public ApexResourceInterface,
	public ApexResource,
	public NvParameterized::SerializationCallback,
	public ApexRWLockable
{
	friend class ApexEmitterAssetDummyAuthoring;
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	EmitterAssetImpl(ModuleEmitterImpl*, ResourceList&, const char* name);
	EmitterAssetImpl(ModuleEmitterImpl* module,
	                 ResourceList& list,
	                 NvParameterized::Interface* params,
	                 const char* name);

	~EmitterAssetImpl();

	/* Asset */
	const char* 			    getName() const
	{
		return mName.c_str();
	}
	AuthObjTypeID			    getObjTypeID() const
	{
		READ_ZONE();
		return mAssetTypeID;
	}
	const char* 			    getObjTypeName() const
	{
		return getClassName();
	}
	uint32_t				forceLoadAssets();

	/* ApexInterface */
	virtual void			    release();

	/* ApexResourceInterface, ApexResource */
	uint32_t			    getListIndex() const
	{
		return m_listIndex;
	}
	void					    setListIndex(class ResourceList& list, uint32_t index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	/* EmitterAsset specific methods */

	EmitterGeomExplicit* 		isExplicitGeom();
	const EmitterGeomExplicit* 	isExplicitGeom() const;

	const EmitterGeom*		getGeom() const
	{
		return mGeom->getEmitterGeom();
	}

	const char* 				getInstancedObjectEffectsAssetName(void) const
	{
		READ_ZONE();
		return mParams->iofxAssetName->name();
	}
	const char* 				getInstancedObjectSimulatorAssetName(void) const
	{
		READ_ZONE();
		return mParams->iosAssetName->name();
	}
	const char* 				getInstancedObjectSimulatorTypeName(void) const
	{
		READ_ZONE();
		return mParams->iosAssetName->className();
	}

	const float &				getDensity() const
	{
		READ_ZONE();
		return mParams->density;
	}

	const float &				getRate() const
	{
		READ_ZONE();
		return mParams->rate;
	}

	const PxVec3 &     getVelocityLow() const
	{
		READ_ZONE();
		return mParams->velocityLow;
	}

	const PxVec3 &     getVelocityHigh() const
	{
		READ_ZONE();
		return mParams->velocityHigh;
	}

	const float &     getLifetimeLow() const
	{
		READ_ZONE();
		return mParams->lifetimeLow;
	}

	const float &     getLifetimeHigh() const
	{
		READ_ZONE();
		return mParams->lifetimeHigh;
	}

	uint32_t						getMaxSamples() const
	{
		READ_ZONE();
		return mParams->maxSamples;
	}

	float						getEmitDuration() const
	{
		READ_ZONE();
		return mParams->emitterDuration;
	}
	
	const EmitterLodParamDesc& getLodParamDesc() const
	{
		READ_ZONE();
		return mLodDesc;
	}

	EmitterActor* 	    createEmitterActor(const EmitterActorDesc&, const Scene&);
	void					    releaseEmitterActor(EmitterActor&);
	void                        destroy();

	EmitterPreview*	    createEmitterPreview(const EmitterPreviewDesc& desc, AssetPreviewScene* previewScene);
	void					    releaseEmitterPreview(EmitterPreview& preview);

	const NvParameterized::Interface*			getAssetNvParameterized() const
	{
		READ_ZONE();
		return mParams;
	}
	/**
	 * \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	 */
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		WRITE_ZONE();
		NvParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}

	NvParameterized::Interface*			getDefaultActorDesc();
	NvParameterized::Interface*			getDefaultAssetPreviewDesc();
	virtual Actor*		createApexActor(const NvParameterized::Interface& params, Scene& apexScene);
	virtual AssetPreview* createApexAssetPreview(const NvParameterized::Interface& params, AssetPreviewScene* previewScene);
	virtual bool isValidForActorCreation(const ::NvParameterized::Interface& actorParams, Scene& /*apexScene*/) const;

	virtual bool isDirty() const
	{
		READ_ZONE();
		return false;
	}

protected:
	/* Typical asset members */
	static const char* 		    getClassName()
	{
		return EMITTER_AUTHORING_TYPE_NAME;
	}
	static AuthObjTypeID	    mAssetTypeID;

	IofxAsset* 			getIofxAsset();                 // callable by actors

	/* NvParameterized Serialization callbacks */
	void						preSerialize(void* userData_ = NULL)
	{
		PX_UNUSED(userData_);
	}
	void						postDeserialize(void* userData_ = NULL);

	void copyLodDesc2(EmitterLodParamDesc& dst,
		const ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& src);
	void copyLodDesc2(ApexEmitterAssetParametersNS::emitterLodParamDesc_Type& dst,
		const EmitterLodParamDesc& src);


	ApexEmitterAssetParameters*	mParams;
	ApexEmitterActorParameters*  mDefaultActorParams;
	EmitterAssetPreviewParameters* mDefaultPreviewParams;

	ModuleEmitterImpl*              mModule;
	ResourceList              mEmitterActors;
	ApexSimpleString            mName;

	/* EmitterAsset specific asset members */
	EmitterGeomBase* 				mGeom;

	/* this lod is only for the getter and setter, it should mirror the parameterized lod desc */
	EmitterLodParamDesc		mLodDesc;

	/* objects that assist in force loading and proper "assets own assets" behavior */
	ApexAssetTracker			mIofxAssetTracker;
	ApexAssetTracker			mIosAssetTracker;
	void						initializeAssetNameTable();

	CurveImpl					mRateVsTimeCurve;

	friend class ModuleEmitterImpl;
	friend class EmitterActorImpl;
	friend class EmitterAssetPreview;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class nvidia::apex::ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class EmitterAssetAuthoringImpl : public EmitterAssetImpl, public ApexAssetAuthoring, public EmitterAssetAuthoring
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	/* EmitterAssetAuthoring */
	EmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l) :
		EmitterAssetImpl(m, l, "ApexEmitterAuthor") {}

	EmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l, const char* name) :
		EmitterAssetImpl(m, l, name) {}

	EmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l, NvParameterized::Interface* params, const char* name) :
		EmitterAssetImpl(m, l, params, name) {}

	~EmitterAssetAuthoringImpl() {}

	EmitterGeomBox* 			setBoxGeom();
	EmitterGeomSphere* 		setSphereGeom();
	EmitterGeomSphereShell* 	setSphereShellGeom();
	EmitterGeomExplicit* 		setExplicitGeom();

	void					setInstancedObjectEffectsAssetName(const char*);
	void					setInstancedObjectSimulatorAssetName(const char*);
	void					setInstancedObjectSimulatorTypeName(const char*);

	void					setDensity(const float& r)
	{
		mParams->density = r;
	}

	void					setRate(const float& r)
	{
		mParams->rate = r;
	}

	// Noise parameters
	void					setVelocityLow(const PxVec3& v)
	{
		mParams->velocityLow.x = v.x;
		mParams->velocityLow.y = v.y;
		mParams->velocityLow.z = v.z;
	}
	void					setVelocityHigh(const PxVec3& v)
	{
		mParams->velocityHigh.x = v.x;
		mParams->velocityHigh.y = v.y;
		mParams->velocityHigh.z = v.z;
	}

	void					setLifetimeLow(const float& l)
	{
		mParams->lifetimeLow = l;
	}
	
	void					setLifetimeHigh(const float& l)
	{
		mParams->lifetimeHigh = l;
	}


	/* Max samples is ignored if using explicit geometry */
	void					setMaxSamples(uint32_t max)
	{
		mParams->maxSamples = max;
	}
	
	void					setLodParamDesc(const EmitterLodParamDesc& d)
	{
		mLodDesc = d;
		copyLodDesc2(mParams->lodParamDesc, d);
	}

	const EmitterGeom*		getGeom() const
	{
		READ_ZONE();
		return EmitterAssetImpl::getGeom();
	}
	const char* 				getInstancedObjectEffectsAssetName(void) const
	{
		return EmitterAssetImpl::getInstancedObjectEffectsAssetName();
	}
	const char* 				getInstancedObjectSimulatorAssetName(void) const
	{
		return EmitterAssetImpl::getInstancedObjectSimulatorAssetName();
	}
	const char* 				getInstancedObjectSimulatorTypeName(void) const
	{
		return EmitterAssetImpl::getInstancedObjectSimulatorTypeName();
	}
	const float& getDensity() const
	{
		return EmitterAssetImpl::getDensity();
	}
	const float& getRate() const
	{
		return EmitterAssetImpl::getRate();
	}
	const PxVec3& getVelocityLow() const
	{
		return EmitterAssetImpl::getVelocityLow();
	}
	const PxVec3& getVelocityHigh() const
	{
		return EmitterAssetImpl::getVelocityHigh();
	}
	const float& getLifetimeLow() const
	{
		return EmitterAssetImpl::getLifetimeLow();
	}
	const float& getLifetimeHigh() const
	{
		return EmitterAssetImpl::getLifetimeHigh();
	}
	uint32_t						 getMaxSamples() const
	{
		return EmitterAssetImpl::getMaxSamples();
	}
	const EmitterLodParamDesc& getLodParamDesc() const
	{
		READ_ZONE();
		return EmitterAssetImpl::getLodParamDesc();
	}

	/* AssetAuthoring */
	const char* 			getName(void) const
	{
		READ_ZONE();
		return EmitterAssetImpl::getName();
	}
	const char* 			getObjTypeName() const
	{
		READ_ZONE();
		return EmitterAssetImpl::getClassName();
	}
	virtual bool			prepareForPlatform(nvidia::apex::PlatformTag)
	{
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void					setToolString(const char* toolName, const char* toolVersion, uint32_t toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}
	NvParameterized::Interface* getNvParameterized() const
	{
		return (NvParameterized::Interface*)getAssetNvParameterized();
	}

	/**
	* \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	*/
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = getNvParameterized();
		mParams = NULL;
		release();
		return ret;
	}


	/* ApexInterface */
	virtual void			release();
};
#endif

}
} // end namespace nvidia

#endif // EMITTER_ASSET_IMPL_H
