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

#if PX_SUPPORT_PVD
#include "NpCast.h"
#include "ScbScenePvdClient.h"
#include "PxActor.h"
#include "PxRenderBuffer.h"
#include "ScbScene.h"
#include "ScbNpDeps.h"
#include "PxPhysics.h"
#include "ScMaterialCore.h"
#include "PxsMaterialManager.h"
#include "ScBodySim.h"
#include "ScConstraintSim.h"
#include "ScParticleSystemCore.h"
#include "ScParticleSystemSim.h"

#include "PxConstraintDesc.h"
#include "ScConstraintCore.h"
#include "PvdTypeNames.h"

#include "gpu/PxParticleGpu.h"
#include "PxPvdUserRenderer.h"
#include "PxvNphaseImplementationContext.h"

using namespace physx;
using namespace physx::Vd;
using namespace physx::pvdsdk;
using namespace Scb;

namespace
{
	PX_FORCE_INLINE	PxU64	getContextId(Scb::Scene& scene) { return scene.getContextId(); }

	///////////////////////////////////////////////////////////////////////////////

	// Sc-to-Np
	PX_FORCE_INLINE static NpConstraint* getNpConstraint(Sc::ConstraintCore* scConstraint)
	{
		const size_t scOffset = reinterpret_cast<size_t>(&(reinterpret_cast<Scb::Constraint*>(0)->getScConstraint()));
		const size_t scbOffset = reinterpret_cast<size_t>(&(reinterpret_cast<NpConstraint*>(0)->getScbConstraint()));
		return reinterpret_cast<NpConstraint*>(reinterpret_cast<char*>(scConstraint) - scOffset - scbOffset);
	}

#if PX_USE_PARTICLE_SYSTEM_API
	// Sc-to-Scb
	PX_FORCE_INLINE static Scb::ParticleSystem* getScbParticleSystem(Sc::ParticleSystemCore* scParticleSystem)
	{
		const size_t offset = reinterpret_cast<size_t>(&(reinterpret_cast<Scb::ParticleSystem*>(0)->getScParticleSystem()));
		return reinterpret_cast<Scb::ParticleSystem*>(reinterpret_cast<char*>(scParticleSystem) - offset);
	}
#endif

	///////////////////////////////////////////////////////////////////////////////

	PX_FORCE_INLINE static const PxActor* getPxActor(const Scb::Actor* scbActor)
	{
		const PxActorType::Enum type = scbActor->getActorCore().getActorCoreType();
		if(type == PxActorType::eRIGID_DYNAMIC)
			return getNpRigidDynamic(static_cast<const Scb::Body*>(scbActor));
		else if(type == PxActorType::eRIGID_STATIC)
			return getNpRigidStatic(static_cast<const Scb::RigidStatic*>(scbActor));
#if PX_USE_PARTICLE_SYSTEM_API
		else if(type == PxActorType::ePARTICLE_SYSTEM)
			return getNpParticleSystem(static_cast<const Scb::ParticleSystem*>(scbActor));
		else if(type == PxActorType::ePARTICLE_FLUID)
			return getNpParticleFluid(static_cast<const Scb::ParticleSystem*>(scbActor));
#endif
		else if(type == PxActorType::eARTICULATION_LINK)
			return getNpArticulationLink(static_cast<const Scb::Body*>(scbActor));
#if PX_USE_CLOTH_API
		else if(type == PxActorType::eCLOTH)
			return getNpCloth(const_cast<Scb::Cloth*>(static_cast<const Scb::Cloth*>(scbActor)));
#endif
		return NULL;
	}

	struct CreateOp
	{
		CreateOp& operator=(const CreateOp&);
		physx::pvdsdk::PvdDataStream& mStream;
		PvdMetaDataBinding& mBinding;
		PsPvd* mPvd;
		PxScene& mScene;
		CreateOp(physx::pvdsdk::PvdDataStream& str, PvdMetaDataBinding& bind, PsPvd* pvd, PxScene& scene)
			: mStream(str), mBinding(bind), mPvd(pvd), mScene(scene)
		{
		}
		template <typename TDataType>
		void operator()(const TDataType& dtype)
		{
			mBinding.createInstance(mStream, dtype, mScene, PxGetPhysics(), mPvd);
		}
		void operator()(const PxArticulationLink&)
		{
		}
#if PX_USE_PARTICLE_SYSTEM_API
		void operator()(const PxParticleSystem& dtype)
		{
			mBinding.createInstance(mStream, dtype, mScene);
		}
		void operator()(const PxParticleFluid& dtype)
		{
			mBinding.createInstance(mStream, dtype, mScene);
		}
#endif
	};

	struct UpdateOp
	{
		UpdateOp& operator=(const UpdateOp&);
		physx::pvdsdk::PvdDataStream& mStream;
		PvdMetaDataBinding& mBinding;
		UpdateOp(physx::pvdsdk::PvdDataStream& str, PvdMetaDataBinding& bind) : mStream(str), mBinding(bind)
		{
		}
		template <typename TDataType>
		void operator()(const TDataType& dtype)
		{
			mBinding.sendAllProperties(mStream, dtype);
		}
	};

	struct DestroyOp
	{
		DestroyOp& operator=(const DestroyOp&);
		physx::pvdsdk::PvdDataStream& mStream;
		PvdMetaDataBinding& mBinding;
		PxScene& mScene;
		DestroyOp(physx::pvdsdk::PvdDataStream& str, PvdMetaDataBinding& bind, PxScene& scene)
			: mStream(str), mBinding(bind), mScene(scene)
		{
		}
		template <typename TDataType>
		void operator()(const TDataType& dtype)
		{
			mBinding.destroyInstance(mStream, dtype, mScene);
		}
		void operator()(const PxArticulationLink& dtype)
		{
			mBinding.destroyInstance(mStream, dtype);
		}
#if PX_USE_PARTICLE_SYSTEM_API
		void operator()(const PxParticleSystem& dtype)
		{
			mBinding.destroyInstance(mStream, dtype, mScene);
		}
		void operator()(const PxParticleFluid& dtype)
		{
			mBinding.destroyInstance(mStream, dtype, mScene);
		}
#endif
	};

	template <typename TOperator>
	inline void BodyTypeOperation(const Scb::Body* scbBody, TOperator op)
	{
		bool isArticulationLink = scbBody->getActorType() == PxActorType::eARTICULATION_LINK;
		if(isArticulationLink)
		{
			const NpArticulationLink* link = getNpArticulationLink(scbBody);
			op(*static_cast<const PxArticulationLink*>(link));
		}
		else
		{
			const NpRigidDynamic* npRigidDynamic = getNpRigidDynamic(scbBody);
			op(*static_cast<const PxRigidDynamic*>(npRigidDynamic));
		}
	}

	template <typename TOperator>
	inline void ActorTypeOperation(const PxActor* actor, TOperator op)
	{
		switch(actor->getType())
		{
		case PxActorType::eRIGID_STATIC:
			op(*static_cast<const PxRigidStatic*>(actor));
			break;
		case PxActorType::eRIGID_DYNAMIC:
			op(*static_cast<const PxRigidDynamic*>(actor));
			break;
		case PxActorType::eARTICULATION_LINK:
			op(*static_cast<const PxArticulationLink*>(actor));
			break;
#if PX_USE_PARTICLE_SYSTEM_API
		case PxActorType::ePARTICLE_SYSTEM:
			op(*static_cast<const PxParticleSystem*>(actor));
			break;
		case PxActorType::ePARTICLE_FLUID:
			op(*static_cast<const PxParticleFluid*>(actor));
			break;
#endif
#if PX_USE_CLOTH_API
		case PxActorType::eCLOTH:
			op(*static_cast<const PxCloth*>(actor));
			break;
#endif
		case PxActorType::eACTOR_COUNT:
		case PxActorType::eACTOR_FORCE_DWORD:
			PX_ASSERT(false);
			break;
		};
	}

	namespace
	{
		struct PvdConstraintVisualizer : public PxConstraintVisualizer
		{
			PX_NOCOPY(PvdConstraintVisualizer)
		public:
			physx::pvdsdk::PvdUserRenderer& mRenderer;
			PvdConstraintVisualizer(const void* id, physx::pvdsdk::PvdUserRenderer& r) : mRenderer(r)
			{
				mRenderer.setInstanceId(id);
			}
			virtual void visualizeJointFrames(const PxTransform& parent, const PxTransform& child)
			{
				mRenderer.visualizeJointFrames(parent, child);
			}

			virtual void visualizeLinearLimit(const PxTransform& t0, const PxTransform& t1, PxReal value, bool active)
			{
				mRenderer.visualizeLinearLimit(t0, t1, PxF32(value), active);
			}

			virtual void visualizeAngularLimit(const PxTransform& t0, PxReal lower, PxReal upper, bool active)
			{
				mRenderer.visualizeAngularLimit(t0, PxF32(lower), PxF32(upper), active);
			}

			virtual void visualizeLimitCone(const PxTransform& t, PxReal ySwing, PxReal zSwing, bool active)
			{
				mRenderer.visualizeLimitCone(t, PxF32(ySwing), PxF32(zSwing), active);
			}

			virtual void visualizeDoubleCone(const PxTransform& t, PxReal angle, bool active)
			{
				mRenderer.visualizeDoubleCone(t, PxF32(angle), active);
			}

			virtual void visualizeLine( const PxVec3& p0, const PxVec3& p1, PxU32 color)
			{
				const PvdDebugLine line(p0, p1, color);
				mRenderer.drawLines(&line, 1);
			}
		};
	}

	class SceneRendererClient : public RendererEventClient, public physx::shdfnd::UserAllocated
	{
		PX_NOCOPY(SceneRendererClient)
	public:
		SceneRendererClient(PvdUserRenderer* renderer, PxPvd* pvd):mRenderer(renderer)
		{
			mStream = PvdDataStream::create(pvd); 
			mStream->createInstance(renderer);
		}

		~SceneRendererClient()
		{
			mStream->destroyInstance(mRenderer);
			mStream->release();
		}

		virtual void handleBufferFlush(const uint8_t* inData, uint32_t inLength)
		{
			mStream->setPropertyValue(mRenderer, "events", inData, inLength);
		}

	private:

		PvdUserRenderer* mRenderer;
		PvdDataStream* mStream;
	};

} // namespace

ScbScenePvdClient::ScbScenePvdClient(Scb::Scene& scene) :
	mPvd			(NULL),
	mScbScene		(scene),
	mPvdDataStream	(NULL),
	mUserRender		(NULL),
	mRenderClient	(NULL),
	mIsConnected	(false)
{
}

ScbScenePvdClient::~ScbScenePvdClient()
{
	if(mPvd)
		mPvd->removeClient(this);
}

void ScbScenePvdClient::updateCamera(const char* name, const PxVec3& origin, const PxVec3& up, const PxVec3& target)
{
	if(mIsConnected)
		mPvdDataStream->updateCamera(name, origin, up, target);
}

void ScbScenePvdClient::drawPoints(const PvdDebugPoint* points, PxU32 count)
{
	if(mUserRender)
		mUserRender->drawPoints(points, count);
}

void ScbScenePvdClient::drawLines(const PvdDebugLine* lines, PxU32 count)
{
	if(mUserRender)
		mUserRender->drawLines(lines, count);
}

void ScbScenePvdClient::drawTriangles(const PvdDebugTriangle* triangles, PxU32 count)
{
	if(mUserRender)
		mUserRender->drawTriangles(triangles, count);
}

void ScbScenePvdClient::drawText(const PvdDebugText& text)
{
	if(mUserRender)
		mUserRender->drawText(text);
}

void ScbScenePvdClient::setScenePvdFlag(PxPvdSceneFlag::Enum flag, bool value)
{
	if(value)
		mFlags |= flag;
	else
		mFlags &= ~flag;
}

void ScbScenePvdClient::onPvdConnected()
{
	if(mIsConnected || !mPvd)
		return;

	mIsConnected = true;

	mPvdDataStream = PvdDataStream::create(mPvd);

	mUserRender = PvdUserRenderer::create();
	mRenderClient = PX_NEW(SceneRendererClient)(mUserRender, mPvd);	
	mUserRender->setClient(mRenderClient);

	sendEntireScene();
}

void ScbScenePvdClient::onPvdDisconnected()
{
	if(!mIsConnected)
		return;
	mIsConnected = false;

	PX_DELETE(mRenderClient);
	mRenderClient = NULL;
	mUserRender->release();
	mUserRender = NULL;
	mPvdDataStream->release();
	mPvdDataStream = NULL;
}

void ScbScenePvdClient::updatePvdProperties()
{
	mMetaDataBinding.sendAllProperties(*mPvdDataStream, *mScbScene.getPxScene());
}

void ScbScenePvdClient::releasePvdInstance()
{
	if(mPvdDataStream)
	{		
		PxScene* theScene = mScbScene.getPxScene();
		// remove from parent	
		mPvdDataStream->removeObjectRef(&PxGetPhysics(), "Scenes", theScene);
		mPvdDataStream->destroyInstance(theScene);
	}
}

// PT: this is only called once, from "onPvdConnected"
void ScbScenePvdClient::sendEntireScene()
{
	NpScene* npScene = static_cast<NpScene*>(mScbScene.getPxScene());

	if(npScene->getFlagsFast() & PxSceneFlag::eREQUIRE_RW_LOCK) // getFlagsFast() will trigger a warning of lock check
		npScene->lockRead(__FILE__, __LINE__);

	PxPhysics& physics = PxGetPhysics();
	{
		PxScene* theScene = mScbScene.getPxScene();
		mPvdDataStream->createInstance(theScene);
		updatePvdProperties();

		// Create parent/child relationship.
		mPvdDataStream->setPropertyValue(theScene, "Physics", reinterpret_cast<const void*>(&physics));
		mPvdDataStream->pushBackObjectRef(&physics, "Scenes", theScene);
	}

	// materials:
	{
		PxsMaterialManager& manager = mScbScene.getScScene().getMaterialManager();
		PxsMaterialManagerIterator iter(manager);
		PxsMaterialCore* mat;
		while(iter.getNextMaterial(mat))
		{
			const PxMaterial* theMaterial = mat->getNxMaterial();
			if(mPvd->registerObject(theMaterial))
				mMetaDataBinding.createInstance(*mPvdDataStream, *theMaterial, physics);
		};
	}

	if(mPvd->getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
	{
		Ps::Array<PxActor*> actorArray;

		// RBs
		// static:
		{
			PxU32 numActors = npScene->getNbActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC);
			actorArray.resize(numActors);
			npScene->getActors(PxActorTypeFlag::eRIGID_STATIC | PxActorTypeFlag::eRIGID_DYNAMIC, actorArray.begin(),
				actorArray.size());
			for(PxU32 i = 0; i < numActors; i++)
			{
				PxActor* pxActor = actorArray[i];
				if(pxActor->is<PxRigidStatic>())
					mMetaDataBinding.createInstance(*mPvdDataStream, *static_cast<PxRigidStatic*>(pxActor), *npScene, physics, mPvd);
				else
					mMetaDataBinding.createInstance(*mPvdDataStream, *static_cast<PxRigidDynamic*>(pxActor), *npScene, physics, mPvd);
			}
		}
		// articulations & links
		{
			Ps::Array<PxArticulation*> articulations;
			PxU32 numArticulations = npScene->getNbArticulations();
			articulations.resize(numArticulations);
			npScene->getArticulations(articulations.begin(), articulations.size());
			for(PxU32 i = 0; i < numArticulations; i++)
				mMetaDataBinding.createInstance(*mPvdDataStream, *articulations[i], *npScene, physics, mPvd);
		}

#if PX_USE_PARTICLE_SYSTEM_API
		// particle systems & fluids:
		{
			PxU32 nbParticleSystems = mScbScene.getScScene().getNbParticleSystems();
			Sc::ParticleSystemCore* const* particleSystems = mScbScene.getScScene().getParticleSystems();
			for(PxU32 i = 0; i < nbParticleSystems; i++)
			{
				Sc::ParticleSystemCore* scParticleSystem = particleSystems[i];
				createPvdInstance(scParticleSystem->getPxParticleBase());
			}
		}
#endif

#if PX_USE_CLOTH_API
		// cloth
		{
			Ps::Array<PxActor*> cloths;
			PxU32 numActors = npScene->getNbActors(PxActorTypeFlag::eCLOTH);
			cloths.resize(numActors);
			npScene->getActors(PxActorTypeFlag::eCLOTH, cloths.begin(), cloths.size());
			for(PxU32 i = 0; i < numActors; i++)
			{
				Scb::Cloth* scbCloth = &static_cast<NpCloth*>(cloths[i])->getScbCloth();
				createPvdInstance(scbCloth);
			}
		}
#endif

		// joints
		{
			Sc::ConstraintCore*const * constraints = mScbScene.getScScene().getConstraints();
			PxU32 nbConstraints = mScbScene.getScScene().getNbConstraints();
			for(PxU32 i = 0; i < nbConstraints; i++)
			{
				updateConstraint(*constraints[i], PxPvdUpdateType::CREATE_INSTANCE);
				updateConstraint(*constraints[i], PxPvdUpdateType::UPDATE_ALL_PROPERTIES);
			}
		}
	}

	if(npScene->getFlagsFast() & PxSceneFlag::eREQUIRE_RW_LOCK)
		npScene->unlockRead();
}

void ScbScenePvdClient::updateConstraint(const Sc::ConstraintCore& scConstraint, PxU32 updateType)
{
	PxConstraintConnector* conn = scConstraint.getPxConnector();
	if(conn && checkPvdDebugFlag())	
		conn->updatePvdProperties(*mPvdDataStream, scConstraint.getPxConstraint(), PxPvdUpdateType::Enum(updateType));
}

void ScbScenePvdClient::createPvdInstance(const PxActor* actor)
{
	if(checkPvdDebugFlag())	
		ActorTypeOperation(actor, CreateOp(*mPvdDataStream, mMetaDataBinding, mPvd, *mScbScene.getPxScene()));
}

void ScbScenePvdClient::updatePvdProperties(const PxActor* actor)
{
	if(checkPvdDebugFlag())	
		ActorTypeOperation(actor, UpdateOp(*mPvdDataStream, mMetaDataBinding));
}

void ScbScenePvdClient::releasePvdInstance(const PxActor* actor)
{
	if(checkPvdDebugFlag())	
		ActorTypeOperation(actor, DestroyOp(*mPvdDataStream, mMetaDataBinding, *mScbScene.getPxScene()));
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Actor* actor)
{
	// PT: why not UPDATE_PVD_PROPERTIES_CHECK() here?
	createPvdInstance(getPxActor(actor));
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Actor* actor)
{
	// PT: why not UPDATE_PVD_PROPERTIES_CHECK() here?
	updatePvdProperties(getPxActor(actor));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Actor* actor)
{
	// PT: why not UPDATE_PVD_PROPERTIES_CHECK() here?
	releasePvdInstance(getPxActor(actor));
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Body* body)
{
	if(checkPvdDebugFlag() && body->getActorType() != PxActorType::eARTICULATION_LINK)	
		BodyTypeOperation(body, CreateOp(*mPvdDataStream, mMetaDataBinding, mPvd, *mScbScene.getPxScene()));
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Body* body)
{
	if(checkPvdDebugFlag())	
		BodyTypeOperation(body, UpdateOp(*mPvdDataStream, mMetaDataBinding));
}

void ScbScenePvdClient::updateKinematicTarget(const Scb::Body* body, const PxTransform& p)
{
	if(checkPvdDebugFlag())	
		mPvdDataStream->setPropertyValue(getNpRigidDynamic(body), "KinematicTarget", p);
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::RigidStatic* rigidStatic)
{	
	if(checkPvdDebugFlag())	
		mMetaDataBinding.createInstance(*mPvdDataStream, *getNpRigidStatic(rigidStatic), *mScbScene.getPxScene(), PxGetPhysics(), mPvd);
}

void ScbScenePvdClient::updatePvdProperties(const Scb::RigidStatic* rigidStatic)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *getNpRigidStatic(rigidStatic));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::RigidObject* rigidObject)
{
	releasePvdInstance(getPxActor(rigidObject));
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Constraint* constraint)
{
	if(checkPvdDebugFlag())	
		updateConstraint(constraint->getScConstraint(), PxPvdUpdateType::CREATE_INSTANCE);
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Constraint* constraint)
{
	if(checkPvdDebugFlag())	
		updateConstraint(constraint->getScConstraint(), PxPvdUpdateType::UPDATE_ALL_PROPERTIES);
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Constraint* constraint)
{	
	const Sc::ConstraintCore& scConstraint = constraint->getScConstraint();
	PxConstraintConnector* conn;
	if(checkPvdDebugFlag() && (conn = scConstraint.getPxConnector()) != NULL)
		conn->updatePvdProperties(*mPvdDataStream, scConstraint.getPxConstraint(), PxPvdUpdateType::RELEASE_INSTANCE);
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Articulation* articulation)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.createInstance(*mPvdDataStream, *getNpArticulation(articulation), *mScbScene.getPxScene(), PxGetPhysics(), mPvd);
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Articulation* articulation)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *getNpArticulation(articulation));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Articulation* articulation)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.destroyInstance(*mPvdDataStream, *getNpArticulation(articulation), *mScbScene.getPxScene());
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::ArticulationJoint* articulationJoint)
{
	PX_UNUSED(articulationJoint);
}

void ScbScenePvdClient::updatePvdProperties(const Scb::ArticulationJoint* articulationJoint)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *getNpArticulationJoint(articulationJoint));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::ArticulationJoint* articulationJoint)
{
	PX_UNUSED(articulationJoint);
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Sc::MaterialCore* materialCore)
{	
	if(checkPvdDebugFlag())	
	{
		const PxMaterial* theMaterial = materialCore->getNxMaterial();
		if(mPvd->registerObject(theMaterial))
			mMetaDataBinding.createInstance(*mPvdDataStream, *theMaterial, PxGetPhysics());
	}
}

void ScbScenePvdClient::updatePvdProperties(const Sc::MaterialCore* materialCore)
{
	if(checkPvdDebugFlag())	
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *materialCore->getNxMaterial());
}

void ScbScenePvdClient::releasePvdInstance(const Sc::MaterialCore* materialCore)
{
	if(checkPvdDebugFlag() && mPvd->unRegisterObject(materialCore->getNxMaterial() ) )
		mMetaDataBinding.destroyInstance(*mPvdDataStream, *materialCore->getNxMaterial(), PxGetPhysics());
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Shape* shape, PxActor& owner)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.createPVDInstance", getContextId(mScbScene));
		const PxShape* npShape = getNpShape(shape);
		mMetaDataBinding.createInstance(*mPvdDataStream, *npShape, static_cast<PxRigidActor&>(owner), PxGetPhysics(), mPvd);
	}
}

static void addShapesToPvd(PxU32 nbShapes, void* const* shapes, const size_t offset, PxActor& pxActor, PsPvd* pvd, PvdDataStream& stream, PvdMetaDataBinding& binding)
{
	PxPhysics& physics = PxGetPhysics();
	for(PxU32 i=0;i<nbShapes;i++)
	{
		const Scb::Shape* shape = reinterpret_cast<Scb::Shape*>(reinterpret_cast<char*>(shapes[i]) + offset);
		const PxShape* npShape = getNpShape(shape);
		binding.createInstance(stream, *npShape, static_cast<PxRigidActor&>(pxActor), physics, pvd);
	}
}

void ScbScenePvdClient::addBodyAndShapesToPvd(Scb::Body& b)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.createPVDInstance", getContextId(mScbScene));
		createPvdInstance(&b);

		const size_t offset = NpShapeGetScPtrOffset() - Scb::Shape::getScOffset();
		PxActor& pxActor = *b.getScBody().getPxActor();

		void* const* shapes;
		const PxU32 nbShapes = NpRigidDynamicGetShapes(b, shapes);
		addShapesToPvd(nbShapes, shapes, offset, pxActor, mPvd, *mPvdDataStream, mMetaDataBinding);
	}
}

void ScbScenePvdClient::addStaticAndShapesToPvd(Scb::RigidStatic& s)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.createPVDInstance", getContextId(mScbScene));
		createPvdInstance(&s);

		const size_t offset = NpShapeGetScPtrOffset() - Scb::Shape::getScOffset();
		PxActor& pxActor = *s.getScStatic().getPxActor();

		void* const* shapes;
		const PxU32 nbShapes = NpRigidStaticGetShapes(s, shapes);
		addShapesToPvd(nbShapes, shapes, offset, pxActor, mPvd, *mPvdDataStream, mMetaDataBinding);
	}
}

void ScbScenePvdClient::updateMaterials(const Scb::Shape* shape)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.updateMaterials(*mPvdDataStream, *getNpShape(const_cast<Scb::Shape*>(shape)), mPvd);
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Shape* shape)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *getNpShape(const_cast<Scb::Shape*>(shape)));
}

void ScbScenePvdClient::releaseAndRecreateGeometry(const Scb::Shape* shape)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.releaseAndRecreateGeometry(*mPvdDataStream, *getNpShape(const_cast<Scb::Shape*>(shape)),
		NpPhysics::getInstance(), mPvd);
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Shape* shape, PxActor& owner)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.releasePVDInstance", getContextId(mScbScene));

		const NpShape* npShape = getNpShape(shape);
		mMetaDataBinding.destroyInstance(*mPvdDataStream, *npShape, static_cast<PxRigidActor&>(owner));

		const PxU32 numMaterials = npShape->getNbMaterials();
		PX_ALLOCA(materialPtr, PxMaterial*, numMaterials);
		npShape->getMaterials(materialPtr, numMaterials);

		for(PxU32 idx = 0; idx < numMaterials; ++idx)
			releasePvdInstance(&(static_cast<NpMaterial*>(materialPtr[idx])->getScMaterial()));
	}
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::originShift(PxVec3 shift)
{
	mMetaDataBinding.originShift(*mPvdDataStream, mScbScene.getPxScene(), shift);
}

template <typename TPropertyType>
void ScbScenePvdClient::sendArray(const void* instance, const char* propName, const Cm::BitMap* bitMap,
	PxU32 nbValidParticles, PxStrideIterator<const TPropertyType>& iterator)
{
	PX_ASSERT(nbValidParticles > 0);
	if(!iterator.ptr())
		return;

	// setup the pvd array  PxParticleFlags
	pvdsdk::DataRef<const PxU8> propData;
	Ps::Array<PxU8> mTempU8Array;
	mTempU8Array.resize(nbValidParticles * sizeof(TPropertyType));
	TPropertyType* tmpArray = reinterpret_cast<TPropertyType*>(mTempU8Array.begin());
	propData = pvdsdk::DataRef<const PxU8>(mTempU8Array.begin(), mTempU8Array.size());

	PxU32 tIdx = 0;
	Cm::BitMap::Iterator it(*bitMap);
	for(PxU32 index = it.getNext(); index != Cm::BitMap::Iterator::DONE; index = it.getNext())
	{
		tmpArray[tIdx++] = iterator[index];
	}
	PX_ASSERT(tIdx == nbValidParticles);

	mPvdDataStream->setPropertyValue(instance, propName, propData,
		pvdsdk::getPvdNamespacedNameForType<TPropertyType>());
}

void ScbScenePvdClient::sendStateDatas(Sc::ParticleSystemCore* psCore)
{
	PX_UNUSED(psCore);
#if PX_USE_PARTICLE_SYSTEM_API

	if(!checkPvdDebugFlag())
		return;

	Scb::ParticleSystem* scbParticleSystem = getScbParticleSystem(psCore);
	bool doProcess = scbParticleSystem->getFlags() & PxParticleBaseFlag::eENABLED;
#if PX_SUPPORT_GPU_PHYSX
	doProcess &= (scbParticleSystem->getDeviceExclusiveAccessGpu() == NULL);
#endif
	if(doProcess)
	{
		Sc::ParticleSystemSim* particleSystem = psCore->getSim();
		Pt::ParticleSystemStateDataDesc stateData;
		particleSystem->getParticleState().getParticlesV(stateData, true, false);
		Pt::ParticleSystemSimDataDesc simParticleData;
		particleSystem->getSimParticleData(simParticleData, false);

		const PxActor* pxActor = getPxActor(scbParticleSystem);

		// mPvdDataStream->setPropertyValue( pxActor, "WorldBounds", psCore->getWorldBounds());
		mPvdDataStream->setPropertyValue(pxActor, "NbParticles", stateData.numParticles);
		mPvdDataStream->setPropertyValue(pxActor, "ValidParticleRange", stateData.validParticleRange);

		if(stateData.validParticleRange > 0)
		{
			mPvdDataStream->setPropertyValue(pxActor, "ValidParticleBitmap", stateData.bitMap->getWords(),
				(stateData.validParticleRange >> 5) + 1);
			sendArray<PxVec3>(pxActor, "Positions", stateData.bitMap, stateData.numParticles, stateData.positions);
			sendArray<PxVec3>(pxActor, "Velocities", stateData.bitMap, stateData.numParticles, stateData.velocities);
			sendArray<PxF32>(pxActor, "RestOffsets", stateData.bitMap, stateData.numParticles, stateData.restOffsets);
			sendArray<PxVec3>(pxActor, "CollisionNormals", stateData.bitMap, stateData.numParticles,
				simParticleData.collisionNormals);
			sendArray<PxF32>(pxActor, "Densities", stateData.bitMap, stateData.numParticles, simParticleData.densities);
			// todo: twoway data if need more particle retrieval

			{ // send PxParticleFlags, we have Pt::ParticleFlags here
				pvdsdk::DataRef<const PxU8> propData;
				Ps::Array<PxU8> mTempU8Array;
				mTempU8Array.resize(stateData.numParticles * sizeof(PxU16));
				PxU16* tmpArray = reinterpret_cast<PxU16*>(mTempU8Array.begin());
				propData = pvdsdk::DataRef<const PxU8>(mTempU8Array.begin(), mTempU8Array.size());

				PxU32 tIdx = 0;
				PxStrideIterator<const Pt::ParticleFlags>& iterator = stateData.flags;
				Cm::BitMap::Iterator it(*stateData.bitMap);
				for(PxU32 index = it.getNext(); index != Cm::BitMap::Iterator::DONE; index = it.getNext())
				{
					tmpArray[tIdx++] = iterator[index].api;
				}

				mPvdDataStream->setPropertyValue(pxActor, "Flags", propData,
					pvdsdk::getPvdNamespacedNameForType<PxU16>());
			}
		}
	}
#endif
}

void ScbScenePvdClient::frameStart(PxReal simulateElapsedTime)
{	
	PX_PROFILE_ZONE("Basic.pvdFrameStart", mScbScene.getContextId());

	if(!mIsConnected)
		return;

	mPvdDataStream->flushPvdCommand();
	mMetaDataBinding.sendBeginFrame(*mPvdDataStream, mScbScene.getPxScene(), simulateElapsedTime);
}

void ScbScenePvdClient::frameEnd()
{
	PX_PROFILE_ZONE("Basic.pvdFrameEnd", mScbScene.getContextId());

	if(!mIsConnected)
	{
		if(mPvd)
			mPvd->flush(); // Even if we aren't connected, we may need to flush buffered events.
		return;
	}

	PxScene* theScene = mScbScene.getPxScene();

	// Send the statistics for last frame.
	void* tmp = NULL;
#if PX_SUPPORT_GPU_PHYSX
	if(mScbScene.getScScene().getSceneGpu())
	{
		NpPhysics& npPhysics = static_cast<NpPhysics&>(theScene->getPhysics());
		PxTriangleMeshCacheStatistics triMeshCacheStats =
			npPhysics.getNpPhysicsGpu().getTriangleMeshCacheStatistics(*theScene);
		tmp = &triMeshCacheStats;
	}
#endif
	mMetaDataBinding.sendStats(*mPvdDataStream, theScene, tmp);

	if(mPvd->getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
	{
#if PX_USE_PARTICLE_SYSTEM_API
		// particle systems & fluids:
		{
			PX_PROFILE_ZONE("PVD.updatePariclesAndFluids", getContextId(mScbScene));
			PxU32 nbParticleSystems = mScbScene.getScScene().getNbParticleSystems();
			Sc::ParticleSystemCore* const* particleSystems = mScbScene.getScScene().getParticleSystems();
			for(PxU32 i = 0; i < nbParticleSystems; i++)
			{
				sendStateDatas(particleSystems[i]);
			}
		}
#endif

#if PX_USE_CLOTH_API
		{
			PX_PROFILE_ZONE("PVD.updateCloths", getContextId(mScbScene));
			mMetaDataBinding.updateCloths(*mPvdDataStream, *theScene);
		}
#endif
	}

	// flush our data to the main connection
	mPvd->flush();

	// End the frame *before* we send the dynamic object current data.
	// This ensures that contacts end up synced with the rest of the system.
	// Note that contacts were sent much earler in NpScene::fetchResults.
	mMetaDataBinding.sendEndFrame(*mPvdDataStream, mScbScene.getPxScene());

	if(mPvd->getInstrumentationFlags() & PxPvdInstrumentationFlag::eDEBUG)
	{
		PX_PROFILE_ZONE("PVD.sceneUpdate", getContextId(mScbScene));

		PvdVisualizer* vizualizer = NULL;
		const bool visualizeJoints = getScenePvdFlagsFast() & PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS;
		if(visualizeJoints)
			vizualizer = this;

		mMetaDataBinding.updateDynamicActorsAndArticulations(*mPvdDataStream, theScene, vizualizer);
	}

	// frame end moved to update contacts to have them in the previous frame.
}

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::createPvdInstance(const Scb::Aggregate* aggregate)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.createPVDInstance", getContextId(mScbScene));
		const NpAggregate* npAggregate = getNpAggregate(aggregate);
		mMetaDataBinding.createInstance(*mPvdDataStream, *npAggregate, *mScbScene.getPxScene());
	}
}

void ScbScenePvdClient::updatePvdProperties(const Scb::Aggregate* aggregate)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendAllProperties(*mPvdDataStream, *getNpAggregate(aggregate));
}

void ScbScenePvdClient::attachAggregateActor(const Scb::Aggregate* aggregate, Scb::Actor* actor)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.attachAggregateActor(*mPvdDataStream, *getNpAggregate(aggregate), *getPxActor(actor));
}

void ScbScenePvdClient::detachAggregateActor(const Scb::Aggregate* aggregate, Scb::Actor* actor)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.detachAggregateActor(*mPvdDataStream, *getNpAggregate(aggregate), *getPxActor(actor));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Aggregate* aggregate)
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.releasePVDInstance", getContextId(mScbScene));
		const NpAggregate* npAggregate = getNpAggregate(aggregate);
		mMetaDataBinding.destroyInstance(*mPvdDataStream, *npAggregate, *mScbScene.getPxScene());
	}
}

///////////////////////////////////////////////////////////////////////////////

#if PX_USE_CLOTH_API
static inline const PxCloth* toPx(const Scb::Cloth* cloth)
{
	const NpCloth* realCloth = getNpCloth(cloth);
	return static_cast<const PxCloth*>(realCloth);
}

void ScbScenePvdClient::createPvdInstance(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.createInstance(*mPvdDataStream, *getNpCloth(cloth), *mScbScene.getPxScene(), PxGetPhysics(), mPvd);
}

void ScbScenePvdClient::sendSimpleProperties(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendSimpleProperties(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendMotionConstraints(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendMotionConstraints(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendSelfCollisionIndices(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendSelfCollisionIndices(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendRestPositions(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendRestPositions(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendSeparationConstraints(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendSeparationConstraints(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendCollisionSpheres(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendCollisionSpheres(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendCollisionCapsules(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendCollisionSpheres(*mPvdDataStream, *toPx(cloth), true);
}

void ScbScenePvdClient::sendCollisionTriangles(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendCollisionTriangles(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendParticleAccelerations(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendParticleAccelerations(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::sendVirtualParticles(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.sendVirtualParticles(*mPvdDataStream, *toPx(cloth));
}

void ScbScenePvdClient::releasePvdInstance(const Scb::Cloth* cloth)
{
	if(checkPvdDebugFlag())
		mMetaDataBinding.destroyInstance(*mPvdDataStream, *toPx(cloth), *mScbScene.getPxScene());
}
#endif // PX_USE_CLOTH_API

///////////////////////////////////////////////////////////////////////////////

void ScbScenePvdClient::updateJoints()
{
	if(checkPvdDebugFlag())
	{
		PX_PROFILE_ZONE("PVD.updateJoints", getContextId(mScbScene));

		const bool visualizeJoints = getScenePvdFlagsFast() & PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS;

		Sc::ConstraintCore*const * constraints = mScbScene.getScScene().getConstraints();
		const PxU32 nbConstraints = mScbScene.getScScene().getNbConstraints();
		PxI64 constraintCount = 0;

		for(PxU32 i=0; i<nbConstraints; i++)
		{
			Sc::ConstraintCore* constraint = constraints[i];
			PxPvdUpdateType::Enum updateType = getNpConstraint(constraint)->isDirty()
				? PxPvdUpdateType::UPDATE_ALL_PROPERTIES
				: PxPvdUpdateType::UPDATE_SIM_PROPERTIES;
			updateConstraint(*constraint, updateType);
			PxConstraintConnector* conn = constraint->getPxConnector();
			// visualization is updated here
			{
				PxU32 typeId = 0;
				void* joint = NULL;
				if(conn)
					joint = conn->getExternalReference(typeId);
				// visualize:
				Sc::ConstraintSim* sim = constraint->getSim();
				if(visualizeJoints && sim && sim->getConstantsLL() && joint && constraint->getVisualize())
				{
					Sc::BodySim* b0 = sim->getBody(0);
					Sc::BodySim* b1 = sim->getBody(1);
					PxTransform t0 = b0 ? b0->getBody2World() : PxTransform(PxIdentity);
					PxTransform t1 = b1 ? b1->getBody2World() : PxTransform(PxIdentity);
					PvdConstraintVisualizer viz(joint, *mUserRender);
					(*constraint->getVisualize())(viz, sim->getConstantsLL(), t0, t1, 0xffffFFFF);
				}
			}
			++constraintCount;
		}

		mUserRender->flushRenderEvents();
	}
}

void ScbScenePvdClient::updateContacts()
{
	if(!checkPvdDebugFlag())
		return;

	PX_PROFILE_ZONE("PVD.updateContacts", getContextId(mScbScene));

	// if contacts are disabled, send empty array and return
	const PxScene* theScene(mScbScene.getPxScene());
	if(!(getScenePvdFlagsFast() & PxPvdSceneFlag::eTRANSMIT_CONTACTS))
	{
		mMetaDataBinding.sendContacts(*mPvdDataStream, *theScene);
		return;
	}

	PxsContactManagerOutputIterator outputIter;

	Sc::ContactIterator contactIter;
	mScbScene.getScScene().initContactsIterator(contactIter, outputIter);
	Sc::ContactIterator::Pair* pair;
	Sc::Contact* contact;
	Ps::Array<Sc::Contact> contacts;
	while ((pair = contactIter.getNextPair()) != NULL)
	{
		while ((contact = pair->getNextContact()) != NULL)
			contacts.pushBack(*contact);
	}
	
	mMetaDataBinding.sendContacts(*mPvdDataStream, *theScene, contacts);
}

void ScbScenePvdClient::updateSceneQueries()
{
	if(checkPvdDebugFlag() && (getScenePvdFlagsFast() & PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES))
		mMetaDataBinding.sendSceneQueries(*mPvdDataStream, *mScbScene.getPxScene(), mPvd);
}

void ScbScenePvdClient::setCreateContactReports(bool b)
{
	mScbScene.getScScene().setCreateContactReports(b);
}

void ScbScenePvdClient::visualize(PxArticulationLink& link)
{
	NpArticulationLink& npLink = static_cast<NpArticulationLink&>(link);
	const void* itemId = npLink.getInboundJoint();
	if(itemId && mUserRender)
	{
		PvdConstraintVisualizer viz(itemId, *mUserRender);
		npLink.visualizeJoint(viz);
	}
}

void ScbScenePvdClient::visualize(const PxRenderBuffer& debugRenderable)
{
	if(mUserRender)
	{
		// PT: I think the mUserRender object can contain extra data (including things coming from the user), because the various
		// draw functions are exposed e.g. in PxPvdSceneClient.h. So I suppose we have to keep the render buffer around regardless
		// of the connection flags. Thus I only skip the "drawRenderbuffer" call, for minimal intrusion into this file.
		if(checkPvdDebugFlag())
		{
			mUserRender->drawRenderbuffer(
				reinterpret_cast<const PvdDebugPoint*>(debugRenderable.getPoints()), debugRenderable.getNbPoints(),
				reinterpret_cast<const PvdDebugLine*>(debugRenderable.getLines()), debugRenderable.getNbLines(),
				reinterpret_cast<const PvdDebugTriangle*>(debugRenderable.getTriangles()), debugRenderable.getNbTriangles());
		}
		mUserRender->flushRenderEvents();
	}
}

#endif
