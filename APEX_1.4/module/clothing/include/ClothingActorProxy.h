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



#ifndef CLOTHING_ACTOR_PROXY_H
#define CLOTHING_ACTOR_PROXY_H

#include "ClothingActor.h"
#include "PsUserAllocated.h"
#include "ClothingAssetImpl.h"
#include "ApexRWLockable.h"
#include "ClothingActorImpl.h"

#include "ReadCheck.h"
#include "WriteCheck.h"

namespace nvidia
{
namespace clothing
{

class ClothingActorProxy : public ClothingActor, public UserAllocated, public ApexResourceInterface, ApexRWLockable
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingActorImpl impl;

#pragma warning(push)
#pragma warning( disable : 4355 ) // disable warning about this pointer in argument list

	ClothingActorProxy(const NvParameterized::Interface& desc, ClothingAssetImpl* asset, ClothingScene& clothingScene, ResourceList* list) :
		impl(desc, this, NULL, asset, &clothingScene)
	{
		list->add(*this);
	}

#pragma warning(pop)

	virtual void release()
	{
		impl.release();			// calls release method on asset
	}

	virtual uint32_t getListIndex() const
	{
		return impl.m_listIndex;
	}

	virtual void setListIndex(class ResourceList& list, uint32_t index)
	{
		impl.m_list = &list;
		impl.m_listIndex = index;
	}

#ifndef WITHOUT_PVD
	virtual void initPvdInstances(pvdsdk::PvdDataStream& pvdStream)
	{
		impl.initPvdInstances(pvdStream);
	}
#endif

	virtual void destroy()
	{
		impl.destroy();
		delete this;
	}

	// Actor base class
	virtual Asset* getOwner() const
	{
		return impl.getOwner();
	}
	virtual PxBounds3 getBounds() const
	{
		return impl.getBounds();
	}

	virtual void lockRenderResources()
	{
		impl.lockRenderResources();
	}

	virtual void unlockRenderResources()
	{
		impl.unlockRenderResources();
	}

	virtual void updateRenderResources(bool rewriteBuffers, void* userRenderData)
	{
		URR_SCOPE;
		impl.updateRenderResources(rewriteBuffers, userRenderData);
	}

	virtual void dispatchRenderResources(UserRenderer& renderer)
	{
		impl.dispatchRenderResources(renderer);
	}

	virtual NvParameterized::Interface* getActorDesc()
	{
		READ_ZONE();
		return impl.getActorDesc();
	}

	virtual void updateState(const PxMat44& globalPose, const PxMat44* newBoneMatrices, uint32_t boneMatricesByteStride, uint32_t numBoneMatrices, ClothingTeleportMode::Enum teleportMode)
	{
		WRITE_ZONE();
		impl.updateState(globalPose, newBoneMatrices, boneMatricesByteStride, numBoneMatrices, teleportMode);
	}

	virtual void updateMaxDistanceScale(float scale, bool multipliable)
	{
		WRITE_ZONE();
		impl.updateMaxDistanceScale(scale, multipliable);
	}

	virtual const PxMat44& getGlobalPose() const
	{
		READ_ZONE();
		return impl.getGlobalPose();
	}

	virtual void setWind(float windAdaption, const PxVec3& windVelocity)
	{
		WRITE_ZONE();
		impl.setWind(windAdaption, windVelocity);
	}

	virtual void setMaxDistanceBlendTime(float blendTime)
	{
		WRITE_ZONE();
		impl.setMaxDistanceBlendTime(blendTime);
	}

	virtual float getMaxDistanceBlendTime() const
	{
		READ_ZONE();
		return impl.getMaxDistanceBlendTime();
	}

	virtual void getPhysicalMeshPositions(void* buffer, uint32_t byteStride)
	{
		READ_ZONE();
		impl.getPhysicalMeshPositions(buffer, byteStride);
	}

	virtual void getPhysicalMeshNormals(void* buffer, uint32_t byteStride)
	{
		READ_ZONE();
		impl.getPhysicalMeshNormals(buffer, byteStride);
	}

	virtual float getMaximumSimulationBudget() const
	{
		READ_ZONE();
		return impl.getMaximumSimulationBudget();
	}

	virtual uint32_t getNumSimulationVertices() const
	{
		READ_ZONE();
		return impl.getNumSimulationVertices();
	}

	virtual const PxVec3* getSimulationPositions()
	{
		READ_ZONE();
		return impl.getSimulationPositions();
	}

	virtual const PxVec3* getSimulationNormals()
	{
		READ_ZONE();
		return impl.getSimulationNormals();
	}

	virtual bool getSimulationVelocities(PxVec3* velocities)
	{
		READ_ZONE();
		return impl.getSimulationVelocities(velocities);
	}

	virtual uint32_t getNumGraphicalVerticesActive(uint32_t submeshIndex) const
	{
		READ_ZONE();
		return impl.getNumGraphicalVerticesActive(submeshIndex);
	}

	virtual PxMat44 getRenderGlobalPose() const
	{
		READ_ZONE();
		return impl.getRenderGlobalPose();
	}

	virtual const PxMat44* getCurrentBoneSkinningMatrices() const
	{
		READ_ZONE();
		return impl.getCurrentBoneSkinningMatrices();
	}

	virtual void setVisible(bool enable)
	{
		WRITE_ZONE();
		impl.setVisible(enable);
	}

	virtual bool isVisible() const
	{
		READ_ZONE();
		return impl.isVisibleBuffered();
	}

	virtual void setFrozen(bool enable)
	{
		WRITE_ZONE();
		impl.setFrozen(enable);
	}

	virtual bool isFrozen() const
	{
		READ_ZONE();
		return impl.isFrozenBuffered();
	}

	virtual ClothSolverMode::Enum getClothSolverMode() const
	{
		READ_ZONE();
		return impl.getClothSolverMode();
	}

	virtual void setGraphicalLOD(uint32_t lod)
	{
		WRITE_ZONE();
		impl.setGraphicalLOD(lod);
	}

	virtual uint32_t getGraphicalLod()
	{
		READ_ZONE();
		return impl.getGraphicalLod();
	}

	virtual bool rayCast(const PxVec3& worldOrigin, const PxVec3& worldDirection, float& time, PxVec3& normal, uint32_t& vertexIndex)
	{
		READ_ZONE();
		return impl.rayCast(worldOrigin, worldDirection, time, normal, vertexIndex);
	}

	virtual void attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& globalPosition)
	{
		WRITE_ZONE();
		impl.attachVertexToGlobalPosition(vertexIndex, globalPosition);
	}

	virtual void freeVertex(uint32_t vertexIndex)
	{
		WRITE_ZONE();
		impl.freeVertex(vertexIndex);
	}

	virtual uint32_t getClothingMaterial() const
	{
		READ_ZONE();
		return impl.getClothingMaterial();
	}

	virtual void setClothingMaterial(uint32_t index)
	{
		WRITE_ZONE();
		impl.setClothingMaterial(index);
	}

	virtual void setOverrideMaterial(uint32_t submeshIndex, const char* overrideMaterialName)
	{
		WRITE_ZONE();
		impl.setOverrideMaterial(submeshIndex, overrideMaterialName);
	}

	virtual void setVelocityCallback(ClothingVelocityCallback* callback)
	{
		WRITE_ZONE();
		impl.setVelocityCallback(callback);
	}

	virtual void setInterCollisionChannels(uint32_t channels)
	{
		WRITE_ZONE();
		impl.setInterCollisionChannels(channels);
	}

	virtual uint32_t getInterCollisionChannels()
	{
		READ_ZONE();
		return impl.getInterCollisionChannels();
	}

	virtual bool isHalfPrecisionAllowed() const
	{
		READ_ZONE();
		return impl.isHalfPrecisionAllowed();
	}

	virtual void setHalfPrecision(bool isAllowed)
	{
		WRITE_ZONE();
		return impl.setHalfPrecision(isAllowed);
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
		impl.setEnableDebugVisualization(state);
	}

	virtual ClothingPlane* createCollisionPlane(const PxPlane& plane)
	{
		WRITE_ZONE();
		return impl.createCollisionPlane(plane);
	}

	virtual ClothingConvex* createCollisionConvex(ClothingPlane** planes, uint32_t numPlanes)
	{
		WRITE_ZONE();
		return impl.createCollisionConvex(planes, numPlanes);
	}

	virtual ClothingSphere* createCollisionSphere(const PxVec3& position, float radius)
	{
		WRITE_ZONE();
		return impl.createCollisionSphere(position, radius);
	}

	virtual ClothingCapsule* createCollisionCapsule(ClothingSphere& sphere1, ClothingSphere& sphere2) 
	{
		WRITE_ZONE();
		return impl.createCollisionCapsule(sphere1, sphere2);
	}

	virtual ClothingTriangleMesh* createCollisionTriangleMesh()
	{
		WRITE_ZONE();
		return impl.createCollisionTriangleMesh();
	}

	virtual ClothingRenderProxy* acquireRenderProxy()
	{
		READ_ZONE();
		return impl.acquireRenderProxy();
	}

#if APEX_UE4
	virtual void simulate(PxF32 dt)
	{
		WRITE_ZONE();
		impl.simulate(dt);
	}
#endif
};


}
} // namespace nvidia

#endif // CLOTHING_ACTOR_PROXY_H
