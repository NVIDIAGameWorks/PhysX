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



#ifndef __DESTRUCTIBLEACTOR_PROXY_H__
#define __DESTRUCTIBLEACTOR_PROXY_H__

#include "Apex.h"
#include "DestructibleActor.h"
#include "DestructibleActorJointProxy.h"
#include "DestructibleActorImpl.h"
#include "DestructibleScene.h"
#include "PsUserAllocated.h"
#include "ApexActor.h"
#if APEX_USE_PARTICLES
#include "EmitterActor.h"
#endif

#include "ApexRWLockable.h"
#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace destructible
{

class DestructibleActorProxy : public DestructibleActor, public ApexResourceInterface, public UserAllocated, public ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	DestructibleActorImpl impl;

#pragma warning(disable : 4355) // disable warning about this pointer in argument list
	DestructibleActorProxy(const NvParameterized::Interface& input, DestructibleAssetImpl& asset, ResourceList& list, DestructibleScene& scene)
		: impl(this, asset, scene)
	{
		WRITE_ZONE();
		NvParameterized::Interface* clonedInput = NULL;
		input.clone(clonedInput);
		list.add(*this);	// Doing this before impl.initialize, since the render proxy created in that function wants a unique ID (only needs to be unique at any given time, can be recycled)
		impl.initialize(clonedInput);
	}

	DestructibleActorProxy(NvParameterized::Interface* input, DestructibleAssetImpl& asset, ResourceList& list, DestructibleScene& scene)
		: impl(this, asset, scene)
	{
		WRITE_ZONE();
		impl.initialize(input);
		list.add(*this);
	}

	~DestructibleActorProxy()
	{
	}

	virtual const DestructibleParameters& getDestructibleParameters() const
	{
		READ_ZONE();
		return impl.getDestructibleParameters();
	}

	virtual void setDestructibleParameters(const DestructibleParameters& destructibleParameters)
	{
		WRITE_ZONE();
		impl.setDestructibleParameters(destructibleParameters);
	}

	virtual const RenderMeshActor* getRenderMeshActor(DestructibleActorMeshType::Enum type = DestructibleActorMeshType::Skinned) const
	{
		READ_ZONE();
		return impl.getRenderMeshActor(type);
	}

	virtual PxMat44	getInitialGlobalPose() const
	{
		READ_ZONE();
		return impl.getInitialGlobalPose();
	}

	virtual void setInitialGlobalPose(const PxMat44& pose)
	{
		WRITE_ZONE();
		impl.setInitialGlobalPose(pose);
	}

	virtual PxVec3	getScale() const
	{
		READ_ZONE();
		return impl.getScale();
	}

	virtual void	applyDamage(float damage, float momentum, const PxVec3& position, const PxVec3& direction, int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX, void* damageActorUserData = NULL)
	{
		WRITE_ZONE();
		return impl.applyDamage(damage, momentum, position, direction, chunkIndex, damageActorUserData);
	}

	virtual void	applyRadiusDamage(float damage, float momentum, const PxVec3& position, float radius, bool falloff, void* damageActorUserData = NULL)
	{
		WRITE_ZONE();
		return impl.applyRadiusDamage(damage, momentum, position, radius, falloff, damageActorUserData);
	}	

	virtual void	getChunkVisibilities(uint8_t* visibilityArray, uint32_t visibilityArraySize) const
	{
		READ_ZONE();
		impl.getChunkVisibilities(visibilityArray, visibilityArraySize);
	}

	virtual uint32_t	getNumVisibleChunks() const
	{
		READ_ZONE();
		return impl.getNumVisibleChunks();
	}

	virtual const uint16_t*	getVisibleChunks() const
	{
		READ_ZONE();
		return impl.getVisibleChunks();
	}

	virtual bool					acquireChunkEventBuffer(const DestructibleChunkEvent*& buffer, uint32_t& bufferSize)
	{
		READ_ZONE();
		return impl.acquireChunkEventBuffer(buffer, bufferSize);
	}

	virtual bool					releaseChunkEventBuffer(bool clearBuffer = true)
	{
		READ_ZONE();
		return impl.releaseChunkEventBuffer(clearBuffer);
	}

	virtual bool					acquirePhysXActorBuffer(physx::PxRigidDynamic**& buffer, uint32_t& bufferSize, uint32_t flags = DestructiblePhysXActorQueryFlags::AllStates)
	{
		READ_ZONE();
		return impl.acquirePhysXActorBuffer(buffer, bufferSize, flags);
	}

	virtual bool					releasePhysXActorBuffer()
	{
		READ_ZONE();
		return impl.releasePhysXActorBuffer();
	}

	virtual physx::PxRigidDynamic*	getChunkPhysXActor(uint32_t index)
	{
		READ_ZONE();
		physx::PxActor* actor = impl.getChunkActor(index);
		PX_ASSERT(actor == NULL || actor->is<physx::PxRigidDynamic>());
		return (physx::PxRigidDynamic*)actor;
	}

	virtual uint32_t		getChunkPhysXShapes(physx::PxShape**& shapes, uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkPhysXShapes(shapes, chunkIndex);
	}

	virtual PxTransform		getChunkPose(uint32_t index) const
	{
		READ_ZONE();
		return impl.getChunkPose(index);
	}

	virtual PxTransform		getChunkTransform(uint32_t index) const
	{
		READ_ZONE();
		return impl.getChunkTransform(index);
	}

	virtual PxVec3			getChunkLinearVelocity(uint32_t index) const
	{
		READ_ZONE();
		return impl.getChunkLinearVelocity(index);
	}

	virtual PxVec3			getChunkAngularVelocity(uint32_t index) const
	{
		READ_ZONE();
		return impl.getChunkAngularVelocity(index);
	}

	virtual const PxMat44	getChunkTM(uint32_t index) const
	{
		READ_ZONE();
		return impl.getChunkTM(index);
	}

	virtual int32_t			getChunkBehaviorGroupIndex(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getBehaviorGroupIndex(chunkIndex);
	}

	virtual uint32_t		 	getChunkActorFlags(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkActorFlags(chunkIndex);
	}

	virtual bool					isChunkDestroyed(int32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.isChunkDestroyed((uint32_t)chunkIndex);
	}

	virtual void			setSkinnedOverrideMaterial(uint32_t index, const char* overrideMaterialName)
	{
		WRITE_ZONE();
		impl.setSkinnedOverrideMaterial(index, overrideMaterialName);
	}

	virtual void			setStaticOverrideMaterial(uint32_t index, const char* overrideMaterialName)
	{
		WRITE_ZONE();
		impl.setStaticOverrideMaterial(index, overrideMaterialName);
	}

	virtual void			setRuntimeFractureOverridePattern(const char* overridePatternName)
	{
		WRITE_ZONE();
		impl.setRuntimeFracturePattern(overridePatternName);
	}

	virtual bool			isInitiallyDynamic() const
	{
		READ_ZONE();
		return impl.isInitiallyDynamic();
	}

	virtual void			setLinearVelocity(const PxVec3& linearVelocity)
	{
		WRITE_ZONE();
		impl.setLinearVelocity(linearVelocity);
	}

	virtual void			setGlobalPose(const PxMat44& pose)
	{
		WRITE_ZONE();
		impl.setGlobalPose(pose);
	}

	virtual bool			getGlobalPose(PxMat44& pose)
	{
		READ_ZONE();
		return impl.getGlobalPoseForStaticChunks(pose);
	}

	virtual void			setAngularVelocity(const PxVec3& angularVelocity)
	{
		WRITE_ZONE();
		impl.setAngularVelocity(angularVelocity);
	}

	virtual void setDynamic(int32_t chunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX)
	{
		WRITE_ZONE();
		impl.setDynamic(chunkIndex);
	}

	virtual bool isDynamic(uint32_t chunkIndex) const
	{
		READ_ZONE();
		if (impl.getDestructibleAsset() != NULL && chunkIndex < impl.getDestructibleAsset()->getChunkCount())
		{
			return impl.getDynamic((int32_t)chunkIndex);
		}
		return false;
	}

	virtual void enableHardSleeping()
	{
		WRITE_ZONE();
		impl.enableHardSleeping();
	}

	virtual void disableHardSleeping(bool wake = false)
	{
		WRITE_ZONE();
		impl.disableHardSleeping(wake);
	}

	virtual bool isHardSleepingEnabled() const
	{
		READ_ZONE();
		return impl.useHardSleeping();
	}

	virtual	bool setChunkPhysXActorAwakeState(uint32_t chunkIndex, bool awake)
	{
		WRITE_ZONE();
		return impl.setChunkPhysXActorAwakeState(chunkIndex, awake);
	}

	virtual bool addForce(uint32_t chunkIndex, const PxVec3& force, physx::PxForceMode::Enum mode, const PxVec3* position = NULL, bool wakeup = true)
	{
		WRITE_ZONE();
		return impl.addForce(chunkIndex, force, mode, position, wakeup);
	}

	virtual int32_t			rayCast(float& time, PxVec3& normal, const PxVec3& worldRayOrig, const PxVec3& worldRayDir, DestructibleActorRaycastFlags::Enum flags, int32_t parentChunkIndex = ModuleDestructibleConst::INVALID_CHUNK_INDEX) const
	{
		READ_ZONE();
		return impl.rayCast(time, normal, worldRayOrig, worldRayDir, flags, parentChunkIndex);
	}

	virtual int32_t			obbSweep(float& time, PxVec3& normal, const PxVec3& worldBoxCenter, const PxVec3& worldBoxExtents, const PxMat33& worldBoxRot, const PxVec3& worldDisplacement, DestructibleActorRaycastFlags::Enum flags) const
	{
		READ_ZONE();
		return impl.obbSweep(time, normal, worldBoxCenter, worldBoxExtents, worldBoxRot, worldDisplacement, flags);
	}

	virtual void cacheModuleData() const
	{
		READ_ZONE();
		return impl.cacheModuleData();
	}

	virtual PxBounds3		getLocalBounds() const
	{
		READ_ZONE();
		return impl.getLocalBounds();
	}

	virtual PxBounds3		getOriginalBounds() const
	{ 
		READ_ZONE();
		return impl.getOriginalBounds();
	}

	virtual bool					isChunkSolitary(int32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.isChunkSolitary( chunkIndex );
	}

	virtual PxBounds3		getChunkBounds(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkBounds( chunkIndex );
	}

	virtual PxBounds3		getChunkLocalBounds(uint32_t chunkIndex) const
	{
		READ_ZONE();
		return impl.getChunkLocalBounds( chunkIndex );
	}

	virtual uint32_t			getSupportDepthChunkIndices(uint32_t* const OutChunkIndices, uint32_t MaxOutIndices) const
	{
		READ_ZONE();
		return impl.getSupportDepthChunkIndices( OutChunkIndices, MaxOutIndices );
	}

	virtual uint32_t			getSupportDepth() const
	{ 
		READ_ZONE();
		return impl.getSupportDepth();
	}

	// Actor methods
	virtual Asset* getOwner() const
	{
		READ_ZONE();
		return impl.mAsset->getAsset();
	}
	virtual void release()
	{
		impl.release();
	}
	virtual void destroy()
	{
		impl.destroy();

		delete this;
	}

	virtual DestructibleRenderable* acquireRenderableReference()
	{
		return impl.acquireRenderableReference();
	}

	// Renderable methods
	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		URR_SCOPE;
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}

	virtual void dispatchRenderResources(UserRenderer& api)
	{
		impl.dispatchRenderResources(api);
	}

	virtual void lockRenderResources()
	{
		Renderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->lockRenderResources();
		}
	}

	virtual void unlockRenderResources()
	{
		Renderable* renderable = impl.getRenderable();
		if (renderable != NULL)
		{
			renderable->unlockRenderResources();
		}
	}

	virtual PxBounds3 getBounds() const
	{
		READ_ZONE();
		Renderable* renderable = const_cast<DestructibleActorImpl*>(&impl)->getRenderable();
		if (renderable != NULL)
		{
			return renderable->getBounds();
		}
		return impl.getBounds();
	}

	virtual void setPhysX3Template(const PhysX3DescTemplate* desc)
	{
		WRITE_ZONE();
		impl.setPhysX3Template(desc);
	}

	virtual bool getPhysX3Template(PhysX3DescTemplate& dest) const
	{
		READ_ZONE();
		return impl.getPhysX3Template(dest);
	}

	PhysX3DescTemplate* createPhysX3DescTemplate() const
	{
		return impl.createPhysX3DescTemplate();
	}

	// ApexResourceInterface methods
	virtual void	setListIndex(ResourceList& list, uint32_t index)
	{
		WRITE_ZONE();
		impl.m_listIndex = index;
		impl.m_list = &list;
	}
	virtual uint32_t	getListIndex() const
	{
		READ_ZONE();
		return impl.m_listIndex;
	}

	virtual void getLodRange(float& min, float& max, bool& intOnly) const
	{
		READ_ZONE();
		impl.getLodRange(min, max, intOnly);
	}

	virtual float getActiveLod() const
	{
		READ_ZONE();
		return impl.getActiveLod();
	}

	virtual void forceLod(float lod)
	{
		WRITE_ZONE();
		impl.forceLod(lod);
	}

	/**
	\brief Selectively enables/disables debug visualization of a specific APEX actor.  Default value it true.
	*/
	virtual void setEnableDebugVisualization(bool state)
	{
		WRITE_ZONE();
		impl.setEnableDebugVisualization(state);
	}


	virtual void setCrumbleEmitterState(bool enable)
	{
		WRITE_ZONE();
		impl.setCrumbleEmitterEnabled(enable);
	}

	virtual void setDustEmitterState(bool enable)
	{
		WRITE_ZONE();
		impl.setDustEmitterEnabled(enable);
	}

	/**
		Sets a preferred render volume for a dust or crumble emitter
	*/
	virtual void setPreferredRenderVolume(RenderVolume* volume, DestructibleEmitterType::Enum type)
	{
		WRITE_ZONE();
		impl.setPreferredRenderVolume(volume, type);
	}

	virtual EmitterActor* getApexEmitter(DestructibleEmitterType::Enum type)
	{
		READ_ZONE();
		return impl.getApexEmitter(type);
	}

	virtual bool recreateApexEmitter(DestructibleEmitterType::Enum type)
	{
		WRITE_ZONE();
		return impl.recreateApexEmitter(type);
	}

	const NvParameterized::Interface* getNvParameterized(DestructibleParameterizedType::Enum type) const
	{
		READ_ZONE();
		switch (type) 
		{
		case DestructibleParameterizedType::State:
			return (const NvParameterized::Interface*)impl.getState();
		case DestructibleParameterizedType::Params:
			return (const NvParameterized::Interface*)impl.getParams();
		default:
			return NULL;
		}
	}

	void setNvParameterized(NvParameterized::Interface* params)
	{
		WRITE_ZONE();
		impl.initialize(params);
	}

	const NvParameterized::Interface* getChunks() const
	{
		return (const NvParameterized::Interface*)impl.getChunks();
	}

	void setChunks(NvParameterized::Interface* chunks)
	{
		impl.initialize(chunks);
	}

    virtual bool setSyncParams(uint32_t userActorID, uint32_t actorSyncFlags, const DestructibleActorSyncState * actorSyncState, const DestructibleChunkSyncState * chunkSyncState)
    {
		WRITE_ZONE();
        return impl.setSyncParams(userActorID, actorSyncFlags, actorSyncState, chunkSyncState);
    }

	virtual bool setHitChunkTrackingParams(bool flushHistory, bool startTracking, uint32_t trackingDepth, bool trackAllChunks)
	{
		WRITE_ZONE();
		return impl.setHitChunkTrackingParams(flushHistory, startTracking, trackingDepth, trackAllChunks);
	}

	virtual bool getHitChunkHistory(const DestructibleHitChunk *& hitChunkContainer, uint32_t & hitChunkCount) const
	{
		READ_ZONE();
		return impl.getHitChunkHistory(hitChunkContainer, hitChunkCount);
	}

	virtual bool forceChunkHits(const DestructibleHitChunk * hitChunkContainer, uint32_t hitChunkCount, bool removeChunks = true, bool deferredEvent = false, PxVec3 damagePosition = PxVec3(0.0f), PxVec3 damageDirection = PxVec3(0.0f))
	{
		WRITE_ZONE();
		return impl.forceChunkHits(hitChunkContainer, hitChunkCount, removeChunks, deferredEvent, damagePosition, damageDirection);
	}

	virtual bool getDamageColoringHistory(const DamageEventCoreData *& damageEventCoreDataContainer, uint32_t & damageEventCoreDataCount) const
	{
		READ_ZONE();
		return impl.getDamageColoringHistory(damageEventCoreDataContainer, damageEventCoreDataCount);
	}

	virtual bool forceDamageColoring(const DamageEventCoreData * damageEventCoreDataContainer, uint32_t damageEventCoreDataCount)
	{
		WRITE_ZONE();
		return impl.forceDamageColoring(damageEventCoreDataContainer, damageEventCoreDataCount);
	}

	virtual void setDeleteFracturedChunks(bool inDeleteChunkMode)
	{
		WRITE_ZONE();
		impl.setDeleteFracturedChunks(inDeleteChunkMode);
	}

	virtual void takeImpact(const PxVec3& force, const PxVec3& position, uint16_t chunkIndex, PxActor const* damageImpactActor)
	{
		WRITE_ZONE();
		impl.takeImpact(force, position, chunkIndex, damageImpactActor);
	}

	virtual uint32_t			getCustomBehaviorGroupCount() const
	{
		READ_ZONE();
		return impl.getCustomBehaviorGroupCount();
	}

	virtual bool					getBehaviorGroup(DestructibleBehaviorGroupDesc& behaviorGroupDesc, int32_t index = -1) const
	{
		READ_ZONE();
		return impl.getBehaviorGroup(behaviorGroupDesc, index);
	}
};


}
} // end namespace nvidia

#endif // __DESTRUCTIBLEACTOR_PROXY_H__
