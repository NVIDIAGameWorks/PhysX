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



#ifndef GROUND_EMITTER_ASSET_IMPL_H
#define GROUND_EMITTER_ASSET_IMPL_H

#include "GroundEmitterAsset.h"
#include "GroundEmitterActor.h"
#include "ApexSDKHelpers.h"
#include "ApexAssetAuthoring.h"
#include "ApexAssetTracker.h"
#include "ApexString.h"
#include "EmitterLodParamDesc.h"
#include "InstancedObjectSimulationIntl.h"
#include "ResourceProviderIntl.h"
#include "IofxAsset.h"
#include "ApexUsingNamespace.h"
// NvParam test
#include "nvparameterized/NvParameterized.h"
#include "ParamArray.h"
#include "GroundEmitterAssetParameters.h"
#include "GroundEmitterActorParameters.h"
#include "EmitterAssetPreviewParameters.h"
#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"
#include "ApexAuthorableObject.h"

namespace nvidia
{
namespace apex
{
class GroundEmitterActor;
class GroundEmitterPreview;
}
namespace emitter
{

class GroundEmitterActorImpl;
class ModuleEmitterImpl;

///Class used to bind the emitter to a specific material
class MaterialFactoryMappingDesc : ApexDesc
{
public:
	/** \brief User defined material name.
	 * The ground emitter will convert this name into an
	 * MaterialID at runtime by a getResource() call to the named resource provider.
	 * Note this is the physical material, not rendering material.
	 */
	const char* physicalMaterialName;

	/** \brief The name of the instanced object effects asset that will render your particles */
	const char* instancedObjectEffectsAssetName;

	/** \brief The asset name of the particle system that will simulate your particles */
	const char* instancedObjectSimulationAssetName;

	/** \brief The asset type of the particle system that will simulate your particles, aka 'BasicIosAsset' */
	const char* instancedObjectSimulationTypeName;

	/** \brief The weight of this factory relative to other factories on the same material */
	float weight;

	/** \brief The maximum slope at which particles will be added to the surface in degrees where 0 is horizontal
	 * and 90 is vertical.
	 */
	float maxSlopeAngle;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE MaterialFactoryMappingDesc() : ApexDesc()
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

		if (!physicalMaterialName ||
		        !instancedObjectEffectsAssetName ||
		        !instancedObjectSimulationAssetName ||
		        !instancedObjectSimulationTypeName)
		{
			return false;
		}

		if (maxSlopeAngle > 90.0f || maxSlopeAngle < 0.0f)
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		physicalMaterialName = NULL;
		instancedObjectEffectsAssetName = NULL;
		instancedObjectSimulationAssetName = NULL;
		instancedObjectSimulationTypeName = "BasicIosAsset";
		weight = 1.0f;
		maxSlopeAngle = 90.0f;  // in default, spawn on even vertical faces
	}
};

///Ground Emitter actor descriptor. Used to create Ground Emitter actors
class GroundEmitterActorDesc : public ApexDesc
{
public:
	uint32_t			raycastCollisionGroups;

	float			radius;						///< The ground emitter actor will create objects within a circle of size 'radius'.
	uint32_t			maxRaycastsPerFrame;		///< The maximum raycasts number per frame.
	float			raycastHeight;				///< The height from which the ground emitter will cast rays at terrain/objects opposite of the 'upDirection'.
	/**
	\brief The height above the ground to emit particles.
	 If greater than 0, the ground emitter will refresh a disc above the player's position rather than
	 refreshing a circle around the player's position.

	*/
	float			density;
	float			spawnHeight;
	///< but it will back off to the minimum density if the actor is LOD resource limited.
	PxActor* 		attachActor;				///< The actor to whuch the emitter will be attatched

	PxVec3	attachRelativePosition;				///< The position of the emitter in the space of the actor, to which it is attatched.
	PxVec3	initialPosition;					///< The position of the emitter immediately after addding to the scene.

	/**
	\brief Orientatiom of the emitter.
		 An identity matrix will result in a +Y up ground emitter, provide a
	     rotation if another orientation is desired.
	*/
	PxTransform			rotation;


	/**
	\brief Pointer to callback that will be used to request materials from positions. Can be NULL; in such case a case ray will be cast
	*/
	MaterialLookupCallback* materialCallback;

	/**
	\brief constructor sets to default.
	*/
	PX_INLINE GroundEmitterActorDesc() : ApexDesc()
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

		if (!rotation.p.isZero())
		{
			return false;
		}

		return true;
	}

private:

	PX_INLINE void init()
	{
		// authored values will be used if these defaults remain
		raycastCollisionGroups = uint32_t(-1);
		radius = 0.0f;
		maxRaycastsPerFrame = 0;
		raycastHeight = 0.0f;
		spawnHeight = -1.0f;

		attachActor = NULL;
		attachRelativePosition = PxVec3(0.0f);
		initialPosition = PxVec3(0.0f);
		rotation = PxTransform(PxIdentity);

		materialCallback = NULL;
	}
};

/**
	Descriptor used to create an ApexEmitter preview.
*/
class GroundEmitterPreviewDesc : public ApexDesc
{
public:
	PxTransform				mPose;						// pose of the preview renderer
	float                    mScale;                     // scaling factor of renderable

	/**
	\brief Constructor sets to default.
	*/
	PX_INLINE GroundEmitterPreviewDesc() : ApexDesc()
	{
		setToDefault();
	}

	/**
	\brief Sets parameters to default.
	*/
	PX_INLINE void setToDefault()
	{
		ApexDesc::setToDefault();

		mPose = PxTransform(PxIdentity);
		mScale = 1.0f;
	}

	/**
	\brief Cchecks the validity of the parameters.
	*/
	PX_INLINE bool isValid() const
	{
		return ApexDesc::isValid();
	}
};


class GroundEmitterAssetImpl :	public GroundEmitterAsset,
	public ApexResourceInterface,
	public ApexResource,
	public ApexRWLockable
{
	friend class GroundEmitterAssetDummyAuthoring;
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	GroundEmitterAssetImpl(ModuleEmitterImpl*, ResourceList&, const char* name);
	GroundEmitterAssetImpl(ModuleEmitterImpl* module,
	                   ResourceList& list,
	                   NvParameterized::Interface* params,
	                   const char* name);
	~GroundEmitterAssetImpl();

	/* Asset */
	const char* 			    getName() const
	{
		return mName.c_str();
	}
	AuthObjTypeID			    getObjTypeID() const
	{
		return mAssetTypeID;
	}
	const char* 			    getObjTypeName() const
	{
		return getClassName();
	}
	uint32_t						forceLoadAssets();

	/* ApexInterface */
	virtual void			    release();

	/* ApexResourceInterface, ApexResource */
	uint32_t					    getListIndex() const
	{
		return m_listIndex;
	}
	void					    setListIndex(class ResourceList& list, uint32_t index)
	{
		m_list = &list;
		m_listIndex = index;
	}

	GroundEmitterActor* 		createActor(const GroundEmitterActorDesc&, Scene&);
	void						releaseActor(GroundEmitterActor&);

	GroundEmitterPreview*	    createEmitterPreview(const GroundEmitterPreviewDesc& desc, AssetPreviewScene* previewScene);
	void					    releaseEmitterPreview(GroundEmitterPreview& preview);

	const float &				getDensity() const
	{
		READ_ZONE();
		return mParams->density;
	}

	const PxVec3 &      getVelocityLow() const
	{
		READ_ZONE();
		return mParams->velocityLow;
	}

	const PxVec3 &      getVelocityHigh() const
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

	float                      getRadius() const
	{
		READ_ZONE();
		return mParams->radius;
	}
	uint32_t					    getMaxRaycastsPerFrame() const
	{
		READ_ZONE();
		return mParams->maxRaycastsPerFrame;
	}
	float					    getRaycastHeight() const
	{
		READ_ZONE();
		return mParams->raycastHeight;
	}
	float					    getSpawnHeight() const
	{
		READ_ZONE();
		return mParams->spawnHeight;
	}
	const char* 			    getRaycastCollisionGroupMaskName() const
	{
		READ_ZONE();
		return mParams->raycastCollisionGroupMaskName;
	}

	/* objects that assist in force loading and proper "assets own assets" behavior */
	ApexAssetTracker			mIofxAssetTracker;
	ApexAssetTracker			mIosAssetTracker;

	const NvParameterized::Interface*			getAssetNvParameterized() const
	{
		return mParams;
	}
	/**
	 * \brief Releases the ApexAsset but returns the NvParameterized::Interface and *ownership* to the caller.
	 */
	virtual NvParameterized::Interface* releaseAndReturnNvParameterizedInterface(void)
	{
		NvParameterized::Interface* ret = mParams;
		mParams = NULL;
		release();
		return ret;
	}
	NvParameterized::Interface*			getDefaultActorDesc();
	NvParameterized::Interface*			getDefaultAssetPreviewDesc();
	virtual Actor*		createApexActor(const NvParameterized::Interface& parms, Scene& apexScene);
	virtual AssetPreview* createApexAssetPreview(const NvParameterized::Interface& params, AssetPreviewScene* previewScene);

	virtual bool isValidForActorCreation(const ::NvParameterized::Interface& /*parms*/, Scene& /*apexScene*/) const
	{
		return true; // TODO implement this method
	}

	virtual bool isDirty() const
	{
		return false;
	}

protected:
	/* Typical asset members */
	static const char* 		    getClassName()
	{
		return GROUND_EMITTER_AUTHORING_TYPE_NAME;
	}
	static AuthObjTypeID	    mAssetTypeID;

	void						postDeserialize(void* userData_ = NULL);

	void                        destroy();
	ModuleEmitterImpl*              mModule;
	ResourceList              mEmitterActors;
	ApexSimpleString            mName;

	//GroundEmitterAssetParameters	mParams;
	PxFileBuf&					serializeMaterialFactory(PxFileBuf& stream, uint32_t matFactoryIdx) const;
	PxFileBuf&					deserializeMaterialFactory(PxFileBuf& stream,
	        uint32_t matFactoryIdx,
	        uint32_t headerVersion,
	        uint32_t assetVersion);

	static void					copyLodDesc(EmitterLodParamDesc& dst, const GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& src);
	static void					copyLodDesc(GroundEmitterAssetParametersNS::emitterLodParamDesc_Type& dst, const EmitterLodParamDesc& src);

	ParamArray<GroundEmitterAssetParametersNS::materialFactoryMapping_Type> *mMaterialFactoryMaps;

	void						initializeAssetNameTable();


	GroundEmitterAssetParameters*   mParams;
	GroundEmitterActorParameters*   mDefaultActorParams;
	EmitterAssetPreviewParameters*  mDefaultPreviewParams;

	uint32_t					mRaycastCollisionGroups; // cache lookup value
	physx::PxFilterData				mRaycastCollisionGroupsMask;

	bool							mShouldUseGroupsMask;

	friend class ModuleEmitterImpl;
	friend class GroundEmitterActorImpl;
	friend class GroundEmitterAssetPreview;
	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class nvidia::apex::ApexAuthorableObject;
};

#ifndef WITHOUT_APEX_AUTHORING
class GroundEmitterAssetAuthoringImpl : public GroundEmitterAssetAuthoring, public ApexAssetAuthoring, public GroundEmitterAssetImpl
{
protected:
	APEX_RW_LOCKABLE_BOILERPLATE

	GroundEmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l) :
		GroundEmitterAssetImpl(m, l, "GroundEmitterAuthor") {}

	GroundEmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l, const char* name) :
		GroundEmitterAssetImpl(m, l, name) {}

	GroundEmitterAssetAuthoringImpl(ModuleEmitterImpl* m, ResourceList& l, NvParameterized::Interface* params, const char* name) :
		GroundEmitterAssetImpl(m, l, params, name) {}

	~GroundEmitterAssetAuthoringImpl();

	EmitterLodParamDesc		mCurLodParamDesc;

public:
	void            release();
	const char* 			getName(void) const
	{
		READ_ZONE();
		return GroundEmitterAssetImpl::getName();
	}
	const char* 	getObjTypeName() const
	{
		READ_ZONE();
		return GroundEmitterAssetImpl::getClassName();
	}
	virtual bool			prepareForPlatform(nvidia::apex::PlatformTag)
	{
		WRITE_ZONE();
		APEX_INVALID_OPERATION("Not Implemented.");
		return false;
	}
	void setToolString(const char* toolName, const char* toolVersion, uint32_t toolChangelist)
	{
		WRITE_ZONE();
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	void			setDensity(const float& d)
	{
		mParams->density = d;
	}

	void            setVelocityLow(const PxVec3& v)
	{
		mParams->velocityLow = v;
	}

	void            setVelocityHigh(const PxVec3& v)
	{
		mParams->velocityHigh = v;
	}

	void            setLifetimeLow(const float& l)
	{
		mParams->lifetimeLow = l;
	}

	void            setLifetimeHigh(const float& l)
	{
		mParams->lifetimeHigh = l;
	}

	void            setRadius(float r)
	{
		mParams->radius = r;
	}

	void            setMaxRaycastsPerFrame(uint32_t m)
	{
		mParams->maxRaycastsPerFrame = m;
	}

	void            setRaycastHeight(float h)
	{
		mParams->raycastHeight = h;
	}

	void            setSpawnHeight(float h)
	{
		mParams->spawnHeight = h;
	}

	void            setRaycastCollisionGroupMaskName(const char* n)
	{
		NvParameterized::Handle stringHandle(*mParams);
		mParams->getParameterHandle("raycastCollisionGroupMaskName", stringHandle);
		mParams->setParamString(stringHandle, n);
	}

	void			setCurLodParamDesc(const EmitterLodParamDesc& d)
	{
		mCurLodParamDesc = d;
	}
	void			addMeshForGroundMaterial(const MaterialFactoryMappingDesc&);

	NvParameterized::Interface* getNvParameterized() const
	{
		READ_ZONE();
		return (NvParameterized::Interface*)getAssetNvParameterized();
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

	template <class T_Module, class T_Asset, class T_AssetAuthoring> friend class nvidia::apex::ApexAuthorableObject;
};
#endif

}
} // end namespace nvidia

#endif // GROUND_EMITTER_ASSET_IMPL_H
