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



#ifndef CLOTHING_ASSET_AUTHORING_IMPL_H
#define CLOTHING_ASSET_AUTHORING_IMPL_H


#include "ClothingAssetAuthoring.h"
#include "ClothingAssetImpl.h"
#include "ClothingGraphicalLodParameters.h"
#include "ApexAssetAuthoring.h"

#include "ReadCheck.h"
#include "WriteCheck.h"

#ifndef WITHOUT_APEX_AUTHORING

namespace nvidia
{
namespace clothing
{

class ClothingPhysicalMeshImpl;


class ClothingAssetAuthoringImpl : public ClothingAssetAuthoring, public ApexAssetAuthoring, public ClothingAssetImpl
{
public:
	APEX_RW_LOCKABLE_BOILERPLATE

	ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list, const char* name);
	ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list);
	ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name);

	// from AssetAuthoring
	virtual const char* getName(void) const
	{
		return ClothingAssetImpl::getName();
	}
	virtual const char* getObjTypeName() const
	{
		return CLOTHING_AUTHORING_TYPE_NAME;
	}
	virtual bool							prepareForPlatform(nvidia::apex::PlatformTag);

	virtual void setToolString(const char* toolName, const char* toolVersion, uint32_t toolChangelist)
	{
		ApexAssetAuthoring::setToolString(toolName, toolVersion, toolChangelist);
	}

	// from ApexInterface
	virtual void							release();

	// from ClothingAssetAuthoring
	virtual void							setDefaultConstrainCoefficients(const ClothingConstrainCoefficients& coeff)
	{
		WRITE_ZONE();
		mDefaultConstrainCoefficients = coeff;
	}
	virtual void							setInvalidConstrainCoefficients(const ClothingConstrainCoefficients& coeff)
	{
		WRITE_ZONE();
		mInvalidConstrainCoefficients = coeff;
	}

	virtual void							setMeshes(uint32_t lod, RenderMeshAssetAuthoring* asset, ClothingPhysicalMesh* mesh,
													float normalResemblance = 90, bool ignoreUnusedVertices = true, 
													IProgressListener* progress = NULL);
	virtual bool							addPlatformToGraphicalLod(uint32_t lod, PlatformTag platform);
	virtual bool							removePlatform(uint32_t lod,  PlatformTag platform);
	virtual uint32_t						getNumPlatforms(uint32_t lod) const;
	virtual PlatformTag						getPlatform(uint32_t lod, uint32_t i) const;
	virtual uint32_t						getNumLods() const;
	virtual int32_t							getLodValue(uint32_t lod) const;
	virtual void							clearMeshes();
	virtual ClothingPhysicalMesh*			getClothingPhysicalMesh(uint32_t graphicalLod) const;

	virtual void							setBoneInfo(uint32_t boneIndex, const char* boneName, const PxMat44& bindPose, int32_t parentIndex);
	virtual void							setRootBone(const char* boneName);
	virtual uint32_t						addBoneConvex(const char* boneName, const PxVec3* positions, uint32_t numPositions);
	virtual uint32_t						addBoneConvex(uint32_t boneIndex, const PxVec3* positions, uint32_t numPositions);
	virtual void							addBoneCapsule(const char* boneName, float capsuleRadius, float capsuleHeight, const PxMat44& localPose);
	virtual void							addBoneCapsule(uint32_t boneIndex, float capsuleRadius, float capsuleHeight, const PxMat44& localPose);
	virtual void							clearBoneActors(const char* boneName);
	virtual void							clearBoneActors(uint32_t boneIndex);
	virtual void							clearAllBoneActors();

	virtual void							setCollision(const char** boneNames, float* radii, PxVec3* localPositions, 
														uint32_t numSpheres, uint16_t* pairs, uint32_t numPairs);
	virtual void							setCollision(uint32_t* boneIndices, float* radii, PxVec3* localPositions, uint32_t numSpheres, 
														uint16_t* pairs, uint32_t numPairs);
	virtual void							clearCollision();

	virtual void							setSimulationHierarchicalLevels(uint32_t levels)
	{
		WRITE_ZONE();
		mParams->simulation.hierarchicalLevels = levels;
		clearCooked();
	}
	virtual void							setSimulationThickness(float thickness)
	{
		WRITE_ZONE();
		mParams->simulation.thickness = thickness;
	}
	virtual void							setSimulationVirtualParticleDensity(float density)
	{
		WRITE_ZONE();
		PX_ASSERT(density >= 0.0f);
		PX_ASSERT(density <= 1.0f);
		mParams->simulation.virtualParticleDensity = PxClamp(density, 0.0f, 1.0f);
	}
	virtual void							setSimulationSleepLinearVelocity(float sleep)
	{
		WRITE_ZONE();
		mParams->simulation.sleepLinearVelocity = sleep;
	}
	virtual void							setSimulationGravityDirection(const PxVec3& gravity)
	{
		WRITE_ZONE();
		mParams->simulation.gravityDirection = gravity.getNormalized();
	}

	virtual void							setSimulationDisableCCD(bool disable)
	{
		WRITE_ZONE();
		mParams->simulation.disableCCD = disable;
	}
	virtual void							setSimulationTwowayInteraction(bool enable)
	{
		WRITE_ZONE();
		mParams->simulation.twowayInteraction = enable;
	}
	virtual void							setSimulationUntangling(bool enable)
	{
		WRITE_ZONE();
		mParams->simulation.untangling = enable;
	}
	virtual void							setSimulationRestLengthScale(float scale)
	{
		WRITE_ZONE();
		mParams->simulation.restLengthScale = scale;
	}

	virtual void							setExportScale(float scale)
	{
		WRITE_ZONE();
		mExportScale = scale;
	}
	virtual void							applyTransformation(const PxMat44& transformation, float scale, bool applyToGraphics, bool applyToPhysics);
	virtual void							updateBindPoses(const PxMat44* newBindPoses, uint32_t newBindPosesCount, bool isInternalOrder, bool collisionMaintainWorldPose);
	virtual void							setDeriveNormalsFromBones(bool enable)
	{
		WRITE_ZONE();
		mDeriveNormalsFromBones = enable;
	}
	virtual NvParameterized::Interface*		getMaterialLibrary();
	virtual bool							setMaterialLibrary(NvParameterized::Interface* materialLibrary, uint32_t materialIndex, bool transferOwnership);
	virtual NvParameterized::Interface*		getRenderMeshAssetAuthoring(uint32_t lodLevel) const;

	// parameterization
	NvParameterized::Interface*				getNvParameterized() const
	{
		return mParams;
	}
	virtual NvParameterized::Interface*		releaseAndReturnNvParameterizedInterface();

	// from NvParameterized::SerializationCallback
	virtual void							preSerialize(void* userData);

	// from ApexAssetAuthoring
	virtual void							setToolString(const char* toolString);

	// internal
	void									destroy();

	virtual bool							setBoneBindPose(uint32_t boneIndex, const PxMat44& bindPose);
	virtual bool							getBoneBindPose(uint32_t boneIndex, PxMat44& bindPose) const;

private:
	// bones
	uint32_t								addBoneConvexInternal(uint32_t boneIndex, const PxVec3* positions, uint32_t numPositions);
	void									addBoneCapsuleInternal(uint32_t boneIndex, float capsuleRadius, float capsuleHeight, const PxMat44& localPose);
	void									clearBoneActorsInternal(int32_t internalBoneIndex);
	void									compressBones() const;
	void									compressBoneCollision();
	void									collectBoneIndices(uint32_t numVertices, const uint16_t* boneIndices, const float* boneWeights, uint32_t numBonesPerVertex) const;

	void									updateMappingAuthoring(ClothingGraphicalLodParameters& graphLod, RenderMeshAssetIntl* renderMeshAssetCopy,
																	RenderMeshAssetAuthoringIntl* renderMeshAssetOrig, float normalResemblance, 
																	bool ignoreUnusedVertices, IProgressListener* progress);
	void									sortSkinMapB(SkinClothMapB* skinClothMap, uint32_t skinClothMapSize, uint32_t* immediateClothMap, uint32_t immediateClothMapSize);

	void									setupPhysicalMesh(ClothingPhysicalMeshParameters& physicalMeshParameters) const;

	bool									checkSetMeshesInput(uint32_t lod, ClothingPhysicalMesh* nxPhysicalMesh, uint32_t& graphicalLodIndexTest);
	void									sortPhysicalMeshes();

	// mesh reordering
	void									sortDeformableIndices(ClothingPhysicalMeshImpl& physicalMesh);


	bool									getGraphicalLodIndex(uint32_t lod, uint32_t& graphicalLodIndex) const;
	uint32_t								addGraphicalLod(uint32_t lod);

	// cooking
	void									clearCooked();

	// access
	bool									addGraphicalMesh(RenderMeshAssetAuthoring* renderMesh, uint32_t graphicalLodIndex);

	Array<ClothingPhysicalMeshImpl*>		mPhysicalMeshesInput;

	float									mExportScale;
	bool									mDeriveNormalsFromBones;
	bool									mOwnsMaterialLibrary;

	ClothingConstrainCoefficients			mDefaultConstrainCoefficients;
	ClothingConstrainCoefficients			mInvalidConstrainCoefficients;

	const char*								mPreviousCookedType;

	ApexSimpleString						mRootBoneName;

	void									initParams();

	// immediate cloth: 1-to-1 mapping from physical to rendering mesh (except for LOD)
	bool									generateImmediateClothMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
															ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, 
															uint32_t* masterFlags, float epsilon, uint32_t& numNotFoundVertices, 
															float normalResemblance, ParamArray<uint32_t>& result, IProgressListener* progress) const;
	bool									generateSkinClothMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
															ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, uint32_t* masterFlags, 
															uint32_t* immediateMap, uint32_t numEmptyInImmediateMap, ParamArray<SkinClothMap>& result,
															float& offsetAlongNormal, bool integrateImmediateMap, IProgressListener* progress) const;

	void									removeMaxDistance0Mapping(ClothingGraphicalLodParameters& graphicalLod, RenderMeshAssetIntl* renderMeshAsset) const;

	bool									generateTetraMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
															ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, uint32_t* masterFlags,
															ParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type>& result, IProgressListener* progress) const;
	float									computeBaryError(float baryX, float baryY) const;
	float									computeTriangleError(const TriangleWithNormals& triangle, const PxVec3& normal) const;


	bool									hasTangents(const RenderMeshAssetIntl& rma);
	uint32_t								getMaxNumGraphicalVertsActive(const ClothingGraphicalLodParameters& graphicalLod, uint32_t submeshIndex);
	bool									isMostlyImmediateSkinned(const RenderMeshAssetIntl& rma, const ClothingGraphicalLodParameters& graphicalLod);
	bool									conditionalMergeMapping(const RenderMeshAssetIntl& rma, ClothingGraphicalLodParameters& graphicalLod);

};

}
} // namespace nvidia

#endif // WITHOUT_APEX_AUTHORING

#endif // CLOTHING_ASSET_AUTHORING_IMPL_H
