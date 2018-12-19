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


#ifndef SAMPLE_PARTICLES_H
#define SAMPLE_PARTICLES_H

#include "PxPhysicsAPI.h"
#include "PhysXSample.h"
#include "ParticleSystem.h"

#if PX_VC
#pragma warning(push)
#pragma warning(disable:4702)
#include <map>
#pragma warning(pop)
#else
#include <map>
#endif

namespace physx
{
	class PxController;
};

class RenderParticleSystemActor;
class RenderBaseActor;

class SampleParticles : public PhysXSample
{
	public:
												SampleParticles(PhysXSampleApplication& app);
		virtual									~SampleParticles();

		///////////////////////////////////////////////////////////////////////////////

		// Implements SampleApplication
		virtual	void							onInit();
        virtual	void						    onInit(bool restart) { onInit(); }
		virtual void							onShutdown();
		virtual void							onDigitalInputEvent(const SampleFramework::InputEvent& , bool val);
		virtual void							onPointerInputEvent(const SampleFramework::InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val);
		virtual	void							onTickPreRender(float dtime);
		virtual void							onSubstepSetup(float dtime, PxBaseTask* cont);		
		
		///////////////////////////////////////////////////////////////////////////////

		// Implements PhysXSampleApplication
		virtual	void							helpRender(PxU32 x, PxU32 y, PxU8 textAlpha);	
		virtual	void							descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha);
		virtual	void							customizeSample(SampleSetup&);
		virtual	void							customizeSceneDesc(PxSceneDesc&);
		virtual	void							customizeRender();				
		virtual void							collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents);
		
	protected:
		// Implements RAWImportCallback
		virtual void							newMesh(const RAWMesh& data);

	private:
		struct Emitter
		{
			Emitter() : emitter(NULL), isEnabled(false), particleSystem(NULL) {}
			void update(float dt);
			class ParticleEmitter* emitter;
			bool isEnabled;
			RenderParticleSystemActor* particleSystem;
		};
	
		class Raygun 
		{
		public:
			Raygun();
			~Raygun();

			void						init(SampleParticles*);
			void						clear();
			bool						isEnabled();
			void						setEnabled(bool);
			void						onSubstep(float dtime);
			void						update(float dtime);
			
		private:
			typedef std::map<PxShape*, PxReal> DebrisLifetimeMap;
			Emitter						mSmokeEmitter;
			Emitter						mDebrisEmitter;
			PxRigidDynamic*				mForceSmokeCapsule;
			PxRigidDynamic*				mForceWaterCapsule;
			class RenderBoxActor*		mRenderActor;

			SampleParticles*			mSample;
			bool						mIsImpacting;
			float						mRbDebrisTimer;
			float						mForceTimer;
			DebrisLifetimeMap			mDebrisLifetime;

			PxConvexMesh*				generateConvex(PxPhysics& sdk, PxCooking& cooking, PxReal scale);
			PxRigidDynamic*				createRayCapsule(bool enableCollision, PxFilterData filterData);
			void						releaseRayCapsule(PxRigidDynamic*& actor);
			void						updateRayCapsule(PxRigidDynamic* actor, const PxTransform& pose, float radiusMaxMultiplier);
		};
		friend			class			Raygun;

		PxU32							mFluidParticleCount;
		PxF32							mFluidEmitterWidth;
		PxF32							mFluidEmitterVelocity;
		PxU32							mSmokeParticleCount;
		PxF32							mSmokeLifetime;
		PxF32							mSmokeEmitterRate;
		PxU32							mDebrisParticleCount;
		
		PxU32							mParticleLoadIndex;
		PxU32							mParticleLoadIndexMax;
		PxF32							mLoadTextAlpha;

		RenderParticleSystemActor*		mWaterfallParticleSystem;
		RenderParticleSystemActor*		mSmokeParticleSystem;
		RenderParticleSystemActor*		mDebrisParticleSystem;
		
		Raygun							mRaygun;		
		std::vector<Emitter>			mEmitters;
#if PX_SUPPORT_GPU_PHYSX
		bool							mRunOnGpu;
#endif

		void							createParticleContent();
		void							releaseParticleContent();

		void							loadAssets();
		void							loadTerrain(const char* path, PxReal xScale, PxReal yScale, PxReal zScale);		
		void							createDrain();
		void							createWaterfallEmitter(const PxTransform& transform);
		void							createHotpotEmitter(const PxVec3& position);
		void							createParticleSystems();
		void							releaseParticleSystems();

		PxParticleFluid*				createFluid(PxU32 maxParticles, PxReal restitution, PxReal viscosity,
													PxReal stiffness, PxReal dynamicFriction, PxReal particleDistance);
		
		PxParticleSystem*				createParticleSystem(PxU32 maxParticles);
		
		RenderMaterial*					getMaterial(PxU32 materialID);
		template<typename T>			void runtimeAssert(const T x, 
											const char* msg, 
											const char* extra1 = 0, 
											const char* extra2 = 0, 
											const char* extra3 = 0);

#if PX_SUPPORT_GPU_PHYSX
		void                            toggleGpu();
#endif
};

#endif
