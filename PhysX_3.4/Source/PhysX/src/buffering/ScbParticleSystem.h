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

#ifndef PX_PHYSICS_SCB_PARTICLE_SYSTEM
#define PX_PHYSICS_SCB_PARTICLE_SYSTEM

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "ScParticleSystemCore.h"

#include "ScbActor.h"

#include "NpPhysics.h"
#include "NpParticleFluidReadData.h"

namespace physx
{
struct PxCudaReadWriteParticleBuffers;

namespace Scb
{
struct ParticleSystemBuffer : public Scb::ActorBuffer
{
	template <PxU32 I, PxU32 Dummy> struct Fns {};	// TODO : make the base class traits visible
	typedef Sc::ParticleSystemCore Core;
	typedef ParticleSystemBuffer Buf;
	enum { BF_Base = ActorBuffer::AttrCount };

	// Regular attributes

	SCB_REGULAR_ATTRIBUTE(BF_Base+2,	PxReal,					Stiffness)
	SCB_REGULAR_ATTRIBUTE(BF_Base+3,	PxReal,					Viscosity)
	SCB_REGULAR_ATTRIBUTE(BF_Base+4,	PxReal,					Damping)
	SCB_REGULAR_ATTRIBUTE(BF_Base+5,	PxVec3,					ExternalAcceleration)
	SCB_REGULAR_ATTRIBUTE(BF_Base+6,	PxPlane,				ProjectionPlane)
	SCB_REGULAR_ATTRIBUTE(BF_Base+7,	PxReal,					ParticleMass)
	SCB_REGULAR_ATTRIBUTE(BF_Base+8,	PxReal,					Restitution)
	SCB_REGULAR_ATTRIBUTE(BF_Base+9,	PxReal,					DynamicFriction)
	SCB_REGULAR_ATTRIBUTE(BF_Base+10,	PxReal,					StaticFriction)
	SCB_REGULAR_ATTRIBUTE(BF_Base+11,	PxFilterData,			SimulationFilterData)
	SCB_REGULAR_ATTRIBUTE(BF_Base+12,	PxParticleBaseFlags,	Flags)

	enum { BF_ResetFiltering	= 1<<(BF_Base+13)	};
};

class DebugIndexPool;

class ParticleSystem : public Scb::Actor
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================

	typedef Sc::ParticleSystemCore Core;
	typedef ParticleSystemBuffer Buf;

	struct UserBufferLock
	{
		UserBufferLock(NpParticleFluidReadData* db, const char* callerName) : dataBuffer(db)	{ if(dataBuffer) { dataBuffer->lock(callerName); } }
		~UserBufferLock()																		{ if(dataBuffer) { dataBuffer->unlock(); } }

		NpParticleFluidReadData* dataBuffer;
	private:
		UserBufferLock& operator=(const UserBufferLock&);
	};

#define LOCK_PARTICLE_USER_BUFFERS(callerName) UserBufferLock userBufferLock(mReadParticleFluidData, callerName);

	struct ForceUpdates
	{
		ForceUpdates() : map(NULL), values(NULL), hasUpdates(false) {}
		void initialize(PxU32 maxParticles);
		void destroy();

		PX_INLINE void add(PxU32 index, const PxVec3& value)
		{
			hasUpdates = true;
			if(!map->test(index))
			{
				map->set(index);
				values[index] = value;
				return;
			}
			values[index] += value;
		}

		PX_INLINE void clear(PxU32 index) 
		{ 
			PX_ASSERT(map); 
			map->reset(index); 
		}

		PX_INLINE void clear()
		{
			if(!hasUpdates)
				return;

			PX_ASSERT(map);
			map->clear();
			hasUpdates = false;
		}

		Cm::BitMap*		map; // can we make this an instance?
		PxVec3*			values;
		bool			hasUpdates;
	};

public:
// PX_SERIALIZATION
											ParticleSystem(const PxEMPTY) :	Scb::Actor(PxEmpty), mParticleSystem(PxEmpty) { mReadParticleFluidData = NULL; }
				void						exportExtraData(PxSerializationContext& stream) { mParticleSystem.exportExtraData(stream); }
				void						importExtraData(PxDeserializationContext& context) { mParticleSystem.importExtraData(context); }
				static void					getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION
											ParticleSystem(const PxActorType::Enum&, PxU32, bool);
											~ParticleSystem();

	//---------------------------------------------------------------------------------
	// Wrapper for Sc::ParticleSystemCore interface
	//---------------------------------------------------------------------------------
	PX_INLINE	PxParticleBase*				getPxParticleSystem();

	PX_INLINE	void						removeFromScene();

				bool						createParticles(const PxParticleCreationData& creationData);
				void						releaseParticles(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer);
				void						releaseParticles();
				void						setPositions(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
													 const PxStrideIterator<const PxVec3>& positionBuffer);
				void						setVelocities(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
													 const PxStrideIterator<const PxVec3>& velocitiesBuffer);
				void						setRestOffsets(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
													const PxStrideIterator<const PxF32>& restOffsetBuffer);
				void						addForces(PxU32 numParticles, const PxStrideIterator<const PxU32>& indexBuffer,
												  const PxStrideIterator<const PxVec3>& forceBuffer, PxForceMode::Enum forceMode);

	PX_INLINE	PxParticleReadData*			lockParticleReadData(PxDataAccessFlags flags);

	PX_INLINE	PxU32						getSimulationMethod() const;

	PX_INLINE 	PxReal						getStiffness() const							{ return read<Buf::BF_Stiffness>(); }
	PX_INLINE 	void 						setStiffness(PxReal v)							{ write<Buf::BF_Stiffness>(v);		}

	PX_INLINE 	PxReal						getViscosity() const							{ return read<Buf::BF_Viscosity>(); }
	PX_INLINE 	void 						setViscosity(PxReal v)							{ write<Buf::BF_Viscosity>(v);		}

	PX_INLINE 	PxReal						getDamping() const								{ return read<Buf::BF_Damping>();	}
	PX_INLINE 	void 						setDamping(PxReal v)							{ write<Buf::BF_Damping>(v);		}

	PX_INLINE 	PxVec3						getExternalAcceleration() const					{ return read<Buf::BF_ExternalAcceleration>(); }
	PX_INLINE 	void 						setExternalAcceleration(const PxVec3& v)		{ write<Buf::BF_ExternalAcceleration>(v);	}

	PX_INLINE 	PxPlane						getProjectionPlane() const						{ return read<Buf::BF_ProjectionPlane>(); }
	PX_INLINE 	void 						setProjectionPlane(const PxPlane& v)			{ write<Buf::BF_ProjectionPlane>(v); }

	PX_INLINE 	PxReal						getParticleMass() const							{ return read<Buf::BF_ParticleMass>(); }
	PX_INLINE 	void						setParticleMass(PxReal v)						{ write<Buf::BF_ParticleMass>(v); }

	PX_INLINE 	PxReal						getRestitution() const							{ return read<Buf::BF_Restitution>(); }
	PX_INLINE 	void 						setRestitution(PxReal v)						{ write<Buf::BF_Restitution>(v);	}

	PX_INLINE 	PxReal						getDynamicFriction() const						{ return read<Buf::BF_DynamicFriction>(); }
	PX_INLINE 	void 						setDynamicFriction(PxReal v)					{ write<Buf::BF_DynamicFriction>(v);}

	PX_INLINE 	PxReal						getStaticFriction() const						{ return read<Buf::BF_StaticFriction>(); }
	PX_INLINE 	void 						setStaticFriction(PxReal v)						{ write<Buf::BF_StaticFriction>(v); }

	PX_INLINE 	PxParticleBaseFlags			getFlags() const								{ return read<Buf::BF_Flags>();}
	PX_INLINE 	void						setFlags(PxParticleBaseFlags v)					{ write<Buf::BF_Flags>(v);		}

	PX_INLINE 	PxFilterData				getSimulationFilterData() const					{ return read<Buf::BF_SimulationFilterData>(); }
	PX_INLINE 	void						setSimulationFilterData(const PxFilterData& v)	{ write<Buf::BF_SimulationFilterData>(v); }
	
	PX_INLINE 	void						resetFiltering();

	PX_INLINE 	PxParticleReadDataFlags		getParticleReadDataFlags() const;
	PX_INLINE 	void						setParticleReadDataFlags(PxParticleReadDataFlags);

	PX_INLINE 	PxU32						getParticleCount() const;
	PX_INLINE	const Cm::BitMap&			getParticleMap() const;

	PX_INLINE 	PxU32 						getMaxParticles() const;

	PX_INLINE 	PxReal						getMaxMotionDistance() const;
	PX_INLINE 	void						setMaxMotionDistance(PxReal);
	PX_INLINE 	PxReal						getRestOffset() const;
	PX_INLINE 	void						setRestOffset(PxReal);
	PX_INLINE 	PxReal						getContactOffset() const;
	PX_INLINE 	void						setContactOffset(PxReal);
	PX_INLINE 	PxReal						getRestParticleDistance() const;
	PX_INLINE 	void						setRestParticleDistance(PxReal);
	PX_INLINE 	PxReal						getGridSize() const;
	PX_INLINE 	void						setGridSize(PxReal);

	PX_INLINE	PxBounds3					getWorldBounds() const;

	//---------------------------------------------------------------------------------
	// Data synchronization
	//---------------------------------------------------------------------------------
				
	// Synchronously called when the scene starts to be simulated.
	void	submitForceUpdates(PxReal timeStep);
	
	// Synchronously called with fetchResults.
	void	syncState();

	//---------------------------------------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------------------------------------
	PX_FORCE_INLINE	const Core&					getScParticleSystem()		const	{ return mParticleSystem; }  // Only use if you know what you're doing!
	PX_FORCE_INLINE	Core&						getScParticleSystem()				{ return mParticleSystem; }  // Only use if you know what you're doing!

	PX_FORCE_INLINE static const ParticleSystem&fromSc(const Core& a)	{ return static_cast<const ParticleSystem&>(Actor::fromSc(a)); }
	PX_FORCE_INLINE static ParticleSystem&		fromSc(Core &a)			{ 
	
		Scb::Actor& actor = Actor::fromSc(a);
		ParticleSystem& ps = static_cast<ParticleSystem&>(actor);
		PX_UNUSED(ps);

		return static_cast<ParticleSystem&>(Actor::fromSc(a)); 
	}

	static size_t getScOffset()	{ return reinterpret_cast<size_t>(&reinterpret_cast<ParticleSystem*>(0)->mParticleSystem);	}

#if PX_SUPPORT_GPU_PHYSX
	PX_INLINE	void enableDeviceExclusiveModeGpu();
	PX_INLINE	PxParticleDeviceExclusiveAccess* getDeviceExclusiveAccessGpu() const;
#endif

private:
	Core							mParticleSystem;

	NpParticleFluidReadData*		mReadParticleFluidData;


	ForceUpdates					mForceUpdatesAcc;
	ForceUpdates					mForceUpdatesVel;

	PX_FORCE_INLINE	const Scb::ParticleSystemBuffer*	getParticleSystemBuffer()	const	{ return reinterpret_cast<const Scb::ParticleSystemBuffer*>(getStream());	}
	PX_FORCE_INLINE	Scb::ParticleSystemBuffer*			getParticleSystemBuffer()			{ return reinterpret_cast<Scb::ParticleSystemBuffer*>(getStream());		}

	//---------------------------------------------------------------------------------
	// Infrastructure for regular attributes
	//---------------------------------------------------------------------------------

	struct Access: public BufferedAccess<Buf, Core, ParticleSystem> {};
	template<PxU32 f> PX_FORCE_INLINE typename Buf::Fns<f,0>::Arg read() const		{	return Access::read<Buf::Fns<f, 0> >(*this, mParticleSystem);	}
	template<PxU32 f> PX_FORCE_INLINE void write(typename Buf::Fns<f,0>::Arg v)		{	Access::write<Buf::Fns<f, 0> >(*this, mParticleSystem, v);		}
	template<PxU32 f> PX_FORCE_INLINE void flush(const Buf& buf)					{	Access::flush<Buf::Fns<f, 0> >(*this, mParticleSystem, buf);	}
};

PX_INLINE PxParticleBase* ParticleSystem::getPxParticleSystem()
{
	return mParticleSystem.getPxParticleBase();
}

PX_INLINE void ParticleSystem::removeFromScene()
{
	PX_ASSERT(!isBuffering() || getControlState()==ControlState::eREMOVE_PENDING);

	mForceUpdatesAcc.destroy();
	mForceUpdatesVel.destroy();
}

PX_INLINE PxParticleReadData* ParticleSystem::lockParticleReadData(PxDataAccessFlags flags)
{
	// Don't use the macro here since releasing the lock should not be done automatically but by the user
	if(isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, "Particle data read not allowed while simulation is running.");
		return NULL;
	}

	if(!mReadParticleFluidData)
		mReadParticleFluidData = PX_NEW(NpParticleFluidReadData)();

	mReadParticleFluidData->lock("PxParticleBase::lockParticleReadData()");
	mReadParticleFluidData->setDataAccessFlags(flags);
	mParticleSystem.getParticleReadData(*mReadParticleFluidData);		
	return mReadParticleFluidData;
}

PX_INLINE void ParticleSystem::resetFiltering()
{
	if(!isBuffering())
	{
		mParticleSystem.resetFiltering();
		UPDATE_PVD_PROPERTIES_OBJECT()
	}
	else
		markUpdated(Buf::BF_ResetFiltering);
}

PX_INLINE PxParticleReadDataFlags ParticleSystem::getParticleReadDataFlags() const
{
	return mParticleSystem.getParticleReadDataFlags();
}

PX_INLINE void ParticleSystem::setParticleReadDataFlags(PxParticleReadDataFlags flags)
{
	if(!isBuffering())
	{
		mParticleSystem.setParticleReadDataFlags(flags);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}	
}

PX_INLINE PxU32 ParticleSystem::getParticleCount() const
{
	return mParticleSystem.getParticleCount();
}

PX_INLINE const Cm::BitMap& ParticleSystem::getParticleMap() const
{
	return mParticleSystem.getParticleMap();
}

PX_INLINE PxU32 ParticleSystem::getMaxParticles() const
{
	return mParticleSystem.getMaxParticles();
}

PX_INLINE PxReal ParticleSystem::getMaxMotionDistance() const
{
	return mParticleSystem.getMaxMotionDistance();
}

PX_INLINE void ParticleSystem::setMaxMotionDistance(PxReal distance) 
{	
	if(!isBuffering())
	{
		mParticleSystem.setMaxMotionDistance(distance);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}	
}

PX_INLINE PxReal ParticleSystem::getRestOffset() const
{
	return mParticleSystem.getRestOffset();
}

PX_INLINE void ParticleSystem::setRestOffset(PxReal restOffset) 
{	
	if(!isBuffering())
	{
		mParticleSystem.setRestOffset(restOffset);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}
}

PX_INLINE PxReal ParticleSystem::getContactOffset() const
{
	return mParticleSystem.getContactOffset();
}

PX_INLINE void ParticleSystem::setContactOffset(PxReal contactOffset) 
{	
	if(!isBuffering())
	{
		mParticleSystem.setContactOffset(contactOffset);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}	
}

PX_INLINE PxReal ParticleSystem::getRestParticleDistance() const
{
	return mParticleSystem.getRestParticleDistance();
}

PX_INLINE void ParticleSystem::setRestParticleDistance(PxReal restParticleDistance) 
{	
	if(!isBuffering())
	{
		mParticleSystem.setRestParticleDistance(restParticleDistance);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}	
}

PX_INLINE PxReal ParticleSystem::getGridSize() const
{
	return mParticleSystem.getGridSize();
}

PX_INLINE void ParticleSystem::setGridSize(PxReal gridSize)
{	 
	if(!isBuffering())
	{
		mParticleSystem.setGridSize(gridSize);
		UPDATE_PVD_PROPERTIES_OBJECT()
	}	
}

PX_INLINE PxBounds3 ParticleSystem::getWorldBounds() const
{
	if(isBuffering())
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, 
				"PxActor::getWorldBounds(): Can't access particle world bounds during simulation without enabling bulk buffering.");
		return PxBounds3();
	}
	
	return mParticleSystem.getWorldBounds();
}

#if PX_SUPPORT_GPU_PHYSX

PX_INLINE void ParticleSystem::enableDeviceExclusiveModeGpu()
{
	mParticleSystem.enableDeviceExclusiveModeGpu();
}

PX_INLINE PxParticleDeviceExclusiveAccess* ParticleSystem::getDeviceExclusiveAccessGpu() const
{
	if(getFlags() & PxParticleBaseFlag::eGPU)
		return mParticleSystem.getDeviceExclusiveAccessGpu();

	return NULL;
}

#endif

}  // namespace Scb

}

#endif // PX_USE_PARTICLE_SYSTEM_API

#endif
