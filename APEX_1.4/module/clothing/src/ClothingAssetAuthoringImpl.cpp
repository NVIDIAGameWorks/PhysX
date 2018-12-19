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



#include "ApexDefs.h"
#include "ApexUsingNamespace.h"

#ifndef WITHOUT_APEX_AUTHORING

#include "ClothingAssetAuthoringImpl.h"
#include "ApexMeshHash.h"
#include "PsSort.h"
#include "ApexPermute.h"
#include "CookingPhysX.h"
#include "ClothingGlobals.h"
#include "ClothingPhysicalMeshImpl.h"

#define MAX_DISTANCE_NAME "MAX_DISTANCE"
#define COLLISION_SPHERE_DISTANCE_NAME "COLLISION_SPHERE_DISTANCE"
#define COLLISION_SPHERE_RADIUS_NAME "COLLISION_SPHERE_RADIUS"
#define USED_FOR_PHYSICS_NAME "USED_FOR_PHYSICS"

#define LATCH_TO_NEAREST_SLAVE_NAME "LATCH_TO_NEAREST_SLAVE"
#define LATCH_TO_NEAREST_MASTER_NAME "LATCH_TO_NEAREST_MASTER"

#include "AbstractMeshDescription.h"
#include "RenderMesh.h"

#include "ApexSDKIntl.h"
#include "AuthorableObjectIntl.h"

#include "PsMathUtils.h"


namespace nvidia
{
namespace clothing
{

struct uint32_t_3
{
	uint32_t indices[3];
};



class TriangleGreater_3
{
public:
	TriangleGreater_3() {}

	TriangleGreater_3(uint32_t* deformableIndices, ClothingConstrainCoefficients* constrainCoeffs) :
		mDeformableIndices(deformableIndices),
		mConstrainCoeffs(constrainCoeffs)
	{}

	inline bool operator()(uint32_t_3 a, uint32_t_3 b) const
	{
		float maxDistA = mConstrainCoeffs[mDeformableIndices[a.indices[0]]].maxDistance;
		float maxDistB = mConstrainCoeffs[mDeformableIndices[b.indices[0]]].maxDistance;
		bool aHasEqualMaxDistances = (maxDistA == mConstrainCoeffs[mDeformableIndices[a.indices[1]]].maxDistance);
		bool bHasEqualMaxDistances = (maxDistB == mConstrainCoeffs[mDeformableIndices[b.indices[1]]].maxDistance);
		for (uint32_t i = 1; i < 3; i++)
		{
			if (aHasEqualMaxDistances)
			{
				aHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[a.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			}
			if (bHasEqualMaxDistances)
			{
				bHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[b.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
			}
			maxDistA = PxMax(maxDistA, mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			maxDistB = PxMax(maxDistB, mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
		}

		if (maxDistA == maxDistB)
		{
			return aHasEqualMaxDistances && !bHasEqualMaxDistances;
		}

		return maxDistA > maxDistB;
	}

private:
	uint32_t* mDeformableIndices;
	ClothingConstrainCoefficients* mConstrainCoeffs;
};



struct uint32_t_4
{
	uint32_t indices[4];
};



class TriangleGreater_4
{
public:
	TriangleGreater_4() {}

	TriangleGreater_4(uint32_t* deformableIndices, ClothingConstrainCoefficients* constrainCoeffs) :
		mDeformableIndices(deformableIndices),
		mConstrainCoeffs(constrainCoeffs)
	{}

	inline bool operator()(uint32_t_4 a, uint32_t_4 b) const
	{
		float maxDistA = mConstrainCoeffs[mDeformableIndices[a.indices[0]]].maxDistance;
		float maxDistB = mConstrainCoeffs[mDeformableIndices[b.indices[0]]].maxDistance;
		bool aHasEqualMaxDistances = (maxDistA == mConstrainCoeffs[mDeformableIndices[a.indices[1]]].maxDistance);
		bool bHasEqualMaxDistances = (maxDistB == mConstrainCoeffs[mDeformableIndices[b.indices[1]]].maxDistance);
		for (uint32_t i = 1; i < 4; i++)
		{
			if (aHasEqualMaxDistances)
			{
				aHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[a.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			}
			if (bHasEqualMaxDistances)
			{
				bHasEqualMaxDistances = (mConstrainCoeffs[mDeformableIndices[b.indices[i - 1]]].maxDistance == mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
			}
			maxDistA = PxMax(maxDistA, mConstrainCoeffs[mDeformableIndices[a.indices[i]]].maxDistance);
			maxDistB = PxMax(maxDistB, mConstrainCoeffs[mDeformableIndices[b.indices[i]]].maxDistance);
		}

		if (maxDistA == maxDistB)
		{
			return aHasEqualMaxDistances && !bHasEqualMaxDistances;
		}

		return maxDistA > maxDistB;
	}

private:
	uint32_t* mDeformableIndices;
	ClothingConstrainCoefficients* mConstrainCoeffs;
};



class BoneEntryPredicate
{
public:
	bool operator()(const ClothingAssetParametersNS::BoneEntry_Type& a, const ClothingAssetParametersNS::BoneEntry_Type& b) const
	{
		// mesh referenced bones first
		if (a.numMeshReferenced == 0 && b.numMeshReferenced > 0)
		{
			return false;
		}
		if (a.numMeshReferenced > 0 && b.numMeshReferenced == 0)
		{
			return true;
		}

		if (a.numMeshReferenced == 0) // both are 0 as they have to be equal here
		{
			PX_ASSERT(b.numMeshReferenced == 0);

			// RB referenced bones next, this will leave non referenced bones at the end
			if (a.numRigidBodiesReferenced != b.numRigidBodiesReferenced)
			{
				return a.numRigidBodiesReferenced > b.numRigidBodiesReferenced;
			}
			else
			{
				return a.externalIndex < b.externalIndex;
			}
		}

		return a.externalIndex < b.externalIndex;
	}
};



class ActorEntryPredicate
{
public:
	bool operator()(const ClothingAssetParametersNS::ActorEntry_Type& a, const ClothingAssetParametersNS::ActorEntry_Type& b) const
	{
		if (a.boneIndex < b.boneIndex)
		{
			return true;
		}
		else if (a.boneIndex > b.boneIndex)
		{
			return false;
		}
		return a.convexVerticesCount < b.convexVerticesCount;
	}
};



static bool getClosestVertex(RenderMeshAssetAuthoringIntl* renderMeshAsset, const PxVec3& position, uint32_t& resultSubmeshIndex,
                             uint32_t& resultGraphicalVertexIndex, const char* bufferName, bool ignoreUnused)
{
	resultSubmeshIndex = 0;
	resultGraphicalVertexIndex = 0;

	bool found = false;

	if (renderMeshAsset != NULL)
	{
		float closestDistanceSquared = FLT_MAX;

		for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); submeshIndex++)
		{
			RenderDataFormat::Enum outFormat = RenderDataFormat::UNSPECIFIED;
			const VertexBuffer& vb = renderMeshAsset->getSubmesh(submeshIndex).getVertexBuffer();
			const VertexFormat& vf = vb.getFormat();
			if (bufferName != NULL)
			{
				VertexFormat::BufferID id = ::strcmp(bufferName, "NORMAL") == 0 ? vf.getSemanticID(RenderVertexSemantic::NORMAL) : vf.getID(bufferName);
				outFormat = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(id));
				if (outFormat == RenderDataFormat::UNSPECIFIED)
				{
					continue;
				}
			}

			const uint8_t* usedForPhysics = NULL;
			if (ignoreUnused)
			{
				uint32_t usedForPhysicsIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(USED_FOR_PHYSICS_NAME));
				outFormat = vf.getBufferFormat(usedForPhysicsIndex);
				if (outFormat == RenderDataFormat::UBYTE1)
				{
					usedForPhysics = (const uint8_t*)vb.getBuffer(usedForPhysicsIndex);
				}
			}

			const uint32_t* slave = NULL;
			if (ignoreUnused)
			{
				uint32_t slaveIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_SLAVE_NAME));
				outFormat = vf.getBufferFormat(slaveIndex);
				if (outFormat == RenderDataFormat::UINT1)
				{
					slave = (const uint32_t*)vb.getBuffer(slaveIndex);
				}
			}

			const uint32_t vertexCount = renderMeshAsset->getSubmesh(submeshIndex).getVertexCount(0); // only 1 part supported
			RenderDataFormat::Enum format;
			uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION));
			const PxVec3* positions = (const PxVec3*)vb.getBufferAndFormat(format, bufferIndex);
			if (format != RenderDataFormat::FLOAT3)
			{
				PX_ALWAYS_ASSERT();
				positions = NULL;
			}
			for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
			{
				if (usedForPhysics != NULL && usedForPhysics[vertexIndex] == 0)
				{
					continue;
				}

				if (slave != NULL && slave[vertexIndex] != 0)
				{
					continue;
				}

				const float distSquared = (position - positions[vertexIndex]).magnitudeSquared();
				if (distSquared < closestDistanceSquared)
				{
					closestDistanceSquared = distSquared;
					resultSubmeshIndex = submeshIndex;
					resultGraphicalVertexIndex = vertexIndex;
					found = true;
				}
			}
		}
	}

	return found;
}




ClothingAssetAuthoringImpl::ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list) :
	ClothingAssetImpl(module, list, "ClothingAuthoring"),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType("Embedded")
{
	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();
}

ClothingAssetAuthoringImpl::ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list, const char* name) :
	ClothingAssetImpl(module, list, name),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType("Embedded")
{
	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();
}

ClothingAssetAuthoringImpl::ClothingAssetAuthoringImpl(ModuleClothingImpl* module, ResourceList& list, NvParameterized::Interface* params, const char* name) :
	ClothingAssetImpl(module, list, params, name),
	mExportScale(1.0f),
	mDeriveNormalsFromBones(false),
	mOwnsMaterialLibrary(true),
	mPreviousCookedType("Embedded")
{
	mDefaultConstrainCoefficients.maxDistance = 0.0f;
	mDefaultConstrainCoefficients.collisionSphereDistance = 0.0f;
	mDefaultConstrainCoefficients.collisionSphereRadius = 0.0f;

	mInvalidConstrainCoefficients.maxDistance = -1.0f;
	mInvalidConstrainCoefficients.collisionSphereDistance = -FLT_MAX;
	mInvalidConstrainCoefficients.collisionSphereRadius = -1.0f;

	initParams();

	if (mParams->rootBoneIndex < (uint32_t)mParams->bones.arraySizes[0])
	{
		mRootBoneName = mParams->bones.buf[mParams->rootBoneIndex].name;
	}
}



void ClothingAssetAuthoringImpl::release()
{
	mModule->mSdk->releaseAssetAuthoring(*this);
}



bool ClothingAssetAuthoringImpl::checkSetMeshesInput(uint32_t lod, ClothingPhysicalMesh* nxPhysicalMesh, uint32_t& graphicalLodIndex)
{
	// index where it will be inserted
	for (graphicalLodIndex = 0; graphicalLodIndex < mGraphicalLods.size(); ++graphicalLodIndex)
	{
		if (mGraphicalLods[graphicalLodIndex]->lod >= lod)
		{
			break;
		}
	}


	if (nxPhysicalMesh != NULL)
	{
		if (mPhysicalMeshesInput.size() != mPhysicalMeshes.size())
		{
			APEX_INVALID_PARAMETER("Trying to operate add a physical mesh to an authoring object that has been deserialized. This is not suppored.");
			return false;
		}

		// check that shared physics meshes are only in subsequent lods
		int32_t i = (int32_t)graphicalLodIndex - 1;
		uint32_t physMeshId = (uint32_t) - 1;
		while (i >= 0)
		{
			physMeshId = mGraphicalLods[(uint32_t)i]->physicalMeshId;
			if (physMeshId != (uint32_t) - 1 && mPhysicalMeshesInput[physMeshId] != nxPhysicalMesh)
			{
				break;
			}
			--i;
		}

		while (i >= 0)
		{
			physMeshId = mGraphicalLods[(uint32_t)i]->physicalMeshId;
			if (physMeshId != (uint32_t) - 1 && mPhysicalMeshesInput[physMeshId] == nxPhysicalMesh)
			{
				APEX_INVALID_PARAMETER("Only subsequent graphical lods can share a physical mesh.");
				return false;
			}
			--i;
		}

		i = (int32_t)graphicalLodIndex + 1;
		physMeshId = (uint32_t) - 1;
		while (i < (int32_t)mGraphicalLods.size())
		{
			physMeshId = mGraphicalLods[(uint32_t)i]->physicalMeshId;
			if (physMeshId != (uint32_t) - 1 && mPhysicalMeshesInput[physMeshId] != nxPhysicalMesh)
			{
				break;
			}
			++i;
		}

		while (i < (int32_t)mGraphicalLods.size())
		{
			physMeshId = mGraphicalLods[(uint32_t)i]->physicalMeshId;
			if (physMeshId != (uint32_t) - 1 && mPhysicalMeshesInput[physMeshId] == nxPhysicalMesh)
			{
				APEX_INVALID_PARAMETER("Only subsequent graphical lods can share a physical mesh.");
				return false;
			}
			++i;
		}
	}
	return true;
}



void ClothingAssetAuthoringImpl::sortPhysicalMeshes()
{
	if (mPhysicalMeshes.size() == 0)
	{
		return;
	}

	// sort physical lods according to references in graphical lods
	Array<uint32_t> new2old(mPhysicalMeshes.size(), (uint32_t) - 1);
	Array<uint32_t> old2new(mPhysicalMeshes.size(), (uint32_t) - 1);
	bool reorderFailed = false;

	uint32_t nextId = 0;
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		uint32_t physMeshId = mGraphicalLods[i]->physicalMeshId;
		if (physMeshId == (uint32_t) - 1)
		{
			continue;
		}

		if (nextId == 0 || new2old[nextId - 1] != physMeshId) // if there's  a new ID
		{
			// the new ID already appeared before, we can't sort
			if (old2new[physMeshId] != (uint32_t) - 1)
			{
				PX_ALWAYS_ASSERT();
				APEX_INTERNAL_ERROR("The assignment of graphics and physics mesh in the asset does not allow ordering of the physical meshes. Reuse of physical mesh is only allowed on subsequend graphical lods.");
				reorderFailed = true;
				break;
			}

			new2old[nextId] = physMeshId;
			old2new[physMeshId] = nextId;
			++nextId;
		}
	}

	if (!reorderFailed)
	{
		// reorder
		ApexPermute<ClothingPhysicalMeshParameters*>(&mPhysicalMeshes[0], &new2old[0], mPhysicalMeshes.size());

		// update references
		for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
		{
			mGraphicalLods[i]->physicalMeshId = old2new[mGraphicalLods[i]->physicalMeshId];
		}

		// clear transition maps
		for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
		{
			ParamArray<SkinClothMapB> transitionDownB(mPhysicalMeshes[i], "transitionDownB", reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDownB));
			transitionDownB.clear();
			ParamArray<SkinClothMapB> transitionUpB(mPhysicalMeshes[i], "transitionUpB", reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUpB));
			transitionUpB.clear();

			ParamArray<SkinClothMap> transitionDown(mPhysicalMeshes[i], "transitionDown", reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDown));
			transitionDown.clear();
			ParamArray<SkinClothMap> transitionUp(mPhysicalMeshes[i], "transitionUp", reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUp));
			transitionUp.clear();
		}
	}
}


void ClothingAssetAuthoringImpl::setMeshes(uint32_t lod, RenderMeshAssetAuthoring* renderMeshAssetDontReference,
											ClothingPhysicalMesh* nxPhysicalMesh,
											float normalResemblance, bool ignoreUnusedVertices,
											IProgressListener* progress)
{
	WRITE_ZONE();
	// check input
	uint32_t graphicalLodIndexTest = (uint32_t) - 1;
	if (!checkSetMeshesInput(lod, nxPhysicalMesh, graphicalLodIndexTest))
	{
		return;
	}

	// get index and add lod if necessary, only adds if lod doesn't exist already
	const uint32_t graphicalLodIndex = addGraphicalLod(lod);
	PX_ASSERT(graphicalLodIndex == graphicalLodIndexTest);
	PX_ASSERT(lod == mGraphicalLods[graphicalLodIndex]->lod);

	clearMapping(graphicalLodIndex);

	// reset counters to 0
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		mBones[i].numMeshReferenced = mBones[i].numRigidBodiesReferenced = 0;
	}

	// remove existing physical of this lod mesh if it is not used by other lod
	const uint32_t oldPhysicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;
	if (oldPhysicalMeshId != (uint32_t) - 1)
	{
		// check if it's referenced by someone else
		bool removePhysicalMesh = true;
		for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
		{
			// don't consider the current graphical lod
			if (mGraphicalLods[i]->lod == lod)
			{
				continue;
			}

			if (mGraphicalLods[i]->physicalMeshId == oldPhysicalMeshId)
			{
				removePhysicalMesh = false;
				break;
			}
		}

		// if it's not referenced, remove it
		if (removePhysicalMesh)
		{
			if (mPhysicalMeshesInput.size() == mPhysicalMeshes.size()) // mPhysicalMeshesInput is not set if the authoring is created from an existing params object
			{
				mPhysicalMeshesInput.replaceWithLast(oldPhysicalMeshId);
			}

			// replace with last and update the references to the last
			mPhysicalMeshes.replaceWithLast(oldPhysicalMeshId);
			for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
			{
				if (mGraphicalLods[i]->physicalMeshId == mPhysicalMeshes.size())
				{
					mGraphicalLods[i]->physicalMeshId = oldPhysicalMeshId;
				}
			}
		}
	}

	// copy physical mesh if we don't already have it
	bool newPhysicalMesh = false;
	ClothingPhysicalMeshParameters* physicalMesh = NULL;

	if (nxPhysicalMesh != NULL)
	{
		ClothingPhysicalMeshImpl* physicalMeshInput = DYNAMIC_CAST(ClothingPhysicalMeshImpl*)(nxPhysicalMesh);
		PX_ASSERT(physicalMeshInput != NULL);

		PX_ASSERT(mPhysicalMeshes.size() == mPhysicalMeshesInput.size());
		for (uint32_t i = 0; i < mPhysicalMeshesInput.size(); i++)
		{
			if (physicalMeshInput == mPhysicalMeshesInput[i]) // TODO check some more stuff in case it has been released and a new one was created at the same address
			{
				physicalMesh = mPhysicalMeshes[i];
				PX_ASSERT(physicalMesh != NULL);
				mGraphicalLods[graphicalLodIndex]->physicalMeshId = i;
				break;
			}
		}
		if (physicalMesh == NULL)
		{
			physicalMesh = DYNAMIC_CAST(ClothingPhysicalMeshParameters*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingPhysicalMeshParameters::staticClassName()));
			physicalMeshInput->makeCopy(physicalMesh);
			physicalMesh->referenceCount = 1;
			mPhysicalMeshes.pushBack(physicalMesh);
			mPhysicalMeshesInput.pushBack(physicalMeshInput);

			ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(physicalMesh);
			mesh->updateSkinningNormals();
			mesh->release();

			newPhysicalMesh = true;
			PX_ASSERT(physicalMesh != NULL);
			mGraphicalLods[graphicalLodIndex]->physicalMeshId = mPhysicalMeshes.size() - 1;
		}
	}

	bool hasLod = addGraphicalMesh(renderMeshAssetDontReference, graphicalLodIndex);
	if (hasLod && physicalMesh)
	{
		PX_ASSERT(mGraphicalLods[graphicalLodIndex] != NULL);
		RenderMeshAssetIntl* renderMeshAssetCopy = reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer);

		// update mapping
		updateMappingAuthoring(*mGraphicalLods[graphicalLodIndex], renderMeshAssetCopy, static_cast<RenderMeshAssetAuthoringIntl*>(renderMeshAssetDontReference),
								normalResemblance, ignoreUnusedVertices, progress);

		// sort physics mesh triangles and vertices
		ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(physicalMesh);
		sortDeformableIndices(*mesh);

		// calculate and setup number of simulated vertices and indices
		setupPhysicalMesh(*physicalMesh);

		// "reordering has to be done after creating the submeshes because the vertices must be sorted per submesh" - not valid anymore
		reorderDeformableVertices(*mesh);

		// re-order vertices in graphical mesh to make tangent recompute faster
		reorderGraphicsVertices(graphicalLodIndex, false);
		removeMaxDistance0Mapping(*mGraphicalLods[graphicalLodIndex], renderMeshAssetCopy);

		mesh->release();
		mesh = NULL;

		// conditionally drop the immediate map (perf optimization)
		conditionalMergeMapping(*renderMeshAssetCopy, *mGraphicalLods[graphicalLodIndex]);
	}

	// keep physical meshes sorted such that the transition maps are correct
	// (needs to be called after 'addGraphicalMesh', so a graphicalLOD deletion is not missed)
	sortPhysicalMeshes();

	bool isIdentity = true;
	Array<int32_t> old2new(mBones.size(), -1);
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		old2new[(uint32_t)mBones[i].externalIndex] = mBones[i].internalIndex;
		isIdentity &= mBones[i].externalIndex == mBones[i].internalIndex;
	}

	if (!isIdentity && hasLod)
	{
		reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer)->permuteBoneIndices(old2new);

		if (newPhysicalMesh)
		{
			const uint32_t physicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;
			ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);
			mesh->permuteBoneIndices(old2new);
			mesh->release();
		}
	}
}



bool ClothingAssetAuthoringImpl::addPlatformToGraphicalLod(uint32_t lod, PlatformTag platform)
{
	WRITE_ZONE();
	uint32_t index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return false;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	// pushback to an array of strings
	NvParameterized::Handle handle(*graphicalLod);
	if (graphicalLod->getParameterHandle("platforms", handle) != NvParameterized::ERROR_NONE)
	{
		return false;
	}

	int32_t numPlatforms = 0;
	graphicalLod->getArraySize(handle, numPlatforms);
	graphicalLod->resizeArray(handle, numPlatforms + 1);
	NvParameterized::Handle elementHandle(*graphicalLod);
	handle.getChildHandle(numPlatforms, elementHandle);
	graphicalLod->setParamString(elementHandle, platform);

	return true;
}


bool ClothingAssetAuthoringImpl::removePlatform(uint32_t lod,  PlatformTag platform)
{
	WRITE_ZONE();
	uint32_t index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return false;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	ParamArray<NvParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->platforms));

	bool removed = false;
	for (int32_t i = (int32_t)platforms.size() - 1; i >= 0 ; --i)
	{
		if (::strcmp(platforms[(uint32_t)i], platform) == 0)
		{
			platforms.replaceWithLast((uint32_t)i);
			removed = true;
		}
	}

	return removed;
}


uint32_t ClothingAssetAuthoringImpl::getNumPlatforms(uint32_t lod) const
{
	uint32_t index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return 0;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	ParamArray<NvParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->platforms));
	return platforms.size();
}


PlatformTag ClothingAssetAuthoringImpl::getPlatform(uint32_t lod, uint32_t i) const
{
	READ_ZONE();
	uint32_t index;
	if (!getGraphicalLodIndex(lod, index))
	{
		return 0;
	}

	ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[index];

	ParamArray<NvParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->platforms));

	if (i >= platforms.size())
	{
		return 0;
	}

	return platforms[i];
}


bool ClothingAssetAuthoringImpl::prepareForPlatform(PlatformTag platform)
{
	bool retVal = false;

	// go through graphical lods and remove the ones that are not tagged with "platform"
	for (int32_t i = (int32_t)mGraphicalLods.size() - 1; i >= 0; --i)
	{
		ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[(uint32_t)i];
		ParamArray<NvParameterized::DummyStringStruct> platforms(graphicalLod, "platforms", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod->platforms));

		bool keep = platforms.size() == 0; // keep it if it has no platforms at all
		for (uint32_t j = 0; j < platforms.size(); j++)
		{
			const char* storedPlatform = platforms[j].buf;
			if (::strcmp(platform, storedPlatform) == 0)
			{
				keep = true;
			}
		}

		if (!keep)
		{
			setMeshes(graphicalLod->lod, NULL, NULL);    // remove
		}
		else
		{
			retVal = true;    // keep
		}
	}

	return retVal;
}



uint32_t ClothingAssetAuthoringImpl::getNumLods() const
{
	READ_ZONE();
	return mGraphicalLods.size();
}



int32_t ClothingAssetAuthoringImpl::getLodValue(uint32_t lod) const
{
	READ_ZONE();
	if (lod < mGraphicalLods.size())
	{
		return (int32_t)mGraphicalLods[lod]->lod;
	}

	return -1;
}



void ClothingAssetAuthoringImpl::clearMeshes()
{
	WRITE_ZONE();
	for (int32_t i = (int32_t)mGraphicalLods.size() - 1; i >= 0; i--)
	{
		setMeshes(mGraphicalLods[(uint32_t)i]->lod, NULL, NULL);
	}
	PX_ASSERT(mGraphicalLods.isEmpty());

	PX_ASSERT(mPhysicalMeshes.size() == mPhysicalMeshesInput.size());
	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		mPhysicalMeshes[i]->destroy();
	}
	mPhysicalMeshes.clear();
	mPhysicalMeshesInput.clear();
}



ClothingPhysicalMesh* ClothingAssetAuthoringImpl::getClothingPhysicalMesh(uint32_t graphicalLod) const
{
	READ_ZONE();
	uint32_t graphicalLodIndex = 0;
	if (!getGraphicalLodIndex(graphicalLod, graphicalLodIndex))
	{
		return NULL;
	}

	uint32_t physicalMeshId = mGraphicalLods[graphicalLodIndex]->physicalMeshId;

	if (physicalMeshId == (uint32_t) - 1)
	{
		return NULL;
	}

	return mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);
}



bool ClothingAssetAuthoringImpl::getBoneBindPose(uint32_t boneIndex, PxMat44& bindPose) const
{
	READ_ZONE();
	bool ret = false;
	if (boneIndex < mBones.size())
	{
		bindPose = mBones[boneIndex].bindPose;
		ret = true;
	}
	return ret;
}

bool ClothingAssetAuthoringImpl::setBoneBindPose(uint32_t boneIndex, const PxMat44& bindPose)
{
	WRITE_ZONE();
	bool ret = false;
	if (boneIndex < mBones.size())
	{
		mBones[boneIndex].bindPose = bindPose;
		ret = true;
	}
	return ret;
}

void ClothingAssetAuthoringImpl::setBoneInfo(uint32_t boneIndex, const char* boneName, const PxMat44& bindPose, int32_t parentIndex)
{
	WRITE_ZONE();
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].externalIndex == (int32_t)boneIndex)
		{
			if (mBones[i].name == NULL || ::strcmp(mBones[i].name, boneName) != 0)
			{
				setBoneName(i, boneName);
			}

			mBones[i].bindPose = bindPose;

			// parentIndex should be an internal index, so let's see
			int32_t oldInternalParent = mBones[i].parentIndex;
			int32_t newInternalParent = oldInternalParent;
			PX_ASSERT((oldInternalParent == -1) == (parentIndex == -1)); // both are -1 or valid
			if (oldInternalParent >= 0)
			{
				PX_ASSERT(parentIndex >= 0);
				PX_ASSERT((uint32_t)oldInternalParent < mBones.size());
				if ((uint32_t)oldInternalParent < mBones.size())
				{
					PX_ASSERT(mBones[(uint32_t)oldInternalParent].internalIndex == oldInternalParent); // just some sanity
					if (mBones[(uint32_t)oldInternalParent].externalIndex != parentIndex)
					{
						// it seems the parent changed, let's hope this doesn't kill us
						for (uint32_t b = 0; b < mBones.size(); b++)
						{
							if (mBones[b].externalIndex == parentIndex)
							{
								newInternalParent = (int32_t)b;
								break;
							}
						}
					}
				}
			}
			mBones[i].parentIndex = newInternalParent;
			return;
		}
	}

	ClothingAssetParametersNS::BoneEntry_Type& bm = mBones.pushBack();
	bm.internalIndex = (int32_t)boneIndex;
	bm.externalIndex = (int32_t)boneIndex;
	bm.numMeshReferenced = 0;
	bm.numRigidBodiesReferenced = 0;
	bm.parentIndex = parentIndex;
	bm.bindPose = bindPose;
	setBoneName(mBones.size() - 1, boneName);
}



void ClothingAssetAuthoringImpl::setRootBone(const char* boneName)
{
	WRITE_ZONE();
	mRootBoneName = boneName;
}



uint32_t ClothingAssetAuthoringImpl::addBoneConvex(const char* boneName, const PxVec3* positions, uint32_t numPositions)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex  == -1)
	{
		return 0;
	}

	return addBoneConvexInternal((uint32_t)internalBoneIndex , positions, numPositions);
}

uint32_t ClothingAssetAuthoringImpl::addBoneConvex(uint32_t boneIndex, const PxVec3* positions, uint32_t numPositions)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex  == -1)
	{
		return 0;
	}

	return addBoneConvexInternal((uint32_t)internalBoneIndex , positions, numPositions);

}



void ClothingAssetAuthoringImpl::addBoneCapsule(const char* boneName, float capsuleRadius, float capsuleHeight, const PxMat44& localPose)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex  == -1)
	{
		return;
	}

	addBoneCapsuleInternal((uint32_t)internalBoneIndex, capsuleRadius, capsuleHeight, localPose);
}



void ClothingAssetAuthoringImpl::addBoneCapsule(uint32_t boneIndex, float capsuleRadius, float capsuleHeight, const PxMat44& localPose)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex  == -1)
	{
		return;
	}

	addBoneCapsuleInternal((uint32_t)internalBoneIndex, capsuleRadius, capsuleHeight, localPose);
}



void ClothingAssetAuthoringImpl::clearBoneActors(const char* boneName)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneName);

	if (internalBoneIndex == -1)
	{
		return;
	}

	clearBoneActorsInternal(internalBoneIndex);
}



void ClothingAssetAuthoringImpl::clearBoneActors(uint32_t boneIndex)
{
	WRITE_ZONE();
	int32_t internalBoneIndex = getBoneInternalIndex(boneIndex);

	if (internalBoneIndex == -1)
	{
		return;
	}

	clearBoneActorsInternal(internalBoneIndex);
}



void ClothingAssetAuthoringImpl::clearAllBoneActors()
{
	WRITE_ZONE();
	mBoneActors.clear();
	mBoneVertices.clear();
	mBonePlanes.clear();
	clearCooked();
}



void ClothingAssetAuthoringImpl::setCollision(const char** boneNames, float* radii, PxVec3* localPositions, uint32_t numSpheres, uint16_t* pairs, uint32_t numPairs)
{
	WRITE_ZONE();
	nvidia::Array<uint32_t> boneIndices(numSpheres, 0);
	for (uint32_t i = 0; i < numSpheres; ++i)
	{
		int32_t internalBoneIndex = getBoneInternalIndex(boneNames[i]);
		if (internalBoneIndex < 0 || internalBoneIndex >= (int32_t)mBones.size())
		{
			APEX_INVALID_PARAMETER("Bone \'%s\' not found, setting to root", boneNames[i]);
			boneIndices[i] = 0;
		}
		else
		{
			boneIndices[i] = (uint32_t)mBones[i].externalIndex;
		}
	}

	setCollision(boneIndices.begin(), radii, localPositions, numSpheres, pairs, numPairs);
}



void ClothingAssetAuthoringImpl::setCollision(uint32_t* boneIndices, float* radii, PxVec3* localPositions, uint32_t numSpheres, uint16_t* pairs, uint32_t numPairs)
{
	WRITE_ZONE();
	if (numPairs & 0x1)
	{
		APEX_INVALID_PARAMETER("numPairs must be a multiple of 2");
		return;
	}

	mBoneSpheres.clear();
	for (uint32_t i = 0; i < numSpheres; ++i)
	{
		int32_t internalBoneIndex = getBoneInternalIndex(boneIndices[i]);

		PX_ASSERT(internalBoneIndex < (int32_t)mBones.size());
		internalBoneIndex = PxClamp(internalBoneIndex, 0, (int32_t)mBones.size() - 1);
		float radius = radii[i];
		if (radius <= 0.0f)
		{
			APEX_INVALID_PARAMETER("Sphere radius must be bigger than 0.0 (sphere %d has radius %f)", i, radius);
			radius = 0.0f;
		}
		ClothingAssetParametersNS::BoneSphere_Type& newEntry = mBoneSpheres.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::BoneSphere_Type));
		newEntry.boneIndex = internalBoneIndex;
		newEntry.radius = radius;
		newEntry.localPos = localPositions[i];
	}

	mSpherePairs.clear();
	for (uint32_t i = 0; i < numPairs; i += 2)
	{
		const uint16_t p1 = PxMin(pairs[i + 0], pairs[i + 1]);
		const uint16_t p2 = PxMax(pairs[i + 0], pairs[i + 1]);
		if (p1 == p2)
		{
			APEX_INVALID_PARAMETER("pairs[%d] and pairs[%d] are identical (%d), skipping", i, i + 1, p1);
			continue;
		}
		else if (p1 >= mBoneSpheres.size() || p2 >= mBoneSpheres.size())
		{
			APEX_INVALID_PARAMETER("pairs[%d] = %d and pairs[%d] = %d are overflowing bone spheres, skipping", i, pairs[i], i + 1, pairs[i + 1]);
		}
		else
		{
			bool skip = false;
			for (uint32_t j = 0; j < mSpherePairs.size(); j += 2)
			{
				if (mSpherePairs[j] == p1 && mSpherePairs[j + 1] == p2)
				{
					APEX_INVALID_PARAMETER("pairs[%d] = %d and pairs[%d] = %d are a duplicate, skipping", i, pairs[i], i + 1, pairs[i + 1]);
					skip = true;
					break;
				}
			}
			if (!skip)
			{
				mSpherePairs.pushBack(p1);
				mSpherePairs.pushBack(p2);
			}
		}
	}
}



void ClothingAssetAuthoringImpl::clearCollision()
{
	WRITE_ZONE();
	mBoneSpheres.clear();
	mSpherePairs.clear();
}



NvParameterized::Interface* ClothingAssetAuthoringImpl::getMaterialLibrary()
{
	READ_ZONE();
	PX_ASSERT(mParams->materialLibrary != NULL);
	return mParams->materialLibrary;
}



bool ClothingAssetAuthoringImpl::setMaterialLibrary(NvParameterized::Interface* materialLibrary, uint32_t materialIndex, bool transferOwnership)
{
	WRITE_ZONE();
	if (::strcmp(materialLibrary->className(), ClothingMaterialLibraryParameters::staticClassName()) == 0)
	{
		if (mParams->materialLibrary != NULL && mOwnsMaterialLibrary)
		{
			mParams->materialLibrary->destroy();
		}

		mParams->materialLibrary = materialLibrary;
		mParams->materialIndex = materialIndex;
		mOwnsMaterialLibrary = transferOwnership;

		return true;
	}

	return false;
}



NvParameterized::Interface* ClothingAssetAuthoringImpl::getRenderMeshAssetAuthoring(uint32_t lodLevel) const
{
	READ_ZONE();
	NvParameterized::Interface* ret = NULL;

	if (lodLevel < mGraphicalLods.size())
	{
		ret = mGraphicalLods[lodLevel]->renderMeshAsset;
	}

	return ret;
}



NvParameterized::Interface* ClothingAssetAuthoringImpl::releaseAndReturnNvParameterizedInterface()
{
	// this is important for destroy() !
	if (!mOwnsMaterialLibrary && mParams->materialLibrary != NULL)
	{
		NvParameterized::Interface* foreignMatLib = mParams->materialLibrary;

		// clone the mat lib
		mParams->materialLibrary = mParams->getTraits()->createNvParameterized(foreignMatLib->className());
		mParams->materialLibrary->copy(*foreignMatLib);
	}
	mOwnsMaterialLibrary = true;

	if (NvParameterized::ERROR_NONE != mParams->callPreSerializeCallback())
	{
		return NULL;
	}

	mParams->setSerializationCallback(NULL, NULL);

	// release the object without mParams
	NvParameterized::Interface* ret = mParams;
	mParams = NULL;

	release();
	return ret;
}



void ClothingAssetAuthoringImpl::preSerialize(void* userData)
{
	PX_ASSERT(userData == NULL);
	PX_UNUSED(userData);


	ParamArray<ClothingAssetParametersNS::CookedEntry_Type> cookedEntries(mParams, "cookedData", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->cookedData));
	if (cookedEntries.isEmpty())
	{
		ClothingAssetParametersNS::CookedEntry_Type entry;
		entry.cookedData = NULL;
		entry.scale = 1.0f;
		cookedEntries.pushBack(entry);
	}

	for (uint32_t i = 0; i < cookedEntries.size(); i++)
	{
		if (cookedEntries[i].cookedData != NULL)
		{
			mPreviousCookedType = cookedEntries[i].cookedData->className();
			cookedEntries[i].cookedData->destroy();
			cookedEntries[i].cookedData = NULL;
		}

		PX_ASSERT(mPreviousCookedType != NULL);
		BackendFactory* cookingFactory = mModule->getBackendFactory(mPreviousCookedType);
		PX_ASSERT(cookingFactory != NULL);
		if (cookingFactory != NULL)
		{
			CookingAbstract* cookingJob = cookingFactory->createCookingJob();
			PX_ASSERT(cookingJob != NULL);
			if (cookingJob)
			{
				prepareCookingJob(*cookingJob, cookedEntries[i].scale, NULL, NULL);

				if (cookingJob->isValid())
				{
					cookedEntries[i].cookedData = cookingJob->execute();
				}
				PX_DELETE_AND_RESET(cookingJob);
			}
		}
	}

	compressBones();

	for (uint32_t graphicalMeshId = 0; graphicalMeshId < mGraphicalLods.size(); graphicalMeshId++)
	{
		RenderMeshAssetIntl* renderMeshAsset = reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalMeshId]->renderMeshAssetPointer);
		for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAsset->getSubmeshCount(); submeshIndex++)
		{
			VertexFormat& format = renderMeshAsset->getInternalSubmesh(submeshIndex).getVertexBufferWritable().getFormatWritable();
			format.setBufferAccess((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::POSITION)), RenderDataAccess::DYNAMIC);
			format.setBufferAccess((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::NORMAL)), RenderDataAccess::DYNAMIC);
			if (format.getBufferFormat((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::TANGENT))) != RenderDataFormat::UNSPECIFIED)
			{
				format.setBufferAccess((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::TANGENT)), RenderDataAccess::DYNAMIC);
				format.setBufferAccess((uint32_t)format.getBufferIndexFromID(format.getSemanticID(RenderVertexSemantic::BINORMAL)), RenderDataAccess::DYNAMIC);
			}
			format.setHasSeparateBoneBuffer(true);
		}
	}

	// create lod transition maps
	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		AbstractMeshDescription other;
		other.pPosition		= mPhysicalMeshes[i]->physicalMesh.vertices.buf;
		other.pNormal		= mPhysicalMeshes[i]->physicalMesh.normals.buf;
		other.numVertices	= mPhysicalMeshes[i]->physicalMesh.numVertices;

		if (i > 0)
		{
			ParamArray<SkinClothMap> transitionDown(mPhysicalMeshes[i], "transitionDown",
					reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionDown));
			if (transitionDown.isEmpty())
			{
				generateSkinClothMap(&other, 1, mPhysicalMeshes[i - 1]->physicalMesh, NULL, NULL, 0, transitionDown,
									mPhysicalMeshes[i]->transitionDownOffset, false, NULL);

				mPhysicalMeshes[i]->transitionDownThickness = 1.0f;
			}
		}
		if (i + 1 < mPhysicalMeshes.size())
		{

			ParamArray<SkinClothMap> transitionUp(mPhysicalMeshes[i], "transitionUp",
			        reinterpret_cast<ParamDynamicArrayStruct*>(&mPhysicalMeshes[i]->transitionUp));
			if (transitionUp.isEmpty())
			{
				generateSkinClothMap(&other, 1, mPhysicalMeshes[i + 1]->physicalMesh, NULL, NULL, 0, transitionUp,
									mPhysicalMeshes[i]->transitionUpOffset, false, NULL);

				mPhysicalMeshes[i]->transitionUpThickness = 1.0f;
			}
		}
	}

	updateBoundingBox();

	ClothingAssetImpl::preSerialize(userData);
}



void ClothingAssetAuthoringImpl::setToolString(const char* toolString)
{
	if (mParams != NULL)
	{
		NvParameterized::Handle handle(*mParams, "toolString");
		PX_ASSERT(handle.isValid());
		if (handle.isValid())
		{
			PX_ASSERT(handle.parameterDefinition()->type() == NvParameterized::TYPE_STRING);
			handle.setParamString(toolString);
		}
	}
}



void ClothingAssetAuthoringImpl::applyTransformation(const PxMat44& transformation, float scale, bool applyToGraphics, bool applyToPhysics)
{
	WRITE_ZONE();
	if (applyToPhysics)
	{
		clearCooked();

		for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
		{
			ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[i]);
			mesh->applyTransformation(transformation, scale);
			mesh->release();
		}

		for (uint32_t i = 0; i < mBones.size(); i++)
		{
			mBones[i].bindPose = transformation * mBones[i].bindPose;
			mBones[i].bindPose.setPosition(mBones[i].bindPose.getPosition() * scale);
		}

		for (uint32_t i = 0; i < mBoneVertices.size(); i++)
		{
			// PH: Do not apply transformation, bindpose was already adapted!
			//mBoneVertices[i] = transformation * mBoneVertices[i];
			mBoneVertices[i] *=  scale;
		}

		for(uint32_t i = 0; i < mBonePlanes.size(); i++)
		{
			mBonePlanes[i].d *= scale;
		}

		for (uint32_t i = 0; i < mBoneActors.size(); i++)
		{
			mBoneActors[i].capsuleRadius *= scale;
			mBoneActors[i].capsuleHeight *= scale;
			mBoneActors[i].localPose.setPosition(mBoneActors[i].localPose.getPosition() * scale);
		}

		for (uint32_t i = 0; i < mBoneSpheres.size(); i++)
		{
			mBoneSpheres[i].radius *= scale;
			mBoneSpheres[i].localPos *= scale;
		}

		mParams->simulation.thickness *= scale;
		
		ClothingMaterialLibraryParameters* materialLib = static_cast<ClothingMaterialLibraryParameters*>(mParams->materialLibrary);
		for (int32_t i = 0; i < materialLib->materials.arraySizes[0]; ++i)
		{
			materialLib->materials.buf[i].selfcollisionThickness *= scale;
		}
	}

	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (applyToGraphics)
		{
			reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer)->applyTransformation(transformation, scale);
		}

		if (applyToPhysics)
		{
			const uint32_t numSkinClothMap = (uint32_t)mGraphicalLods[i]->skinClothMap.arraySizes[0];
			SkinClothMap* skinClothMap = mGraphicalLods[i]->skinClothMap.buf;

			mGraphicalLods[i]->skinClothMapThickness *= scale;

			for (uint32_t j = 0; j < numSkinClothMap; j++)
			{
				// make sure no INF is created
				if (skinClothMap[j].vertexBary.z != PX_MAX_F32) skinClothMap[j].vertexBary.z *= scale;
				if (skinClothMap[j].normalBary.z != PX_MAX_F32) skinClothMap[j].normalBary.z *= scale;
				if (skinClothMap[j].tangentBary.z != PX_MAX_F32) skinClothMap[j].tangentBary.z *= scale;
			}
			const PxMat33 t(transformation.column0.getXYZ(), transformation.column1.getXYZ(),	transformation.column2.getXYZ());

			if (t.getDeterminant() * scale < 0.0f)
			{
				const uint32_t numTetraMap = (uint32_t)mGraphicalLods[i]->tetraMap.arraySizes[0];
				ClothingGraphicalLodParametersNS::TetraLink_Type* tetraMap = mGraphicalLods[i]->tetraMap.buf;

				for (uint32_t j = 0; j < numTetraMap; j++)
				{
					PxVec3 bary = tetraMap[j].vertexBary;
					bary.z = 1.0f - bary.x - bary.y - bary.z;
					tetraMap[j].vertexBary = bary;

					bary = tetraMap[j].normalBary;
					bary.z = 1.0f - bary.x - bary.y - bary.z;
					tetraMap[j].normalBary = bary;
				}
			}
		}
	}
}



void ClothingAssetAuthoringImpl::updateBindPoses(const PxMat44* newBindPoses, uint32_t newBindPosesCount, bool isInternalOrder, bool collisionMaintainWorldPose)
{
	WRITE_ZONE();
	Array<PxMat44> transformation(mBones.size());

	bool hasSkew = false;
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		PX_ASSERT(mBones[i].internalIndex == (int32_t)i);
		const uint32_t externalIndex = isInternalOrder ? i : mBones[i].externalIndex;

		if (externalIndex >= newBindPosesCount)
		{
			transformation[i] = PxMat44(PxIdentity);
		}
		else
		{
			PxMat44 temp;
			temp = mBones[i].bindPose.inverseRT();
			mBones[i].bindPose = newBindPoses[externalIndex];
			transformation[i] = temp * mBones[i].bindPose;

			PxMat33 m(transformation[i].column0.getXYZ(), transformation[i].column1.getXYZ(), transformation[i].column2.getXYZ());
			float det = m.getDeterminant();

			if (PxAbs(det) < 0.99f)
			{
				hasSkew = true;
			}
		}
	}

	if (hasSkew)
	{
		APEX_INVALID_PARAMETER("Skew on matrices is not allowed, aborting");
		return;
	}

	int32_t errorInSize = -1;

	if (collisionMaintainWorldPose)
	{
		for (uint32_t i = 0; i < mBoneActors.size(); i++)
		{
			const uint32_t boneIndex = (uint32_t)mBoneActors[i].boneIndex;
			if (transformation[boneIndex].transform(PxVec3(1.f)) != PxVec3(1.f))
			{
				if (mBoneActors[i].convexVerticesCount == 0)
				{
					// capsule
					PxMat33 m33(transformation[boneIndex].column0.getXYZ(), transformation[boneIndex].column1.getXYZ(), transformation[boneIndex].column2.getXYZ());
					PxMat44 invTransformation(m33.getInverse(), m33.transform(-transformation[boneIndex].getPosition()));
					mBoneActors[i].localPose = invTransformation * mBoneActors[i].localPose;
				}
				else
				{
					// convex
					const uint32_t start = mBoneActors[i].convexVerticesStart;
					const uint32_t end = start + mBoneActors[i].convexVerticesCount;
					for (uint32_t j = start; j < end; j++)
					{
						PX_ASSERT(j < mBoneVertices.size());
						mBoneVertices[j] = transformation[boneIndex].transform(mBoneVertices[j]);
					}
				}
			}
			else
			{
				const uint32_t boneIdx = (uint32_t)mBoneActors[i].boneIndex;
				const int32_t externalIndex = isInternalOrder ? (int32_t)boneIdx : mBones[boneIdx].externalIndex;

				errorInSize = PxMax(errorInSize, externalIndex);
			}
		}
		for (uint32_t i = 0; i < mBoneSpheres.size(); i++)
		{
			const uint32_t boneIndex = (uint32_t)mBoneSpheres[i].boneIndex;
			if (transformation[boneIndex].transform(PxVec3(1.f)) != PxVec3(1.f))
			{
				PxMat33 invTransformation(transformation[boneIndex].column0.getXYZ(), transformation[boneIndex].column1.getXYZ(), transformation[boneIndex].column2.getXYZ());
				invTransformation = invTransformation.getInverse();
				mBoneSpheres[i].localPos = invTransformation.transform(mBoneSpheres[i].localPos) 
					+ invTransformation.transform(-transformation[boneIndex].getPosition());
			}
			else
			{
				const int32_t externalIndex = isInternalOrder ? (int32_t)boneIndex : mBones[boneIndex].externalIndex;
				errorInSize = PxMax(errorInSize, externalIndex);
			}
		}
	}

#if 0
	// PH: This proved to be actually wrong. We should just adapt the bind pose without moving
	//     the meshes AT ALL.
	for (uint32_t physicalMeshIndex = 0; physicalMeshIndex < mPhysicalMeshes.size(); physicalMeshIndex++)
	{

		const uint32_t numBonesPerVertex = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.numBonesPerVertex;
		if (numBonesPerVertex > 0)
		{
			const uint32_t numVertices = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.numVertices;
			PxVec3* positions = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.vertices.buf;
			PxVec3* normals = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.normals.buf;
			uint16_t* boneIndices = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.boneIndices.buf;
			float* boneWeights = mPhysicalMeshes[physicalMeshIndex]->physicalMesh.boneWeights.buf;
			PX_ASSERT(positions != NULL);
			PX_ASSERT(normals != NULL);
			PX_ASSERT(numBonesPerVertex == 1 || boneWeights != NULL);

			for (uint32_t vertexID = 0; vertexID < numVertices; vertexID++)
			{
				PxVec3 position(0.0f, 0.0f, 0.0f);
				PxVec3 normal(0.0f, 0.0f, 0.0f);
				float sumWeight = 0.0f;
				for (uint32_t k = 0; k < numBonesPerVertex; k++)
				{
					const float weight = numBonesPerVertex > 1 ? boneWeights[vertexID * numBonesPerVertex + k] : 1.0f;
					if (weight > 0.0f)
					{
						const PxMat44 matrix = transformation[boneIndices[vertexID * numBonesPerVertex + k]];
						sumWeight += weight;
						position += matrix.transform(positions[vertexID]) * weight;
						normal += matrix.rotate(normals[vertexID]) * weight;
					}
				}
				if (sumWeight > 0.0f)
				{
					PX_ASSERT(sumWeight >= 0.9999f);
					PX_ASSERT(sumWeight <= 1.0001f);

					positions[vertexID] = position;
					normals[vertexID] = normal;
				}
			}
		}
	}

	PX_ASSERT(mGraphicalLods.size() == mGraphicalMeshesRuntime.size());
	for (uint32_t graphicalMeshIndex = 0; graphicalMeshIndex < mGraphicalLods.size(); graphicalMeshIndex++)
	{
		ClothingGraphicalMeshAsset meshAsset(*mGraphicalMeshesRuntime[graphicalMeshIndex]);
		const uint32_t submeshCount = meshAsset.getSubmeshCount();

		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; submeshIndex++)
		{
			const uint32_t numBonesPerVertex = meshAsset.getNumBonesPerVertex(submeshIndex);
			const uint32_t numVertices = meshAsset.getNumVertices(submeshIndex);

			if (numBonesPerVertex > 0 && numVertices > 0)
			{
				RenderDataFormat::Enum outFormat;
				const uint16_t* boneIndices = (const uint16_t*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_INDEX, outFormat);
				const float* boneWeights = (const float*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_WEIGHT, outFormat);

				PxVec3* positions =	(PxVec3*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::POSITION, outFormat);
				PxVec3* normals =	(PxVec3*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::NORMAL, outFormat);
				PxVec3* tangents =	(PxVec3*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::TANGENT, outFormat);
				PxVec3* bitangents =	(PxVec3*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BINORMAL, outFormat);

				if (boneWeights != NULL || numBonesPerVertex == 1)
				{
					for (uint32_t vertexID = 0; vertexID < numVertices; vertexID++)
					{
						PxVec3 position(0.0f, 0.0f, 0.0f);
						PxVec3 normal(0.0f, 0.0f, 0.0f);
						PxVec3 tangent(0.0f, 0.0f, 0.0f);
						PxVec3 bitangent(0.0f, 0.0f, 0.0f);
						float sumWeight = 0.0f;

						for (uint32_t k = 0; k < numBonesPerVertex; k++)
						{
							const float weight = (boneWeights == NULL) ? 1.0f : boneWeights[vertexID * numBonesPerVertex + k];
							if (weight > 0.0f)
							{
								const PxMat44 matrix = transformation[boneIndices[vertexID * numBonesPerVertex + k]];

								if (positions != NULL)
								{
									position += matrix.transform(positions[vertexID]) * weight;
								}
								if (normals != NULL)
								{
									normal += matrix.rotate(normals[vertexID]) * weight;
								}
								if (tangents != NULL)
								{
									tangent += matrix.rotate(tangents[vertexID]) * weight;
								}
								if (bitangents != NULL)
								{
									bitangent += matrix.rotate(bitangents[vertexID]) * weight;
								}
							}
						}

						if (sumWeight != 0.0f)
						{
							PX_ASSERT(sumWeight > 0.9999f);
							PX_ASSERT(sumWeight < 1.0001f);

							// copy back
							if (positions != NULL)
							{
								positions[vertexID] = position;
							}
							if (normals != NULL)
							{
								normals[vertexID] = normal * ClothingUserRecompute::invSqrt(normal.magnitudeSquared());
							}
							if (tangents != NULL)
							{
								tangents[vertexID] = tangent * ClothingUserRecompute::invSqrt(tangent.magnitudeSquared());
							}
							if (bitangents != NULL)
							{
								bitangents[vertexID] = bitangent * ClothingUserRecompute::invSqrt(bitangent.magnitudeSquared());
							}
						}
					}
				}
			}
		}
	}
#endif

	if (errorInSize > -1)
	{
		APEX_INVALID_PARAMETER("newBindPosesCount must be bigger than %d (is %d)", errorInSize, newBindPosesCount);
	}
}


void ClothingAssetAuthoringImpl::destroy()
{
	if (!mOwnsMaterialLibrary)
	{
		PX_ASSERT(mParams != NULL);
		mParams->materialLibrary = NULL;
	}

	ClothingAssetImpl::destroy(); // delete gets called in here
}



// ----- protected methods ----------------


uint32_t ClothingAssetAuthoringImpl::addBoneConvexInternal(uint32_t boneIndex, const PxVec3* positions, uint32_t numPositions)
{
	// compute average
	PxVec3 average(0.0f, 0.0f, 0.0f);

	uint32_t maxNumberPositions = PxMin(20u, numPositions);
	uint32_t newNumPositions = 0;

	Array<PxVec3> newPositions(numPositions);
	Array<float> minDist(numPositions, PX_MAX_F32);

	if (numPositions > 0)
	{
		for (uint32_t i = 0; i < numPositions; i++)
		{
			average += positions[i];
		}
		average /= (float)numPositions;

		float squaredDistFromAverage = (average - positions[0]).magnitudeSquared();
		uint32_t startVertex = 0;
		for (uint32_t i = 1; i < numPositions; i++)
		{
			float squaredDist = (average - positions[i]).magnitudeSquared();
			if (squaredDist > squaredDistFromAverage)
			{
				squaredDistFromAverage = squaredDist;
				startVertex = i;
			}
		}

		for (uint32_t i = 0; i < numPositions; i++)
		{
			newPositions[i] = positions[i];
		}

		if (startVertex != 0)
		{
			newPositions[0] = positions[startVertex];
			newPositions[startVertex] = positions[0];
		}


		for (uint32_t i = 1; i < maxNumberPositions; i++)
		{
			float max = 0.0f;
			int32_t maxj = -1;
			for (uint32_t j = i; j < numPositions; j++)
			{
				const float distSquared = (newPositions[j] - newPositions[i - 1]).magnitudeSquared();
				if (distSquared < minDist[j])
				{
					minDist[j] = distSquared;
				}

				if (minDist[j] > max)
				{
					max = minDist[j];
					maxj = (int32_t)j;
				}
			}

			if (maxj < 0)
			{
				break;
			}

			const PxVec3 v = newPositions[i];
			newPositions[i] = newPositions[(uint32_t)maxj];
			newPositions[(uint32_t)maxj] = v;

			const float dist = minDist[i];
			minDist[i] = minDist[(uint32_t)maxj];
			minDist[(uint32_t)maxj] = dist;
			newNumPositions = i + 1;
		}

		ClothingAssetParametersNS::ActorEntry_Type& newEntry = mBoneActors.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::ActorEntry_Type));
		newEntry.boneIndex = (int32_t)boneIndex;
		newEntry.convexVerticesStart = mBoneVertices.size();
		newEntry.convexVerticesCount = newNumPositions;
		for (uint32_t i = 0; i < newNumPositions; i++)
		{
			mBoneVertices.pushBack(newPositions[i]);
		}
		compressBoneCollision();
	}
	clearCooked();


	// extract planes from points
	ConvexHullImpl convexHull;
	convexHull.init();
	Array<PxPlane> planes;

	convexHull.buildFromPoints(&newPositions[0], newNumPositions, sizeof(PxVec3));

	uint32_t planeCount = convexHull.getPlaneCount();
	if (planeCount + mBonePlanes.size() > 32)
	{
		APEX_DEBUG_WARNING("The asset is trying to use more than 32 planes for convexes. The collision convex will not be simulated with 3.x cloth.");
	}
	else
	{
		uint32_t convex = 0; // each bit references a plane
		for (uint32_t i = 0; i < planeCount; ++i)
		{
			PxPlane plane = convexHull.getPlane(i);
			convex |= 1 << mBonePlanes.size();
			
			ClothingAssetParametersNS::BonePlane_Type& newEntry = mBonePlanes.pushBack();
			memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::BonePlane_Type));
			newEntry.boneIndex = (int32_t)boneIndex;
			newEntry.n = plane.n;
			newEntry.d = plane.d;
		}

		mCollisionConvexes.pushBack(convex);
	}


	return newNumPositions;
}



void ClothingAssetAuthoringImpl::addBoneCapsuleInternal(uint32_t boneIndex, float capsuleRadius, float capsuleHeight, const PxMat44& localPose)
{
	PX_ASSERT(boneIndex < mBones.size());
	if (capsuleRadius > 0)
	{
		ClothingAssetParametersNS::ActorEntry_Type& newEntry = mBoneActors.pushBack();
		memset(&newEntry, 0, sizeof(ClothingAssetParametersNS::ActorEntry_Type));
		newEntry.boneIndex = (int32_t)boneIndex;
		newEntry.capsuleRadius = capsuleRadius;
		newEntry.capsuleHeight = capsuleHeight;
		newEntry.localPose = localPose;
	}
}



void ClothingAssetAuthoringImpl::clearBoneActorsInternal(int32_t internalBoneIndex)
{
	PX_ASSERT(internalBoneIndex >= 0);
	for (uint32_t i = 0; i < mBoneActors.size(); i++)
	{
		if (mBoneActors[i].boneIndex == internalBoneIndex)
		{
			mBoneActors[i].boneIndex = -1;
		}
	}

	compressBoneCollision();
}



void ClothingAssetAuthoringImpl::compressBones() const
{
	if (mBones.isEmpty())
	{
		return;
	}

	// reset counters
	mParams->rootBoneIndex = uint32_t(-1);
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		mBones[i].numMeshReferenced = 0;
		mBones[i].numRigidBodiesReferenced = 0;

		if (::strcmp(mBones[i].name, mRootBoneName.c_str()) == 0)
		{
			// set root bone index
			mParams->rootBoneIndex = i;

			// declare bone as referenced
			mBones[i].numRigidBodiesReferenced++;
		}
	}

	// update bone reference count from graphics mesh
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer));

		for (uint32_t submeshIndex = 0; submeshIndex < meshAsset.getSubmeshCount(); submeshIndex++)
		{
			RenderDataFormat::Enum outFormat;
			const uint16_t* boneIndices = (const uint16_t*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_INDEX, outFormat);
			if (outFormat != RenderDataFormat::USHORT1 && outFormat != RenderDataFormat::USHORT2 &&
			        outFormat != RenderDataFormat::USHORT3 && outFormat != RenderDataFormat::USHORT4)
			{
				boneIndices = NULL;
			}

			const float* boneWeights = (const float*)meshAsset.getVertexBuffer(submeshIndex, RenderVertexSemantic::BONE_WEIGHT, outFormat);
			if (outFormat != RenderDataFormat::FLOAT1 && outFormat != RenderDataFormat::FLOAT2 &&
			        outFormat != RenderDataFormat::FLOAT3 && outFormat != RenderDataFormat::FLOAT4)
			{
				boneWeights = NULL;
			}

			collectBoneIndices(meshAsset.getNumVertices(submeshIndex), boneIndices, boneWeights, meshAsset.getNumBonesPerVertex(submeshIndex));
		}
	}

	// update bone reference count from physics mesh
	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[i]->physicalMesh;
		collectBoneIndices(physicalMesh.numVertices, physicalMesh.boneIndices.buf, physicalMesh.boneWeights.buf, physicalMesh.numBonesPerVertex);
	}

	// update bone reference count from bone actors
	for (uint32_t i = 0; i < mBoneActors.size(); i++)
	{
		mBones[(uint32_t)mBoneActors[i].boneIndex].numRigidBodiesReferenced++;
	}

	// update bone reference count from spheres
	for (uint32_t i = 0; i < mBoneSpheres.size(); i++)
	{
		mBones[(uint32_t)mBoneSpheres[i].boneIndex].numRigidBodiesReferenced++;
	}

	// update bone reference count from spheres
	for (uint32_t i = 0; i < mBonePlanes.size(); i++)
	{
		mBones[(uint32_t)mBonePlanes[i].boneIndex].numRigidBodiesReferenced++;
	}

	// sort the bones to the following structure:
	// |-- bones referenced by mesh --|-- bones referenced by collision RBs, but not mesh --|-- unreferenced bones --|
	{
		BoneEntryPredicate predicate;
		sort(mBones.begin(), mBones.size(), predicate);
	}


	// create map from old indices to new ones and store the number of used bones
	Array<int32_t> old2new(mBones.size(), -1);
	mParams->bonesReferenced = 0;
	mParams->bonesReferencedByMesh = 0;
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].numMeshReferenced > 0)
		{
			mParams->bonesReferencedByMesh++;
		}

		if (mBones[i].numMeshReferenced > 0 || mBones[i].numRigidBodiesReferenced > 0)
		{
			mParams->bonesReferenced++;
		}

		old2new[(uint32_t)mBones[i].internalIndex] = (int32_t)i;
		mBones[i].internalIndex = (int32_t)i;
	}

	// update bone indices in parent
	for (uint32_t i = 0; i < mBones.size(); i++)
	{
		if (mBones[i].parentIndex != -1)
		{
			mBones[i].parentIndex = old2new[(uint32_t)mBones[i].parentIndex];
		}
	}

	// update bone indices in bone actors
	for (uint32_t i = 0; i < mBoneActors.size(); i++)
	{
		PX_ASSERT(mBoneActors[i].boneIndex != -1);
		mBoneActors[i].boneIndex = old2new[(uint32_t)mBoneActors[i].boneIndex];
	}

	// update bone indices in bone spheres
	for (uint32_t i = 0; i < mBoneSpheres.size(); i++)
	{
		PX_ASSERT(mBoneSpheres[i].boneIndex != -1);
		mBoneSpheres[i].boneIndex = old2new[(uint32_t)mBoneSpheres[i].boneIndex];
	}

	// update bone indices in bone planes
	for (uint32_t i = 0; i < mBonePlanes.size(); i++)
	{
		PX_ASSERT(mBonePlanes[i].boneIndex != -1);
		mBonePlanes[i].boneIndex = old2new[(uint32_t)mBonePlanes[i].boneIndex];
	}

	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer)->permuteBoneIndices(old2new);
	}

	for (uint32_t i = 0; i < mPhysicalMeshes.size(); i++)
	{
		ClothingPhysicalMeshImpl* mesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[i]);
		mesh->permuteBoneIndices(old2new);
		mesh->release();
	}

	if (mParams->rootBoneIndex == uint32_t(-1))
	{
		// no root bone defined, find one within referenced bones
		mParams->rootBoneIndex = 0;
		uint32_t minDepth = mBones.size();
		for (uint32_t i = 0; i < mParams->bonesReferenced; i++)
		{
			uint32_t depth = 0;
			int32_t parent = mBones[i].parentIndex;
			while (parent != -1 && depth < mBones.size())
			{
				parent = mBones[(uint32_t)parent].parentIndex;
				depth++;
			}

			if (depth < minDepth)
			{
				minDepth = depth;
				mParams->rootBoneIndex = i;
			}
		}
	}
	else
	{
		// update root bone index
		mParams->rootBoneIndex = (uint32_t)old2new[mParams->rootBoneIndex];
	}
	PX_ASSERT(mParams->rootBoneIndex < mParams->bonesReferenced);
}



void ClothingAssetAuthoringImpl::compressBoneCollision()
{
	const PxVec3* oldBoneVertices = (const PxVec3*)GetInternalApexSDK()->getTempMemory(sizeof(PxVec3) * mBoneVertices.size());
	if (oldBoneVertices == NULL)
	{
		return;
	}

	memcpy(const_cast<PxVec3*>(oldBoneVertices), mBoneVertices.begin(), sizeof(PxVec3) * mBoneVertices.size());

	// clean out all unused actors
	for (int32_t i = (int32_t)mBoneActors.size() - 1; i >= 0; i--)
	{
		if (mBoneActors[(uint32_t)i].boneIndex < 0)
		{
			mBoneActors.replaceWithLast((uint32_t)i);
		}
	}
	if (!mBoneActors.isEmpty())
	{
		nvidia::sort(mBoneActors.begin(), mBoneActors.size(), ActorEntryPredicate());
	}

	uint32_t boneVerticesWritten = 0;
	for (uint32_t i = 0; i < mBoneActors.size(); i++)
	{
		if (mBoneActors[i].convexVerticesCount == 0)
		{
			mBoneActors[i].convexVerticesStart = 0;
		}
		else
		{
			const uint32_t oldStart = mBoneActors[i].convexVerticesStart;
			const uint32_t count = mBoneActors[i].convexVerticesCount;
			mBoneActors[i].convexVerticesStart = boneVerticesWritten;
			for (uint32_t j = 0; j < count; j++)
			{
				mBoneVertices[boneVerticesWritten++] = oldBoneVertices[oldStart + j];
			}
		}
	}
	mBoneVertices.resize(boneVerticesWritten);

	GetInternalApexSDK()->releaseTempMemory(const_cast<PxVec3*>(oldBoneVertices));
	clearCooked();
}



void ClothingAssetAuthoringImpl::collectBoneIndices(uint32_t numVertices, const uint16_t* boneIndices, const float* boneWeights, uint32_t numBonesPerVertex) const
{
	for (uint32_t i = 0; i < numVertices; i++)
	{
		for (uint32_t j = 0; j < numBonesPerVertex; j++)
		{
			uint16_t index = boneIndices[i * numBonesPerVertex + j];
			float weight = (boneWeights == NULL) ? 1.0f : boneWeights[i * numBonesPerVertex + j];
			if (weight > 0.0f)
			{
				PX_ASSERT(index < mBones.size());
				PX_ASSERT(mBones[index].internalIndex == (int32_t)index);
				mBones[index].numMeshReferenced++;
			}
		}
	}
}



struct Confidentially
{
	Confidentially() : maxDistConfidence(0.0f), collisionDistConfidence(0.0f), collisionRadiusConfidence(0.0f), normalConfidence(0.0f) {}
	float maxDistConfidence;
	float collisionDistConfidence;
	float collisionRadiusConfidence;
	float normalConfidence;
};



void ClothingAssetAuthoringImpl::updateMappingAuthoring(ClothingGraphicalLodParameters& graphicalLod, RenderMeshAssetIntl* renderMeshAssetCopy,
		RenderMeshAssetAuthoringIntl* renderMeshAssetOrig, float normalResemblance, bool ignoreUnusedVertices, IProgressListener* progressListener)
{
	if (graphicalLod.physicalMeshId == (uint32_t) - 1 || renderMeshAssetCopy == NULL)
	{
		return;
	}

	const uint32_t physicalMeshId = graphicalLod.physicalMeshId;

	if (normalResemblance < 0)
	{
		APEX_DEBUG_WARNING("A normal resemblance of %f not allowed, must be positive.", normalResemblance);
		normalResemblance = 90.0f;
	}
	else if (normalResemblance < 5)
	{
		APEX_DEBUG_WARNING("A physicalNormal resemblance of %f is very small, it might discard too many values", normalResemblance);
	}
	else if (normalResemblance > 90.0f)
	{
		normalResemblance = 90.0f;
	}

	ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh = mPhysicalMeshes[physicalMeshId]->physicalMesh;
	uint32_t* masterFlags = mPhysicalMeshesInput[physicalMeshId]->getMasterFlagsBuffer();

	bool skipConstraints = false;
	if (physicalMesh.constrainCoefficients.arraySizes[0] != 0)
	{
		skipConstraints = true;
	}

	PX_ASSERT(graphicalLod.immediateClothMap.buf == NULL);
	PX_ASSERT(graphicalLod.skinClothMapB.buf == NULL);
	PX_ASSERT(graphicalLod.tetraMap.buf == NULL);

	HierarchicalProgressListener progress(100, progressListener);

	Array<AbstractMeshDescription> targetMeshes(renderMeshAssetCopy->getSubmeshCount());
	uint32_t numTotalVertices = 0;
	bool hasTangents = false;
	for (uint32_t submeshIndex = 0; submeshIndex < targetMeshes.size(); submeshIndex++)
	{
		const VertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();

		RenderDataFormat::Enum outFormat;

		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION));
		targetMeshes[submeshIndex].pPosition = (PxVec3*)vb.getBufferAndFormat(outFormat, bufferIndex);
		PX_ASSERT(outFormat == RenderDataFormat::FLOAT3);

		bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::NORMAL));
		targetMeshes[submeshIndex].pNormal = (PxVec3*)(vb.getBufferAndFormat(outFormat, bufferIndex));
		if (outFormat != RenderDataFormat::FLOAT3)
		{
			// Phil - you might need to handle other normal formats
			targetMeshes[submeshIndex].pNormal = NULL;
		}

		bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::TANGENT));
		const void* tangents = vb.getBufferAndFormat(outFormat, bufferIndex);
		if (outFormat == RenderDataFormat::FLOAT3)
		{
			targetMeshes[submeshIndex].pTangent = (PxVec3*)tangents;
			hasTangents = true;
		}
		else if (outFormat == RenderDataFormat::FLOAT4)
		{
			targetMeshes[submeshIndex].pTangent4 = (PxVec4*)tangents;
			hasTangents = true;
		}

		const uint32_t numVertices = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexCount(0);
		targetMeshes[submeshIndex].numVertices = numVertices;

		bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_SLAVE_NAME));
		uint32_t* submeshSlaveFlags = (uint32_t*)vb.getBufferAndFormat(outFormat, bufferIndex);
		targetMeshes[submeshIndex].pVertexFlags = submeshSlaveFlags;
		PX_ASSERT(submeshSlaveFlags == NULL || outFormat == RenderDataFormat::UINT1);

		if (!skipConstraints)
		{
			bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(LATCH_TO_NEAREST_MASTER_NAME));
			uint32_t* submeshMasterFlags = (uint32_t*)vb.getBufferAndFormat(outFormat, bufferIndex);
			PX_ASSERT(submeshMasterFlags == NULL || outFormat == RenderDataFormat::UINT1);

			if (submeshSlaveFlags != NULL && submeshMasterFlags != NULL)
			{
				// overwrite the empty slaves with the master mask
				// provides self-attachment, very important!
				// the original slave values are in the orig render mesh still
				for (uint32_t i = 0; i < numVertices; i++)
				{
					if (submeshSlaveFlags[i] == 0)
					{
						submeshSlaveFlags[i] = submeshMasterFlags[i];
					}
					if (submeshSlaveFlags[i] == 0)
					{
						submeshSlaveFlags[i] = 0xffffffff;
					}
				}
			}
		}
		else
		{
			if (submeshSlaveFlags != NULL)
			{
				// overwrite the empty slaves with 0xffffffff flags
				for (uint32_t i = 0; i < numVertices; i++)
				{
					if (submeshSlaveFlags[i] == 0)
					{
						submeshSlaveFlags[i] = 0xffffffff;
					}
				}
			}
		}

		numTotalVertices += targetMeshes[submeshIndex].numVertices;
	}

	if (physicalMesh.isTetrahedralMesh)
	{
		ParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type> tetraMap(&graphicalLod, "tetraMap", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod.tetraMap));
		progress.setSubtaskWork(50, "Generate Tetra Map");
		generateTetraMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, masterFlags, tetraMap, &progress);
		progress.completeSubtask();
	}
	else
	{
		ParamArray<uint32_t> immediateClothMap(&graphicalLod, "immediateClothMap", reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod.immediateClothMap));
		uint32_t numNotFoundVertices = 0;
		progress.setSubtaskWork(5, "Generate immediate mapping");


		generateImmediateClothMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, masterFlags,
								0.0f, numNotFoundVertices, normalResemblance, immediateClothMap, &progress);

		progress.completeSubtask();

		if (immediateClothMap.isEmpty() || numNotFoundVertices > 0 || hasTangents)
		{
			// if more than 3/4 of all vertices are not found, completely forget about immediate mode.
			const bool clearImmediateMap = (numNotFoundVertices > numTotalVertices * 3 / 4);

			progress.setSubtaskWork(45, "Generate mesh-mesh skinning");
			ParamArray<SkinClothMap> skinClothMap(&graphicalLod, "skinClothMap",
								reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod.skinClothMap));

			generateSkinClothMap(targetMeshes.begin(), targetMeshes.size(), physicalMesh, 
				masterFlags, graphicalLod.immediateClothMap.buf, numNotFoundVertices, 
				skinClothMap, graphicalLod.skinClothMapOffset, clearImmediateMap, &progress);

			graphicalLod.skinClothMapThickness = 1.0f;

			progress.completeSubtask();

			if (clearImmediateMap)
			{
				immediateClothMap.clear();
			}
		}
	}

	if (physicalMesh.normals.arraySizes[0] == 0)
	{
		progress.setSubtaskWork(50, "Update Painting");

		// update painting stuff from all submeshes

		ClothingPhysicalMeshImpl* tempMesh = mModule->createPhysicalMeshInternal(mPhysicalMeshes[physicalMeshId]);

		tempMesh->allocateNormalBuffer();
		if (!skipConstraints)
		{
			tempMesh->allocateConstrainCoefficientBuffer();
		}

		AbstractMeshDescription pMesh;
		pMesh.pPosition = physicalMesh.vertices.buf;
		pMesh.pNormal = physicalMesh.normals.buf;
		ClothingPhysicalMeshParametersNS::ConstrainCoefficient_Type* myConstrains = physicalMesh.constrainCoefficients.buf;
		pMesh.numVertices = physicalMesh.numVertices;

		pMesh.pIndices = physicalMesh.indices.buf;
		pMesh.numIndices = physicalMesh.numIndices;

		uint32_t maxNumBonesPerVertex = 0;
		for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAssetCopy->getSubmeshCount(); submeshIndex++)
		{
			const VertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
			RenderDataFormat::Enum format = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX)));
			const uint32_t numBonesPerVertex = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, format);
			maxNumBonesPerVertex = PxMax(maxNumBonesPerVertex, numBonesPerVertex);
		}

		if (maxNumBonesPerVertex > 0)
		{
			tempMesh->setNumBonesPerVertex(maxNumBonesPerVertex);
			tempMesh->allocateBoneIndexAndWeightBuffers();
			pMesh.numBonesPerVertex = maxNumBonesPerVertex;
			pMesh.pBoneIndices = physicalMesh.boneIndices.buf;
			pMesh.pBoneWeights = physicalMesh.boneWeights.buf;
		}

		Array<Confidentially> confidence(pMesh.numVertices);

		uint32_t graphicalVertexOffset = 0;
		const uint32_t numGraphicalVerticesTotal = numTotalVertices;
		PX_ASSERT(renderMeshAssetCopy->getSubmeshCount() == renderMeshAssetOrig->getSubmeshCount());

		for (uint32_t submeshIndex = 0; submeshIndex < renderMeshAssetCopy->getSubmeshCount(); submeshIndex++)
		{
			const VertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();

			if (vb.getVertexCount() == 0)
			{
				APEX_DEBUG_WARNING("submesh %d has no vertices at all!", submeshIndex);
				continue;
			}

			const VertexBuffer& vbOrig = renderMeshAssetOrig->getSubmesh(submeshIndex).getVertexBuffer();
			PX_ASSERT(vbOrig.getVertexCount() == vb.getVertexCount());

			const VertexFormat& vf = vb.getFormat();
			const uint32_t graphicalMaxDistanceIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME));
			RenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalMaxDistanceIndex);
			const float* graphicalMaxDistance = outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
														reinterpret_cast<const float*>(vb.getBuffer(graphicalMaxDistanceIndex));
			PX_ASSERT(graphicalMaxDistance == NULL || outFormat == RenderDataFormat::FLOAT1);

			const uint32_t graphicalCollisionSphereRadiusIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME));
			outFormat = vf.getBufferFormat(graphicalCollisionSphereRadiusIndex);
			const float* graphicalCollisionSphereRadius = outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
														reinterpret_cast<const float*>(vb.getBuffer(graphicalCollisionSphereRadiusIndex));
			PX_ASSERT(graphicalCollisionSphereRadius == NULL || outFormat == RenderDataFormat::FLOAT1);

			const uint32_t graphicalCollisionSphereDistanceIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME));
			outFormat = vf.getBufferFormat(graphicalCollisionSphereDistanceIndex);
			const float* graphicalCollisionSphereDistance = outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
														reinterpret_cast<const float*>(vb.getBuffer(graphicalCollisionSphereDistanceIndex));
			PX_ASSERT(graphicalCollisionSphereDistance == NULL || outFormat == RenderDataFormat::FLOAT1);
			PX_ASSERT((graphicalCollisionSphereDistance == NULL && graphicalCollisionSphereRadius == NULL) || (graphicalCollisionSphereDistance != NULL && graphicalCollisionSphereRadius != NULL));

			const uint32_t graphicalUsedForPhysicsIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(USED_FOR_PHYSICS_NAME));
			outFormat = vf.getBufferFormat(graphicalUsedForPhysicsIndex);
			const uint8_t* graphicalUsedForPhysics = outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
														reinterpret_cast<const uint8_t*>(vb.getBuffer(graphicalUsedForPhysicsIndex));
			PX_ASSERT(graphicalUsedForPhysics == NULL || outFormat == RenderDataFormat::UBYTE1);

			const uint32_t graphicalSlaveIndex = (uint32_t)vbOrig.getFormat().getBufferIndexFromID(vbOrig.getFormat().getID(LATCH_TO_NEAREST_SLAVE_NAME));
			outFormat = vbOrig.getFormat().getBufferFormat(graphicalSlaveIndex);
			const uint32_t* graphicalSlavesOrig = outFormat != RenderDataFormat::UINT1 ? NULL :
														reinterpret_cast<const uint32_t*>(vbOrig.getBuffer(graphicalSlaveIndex));

			// should not be used anymore!
			outFormat = RenderDataFormat::UNSPECIFIED;

			RenderDataFormat::Enum normalFormat;
			uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::NORMAL));
			const PxVec3* graphicalNormals = reinterpret_cast<const PxVec3*>(vb.getBufferAndFormat(normalFormat, bufferIndex));
			if (normalFormat != RenderDataFormat::FLOAT3)
			{
				// Phil - you might need to handle other normal formats
				graphicalNormals = NULL;
			}

			RenderDataFormat::Enum positionFormat;
			bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::POSITION));
			const PxVec3* graphicalPositions = reinterpret_cast<const PxVec3*>(vb.getBufferAndFormat(positionFormat, bufferIndex));
			if (positionFormat != RenderDataFormat::FLOAT3)
			{
				graphicalPositions = NULL;
			}

			RenderDataFormat::Enum boneIndexFormat;
			bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
			const uint16_t* graphicalBoneIndices = (const uint16_t*)vb.getBufferAndFormat(boneIndexFormat, bufferIndex);
			if (boneIndexFormat != RenderDataFormat::USHORT1 && boneIndexFormat != RenderDataFormat::USHORT2 &&
					boneIndexFormat != RenderDataFormat::USHORT3 && boneIndexFormat != RenderDataFormat::USHORT4)
			{
				// Phil - you might need to handle other normal formats
				graphicalBoneIndices = NULL;
			}

			RenderDataFormat::Enum boneWeightFormat;
			bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_WEIGHT));
			const float* graphicalBoneWeights = reinterpret_cast<const float*>(vb.getBufferAndFormat(boneWeightFormat, bufferIndex));
			if (boneWeightFormat != RenderDataFormat::FLOAT1 && boneWeightFormat != RenderDataFormat::FLOAT2 &&
				boneWeightFormat != RenderDataFormat::FLOAT3 && boneWeightFormat != RenderDataFormat::FLOAT4)
			{
				// Phil - you might need to handle other normal formats
				graphicalBoneWeights = NULL;
			}

			const uint32_t numBonesPerVertex = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, boneIndexFormat);
			PX_ASSERT((graphicalBoneIndices == NULL && graphicalBoneWeights == NULL && numBonesPerVertex == 0) ||
					(graphicalBoneIndices != NULL && numBonesPerVertex > 0));

			const uint32_t numGraphicalVertices = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexCount(0);
			for (uint32_t graphicalVertexIndex = 0; graphicalVertexIndex < numGraphicalVertices; graphicalVertexIndex++)
			{
				if ((graphicalVertexIndex & 0xff) == 0)
				{
					const int32_t percent = int32_t(100 * (graphicalVertexIndex + graphicalVertexOffset) / numGraphicalVerticesTotal);
					progress.setProgress(percent);
				}

				// figure out how many physical vertices this graphical vertex relates to
				uint32_t indices[4];
				float trust[4];

				const bool isSlave =
					(graphicalUsedForPhysics != NULL && graphicalUsedForPhysics[graphicalVertexIndex] == 0) ||
					(graphicalSlavesOrig != NULL && graphicalSlavesOrig[graphicalVertexIndex] != 0);

				uint32_t numIndices = getCorrespondingPhysicalVertices(graphicalLod, submeshIndex, graphicalVertexIndex, pMesh, graphicalVertexOffset, indices, trust);

				// now we have the relevant physical vertices in indices[4]

				if (!skipConstraints && graphicalMaxDistance != NULL && graphicalMaxDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.maxDistance &&
					graphicalMaxDistance[graphicalVertexIndex] < 0.0f)
				{
					APEX_INVALID_PARAMETER("Max Distance at vertex %d (submesh %d) must be >= 0.0 (is %f) or equal to invalid (%f)",
											graphicalVertexIndex, submeshIndex, graphicalMaxDistance[graphicalVertexIndex], mInvalidConstrainCoefficients.maxDistance);
				}

				for (uint32_t temporalIndex = 0; temporalIndex < numIndices; temporalIndex++)
				{
					const uint32_t vertexIndex = indices[temporalIndex];
					const float vertexTrust = trust[temporalIndex];

					if (ignoreUnusedVertices && isSlave)
					{
						continue;
					}

					if (vertexTrust < -0.0001f)
					{
						continue;
					}

					const float confidenceDelta = 0.1f;
					if (!skipConstraints)
					{
						if (graphicalMaxDistance != NULL && graphicalMaxDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.maxDistance)
						{
							if (vertexTrust + confidenceDelta > confidence[vertexIndex].maxDistConfidence)
							{
								confidence[vertexIndex].maxDistConfidence = vertexTrust;

								float& target = myConstrains[vertexIndex].maxDistance;
								const float source = graphicalMaxDistance[graphicalVertexIndex];

								target = source;
							}
						}

						if (graphicalCollisionSphereDistance != NULL && graphicalCollisionSphereDistance[graphicalVertexIndex] != mInvalidConstrainCoefficients.collisionSphereDistance)
						{
							if (vertexTrust + confidenceDelta > confidence[vertexIndex].collisionDistConfidence)
							{
								confidence[vertexIndex].collisionDistConfidence = vertexTrust;

								const float source = graphicalCollisionSphereDistance[graphicalVertexIndex];
								float& target = myConstrains[vertexIndex].collisionSphereDistance;

								target = source;
							}
						}

						if (graphicalCollisionSphereRadius != NULL && graphicalCollisionSphereRadius[graphicalVertexIndex] != mInvalidConstrainCoefficients.collisionSphereRadius)
						{
							if (vertexTrust + confidenceDelta > confidence[vertexIndex].collisionRadiusConfidence)
							{
								confidence[vertexIndex].collisionRadiusConfidence = vertexTrust;

								float& target = myConstrains[vertexIndex].collisionSphereRadius;
								const float source = graphicalCollisionSphereRadius[graphicalVertexIndex];

								target = source;
							}
						}
					}

					if (graphicalBoneIndices != NULL)
					{
						for (uint32_t i = 0; i < numBonesPerVertex; i++)
						{
							const float weight = (graphicalBoneWeights != NULL) ? graphicalBoneWeights[graphicalVertexIndex * numBonesPerVertex + i] : 1.0f;
							if (weight > 0.0f)
							{
								tempMesh->addBoneToVertex(vertexIndex, graphicalBoneIndices[graphicalVertexIndex * numBonesPerVertex + i], weight);
							}
						}
					}

					if (!isSlave)
					{
						if (vertexTrust + confidenceDelta > confidence[vertexIndex].normalConfidence)
						{
							confidence[vertexIndex].normalConfidence = vertexTrust;

							pMesh.pNormal[vertexIndex] = graphicalNormals[graphicalVertexIndex];
						}
					}
				}
			}

			graphicalVertexOffset += numGraphicalVertices;
		}


		bool hasMaxDistance = false;
		bool hasCollisionSphereDistance = false;
		bool hasCollisionSphereRadius = false;
		bool hasBoneWeights = false;
		for (uint32_t i = 0; i < renderMeshAssetCopy->getSubmeshCount(); i++)
		{
			const VertexFormat& vf = renderMeshAssetCopy->getSubmesh(i).getVertexBuffer().getFormat();
			RenderDataFormat::Enum outFormat;
			outFormat = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME)));
			hasMaxDistance				|= outFormat == RenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME)));
			hasCollisionSphereDistance	|= outFormat == RenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME)));
			hasCollisionSphereRadius	|= outFormat == RenderDataFormat::FLOAT1;
			outFormat = vf.getBufferFormat((uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_WEIGHT)));
			hasBoneWeights				|= outFormat != RenderDataFormat::UNSPECIFIED;
		}

		// all values that have not been set are set by nearest neighbor
		uint32_t count[5] = { 0, 0, 0, 0, 0 };
		const uint32_t numVertices = physicalMesh.numVertices;
		for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			if (!skipConstraints)
			{
				float& physicalMaxDistance = myConstrains[vertexIndex].maxDistance;
				if (physicalMaxDistance == PX_MAX_F32)
				{
					count[0]++;
					uint32_t submeshIndex = 0, graphicalVertexIndex = 0;
					if (hasMaxDistance && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, MAX_DISTANCE_NAME, ignoreUnusedVertices))
					{
						const VertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
						const uint32_t graphicalMaxDistanceIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME));
						RenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalMaxDistanceIndex);
						const float* graphicalMaxDistance = reinterpret_cast<const float*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalMaxDistanceIndex));

						PX_ASSERT(outFormat == RenderDataFormat::FLOAT1);
						if (outFormat == RenderDataFormat::FLOAT1)
						{
							physicalMaxDistance = PxMax(0.0f, graphicalMaxDistance[graphicalVertexIndex]);
						}
					}
					else
					{
						physicalMaxDistance = 0.0f;
					}
				}

				float& physicalCollisionSphereDistance = myConstrains[vertexIndex].collisionSphereDistance;
				if (physicalCollisionSphereDistance == PX_MAX_F32)
				{
					count[1]++;
					uint32_t submeshIndex = 0, graphicalVertexIndex = 0;
					if (hasCollisionSphereDistance && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, COLLISION_SPHERE_DISTANCE_NAME, ignoreUnusedVertices))
					{
						const VertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
						const uint32_t graphicalCollisionSphereDistanceIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_DISTANCE_NAME));
						RenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalCollisionSphereDistanceIndex);
						const float* graphicalCollisionSphereDistance = reinterpret_cast<const float*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalCollisionSphereDistanceIndex));

						PX_ASSERT(outFormat == RenderDataFormat::FLOAT1);
						if (outFormat == RenderDataFormat::FLOAT1)
						{
							physicalCollisionSphereDistance = graphicalCollisionSphereDistance[graphicalVertexIndex];
						}
					}
					else
					{
						physicalCollisionSphereDistance = 0.0f;
					}
				}

				float& physicalCollisionSphereRadius = myConstrains[vertexIndex].collisionSphereRadius;
				if (physicalCollisionSphereRadius == PX_MAX_F32)
				{
					count[2]++;
					uint32_t submeshIndex = 0, graphicalVertexIndex = 0;
					if (hasCollisionSphereRadius && getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, COLLISION_SPHERE_RADIUS_NAME, ignoreUnusedVertices))
					{
						const VertexFormat& vf = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getFormat();
						const uint32_t graphicalCollisionSphereRadiusIndex = (uint32_t)vf.getBufferIndexFromID(vf.getID(COLLISION_SPHERE_RADIUS_NAME));
						RenderDataFormat::Enum outFormat = vf.getBufferFormat(graphicalCollisionSphereRadiusIndex);
						const float* graphicalCollisionSphereRadius = reinterpret_cast<const float*>(renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer().getBuffer(graphicalCollisionSphereRadiusIndex));

						PX_ASSERT(outFormat == RenderDataFormat::FLOAT1);
						if (outFormat == RenderDataFormat::FLOAT1)
						{
							physicalCollisionSphereRadius = graphicalCollisionSphereRadius[graphicalVertexIndex];
						}
					}
					else
					{
						physicalCollisionSphereRadius = 0.0f;
					}
				}
			}

			PxVec3& physicalNormal = pMesh.pNormal[vertexIndex];
			if (physicalNormal.isZero() && !(mDeriveNormalsFromBones && pMesh.numBonesPerVertex > 0))
			{
				count[3]++;
				uint32_t submeshIndex = 0, graphicalVertexIndex = 0;
				if (getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, NULL, true))
				{
					RenderDataFormat::Enum normalFormat;
					const VertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
					const VertexFormat& vf = vb.getFormat();
					uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::NORMAL));
					const PxVec3* graphicalNormals = (const PxVec3*)vb.getBufferAndFormat(normalFormat, bufferIndex);
					if (normalFormat != RenderDataFormat::FLOAT3)
					{
						// Phil - you might need to handle other normal formats
						graphicalNormals = NULL;
					}
					if (graphicalNormals != NULL)
					{
						physicalNormal = graphicalNormals[graphicalVertexIndex];
					}
				}
				else
				{
					physicalNormal = PxVec3(0.0f, 1.0f, 0.0f);
				}
			}

			float* boneWeights = (pMesh.pBoneWeights != NULL) ? pMesh.pBoneWeights + (vertexIndex * pMesh.numBonesPerVertex) : NULL;
			if (boneWeights != NULL && hasBoneWeights)
			{
				// is first weight already 0.0 ?
				if (boneWeights[0] <= 0.0f)
				{
					count[4]++;
					uint32_t submeshIndex = 0, graphicalVertexIndex = 0;
					if (getClosestVertex(renderMeshAssetOrig, pMesh.pPosition[vertexIndex], submeshIndex, graphicalVertexIndex, NULL, ignoreUnusedVertices))
					{
						const VertexBuffer& vb = renderMeshAssetCopy->getSubmesh(submeshIndex).getVertexBuffer();
						const VertexFormat& vf = vb.getFormat();
						uint16_t* boneIndices = (pMesh.pBoneIndices != NULL) ? pMesh.pBoneIndices + (vertexIndex * pMesh.numBonesPerVertex) : NULL;
						if (boneIndices != NULL)
						{
							for (uint32_t i = 0; i < pMesh.numBonesPerVertex; i++)
							{
								boneIndices[i] = 0;
								boneWeights[i] = 0.0f;
							}

							RenderDataFormat::Enum boneIndexFormat;
							uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_INDEX));
							const uint16_t* graphicalBoneIndices = reinterpret_cast<const uint16_t*>(vb.getBufferAndFormat(boneIndexFormat, bufferIndex));
							if (boneIndexFormat != RenderDataFormat::USHORT1 && boneIndexFormat != RenderDataFormat::USHORT2 &&
								boneIndexFormat != RenderDataFormat::USHORT3 && boneIndexFormat != RenderDataFormat::USHORT4)
							{
								// Phil - you might need to handle other normal formats
								graphicalBoneIndices = NULL;
							}

							RenderDataFormat::Enum boneWeightFormat;
							bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(RenderVertexSemantic::BONE_WEIGHT));
							const float* graphicalBoneWeights = (const float*)vb.getBufferAndFormat(boneWeightFormat, bufferIndex);
							if (boneWeightFormat != RenderDataFormat::FLOAT1 && boneWeightFormat != RenderDataFormat::FLOAT2 &&
								boneWeightFormat != RenderDataFormat::FLOAT3 && boneWeightFormat != RenderDataFormat::FLOAT4)
							{
								// Phil - you might need to handle other normal formats
								graphicalBoneWeights = NULL;
							}

							const uint32_t graphicalNumBonesPerVertex = vertexSemanticFormatElementCount(RenderVertexSemantic::BONE_INDEX, boneIndexFormat);
							for (uint32_t i = 0; i < graphicalNumBonesPerVertex; i++)
							{
								const float weight = graphicalBoneWeights[graphicalVertexIndex * graphicalNumBonesPerVertex + i];
								const uint16_t index = graphicalBoneIndices[graphicalVertexIndex * graphicalNumBonesPerVertex + i];
								tempMesh->addBoneToVertex(vertexIndex, index, weight);
							}
						}
					}
				}
				tempMesh->normalizeBonesOfVertex(vertexIndex);
			}
		}

		if (mDeriveNormalsFromBones && pMesh.numBonesPerVertex > 0)
		{
			for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
			{
				const PxVec3& pos = pMesh.pPosition[vertexIndex];
				PxVec3& normal = pMesh.pNormal[vertexIndex];
				normal = PxVec3(0.0f);

				for (uint32_t j = 0; j < pMesh.numBonesPerVertex; j++)
				{
					const float boneWeight = pMesh.pBoneWeights[vertexIndex * pMesh.numBonesPerVertex + j];

					if (boneWeight == 0.0f)
					{
						continue;
					}

					const uint16_t externalBoneIndex = pMesh.pBoneIndices[vertexIndex * pMesh.numBonesPerVertex + j];

					const int32_t internalBoneIndex = getBoneInternalIndex(externalBoneIndex);

					const ClothingAssetParametersNS::BoneEntry_Type& bone = mBones[(uint32_t)internalBoneIndex];
					PxVec3 closest  = bone.bindPose.getPosition();
					float  minDist2 = (closest - pos).magnitudeSquared();

					// find closest point on outgoing bones
					for (uint32_t k = 0; k < mBones.size(); k++)
					{
						if (mBones[k].parentIndex != internalBoneIndex)
						{
							continue;
						}

						// closest point on segment
						const PxVec3& a = bone.bindPose.getPosition();
						const PxVec3& b = mBones[k].bindPose.getPosition();
						if (a == b)
						{
							continue;
						}

						const PxVec3 d = b - a;
						float s = (pos - a).dot(d) / d.magnitudeSquared();
						s = PxClamp(s, 0.0f, 1.0f);

						PxVec3 proj = a + d * s;
						float dist2 = (proj - pos).magnitudeSquared();

						if (dist2 < minDist2)
						{
							minDist2 = dist2;
							closest = proj;
						}
					}

					PxVec3 n = pos - closest;
					n.normalize();
					normal += n * boneWeight;
				}
				normal.normalize();
			}

			tempMesh->smoothNormals(3);
		}


		progress.completeSubtask();

		tempMesh->release();
		tempMesh = NULL;
	}
}



bool ClothingAssetAuthoringImpl::hasTangents(const RenderMeshAssetIntl& rma)
{
	bool bHasTangents = false;
	for (uint32_t submeshIndex = 0; submeshIndex < rma.getSubmeshCount(); submeshIndex++)
	{
		const VertexBuffer& vb = rma.getSubmesh(submeshIndex).getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();
		uint32_t bufferIndex = (uint32_t)vf.getBufferIndexFromID(vf.getSemanticID(apex::RenderVertexSemantic::TANGENT));
		RenderDataFormat::Enum outFormat;
		const void* tangents = vb.getBufferAndFormat(outFormat, bufferIndex);
		if (tangents != NULL)
		{
			bHasTangents = true;
			break;
		}
	}

	return bHasTangents;
}



uint32_t ClothingAssetAuthoringImpl::getMaxNumGraphicalVertsActive(const ClothingGraphicalLodParameters& graphicalLod, uint32_t submeshIndex)
{
	uint32_t numVerts = 0;

	const uint32_t numParts = (uint32_t)graphicalLod.physicsMeshPartitioning.arraySizes[0];
	ClothingGraphicalLodParametersNS::PhysicsMeshPartitioning_Type* parts = graphicalLod.physicsMeshPartitioning.buf;

	for (uint32_t i = 0; i < numParts; ++i)
	{
		if (parts[i].graphicalSubmesh == submeshIndex)
		{
			numVerts = PxMax(numVerts, parts[i].numSimulatedVertices);
		}
	}

	return numVerts;
}



bool ClothingAssetAuthoringImpl::isMostlyImmediateSkinned(const RenderMeshAssetIntl& rma, const ClothingGraphicalLodParameters& graphicalLod)
{
	uint32_t immediateMapSize = (uint32_t)graphicalLod.immediateClothMap.arraySizes[0];
	if (immediateMapSize == 0)
		return false;

	// figure out the number of immediate skinned verts.. more complicated than i thought.
	uint32_t numImmediateSkinnedVerts = 0;
	uint32_t totalSkinnedVerts = 0;
	uint32_t* immediateMap = graphicalLod.immediateClothMap.buf;
	uint32_t numSubmeshes = rma.getSubmeshCount();
	uint32_t vertexOffset = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < numSubmeshes; ++submeshIndex)
	{
		const apex::RenderSubmesh& submesh = rma.getSubmesh(submeshIndex);
		uint32_t numVertsToSkin = getMaxNumGraphicalVertsActive(graphicalLod, submeshIndex);
		for (uint32_t vertexIndex = 0; vertexIndex < numVertsToSkin; ++vertexIndex)
		{
			uint32_t mapId = vertexOffset + vertexIndex;

			PX_ASSERT(mapId < immediateMapSize);
			const uint32_t mapEntry = immediateMap[mapId];
			const uint32_t flags = mapEntry & ~ClothingConstants::ImmediateClothingReadMask;

			// count the number of mesh-mesh skinned verts (the others are skinned to bones)
			++totalSkinnedVerts;
			if ((flags & ClothingConstants::ImmediateClothingInSkinFlag) == 0)
			{
				++numImmediateSkinnedVerts;
			}

		}
		vertexOffset += submesh.getVertexCount(0);
	}

	return 2*numImmediateSkinnedVerts > totalSkinnedVerts;
}



bool ClothingAssetAuthoringImpl::conditionalMergeMapping(const RenderMeshAssetIntl& rma, ClothingGraphicalLodParameters& graphicalLod)
{
	bool merged = false;
	// it's faster to do mesh-mesh skinning on all verts, instead of
	// mesh-mesh skinning + immediateSkinning + tangent recompute
	if (hasTangents(rma) && !isMostlyImmediateSkinned(rma, graphicalLod))
	{
		merged = mergeMapping(&graphicalLod);
	}

	return merged;
}



class SkinClothMapPredicate
{
public:
	bool operator()(const SkinClothMapB& map1, const SkinClothMapB& map2) const
	{
		if (map1.submeshIndex < map2.submeshIndex)
		{
			return true;
		}
		else if (map1.submeshIndex > map2.submeshIndex)
		{
			return false;
		}

		return map1.faceIndex0 < map2.faceIndex0;
	}
};



void ClothingAssetAuthoringImpl::sortSkinMapB(SkinClothMapB* skinClothMap, uint32_t skinClothMapSize, uint32_t* immediateClothMap, uint32_t immediateClothMapSize)
{

	sort<SkinClothMapB, SkinClothMapPredicate>(skinClothMap, skinClothMapSize, SkinClothMapPredicate());

	if (immediateClothMap != NULL)
	{
		for (uint32_t j = 0; j < skinClothMapSize; j++)
		{
			const uint32_t vertexIndex = skinClothMap[j].vertexIndexPlusOffset;
			if (vertexIndex < immediateClothMapSize)
			{
				PX_ASSERT((immediateClothMap[vertexIndex] & ClothingConstants::ImmediateClothingInSkinFlag) != 0);
				immediateClothMap[vertexIndex] = j | ClothingConstants::ImmediateClothingInSkinFlag;
			}
		}
	}
}



class F32Greater
{
public:
	PX_INLINE bool operator()(float v1, float v2) const
	{
		return v1 > v2;
	}
};




void ClothingAssetAuthoringImpl::setupPhysicalMesh(ClothingPhysicalMeshParameters& physicalMesh) const
{
	const uint32_t numIndicesPerElement = (physicalMesh.physicalMesh.isTetrahedralMesh) ? 4u : 3u;

	// index buffer is sorted such that each triangle (or tetrahedra) has a higher or equal max distance than all tuples right of it.
	for (uint32_t i = 0; i < physicalMesh.physicalMesh.numIndices; i += numIndicesPerElement)
	{
		float triangleMaxDistance = getMaxMaxDistance(physicalMesh.physicalMesh, i, numIndicesPerElement);
		if (triangleMaxDistance == 0.0f								// don't simulate triangles that may not move
			|| i == physicalMesh.physicalMesh.numIndices - numIndicesPerElement) // all vertices are painted, i.e. this is the last tuple.
		{
			const uint32_t maxIndex = (i == physicalMesh.physicalMesh.numIndices - numIndicesPerElement) ? i + numIndicesPerElement : i;
			physicalMesh.physicalMesh.numSimulatedIndices = maxIndex;

			// these values get set in reorderDeformableVertices
			physicalMesh.physicalMesh.numSimulatedVertices = 0;
			physicalMesh.physicalMesh.numMaxDistance0Vertices = 0;

			if (triangleMaxDistance == 0.0f)
			{
				break;
			}
		}
	}
}



void ClothingAssetAuthoringImpl::sortDeformableIndices(ClothingPhysicalMeshImpl& physicalMesh)
{
	if (physicalMesh.getNumVertices() == 0 || physicalMesh.getConstrainCoefficientBuffer() == NULL)
	{
		return;
	}

	uint32_t* deformableIndices = physicalMesh.getIndicesBuffer();
	ClothingConstrainCoefficients* constrainCoeffs = physicalMesh.getConstrainCoefficientBuffer();

	Array<uint32_t> deformableIndicesPermutation;
	deformableIndicesPermutation.resize(physicalMesh.getNumIndices());

	uint32_t numIndices = physicalMesh.isTetrahedralMesh() ? 4u : 3u;

	for (uint32_t i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		deformableIndicesPermutation[i] = i;
	}

	if (numIndices == 3)
	{
		TriangleGreater_3 triangleGreater(deformableIndices, constrainCoeffs);
		nvidia::sort((uint32_t_3*)&deformableIndicesPermutation[0], physicalMesh.getNumIndices() / numIndices, triangleGreater);
	}
	else if (numIndices == 4)
	{
		TriangleGreater_4 triangleGreater(deformableIndices, constrainCoeffs);
		nvidia::sort((uint32_t_4*)&deformableIndicesPermutation[0], physicalMesh.getNumIndices() / numIndices, triangleGreater);
	}
	else
	{
		PX_ALWAYS_ASSERT();
	}


	// inverse permutation
	Array<int32_t> invPerm(physicalMesh.getNumIndices());
	for (uint32_t i = 0; i < physicalMesh.getNumIndices(); i++)
	{
		PX_ASSERT(deformableIndicesPermutation[i] < physicalMesh.getNumIndices());
		invPerm[deformableIndicesPermutation[i]] = (int32_t)i;
	}


	// apply permutation
	ApexPermute<uint32_t>(deformableIndices, &deformableIndicesPermutation[0], physicalMesh.getNumIndices());

	// update mappings into deformable index buffer
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mPhysicalMeshes[mGraphicalLods[i]->physicalMeshId] == physicalMesh.getNvParameterized())
		{
			ClothingGraphicalMeshAssetWrapper meshAsset(reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[i]->renderMeshAssetPointer));
			ClothingGraphicalLodParameters* graphicalLod = mGraphicalLods[i];

			const uint32_t numGraphicalVertices = meshAsset.getNumTotalVertices();
			if (graphicalLod->tetraMap.arraySizes[0] > 0)
			{
				for (uint32_t j = 0; j < numGraphicalVertices; j++)
				{
					graphicalLod->tetraMap.buf[j].tetraIndex0 = (uint32_t)invPerm[graphicalLod->tetraMap.buf[j].tetraIndex0];
				}
			}
			const uint32_t skinClothMapBSize = (uint32_t)graphicalLod->skinClothMapB.arraySizes[0];
			if (skinClothMapBSize > 0)
			{
				for (uint32_t j = 0; j < skinClothMapBSize; j++)
				{
					graphicalLod->skinClothMapB.buf[j].faceIndex0 = (uint32_t)invPerm[graphicalLod->skinClothMapB.buf[j].faceIndex0];
				}

				sortSkinMapB(graphicalLod->skinClothMapB.buf, (uint32_t)graphicalLod->skinClothMapB.arraySizes[0], graphicalLod->immediateClothMap.buf, numGraphicalVertices);
			}
		}
	}

	physicalMesh.updateMaxMaxDistance();
}



bool ClothingAssetAuthoringImpl::getGraphicalLodIndex(uint32_t lod, uint32_t& graphicalLodIndex) const
{
	graphicalLodIndex = UINT32_MAX;
	for (uint32_t i = 0; i < mGraphicalLods.size(); i++)
	{
		if (mGraphicalLods[i]->lod == lod)
		{
			graphicalLodIndex = i;
			return true;
		}
	}
	return false;
}



uint32_t ClothingAssetAuthoringImpl::addGraphicalLod(uint32_t lod)
{
	uint32_t lodIndex = (uint32_t) - 1;
	if (getGraphicalLodIndex(lod, lodIndex))
	{
		return lodIndex;
	}

	ClothingGraphicalLodParameters* newLod = DYNAMIC_CAST(ClothingGraphicalLodParameters*)(GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingGraphicalLodParameters::staticClassName()));
	newLod->lod = lod;
	newLod->renderMeshAssetPointer = NULL;
	PX_ASSERT(newLod->physicalMeshId == (uint32_t) - 1);

	mGraphicalLods.pushBack(NULL);

	// insertion sort
	int32_t current = (int32_t)mGraphicalLods.size() - 1;
	while (current > 0 && mGraphicalLods[(uint32_t)current - 1]->lod > newLod->lod)
	{
		mGraphicalLods[(uint32_t)current] = mGraphicalLods[(uint32_t)current - 1];
		current--;
	}
	PX_ASSERT(current >= 0);
	PX_ASSERT((uint32_t)current < mGraphicalLods.size());
	mGraphicalLods[(uint32_t)current] = newLod;

	return (uint32_t)current;
}



void ClothingAssetAuthoringImpl::clearCooked()
{
	ParamArray<ClothingAssetParametersNS::CookedEntry_Type> cookedEntries(mParams, "cookedData", reinterpret_cast<ParamDynamicArrayStruct*>(&mParams->cookedData));

	if (cookedEntries.size() > 0)
	{
		if(cookedEntries[0].cookedData)
		{
			mPreviousCookedType = cookedEntries[0].cookedData->className();
		}
	}

	cookedEntries.clear();
}



bool ClothingAssetAuthoringImpl::addGraphicalMesh(RenderMeshAssetAuthoring* renderMesh, uint32_t graphicalLodIndex)
{
	if (mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer != NULL)
	{
		reinterpret_cast<RenderMeshAssetIntl*>(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer)->release();
		mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer = NULL;
	}
	if (mGraphicalLods[graphicalLodIndex]->renderMeshAsset != NULL)
	{
		// PH: did the param object already get destroyed in the release call above?
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset->destroy();
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset = NULL;
	}

	if (renderMesh != NULL)
	{
		const uint32_t additionalSize = 7;
		char buf[16];
		int bufSize = 16;
		char* rmaName = buf;
		if (strlen(getName()) + additionalSize > 15)
		{
			bufSize = int32_t(strlen(getName()) + additionalSize + 2);
			rmaName = (char*)PX_ALLOC((size_t)bufSize, PX_DEBUG_EXP("ClothingAssetAuthoring::addGraphicalMesh"));
		}
		nvidia::snprintf(rmaName, (size_t)bufSize, "%s_RMA%.3d", getName(), mGraphicalLods[graphicalLodIndex]->lod);

		PX_ASSERT(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer == NULL);
		//mGraphicalMeshesRuntime[graphicalLodIndex] = DYNAMIC_CAST(RenderMeshAssetIntl*)(GetApexSDK()->createAsset(*renderMesh, rmaName));
		NvParameterized::Interface* newAsset = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(renderMesh->getNvParameterized()->className());
		newAsset->copy(*renderMesh->getNvParameterized());

		RenderMeshAssetIntl* rma = static_cast<RenderMeshAssetIntl*>(GetApexSDK()->createAsset(newAsset, rmaName));
		PX_ASSERT(rma->getAssetNvParameterized()->equals(*renderMesh->getNvParameterized()));
		if (rma->mergeBinormalsIntoTangents())
		{
			APEX_DEBUG_INFO("The ApexRenderMesh has Tangent and Binormal semantic (both FLOAT3), but clothing needs only Tangent (with FLOAT4). Converted internally");
		}
		mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer = rma;

		PX_ASSERT(::strcmp(newAsset->className(), "RenderMeshAssetParameters") == 0);
		mGraphicalLods[graphicalLodIndex]->renderMeshAsset = newAsset;

		// make sure the isReferenced value is set!
		NvParameterized::Handle handle(*newAsset);
		newAsset->getParameterHandle("isReferenced", handle);
		PX_ASSERT(handle.isValid());
		if (handle.isValid())
		{
			bool val;
			handle.getParamBool(val);
			PX_ASSERT(!val);
			handle.setParamBool(true);
		}

		if (rmaName != buf)
		{
			PX_FREE(rmaName);
			rmaName = buf;
		}

		return true;
	}
	else
	{
		PX_ASSERT(mGraphicalLods[graphicalLodIndex] != NULL);

		// store the pointer to the element that is removed
		PX_ASSERT(mGraphicalLods[graphicalLodIndex]->renderMeshAssetPointer == NULL);
		mGraphicalLods[graphicalLodIndex]->destroy();

		for (uint32_t i = graphicalLodIndex; i < mGraphicalLods.size() - 1; i++)
		{
			mGraphicalLods[i] = mGraphicalLods[i + 1];
		}

		mGraphicalLods.back() = NULL; // set last element to NULL, otherwise the referred object gets destroyed in popBack
		mGraphicalLods.popBack();

		return false;
	}
}



void ClothingAssetAuthoringImpl::initParams()
{
	PX_ASSERT(mParams != NULL);
	if (mParams != NULL)
	{
		mParams->setSerializationCallback(this, NULL);
	}

	if (mParams->materialLibrary == NULL)
	{
		mOwnsMaterialLibrary = true;
		mParams->materialLibrary = GetInternalApexSDK()->getParameterizedTraits()->createNvParameterized(ClothingMaterialLibraryParameters::staticClassName());
	}
}



bool ClothingAssetAuthoringImpl::generateImmediateClothMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, uint32_t* masterFlags,
		float epsilon, uint32_t& numNotFoundVertices, float normalResemblance, ParamArray<uint32_t>& result,
		IProgressListener* progress) const
{
	// square because distance is also squared.
	epsilon = epsilon * epsilon;

	bool haveAllBuffers = true;

	uint32_t numGraphicalVertices = 0;
	for (uint32_t i = 0; i < numTargetMeshes; i++)
	{
		numGraphicalVertices += targetMeshes[i].numVertices;
		bool hasAllBuffers = true;
		hasAllBuffers &= targetMeshes[i].pPosition != NULL;
		hasAllBuffers &= targetMeshes[i].pNormal != NULL;

		haveAllBuffers &= hasAllBuffers;

		if (!hasAllBuffers)
		{
			APEX_INTERNAL_ERROR("Render mesh asset does not have either position or normal for submesh %d!", i);
		}
	}

	if (!haveAllBuffers)
	{
		numNotFoundVertices = 0;
		return false;
	}

	result.resize(numGraphicalVertices);
	for (uint32_t i = 0; i < numGraphicalVertices; i++)
	{
		result[i] = ClothingConstants::ImmediateClothingInvalidValue;
	}

	numNotFoundVertices = 0;
	float			notFoundError = 0.0f;
	float			maxDotError = 0.0f;
	const float		maxDotMinimum = PxClamp(PxCos(physx::shdfnd::degToRad(normalResemblance)), 0.0f, 1.0f);

	const PxVec3*	physicalPositions = physicalMesh.vertices.buf;
	const PxVec3*	physicalNormals = physicalMesh.skinningNormals.buf;
	const uint32_t*	physicsMasterFlags = masterFlags;
	const uint32_t	numPhysicsVertices = physicalMesh.numVertices;
	PX_ASSERT(physicsMasterFlags != NULL);

	uint32_t submeshVertexOffset = 0;
	for (uint32_t submeshIndex = 0; submeshIndex < numTargetMeshes; submeshIndex++)
	{
		const PxVec3*	positions = targetMeshes[submeshIndex].pPosition;
		const PxVec3*	normals = targetMeshes[submeshIndex].pNormal;
		const uint32_t*	slaveFlags = targetMeshes[submeshIndex].pVertexFlags;
		const uint32_t	numVertices = targetMeshes[submeshIndex].numVertices;

		for (uint32_t index = 0; index < numVertices; index++)
		{
			if (progress != NULL && ((index & 0xff) == 0))
			{
				const int32_t percent = int32_t(100 * (index + submeshVertexOffset) / numGraphicalVertices);

				progress->setProgress(percent);
			}

			float minDistanceSquared = FLT_MAX;
			int32_t optimalMatch = -1;
			float maxDot = 0.0f;
			const uint32_t slave = slaveFlags != NULL ? slaveFlags[index] : 0xffffffff;

			for (uint32_t vertexIndex = 0; vertexIndex < numPhysicsVertices && (minDistanceSquared > 0 || maxDot < maxDotMinimum); vertexIndex++)
			{
				const uint32_t master = physicsMasterFlags[vertexIndex];
				if ((master & slave) == 0)
				{
					continue;
				}

				const float distSquared = (physicalPositions[vertexIndex] - positions[index]).magnitudeSquared();
				const float dot = normals[index].dot(physicalNormals[vertexIndex]);
				if (distSquared < minDistanceSquared || (distSquared == minDistanceSquared && PxAbs(dot) > PxAbs(maxDot)))
				{
					minDistanceSquared = distSquared;
					optimalMatch = (int32_t)vertexIndex;
					maxDot = dot;
				}
			}

			if (optimalMatch == -1 || minDistanceSquared > epsilon || PxAbs(maxDot) < maxDotMinimum)
			{
				notFoundError += sqrtf(minDistanceSquared);
				maxDotError += PxAbs(maxDot);

				if (PxAbs(maxDot) < maxDotMinimum && minDistanceSquared <= epsilon)
				{
					result[index + submeshVertexOffset] = (uint32_t)optimalMatch;
					result[index + submeshVertexOffset] |= ClothingConstants::ImmediateClothingBadNormal;
				}
				else
				{
					result[index + submeshVertexOffset] = ClothingConstants::ImmediateClothingInvalidValue;
				}
				numNotFoundVertices++;
			}
			else
			{
				result[index + submeshVertexOffset] = (uint32_t)optimalMatch;
				if (maxDot < 0)
				{
					result[index + submeshVertexOffset] |= ClothingConstants::ImmediateClothingInvertNormal;
				}
			}
		}

		submeshVertexOffset += numVertices;
	}

	return result.size() == numGraphicalVertices;
}



bool ClothingAssetAuthoringImpl::generateSkinClothMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
							ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, 
							uint32_t* masterFlags, uint32_t* immediateMap, uint32_t numEmptyInImmediateMap, 
							ParamArray<SkinClothMap>& result, float& offsetAlongNormal, 
							bool integrateImmediateMap, IProgressListener* progress) const
{
	if (immediateMap == NULL || integrateImmediateMap)
	{
		uint32_t sum = 0;
		for (uint32_t i = 0; i < numTargetMeshes; i++)
		{
			sum += targetMeshes[i].numVertices;
		}
		result.resize(sum);
	}
	else
	{
		result.resize(numEmptyInImmediateMap);
	}

	// figure out some details about the physical mesh
	AbstractMeshDescription srcPM;
	srcPM.numIndices	= physicalMesh.numIndices;
	srcPM.numVertices	= physicalMesh.numVertices;
	srcPM.pIndices		= physicalMesh.indices.buf;
	srcPM.pNormal		= physicalMesh.skinningNormals.buf;
	srcPM.pPosition		= physicalMesh.vertices.buf;
	srcPM.pVertexFlags	= masterFlags;

	srcPM.UpdateDerivedInformation(NULL);

	// PH: Negating this leads to interesting effects, but also to some side effects...
	offsetAlongNormal = DEFAULT_PM_OFFSET_ALONG_NORMAL_FACTOR * srcPM.avgEdgeLength;

	const uint32_t physNumIndices = physicalMesh.numIndices;

	// compute mapping
	Array<TriangleWithNormals> triangles;
	triangles.reserve(physNumIndices / 3);

	// create a list of physics mesh triangles
	float avgHalfDiagonal = 0.0f;
	for (uint32_t i = 0; i < physNumIndices; i += 3)
	{
		TriangleWithNormals triangle;

		// store vertex information in triangle
		triangle.master = 0;
		for (uint32_t j = 0; j < 3; j++)
		{
			triangle.vertices[j]	= srcPM.pPosition[srcPM.pIndices[i + j]];
			triangle.normals[j]		= srcPM.pNormal[srcPM.pIndices[i + j]];
			triangle.master			|= srcPM.pVertexFlags != NULL ? srcPM.pVertexFlags[srcPM.pIndices[i + j]] : 0xffffffff;
		}
		triangle.faceIndex0 = i;

		triangle.init();
		triangle.bounds.fattenFast(srcPM.avgEdgeLength);

		PxVec3 boundsDiag = triangle.bounds.getExtents();
		avgHalfDiagonal += boundsDiag.magnitude();
		triangles.pushBack(triangle);
	}
	avgHalfDiagonal /= triangles.size();

	// hash the triangles
	ApexMeshHash hash;
	hash.setGridSpacing(avgHalfDiagonal);
	for (uint32_t i = 0; i < triangles.size(); i++)
	{
		hash.add(triangles[i].bounds, i);
	}

	// find the best triangle for each graphical vertex
	SkinClothMap* mapEntry = result.begin();

	Array<uint32_t> queryResult;

	uint32_t targetOffset = 0;
	for (uint32_t targetIndex = 0; targetIndex < numTargetMeshes; targetIndex++)
	{
		const uint32_t numVertices = targetMeshes[targetIndex].numVertices;
		for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			const uint32_t slave = targetMeshes[targetIndex].pVertexFlags != NULL ? targetMeshes[targetIndex].pVertexFlags[vertexIndex] : 0xffffffff;
			PX_ASSERT(slave != 0);

			const uint32_t	index = vertexIndex + targetOffset;
			const PxVec3	position = targetMeshes[targetIndex].pPosition[vertexIndex];
			const PxVec3	normal = targetMeshes[targetIndex].pNormal[vertexIndex];
			const PxVec3	normalRelative = position + (normal * offsetAlongNormal);
			PxVec3 tangentRelative(0.0f);
			if (targetMeshes[targetIndex].pTangent != NULL)
			{
				tangentRelative = position + (targetMeshes[targetIndex].pTangent[vertexIndex] * offsetAlongNormal);
			}
			else if (targetMeshes[targetIndex].pTangent4 != NULL)
			{
				const PxVec3 tangent(targetMeshes[targetIndex].pTangent4[vertexIndex].x,
									targetMeshes[targetIndex].pTangent4[vertexIndex].y,
									targetMeshes[targetIndex].pTangent4[vertexIndex].z);
				tangentRelative = position + (tangent * offsetAlongNormal);
			}

			if (((immediateMap != NULL) && ((immediateMap[index] & ClothingConstants::ImmediateClothingBadNormal) != 0)) || integrateImmediateMap)
			{

				// read physics vertex
				const uint32_t bestVertex = immediateMap[index] & ClothingConstants::ImmediateClothingReadMask;

				// mark entry as invalid, because we put it into the skinClothMap
				immediateMap[index] = ClothingConstants::ImmediateClothingInvalidValue;

				float bestError = PX_MAX_F32;
				int32_t bestIndex = -1;

				for (uint32_t pIndex = 0; pIndex < physNumIndices; pIndex++)
				{
					if (srcPM.pIndices[pIndex] == bestVertex)
					{
						// this is a triangle that contains bestVertex (from the immediate map)

						uint32_t faceIndex0	= pIndex - (pIndex % 3);
						uint32_t triangleIndex	= faceIndex0 / 3;
						PX_ASSERT(triangleIndex < triangles.size());

						TriangleWithNormals& triangle = triangles[triangleIndex];
						PX_ASSERT(triangle.faceIndex0 == faceIndex0);

						if (triangle.doNotUse)
						{
							continue;
						}

						if ((triangle.master & slave) == 0)
						{
							continue;
						}

						ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, int32_t(vertexIndex + targetOffset), false);

						if (triangle.valid != 2)
						{
							continue;
						}

						float error = computeTriangleError(triangle, normal);

						// use the best triangle that contains the vertex
						if (error < bestError)
						{
							bestIndex = (int32_t)triangleIndex;
							bestError = error;
						}
					}
				}
				//PX_ASSERT(bestIndex != -1);
				if (bestIndex != -1)
				{
					immediateMap[index] = (uint32_t)(mapEntry - result.begin());
					immediateMap[index] |= ClothingConstants::ImmediateClothingInSkinFlag;

					mapEntry->vertexBary	= triangles[(uint32_t)bestIndex].tempBaryVertex;
					uint32_t faceIndex		= triangles[(uint32_t)bestIndex].faceIndex0;
					mapEntry->vertexIndex0	= srcPM.pIndices[faceIndex + 0];
					mapEntry->vertexIndex1	= srcPM.pIndices[faceIndex + 1];
					mapEntry->vertexIndex2	= srcPM.pIndices[faceIndex + 2];

					mapEntry->normalBary	= triangles[(uint32_t)bestIndex].tempBaryNormal;
					mapEntry->tangentBary	= triangles[(uint32_t)bestIndex].tempBaryTangent;
					mapEntry->vertexIndexPlusOffset = index;
					mapEntry++;

					continue;
				}
				//PX_ASSERT(0 && "generateSkinClothMapC: We should never end up here");
			}

			if (immediateMap != NULL && immediateMap[index] != ClothingConstants::ImmediateClothingInvalidValue)
			{
				continue;
			}

			if (progress != NULL && (index & 0xf) == 0)
			{
				const uint32_t location = (uint32_t)(mapEntry - result.begin());
				const int32_t percent = int32_t(100 * location / result.size());

				progress->setProgress(percent);
			}

			int32_t bestTriangleNr = -1;
			float bestTriangleError = PX_MAX_F32;

			// query for physical triangles around the graphics vertex
			hash.query(position, queryResult);
			for (uint32_t q = 0; q < queryResult.size(); q++)
			{
				const uint32_t triangleNr = queryResult[q];
				TriangleWithNormals& triangle = triangles[triangleNr];

				if (triangle.doNotUse)
				{
					continue;
				}

				if ((triangle.master & slave) == 0)
				{
					continue;
				}

				if (!triangle.bounds.contains(position))
				{
					continue;
				}

				ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, int32_t(vertexIndex + targetOffset), false);

				if (triangle.valid != 2)
				{
					continue;
				}

				const float error = computeTriangleError(triangle, normal);

				if (error < bestTriangleError)
				{
					bestTriangleNr = (int32_t)triangleNr;
					bestTriangleError = error;
				}
			}

			if (bestTriangleNr < 0)
			{
				// nothing was found nearby, search in all triangles
				bestTriangleError = PX_MAX_F32;
				for (uint32_t j = 0; j < triangles.size() && bestTriangleError > 0.0f; j++)
				{
					TriangleWithNormals& triangle = triangles[j];

					if (triangle.doNotUse)
					{
						continue;
					}

					if ((triangle.master & slave) == 0)
					{
						continue;
					}

					triangle.timestamp = -1;
					ModuleClothingHelpers::computeTriangleBarys(triangle, position, normalRelative, tangentRelative, offsetAlongNormal, int32_t(vertexIndex + targetOffset), false);

					if (triangle.valid == 0)
					{
						continue;
					}

					float error = computeTriangleError(triangle, normal);

					// increase the error a lot, but still better than nothing
					if (triangle.valid != 2)
					{
						error += 100.0f;
					}

					if (error < bestTriangleError)
					{
						bestTriangleError = error;
						bestTriangleNr = (int32_t)j;
					}
				}
			}

			if (bestTriangleNr >= 0)
			{
				const TriangleWithNormals& bestTriangle = triangles[(uint32_t)bestTriangleNr];

				if (immediateMap != NULL)
				{
					PX_ASSERT(immediateMap[index] == ClothingConstants::ImmediateClothingInvalidValue);
					immediateMap[index] = (uint32_t)(mapEntry - result.begin());
					immediateMap[index] |= ClothingConstants::ImmediateClothingInSkinFlag;
				}

				PX_ASSERT(bestTriangle.faceIndex0 % 3 == 0);
				//mapEntry->tetraIndex = bestTriangle.tetraIndex;
				//mapEntry->submeshIndex = targetIndex;

				mapEntry->vertexBary = bestTriangle.tempBaryVertex;
				uint32_t faceIndex = bestTriangle.faceIndex0;
				mapEntry->vertexIndex0 = srcPM.pIndices[faceIndex + 0];
				mapEntry->vertexIndex1 = srcPM.pIndices[faceIndex + 1];
				mapEntry->vertexIndex2 = srcPM.pIndices[faceIndex + 2];
				mapEntry->normalBary = bestTriangle.tempBaryNormal;
				mapEntry->tangentBary = bestTriangle.tempBaryTangent;
				mapEntry->vertexIndexPlusOffset = index;
				mapEntry++;
			}
			else if (immediateMap != NULL)
			{
				PX_ASSERT(immediateMap[index] == ClothingConstants::ImmediateClothingInvalidValue);
			}
		}

		targetOffset += numVertices;
	}


	uint32_t sizeused = (uint32_t)(mapEntry - result.begin());
	if (sizeused < result.size())
	{
		APEX_DEBUG_WARNING("%d vertices could not be mapped, they will be static!", result.size() - sizeused);
	}
	result.resize(sizeused);

	return true;
}

template <typename T>
class SkinClothMapPredicate2
{
public:
	bool operator()(const T& map1, const T& map2) const
	{
		return map1.vertexIndexPlusOffset < map2.vertexIndexPlusOffset;
	}
};


void ClothingAssetAuthoringImpl::removeMaxDistance0Mapping(ClothingGraphicalLodParameters& graphicalLod, RenderMeshAssetIntl* renderMeshAsset) const
{
	ParamArray<SkinClothMap> skinClothMap(&graphicalLod, "skinClothMap",
		reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod.skinClothMap));

	ParamArray<uint32_t> immediateClothMap(&graphicalLod, "immediateClothMap",
		reinterpret_cast<ParamDynamicArrayStruct*>(&graphicalLod.immediateClothMap));

	// temp array to keep the simulated verts, as we want to discard the fixed verts
	Array<SkinClothMap> skinClothMapNew;
	skinClothMapNew.reserve(skinClothMap.size());

	uint32_t offset = 0;
	for (uint32_t s = 0; s < renderMeshAsset->getSubmeshCount(); s++)
	{
		// get submesh data
		const VertexBuffer& vb = renderMeshAsset->getSubmesh(s).getVertexBuffer();
		const VertexFormat& vf = vb.getFormat();

		const uint32_t graphicalMaxDistanceIndex	= (uint32_t)vf.getBufferIndexFromID(vf.getID(MAX_DISTANCE_NAME));
		RenderDataFormat::Enum outFormat			= vf.getBufferFormat(graphicalMaxDistanceIndex);
		const float* graphicalMaxDistance			= outFormat == RenderDataFormat::UNSPECIFIED ? NULL :
													reinterpret_cast<const float*>(vb.getBuffer(graphicalMaxDistanceIndex));

		const uint32_t numVertices = renderMeshAsset->getSubmesh(s).getVertexCount(0);
		if (graphicalMaxDistance == NULL)
		{
			PX_ALWAYS_ASSERT();
			offset += numVertices;
			continue;
		}

		PX_ASSERT(outFormat == RenderDataFormat::FLOAT1);

		for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			const uint32_t index = vertexIndex + offset;

			uint32_t skinClothMapIndex = (uint32_t)-1;
			if (immediateClothMap.size() > index && immediateClothMap[index] & ClothingConstants::ImmediateClothingInSkinFlag)
			{
				// read the index out of the skinClothMap
				skinClothMapIndex = immediateClothMap[index] & ClothingConstants::ImmediateClothingReadMask;
				immediateClothMap[index] = skinClothMapNew.size() | ClothingConstants::ImmediateClothingInSkinFlag;
			}
			else if (skinClothMap.size() > index && index == skinClothMap[index].vertexIndexPlusOffset)
			{
				// if there's no immediateMap, all verts should be in the skinClothMap
				skinClothMapIndex = index;
			}
			else
			{
				// we only get here, if there are some verts without mapping -> bad!
				for (uint32_t i = 0; i < skinClothMap.size(); i++)
				{
					if (index == skinClothMap[i].vertexIndexPlusOffset)
					{	
						skinClothMapIndex = i;
					}
				}
			}

			if (skinClothMapIndex != (uint32_t)-1)
			{
				if (graphicalMaxDistance[vertexIndex] != mInvalidConstrainCoefficients.maxDistance 
					&& graphicalMaxDistance[vertexIndex] == 0.0f)
				{
					// non-simulated verts are removed from the skinClothMap (not added to skinClothMapNew)
					if (immediateClothMap.size() > index)
					{
						immediateClothMap[index] = ClothingConstants::ImmediateClothingInvalidValue;
					}
				}
				else
				{
					// keep the entry for simulated verts
					skinClothMapNew.pushBack(skinClothMap[skinClothMapIndex]);
				}
			}
		}

		offset += numVertices;
	}

	// store reduced skinClothMap
	skinClothMap.resize(skinClothMapNew.size());
	for (uint32_t i = 0; i < skinClothMapNew.size(); ++i)
	{
		skinClothMap[i] = skinClothMapNew[i];
	}
}

bool ClothingAssetAuthoringImpl::generateTetraMap(const AbstractMeshDescription* targetMeshes, uint32_t numTargetMeshes,
		ClothingPhysicalMeshParametersNS::PhysicalMesh_Type& physicalMesh, uint32_t* /*masterFlags*/,
		ParamArray<ClothingGraphicalLodParametersNS::TetraLink_Type>& result, IProgressListener* progress) const
{
	uint32_t numGraphicalVertices = 0;


	bool haveAllBuffers = true;

	for (uint32_t submeshIndex = 0; submeshIndex < numTargetMeshes; submeshIndex++)
	{
		numGraphicalVertices += targetMeshes[submeshIndex].numVertices;

		bool hasAllBuffers = true;
		hasAllBuffers &= targetMeshes[submeshIndex].pPosition != NULL;
		hasAllBuffers &= targetMeshes[submeshIndex].pNormal != NULL;

		haveAllBuffers &= hasAllBuffers;
		if (!hasAllBuffers)
		{
			APEX_INTERNAL_ERROR("Render mesh asset does not have either position or normal for submesh %d!", submeshIndex);
		}
	}

	if (!haveAllBuffers)
	{
		return false;
	}

	result.resize(numGraphicalVertices);
	memset(result.begin(), 0, sizeof(ClothingGraphicalLodParametersNS::TetraLink_Type) * numGraphicalVertices);

	uint32_t submeshVertexOffset = 0;

	const uint32_t* physicalIndices = physicalMesh.indices.buf;
	const PxVec3* physicalPositions = physicalMesh.vertices.buf;

	for (uint32_t targetIndex = 0; targetIndex < numTargetMeshes; targetIndex++)
	{
		const uint32_t numVertices = targetMeshes[targetIndex].numVertices;

		const PxVec3* positions = targetMeshes[targetIndex].pPosition;
		const PxVec3* normals = targetMeshes[targetIndex].pNormal;

		for (uint32_t vertexIndex = 0; vertexIndex < numVertices; vertexIndex++)
		{
			const uint32_t index = vertexIndex + submeshVertexOffset;
			if (progress != NULL && (index & 0x3f) == 0)
			{
				const int32_t percent = int32_t(100 * index / numGraphicalVertices);
				progress->setProgress(percent);
			}

			const PxVec3 position = positions[vertexIndex];

			float bestWorstBary = FLT_MAX;
			int32_t bestTet = -1;
			PxVec3 bestBary(0.0f, 0.0f, 0.0f);
			for (uint32_t j = 0; j < physicalMesh.numIndices; j += 4)
			{
				PxVec3 p[4];
				for (uint32_t k = 0; k < 4; k++)
				{
					p[k] = physicalPositions[physicalIndices[j + k]];
				}

				PxVec3 bary;
				generateBarycentricCoordinatesTet(p[0], p[1], p[2], p[3], position, bary);
				float baryU = 1 - bary.x - bary.y - bary.z;
				float worstBary = 0.0f;
				worstBary = PxMax(worstBary, -bary.x);
				worstBary = PxMax(worstBary, -bary.y);
				worstBary = PxMax(worstBary, -bary.z);
				worstBary = PxMax(worstBary, -baryU);
				worstBary = PxMax(worstBary, bary.x - 1);
				worstBary = PxMax(worstBary, bary.y - 1);
				worstBary = PxMax(worstBary, bary.z - 1);
				worstBary = PxMax(worstBary, baryU - 1);
				//PX_ASSERT(worstBary + bestWorstBary > 0 && "they must not be 0 both!!!");
				if (worstBary < bestWorstBary)
				{
					bestWorstBary = worstBary;
					bestTet = (int32_t)j;
					bestBary = bary;
				}
			}

			PX_ASSERT(result[index].tetraIndex0 == 0);

			result[index].vertexBary = bestBary;
			result[index].tetraIndex0 = (uint32_t)bestTet;

			// compute barycentric coordinates of normal
			PxVec3 normal(1.0f, 0.0f, 0.0f);
			if (normals != NULL)
			{
				normal = normals[vertexIndex];
				normal.normalize();
			}
			const PxVec3& pa = physicalPositions[physicalIndices[bestTet + 0]];
			const PxVec3& pb = physicalPositions[physicalIndices[bestTet + 1]];
			const PxVec3& pc = physicalPositions[physicalIndices[bestTet + 2]];
			const PxVec3& pd = physicalPositions[physicalIndices[bestTet + 3]];
			PxBounds3 bounds;
			bounds.setEmpty();
			bounds.include(pa);
			bounds.include(pb);
			bounds.include(pc);
			bounds.include(pd);
			// we use a second point above pos, along the normal.
			// The offset must be small but arbitrary since we normalize the resulting normal during skinning
			const float offset = (bounds.minimum - bounds.maximum).magnitude() * 0.01f;
			generateBarycentricCoordinatesTet(pa, pb, pc, pd, position + normal * offset, result[index].normalBary);

		}


		submeshVertexOffset += numVertices;
	}

	return true;
}



float ClothingAssetAuthoringImpl::computeBaryError(float baryX, float baryY) const
{
#if 0
	const float triangleSize = 1.0f;
	const float baryZ = 1.0f - baryX - baryY;

	const float errorX = (baryX - (1.0f / 3.0f)) * triangleSize;
	const float errorY = (baryY - (1.0f / 3.0f)) * triangleSize;
	const float errorZ = (baryZ - (1.0f / 3.0f)) * triangleSize;

	return (errorX * errorX) + (errorY * errorY) + (errorZ * errorZ);
#elif 0
	float dist = 0.0f;
	if (-baryX > dist)
	{
		dist = -baryX;
	}
	if (-baryY > dist)
	{
		dist = -baryY;
	}
	float sum = baryX + baryY - 1.0f;
	if (sum > dist)
	{
		dist = sum;
	}
	return dist * dist;
#else
	const float baryZ = 1.0f - baryX - baryY;

	const float errorX = PxMax(PxAbs(baryX - 0.5f) - 0.5f, 0.0f);
	const float errorY = PxMax(PxAbs(baryY - 0.5f) - 0.5f, 0.0f);
	const float errorZ = PxMax(PxAbs(baryZ - 0.5f) - 0.5f, 0.0f);

	return (errorX * errorX) + (errorY * errorY) + (errorZ * errorZ);
#endif
}



float ClothingAssetAuthoringImpl::computeTriangleError(const TriangleWithNormals& triangle, const PxVec3& normal) const
{
	PxVec3 faceNormal = (triangle.vertices[1] - triangle.vertices[0]).cross(triangle.vertices[2] - triangle.vertices[0]);
	faceNormal.normalize();

	const float avgTriangleEdgeLength = ((triangle.vertices[0] - triangle.vertices[1]).magnitude() +
	                                     (triangle.vertices[0] - triangle.vertices[2]).magnitude() +
	                                     (triangle.vertices[1] - triangle.vertices[2]).magnitude()) / 3.0f;

	float error = computeBaryError(triangle.tempBaryVertex.x, triangle.tempBaryVertex.y);

	PX_ASSERT(PxAbs(1 - normal.magnitude()) < 0.001f); // make sure it's normalized.

	//const float normalWeight = faceNormal.cross(normal).magnitude(); // 0 for co-linear, 1 for perpendicular
	const float normalWeight = 0.5f * (1.0f - faceNormal.dot(normal));

	error += PxClamp(normalWeight, 0.0f, 1.0f) * computeBaryError(triangle.tempBaryNormal.x, triangle.tempBaryNormal.y);

	const float heightValue = triangle.tempBaryVertex.z / avgTriangleEdgeLength;
	const float heightWeight = 0.1f + 2.5f * computeBaryError(triangle.tempBaryVertex.x, triangle.tempBaryVertex.y);
	const float heightError = heightWeight * PxAbs(heightValue);
	error += heightError;

	return error;
}

}
} // namespace nvidia

#endif // WITHOUT_APEX_AUTHORING

