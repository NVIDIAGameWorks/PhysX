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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "foundation/PxMemory.h"
#include "ParticleSystem.h"
#include "ParticleFactory.h"
#include "PsMathUtils.h"
#include "PsBitUtils.h"
#include "PxTkFile.h"
#include "PhysXSample.h"
#include "SampleArray.h"
#include <algorithm>

#if defined(RENDERER_ENABLE_CUDA_INTEROP)
#include <cuda.h>

namespace
{
	void checkSuccess(CUresult r)
	{
		PX_ASSERT(r == CUDA_SUCCESS);
	}
}

#endif

ParticleSystem::ParticleSystem(PxParticleBase* _mParticleSystem, bool _useInstancedMeshes) :
	mParticleSystem(_mParticleSystem),
	mParticlesPositions(_mParticleSystem->getMaxParticles()),
	mParticlesVelocities(_mParticleSystem->getMaxParticles()),
	mParticlesOrientations(_mParticleSystem->getMaxParticles()),
	mParticleLifetime(0.0f),
	mValidParticleRange(0),
	mUseInstancedMeshes(_useInstancedMeshes),
	mParticlesOrientationsDevice(0),
	mParticleLifetimeDevice(0),
	mParticleValidityDevice(0),
	mCtxMgr(NULL)
{
	mNumParticles = _mParticleSystem->getMaxParticles();
	setUseLifetime(false);
	if(mUseInstancedMeshes) 
	{
		initializeParticlesOrientations();
	}
	mIndexPool = PxParticleExt::createIndexPool(mNumParticles);
	mParticleValidity = (PxU32*)PX_ALLOC(((_mParticleSystem->getMaxParticles() + 31) >> 5) << 2, "validParticleBitmap");

#ifdef RENDERER_ENABLE_CUDA_INTEROP

	PxScene* scene = _mParticleSystem->getScene();
	if (scene)
	{
		PxGpuDispatcher* dispatcher = scene->getTaskManager()->getGpuDispatcher();
		// contxt must be created in at least one valid interop mode
		if (dispatcher && (mCtxMgr = dispatcher->getCudaContextManager()) && 
			mCtxMgr->getInteropMode() != PxCudaInteropMode::D3D10_INTEROP &&
			mCtxMgr->getInteropMode() != PxCudaInteropMode::D3D11_INTEROP)
		{		
			mCtxMgr = NULL;
		}
	}

	if (mCtxMgr)
	{
		mCtxMgr->acquireContext();

		checkSuccess(cuMemAlloc(&mParticleValidityDevice, sizeof(PxU32)*(_mParticleSystem->getMaxParticles() + 31) >> 5));
		checkSuccess(cuMemAlloc(&mParticleLifetimeDevice, sizeof(PxU32)*(_mParticleSystem->getMaxParticles())));

		if(mUseInstancedMeshes)
		{
			checkSuccess(cuMemAlloc(&mParticlesOrientationsDevice, sizeof(PxMat33)*_mParticleSystem->getMaxParticles()));
			checkSuccess(cuMemcpyHtoDAsync(mParticlesOrientationsDevice, &mParticlesOrientations[0], sizeof(PxMat33)*_mParticleSystem->getMaxParticles(), 0));
		}

		mCtxMgr->releaseContext();
	}
#endif
}

void ParticleSystem::initializeParticlesOrientations() 
{
	for(PxU32 i = 0; i < mParticleSystem->getMaxParticles(); ++i) 
	{
		mParticlesOrientations[i].column2 = PxVec3(getSampleRandom().rand(0.0f, 1.0f),
													getSampleRandom().rand(0.0f, 1.0f),
													getSampleRandom().rand(0.0f, 1.0f)).getNormalized();
		PxVec3 vUp(0.0f, 1.0f, 0.0f);
		mParticlesOrientations[i].column0 = vUp.cross(mParticlesOrientations[i].column2).getNormalized();
		mParticlesOrientations[i].column1 = mParticlesOrientations[i].column2.
											cross(mParticlesOrientations[i].column0).
											getNormalized();
	}
}

ParticleSystem::~ParticleSystem() 
{
#ifdef RENDERER_ENABLE_CUDA_INTEROP
	if (mCtxMgr)
	{
		mCtxMgr->acquireContext();
		checkSuccess(cuMemFree(mParticleValidityDevice));
		checkSuccess(cuMemFree(mParticleLifetimeDevice));
		checkSuccess(cuMemFree(mParticlesOrientationsDevice));
		mCtxMgr->releaseContext();
	}
#endif

	PX_FREE(mParticleValidity);
	if (mParticleSystem)
	{
		mParticleSystem->release();
		mParticleSystem = NULL;
	}
	if (mIndexPool)
	{
		mIndexPool->release();
		mIndexPool = NULL;
	}	
}

/* enables limiting particles lifetime */
void ParticleSystem::setUseLifetime(bool use) 
{ 
	mUseLifetime = use; 
}

/* returns true if limiting particles lifetime is enabled */
bool ParticleSystem::useLifetime() 
{ 
	return mUseLifetime; 
}

/* setUseLifetime(true) before setting this */
void ParticleSystem::setLifetime(PxReal lt) 
{
	PX_ASSERT(lt >= 0.0f);
	mParticleLifetime = lt;
	mParticleLifes.resize(mParticleSystem->getMaxParticles());
	std::fill(mParticleLifes.begin(), mParticleLifes.end(), mParticleLifetime);
}

/* Modifies rotation matrix of the particle 
	Different rotation rules here.
*/
void ParticleSystem::modifyRotationMatrix(PxMat33& rotMatrix, PxReal deltaTime, const PxVec3& velocity) 
{
	PxVec3 delta = PxVec3(rotMatrix.column1 - 
							rotMatrix.column0).getNormalized() * 
							deltaTime * 
							velocity.magnitude();
    //PxVec3 vUp(0.0f, 1.0f, 0.0f);
	rotMatrix.column2 = (rotMatrix.column2 + delta).getNormalized();
	rotMatrix.column0 = rotMatrix.column1.cross(rotMatrix.column2).getNormalized();
	rotMatrix.column1 = rotMatrix.column2.cross(rotMatrix.column0).getNormalized();
}


/* fetches particles positions from library, 
	removes invalid particles 
	(intersected with drain, non-positive lifetime),
	creates new particles */
void ParticleSystem::update(float deltaTime)
{
	mNumParticles = 0;
	PxParticleReadData* mParticleSystemData = mParticleSystem->lockParticleReadData();
	PX_ASSERT(mParticleSystemData);
	std::vector<PxU32> mTmpIndexArray;
	PxU32 newValidRange = 0;
	if (mParticleSystemData->validParticleRange > 0) 
	{
		PxStrideIterator<const PxVec3> positions(mParticleSystemData->positionBuffer);
		PxStrideIterator<const PxVec3> velocities(mParticleSystemData->velocityBuffer);
		PxStrideIterator<const PxParticleFlags> particleFlags(mParticleSystemData->flagsBuffer);
		PxMemCopy(mParticleValidity, mParticleSystemData->validParticleBitmap, ((mParticleSystemData->validParticleRange + 31) >> 5) << 2);

		// copy particles positions
		for (PxU32 w = 0; w <= (mParticleSystemData->validParticleRange-1) >> 5; w++)
		{
			for (PxU32 b = mParticleSystemData->validParticleBitmap[w]; b; b &= b-1) 
			{
				PxU32 index = (w << 5 | Ps::lowestSetBit(b));
				bool removed = false;
				if (particleFlags[index] & PxParticleFlag::eCOLLISION_WITH_DRAIN || 
					particleFlags[index] & PxParticleFlag::eSPATIAL_DATA_STRUCTURE_OVERFLOW) 
				{
					mTmpIndexArray.push_back(index);
					removed = true;
				} 
				else if(mUseLifetime) 
				{
					if(mParticleLifes[index] < 0.0) 
					{
						mParticleLifes[index] = mParticleLifetime;
						mTmpIndexArray.push_back(index);
						removed = true;
					}
				}
				if(!removed) 
				{
					mParticlesPositions[index] = positions[index];
					mParticlesVelocities[index] = velocities.ptr() ? velocities[index] : PxVec3(0.0f);						
					if(mUseInstancedMeshes) 
					{
						modifyRotationMatrix(mParticlesOrientations[index], deltaTime, velocities[index]);
					}
					if(mUseLifetime) 
					{
						mParticleLifes[index] -= deltaTime;
					}
					mNumParticles++;
					newValidRange = index;
				}
				else
				{
					mParticleValidity[w] &= (b-1);
				}
			}
		}
	}
	
	mValidParticleRange = newValidRange;

	mParticleSystemData->unlock();

#ifdef RENDERER_ENABLE_CUDA_INTEROP
	if (mCtxMgr && (mParticleSystem->getParticleBaseFlags()&PxParticleBaseFlag::eGPU))
	{
		mCtxMgr->acquireContext();

		if (mValidParticleRange)
			cuMemcpyHtoDAsync(mParticleValidityDevice, &mParticleValidity[0], sizeof(PxU32)*(mParticleSystem->getMaxParticles() + 31) >> 5, 0);		

		if (mUseLifetime && mParticleLifes.size())
			cuMemcpyHtoDAsync(mParticleLifetimeDevice, &mParticleLifes[0], sizeof(PxReal)*mValidParticleRange, 0);

		if (mUseInstancedMeshes)
			cuMemcpyHtoDAsync(mParticlesOrientationsDevice, &mParticlesOrientations[0], sizeof(PxMat33)*mValidParticleRange, 0);

		mCtxMgr->releaseContext();
	}
#endif

	if(mNumParticles > 0 && mTmpIndexArray.size() != 0) 
	{
		PxStrideIterator<const PxU32> indexData(&mTmpIndexArray[0]);
		mParticleSystem->releaseParticles(static_cast<PxU32>(mTmpIndexArray.size()), indexData);
		mIndexPool->freeIndices(static_cast<PxU32>(mTmpIndexArray.size()), indexData);
	}
}

/* creates particles in the PhysX SDK */
void ParticleSystem::createParticles(ParticleData& particles)
{
	particles.numParticles = PxMin(particles.numParticles, mParticleSystem->getMaxParticles() - mNumParticles);

	if (particles.numParticles > 0)
	{
		std::vector<PxU32> mTmpIndexArray;
		mTmpIndexArray.resize(particles.numParticles);
		PxStrideIterator<PxU32> indexData(&mTmpIndexArray[0]);
		// allocateIndices() may clamp the number of inserted particles
		particles.numParticles = mIndexPool->allocateIndices(particles.numParticles, indexData);

		PxParticleCreationData particleCreationData;
		particleCreationData.numParticles = particles.numParticles;
		particleCreationData.indexBuffer = PxStrideIterator<const PxU32>(&mTmpIndexArray[0]);
		particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(&particles.positions[0]);
		particleCreationData.velocityBuffer	= PxStrideIterator<const PxVec3>(&particles.velocities[0]);
		mNumParticles += particles.numParticles;
		bool ok = mParticleSystem->createParticles(particleCreationData);
		PX_UNUSED(ok);
		PX_ASSERT(ok);
	}	
}

/* Returns pointer to the internal PxParticleBase */
PxParticleBase* ParticleSystem::getPxParticleBase() 
{ 
	return mParticleSystem; 
}

/* Returns pointer to the particles positions */
const std::vector<PxVec3>& ParticleSystem::getPositions() 
{ 
	return mParticlesPositions; 
} 

/* Returns pointer to the particles velocities */
const std::vector<PxVec3>& ParticleSystem::getVelocities() 
{ 
	return mParticlesVelocities; 
}

/* Returns pointer to the particles orientations */
const std::vector<PxMat33>& ParticleSystem::getOrientations() 
{ 
	return mParticlesOrientations; 
} 

/* Returns pointer to the particles validity */
const PxU32* ParticleSystem::getValidity() 
{ 
	return mParticleValidity; 
} 

/* Returns range of vaild particles index */
PxU32  ParticleSystem::getValidParticleRange()
{
	return mValidParticleRange;
}

/* Returns pointer to the particles lifetimes */
const std::vector<PxReal>& ParticleSystem::getLifetimes() 
{
	return mParticleLifes; 
}

/* Returns number of particles */
PxU32 ParticleSystem::getNumParticles() 
{ 
	PxParticleReadData* particleReadData = mParticleSystem->lockParticleReadData();
	PX_ASSERT(particleReadData);
	PxU32 numParticles = particleReadData->nbValidParticles;
	particleReadData->unlock();
	return numParticles;
}

PxU32 ParticleSystem::createParticles(const PxParticleCreationData& particles, PxStrideIterator<PxU32>* particleIndices, PxReal lifetime)
{
	PX_ASSERT(lifetime >= 0.0f);
	//its not supported currently to pass in particle indices, as they are created here.
	PX_ASSERT(particles.indexBuffer.ptr() == NULL);

	PxParticleCreationData particlesCopy(particles);
	SampleArray<PxU32> mTmpIndexArray;
	mTmpIndexArray.resize(particles.numParticles);

	PxU32 numAllocatedIndices = mIndexPool->allocateIndices(particles.numParticles, PxStrideIterator<PxU32>(mTmpIndexArray.begin()));
	particlesCopy.indexBuffer = PxStrideIterator<PxU32>(mTmpIndexArray.begin());
	particlesCopy.numParticles = numAllocatedIndices;

	bool isSuccess = mParticleSystem->createParticles(particlesCopy);
	PX_UNUSED(isSuccess);
	PX_ASSERT(isSuccess);

	if (mUseLifetime)
	{
		for (PxU32 i = 0; i < numAllocatedIndices; i++)
		{
			PxU32 index = mTmpIndexArray[i];
			PX_ASSERT(index < mParticleSystem->getMaxParticles());
			mParticleLifes[index] = lifetime;
		}
	}

	if (particleIndices)
	{
		for (PxU32 i = 0; i < numAllocatedIndices; i++)
			(*particleIndices)[i] = mTmpIndexArray[i];
	}

	return numAllocatedIndices;
}

PxU32 ParticleSystem::createParticles(const ParticleData& particles, PxReal lifetime)
{
	PxParticleCreationData particleCreationData;
	particleCreationData.numParticles = particles.numParticles;
	particleCreationData.positionBuffer = PxStrideIterator<const PxVec3>(particles.positions.begin());
	particleCreationData.velocityBuffer	= PxStrideIterator<const PxVec3>(particles.velocities.begin());

	if (particles.restOffsets.size() > 0)
		particleCreationData.restOffsetBuffer = PxStrideIterator<const PxF32>(particles.restOffsets.begin());

	return createParticles(particleCreationData, NULL, lifetime);
}

PxU32 ParticleSystem::createParticleSphere(PxU32 maxParticles, float particleDistance, const PxVec3& center, const PxVec3& vel, PxReal lifetime, PxReal restOffsetVariance)
{
	float sideNumFloat = physx::shdfnd::pow(3.0f*maxParticles/(4.0f*PxPi), 1.0f/3.0f); 
	PxU32 sideNum = static_cast<PxU32>(physx::shdfnd::ceil(sideNumFloat));

	ParticleData initData(PxMin(mParticleSystem->getMaxParticles() - getNumParticles(), sideNum*sideNum*sideNum));
	CreateParticleSphere(initData, center, vel, particleDistance, sideNum);
	if (restOffsetVariance > 0.0f)
		SetParticleRestOffsetVariance(initData, mParticleSystem->getRestOffset(), restOffsetVariance);
	
	return createParticles(initData, lifetime);
}

PxU32 ParticleSystem::createParticleCube(PxU32 numX, PxU32 numY, PxU32 numZ, float particleDistance, const PxVec3& center, const PxVec3& vel, PxReal lifetime, PxReal restOffsetVariance)
{
//	PxU32 numParticles = numX * numY * numZ;

	PxBounds3 aabb;
	aabb.minimum = center - particleDistance * 0.5f * (PxVec3((PxReal)numX, (PxReal)numY, (PxReal)numZ) + PxVec3(0.5f));
	aabb.maximum = center + particleDistance * 0.5f * (PxVec3((PxReal)numX, (PxReal)numY, (PxReal)numZ) + PxVec3(0.5f));
	return createParticleCube(aabb, particleDistance, vel, lifetime, restOffsetVariance);
}

PxU32 ParticleSystem::createParticleCube(const PxBounds3& aabb, float particleDistance, const PxVec3& vel, PxReal lifetime, PxReal restOffsetVariance)
{
	PxVec3 aabbDim = aabb.getExtents();
	aabbDim *= 2.0f;
	unsigned sideNumX = (unsigned)PxMax(1.0f, physx::shdfnd::floor(aabbDim.x / particleDistance));
	unsigned sideNumY = (unsigned)PxMax(1.0f, physx::shdfnd::floor(aabbDim.y / particleDistance));
	unsigned sideNumZ = (unsigned)PxMax(1.0f, physx::shdfnd::floor(aabbDim.z / particleDistance));

	PxU32 numParticles = PxMin(sideNumX * sideNumY * sideNumZ, mParticleSystem->getMaxParticles() - getNumParticles());
	ParticleData initData(numParticles);
	CreateParticleAABB(initData, aabb, vel, particleDistance);
	if (restOffsetVariance > 0.0f)
		SetParticleRestOffsetVariance(initData, mParticleSystem->getRestOffset(), restOffsetVariance);

	return createParticles(initData, lifetime);
}

PxU32 ParticleSystem::createParticleRand(PxU32 numParticles,const PxVec3& particleRange, const PxVec3& center, const PxVec3& vel, PxReal lifetime, PxReal restOffsetVariance)
{
	ParticleData initData(numParticles);
	CreateParticleRand(initData, center, particleRange, vel);
	if (restOffsetVariance > 0.0f)
		SetParticleRestOffsetVariance(initData, mParticleSystem->getRestOffset(), restOffsetVariance);
	
	return createParticles(initData, lifetime);
}

PxU32 ParticleSystem::createParticlesFromFile(const char* particleFileName)
{
	PxU32 count = 0;
	SampleFramework::File* file = NULL;
	PxToolkit::fopen_s(&file, particleFileName, "rb");
	if (!file)
		return 0;
		
	bool readSuccess = fread(&count, 1, sizeof(PxU32), file) == sizeof(PxU32);
	if (!readSuccess)
		return 0;
		
	SampleArray<PxVec3> positions;
	SampleArray<PxVec3> velocities;
	positions.resize(count);
	velocities.resize(count);

	for (PxU32 i = 0; i < count; ++i)
	{
		readSuccess &= fread(&positions[i], 1, sizeof(PxVec3), file) == sizeof(PxVec3);
		readSuccess &= fread(&velocities[i], 1, sizeof(PxVec3), file) == sizeof(PxVec3);
	}

	PxU32 numNewParticles = 0;
	if (readSuccess)
	{
		PxParticleCreationData particleData;
		particleData.numParticles = count;
		particleData.positionBuffer = PxStrideIterator<PxVec3>(positions.begin());
		particleData.velocityBuffer = PxStrideIterator<PxVec3>(velocities.begin());
		numNewParticles = createParticles(particleData);
	}
	fclose(file);
	return numNewParticles;
	
}

bool ParticleSystem::dumpParticlesToFile(const char* particleFileName)
{
	SampleFramework::File* file = NULL;
	PxToolkit::fopen_s(&file, particleFileName, "wb");
	if (!file)
		return false;

	PxParticleReadData* prd = mParticleSystem->lockParticleReadData();
	if (!prd->positionBuffer.ptr())
		return false;

	PxStrideIterator<const PxVec3> positions(prd->positionBuffer);

	//zero velocities if no velocity buffer available
	PxVec3 zero(0.0f);
	PxStrideIterator<const PxVec3> velocities = (prd->velocityBuffer.ptr()) ? prd->velocityBuffer : PxStrideIterator<const PxVec3>(&zero, 0);
	
	//write particle count;
	bool writeSuccess = fwrite(&prd->nbValidParticles, 1, sizeof(PxU32), file) == sizeof(PxU32);

	//write particles
	if (prd->validParticleRange > 0)
	{
		for (PxU32 w = 0; w <= (prd->validParticleRange-1) >> 5; w++)
			for (PxU32 b = prd->validParticleBitmap[w]; b; b &= b-1)
			{
				PxU32 index = (w<<5|physx::shdfnd::lowestSetBit(b));
				writeSuccess &= fwrite(&positions[index], 1, sizeof(PxVec3), file) == sizeof(PxVec3);
				writeSuccess &= fwrite(&velocities[index], 1, sizeof(PxVec3), file) == sizeof(PxVec3);
			}
	}
	
	prd->unlock();
	fclose(file);
	return writeSuccess;
}

void ParticleSystem::releaseParticles(const SampleArray<PxU32>& indices)
{
	if (indices.size() == 0)
		return;

	PxStrideIterator<const PxU32> indexData(indices.begin());
	mParticleSystem->releaseParticles(indices.size(), indexData);
	mIndexPool->freeIndices(indices.size(), indexData);
}

void ParticleSystem::releaseParticles()
{
	mParticleSystem->releaseParticles();
	mIndexPool->freeIndices();
}

#endif // PX_USE_PARTICLE_SYSTEM_API
