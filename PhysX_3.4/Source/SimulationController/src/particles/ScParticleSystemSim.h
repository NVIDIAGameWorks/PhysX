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


#ifndef PX_PHYSICS_SCP_PARTICLE_SYSTEM_SIM
#define PX_PHYSICS_SCP_PARTICLE_SYSTEM_SIM

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_PARTICLE_SYSTEM_API

#include "ScScene.h"
#include "ScActorSim.h"
#include "ScRigidSim.h"
#include "PsPool.h"

#include "ScParticlePacketShape.h"
#include "PtParticleSystemSim.h"

namespace physx
{

#if PX_SUPPORT_GPU_PHYSX
class PxParticleDeviceExclusiveAccess;
#endif

namespace Pt
{
	class Context;
	class ParticleSystemSim;
	class ParticleShape;
	class ParticleSystemState;
	struct ParticleSystemSimDataDesc;
	struct ParticleShapesUpdateInput;
	struct ParticleCollisionUpdateInput;
}

namespace Sc
{
	class ParticleSystemCore;
	class ShapeSim;
	class ParticlePacketShape;

#define PX_PARTICLE_SYSTEM_DEBUG_RENDERING			1

	class ParticleSystemSim : public ActorSim
	{
	public:

		ParticleSystemSim(Scene&, ParticleSystemCore&);
		
		void					release(bool releaseStateBuffers);

		PxFilterData			getSimulationFilterData() const;
		void					scheduleRefiltering();
		void					resetFiltering();

		void					setFlags(PxU32 flags);
		PxU32					getInternalFlags() const;

		void					getSimParticleData(Pt::ParticleSystemSimDataDesc& simParticleData, bool devicePtr) const;
		Pt::ParticleSystemState& getParticleState();

		void					addInteraction(const ParticlePacketShape& particleShape, const ShapeSim& shape, const PxU32 ccdPass);
		void					removeInteraction(const ParticlePacketShape& particleShape, const ShapeSim& shape, bool isDyingRb, const PxU32 ccdPass);
		void					onRbShapeChange(const ParticlePacketShape& particleShape, const ShapeSim& shape);

		void					processShapesUpdate();
#if PX_SUPPORT_GPU_PHYSX
		Ps::IntBool				isGpu() const { return mLLSim->isGpuV(); }
#endif
		// batched updates
		static PxBaseTask&	scheduleShapeGeneration(Pt::Context& context, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation);
		static PxBaseTask&	scheduleDynamicsCpu(Pt::Context& context, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation);
		static PxBaseTask&	scheduleCollisionPrep(Pt::Context& context, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation);
		static PxBaseTask&	scheduleCollisionCpu(Pt::Context& context, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation);
		static PxBaseTask&	schedulePipelineGpu(Pt::Context& context, const Ps::Array<ParticleSystemSim*>& particleSystems, PxBaseTask& continuation);

		//---------------------------------------------------------------------------------
		// Actor implementation
		//---------------------------------------------------------------------------------
	public:
		// non-DDI methods:

		// Core functionality
		void				startStep();
		void				endStep();

		void				unlinkParticleShape(ParticlePacketShape* particleShape);

		ParticleSystemCore&	getCore() const;
		
#if PX_SUPPORT_GPU_PHYSX
		void				enableDeviceExclusiveModeGpu();
		PxParticleDeviceExclusiveAccess*
							getDeviceExclusiveAccessGpu() const;
#endif

	private:
		~ParticleSystemSim() {}

		void				createShapeUpdateInput(Pt::ParticleShapesUpdateInput& input);	
		void				createCollisionUpdateInput(Pt::ParticleCollisionUpdateInput& input);	
		void				updateRigidBodies();
		void				prepareCollisionInput(PxBaseTask* continuation);

		// ParticleSystem packet handling
		void				releaseParticlePacketShapes();
		PX_INLINE void		addParticlePacket(Pt::ParticleShape* llParticleShape);
		PX_INLINE void		removeParticlePacket(const Pt::ParticleShape * llParticleShape);


#if PX_ENABLE_DEBUG_VISUALIZATION
	public:
		void visualizeStartStep(Cm::RenderOutput& out);
		void visualizeEndStep(Cm::RenderOutput& out);

	private:
		void	visualizeParticlesBounds(Cm::RenderOutput& out);
		void	visualizeParticles(Cm::RenderOutput& out);
		void	visualizeCollisionNormals(Cm::RenderOutput& out);
		void	visualizeSpatialGrid(Cm::RenderOutput& out);
		void	visualizeBroadPhaseBounds(Cm::RenderOutput& out);
		void	visualizeInteractions(Cm::RenderOutput& out);	// MS: Might be helpful for debugging
#endif  // PX_ENABLE_DEBUG_VISUALIZATION


	private:
		Pt::ParticleSystemSim* mLLSim;

		// Array of particle packet shapes
		Ps::Pool<ParticlePacketShape> mParticlePacketShapePool;
		Ps::Array<ParticlePacketShape*> mParticlePacketShapes;

		// Count interactions for sizing the contact manager stream
		PxU32 mInteractionCount;

		typedef Cm::DelegateTask<Sc::ParticleSystemSim, &Sc::ParticleSystemSim::prepareCollisionInput> CollisionInputPrepTask;
		CollisionInputPrepTask mCollisionInputPrepTask;
	};

} // namespace Sc

}

#endif	// PX_USE_PARTICLE_SYSTEM_API

#endif
