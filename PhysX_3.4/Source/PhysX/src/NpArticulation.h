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


#ifndef PX_PHYSICS_NP_ARTICULATION
#define PX_PHYSICS_NP_ARTICULATION

#include "PxArticulation.h"
#include "CmPhysXCommon.h"

#if PX_ENABLE_DEBUG_VISUALIZATION
#include "CmRenderOutput.h"
#endif

#include "ScbArticulation.h"
#include "NpArticulationLink.h"

namespace physx
{

class NpArticulationLink;
class NpScene;
class NpAggregate;
class PxAggregate;


class NpArticulation : public PxArticulation, public Ps::UserAllocated
{
//= ATTENTION! =====================================================================================
// Changing the data layout of this class breaks the binary serialization format.  See comments for 
// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
// accordingly.
//==================================================================================================
public:
	virtual										~NpArticulation();

// PX_SERIALIZATION
												NpArticulation(PxBaseFlags baseFlags) :	PxArticulation(baseFlags), mArticulation(PxEmpty), mArticulationLinks(PxEmpty)	{}
	virtual			void						exportExtraData(PxSerializationContext& stream);
					void						importExtraData(PxDeserializationContext& context);
					void						resolveReferences(PxDeserializationContext& context);
	virtual	        void						requiresObjects(PxProcessPxBaseCallback& c);
	static			NpArticulation*				createObject(PxU8*& address, PxDeserializationContext& context);
	static			void						getBinaryMetaData(PxOutputStream& stream);
//~PX_SERIALIZATION

	//---------------------------------------------------------------------------------
	// PxArticulation implementation
	//---------------------------------------------------------------------------------
	virtual			void						release();

	virtual			PxScene*					getScene() const;

	virtual			PxU32						getInternalDriveIterations() const;
	virtual			void						setInternalDriveIterations(PxU32 iterations);

	virtual			PxU32						getExternalDriveIterations() const;
	virtual			void						setExternalDriveIterations(PxU32 iterations);

	virtual			PxU32						getMaxProjectionIterations() const;
	virtual			void						setMaxProjectionIterations(PxU32 iterations);

	virtual			PxReal						getSeparationTolerance() const;
	virtual			void						setSeparationTolerance(PxReal tolerance);

	virtual			void						setSleepThreshold(PxReal threshold);
	virtual			PxReal						getSleepThreshold() const;

	virtual			void						setStabilizationThreshold(PxReal threshold);
	virtual			PxReal						getStabilizationThreshold() const;

	virtual			void						setWakeCounter(PxReal wakeCounterValue);
	virtual			PxReal						getWakeCounter() const;

	virtual			void						setSolverIterationCounts(PxU32 positionIters, PxU32 velocityIters);
	virtual			void						getSolverIterationCounts(PxU32 & positionIters, PxU32 & velocityIters) const;

	virtual			bool						isSleeping() const;
	virtual			void						wakeUp();
	virtual			void						putToSleep();

	virtual			PxArticulationLink*			createLink(PxArticulationLink* parent, const PxTransform& pose);

	virtual			PxU32						getNbLinks() const;
	virtual			PxU32						getLinks(PxArticulationLink** userBuffer, PxU32 bufferSize, PxU32 startIndex) const;

	virtual			PxBounds3					getWorldBounds(float inflation=1.01f) const;

	virtual			PxAggregate*				getAggregate() const;

	// Debug name
	virtual			void						setName(const char*);
	virtual			const char*					getName() const;

	virtual		PxArticulationDriveCache* 
								createDriveCache(PxReal compliance, PxU32 driveIterations) const;

	virtual		void			updateDriveCache(PxArticulationDriveCache& cache, PxReal compliance, PxU32 driveIterations) const;

	virtual		void			releaseDriveCache(PxArticulationDriveCache&) const;

	virtual		void			applyImpulse(PxArticulationLink*,
											 const PxArticulationDriveCache& driveCache,
											 const PxVec3& force,
											 const PxVec3& torque);

	virtual		void			computeImpulseResponse(PxArticulationLink*,
													   PxVec3& linearResponse, 
													   PxVec3& angularResponse,
													   const PxArticulationDriveCache& driveCache,
													   const PxVec3& force,
													   const PxVec3& torque) const;




	//---------------------------------------------------------------------------------
	// Miscellaneous
	//---------------------------------------------------------------------------------
												NpArticulation();

	PX_INLINE		void						addToLinkList(NpArticulationLink& link)				{ mArticulationLinks.pushBack(&link); }
					bool						removeLinkFromList(NpArticulationLink& link)		{ PX_ASSERT(mArticulationLinks.find(&link) != mArticulationLinks.end()); return mArticulationLinks.findAndReplaceWithLast(&link); }
	PX_FORCE_INLINE	NpArticulationLink* const*	getLinks()											{ return &mArticulationLinks.front(); }

					NpArticulationLink*			getRoot();

	PX_FORCE_INLINE	const Scb::Articulation&	getArticulation()							const	{ return mArticulation; }
	PX_FORCE_INLINE	Scb::Articulation&			getArticulation()									{ return mArticulation; }

					NpScene*					getAPIScene() const;
					NpScene*					getOwnerScene() const;		// the scene the user thinks the actor is in, or from which the actor is pending removal

	PX_FORCE_INLINE	void						setAggregate(NpAggregate* a) { mAggregate = a; }

					void						wakeUpInternal(bool forceWakeUp, bool autowake);

	PX_FORCE_INLINE	Scb::Articulation&			getScbArticulation()			{ return mArticulation; }
	PX_FORCE_INLINE	const Scb::Articulation&	getScbArticulation() const		{ return mArticulation; }


#if PX_ENABLE_DEBUG_VISUALIZATION
public:
					void						visualize(Cm::RenderOutput& out, NpScene* scene);
#endif

private:
					Scb::Articulation 			mArticulation;
					NpArticulationLinkArray		mArticulationLinks;
					NpAggregate*				mAggregate;
					const char*					mName;
};


}

#endif
