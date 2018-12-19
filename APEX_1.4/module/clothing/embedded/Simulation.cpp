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


#include "Simulation.h"

#include "ModuleClothingImpl.h"
#include "ClothingScene.h"
#include "ClothingCookedPhysX3Param.h"

#include "DebugRenderParams.h"
#include "ClothingDebugRenderParams.h"
//#include "RenderDebugInterface.h"
#include "RenderDebugInterface.h"

#include "ModuleClothingHelpers.h"
#include "ClothStructs.h"

// only for the phase flags
#include "ExtClothFabricCooker.h"

// from LowLevelCloth
#include "Cloth.h"
#include "Fabric.h"
#include "Factory.h"
#include "Range.h"
#include "Solver.h"

#include "ApexSDKIntl.h"
#include "SceneIntl.h"
#include "PxCudaContextManager.h"
#include "PxGpuDispatcher.h"

#include "PsIntrinsics.h"
#include "ProfilerCallback.h"

#include <ApexCollision.h>
#include "ApexMath.h"

#include "Lock.h"

#include "ClothingCollisionImpl.h"

// visualize convexes
#include "ApexSharedUtils.h"

// pvd
#include "ApexPvdClient.h"

#include "PxPvdDataStream.h"
#include "PxPvdUserRenderer.h"

#if PX_PHYSICS_VERSION_MAJOR == 3
#include "ScopedPhysXLock.h"
#endif

namespace nvidia
{
namespace clothing
{

using namespace physx;

Simulation::Simulation(ClothingScene* clothingScene, bool useCuda) : SimulationAbstract(clothingScene),
	mCookedData(NULL),
	mIndices(NULL),
	mRestPositions(NULL),
	mConstrainCoeffs(NULL),
	mCloth(NULL),
	mNumAssetSpheres(0),
	mNumAssetCapsules(0),
	mNumAssetCapsulesInvalid(0),
	mNumAssetConvexes(0),
	mConstrainConstantsDirty(false),
	mMotionConstrainScale(1.0f),
	mMotionConstrainBias(0.0f),
	mNumBackstopConstraints(-1),
	mScaledGravity(0.0f),
	mLastTimestep(0.0f),
	mLocalSpaceSim(false),
	mGlobalPose(PxMat44(PxIdentity)),
	mGlobalPosePrevious(PxMat44(PxIdentity)),
	mGlobalPoseNormalized(PxMat44(PxIdentity)),
	mGlobalPoseNormalizedInv(PxMat44(PxIdentity)),
	mActorScale(0.0f),
	mTetherLimit(0.0f),
	mTeleported(false),
	mIsStatic(false)
{
	PX_ASSERT(clothingScene != NULL);
	PX_UNUSED(useCuda);

#if PX_WINDOWS_FAMILY
	mUseCuda = useCuda;
#else
	mUseCuda = false; // disabled on consoles
#endif
}



Simulation::~Simulation()
{
	if (mCloth != NULL)
	{
		mClothingScene->lockScene();
		mClothingScene->getClothSolver(mUseCuda)->removeCloth(mCloth);
		delete mCloth;
		mClothingScene->unlockScene();
		mCloth = NULL;
	}
}



bool Simulation::needsExpensiveCreation()
{
	// disable caching of unused objects!
	return false;
}



bool Simulation::needsAdaptiveTargetFrequency()
{
	// this is handled by the cloth solver directly
	return false;
}



bool Simulation::needsManualSubstepping()
{
	// the solver will interpolate the skinned positions itself
	return false;
}



bool Simulation::needsLocalSpaceGravity()
{
	return false;
}



uint32_t Simulation::getNumSolverIterations() const
{
	uint32_t numSolverIterations = 0;
	if (mCloth != NULL)
	{
		numSolverIterations = (uint32_t)PxMax(1, int(mLastTimestep * mCloth->getSolverFrequency() + 0.5f));
	}
	return numSolverIterations;
}



bool Simulation::setCookedData(NvParameterized::Interface* cookedData, float actorScale)
{
	PX_ASSERT(cookedData != NULL);

	mActorScale = actorScale;
	PX_ASSERT(mActorScale > 0.0f);

	if (::strcmp(cookedData->className(), ClothingCookedPhysX3Param::staticClassName()) != 0)
	{
		PX_ALWAYS_ASSERT();
		return false;
	}

	mCookedData = static_cast<ClothingCookedPhysX3Param*>(cookedData);

	return true;
}


bool Simulation::initPhysics(uint32_t _physicalMeshId, uint32_t* indices, PxVec3* restPositions, tMaterial* material, const PxMat44& /*globalPose*/, const PxVec3& scaledGravity, bool /*localSpaceSim*/)
{
	PX_ASSERT(mCookedData != NULL);

	while (mCookedData->physicalMeshId != _physicalMeshId)
	{
		mCookedData = static_cast<ClothingCookedPhysX3Param*>(mCookedData->nextCookedData);
	}

	PX_ASSERT(mCookedData != NULL);
	PX_ASSERT(mCookedData->physicalMeshId == _physicalMeshId);

	mIndices = indices;
	mRestPositions = restPositions;

	if (mCookedData != NULL)
	{
#if PX_PHYSICS_VERSION_MAJOR == 3
		SCOPED_PHYSX_LOCK_WRITE(mClothingScene->getApexScene());
#else
		WRITE_LOCK(*mClothingScene->getApexScene());
#endif
		// PH: mUseCuda is passed by reference. If for whatever reason a FactoryGPU could not be created, a FactoryCPU is returned and mUseCuda will be false
		ClothFactory factory = mClothingScene->getClothFactory(mUseCuda);
		nvidia::Mutex::ScopedLock _wlockFactory(*factory.mutex);


		// find if there's a shared fabric
		cloth::Fabric* fabric = NULL;
		if (factory.factory->getPlatform() == cloth::Factory::CPU)
		{
			fabric = (cloth::Fabric*)mCookedData->fabricCPU;
		}
		else
		{
			for (int32_t i = 0; i < mCookedData->fabricGPU.arraySizes[0]; ++i)
			{
				if (mCookedData->fabricGPU.buf[i].factory == factory.factory)
				{
					fabric = (cloth::Fabric*)mCookedData->fabricGPU.buf[i].fabricGPU;
					break;
				}
			}
		}

		if (fabric == NULL)
		{
			nvidia::Array<uint32_t> phases((uint32_t)mCookedData->deformablePhaseDescs.arraySizes[0]);
			for (uint32_t i = 0; i < phases.size(); i++)
				phases[i] = mCookedData->deformablePhaseDescs.buf[i].setIndex;
			nvidia::Array<uint32_t> sets((uint32_t)mCookedData->deformableSets.arraySizes[0]);
			for (uint32_t i = 0; i < sets.size(); i++)
			{
				sets[i] = mCookedData->deformableSets.buf[i].fiberEnd;
			}
			cloth::Range<uint32_t> indices(mCookedData->deformableIndices.buf,   mCookedData->deformableIndices.buf     + mCookedData->deformableIndices.arraySizes[0]);
			cloth::Range<float> restLengths(mCookedData->deformableRestLengths.buf, mCookedData->deformableRestLengths.buf + mCookedData->deformableRestLengths.arraySizes[0]);
			cloth::Range<uint32_t> tetherAnchors(mCookedData->tetherAnchors.buf, mCookedData->tetherAnchors.buf + mCookedData->tetherAnchors.arraySizes[0]);
			cloth::Range<float> tetherLengths(mCookedData->tetherLengths.buf, mCookedData->tetherLengths.buf + mCookedData->tetherLengths.arraySizes[0]);

			PX_PROFILE_ZONE("ClothingActorImpl::createClothFabric", GetInternalApexSDK()->getContextId());

			// TODO use PhysX interface to scale tethers when available
			for (int i = 0; i < mCookedData->tetherLengths.arraySizes[0]; ++i)
			{
				mCookedData->tetherLengths.buf[i] *= simulation.restLengthScale;
			}

			fabric = factory.factory->createFabric(
			             mCookedData->numVertices,
			             cloth::Range<uint32_t>(phases.begin(), phases.end()),
			             cloth::Range<uint32_t>(sets.begin(), sets.end()),
			             restLengths,
			             indices,
			             tetherAnchors,
			             tetherLengths
			         );


			// store new fabric pointer so it can be shared
			if (factory.factory->getPlatform() == cloth::Factory::CPU)
			{
				mCookedData->fabricCPU = fabric;
			}
			else
			{
				NvParameterized::Handle handle(*mCookedData);
				int32_t arraysize = 0;

				if (mCookedData->getParameterHandle("fabricGPU", handle) == NvParameterized::ERROR_NONE)
				{
					handle.getArraySize(arraysize, 0);
					handle.resizeArray(arraysize + 1);
					PX_ASSERT(mCookedData->fabricGPU.arraySizes[0] == arraysize+1);
					
					ClothingCookedPhysX3ParamNS::FabricGPU_Type fabricGPU;
					fabricGPU.fabricGPU = fabric;
					fabricGPU.factory = factory.factory;
					mCookedData->fabricGPU.buf[arraysize] = fabricGPU;
				}
			}


			if (simulation.restLengthScale != 1.0f && fabric != NULL)
			{
				uint32_t numPhases = phases.size();
				float* restValueScales = (float*)GetInternalApexSDK()->getTempMemory(numPhases * sizeof(float));
				(fabric)->scaleRestvalues( simulation.restLengthScale );
				GetInternalApexSDK()->releaseTempMemory(restValueScales);
			}
		}

		if (fabric != NULL && mCloth == NULL)
		{
			PX_ASSERT(mCookedData->deformableInvVertexWeights.arraySizes[0] == (int32_t)mCookedData->numVertices);

			Array<PxVec4> startPositions(mCookedData->numVertices);
			for (uint32_t i = 0; i < mCookedData->numVertices; i++)
			{
				startPositions[i] = PxVec4(sdkWritebackPosition[i], mCookedData->deformableInvVertexWeights.buf[i]);
			}

			const PxVec4* pos = (const PxVec4*)startPositions.begin();

			cloth::Range<const PxVec4> startPos(pos, pos + startPositions.size());

			PX_PROFILE_ZONE("ClothingActorImpl::createCloth", GetInternalApexSDK()->getContextId());

			mCloth = factory.factory->createCloth(startPos, *((cloth::Fabric*)fabric));
		}

		if (mCloth != NULL)
		{
			// setup capsules
			const uint32_t numSupportedCapsules = 32;
			const uint32_t* collisionIndicesEnd = (mCollisionCapsules.size() > 2 * numSupportedCapsules) ? &mCollisionCapsules[2 * numSupportedCapsules] : mCollisionCapsules.end();
			cloth::Range<const uint32_t> cIndices(mCollisionCapsules.begin(), collisionIndicesEnd);
			mCloth->setCapsules(cIndices,0,mCloth->getNumCapsules());

			// setup convexes
			cloth::Range<const uint32_t> convexes(mCollisionConvexes.begin(), mCollisionConvexes.end());
			mCloth->setConvexes(convexes,0,mCloth->getNumConvexes());

			mClothingScene->lockScene();
			mClothingScene->getClothSolver(mUseCuda)->addCloth(mCloth);
			mClothingScene->unlockScene();
			mIsStatic = false;

			// add virtual particles
			const uint32_t numVirtualParticleIndices = (uint32_t)mCookedData->virtualParticleIndices.arraySizes[0];
			const uint32_t numVirtualParticleWeights = (uint32_t)mCookedData->virtualParticleWeights.arraySizes[0];
			if (numVirtualParticleIndices > 0)
			{
				cloth::Range<const uint32_t[4]> vIndices((const uint32_t(*)[4])(mCookedData->virtualParticleIndices.buf), (const uint32_t(*)[4])(mCookedData->virtualParticleIndices.buf + numVirtualParticleIndices));
				cloth::Range<const PxVec3> weights((PxVec3*)mCookedData->virtualParticleWeights.buf, (PxVec3*)(mCookedData->virtualParticleWeights.buf + numVirtualParticleWeights));
				mCloth->setVirtualParticles(vIndices, weights);
			}

			const uint32_t numSelfcollisionIndices = (uint32_t)mCookedData->selfCollisionIndices.arraySizes[0];
			ModuleClothingImpl* module = static_cast<ModuleClothingImpl*>(mClothingScene->getModule());
			if (module->useSparseSelfCollision() && numSelfcollisionIndices > 0)
			{
				cloth::Range<const uint32_t> vIndices(mCookedData->selfCollisionIndices.buf, mCookedData->selfCollisionIndices.buf + numSelfcollisionIndices);
				mCloth->setSelfCollisionIndices(vIndices);
			}

			applyCollision();

			mTeleported = true; // need to clear inertia
		}
	}


	// configure phases
	mPhaseConfigs.clear();

	// if this is hit, PhaseConfig has changed. check if we need to adapt something below.
	PX_COMPILE_TIME_ASSERT(sizeof(cloth::PhaseConfig) == 20);

	const uint32_t numPhaseDescs = (uint32_t)mCookedData->deformablePhaseDescs.arraySizes[0];
	for (uint32_t i = 0; i < numPhaseDescs; ++i)
	{
		cloth::PhaseConfig phaseConfig;
		phaseConfig.mPhaseIndex = uint16_t(i);
		phaseConfig.mStiffness = 1.0f;
		phaseConfig.mStiffnessMultiplier = 1.0f;

		mPhaseConfigs.pushBack(phaseConfig);
	}

	if (mCloth != NULL)
	{
		cloth::Range<cloth::PhaseConfig> phaseConfig(mPhaseConfigs.begin(), mPhaseConfigs.end());
		mCloth->setPhaseConfig(phaseConfig);
	}

	// apply clothing material after phases are set up
	if (material != NULL)
	{
		applyClothingMaterial(material, scaledGravity);
	}

	physicalMeshId = _physicalMeshId;

	return (mCloth != NULL);
}


void Simulation::initCollision(tBoneActor* boneActors, uint32_t numBoneActors,
									 tBoneSphere* boneSpheres, uint32_t numBoneSpheres,
									 uint16_t* spherePairIndices, uint32_t numSpherePairIndices,
									 tBonePlane* bonePlanes, uint32_t numBonePlanes,
									 uint32_t* convexes, uint32_t numConvexes, tBoneEntry* bones,
									 const PxMat44* boneTransforms,
									 ResourceList& actorPlanes,
									 ResourceList& actorConvexes,
									 ResourceList& actorSpheres,
									 ResourceList& actorCapsules,
									 ResourceList& actorTriangleMeshes,
									 const tActorDescTemplate& /*actorDesc*/, const tShapeDescTemplate& /*shapeDesc*/, float actorScale,
									 const PxMat44& globalPose, bool localSpaceSim)
{
	// these need to be initialized here, because they are read in
	// updateCollision
	mLocalSpaceSim = localSpaceSim;
	setGlobalPose(globalPose); // initialize current frame
	setGlobalPose(globalPose); // initialize previous frame

	if (numBoneActors + numBoneSpheres + actorPlanes.getSize() + actorSpheres.getSize() + actorTriangleMeshes.getSize() == 0)
	{
		return;
	}

	if (numBoneActors > 0 && numBoneSpheres > 0)
	{
		// ignore case where both exist
		APEX_INVALID_PARAMETER("This asset contains regular collision volumes and new ones. Having both is not supported, ignoring the regular ones");
		numBoneActors = 0;
	}

	mActorScale = actorScale;

	// Note: each capsule will have two spheres at each end, nothing is shared, so the index map is quite trivial so far
	for (uint32_t i = 0; i < numBoneActors; i++)
	{
		if (boneActors[i].convexVerticesCount == 0)
		{
			PX_ASSERT(boneActors[i].capsuleRadius > 0.0f);
			if (mCollisionCapsules.size() < 32)
			{
				uint32_t index = mCollisionCapsules.size();
				mCollisionCapsules.pushBack(index);
				mCollisionCapsules.pushBack(index + 1);
			}
			else
			{
				uint32_t index = mCollisionCapsules.size() + mCollisionCapsulesInvalid.size();
				mCollisionCapsulesInvalid.pushBack(index);
				mCollisionCapsulesInvalid.pushBack(index + 1);
			}
		}
	}

	// now add the sphere pairs for PhysX3 capsules
	for (uint32_t i = 0; i < numSpherePairIndices; i += 2)
	{
		if (spherePairIndices[i] < 32 && spherePairIndices[i + 1] < 32)
		{
			mCollisionCapsules.pushBack(spherePairIndices[i]);
			mCollisionCapsules.pushBack(spherePairIndices[i + 1]);
		}
		else
		{
			mCollisionCapsulesInvalid.pushBack(spherePairIndices[i]);
			mCollisionCapsulesInvalid.pushBack(spherePairIndices[i + 1]);
		}
	}
	mNumAssetCapsules = mCollisionCapsules.size();
	mNumAssetCapsulesInvalid = mCollisionCapsulesInvalid.size();

	// convexes
	for (uint32_t i = 0; i < numConvexes; ++i)
	{
		mCollisionConvexes.pushBack(convexes[i]);
	}
	mNumAssetConvexes = mCollisionConvexes.size();

	// notify triangle meshes of initialization
	for (uint32_t i = 0; i < actorTriangleMeshes.getSize(); ++i)
	{
		ClothingTriangleMeshImpl* mesh = (ClothingTriangleMeshImpl*)(actorTriangleMeshes.getResource(i));
		mesh->setId(-1); // this makes sure that mesh->update does not try read non-existing previous frame data
	}

	updateCollision(boneActors, numBoneActors, boneSpheres, numBoneSpheres, bonePlanes, numBonePlanes, bones, boneTransforms,
					actorPlanes, actorConvexes, actorSpheres, actorCapsules, actorTriangleMeshes, false);

	if (!mCollisionCapsulesInvalid.empty())
	{
		PX_ASSERT(mCollisionSpheres.size() > 32);
		if (mCollisionSpheres.size() > 32)
		{
			APEX_INVALID_PARAMETER("This asset has %d collision volumes, but only 32 are supported. %d will be ignored!", mCollisionSpheres.size(), mCollisionSpheres.size() - 32);
		}
	}
}



class CollisionCompare
{
public:
	PX_INLINE bool operator()(const ApexResourceInterface* a, const ApexResourceInterface* b) const
	{
		ClothingCollisionImpl* collisionA = (ClothingCollisionImpl*)a;
		ClothingCollisionImpl* collisionB = (ClothingCollisionImpl*)b;
		return (uint32_t)collisionA->getId() < (uint32_t)collisionB->getId(); // cast to uint32_t so we get -1 at the end
	}
};



void Simulation::updateCollision(tBoneActor* boneActors, uint32_t numBoneActors,
									   tBoneSphere* boneSpheres, uint32_t numBoneSpheres,
									   tBonePlane* bonePlanes, uint32_t numBonePlanes,
									   tBoneEntry* bones, const PxMat44* boneTransforms,
									   ResourceList& actorPlanes,
									   ResourceList& actorConvexes,
									   ResourceList& actorSpheres,
									   ResourceList& actorCapsules,
									   ResourceList& actorTriangleMeshes,
									   bool /*teleport*/)
{
	if (numBoneActors > 0 && numBoneSpheres > 0)
	{
		// error message already emitted in initCollision
		numBoneActors = 0;
	}

	// Note: if we have more than 32 collision spheres, we add them to the array, but we don't pass more than 32 of them to the PxCloth (allows to still debug render them in red)

	const float collisionThickness = simulation.thickness / 2.0f;

	PX_ASSERT(mActorScale != 0.0f);

	if (numBoneActors > 0)
	{
		// old style
		if (mCollisionSpheres.empty())
		{
			// resize them the first time
			uint32_t count = 0;
			for (uint32_t i = 0; i < numBoneActors; i++)
			{
				count += (boneActors[i].convexVerticesCount == 0) ? 2 : 0;
			}
			mNumAssetSpheres = count;
			mCollisionSpheres.resize(count);
		}

		uint32_t writeIndex = 0;
		for (uint32_t i = 0; i < numBoneActors; i++)
		{
			if (boneActors[i].convexVerticesCount == 0)
			{
				PX_ASSERT(boneActors[i].capsuleRadius > 0.0f);
				if (boneActors[i].capsuleRadius > 0.0f)
				{
					const int32_t boneIndex = boneActors[i].boneIndex;
					PX_ASSERT(boneIndex >= 0);
					if (boneIndex >= 0)
					{
						const PxMat44 boneBindPose = bones[boneIndex].bindPose;
						const PxMat44& diff = boneTransforms[boneIndex];

						const PxMat44 globalPose = diff * boneBindPose * (PxMat44)boneActors[i].localPose;

						const PxVec3 vertex(0.0f, boneActors[i].capsuleHeight * 0.5f, 0.0f);
						const float radius = (boneActors[i].capsuleRadius + collisionThickness) * mActorScale;
						mCollisionSpheres[writeIndex++] = PxVec4(globalPose.transform(vertex), radius);
						mCollisionSpheres[writeIndex++] = PxVec4(globalPose.transform(-vertex), radius);
					}
				}
			}
		}
		PX_ASSERT(writeIndex == mNumAssetSpheres);
	}
	else if (numBoneSpheres > 0)
	{
		// new style

		// write physx3 bone spheres
		mNumAssetSpheres = numBoneSpheres;
		mCollisionSpheres.resize(numBoneSpheres);
		for (uint32_t i = 0; i < mCollisionSpheres.size(); ++i)
		{
			const int32_t boneIndex = boneSpheres[i].boneIndex;
			PX_ASSERT(boneIndex >= 0);

			const PxMat44 boneBindPose = bones[boneIndex].bindPose;
			const PxMat44& diff = boneTransforms[boneIndex];

			PxVec3 globalPos = diff.transform(boneBindPose.transform(boneSpheres[i].localPos));

			mCollisionSpheres[i] = PxVec4(globalPos, (boneSpheres[i].radius + collisionThickness) * mActorScale);
		}
	}

	// collision spheres from actor
	if (mReleasedSphereIds.size() > 0)
	{
		// make sure the order of id's doesn't change
		CollisionCompare compare;
		actorSpheres.sort(compare);
	}
	mCollisionSpheres.resize(mNumAssetSpheres + actorSpheres.getSize());
	for (uint32_t i = 0; i < actorSpheres.getSize(); ++i)
	{
		uint32_t sphereId = mNumAssetSpheres + i;
		ClothingSphereImpl* actorSphere = DYNAMIC_CAST(ClothingSphereImpl*)(actorSpheres.getResource(i));
		actorSphere->setId((int32_t)sphereId);
		PxVec3 pos = actorSphere->getPosition();
		if (mLocalSpaceSim)
		{
			pos = mGlobalPoseNormalizedInv.transform(pos);
		}

		PxVec4 sphere(pos, actorSphere->getRadius());
		mCollisionSpheres[sphereId] = sphere;
	}

	// collision capsules from actor
	mCollisionCapsules.resizeUninitialized(mNumAssetCapsules);
	mCollisionCapsulesInvalid.resizeUninitialized(mNumAssetCapsulesInvalid);
	for (uint32_t i = 0; i < actorCapsules.getSize(); ++i)
	{
		ClothingCapsuleImpl* actorCapsule = DYNAMIC_CAST(ClothingCapsuleImpl*)(actorCapsules.getResource(i));
		ClothingSphereImpl** spheres = (ClothingSphereImpl**)actorCapsule->getSpheres();
		uint32_t s0 = (uint32_t)spheres[0]->getId();
		uint32_t s1 = (uint32_t)spheres[1]->getId();
		if (s0 > 32 || s1 > 32)
		{
			mCollisionCapsulesInvalid.pushBack(s0);
			mCollisionCapsulesInvalid.pushBack(s1);
		}
		else
		{
			mCollisionCapsules.pushBack(s0);
			mCollisionCapsules.pushBack(s1);
		}
	}


	// collision planes of convexes
	mCollisionPlanes.resize(numBonePlanes + actorPlanes.getSize());
	for (uint32_t i = 0; i < numBonePlanes; ++i)
	{
		const int32_t boneIndex = bonePlanes[i].boneIndex;
		PX_ASSERT(boneIndex >= 0);
		if (boneIndex >= 0)
		{
			const PxMat44 boneBindPose = bones[boneIndex].bindPose;
			const PxMat44& diff = boneTransforms[boneIndex];

			PxVec3 p = diff.transform(boneBindPose.transform(bonePlanes[i].n * -bonePlanes[i].d));
			PxVec3 n = diff.rotate(boneBindPose.rotate(bonePlanes[i].n));

			PxPlane skinnedPlane(p, n);

			mCollisionPlanes[i] = PxVec4(skinnedPlane.n, skinnedPlane.d);
		}
	}


	// collision convexes and planes from actor
	mCollisionConvexes.resizeUninitialized(mNumAssetConvexes);
	mCollisionConvexesInvalid.clear();

	// planes
	if (mReleasedPlaneIds.size() > 0)
	{
		// make sure the order of id's doesn't change
		CollisionCompare compare;
		actorPlanes.sort(compare);
	}
	for (uint32_t i = 0; i < actorPlanes.getSize(); ++i)
	{
		uint32_t planeId = (uint32_t)(numBonePlanes + i);
		ClothingPlaneImpl* actorPlane = DYNAMIC_CAST(ClothingPlaneImpl*)(actorPlanes.getResource(i));
		actorPlane->setId((int32_t)planeId);
		PxPlane plane = actorPlane->getPlane();
		if (mLocalSpaceSim)
		{
			PxVec3 p = plane.pointInPlane();
			plane = PxPlane(mGlobalPoseNormalizedInv.transform(p), mGlobalPoseNormalizedInv.rotate(plane.n));
		}
		mCollisionPlanes[planeId] = PxVec4(plane.n, plane.d);

		// create a convex for unreferenced planes (otherwise they don't collide)
		if (actorPlane->getRefCount() == 0 && planeId <= 32)
		{
			mCollisionConvexes.pushBack(1u << planeId);
		}
	}

	// convexes
	for (uint32_t i = 0; i < actorConvexes.getSize(); ++i)
	{
		ClothingConvexImpl* convex = DYNAMIC_CAST(ClothingConvexImpl*)(actorConvexes.getResource(i));

		uint32_t convexMask = 0;
		ClothingPlaneImpl** planes = (ClothingPlaneImpl**)convex->getPlanes();
		for (uint32_t j = 0; j < convex->getNumPlanes(); ++j)
		{
			ClothingPlaneImpl* plane = planes[j];
			uint32_t planeId = (uint32_t)plane->getId();
			if (planeId > 32)
			{
				convexMask = 0;
				break;
			}
			convexMask |= 1 << planeId;
		}

		if (convexMask > 0)
		{
			mCollisionConvexes.pushBack(convexMask);
		}
		else
		{
			mCollisionConvexesInvalid.pushBack(convex);
		}
	}

	// triangles
	PX_ASSERT(mCollisionTrianglesOld.empty());
	nvidia::Array<PxVec3> collisionTrianglesTemp; // mCollisionTriangles is used in update, so we cannot clear it
	for (uint32_t i = 0; i < actorTriangleMeshes.getSize(); ++i)
	{
		ClothingTriangleMeshImpl* mesh = (ClothingTriangleMeshImpl*)(actorTriangleMeshes.getResource(i));

		const PxMat44& pose = mesh->getPose();
		PxTransform tm(pose);
		if (mLocalSpaceSim)
		{
			tm = PxTransform(mGlobalPoseNormalizedInv) * tm;
		}

 		mesh->update(tm, mCollisionTriangles, mCollisionTrianglesOld, collisionTrianglesTemp);
	}
	mCollisionTriangles.swap(collisionTrianglesTemp);
}



void Simulation::releaseCollision(ClothingCollisionImpl& collision)
{
	ClothingSphereImpl* sphere = DYNAMIC_CAST(ClothingSphereImpl*)(collision.isSphere());
	if (sphere != NULL)
	{
		int32_t id = sphere->getId();
		if (id != -1)
		{
			mReleasedSphereIds.pushBack((uint32_t)id);
		}
		return;
	}

	ClothingPlaneImpl* plane = DYNAMIC_CAST(ClothingPlaneImpl*)(collision.isPlane());
	if (plane != NULL)
	{
		int32_t id = plane->getId();
		if (id != -1)
		{
			mReleasedPlaneIds.pushBack((uint32_t)id);
		}
		return;
	}
}



void Simulation::updateCollisionDescs(const tActorDescTemplate& /*actorDesc*/, const tShapeDescTemplate& /*shapeDesc*/)
{
}



void Simulation::disablePhysX(Actor* /*dummy*/)
{
	PX_ASSERT(false);
}



void Simulation::reenablePhysX(Actor* /*newMaster*/, const PxMat44& /*globalPose*/)
{
	PX_ASSERT(false);
}



void Simulation::fetchResults(bool computePhysicsMeshNormals)
{
	if (mCloth != NULL)
	{
		{
			cloth::Range<PxVec4> particles = mCloth->getCurrentParticles();

			PX_ASSERT(particles.size() == sdkNumDeformableVertices);
			for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
			{
				sdkWritebackPosition[i] = particles[i].getXYZ();
				PX_ASSERT(sdkWritebackPosition[i].isFinite());
			}
		}

		// compute the normals
		if (computePhysicsMeshNormals)
		{
			memset(sdkWritebackNormal, 0, sizeof(PxVec3) * sdkNumDeformableVertices);
			for (uint32_t i = 0; i < sdkNumDeformableIndices; i += 3)
			{
				PxVec3 v1 = sdkWritebackPosition[mIndices[i + 1]] - sdkWritebackPosition[mIndices[i]];
				PxVec3 v2 = sdkWritebackPosition[mIndices[i + 2]] - sdkWritebackPosition[mIndices[i]];
				PxVec3 faceNormal = v1.cross(v2);

				for (uint32_t j = 0; j < 3; j++)
				{
					sdkWritebackNormal[mIndices[i + j]] += faceNormal;
				}
			}

			for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
			{
				sdkWritebackNormal[i].normalize();
			}
		}
	}
	else
	{
		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			sdkWritebackPosition[i] = skinnedPhysicsPositions[i];
			sdkWritebackNormal[i] = skinnedPhysicsNormals[i];
		}
	}
}




bool Simulation::isSimulationMeshDirty() const
{
	return true; // always expect something to change
}



void Simulation::clearSimulationMeshDirt()
{
}



void Simulation::setStatic(bool on)
{
	if (on)
	{
		if (mIsStatic && !mCloth->isAsleep())
		{
			APEX_INTERNAL_ERROR("Cloth has not stayed static. Something must have woken it up.");
		}
		mCloth->putToSleep();
	}
	else
	{
		mCloth->wakeUp();
	}
	mIsStatic = on;
}



bool Simulation::applyPressure(float /*pressure*/)
{
	return false;
}



bool Simulation::raycast(const PxVec3& rayOrigin, const PxVec3& rayDirection, float& _hitTime, PxVec3& _hitNormal, uint32_t& _vertexIndex)
{
	const uint32_t numIndices = sdkNumDeformableIndices;
	float hitTime = PX_MAX_F32;
	uint32_t hitIndex = 0xffffffff;
	uint32_t hitVertexIndex = 0;
	for (uint32_t i = 0; i < numIndices; i += 3)
	{
		float t = 0, u = 0, v = 0;

		if (APEX_RayTriangleIntersect(rayOrigin, rayDirection,
		                              sdkWritebackPosition[mIndices[i + 0]],
		                              sdkWritebackPosition[mIndices[i + 1]],
		                              sdkWritebackPosition[mIndices[i + 2]],
		                              t, u, v))
		{
			if (t < hitTime)
			{
				hitTime = t;
				hitIndex = i;
				float w = 1 - u - v;
				if (w >= u && w >= v)
				{
					hitVertexIndex = mIndices[i];
				}
				else if (u > w && u >= v)
				{
					hitVertexIndex = mIndices[i + 1];
				}
				else
				{
					hitVertexIndex = mIndices[i + 2];
				}
			}
		}
	}

	if (hitIndex != 0xffffffff)
	{
		_hitTime = hitTime;
		_hitNormal = PxVec3(0.0f, 1.0f, 0.0f);
		_vertexIndex = hitVertexIndex;
		return true;
	}

	return false;
}



void Simulation::attachVertexToGlobalPosition(uint32_t vertexIndex, const PxVec3& globalPosition)
{
	if (mCloth == NULL)
	{
		return;
	}

	cloth::Range<PxVec4> curParticles = mCloth->getCurrentParticles();
	cloth::Range<PxVec4> prevParticles = mCloth->getPreviousParticles();

	PX_ASSERT(vertexIndex < curParticles.size());
	PX_ASSERT(vertexIndex < prevParticles.size());

	// the .w component contains inverse mass of the vertex
	// the solver needs it set on both current and previous
	// (current contains an adjusted mass, scaled or zeroed by distance constraints)
	curParticles[vertexIndex] = PxVec4(globalPosition, 0.0f);
	prevParticles[vertexIndex].w = 0;
}



void Simulation::freeVertex(uint32_t vertexIndex)
{
	if (mCloth == NULL)
	{
		return;
	}

	const float weight = mCookedData->deformableInvVertexWeights.buf[vertexIndex];

	cloth::Range<PxVec4> curParticles = mCloth->getPreviousParticles();
	cloth::Range<PxVec4> prevParticles = mCloth->getPreviousParticles();

	PX_ASSERT(vertexIndex < curParticles.size());
	PX_ASSERT(vertexIndex < prevParticles.size());

	// the .w component contains inverse mass of the vertex
	// the solver needs it set on both current and previous
	// (current contains an adjusted mass, scaled or zeroed by distance constraints)
	curParticles[vertexIndex].w = weight;
	prevParticles[vertexIndex].w = weight;
}



void Simulation::setGlobalPose(const PxMat44& globalPose)
{
	mGlobalPosePrevious = mGlobalPose;
	mGlobalPose = mGlobalPoseNormalized = globalPose;

	mGlobalPoseNormalized.column0.normalize();
	mGlobalPoseNormalized.column1.normalize();
	mGlobalPoseNormalized.column2.normalize();

	mGlobalPoseNormalizedInv = mGlobalPoseNormalized.inverseRT();

	mTeleported = false;
}



void Simulation::applyGlobalPose()
{
	if (mCloth == NULL || mIsStatic)
	{
		return;
	}

	PxTransform pose = mLocalSpaceSim ? PxTransform(mGlobalPoseNormalized) : PxTransform(PxIdentity);

	mCloth->setTranslation(pose.p);
	mCloth->setRotation(pose.q);

	if (mTeleported)
	{
		mCloth->clearInertia();
	}
}



NvParameterized::Interface* Simulation::getCookedData()
{
	return NULL;
}



void Simulation::verifyTimeStep(float substepSize)
{
	mLastTimestep = substepSize;
}


#ifndef WITHOUT_DEBUG_VISUALIZE
void Simulation::visualizeConvexes(RenderDebugInterface& renderDebug)
{
	if(mCloth != NULL && mCollisionConvexes.size() > 0)
	{
		ConvexMeshBuilder builder(&mCollisionPlanes[0]);


		float scale = mCloth->getBoundingBoxScale().maxElement();

		for(uint32_t i=0; i<mCollisionConvexes.size(); ++i)
		{
			builder(mCollisionConvexes[i], scale);
		}

		for (uint32_t i = 0; i < builder.mIndices.size(); i += 3)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(builder.mVertices[builder.mIndices[i]], builder.mVertices[builder.mIndices[i+1]], builder.mVertices[builder.mIndices[i+2]]);
		}
	}
}



void Simulation::visualizeConvexesInvalid(RenderDebugInterface& renderDebug)
{
	// this is rather slow and unprecise
	for (uint32_t i = 0; i < mCollisionConvexesInvalid.size(); ++i)
	{
		ClothingConvexImpl* convex = mCollisionConvexesInvalid[i];
		ClothingPlaneImpl** convexPlanes = (ClothingPlaneImpl**)convex->getPlanes();
		ConvexHullImpl hull;
		hull.init();
		Array<PxPlane> planes;
		for (uint32_t j = 0; j < convex->getNumPlanes(); ++j)
		{
			PxPlane plane = convexPlanes[j]->getPlane();
			if (mLocalSpaceSim)
			{
				PxVec3 p = plane.pointInPlane();
				plane = PxPlane(mGlobalPoseNormalizedInv.transform(p), mGlobalPoseNormalizedInv.rotate(plane.n));
			}
			planes.pushBack(plane);
		}

		hull.buildFromPlanes(planes.begin(), planes.size(), 0.1f);

		// TODO render triangles (or polygons)
		for (uint32_t j = 0; j < hull.getEdgeCount(); j++)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(hull.getVertex(hull.getEdgeEndpointIndex(j, 0)), hull.getVertex(hull.getEdgeEndpointIndex(j, 1)));
		}

		if (hull.getEdgeCount() == 0)
		{
			float planeSize = mCloth ? mCloth->getBoundingBoxScale().maxElement() * 0.3f : 1.0f;
			for (uint32_t j = 0; j < planes.size(); ++j)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->debugPlane(PxPlane(planes[j].n, planes[j].d), planeSize, planeSize);
			}
		}
	}
}
#endif



void Simulation::visualize(RenderDebugInterface& renderDebug, ClothingDebugRenderParams& clothingDebugParams)
{
#ifdef WITHOUT_DEBUG_VISUALIZE
	PX_UNUSED(renderDebug);
	PX_UNUSED(clothingDebugParams);
#else
	if (!clothingDebugParams.Actors)
	{
		return;
	}

	using RENDER_DEBUG::DebugColors;
	using RENDER_DEBUG::DebugRenderState;
	const PxMat44 globalPose = *RENDER_DEBUG_IFACE(&renderDebug)->getPoseTyped();

	if (clothingDebugParams.CollisionShapes || clothingDebugParams.CollisionShapesWire)
	{
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

		// Wireframe only when solid is not set, when both are on, just do the solid thing
		if (!clothingDebugParams.CollisionShapes)
		{
			RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidShaded);
			RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidWireShaded);
		}
		else
		{
			RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(DebugRenderState::SolidShaded);
			RENDER_DEBUG_IFACE(&renderDebug)->removeFromCurrentState(DebugRenderState::SolidWireShaded);
		}

		const uint32_t colorLightGray = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::LightGray);
		const uint32_t colorGray = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Gray);
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);

		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorLightGray);

		PX_ALLOCA(usedSpheres, bool, mCollisionSpheres.size());
		for (uint32_t i = 0; i < mCollisionSpheres.size(); i++)
		{
			usedSpheres[i] = false;
		}

		const uint32_t numIndices1 = mCollisionCapsules.size();
		const uint32_t numIndices2 = mCollisionCapsulesInvalid.size();
		const uint32_t numIndices = numIndices2 + numIndices1;
		for (uint32_t i = 0; i < numIndices; i += 2)
		{
			const bool valid = i < numIndices1;
			const uint32_t index1 = valid ? mCollisionCapsules[i + 0] : mCollisionCapsulesInvalid[i + 0 - numIndices1];
			const uint32_t index2 = valid ? mCollisionCapsules[i + 1] : mCollisionCapsulesInvalid[i + 1 - numIndices1];

			RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(valid ? colorLightGray : colorRed);

			PxVec3 pos1 = mCollisionSpheres[index1].getXYZ();
			PxVec3 pos2 = mCollisionSpheres[index2].getXYZ();

			PxVec3 capsuleAxis = pos1 - pos2;
			const float axisHeight = capsuleAxis.normalize();

			PxMat44 capsulePose;
			{
				// construct matrix from this
				const PxVec3 capsuleDefaultAxis(0.0f, 1.0f, 0.0f);
				PxVec3 axis = capsuleDefaultAxis.cross(capsuleAxis).getNormalized();
				const float angle = PxAcos(capsuleDefaultAxis.dot(capsuleAxis));
				if (angle < 0.001f || angle + 0.001 > PxPi || axis.isZero())
				{
					axis = PxVec3(0.0f, 1.0f, 0.0f);
				}
				PxQuat q(angle, axis);
				capsulePose = PxMat44(q);
				capsulePose.setPosition((pos1 + pos2) * 0.5f);
			}

			const float radius1 = mCollisionSpheres[index1].w;
			const float radius2 = mCollisionSpheres[index2].w;

			RENDER_DEBUG_IFACE(&renderDebug)->setPose(globalPose * capsulePose);
			RENDER_DEBUG_IFACE(&renderDebug)->debugCapsuleTapered(radius1, radius2, axisHeight, 2);

			usedSpheres[index1] = true;
			usedSpheres[index2] = true;
		}

		for (uint32_t i = 0; i < mCollisionSpheres.size(); i++)
		{
			if (!usedSpheres[i])
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(i < 32 ? colorGray : colorRed);
				RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(mCollisionSpheres[i].getXYZ(), mCollisionSpheres[i].w);
			}
		}
		RENDER_DEBUG_IFACE(&renderDebug)->setPose(globalPose);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorLightGray);
		visualizeConvexes(renderDebug);
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
		visualizeConvexesInvalid(renderDebug);

		// collision triangles
		PX_ASSERT(mCollisionTriangles.size() % 3 == 0);
		uint32_t numTriangleVertsInCloth = mCloth ? 3*mCloth->getNumTriangles() : mCollisionTriangles.size();
		for (uint32_t i = 0; i < mCollisionTriangles.size(); i += 3)
		{
			if (i < numTriangleVertsInCloth)
			{
				// only 500 triangles simulated in cuda
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorLightGray);
			}
			else
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
			}
			RENDER_DEBUG_IFACE(&renderDebug)->debugTri(mCollisionTriangles[i + 0], mCollisionTriangles[i + 1], mCollisionTriangles[i + 2]);
		}

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}

	if (clothingDebugParams.LengthFibers ||
	        clothingDebugParams.CrossSectionFibers ||
	        clothingDebugParams.BendingFibers ||
	        clothingDebugParams.ShearingFibers)
	{
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);

		for (uint32_t pc = 0; pc < mPhaseConfigs.size(); ++pc)
		{
			const uint32_t phaseIndex = mPhaseConfigs[pc].mPhaseIndex;
			PX_ASSERT(phaseIndex < (uint32_t)mCookedData->deformablePhaseDescs.arraySizes[0]);

			const uint32_t setIndex = mCookedData->deformablePhaseDescs.buf[phaseIndex].setIndex;

			const PxClothFabricPhaseType::Enum type = (PxClothFabricPhaseType::Enum)mCookedData->deformablePhaseDescs.buf[phaseIndex].phaseType;

			float stretchRangeMultiplier = mPhaseConfigs[pc].mStretchLimit;
			float compressionRangeMultiplier = mPhaseConfigs[pc].mCompressionLimit;

			float stiffnessScale = mPhaseConfigs[pc].mStiffnessMultiplier;
			uint8_t brightness = (uint8_t)(64 * stiffnessScale + 64);
			if (stiffnessScale == 1.f)
			{
				brightness = 255;
			}
			else if (stiffnessScale == 0.f)
			{
				brightness = 0;
			}
			uint32_t rangeColor = uint32_t(brightness | (brightness << 8) | (brightness << 16));
			uint32_t stretchRangeColor		= rangeColor;
			uint32_t compressionRangeColor = rangeColor;
			if (stretchRangeMultiplier > 1.f)
			{
				// red
				rangeColor |= 0xFF << 16;
			}
			else if (compressionRangeMultiplier < 1.f)
			{
				// blue
				rangeColor |= 0xFF << 0;
			}
			if (stiffnessScale == 1)
			{
				rangeColor = 0xFFFFFF;
			}

			bool ok = false;
			ok |= clothingDebugParams.LengthFibers && type == PxClothFabricPhaseType::eVERTICAL;
			ok |= clothingDebugParams.CrossSectionFibers && type == PxClothFabricPhaseType::eHORIZONTAL;
			ok |= clothingDebugParams.BendingFibers && type == PxClothFabricPhaseType::eBENDING;
			ok |= clothingDebugParams.ShearingFibers && type == PxClothFabricPhaseType::eSHEARING;

			if (ok)
			{
				const uint32_t fromIndex	= setIndex ? mCookedData->deformableSets.buf[setIndex - 1].fiberEnd : 0;
				const uint32_t toIndex		= mCookedData->deformableSets.buf[setIndex].fiberEnd;

				if ((int32_t)toIndex > mCookedData->deformableIndices.arraySizes[0])
				{
					break;
				}

				for (uint32_t f = fromIndex; f < toIndex; ++f)
				{
					uint32_t	posIndex1	= mCookedData->deformableIndices.buf[2 * f];
					uint32_t	posIndex2	= mCookedData->deformableIndices.buf[2 * f + 1]; 

					PX_ASSERT((int32_t)posIndex2 <= mCookedData->deformableIndices.arraySizes[0]);
					PX_ASSERT(mCookedData->deformableIndices.buf[posIndex1] < sdkNumDeformableVertices);

					PxVec3	pos1		= sdkWritebackPosition[posIndex1];
					PxVec3	pos2		= sdkWritebackPosition[posIndex2];

					const float restLength	= mCookedData->deformableRestLengths.buf[f] * simulation.restLengthScale;
					PxVec3 dir				= pos2 - pos1;
					PxVec3 middle			= pos1 + 0.5f * dir;
					const float simLength	= dir.normalize();
					PxVec3 edge				= dir * restLength;
					PxVec3 e1				= middle - 0.5f * edge;
					PxVec3 e2				= middle + 0.5f * edge;

					if (clothingDebugParams.FiberRange && type != PxClothFabricPhaseType::eBENDING)
					{
						PxVec3 stretchRangeOffset		= edge;
						PxVec3 compressionRangeOffset	= edge;

						if (stretchRangeMultiplier > 1.f)
						{
							stretchRangeOffset *= 0.5f * (1.0f - stretchRangeMultiplier);

							RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(stretchRangeColor);
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e1, e1 + stretchRangeOffset);
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e2, e2 - stretchRangeOffset);
						}
						
						if (compressionRangeMultiplier < 1.f)
						{
							compressionRangeOffset *= 0.5f * (1.0f - compressionRangeMultiplier);

							RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(compressionRangeColor);
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e1, e1 + compressionRangeOffset);
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e2, e2 - compressionRangeOffset);
						}

						RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(0xFFFFFFFF);
						RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(pos1, 0.01f);
						RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(pos2, 0.01f);
						if (compressionRangeMultiplier < 1.0f)
						{
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e1 + compressionRangeOffset, e2 - compressionRangeOffset);
						}
						else
						{
							RENDER_DEBUG_IFACE(&renderDebug)->debugLine(e1, e2);
						}
					}
					else
					{
						if (simLength < restLength || type == PxClothFabricPhaseType::eBENDING)
						{
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(pos1, pos2, colorGreen, colorGreen);
						}
						else
						{
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(pos1, e1, colorRed, colorRed);
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(e1, e2, colorGreen, colorGreen);
							RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(e2, pos2, colorRed, colorRed);
						}
					}
				}
			}
		}
	}

	if (clothingDebugParams.TethersActive || clothingDebugParams.TethersInactive)
	{
		const uint32_t colorDarkBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Blue);
		const uint32_t colorLightBlue = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::LightBlue);
		const uint32_t colorGreen = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Green);
		const uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);

		const uint32_t numTetherAnchors = (uint32_t)mCookedData->tetherAnchors.arraySizes[0];
		for (uint32_t i = 0; i < numTetherAnchors; ++i)
		{
			uint32_t anchorIndex = mCookedData->tetherAnchors.buf[i];
			PX_ASSERT(anchorIndex < sdkNumDeformableVertices);
			const PxVec3 p1 = sdkWritebackPosition[anchorIndex];
			const PxVec3 p2 = sdkWritebackPosition[i % sdkNumDeformableVertices];
			PxVec3 dir = p2 - p1;
			const float d = dir.normalize();
			const float tetherLength = mCookedData->tetherLengths.buf[i];

			if (d < tetherLength)
			{
				if (d < tetherLength * 0.99)
				{
					if (clothingDebugParams.TethersInactive)
					{
						RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorDarkBlue);
						RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p1, p2);
					}
				}
				else if (clothingDebugParams.TethersActive)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorLightBlue);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p1, p2);
				}
			}
			else if (clothingDebugParams.TethersActive)
			{
				const PxVec3 p = p1 + tetherLength * dir;
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorLightBlue);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p1, p);
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorGreen);
				const PxVec3 p_ = p1 + dir * PxMin(tetherLength * mTetherLimit, d);
				RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p, p_);

				if (d > tetherLength * mTetherLimit)
				{
					RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);
					RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p_, p2);
				}
			}
		}
	}

	if (clothingDebugParams.MassScale && mCloth != NULL && mCloth->getCollisionMassScale() > 0.0f)
	{
		cloth::Range<const PxVec4> curParticles = mCloth->getCurrentParticles();
		cloth::Range<const PxVec4> prevParticles = mCloth->getPreviousParticles();

		uint32_t colorRed = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Red);

		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
		RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(colorRed);

		// draw a point anywhere the mass difference between cur and prev is non-zero
		for (uint32_t i = 0; i < curParticles.size(); ++i)
		{
			float curInvMass = curParticles[i][3];
			float prevInvMass = prevParticles[i][3];
			float massDelta = curInvMass - prevInvMass;

			// ignore prevInvMass of 0.0f because it is probably a motion constraint
			if (massDelta > 0.0f && prevInvMass > 0.0f)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->debugPoint(PxVec3(curParticles[i][0], curParticles[i][1], curParticles[i][2]), massDelta * 10.0f);
			}
		}

		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}

	if (clothingDebugParams.VirtualCollision)
	{
		uint32_t colorParticle = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::Gold);
		uint32_t colorVertex = RENDER_DEBUG_IFACE(&renderDebug)->getDebugColor(DebugColors::White);

		const uint32_t numVirtualParticleIndices = (uint32_t)mCookedData->virtualParticleIndices.arraySizes[0];
		for (uint32_t i = 0; i < numVirtualParticleIndices; i += 4)
		{
			const PxVec3 positions[3] =
			{
				sdkWritebackPosition[mCookedData->virtualParticleIndices.buf[i + 0]],
				sdkWritebackPosition[mCookedData->virtualParticleIndices.buf[i + 1]],
				sdkWritebackPosition[mCookedData->virtualParticleIndices.buf[i + 2]],
			};

			const uint32_t weightIndex = mCookedData->virtualParticleIndices.buf[i + 3];

			PxVec3 particlePos(0.0f);

			uint32_t colors[3] =
			{
				colorVertex,
				colorVertex,
				colorVertex,
			};

			for (uint32_t j = 0; j < 3; j++)
			{
				const float weight = mCookedData->virtualParticleWeights.buf[3 * weightIndex + j];
				particlePos += weight * positions[j];

				uint8_t* colorParts = (uint8_t*)(colors + j);
				for (uint32_t k = 0; k < 4; k++)
				{
					colorParts[k] = (uint8_t)(weight * colorParts[k]);
				}
			}

			for (uint32_t j = 0; j < 3; j++)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->debugGradientLine(particlePos, positions[j], colorParticle, colors[j]);
			}
		}
	}

	ModuleClothingImpl* module = static_cast<ModuleClothingImpl*>(mClothingScene->getModule());
	if (clothingDebugParams.SelfCollision && module->useSparseSelfCollision())
	{
		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();
		RENDER_DEBUG_IFACE(&renderDebug)->addToCurrentState(DebugRenderState::SolidShaded);

		const PxVec3* const positions = sdkWritebackPosition;
		uint32_t* indices = mCookedData->selfCollisionIndices.buf;
		uint32_t numIndices = (uint32_t)mCookedData->selfCollisionIndices.arraySizes[0];

		PxMat44 pose = PxMat44(PxIdentity);
		for (uint32_t i = 0; i < numIndices; ++i)
		{
			uint32_t index = indices[i];
			pose.setPosition(positions[index]);
			RENDER_DEBUG_IFACE(&renderDebug)->debugSphere(pose.getPosition(), 0.5f * mCloth->getSelfCollisionDistance());
		}
		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}

	if (clothingDebugParams.SelfCollisionAttenuation > 0.0f)
	{
		createAttenuationData();

		RENDER_DEBUG_IFACE(&renderDebug)->pushRenderState();

		for (uint32_t i = 0; i < mSelfCollisionAttenuationPairs.size(); i += 2)
		{
			float val = mSelfCollisionAttenuationValues[i/2];
			PX_ASSERT(val <= 1.0f);

			if (val > clothingDebugParams.SelfCollisionAttenuation)
				continue;

			uint8_t c = (uint8_t)(UINT8_MAX * val);
			if (val == 0.0f)
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(0x000000FF);
			}
			else
			{
				RENDER_DEBUG_IFACE(&renderDebug)->setCurrentColor(uint32_t(c << 16 | c << 8 | c));
			}
			PxVec3 p0 = sdkWritebackPosition[mSelfCollisionAttenuationPairs[i]];
			PxVec3 p1 = sdkWritebackPosition[mSelfCollisionAttenuationPairs[i+1]];
			RENDER_DEBUG_IFACE(&renderDebug)->debugLine(p0, p1);
		}
		
		RENDER_DEBUG_IFACE(&renderDebug)->popRenderState();
	}
#endif // WITHOUT_DEBUG_VISUALIZE
}


#ifndef WITHOUT_DEBUG_VISUALIZE
// solver logic is replicated here
void Simulation::createAttenuationData()
{
	if (mSelfCollisionAttenuationPairs.size() > 0)
		return;
	
	PX_ASSERT(mSelfCollisionAttenuationValues.size() == 0);

	float collD2 = mCloth->getSelfCollisionDistance();
	collD2 = collD2 * collD2;

	// it's just debug rendering, n^2 probably won't hurt us here
	for (uint32_t i = 0; i < sdkNumDeformableVertices; ++i)
	{
		for (uint32_t j = i+1; j < sdkNumDeformableVertices; ++j)
		{
			float restD2 = (mRestPositions[j] - mRestPositions[i]).magnitudeSquared();
			if (restD2 < collD2)
			{
				// closer than rest distance. pair is ignored by selfcollision
				mSelfCollisionAttenuationPairs.pushBack(PxMin(i,j));
				mSelfCollisionAttenuationPairs.pushBack(PxMax(i,j));
				mSelfCollisionAttenuationValues.pushBack(0.0f);
			}
			else if(restD2 < 4*collD2)
			{
				// within the doubled rest distance. selfcollision stiffness is attenuated
				mSelfCollisionAttenuationPairs.pushBack(PxMin(i,j));
				mSelfCollisionAttenuationPairs.pushBack(PxMax(i,j));

				float ratio = sqrtf(restD2 / collD2) - 1.0f;
				mSelfCollisionAttenuationValues.pushBack(ratio);
			}
		}
	}
}
#endif



#ifndef WITHOUT_PVD
void Simulation::updatePvd(pvdsdk::PvdDataStream& /*pvdStream*/, pvdsdk::PvdUserRenderer& pvdRenderer, ApexResourceInterface* clothingActor, bool localSpaceSim)
{
	// update rendering
	pvdRenderer.setInstanceId(clothingActor);

	PX_ASSERT(sdkNumDeformableIndices%3 == 0);
	uint32_t numTriangles = sdkNumDeformableIndices/3;
	pvdsdk::PvdDebugTriangle* pvdTriangles = (pvdsdk::PvdDebugTriangle*)GetInternalApexSDK()->getTempMemory(numTriangles * sizeof(pvdsdk::PvdDebugTriangle));
	uint32_t color = (uint32_t)RENDER_DEBUG::DebugColors::Yellow;
	for (uint32_t i = 0; i < numTriangles; ++i)
	{
		if (localSpaceSim)
		{
			pvdTriangles[i].pos0 = mGlobalPose.transform(sdkWritebackPosition[mIndices[3*i+0]]);
			pvdTriangles[i].pos1 = mGlobalPose.transform(sdkWritebackPosition[mIndices[3*i+1]]);
			pvdTriangles[i].pos2 = mGlobalPose.transform(sdkWritebackPosition[mIndices[3*i+2]]);
		}
		else
		{
			pvdTriangles[i].pos0 = sdkWritebackPosition[mIndices[3*i+0]];
			pvdTriangles[i].pos1 = sdkWritebackPosition[mIndices[3*i+1]];
			pvdTriangles[i].pos2 = sdkWritebackPosition[mIndices[3*i+2]];
		}

		pvdTriangles[i].color0 = color;
		pvdTriangles[i].color1 = color;
		pvdTriangles[i].color2 = color;
	}
	pvdRenderer.drawTriangles(pvdTriangles, numTriangles);
	GetInternalApexSDK()->releaseTempMemory(pvdTriangles);
}
#endif



GpuSimMemType::Enum Simulation::getGpuSimMemType() const
{
	// should we remove this?
	/*
	if (mUseCuda && mCloth != NULL)
	{
		cloth::Solver* solver = mClothingScene->getClothSolver(mUseCuda);
		if (solver != NULL)
		{
			uint32_t numSharedPos = solver->getNumSharedPositions(mCloth);
			GpuSimMemType::Enum type = (GpuSimMemType::Enum)numSharedPos;
			return type;
		}
	}
	*/
	return GpuSimMemType::UNDEFINED;
}



void Simulation::setPositions(PxVec3* /*positions*/)
{
	PX_ALWAYS_ASSERT();
	// not necessary for now, maybe when supporting physics LOD
}



void Simulation::setConstrainCoefficients(const tConstrainCoeffs* assetCoeffs, float maxDistanceBias, float maxDistanceScale, float /*maxDistanceDeform*/, float /*actorScale*/)
{
	if (mCloth == NULL)
	{
		return;
	}

	// Note: the spherical constraint distances are only computed here. They get set in the updateConstrainPositions method
	//       The reason for this is that it doesn't behave well when being set twice. Also skinnedPhysicsPositions are not
	//       always initialized when this method is called!

	PX_ASSERT(mConstrainCoeffs == NULL || mConstrainCoeffs == assetCoeffs);
	mConstrainCoeffs = assetCoeffs;

	// Note: maxDistanceScale already has actorScale included...

	mMotionConstrainBias = -maxDistanceBias;
	mMotionConstrainScale = maxDistanceScale;

	if (mNumBackstopConstraints == -1)
	{
		mNumBackstopConstraints = 0;
		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			mNumBackstopConstraints += assetCoeffs[i].collisionSphereRadius > 0.0f ? 1 : 0;
		}
	}

	if (mConstrainConstants.size() != sdkNumDeformableVertices)
	{
		mConstrainConstants.resize(sdkNumDeformableVertices, ConstrainConstants());

		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			mConstrainConstants[i].motionConstrainDistance = PxMax(0.0f, assetCoeffs[i].maxDistance);
			mConstrainConstants[i].backstopDistance = PxMax(0.0f, assetCoeffs[i].collisionSphereDistance) + assetCoeffs[i].collisionSphereRadius;
			mConstrainConstants[i].backstopRadius = assetCoeffs[i].collisionSphereRadius;
		}

		mConstrainConstantsDirty = true;
	}
}



void Simulation::getVelocities(PxVec3* velocities) const
{
	if (mCloth == NULL)
	{
		return;
	}

	PX_PROFILE_ZONE("SimulationPxCloth::getVelocities", GetInternalApexSDK()->getContextId());

	PX_ALIGN(16, PxMat44 oldFrameDiff) = PxMat44(PxIdentity);
	
	bool useOldFrame = false;
	if (mGlobalPose != mGlobalPosePrevious && mLocalSpaceSim && mLastTimestep > 0.0f)
	{
		oldFrameDiff = mGlobalPosePrevious;
		oldFrameDiff.column0.normalize();
		oldFrameDiff.column1.normalize();
		oldFrameDiff.column2.normalize();
		const float w = mCloth->getPreviousIterationDt() / mLastTimestep;
		oldFrameDiff = interpolateMatrix(w, oldFrameDiff, mGlobalPoseNormalized);
		oldFrameDiff = mGlobalPoseNormalized.inverseRT() * oldFrameDiff;
		oldFrameDiff.column3[3] = 0.f; //V3StoreU internally check that vec3 has zero at index 3
		useOldFrame = true;
	}

	const float previousIterDt = mCloth->getPreviousIterationDt();
	const float invTimeStep = previousIterDt > 0.0f ? 1.0f / previousIterDt : 0.0f;

	const cloth::Range<PxVec4> newPositions = mCloth->getCurrentParticles();
	const cloth::Range<PxVec4> oldPositions = mCloth->getPreviousParticles();

	if (useOldFrame)
	{
		// use SIMD code only here, it was slower for the non-matrix-multiply codepath :(

		// In localspace (and if the localspace has changed, i.e. frameDiff != ID) the previous positions are in a
		// different frame, interpolated for each iteration. We need to generate that interpolated frame (20 lines above)
		// and then apply the diff to the previous positions to move them into the same frame as the current positions.
		// This is the same frame as we refer to 'current local space'.
		using namespace physx::shdfnd::aos;
		const Vec3V invTime = V3Load(invTimeStep);
		PX_ASSERT(((size_t)(&newPositions[0].x) & 0xf) == 0); // 16 byte aligned?
		PX_ASSERT(((size_t)(&oldPositions[0].x) & 0xf) == 0); // 16 byte aligned?
		const Mat34V frameDiff = (Mat34V&)oldFrameDiff;

		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			const Vec3V newPos = Vec3V_From_Vec4V(V4LoadA(&newPositions[i].x));
			const Vec3V oldPos = Vec3V_From_Vec4V(V4LoadA(&oldPositions[i].x));
			const Vec3V oldPosReal = M34MulV3(frameDiff, oldPos);

			const Vec3V velocity = V3Mul(V3Sub(newPos, oldPosReal), invTime);
			V3StoreU(velocity, velocities[i]);
		}
	}
	else
	{
		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			const PxVec3& newPos = PxVec3(newPositions[i].x, newPositions[i].y, newPositions[i].z);
			const PxVec3& oldPos = PxVec3(oldPositions[i].x, oldPositions[i].y, oldPositions[i].z);

			PxVec3 d = newPos - oldPos;
			velocities[i] = d * invTimeStep;
		}
	}

	// no unmap since we only read
}



void Simulation::setVelocities(PxVec3* velocities)
{
	if (mCloth == NULL || mIsStatic)
	{
		return;
	}

	PX_PROFILE_ZONE("ClothingActorImpl::setVelocities", GetInternalApexSDK()->getContextId());

	const float timeStep = mCloth->getPreviousIterationDt();

	cloth::Range<PxVec4> newPositions = mCloth->getCurrentParticles();
	cloth::Range<PxVec4> oldPositions = mCloth->getPreviousParticles(); // read the data, the .w is vital!

	// assuming the weights are still up to date!

	PX_ALIGN(16, PxMat44 oldFrameDiff) = PxMat44(PxIdentity);
	bool useOldFrame = false;
	if (mGlobalPose != mGlobalPosePrevious && mLocalSpaceSim)
	{
		oldFrameDiff = mGlobalPosePrevious;
		oldFrameDiff.column0.normalize();
		oldFrameDiff.column1.normalize();
		oldFrameDiff.column2.normalize();
		const float w = mCloth->getPreviousIterationDt() / mLastTimestep;
		oldFrameDiff = interpolateMatrix(w, oldFrameDiff, mGlobalPoseNormalized);
		oldFrameDiff = oldFrameDiff.inverseRT() * mGlobalPoseNormalized;
		useOldFrame = true;
	}

	if (useOldFrame)
	{
		using namespace physx::shdfnd::aos;

		const Vec3V time = V3Load(timeStep);

		PX_ASSERT(((size_t)(&newPositions[0].x) & 0xf) == 0); // 16 byte aligned?
		PX_ASSERT(((size_t)(&oldPositions[0].x) & 0xf) == 0); // 16 byte aligned?
		const Mat34V frameDiff = (Mat34V&)oldFrameDiff;
		BoolV mask = BTTTF();

		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			const Vec3V velocity = V3LoadU(velocities[i]);
			const Vec4V newPos = V4LoadA(&newPositions[i].x);
			const Vec4V oldWeight = V4Load(oldPositions[i].w);
			const Vec3V oldPosReal = V3NegMulSub(velocity, time, Vec3V_From_Vec4V(newPos)); // newPos - velocity * time
			const Vec3V oldPos = M34MulV3(frameDiff, oldPosReal);
			const Vec4V oldPosOut = V4Sel(mask, Vec4V_From_Vec3V(oldPos), oldWeight);

			aos::V4StoreA(oldPosOut, &oldPositions[i].x);
		}
	}
	else
	{
		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			PxVec3* oldPos = (PxVec3*)(oldPositions.begin() + i);
			const PxVec3* const newPos = (const PxVec3 * const)(newPositions.begin() + i);
			*oldPos = *newPos - velocities[i] * timeStep;
		}
	}
}



bool Simulation::applyWind(PxVec3* velocities, const PxVec3* normals, const tConstrainCoeffs* coeffs, const PxVec3& wind, float adaption, float /*dt*/)
{
	if (mCloth == NULL || mIsStatic)
	{
		return false;
	}

	// here we leave velocities untouched

	if (adaption > 0.0f)
	{
		cloth::Range<PxVec4> accelerations = mCloth->getParticleAccelerations();
		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			PxVec3 velocity = velocities[i];
			PxVec3 dv = wind - velocity;

			if (coeffs[i].maxDistance > 0.0f && !dv.isZero())
			{
				// scale the wind depending on angle
				PxVec3 normalizedDv = dv;
				normalizedDv *= ModuleClothingHelpers::invSqrt(normalizedDv.magnitudeSquared());
				const float dot = normalizedDv.dot(normals[i]);
				dv *= PxMin(1.0f, PxAbs(dot) * adaption); // factor should not exceed 1.0f

				// We set the acceleration such that we get
				// end velocity = velocity + (wind - velocity) * dot * adaption * dt.
				// using
				// end velocity = velocity + acceleration * dt
				accelerations[i] = PxVec4(dv, 0.0f);
			}
			else
			{
				accelerations[i].setZero();
			}
		}
	}
	else
	{
		mCloth->clearParticleAccelerations();
	}

	return false;
}


void Simulation::setTeleportWeight(float weight, bool reset, bool localSpaceSim)
{
	if (mCloth != NULL && weight > 0.0f && !mIsStatic)
	{
		mTeleported = true;

		if (reset)
		{
			cloth::Range<PxVec4> curPos = mCloth->getCurrentParticles();
			cloth::Range<PxVec4> prevPos = mCloth->getPreviousParticles();

			const uint32_t numParticles = (uint32_t)curPos.size();
			for (uint32_t i = 0; i < numParticles; i++)
			{
				curPos[i] = PxVec4(skinnedPhysicsPositions[i], curPos[i].w);
				prevPos[i] = PxVec4(skinnedPhysicsPositions[i], prevPos[i].w);
			}
			mCloth->clearParticleAccelerations();
		}
		else if (!localSpaceSim)
		{
			cloth::Range<PxVec4> curPos = mCloth->getCurrentParticles();
			cloth::Range<PxVec4> prevPos = mCloth->getPreviousParticles();

			const uint32_t numParticles = (uint32_t)curPos.size();

			PxMat44 globalPosePreviousNormalized = mGlobalPosePrevious;
			globalPosePreviousNormalized.column0.normalize();
			globalPosePreviousNormalized.column1.normalize();
			globalPosePreviousNormalized.column2.normalize();

			const PxMat44 realTransform = mGlobalPoseNormalized * globalPosePreviousNormalized.inverseRT();

			for (uint32_t i = 0; i < numParticles; i++)
			{
				curPos[i] = PxVec4(realTransform.transform(sdkWritebackPosition[i]), curPos[i].w);
				prevPos[i] = PxVec4(realTransform.transform(prevPos[i].getXYZ()), prevPos[i].w);
			}
		}
	}

	mLocalSpaceSim = localSpaceSim;
}



void Simulation::setSolverIterations(uint32_t /*iterations*/)
{
	/*
	if (mCloth != NULL)
	{
		mSolverIterationsPerSecond = iterations * 50.0f;
		mCloth->setSolverFrequency(mSolverIterationsPerSecond);
	}
	*/
}



void Simulation::updateConstrainPositions(bool isDirty)
{
	if (mCloth == NULL || mIsStatic)
	{
		return;
	}

	PX_ASSERT(mConstrainCoeffs != NULL); // guarantees that setConstrainCoefficients has been called before!

	if (mConstrainConstantsDirty || isDirty)
	{
		if (mTeleported)
		{
			mCloth->clearMotionConstraints();
		}
		cloth::Range<PxVec4> sphericalConstraints = mCloth->getMotionConstraints();

		PX_ASSERT(sphericalConstraints.size() == sdkNumDeformableVertices);

		for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
		{
			// we also must write the .w component to be sure everything works!
			sphericalConstraints[i] = PxVec4(skinnedPhysicsPositions[i], mConstrainConstants[i].motionConstrainDistance);
		}

		if (mNumBackstopConstraints > 0)
		{
			if (mTeleported)
			{
				mCloth->clearSeparationConstraints();
			}
			cloth::Range<PxVec4> backstopConstraints = mCloth->getSeparationConstraints();

			for (uint32_t i = 0; i < sdkNumDeformableVertices; i++)
			{
				backstopConstraints[i] = PxVec4(skinnedPhysicsPositions[i] - mConstrainConstants[i].backstopDistance * skinnedPhysicsNormals[i], mConstrainConstants[i].backstopRadius);
			}
		}

		mConstrainConstantsDirty = false;
	}

	mCloth->setMotionConstraintScaleBias(mMotionConstrainScale, mMotionConstrainBias);
}



bool Simulation::applyClothingMaterial(tMaterial* material, PxVec3 scaledGravity)
{
	if (mCloth == NULL || material == NULL || mIsStatic)
	{
		return false;
	}

	// solver iterations
	mCloth->setSolverFrequency(material->solverFrequency);

	// filter window for handling dynamic timesteps. smooth over 2s.
	mCloth->setAcceleationFilterWidth(2 * (uint32_t)material->solverFrequency);

	// damping scale is here to remove the influence of the stiffness frequency from all damping values
	// (or to be more precise, to use 10 as a stiffness frequency)
	const float dampingStiffnessFrequency = 10.0f;
	const float exponentDamping = dampingStiffnessFrequency / material->stiffnessFrequency * physx::shdfnd::log2(1 - material->damping);
	const float exponentDrag = dampingStiffnessFrequency / material->stiffnessFrequency * physx::shdfnd::log2(1 - material->drag);
	const float newDamping = 1.0f - ::expf(exponentDamping * 0.693147180559945309417f); // exp -> exp2, 0.69 = ln(2)
	const float newDrag = 1.0f - ::expf(exponentDrag * 0.693147180559945309417f); // exp -> exp2

	// damping
	// TODO damping as vector
	mCloth->setDamping(PxVec3(newDamping));

	mCloth->setStiffnessFrequency(material->stiffnessFrequency);

	// drag
	// TODO expose linear and angular drag separately
	mCloth->setLinearDrag(PxVec3(newDrag));
	mCloth->setAngularDrag(PxVec3(newDrag));

	// friction
	mCloth->setFriction(material->friction);

	// gravity
	PxVec3 gravity;
	gravity[0] = scaledGravity.x * material->gravityScale;
	gravity[1] = scaledGravity.y * material->gravityScale;
	gravity[2] = scaledGravity.z * material->gravityScale;
	mScaledGravity = scaledGravity * material->gravityScale;
	mCloth->setGravity(mScaledGravity);

	// inertia scale
	// TODO expose linear and angular inertia separately
	mCloth->setLinearInertia(PxVec3(material->inertiaScale));
	mCloth->setAngularInertia(PxVec3(material->inertiaScale));

	// mass scale
	mCloth->setCollisionMassScale(material->massScale);

	// tether settings
	mCloth->setTetherConstraintScale(material->tetherLimit);
	mCloth->setTetherConstraintStiffness(material->tetherStiffness);


	// remember for debug rendering
	mTetherLimit = material->tetherLimit;

	// self collision
	// clear debug render data if it's not needed, or stale
	if(mClothingScene->getDebugRenderParams()->SelfCollisionAttenuation == 0.0f || material->selfcollisionThickness * mActorScale != mCloth->getSelfCollisionDistance())
	{
		mSelfCollisionAttenuationPairs.clear();
		mSelfCollisionAttenuationValues.clear();
	}

	if (	(mCloth->getSelfCollisionDistance() == 0.0f || mCloth->getSelfCollisionStiffness() == 0.0f)
		&&	(material->selfcollisionThickness * mActorScale > 0.0f && material->selfcollisionStiffness > 0.0)
		)
	{
		// turning on
		setRestPositions(true);
	}
	else if(	(mCloth->getSelfCollisionDistance() > 0.0f && mCloth->getSelfCollisionStiffness() > 0.0f)
		&&	(material->selfcollisionThickness * mActorScale == 0.0f || material->selfcollisionStiffness == 0.0)
		)
	{
		// turning off
		setRestPositions(false);
	}
	mCloth->setSelfCollisionDistance(material->selfcollisionThickness * mActorScale);
	mCloth->setSelfCollisionStiffness(material->selfcollisionStiffness);

	for (uint32_t i = 0; i < mPhaseConfigs.size(); i++)
	{
		PxClothFabricPhaseType::Enum phaseType = (PxClothFabricPhaseType::Enum)mCookedData->deformablePhaseDescs.buf[mPhaseConfigs[i].mPhaseIndex].phaseType;

		if (phaseType == PxClothFabricPhaseType::eVERTICAL)
		{
			mPhaseConfigs[i].mStiffness = material->verticalStretchingStiffness;
			mPhaseConfigs[i].mStiffnessMultiplier = material->verticalStiffnessScaling.scale;
			mPhaseConfigs[i].mCompressionLimit = material->verticalStiffnessScaling.compressionRange;
			mPhaseConfigs[i].mStretchLimit = material->verticalStiffnessScaling.stretchRange;
		}
		else if (phaseType == PxClothFabricPhaseType::eHORIZONTAL)
		{
			mPhaseConfigs[i].mStiffness = material->horizontalStretchingStiffness;
			mPhaseConfigs[i].mStiffnessMultiplier = material->horizontalStiffnessScaling.scale;
			mPhaseConfigs[i].mCompressionLimit = material->horizontalStiffnessScaling.compressionRange;
			mPhaseConfigs[i].mStretchLimit = material->horizontalStiffnessScaling.stretchRange;
		}
		else if (phaseType == PxClothFabricPhaseType::eBENDING)
		{
			mPhaseConfigs[i].mStiffness = material->bendingStiffness;
			mPhaseConfigs[i].mStiffnessMultiplier = material->bendingStiffnessScaling.scale;
			mPhaseConfigs[i].mCompressionLimit = material->bendingStiffnessScaling.compressionRange;
			mPhaseConfigs[i].mStretchLimit = material->bendingStiffnessScaling.stretchRange;
		}
		else
		{
			PX_ASSERT(phaseType == PxClothFabricPhaseType::eSHEARING);
			mPhaseConfigs[i].mStiffness = material->shearingStiffness;
			mPhaseConfigs[i].mStiffnessMultiplier = material->shearingStiffnessScaling.scale;
			mPhaseConfigs[i].mCompressionLimit = material->shearingStiffnessScaling.compressionRange;
			mPhaseConfigs[i].mStretchLimit = material->shearingStiffnessScaling.stretchRange;
		}
	}

	cloth::Range<cloth::PhaseConfig> phaseConfig(mPhaseConfigs.begin(), mPhaseConfigs.end());
	mCloth->setPhaseConfig(phaseConfig);

	return true;
}



void Simulation::setRestPositions(bool on)
{
	if (mCloth == NULL || mIsStatic)
		return;


	if (on)
	{
		PxVec4* tempRestPositions = (PxVec4*)GetInternalApexSDK()->getTempMemory(sdkNumDeformableVertices * sizeof(PxVec4));

		for (uint32_t i = 0; i < sdkNumDeformableVertices; ++i)
		{
			tempRestPositions[i] = PxVec4(mRestPositions[i]*mActorScale, 0.0f);
		}

		mCloth->setRestPositions(cloth::Range<PxVec4>(tempRestPositions, tempRestPositions + sdkNumDeformableVertices));

		GetInternalApexSDK()->releaseTempMemory(tempRestPositions);
	}
	else
	{
		mCloth->setRestPositions(cloth::Range<PxVec4>());
	}
}



void Simulation::applyClothingDesc(tClothingDescTemplate& /*clothingTemplate*/)
{
}



void Simulation::setInterCollisionChannels(uint32_t channels)
{
	if (mCloth != NULL)
	{
		mCloth->setUserData((void*)(size_t)channels);
	}
}


#if APEX_UE4
void Simulation::simulate(float dt)
{
	return mCloth->simulate(dt);
}
#endif


void Simulation::setHalfPrecisionOption(bool isAllowed)
{
	if (mCloth != NULL)
	{
		mCloth->setHalfPrecisionOption(isAllowed);
	}
}



void Simulation::releaseFabric(NvParameterized::Interface* _cookedData)
{
	if (::strcmp(_cookedData->className(), ClothingCookedPhysX3Param::staticClassName()) == 0)
	{
		ClothingCookedPhysX3Param* cookedData = static_cast<ClothingCookedPhysX3Param*>(_cookedData);

		while (cookedData != NULL)
		{
			if (cookedData->fabricCPU != NULL)
			{
				cloth::Fabric* fabric = static_cast<cloth::Fabric*>(cookedData->fabricCPU);
				delete fabric;
				cookedData->fabricCPU = NULL;
			}

			for (int32_t i = 0; i < cookedData->fabricGPU.arraySizes[0]; ++i)
			{
				cloth::Fabric* fabric = static_cast<cloth::Fabric*>(cookedData->fabricGPU.buf[i].fabricGPU);
				delete fabric;
			}
			NvParameterized::Handle handle(*cookedData);
			if (cookedData->getParameterHandle("fabricGPU", handle) == NvParameterized::ERROR_NONE)
			{
				handle.resizeArray(0);
			}

			cookedData = static_cast<ClothingCookedPhysX3Param*>(cookedData->nextCookedData);
		}
	}
}



void Simulation::applyCollision()
{
	if (mCloth != NULL && !mIsStatic)
	{
		// spheres
		uint32_t numReleased = mReleasedSphereIds.size();
		if (numReleased > 0)
		{
			// remove all deleted spheres
			// biggest id's first, such that we don't
			// invalidate remaining id's
			nvidia::sort<uint32_t>(&mReleasedSphereIds[0], numReleased);
			for (int32_t i = (int32_t)numReleased-1; i >= 0; --i)
			{
				uint32_t id = mReleasedSphereIds[(uint32_t)i];
				if(id < 32)
				{
					mCloth->setSpheres(cloth::Range<const PxVec4>(),id, id+1);
				}
			}
			mReleasedSphereIds.clear();
		}
		PxVec4* end = (mCollisionSpheres.size() > 32) ? mCollisionSpheres.begin() + 32 : mCollisionSpheres.end();
		cloth::Range<const PxVec4> spheres((PxVec4*)mCollisionSpheres.begin(), end);
		mCloth->setSpheres(spheres, 0, mCloth->getNumSpheres());

		// capsules
		cloth::Range<const uint32_t> capsules(mCollisionCapsules.begin(), mCollisionCapsules.end());
		mCloth->setCapsules(capsules, 0, mCloth->getNumCapsules());

		// planes
		numReleased = mReleasedPlaneIds.size();
		if (numReleased > 0)
		{
			// remove all deleted planes
			// biggest id's first, such that we don't
			// invalidate remaining id's
			nvidia::sort<uint32_t>(&mReleasedPlaneIds[0], numReleased);
			for (int32_t i = (int32_t)numReleased-1; i >= 0; --i)
			{
				uint32_t id = mReleasedPlaneIds[(uint32_t)i];
				if(id < 32)
				{
					mCloth->setPlanes(cloth::Range<const PxVec4>(),id, id+1);
				}
			}
			mReleasedPlaneIds.clear();
		}

		end = (mCollisionPlanes.size() > 32) ? mCollisionPlanes.begin() + 32 : mCollisionPlanes.end();
		cloth::Range<const PxVec4> planes((PxVec4*)mCollisionPlanes.begin(), end);
		mCloth->setPlanes(planes, 0, mCloth->getNumPlanes());

		// convexes
		cloth::Range<const uint32_t> convexes(mCollisionConvexes.begin(), mCollisionConvexes.end());
		mCloth->setConvexes(convexes,0,mCloth->getNumConvexes());

		// triangle meshes
		// If mCollisionTrianglesOld is empty, updateCollision hasn't been called.
		// In that case there have been no changes, so use the same buffer for old
		// and new triangle positions.
		cloth::Range<const PxVec3> trianglesOld(
												(mCollisionTrianglesOld.size() > 0) ? mCollisionTrianglesOld.begin() : mCollisionTriangles.begin(),
												(mCollisionTrianglesOld.size() > 0) ? mCollisionTrianglesOld.end() : mCollisionTriangles.end()
												);
		cloth::Range<const PxVec3> triangles(mCollisionTriangles.begin(), mCollisionTriangles.end());
		mCloth->setTriangles(trianglesOld, triangles, 0);
		mCollisionTrianglesOld.clear();

		mCloth->enableContinuousCollision(!simulation.disableCCD);
		if (mTeleported)
		{
			mCloth->clearInterpolation();
		}
	}
}



bool Simulation::allocateHostMemory(MappedArray& mappedArray)
{
	bool allocated = false;
	if (mappedArray.hostMemory.size() != mappedArray.deviceMemory.size())
	{
		mappedArray.hostMemory.resize((uint32_t)mappedArray.deviceMemory.size());
		allocated = true; // read the first time to init the data!
	}
	return allocated;
}



ClothingSceneSimulateTask::ClothingSceneSimulateTask(SceneIntl* apexScene, ClothingScene* scene, ModuleClothingImpl* module, profile::PxProfileZoneManager* manager) :
	mModule(module),
	mApexScene(apexScene),
	mScene(scene),
	mSimulationDelta(0.0f),
	mSolverGPU(NULL), mSolverCPU(NULL),
	mProfileSolverGPU(NULL), mProfileSolverCPU(NULL),
	mWaitForSolverTask(NULL),
	mProfileManager(manager),
	mFailedGpuFactory(false)
{
	PX_UNUSED(mFailedGpuFactory);
#ifndef PHYSX_PROFILE_SDK
	PX_UNUSED(mProfileSolverGPU);
#endif
}



ClothingSceneSimulateTask::~ClothingSceneSimulateTask()
{
	PX_ASSERT(mSolverGPU == NULL);

	if (mSolverCPU != NULL)
	{
		delete mSolverCPU;
		mSolverCPU = NULL;
	}

#ifdef PHYSX_PROFILE_SDK
	if (mProfileSolverGPU != NULL)
	{
		mProfileSolverGPU->release();
		mProfileSolverGPU = NULL;
	}

	if (mProfileSolverCPU != NULL)
	{
		mProfileSolverCPU->release();
		mProfileSolverCPU = NULL;
	}
#endif
}



void ClothingSceneSimulateTask::setWaitTask(PxBaseTask* waitForSolver)
{
	mWaitForSolverTask = waitForSolver;
}



void ClothingSceneSimulateTask::setDeltaTime(float simulationDelta)
{
	mSimulationDelta = simulationDelta;
}



float ClothingSceneSimulateTask::getDeltaTime()
{
	return mSimulationDelta;
}



cloth::Solver* ClothingSceneSimulateTask::getSolver(ClothFactory factory)
{
	PX_ASSERT(factory.factory != NULL);
	PX_ASSERT(factory.mutex != NULL);

#if PX_WINDOWS_FAMILY
	if (factory.factory->getPlatform() == cloth::Factory::CUDA)
	{
		if (mSolverGPU == NULL)
		{
			PX_ASSERT(mProfileSolverGPU == NULL);
			if (mProfileManager != NULL)
			{
#ifdef PHYSX_PROFILE_SDK
				mProfileSolverGPU = &mProfileManager->createProfileZone("CUDA Cloth", profile::PxProfileNames());
#endif
			}

			nvidia::Mutex::ScopedLock wlock(*factory.mutex);
			mSolverGPU = factory.factory->createSolver(mProfileSolverGPU, mApexScene->getTaskManager());
		}

		PX_ASSERT(mSolverGPU != NULL);
		return mSolverGPU;
	}
#endif

	if (factory.factory->getPlatform() == cloth::Factory::CPU && mSolverCPU == NULL)
	{
		PX_ASSERT(mProfileSolverCPU == NULL);
		if (mProfileManager != NULL)
		{
#ifdef PHYSX_PROFILE_SDK
			mProfileSolverCPU = &mProfileManager->createProfileZone("CPU Cloth", profile::PxProfileNames());
#endif
		}

		nvidia::Mutex::ScopedLock wlock(*factory.mutex);
		mSolverCPU = factory.factory->createSolver(mProfileSolverCPU, mApexScene->getTaskManager());
	}

	PX_ASSERT(mSolverCPU != NULL);
	return mSolverCPU;
}



void ClothingSceneSimulateTask::clearGpuSolver()
{
#if PX_WINDOWS_FAMILY

#ifdef PHYSX_PROFILE_SDK
	if (mProfileSolverGPU != NULL)
	{
		mProfileSolverGPU->release();
		mProfileSolverGPU = NULL;
	}
#endif

	if (mSolverGPU != NULL)
	{
		delete mSolverGPU;
		mSolverGPU = NULL;
	}
#endif
}



void ClothingSceneSimulateTask::run()
{
	PX_ASSERT(mSimulationDelta > 0.0f);
	PX_ASSERT(mWaitForSolverTask != NULL);

	mScene->setSceneRunning(true);

	PxBaseTask* task1 = NULL;
	PxBaseTask* task2 = NULL;

	float interCollisionDistance = mModule->getInterCollisionDistance();
	float interCollisionStiffness = mModule->getInterCollisionStiffness();
	uint32_t interCollisionIterations = mModule->getInterCollisionIterations();

	if (mSolverCPU != NULL)
	{
		mSolverCPU->setInterCollisionDistance(interCollisionDistance);
		mSolverCPU->setInterCollisionStiffness(interCollisionStiffness);
		mSolverCPU->setInterCollisionNbIterations(interCollisionIterations);
		mSolverCPU->setInterCollisionFilter(interCollisionFilter);

		task1 = &mSolverCPU->simulate(mSimulationDelta, *mWaitForSolverTask);
	}
	if (mSolverGPU != NULL)
	{
		mSolverGPU->setInterCollisionDistance(interCollisionDistance);
		mSolverGPU->setInterCollisionStiffness(interCollisionStiffness);
		mSolverGPU->setInterCollisionNbIterations(interCollisionIterations);
		mSolverGPU->setInterCollisionFilter(interCollisionFilter);

		task2 = &mSolverGPU->simulate(mSimulationDelta, *mWaitForSolverTask);
	}

	// only remove the references when both simulate() methods have been called
	if (task1 != NULL)
	{
		task1->removeReference();
	}

	if (task2 != NULL)
	{
		task2->removeReference();
	}
}



const char* ClothingSceneSimulateTask::getName() const
{
	return "Simulate";
}



bool ClothingSceneSimulateTask::interCollisionFilter(void* user0, void* user1)
{
	size_t collisionChannels0 = reinterpret_cast<size_t>(user0);
	size_t collisionChannels1 = reinterpret_cast<size_t>(user1);
	return (collisionChannels0 & collisionChannels1) > 0;
}



WaitForSolverTask::WaitForSolverTask(ClothingScene* scene) :
	mScene(scene)
{
}



void WaitForSolverTask::run()
{
	mScene->setSceneRunning(false);
	mScene->embeddedPostSim();
}

const char* WaitForSolverTask::getName() const
{
	return "WaitForSolverTask";
}

}
} // namespace nvidia
