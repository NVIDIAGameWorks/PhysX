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

#include "ScNPhaseCore.h"
#include "ScShapeInteraction.h"
#include "ScTriggerInteraction.h"
#include "ScElementInteractionMarker.h"
#include "ScConstraintInteraction.h"
#include "ScSimStats.h"
#include "ScObjectIDTracker.h"
#include "ScActorElementPair.h"
#include "ScSimStats.h"

#if PX_USE_PARTICLE_SYSTEM_API
	#include "ScParticleSystemCore.h"
	#include "ScParticleBodyInteraction.h"
	#include "ScParticlePacketShape.h"
	#include "GuOverlapTests.h"
	#include "GuBox.h"
#endif

#if PX_USE_CLOTH_API
	#include "ScClothCore.h"
	#include "ScClothSim.h"
#endif // PX_USE_CLOTH_API

#include "PsThread.h"
#include "BpBroadPhase.h"

using namespace physx;
using namespace Sc;
using namespace Gu;

struct Sc::FilterPair
{
	enum Enum
	{
		ELEMENT_ELEMENT = 0,
		ELEMENT_ACTOR = 1,
		INVALID = 2						// Make sure this is the last one
	};

	template<typename T> T*getPtr()	const {	return reinterpret_cast<T*>(ptrAndType&~3); }
	Enum getType()					const {	return Enum(ptrAndType&3); }

private:
	FilterPair(void* ptr = 0, Enum type = INVALID) { uintptr_t p = reinterpret_cast<uintptr_t>(ptr); PX_ASSERT(!(p&3)); ptrAndType = p|type; }
	uintptr_t ptrAndType;	// packed type
	friend class FilterPairManager;
};

class Sc::FilterPairManager : public Ps::UserAllocated
{
	PX_NOCOPY(FilterPairManager)
public:
	FilterPairManager()
	: mPairs(PX_DEBUG_EXP("FilterPairManager Array"))
	, mFree(INVALID_FILTER_PAIR_INDEX)
	{}

	PxU32 acquireIndex()
	{
		PxU32 index;
		if(mFree == INVALID_FILTER_PAIR_INDEX)
		{
			index = mPairs.size();
			mPairs.pushBack(FilterPair());
		}
		else
		{
			index = PxU32(mFree);
			mFree = mPairs[index].ptrAndType;
			mPairs[index] = FilterPair();
		}
		return index;
	}

	void releaseIndex(PxU32 index)
	{		
		mPairs[index].ptrAndType = mFree;
		mFree = index;
	}

	void setPair(PxU32 index, Sc::ElementSimInteraction* ptr, FilterPair::Enum type)
	{
		mPairs[index] = FilterPair(ptr, type);
	}

	FilterPair&	operator[](PxU32 index)
	{
		return mPairs[index];
	}

	PxU32 findIndex(Sc::ElementSimInteraction* ptr)
	{
		return ptr->getFilterPairIndex();
	}

private:	
	typedef Ps::HashMap<void*, PxU32> InteractionToIndex;

	Ps::Array<FilterPair> mPairs;	
	uintptr_t mFree;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sc::NPhaseCore::NPhaseCore(Scene& scene, const PxSceneDesc& sceneDesc) :
	mOwnerScene						(scene),
	mContactReportActorPairSet		(PX_DEBUG_EXP("contactReportPairSet")),
	mPersistentContactEventPairList	(PX_DEBUG_EXP("persistentContactEventPairs")),
	mNextFramePersistentContactEventPairIndex(0),
	mForceThresholdContactEventPairList	(PX_DEBUG_EXP("forceThresholdContactEventPairs")),
	mContactReportBuffer			(sceneDesc.contactReportStreamBufferSize, (sceneDesc.flags & PxSceneFlag::eDISABLE_CONTACT_REPORT_BUFFER_RESIZE)),
	mActorPairPool					(PX_DEBUG_EXP("actorPairPool")),
	mActorPairReportPool			(PX_DEBUG_EXP("actorPairReportPool")),
	mActorElementPairPool			(PX_DEBUG_EXP("actorElementPool")),
	mShapeInteractionPool			(Ps::AllocatorTraits<ShapeInteraction>::Type(PX_DEBUG_EXP("shapeInteractionPool")), 256),
	mTriggerInteractionPool			(PX_DEBUG_EXP("triggerInteractionPool")),
	mActorPairContactReportDataPool	(PX_DEBUG_EXP("actorPairContactReportPool")),
	mInteractionMarkerPool			(PX_DEBUG_EXP("interactionMarkerPool"))
#if PX_USE_PARTICLE_SYSTEM_API
	,mParticleBodyPool				(PX_DEBUG_EXP("particleBodyPool"))
#endif
#if PX_USE_CLOTH_API
	,mClothPool						(PX_DEBUG_EXP("clothPool"))
#endif
	,mMergeProcessedTriggerInteractions (scene.getContextId(), this, "ScNPhaseCore.mergeProcessedTriggerInteractions")
	,mTmpTriggerProcessingBlock		(NULL)
	,mTriggerPairsToDeactivateCount	(0)
{
	mFilterPairManager = PX_NEW(FilterPairManager);
}

Sc::NPhaseCore::~NPhaseCore()
{
	// Clear pending actor pairs (waiting on contact report callback)
	clearContactReportActorPairs(false);
	PX_DELETE(mFilterPairManager);
}

PxU32 Sc::NPhaseCore::getDefaultContactReportStreamBufferSize() const
{
	return mContactReportBuffer.getDefaultBufferSize();
}

Sc::ElementSimInteraction* Sc::NPhaseCore::findInteraction(ElementSim* _element0, ElementSim* _element1)
{
	const Ps::HashMap<ElementSimKey, ElementSimInteraction*>::Entry* pair = mElementSimMap.find(ElementSimKey(_element0, _element1));
	return pair ? pair->second : NULL;
}

PxFilterInfo Sc::NPhaseCore::onOverlapFilter(ElementSim* volume0, ElementSim* volume1)
{
	PX_ASSERT(!findInteraction(volume0, volume1));

/*	if(0)
	{
		(void)volume0;
		(void)volume1;
		(void)pair;
		PxFilterInfo finfo;
		finfo.filterFlags = PxFilterFlag::eKILL;
		return finfo;
	}*/

	PX_ASSERT(PxMax(volume0->getElementType(), volume1->getElementType()) == ElementType::eSHAPE);

	ShapeSim* s0 = static_cast<ShapeSim*>(volume1);
	ShapeSim* s1 = static_cast<ShapeSim*>(volume0);

	// No actor internal interactions
	PX_ASSERT(&s0->getActor() != &s1->getActor());

	PxU32 isTriggerPair = 0;
	const PxFilterInfo finfo = filterRbCollisionPair(*s0, *s1, INVALID_FILTER_PAIR_INDEX, isTriggerPair, false);

	return finfo;
}

Sc::Interaction* Sc::NPhaseCore::onOverlapCreated(ElementSim* volume0, ElementSim* volume1, const PxU32 ccdPass)
{
#if PX_USE_PARTICLE_SYSTEM_API
	PX_COMPILE_TIME_ASSERT(ElementType::eSHAPE < ElementType::ePARTICLE_PACKET);
#elif PX_USE_CLOTH_API
	PX_COMPILE_TIME_ASSERT(ElementType::eSHAPE < ElementType::eCLOTH);
	PX_UNUSED(ccdPass);
#else
	PX_UNUSED(ccdPass);
#endif

	PX_ASSERT(!findInteraction(volume0, volume1));

	ElementSim* volumeLo = volume0;
	ElementSim* volumeHi = volume1;

	// PT: we already order things here? So why do we also have a swap in contact managers?
	// PT: TODO: use a map here again
	if (volumeLo->getElementType() > volumeHi->getElementType())
	{
		volumeLo = volume1;
		volumeHi = volume0;
	}

	switch (volumeHi->getElementType())
	{
#if PX_USE_PARTICLE_SYSTEM_API
		case ElementType::ePARTICLE_PACKET:
			{
				ParticlePacketShape* shapeHi = static_cast<ParticlePacketShape*>(volumeHi);

				if (volumeLo->getElementType() != ElementType::eSHAPE)
					break;	// Only interactions with rigid body shapes are supported

				ShapeSim* shapeLo = static_cast<ShapeSim*>(volumeLo);

				if (shapeLo->getActor().isDynamicRigid()
					&& !(shapeHi->getParticleSystem().getCore().getFlags() & PxParticleBaseFlag::eCOLLISION_WITH_DYNAMIC_ACTORS))
					return NULL;	// Skip dynamic rigids if corresponding flag is set on the particle system

				{
					const PxGeometryType::Enum geoType = shapeLo->getGeometryType();
					if (geoType == PxGeometryType::eTRIANGLEMESH || geoType == PxGeometryType::eHEIGHTFIELD)
					{
						PxBounds3 particleShapeBounds;
						shapeHi->computeWorldBounds(particleShapeBounds);
						bool isIntersecting = false;
						switch (geoType)
						{
							case PxGeometryType::eTRIANGLEMESH:
							{
								PX_ALIGN(16, PxTransform globalPose);
								shapeLo->getAbsPoseAligned(&globalPose);

								isIntersecting = Gu::checkOverlapAABB_triangleGeom(shapeLo->getCore().getGeometry(), globalPose, particleShapeBounds);
							}
							break;
							case PxGeometryType::eHEIGHTFIELD:
							{
								PX_ALIGN(16, PxTransform globalPose);
								shapeLo->getAbsPoseAligned(&globalPose);

								isIntersecting = Gu::checkOverlapAABB_heightFieldGeom(shapeLo->getCore().getGeometry(), globalPose, particleShapeBounds);
							}
							break;
							case PxGeometryType::eSPHERE:
							case PxGeometryType::ePLANE:
							case PxGeometryType::eCAPSULE:
							case PxGeometryType::eBOX:
							case PxGeometryType::eCONVEXMESH:
							case PxGeometryType::eGEOMETRY_COUNT:
							case PxGeometryType::eINVALID:
								break;
						}

						if (!isIntersecting)
							return NULL;
					}
					return createParticlePacketBodyInteraction(*shapeHi, *shapeLo, ccdPass);
				}
			}
#endif	// PX_USE_PARTICLE_SYSTEM_API

#if PX_USE_CLOTH_API
		case ElementType::eCLOTH:
            {
				if (volumeLo->getElementType() != ElementType::eSHAPE)
                    break;	// Only interactions with rigid body shapes are supported

				ClothShape* shapeHi = static_cast<ClothShape*>(volumeHi);
				ClothSim& clothSim = shapeHi->getClothSim();
                ClothCore& clothCore = clothSim.getCore();

				if(~clothCore.getClothFlags() & PxClothFlag::eSCENE_COLLISION)
					return NULL;

				ShapeSim* shapeLo = static_cast<ShapeSim*>(volumeLo);

				PxFilterInfo finfo = runFilter(*shapeHi, *shapeLo, INVALID_FILTER_PAIR_INDEX, true);
				if (finfo.filterFlags & (PxFilterFlag::eKILL | PxFilterFlag::eSUPPRESS))  // those are treated the same for cloth
				{
					PX_ASSERT(finfo.filterPairIndex == INVALID_FILTER_PAIR_INDEX);  // No filter callback pair info for killed pairs
					return NULL;
				}

				if(clothSim.addCollisionShape(shapeLo))
				{
					// only add an element when the collision shape wasn't rejected
					// due to hitting the sphere/plane limit of the cloth.
					ClothListElement element(&clothSim, mClothOverlaps[shapeLo].mNext);
					mClothOverlaps[shapeLo].mNext = mClothPool.construct(element);
				}
				return NULL;
            }
#endif // PX_USE_CLOTH_API

		case ElementType::eSHAPE:
			{
				ShapeSim* shapeHi = static_cast<ShapeSim*>(volumeHi);
				ShapeSim* shapeLo = static_cast<ShapeSim*>(volumeLo);

				// No actor internal interactions
				PX_ASSERT(&shapeHi->getActor() != &shapeLo->getActor());

				return createRbElementInteraction(*shapeHi, *shapeLo, NULL, NULL, NULL);
			}
		case ElementType::eCOUNT:
			PX_ASSERT(0);
			break;
	}
	return NULL;
}

static PX_FORCE_INLINE void prefetchPairElements(const Bp::AABBOverlap& pair, const ElementSim** elementBuffer)
{
	const ElementSim* e0 = reinterpret_cast<const ElementSim*>(pair.mUserData0);
	const ElementSim* e1 = reinterpret_cast<const ElementSim*>(pair.mUserData1);

	Ps::prefetchLine(e0);
	Ps::prefetchLine(e1);

	*elementBuffer = e0;
	*(elementBuffer+1) = e1;
}

static PX_FORCE_INLINE void prefetchPairActors(const ElementSim& e0, const ElementSim& e1, const ActorSim** actorBuffer)
{
	const ActorSim* a0 = static_cast<const ActorSim*>(&e0.getActor());
	const ActorSim* a1 = static_cast<const ActorSim*>(&e1.getActor());

	Ps::prefetchLine(a0);
	Ps::prefetchLine(reinterpret_cast<const PxU8*>(a0) + 128);
	Ps::prefetchLine(a1);
	Ps::prefetchLine(reinterpret_cast<const PxU8*>(a1) + 128);

	*actorBuffer = a0;
	*(actorBuffer+1) = a1;
}

static PX_FORCE_INLINE void prefetchPairShapesCore(const ElementSim& e0, const ElementSim& e1)
{
	if (e0.getElementType() == ElementType::eSHAPE)
	{
		const ShapeCore* sc = &static_cast<const ShapeSim&>(e0).getCore();
		Ps::prefetchLine(sc);
		Ps::prefetchLine(reinterpret_cast<const PxU8*>(sc) + 128);
	}
	if (e1.getElementType() == ElementType::eSHAPE)
	{
		const ShapeCore* sc = &static_cast<const ShapeSim&>(e1).getCore();
		Ps::prefetchLine(sc);
		Ps::prefetchLine(reinterpret_cast<const PxU8*>(sc) + 128);
	}
}

static PX_FORCE_INLINE void prefetchPairActorsCore(const ActorSim& a0, const ActorSim& a1)
{
	ActorCore* ac0 = &a0.getActorCore();
	Ps::prefetchLine(ac0);
	Ps::prefetchLine((reinterpret_cast<PxU8*>(ac0)) + 128);
	ActorCore* ac1 = &a1.getActorCore();
	Ps::prefetchLine(ac1);
	Ps::prefetchLine((reinterpret_cast<PxU8*>(ac1)) + 128);
}

static PX_FORCE_INLINE Sc::Interaction* processElementPair(Sc::NPhaseCore& nPhaseCore, const Bp::AABBOverlap& pair, const PxU32 ccdPass)
{
	ElementSim* e0 = reinterpret_cast<ElementSim*>(pair.mUserData0);
	ElementSim* e1 = reinterpret_cast<ElementSim*>(pair.mUserData1);
	return nPhaseCore.onOverlapCreated(e0, e1, ccdPass);
}

void Sc::NPhaseCore::onOverlapCreated(const Bp::AABBOverlap* PX_RESTRICT pairs, PxU32 pairCount, const PxU32 ccdPass)
{
#if PX_USE_PARTICLE_SYSTEM_API
	PX_COMPILE_TIME_ASSERT(ElementType::eSHAPE < ElementType::ePARTICLE_PACKET);
#elif PX_USE_CLOTH_API
	PX_COMPILE_TIME_ASSERT(ElementType::eSHAPE < ElementType::eCLOTH);
#endif

	PxU32 pairIdx = 0;

	const PxU32 prefetchLookAhead = 4;
	const ElementSim* batchedElements[prefetchLookAhead * 2];
	const ActorSim* batchedActors[prefetchLookAhead * 2];

	PxU32 batchIterCount = pairCount / prefetchLookAhead;

	for(PxU32 i=1; i < batchIterCount; i++)
	{
		// prefetch elements for next batch
		{
			PX_ASSERT(prefetchLookAhead >= 4);
			prefetchPairElements(pairs[pairIdx + prefetchLookAhead], batchedElements);
			prefetchPairElements(pairs[pairIdx + prefetchLookAhead + 1], batchedElements + 2);
			prefetchPairElements(pairs[pairIdx + prefetchLookAhead + 2], batchedElements + 4);
			prefetchPairElements(pairs[pairIdx + prefetchLookAhead + 3], batchedElements + 6);
			// unrolling by hand leads to better perf on XBox
		}

		processElementPair(*this, pairs[pairIdx + 0], ccdPass);

		// prefetch actor sim for next batch
		{
			PX_ASSERT(prefetchLookAhead >= 4);
			prefetchPairActors(*batchedElements[0], *batchedElements[1], batchedActors);
			prefetchPairActors(*batchedElements[2], *batchedElements[3], batchedActors + 2);
			prefetchPairActors(*batchedElements[4], *batchedElements[5], batchedActors + 4);
			prefetchPairActors(*batchedElements[6], *batchedElements[7], batchedActors + 6);
			// unrolling by hand leads to better perf on XBox
		}

		processElementPair(*this, pairs[pairIdx + 1], ccdPass);

		// prefetch shape core for next batch
		{
			PX_ASSERT(prefetchLookAhead >= 4);
			prefetchPairShapesCore(*batchedElements[0], *batchedElements[1]);
			prefetchPairShapesCore(*batchedElements[2], *batchedElements[3]);
			prefetchPairShapesCore(*batchedElements[4], *batchedElements[5]);
			prefetchPairShapesCore(*batchedElements[6], *batchedElements[7]);
			// unrolling by hand leads to better perf on XBox
		}

		processElementPair(*this, pairs[pairIdx + 2], ccdPass);

		// prefetch actor core for next batch
		{
			PX_ASSERT(prefetchLookAhead >= 4);
			prefetchPairActorsCore(*batchedActors[0], *batchedActors[1]);
			prefetchPairActorsCore(*batchedActors[2], *batchedActors[3]);
			prefetchPairActorsCore(*batchedActors[4], *batchedActors[5]);
			prefetchPairActorsCore(*batchedActors[6], *batchedActors[7]);
			// unrolling by hand leads to better perf on XBox
		}

		processElementPair(*this, pairs[pairIdx + 3], ccdPass);

		pairIdx += prefetchLookAhead;
	}

	// process remaining pairs
	for(PxU32 i=pairIdx; i < pairCount; i++)
	{
		processElementPair(*this, pairs[i], ccdPass);
	}
}

void Sc::NPhaseCore::registerInteraction(ElementSimInteraction* interaction)
{
	mElementSimMap.insert(ElementSimKey(&interaction->getElement0(), &interaction->getElement1()), interaction);
}

void Sc::NPhaseCore::unregisterInteraction(ElementSimInteraction* interaction)
{
	mElementSimMap.erase(ElementSimKey(&interaction->getElement0(), &interaction->getElement1()));
}

ElementSimInteraction* Sc::NPhaseCore::onOverlapRemovedStage1(ElementSim* volume0, ElementSim* volume1)
{
	ElementSimInteraction* pair = findInteraction(volume0, volume1);
	return pair;
}

void Sc::NPhaseCore::onOverlapRemoved(ElementSim* volume0, ElementSim* volume1, const PxU32 ccdPass, void* elemSim, PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	ElementSim* elementHi = volume1;
	ElementSim* elementLo = volume0;
	// No actor internal interactions
	PX_ASSERT(&elementHi->getActor() != &elementLo->getActor());

	// PT: TODO: get rid of 'findInteraction', cf US10491
	ElementSimInteraction* interaction = elemSim ? reinterpret_cast<ElementSimInteraction*>(elemSim) : findInteraction(elementHi, elementLo);

	// MS: The check below is necessary since at the moment LowLevel broadphase still tracks
	//     killed pairs and hence reports lost overlaps
	if (interaction)
	{
		PxU32 flags = PxU32(PairReleaseFlag::eWAKE_ON_LOST_TOUCH);
		PX_ASSERT(interaction->isElementInteraction());
		releaseElementPair(static_cast<ElementSimInteraction*>(interaction), flags, ccdPass, true, outputs, useAdaptiveForce);
	}

#if PX_USE_CLOTH_API
    // Cloth doesn't use interactions
    if (elementLo->getElementType() == ElementType::eCLOTH)
        Ps::swap(elementLo, elementHi);
    if (elementHi->getElementType() == ElementType::eCLOTH &&
        elementLo->getElementType() == ElementType::eSHAPE)
    {
        ShapeSim* shapeLo = static_cast<ShapeSim*>(elementLo);
        ClothShape* shapeHi = static_cast<ClothShape*>(elementHi);
		ClothSim& clothSim = shapeHi->getClothSim();

		clothSim.removeCollisionShape(shapeLo);
		removeClothOverlap(&clothSim, shapeLo);
    }
#endif // PX_USE_CLOTH_API
}

// MS: TODO: optimize this for the actor release case?
void Sc::NPhaseCore::onVolumeRemoved(ElementSim* volume, PxU32 flags, PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	const PxU32 ccdPass = 0;

	switch (volume->getElementType())
	{
		case ElementType::eSHAPE:
			{
				flags |= PairReleaseFlag::eSHAPE_BP_VOLUME_REMOVED;

				// Release interactions
				// IMPORTANT: Iterate from the back of the list to the front as we release interactions which
				//            triggers a replace with last
				ElementSim::ElementInteractionReverseIterator iter = volume->getElemInteractionsReverse();
				ElementSimInteraction* interaction = iter.getNext();
				while(interaction)
				{
#if PX_USE_PARTICLE_SYSTEM_API
					PX_ASSERT(	(interaction->getType() == InteractionType::eMARKER) ||
								(interaction->getType() == InteractionType::eOVERLAP) ||
								(interaction->getType() == InteractionType::eTRIGGER) ||
								(interaction->getType() == InteractionType::ePARTICLE_BODY) );
#else
					PX_ASSERT(	(interaction->getType() == InteractionType::eMARKER) ||
								(interaction->getType() == InteractionType::eOVERLAP) ||
								(interaction->getType() == InteractionType::eTRIGGER) );
#endif

					releaseElementPair(interaction, flags, ccdPass, true, outputs, useAdaptiveForce);

					interaction = iter.getNext();
				}

#if PX_USE_CLOTH_API
				ShapeSim* shape = static_cast<ShapeSim*>(volume);
				if(const Ps::HashMap<const ShapeSim*, ClothListElement>::Entry* entry = mClothOverlaps.find(shape))
				{
					for(ClothListElement* it = entry->second.mNext; it;)
					{
						it->mClothSim->removeCollisionShape(shape);
						ClothListElement* next = it->mNext;
						mClothPool.deallocate(it);
						it = next;
					}
					mClothOverlaps.erase(shape);
				}
#endif
				break;
			}
#if PX_USE_PARTICLE_SYSTEM_API
		case ElementType::ePARTICLE_PACKET:
			{
				flags |= PairReleaseFlag::eBP_VOLUME_REMOVED;

				// Release interactions
				ParticlePacketShape* ps = static_cast<ParticlePacketShape*>(volume);

				PxU32 nbInteractions = ps->getInteractionsCount();
				ParticleElementRbElementInteraction** interactions = ps->getInteractions() + nbInteractions;
				while(nbInteractions--)
				{
					ParticleElementRbElementInteraction* interaction = *--interactions;
					PX_ASSERT((*interaction).getType() == InteractionType::ePARTICLE_BODY);
					// The ccdPass parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline is running.
					releaseElementPair(static_cast<ElementSimInteraction*>(interaction), flags, ccdPass, true, outputs, useAdaptiveForce);
				}
				break;
			}
#endif	// PX_USE_PARTICLE_SYSTEM_API
#if PX_USE_CLOTH_API
        case ElementType::eCLOTH:
            {
                // nothing to do
            }
            break;
#endif // PX_USE_CLOTH_API
		case ElementType::eCOUNT:
			{
				PX_ASSERT(0);
				break;
			}
	}
}

#if PX_USE_CLOTH_API
void Sc::NPhaseCore::removeClothOverlap(ClothSim* clothSim, const ShapeSim* shapeSim)
{
	// When a new overlap was rejected due to the sphere/plane limit, 
	// the entry to delete does not exist. 
	for(ClothListElement* it = &mClothOverlaps[shapeSim]; 
		ClothListElement* next = it->mNext; it = next)
	{
		if(clothSim == next->mClothSim)
		{
			it->mNext = next->mNext;
			mClothPool.deallocate(next);
			break;
		}
	}
}
#endif

Sc::ElementSimInteraction* Sc::NPhaseCore::createRbElementInteraction(const PxFilterInfo& finfo, ShapeSim& s0, ShapeSim& s1, PxsContactManager* contactManager, Sc::ShapeInteraction* shapeInteraction,
	Sc::ElementInteractionMarker* interactionMarker, PxU32 isTriggerPair)
{
	ElementSimInteraction* pair = NULL;

	if ((finfo.filterFlags & PxFilterFlag::eSUPPRESS) == false)
	{
		if (!isTriggerPair)
		{
			pair = createShapeInteraction(s0, s1, finfo.pairFlags, contactManager, shapeInteraction);
		}
		else
		{
			TriggerInteraction* ti = createTriggerInteraction(s0, s1, finfo.pairFlags);
			pair = ti;
		}
	}
	else
	{
		pair = createElementInteractionMarker(s0, s1, interactionMarker);
	}

	if (finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX)
	{
		// Mark the pair as a filter callback pair
		pair->raiseInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);

		// Filter callback pair: Set the link to the interaction
		mFilterPairManager->setPair(finfo.filterPairIndex, pair, FilterPair::ELEMENT_ELEMENT);
		pair->setFilterPairIndex(finfo.filterPairIndex);
	}

	return pair;
}

Sc::ElementSimInteraction* Sc::NPhaseCore::createRbElementInteraction(ShapeSim& s0, ShapeSim& s1, PxsContactManager* contactManager, Sc::ShapeInteraction* shapeInteraction,
	Sc::ElementInteractionMarker* interactionMarker)
{
	PxU32 isTriggerPair = 0;
	const PxFilterInfo finfo = filterRbCollisionPair(s0, s1, INVALID_FILTER_PAIR_INDEX, isTriggerPair, false);

	if (finfo.filterFlags & PxFilterFlag::eKILL)
	{
		PX_ASSERT(finfo.filterPairIndex == INVALID_FILTER_PAIR_INDEX);  // No filter callback pair info for killed pairs
		return NULL;
	}

	return createRbElementInteraction(finfo, s0, s1, contactManager, shapeInteraction, interactionMarker, isTriggerPair);
}

#if PX_USE_PARTICLE_SYSTEM_API
// PT: function used only once, so safe to force inline
static PX_FORCE_INLINE Sc::ParticleElementRbElementInteraction* findParticlePacketBodyInteraction(ParticlePacketShape* ps, ActorSim* actor)
{
	ParticleElementRbElementInteraction** interactions = ps->getInteractions();
	PxU32 count = ps->getInteractionsCount();

	while(count--)
	{
		ParticleElementRbElementInteraction*const interaction = *interactions++;

		PX_ASSERT(	(interaction->getActor0().getActorType() == PxActorType::ePARTICLE_SYSTEM) ||
					(interaction->getActor0().getActorType() == PxActorType::ePARTICLE_FLUID) );

		if ((&interaction->getActor1() == actor) && (&interaction->getParticleShape() == ps))
		{
			PX_ASSERT(interaction->getType() == InteractionType::ePARTICLE_BODY);
			return interaction;
		}
	}
	return NULL;
}

Sc::ElementSimInteraction* Sc::NPhaseCore::createParticlePacketBodyInteraction(ParticlePacketShape& ps, ShapeSim& s, const PxU32 ccdPass)
{
	ActorElementPair* actorElementPair = NULL;

	Sc::ActorSim& shapeActorSim = s.getActor();
	Sc::ActorSim& psActorSim = ps.getActor();

	ParticleElementRbElementInteraction* pbi = findParticlePacketBodyInteraction(&ps, &shapeActorSim);
	if(pbi)
	{
		// There already is an interaction between the shape and the particleSystem/...
		// In that case, fetch the filter information from the existing interaction.

		actorElementPair = pbi->getActorElementPair();
		PX_ASSERT(actorElementPair);
	}
	else
	{
		PxFilterInfo finfo = runFilter(ps, s, INVALID_FILTER_PAIR_INDEX, true);

		// Note: It is valid to check here for killed pairs and not in the if-section above since in the
		//       case of killed pairs you won't ever get in the if-section.
		if(finfo.filterFlags & PxFilterFlag::eKILL)
		{
			PX_ASSERT(finfo.filterPairIndex == INVALID_FILTER_PAIR_INDEX);  // No filter callback pair info for killed pairs
			return NULL;
		}

		const bool isSuppressed = finfo.filterFlags & PxFilterFlag::eSUPPRESS;
		actorElementPair = mActorElementPairPool.construct(psActorSim, s, finfo.pairFlags);
		actorElementPair->markAsSuppressed(isSuppressed);

		actorElementPair->markAsFilterPair(finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX);

		if (finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX)
		{
			mFilterPairManager->setPair(finfo.filterPairIndex, NULL, FilterPair::ELEMENT_ACTOR);
			pbi->setFilterPairIndex(INVALID_FILTER_PAIR_INDEX);
		}
	}

	ElementSimInteraction* pair = insertParticleElementRbElementPair(ps, s, actorElementPair, ccdPass);
	if(actorElementPair->isFilterPair())
		pair->raiseInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);  // Mark the pair as a filter callback pair

	return pair;
}
#endif

void Sc::NPhaseCore::managerNewTouch(Sc::ShapeInteraction& interaction, const PxU32 /*ccdPass*/, bool /*adjustCounters*/, PxsContactManagerOutputIterator& /*outputs*/)
{
	//(1) if the pair hasn't already been assigned, look it up!

	ActorPair* actorPair = interaction.getActorPair();

	if(!actorPair)
	{
		ShapeSim& s0 = static_cast<ShapeSim&>(interaction.getElement0());
		ShapeSim& s1 = static_cast<ShapeSim&>(interaction.getElement1());
		actorPair = findActorPair(&s0, &s1, interaction.isReportPair());
		actorPair->incRefCount(); //It's being referenced by a new pair...
		interaction.setActorPair(*actorPair);
	}	
}

Sc::ShapeInteraction* Sc::NPhaseCore::createShapeInteraction(ShapeSim& s0, ShapeSim& s1, PxPairFlags pairFlags, PxsContactManager* contactManager, Sc::ShapeInteraction* shapeInteraction)
{
	ShapeSim* _s0 = &s0;
	ShapeSim* _s1 = &s1;

	/*
	This tries to ensure that if one of the bodies is static or kinematic, it will be body B
	There is a further optimization to force all pairs that share the same bodies to have
	the same body ordering.  This reduces the number of required partitions in the parallel solver.
	Sorting rules are:
	If bodyA is static, swap
	If bodyA is rigidDynamic and bodyB is articulation, swap
	If bodyA is in an earlier BP group than bodyB, swap
	*/
	{
		Sc::RigidSim& rs0 = s0.getRbSim();
		Sc::RigidSim& rs1 = s1.getRbSim();

		const PxActorType::Enum actorType0 = rs0.getActorType();
		const PxActorType::Enum actorType1 = rs1.getActorType();
		if( actorType0 == PxActorType::eRIGID_STATIC
			 || (actorType0 == PxActorType::eRIGID_DYNAMIC && actorType1 == PxActorType::eARTICULATION_LINK)
			 || ((actorType0 == PxActorType::eRIGID_DYNAMIC && actorType1 == PxActorType::eRIGID_DYNAMIC) && s0.getBodySim()->isKinematic())
			 || (actorType0 == actorType1 && rs0.getRigidID() < rs1.getRigidID()))
			Ps::swap(_s0, _s1);
	}

	ShapeInteraction* si = shapeInteraction ? shapeInteraction : mShapeInteractionPool.allocate();
	PX_PLACEMENT_NEW(si, ShapeInteraction)(*_s0, *_s1, pairFlags, contactManager);

	PX_ASSERT(si->mReportPairIndex == INVALID_REPORT_PAIR_ID);

	return si;
}

Sc::TriggerInteraction* Sc::NPhaseCore::createTriggerInteraction(ShapeSim& s0, ShapeSim& s1, PxPairFlags triggerFlags)
{
	ShapeSim* triggerShape;
	ShapeSim* otherShape;

	if (s1.getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
	{
		triggerShape = &s1;
		otherShape = &s0;
	}
	else
	{
		triggerShape = &s0;
		otherShape = &s1;
	}
	TriggerInteraction* pair = mTriggerInteractionPool.construct(*triggerShape, *otherShape);
	pair->setTriggerFlags(triggerFlags);
	return pair;
}

Sc::ElementInteractionMarker* Sc::NPhaseCore::createElementInteractionMarker(ElementSim& e0, ElementSim& e1, Sc::ElementInteractionMarker* interactionMarker)
{
	ElementInteractionMarker* pair = interactionMarker ? interactionMarker : mInteractionMarkerPool.allocate();
	PX_PLACEMENT_NEW(pair, ElementInteractionMarker)(e0, e1, interactionMarker != NULL);
	return pair;
}

static PX_INLINE PxFilterFlags checkFilterFlags(PxFilterFlags filterFlags)
{
	if ((filterFlags & (PxFilterFlag::eKILL | PxFilterFlag::eSUPPRESS)) == (PxFilterFlag::eKILL | PxFilterFlag::eSUPPRESS))
	{
#if PX_CHECKED
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Filtering: eKILL and eSUPPRESS must not be set simultaneously. eSUPPRESS will be used.");
#endif
		filterFlags.clear(PxFilterFlag::eKILL);
	}

	return filterFlags;
}

static PX_INLINE PxPairFlags checkRbPairFlags(	const ShapeSim& s0, const ShapeSim& s1,
												const Sc::BodySim* bs0, const Sc::BodySim* bs1,
												PxPairFlags pairFlags, PxFilterFlags filterFlags)
{
	if(filterFlags & (PxFilterFlag::eSUPPRESS | PxFilterFlag::eKILL))
		return pairFlags;

	if (bs0 && bs0->isKinematic() && 
		bs1 && bs1->isKinematic() && 
		(pairFlags & PxPairFlag::eSOLVE_CONTACT))
	{
#if PX_CHECKED
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Filtering: Resolving contacts between two kinematic objects is invalid. Contacts will not get resolved.");
#endif
		pairFlags.clear(PxPairFlag::eSOLVE_CONTACT);
	}

#if PX_CHECKED
	// we want to avoid to run contact generation for pairs that should not get resolved or have no contact/trigger reports
	if (!(PxU32(pairFlags) & (PxPairFlag::eSOLVE_CONTACT | ShapeInteraction::CONTACT_REPORT_EVENTS)))
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Filtering: Pair with no contact/trigger reports detected, nor is PxPairFlag::eSOLVE_CONTACT set. It is recommended to suppress/kill such pairs for performance reasons.");
	}
	else if(!(pairFlags & (PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eDETECT_CCD_CONTACT)))
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING,  __FILE__, __LINE__, "Filtering: Pair did not request either eDETECT_DISCRETE_CONTACT or eDETECT_CCD_CONTACT. It is recommended to suppress/kill such pairs for performance reasons.");
	}
#endif

#if PX_CHECKED
	if(((s0.getFlags() & PxShapeFlag::eTRIGGER_SHAPE)!=0 || (s1.getFlags() & PxShapeFlag::eTRIGGER_SHAPE)!=0) &&
		(pairFlags & PxPairFlag::eTRIGGER_DEFAULT) && (pairFlags & PxPairFlag::eDETECT_CCD_CONTACT))
	{
		Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING,  __FILE__, __LINE__, "Filtering: CCD isn't supported on Triggers yet");
	}
#else
	PX_UNUSED(s0);
	PX_UNUSED(s1);
#endif

	return pairFlags;
}

static Sc::InteractionType::Enum getRbElementInteractionType(const ShapeSim* primitive0, const ShapeSim* primitive1, PxFilterFlags filterFlag)
{
	if(filterFlag & PxFilterFlag::eKILL)
		return InteractionType::eINVALID;

	if(filterFlag & PxFilterFlag::eSUPPRESS)
		return InteractionType::eMARKER;

	if(primitive0->getFlags() & PxShapeFlag::eTRIGGER_SHAPE
    || primitive1->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
		return InteractionType::eTRIGGER;

	PX_ASSERT(	(primitive0->getGeometryType() != PxGeometryType::eTRIANGLEMESH) ||
				(primitive1->getGeometryType() != PxGeometryType::eTRIANGLEMESH));

	return InteractionType::eOVERLAP;
}

Sc::ElementSimInteraction* Sc::NPhaseCore::refilterInteraction(ElementSimInteraction* pair, const PxFilterInfo* filterInfo, bool removeFromDirtyList, PxsContactManagerOutputIterator& outputs,
	bool useAdaptiveForce)
{
	const InteractionType::Enum oldType = pair->getType();

	switch (oldType)
	{
		case InteractionType::eTRIGGER:
		case InteractionType::eMARKER:
		case InteractionType::eOVERLAP:
			{
				PX_ASSERT(pair->getElement0().getElementType() == ElementType::eSHAPE);
				PX_ASSERT(pair->getElement1().getElementType() == ElementType::eSHAPE);

				ShapeSim& s0 = static_cast<ShapeSim&>(pair->getElement0());
				ShapeSim& s1 = static_cast<ShapeSim&>(pair->getElement1());

				PxFilterInfo finfo;
				if (filterInfo)
				{
					// The filter changes are provided by an outside source (the user filter callback)

					finfo = *filterInfo;
					PX_ASSERT(finfo.filterPairIndex!=INVALID_FILTER_PAIR_INDEX);

					if ((finfo.filterFlags & PxFilterFlag::eKILL) &&
						((finfo.filterFlags & PxFilterFlag::eNOTIFY) == PxFilterFlag::eNOTIFY) )
					{
						callPairLost(pair->getElement0(), pair->getElement1(), finfo.filterPairIndex, false);
						mFilterPairManager->releaseIndex(finfo.filterPairIndex);
						finfo.filterPairIndex = INVALID_FILTER_PAIR_INDEX;
					}

					Sc::BodySim* bs0 = s0.getBodySim();
					Sc::BodySim* bs1 = s1.getBodySim();
					finfo.pairFlags = checkRbPairFlags(s0, s1, bs0, bs1, finfo.pairFlags, finfo.filterFlags);

					//!!!Filter  Issue: Need to check whether the requested changes don't violate hard constraints.
					//                  - Assert that the shapes are not connected through a collision disabling joint
				}
				else
				{
					PxU32 filterPairIndex =  INVALID_FILTER_PAIR_INDEX;
					if (pair->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR))
					{
						filterPairIndex = mFilterPairManager->findIndex(pair);
						PX_ASSERT(filterPairIndex!=INVALID_FILTER_PAIR_INDEX);

						callPairLost(pair->getElement0(), pair->getElement1(), filterPairIndex, false);
					}

					PxU32 isTriggerPair;
					finfo = filterRbCollisionPair(s0, s1, filterPairIndex, isTriggerPair, true);
					PX_UNUSED(isTriggerPair);
				}

				if (pair->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR) &&
					((finfo.filterFlags & PxFilterFlag::eNOTIFY) != PxFilterFlag::eNOTIFY) )
				{
					// The pair was a filter callback pair but not any longer
					pair->clearInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);

					if (finfo.filterPairIndex!=INVALID_FILTER_PAIR_INDEX)
					{
						mFilterPairManager->releaseIndex(finfo.filterPairIndex);
						finfo.filterPairIndex = INVALID_FILTER_PAIR_INDEX;
					}
				}

				const InteractionType::Enum newType = getRbElementInteractionType(&s0, &s1, finfo.filterFlags);
				if (pair->getType() != newType)  //Only convert interaction type if the type has changed
				{
					return convert(pair, newType, finfo, removeFromDirtyList, outputs, useAdaptiveForce);
				}
				else
				{
					//The pair flags might have changed, we need to forward the new ones
					if (oldType == InteractionType::eOVERLAP)
					{
						ShapeInteraction* si = static_cast<ShapeInteraction*>(pair);

						const PxU32 newPairFlags = finfo.pairFlags;
						const PxU32 oldPairFlags = si->getPairFlags();
						PX_ASSERT((newPairFlags & ShapeInteraction::PAIR_FLAGS_MASK) == newPairFlags);
						PX_ASSERT((oldPairFlags & ShapeInteraction::PAIR_FLAGS_MASK) == oldPairFlags);

						if (newPairFlags != oldPairFlags)
						{
							if (!(oldPairFlags & ShapeInteraction::CONTACT_REPORT_EVENTS) && (newPairFlags & ShapeInteraction::CONTACT_REPORT_EVENTS) && (si->getActorPair() == NULL || !si->getActorPair()->isReportPair()))
							{
								// for this actor pair there was no shape pair that requested contact reports but now there is one
								// -> all the existing shape pairs need to get re-adjusted to point to an ActorPairReport instance instead.
								findActorPair(&s0, &s1, Ps::IntTrue);
							}

							if (si->readFlag(ShapeInteraction::IN_PERSISTENT_EVENT_LIST) && (!(newPairFlags & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)))
							{
								// the new report pair flags don't require persistent checks anymore -> remove from persistent list
								// Note: The pair might get added to the force threshold list later
								if (si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST))
								{
									removeFromPersistentContactEventPairs(si);
								}
								else
								{
									si->clearFlag(ShapeInteraction::WAS_IN_PERSISTENT_EVENT_LIST);
								}
							}

							if (newPairFlags & ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS)
							{
								PX_ASSERT((si->mReportPairIndex == INVALID_REPORT_PAIR_ID) || (!si->readFlag(ShapeInteraction::WAS_IN_PERSISTENT_EVENT_LIST)));

								if (si->mReportPairIndex == INVALID_REPORT_PAIR_ID && si->readInteractionFlag(InteractionFlag::eIS_ACTIVE))
								{
									PX_ASSERT(!si->readFlag(ShapeInteraction::WAS_IN_PERSISTENT_EVENT_LIST));  // sanity check: an active pair should never have this flag set

									if (si->hasTouch())
										addToForceThresholdContactEventPairs(si);
								}
							}
							else if ((oldPairFlags & ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS))
							{
								// no force threshold events needed any longer -> clear flags
								si->clearFlag(ShapeInteraction::FORCE_THRESHOLD_EXCEEDED_FLAGS);

								if (si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST))
									removeFromForceThresholdContactEventPairs(si);
							}
						}
						si->setPairFlags(finfo.pairFlags);
					}
					else if (oldType == InteractionType::eTRIGGER)
						static_cast<TriggerInteraction*>(pair)->setTriggerFlags(finfo.pairFlags);

					return pair;
				}
			}
#if (PX_USE_PARTICLE_SYSTEM_API)
		case InteractionType::ePARTICLE_BODY:
			{
				ParticleElementRbElementInteraction* pbi = static_cast<ParticleElementRbElementInteraction*>(pair);

				ActorElementPair* aep = pbi->getActorElementPair();

				PxFilterInfo finfo;
				if (filterInfo)
				{
					// The filter changes are provided by an outside source (the user filter callback)

					finfo = *filterInfo;

					PX_ASSERT((finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX && aep->isFilterPair()) ||
							(finfo.filterPairIndex == INVALID_FILTER_PAIR_INDEX && !aep->isFilterPair()) );
					PX_ASSERT(aep->getPairFlags() == finfo.pairFlags);
					PX_ASSERT(!(finfo.filterFlags & PxFilterFlag::eKILL) ||
							((finfo.filterFlags & PxFilterFlag::eKILL) && aep->isKilled()));
					PX_ASSERT(!(finfo.filterFlags & PxFilterFlag::eSUPPRESS) ||
							((finfo.filterFlags & PxFilterFlag::eSUPPRESS) && aep->isSuppressed()) );
					// This should have been done at statusChange() level (see fireCustomFilteringCallbacks())

					if (finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX && aep->isKilled() && pair->isLastFilterInteraction())
					{
						// This is the last of (possibly) multiple element-element interactions of the same element-actor interaction
						// --> Kill the filter callback pair

						callPairLost(pbi->getElement0(), pbi->getElement1(), finfo.filterPairIndex, false);
						mFilterPairManager->releaseIndex(finfo.filterPairIndex);
						finfo.filterPairIndex = INVALID_FILTER_PAIR_INDEX;
					}
				}
				else
				{
					if (!aep->hasBeenRefiltered(mOwnerScene.getTimeStamp()))
					{
						// The first of (possibly) multiple element-element interactions of the same element-actor interaction
						// - For filter callback pairs, call pairLost()
						// - Run the filter
						// - Pass new pair flags on

						PxU32 filterPairIndex = INVALID_FILTER_PAIR_INDEX;
						if (pbi->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR))
						{
							filterPairIndex = mFilterPairManager->findIndex(pbi);
							PX_ASSERT(filterPairIndex!=INVALID_FILTER_PAIR_INDEX);

							callPairLost(pbi->getElement0(), pbi->getElement1(), filterPairIndex, false);
						}

						finfo = runFilter(pair->getElement0(), pair->getElement1(), filterPairIndex, true);
						// Note: The filter run will remove the callback pair if the element-actor interaction should get killed
						//       or if the pair is no longer a filter callback pair

						aep->markAsFilterPair(finfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX);

						aep->setPairFlags(finfo.pairFlags);  // Pass on the pair flags (this needs only be done for the first pair)

						if (finfo.filterFlags & PxFilterFlag::eKILL)
						{
							aep->markAsKilled(true);  // Set such that other pairs of the same element-actor interaction know that they have to give their lives for a higher goal...
						}
						else if (finfo.filterFlags & PxFilterFlag::eSUPPRESS)
							aep->markAsSuppressed(true);
						else
						{
							aep->markAsSuppressed(false);
						}
					}
				}

				if (aep->isFilterPair())
				{
					pair->raiseInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);
				}
				else if (pair->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR))
				{
					// The pair was a filter callback pair but not any longer
					pair->clearInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);
				}

				if (aep->isKilled())
				{
					PX_ASSERT(oldType == InteractionType::ePARTICLE_BODY);
					// The ccdPass parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline is running.
					pool_deleteParticleElementRbElementPair(pbi, 0, 0);
					return NULL;
				}

				pbi->checkLowLevelActivationState();

				return pair;
			}
#endif  // PX_USE_PARTICLE_SYSTEM_API
			case InteractionType::eCONSTRAINTSHADER:
			case InteractionType::eARTICULATION:
			case InteractionType::eTRACKED_IN_SCENE_COUNT:
			case InteractionType::eINVALID:
			PX_ASSERT(0);
			break;
	}
	return NULL;
}

PX_INLINE void Sc::NPhaseCore::callPairLost(const ElementSim& e0, const ElementSim& e1, PxU32 pairID, bool objVolumeRemoved) const
{
	PxFilterData fd0, fd1;
	PxFilterObjectAttributes fa0, fa1;

	e0.getFilterInfo(fa0, fd0);
	e1.getFilterInfo(fa1, fd1);

	mOwnerScene.getFilterCallbackFast()->pairLost(pairID, fa0, fd0, fa1, fd1, objVolumeRemoved);
}

PX_INLINE void Sc::NPhaseCore::runFilterShader(const ElementSim& e0, const ElementSim& e1,
										   PxFilterObjectAttributes& attr0, PxFilterData& filterData0,
										   PxFilterObjectAttributes& attr1, PxFilterData& filterData1,
										   PxFilterInfo& filterInfo)
{
	e0.getFilterInfo(attr0, filterData0);
	e1.getFilterInfo(attr1, filterData1);

	filterInfo.filterFlags = mOwnerScene.getFilterShaderFast()(	attr0, filterData0,
																attr1, filterData1,
																filterInfo.pairFlags,
																mOwnerScene.getFilterShaderDataFast(),
																mOwnerScene.getFilterShaderDataSizeFast() );
}

static PX_FORCE_INLINE void fetchActorAndShape(const ElementSim& e, PxActor*& a, PxShape*& s)
{
	if (e.getElementType() == ElementType::eSHAPE)
	{
		const ShapeSim& sim = static_cast<const ShapeSim&>(e);
		a = sim.getRbSim().getPxActor();
		s = sim.getPxShape();
	}
#if PX_USE_PARTICLE_SYSTEM_API
	else
	{
		if (e.getElementType() == ElementType::ePARTICLE_PACKET)
			a = (static_cast<const Sc::ParticlePacketShape&>(e)).getParticleSystem().getCore().getPxParticleBase();
		s = NULL;
	}
#endif
}

PX_INLINE PxFilterInfo Sc::NPhaseCore::runFilter(const ElementSim& e0, const ElementSim& e1, PxU32 filterPairIndex, bool doCallbacks)
{
	PxFilterInfo filterInfo;

	PxFilterData fd0, fd1;
	PxFilterObjectAttributes fa0, fa1;

	runFilterShader(e0, e1, fa0, fd0, fa1, fd1, filterInfo);

	if (filterInfo.filterFlags & PxFilterFlag::eCALLBACK)
	{
		if (mOwnerScene.getFilterCallbackFast())
		{
			if (!doCallbacks)
			{
				//KS - return 
				return filterInfo;
			}
			else
			{

				if (filterPairIndex == INVALID_FILTER_PAIR_INDEX)
					filterPairIndex = mFilterPairManager->acquireIndex();
				// If a FilterPair is provided, then we use it, else we create a new one
				// (A FilterPair is provided in the case for a pairLost()-pairFound() sequence after refiltering)

				PxActor* a0, *a1;
				PxShape* s0, *s1;
				fetchActorAndShape(e0, a0, s0);
				fetchActorAndShape(e1, a1, s1);

				filterInfo.filterFlags = mOwnerScene.getFilterCallbackFast()->pairFound(filterPairIndex, fa0, fd0, a0, s0, fa1, fd1, a1, s1, filterInfo.pairFlags);
				filterInfo.filterPairIndex = filterPairIndex;
			}
		}
		else
		{
			filterInfo.filterFlags.clear(PxFilterFlag::eNOTIFY);
			Ps::getFoundation().error(PxErrorCode::eDEBUG_WARNING, __FILE__, __LINE__, "Filtering: eCALLBACK set but no filter callback defined.");
		}
	}

	filterInfo.filterFlags = checkFilterFlags(filterInfo.filterFlags);

	if (filterPairIndex!=INVALID_FILTER_PAIR_INDEX && ((filterInfo.filterFlags & PxFilterFlag::eKILL) || ((filterInfo.filterFlags & PxFilterFlag::eNOTIFY) != PxFilterFlag::eNOTIFY)))
	{
		if ((filterInfo.filterFlags & PxFilterFlag::eKILL) && ((filterInfo.filterFlags & PxFilterFlag::eNOTIFY) == PxFilterFlag::eNOTIFY))
			mOwnerScene.getFilterCallbackFast()->pairLost(filterPairIndex, fa0, fd0, fa1, fd1, false);

		if ((filterInfo.filterFlags & PxFilterFlag::eNOTIFY) != PxFilterFlag::eNOTIFY)
		{
			// No notification, hence we don't need to treat it as a filter callback pair anymore.
			// Make sure that eCALLBACK gets removed as well
			filterInfo.filterFlags.clear(PxFilterFlag::eNOTIFY);
		}

		mFilterPairManager->releaseIndex(filterPairIndex);
		filterInfo.filterPairIndex = INVALID_FILTER_PAIR_INDEX;
	}

	// Sanity checks
	PX_ASSERT(	(filterInfo.filterFlags != PxFilterFlag::eKILL) ||
				((filterInfo.filterFlags == PxFilterFlag::eKILL) && (filterInfo.filterPairIndex == INVALID_FILTER_PAIR_INDEX)) );
	PX_ASSERT(	((filterInfo.filterFlags & PxFilterFlag::eNOTIFY) != PxFilterFlag::eNOTIFY) ||
				(((filterInfo.filterFlags & PxFilterFlag::eNOTIFY) == PxFilterFlag::eNOTIFY) && filterInfo.filterPairIndex!=INVALID_FILTER_PAIR_INDEX) );

	return filterInfo;
}

PX_FORCE_INLINE PxFilterInfo Sc::NPhaseCore::filterOutRbCollisionPair(PxU32 filterPairIndex, const PxFilterFlags filterFlags)
{
	if(filterPairIndex!=INVALID_FILTER_PAIR_INDEX)
		mFilterPairManager->releaseIndex(filterPairIndex);

	return PxFilterInfo(filterFlags);
}

PxFilterInfo Sc::NPhaseCore::filterRbCollisionPairSecondStage(const ShapeSim& s0, const ShapeSim& s1, const Sc::BodySim* b0, const Sc::BodySim* b1, PxU32 filterPairIndex, bool runCallbacks)
{
	PxFilterInfo filterInfo = runFilter(s0, s1, filterPairIndex, runCallbacks);
	if (runCallbacks || (!(filterInfo.filterFlags & PxFilterFlag::eCALLBACK)))
		filterInfo.pairFlags = checkRbPairFlags(s0, s1, b0, b1, filterInfo.pairFlags, filterInfo.filterFlags);
	return filterInfo;
}

PxFilterInfo Sc::NPhaseCore::filterRbCollisionPair(const ShapeSim& s0, const ShapeSim& s1, PxU32 filterPairIndex, PxU32& isTriggerPair, bool runCallbacks)
{
	const Sc::BodySim* b0 = s0.getBodySim();
	const Sc::BodySim* b1 = s1.getBodySim();

	//  if not triggers...
	PX_COMPILE_TIME_ASSERT(PxU32(PxShapeFlag::eTRIGGER_SHAPE) < (1 << ((sizeof(PxShapeFlags::InternalType) * 8) - 1)));
	const PxU32 triggerMask = PxShapeFlag::eTRIGGER_SHAPE | (PxShapeFlag::eTRIGGER_SHAPE << 1);
	const PxU32 triggerPair = (s0.getFlags() & PxShapeFlag::eTRIGGER_SHAPE) | ((s1.getFlags() & PxShapeFlag::eTRIGGER_SHAPE) << 1);
	isTriggerPair = triggerPair;
	if(!(triggerPair & triggerMask))
	{
		const bool isS0Kinematic = b0 ? b0->isKinematic() : false;
		const bool isS1Kinematic = b1 ? b1->isKinematic() : false;

		// ...and if at least one object is kinematic
		const bool kinematicPair = isS0Kinematic | isS1Kinematic;
		if(kinematicPair)
		{
			PxSceneFlags sceneFlags = mOwnerScene.getPublicFlags();
			const PxPairFilteringMode::Enum kineKineFilteringMode = mOwnerScene.getKineKineFilteringMode();
			if(kineKineFilteringMode==PxPairFilteringMode::eKEEP)
				sceneFlags |= PxSceneFlag::eENABLE_KINEMATIC_PAIRS;
			else if(kineKineFilteringMode==PxPairFilteringMode::eSUPPRESS)
				sceneFlags &= ~PxSceneFlag::eENABLE_KINEMATIC_PAIRS;

			const PxPairFilteringMode::Enum staticKineFilteringMode = mOwnerScene.getStaticKineFilteringMode();
			if(staticKineFilteringMode==PxPairFilteringMode::eKEEP)
				sceneFlags |= PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS;
			else if(staticKineFilteringMode==PxPairFilteringMode::eSUPPRESS)
				sceneFlags &= ~PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS;

			// ...then ignore kinematic vs. static pairs
			if(!(sceneFlags & PxSceneFlag::eENABLE_KINEMATIC_STATIC_PAIRS))
			{
				if(!b0 || !b1)
					return filterOutRbCollisionPair(filterPairIndex, PxFilterFlag::eSUPPRESS);
			}

			// ...and ignore kinematic vs. kinematic pairs
			if(!(sceneFlags & PxSceneFlag::eENABLE_KINEMATIC_PAIRS))
			{
				if(isS0Kinematic && isS1Kinematic)
					return filterOutRbCollisionPair(filterPairIndex, PxFilterFlag::eSUPPRESS);
			}
		}
	}
	else
	{
		const PxSceneFlags sceneFlags = mOwnerScene.getPublicFlags();
		if (!(sceneFlags & PxSceneFlag::eDEPRECATED_TRIGGER_TRIGGER_REPORTS))
		{
			if ((triggerPair & triggerMask) != triggerMask)  // only one shape is a trigger
			{
				return filterRbCollisionPairSecondStage(s0, s1, b0, b1, filterPairIndex, runCallbacks);
			}
			else
			{
				// trigger-trigger pairs are not supported
				return filterOutRbCollisionPair(filterPairIndex, PxFilterFlag::eKILL);
			}
		}
		else
		{
			return filterRbCollisionPairSecondStage(s0, s1, b0, b1, filterPairIndex, runCallbacks);
		}
	}

	// If the bodies of the shape pair are connected by a joint, we need to check whether this connection disables the collision.
	// Note: As an optimization, the dynamic bodies have a flag which specifies whether they have any constraints at all. That works
	//       because a constraint has at least one dynamic body and an interaction is tracked by both objects.
	bool collisionDisabled = false;
	bool hasConstraintConnection = false;

	ActorSim& rbActor0 = s0.getActor();
	ActorSim& rbActor1 = s1.getActor();

	if (b0 && b0->readInternalFlag(Sc::BodySim::BF_HAS_CONSTRAINTS))
		hasConstraintConnection = b0->isConnectedTo(rbActor1, collisionDisabled);
	else if (b1 && b1->readInternalFlag(Sc::BodySim::BF_HAS_CONSTRAINTS))
		hasConstraintConnection = b1->isConnectedTo(rbActor0, collisionDisabled);

	if (!(hasConstraintConnection && collisionDisabled))
	{
		if ((rbActor0.getActorType() != PxActorType::eARTICULATION_LINK) || (rbActor1.getActorType() != PxActorType::eARTICULATION_LINK))
		{
			return filterRbCollisionPairSecondStage(s0, s1, b0, b1, filterPairIndex, runCallbacks);
		}
		else
		{
			// If the bodies are articulation links of the same articulation and one link is the parent of the other link, then disable collision

			PxU32 size = rbActor0.getActorInteractionCount();
			Interaction** interactions = rbActor0.getActorInteractions();
			while(size--)
			{
				const Interaction* interaction = *interactions++;
				if(interaction->getType() == InteractionType::eARTICULATION)
				{
					if((&interaction->getActor0() == &rbActor1) || (&interaction->getActor1() == &rbActor1))
						return filterOutRbCollisionPair(filterPairIndex, PxFilterFlag::eKILL);
				}
			}
		}

		return filterRbCollisionPairSecondStage(s0, s1, b0, b1, filterPairIndex, runCallbacks);
	}
	else
	{
		return filterOutRbCollisionPair(filterPairIndex, PxFilterFlag::eSUPPRESS);
	}
}

Sc::ActorPair* Sc::NPhaseCore::findActorPair(ShapeSim* s0, ShapeSim* s1, Ps::IntBool isReportPair)
{
	PX_ASSERT(!(s0->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
		   && !(s1->getFlags() & PxShapeFlag::eTRIGGER_SHAPE));
	// This method is only for the case where a ShapeInteraction is going to be created.
	// Else we might create an ActorPair that does not get referenced and causes a mem leak.
	
	BodyPairKey key;

	RigidSim* aLess = &s0->getRbSim();
	RigidSim* aMore = &s1->getRbSim();

	if (aLess->getRigidID() > aMore->getRigidID())
		Ps::swap(aLess, aMore);

	key.mSim0 = aLess->getRigidID();
	key.mSim1 = aMore->getRigidID();

	Sc::ActorPair*& actorPair = mActorPairMap[key];
	
	//PX_ASSERT(actorPair == NULL || actorPair->getRefCount() > 0);
	if(actorPair == NULL)
	{
		if (!isReportPair)
			actorPair =  mActorPairPool.construct();
		else
			actorPair = mActorPairReportPool.construct(s0->getRbSim(), s1->getRbSim());
	}
	
	Ps::IntBool actorPairHasReports = actorPair->isReportPair();

	if (!isReportPair || actorPairHasReports)
		return actorPair;
	else
	{
		PxU32 size = aLess->getActorInteractionCount();
		Interaction** interactions = aLess->getActorInteractions();
		
		ActorPairReport* actorPairReport = mActorPairReportPool.construct(s0->getRbSim(), s1->getRbSim());
		actorPairReport->convert(*actorPair);

		while (size--)
		{
			Interaction* interaction = *interactions++;
			if ((&interaction->getActor0() == aMore) || (&interaction->getActor1() == aMore))
			{
				PX_ASSERT(((&interaction->getActor0() == aLess) || (&interaction->getActor1() == aLess)));

				if(interaction->getType() == InteractionType::eOVERLAP)
				{
					ShapeInteraction* si = static_cast<ShapeInteraction*>(interaction);
					if(si->getActorPair() != NULL)
						si->setActorPair(*actorPairReport);
				}
			}
		}
		actorPair = actorPairReport;
	}
	return actorPair;
}

PX_FORCE_INLINE void Sc::NPhaseCore::destroyActorPairReport(ActorPairReport& aPair)
{
	PX_ASSERT(aPair.isReportPair());
	
	aPair.releaseContactReportData(*this);
	mActorPairReportPool.destroy(&aPair);
}

Sc::ElementSimInteraction* Sc::NPhaseCore::convert(ElementSimInteraction* pair, InteractionType::Enum newType, PxFilterInfo& filterInfo, bool removeFromDirtyList,
	PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	PX_ASSERT(newType != pair->getType());

	ElementSim& elementA = pair->getElement0();
	ElementSim& elementB = pair->getElement1();

	// Wake up the actors of the pair
	if ((pair->getActor0().getActorType() == PxActorType::eRIGID_DYNAMIC) && !(static_cast<BodySim&>(pair->getActor0()).isActive()))
		static_cast<BodySim&>(pair->getActor0()).internalWakeUp();
	if ((pair->getActor1().getActorType() == PxActorType::eRIGID_DYNAMIC) && !(static_cast<BodySim&>(pair->getActor1()).isActive()))
		static_cast<BodySim&>(pair->getActor1()).internalWakeUp();

	// Since the FilterPair struct might have been re-used in the newly created interaction, we need to clear
	// the filter pair marker of the old interaction to avoid that the FilterPair gets deleted by the releaseElementPair()
	// call that follows.
	pair->clearInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);

	// PT: we need to unregister the old interaction *before* creating the new one, because Sc::NPhaseCore::registerInteraction will use
	// ElementSim pointers which are the same for both.
	unregisterInteraction(pair);
	releaseElementPair(pair, PairReleaseFlag::eWAKE_ON_LOST_TOUCH | PairReleaseFlag::eBP_VOLUME_REMOVED, 0, removeFromDirtyList, outputs, useAdaptiveForce);

	ElementSimInteraction* result = NULL;
	switch (newType)
	{
		case InteractionType::eINVALID:
			// This means the pair should get killed
			break;
		case InteractionType::eMARKER:
			{
			result = createElementInteractionMarker(elementA, elementB, NULL);
			break;
			}
		case InteractionType::eOVERLAP:
			{
			PX_ASSERT(elementA.getElementType() == ElementType::eSHAPE && elementB.getElementType() == ElementType::eSHAPE);
			result = createShapeInteraction(static_cast<ShapeSim&>(elementA), static_cast<ShapeSim&>(elementB), filterInfo.pairFlags, NULL, NULL);
			break;
			}
		case InteractionType::eTRIGGER:
			{
			PX_ASSERT(elementA.getElementType() == ElementType::eSHAPE && elementB.getElementType() == ElementType::eSHAPE);
			result = createTriggerInteraction(static_cast<ShapeSim&>(elementA), static_cast<ShapeSim&>(elementB), filterInfo.pairFlags);
			break;
			}
		case InteractionType::eCONSTRAINTSHADER:
#if PX_USE_PARTICLE_SYSTEM_API
		case InteractionType::ePARTICLE_BODY:
#endif
		case InteractionType::eARTICULATION:
		case InteractionType::eTRACKED_IN_SCENE_COUNT:
			PX_ASSERT(0);
			break;
	};

	if (filterInfo.filterPairIndex != INVALID_FILTER_PAIR_INDEX)
	{
		PX_ASSERT(result);
		// If a filter callback pair is going to get killed, then the FilterPair struct should already have
		// been deleted.

		// Mark the new interaction as a filter callback pair
		result->raiseInteractionFlag(InteractionFlag::eIS_FILTER_PAIR);

		mFilterPairManager->setPair(filterInfo.filterPairIndex, result, FilterPair::ELEMENT_ELEMENT);
		result->setFilterPairIndex(filterInfo.filterPairIndex);
	}

	return result;
}


namespace physx
{
namespace Sc
{
static bool findTriggerContacts(TriggerInteraction* tri, bool toBeDeleted, bool volumeRemoved,
								PxTriggerPair& triggerPair, TriggerPairExtraData& triggerPairExtra,
								SimStats::TriggerPairCountsNonVolatile& triggerPairStats)
{
	ShapeSim& s0 = tri->getTriggerShape();
	ShapeSim& s1 = tri->getOtherShape();

	const PxPairFlags pairFlags = tri->getTriggerFlags();
	PxPairFlags pairEvent;

	bool overlap;
	PxU8 testForRemovedShapes = 0;
	if (toBeDeleted)
	{
		// The trigger interaction is to lie down in its tomb, hence we know that the overlap is gone.
		// What remains is to check whether the interaction was deleted because of a shape removal in
		// which case we need to later check for removed shapes.

		overlap = false;

		if (volumeRemoved)
		{
			// Note: only the first removed volume can be detected when the trigger interaction is deleted but at a later point the second volume might get removed too.
			testForRemovedShapes = TriggerPairFlag::eTEST_FOR_REMOVED_SHAPES;
		}
	}
	else
	{
#if PX_ENABLE_SIM_STATS
		PX_ASSERT(s0.getGeometryType() < PxGeometryType::eCONVEXMESH+1);  // The first has to be the trigger shape
		triggerPairStats[s0.getGeometryType()][s1.getGeometryType()]++;
#else
		PX_UNUSED(triggerPairStats);
#endif

		ShapeSim* primitive0 = &s0;
		ShapeSim* primitive1 = &s1;

		PX_ASSERT(primitive0->getFlags() & PxShapeFlag::eTRIGGER_SHAPE
			   || primitive1->getFlags() & PxShapeFlag::eTRIGGER_SHAPE);

		// Reorder them if needed
		if(primitive0->getGeometryType() > primitive1->getGeometryType())
			Ps::swap(primitive0, primitive1);

		const Gu::GeomOverlapFunc overlapFunc =
			Gu::getOverlapFuncTable()[primitive0->getGeometryType()][primitive1->getGeometryType()];

		PX_ALIGN(16, PxTransform globalPose0);
		primitive0->getAbsPoseAligned(&globalPose0);

		PX_ALIGN(16, PxTransform globalPose1);
		primitive1->getAbsPoseAligned(&globalPose1);

		PX_ASSERT(overlapFunc);
		overlap = overlapFunc(	primitive0->getCore().getGeometry(), globalPose0,
								primitive1->getCore().getGeometry(), globalPose1,
								&tri->getTriggerCache());
	}

	const bool hadOverlap = tri->lastFrameHadContacts();
	if (hadOverlap)
	{
		if (!overlap)
			pairEvent = PxPairFlag::eNOTIFY_TOUCH_LOST;
	}
	else
	{
		if (overlap)
			pairEvent = PxPairFlag::eNOTIFY_TOUCH_FOUND;
	}
	tri->updateLastFrameHadContacts(overlap);

	const PxPairFlags triggeredFlags = pairEvent & pairFlags;
	if (triggeredFlags)
	{
		triggerPair.triggerShape = s0.getPxShape();
		triggerPair.otherShape = s1.getPxShape();
		triggerPair.status = PxPairFlag::Enum(PxU32(pairEvent));
		triggerPair.flags = PxTriggerPairFlags(testForRemovedShapes);

		const RigidCore& rigidCore0 = s0.getRbSim().getRigidCore();
		const RigidCore& rigidCore1 = s1.getRbSim().getRigidCore();

		triggerPair.triggerActor = static_cast<PxRigidActor*>(rigidCore0.getPxActor());
		triggerPair.otherActor = static_cast<PxRigidActor*>(rigidCore1.getPxActor());

		triggerPairExtra = TriggerPairExtraData(s0.getID(), s1.getID(),
									rigidCore0.getOwnerClient(), rigidCore1.getOwnerClient(),
									rigidCore0.getClientBehaviorFlags(), rigidCore1.getClientBehaviorFlags());
		return true;
	}
	return false;
}


class TriggerContactTask : public Cm::Task
{
private:
	TriggerContactTask& operator = (const TriggerContactTask&);

public:
	TriggerContactTask(	Interaction* const* triggerPairs, PxU32 triggerPairCount, Ps::Mutex& lock,
						TriggerInteraction** pairsToDeactivate, volatile PxI32& pairsToDeactivateCount,
						Scene& scene)
		:
		Cm::Task(scene.getContextId()),
		mTriggerPairs(triggerPairs),
		mTriggerPairCount(triggerPairCount),
		mLock(lock),
		mPairsToDeactivate(pairsToDeactivate),
		mPairsToDeactivateCount(pairsToDeactivateCount),
		mScene(scene)
	{
	}

	virtual void runInternal()
	{
		SimStats::TriggerPairCountsNonVolatile triggerPairStats;
#if PX_ENABLE_SIM_STATS
		PxMemZero(&triggerPairStats, sizeof(SimStats::TriggerPairCountsNonVolatile));
#endif
		PxTriggerPair triggerPair[sTriggerPairsPerTask];
		TriggerPairExtraData triggerPairExtra[sTriggerPairsPerTask];
		PxU32 triggerReportItemCount = 0;

		TriggerInteraction* deactivatePairs[sTriggerPairsPerTask];
		PxI32 deactivatePairCount = 0;

		for(PxU32 i=0; i < mTriggerPairCount; i++)
		{
			TriggerInteraction* tri = static_cast<TriggerInteraction*>(mTriggerPairs[i]);

			PX_ASSERT(tri->readInteractionFlag(InteractionFlag::eIS_ACTIVE));
			
			if (findTriggerContacts(tri, false, false, triggerPair[triggerReportItemCount], triggerPairExtra[triggerReportItemCount], triggerPairStats))
				triggerReportItemCount++;

			if (!(tri->readFlag(TriggerInteraction::PROCESS_THIS_FRAME)))
			{
				// active trigger pairs for which overlap tests were not forced should remain in the active list
				// to catch transitions between overlap and no overlap
				continue;
			}
			else
			{
				tri->clearFlag(TriggerInteraction::PROCESS_THIS_FRAME);

				// explicitly scheduled overlap test is done (after object creation, teleport, ...). Check if trigger pair should remain active or not.

				if (!tri->onActivate(0))
				{
					PX_ASSERT(tri->readInteractionFlag(InteractionFlag::eIS_ACTIVE));
					// Why is the assert enough?
					// Once an explicit overlap test is scheduled, the interaction can not get deactivated anymore until it got processed.

					tri->clearInteractionFlag(InteractionFlag::eIS_ACTIVE);
					deactivatePairs[deactivatePairCount] = tri;
					deactivatePairCount++;
				}
			}
		}

		if (triggerReportItemCount)
		{
			PxTriggerPair* triggerPairBuffer;
			TriggerPairExtraData* triggerPairExtraBuffer;

			{
				Ps::Mutex::ScopedLock lock(mLock);

				mScene.reserveTriggerReportBufferSpace(triggerReportItemCount, triggerPairBuffer, triggerPairExtraBuffer);

				PxMemCopy(triggerPairBuffer, triggerPair, sizeof(PxTriggerPair) * triggerReportItemCount);
				PxMemCopy(triggerPairExtraBuffer, triggerPairExtra, sizeof(TriggerPairExtraData) * triggerReportItemCount);
			}
		}

		if (deactivatePairCount)
		{
			PxI32 newSize = Ps::atomicAdd(&mPairsToDeactivateCount, deactivatePairCount);
			PxMemCopy(mPairsToDeactivate + newSize - deactivatePairCount, deactivatePairs, sizeof(TriggerInteraction*) * deactivatePairCount);
		}

#if PX_ENABLE_SIM_STATS
		SimStats& simStats = mScene.getStatsInternal();
		for(PxU32 i=0; i < PxGeometryType::eCONVEXMESH+1; i++)
		{
			for(PxU32 j=0; j < PxGeometryType::eGEOMETRY_COUNT; j++)
			{
				if (triggerPairStats[i][j] != 0)
					Ps::atomicAdd(&simStats.numTriggerPairs[i][j], triggerPairStats[i][j]);
			}
		}
#endif
	}

	virtual const char* getName() const
	{
		return "ScNPhaseCore.triggerInteractionWork";
	}

public:
	static const PxU32 sTriggerPairsPerTask = 64;

private:
	Interaction* const* mTriggerPairs;
	const PxU32 mTriggerPairCount;
	Ps::Mutex& mLock;
	TriggerInteraction** mPairsToDeactivate;
	volatile PxI32& mPairsToDeactivateCount;
	Scene& mScene;
};

}  // namespace Sc
}  // namespace physx


void Sc::NPhaseCore::processTriggerInteractions(PxBaseTask* continuation)
{
	PX_ASSERT(!mTmpTriggerProcessingBlock);
	PX_ASSERT(mTriggerPairsToDeactivateCount == 0);

	Scene& scene = mOwnerScene;

	// Triggers
	Interaction** triggerInteractions = mOwnerScene.getActiveInteractions(InteractionType::eTRIGGER);
	const PxU32 pairCount = mOwnerScene.getNbActiveInteractions(InteractionType::eTRIGGER);

	if (pairCount > 0)
	{
		const PxU32 taskCountWithoutRemainder = pairCount / TriggerContactTask::sTriggerPairsPerTask;
		const PxU32 maxTaskCount = taskCountWithoutRemainder + 1;
		const PxU32 pairPtrSize = pairCount * sizeof(TriggerInteraction*);
		const PxU32 memBlockSize = pairPtrSize + (maxTaskCount * sizeof(TriggerContactTask));
		void* triggerProcessingBlock = scene.getLowLevelContext()->getScratchAllocator().alloc(memBlockSize, true);
		if (triggerProcessingBlock)
		{
			const bool hasMultipleThreads = scene.getTaskManager().getCpuDispatcher()->getWorkerCount() > 1;
			const bool moreThanOneBatch = pairCount > TriggerContactTask::sTriggerPairsPerTask;
			const bool scheduleTasks = hasMultipleThreads && moreThanOneBatch;
			// when running on a single thread, the task system seems to cause the main overhead (locking and atomic operations
			// seemed less of an issue). Hence, the tasks get run directly in that case. Same if there is only one batch.

			mTmpTriggerProcessingBlock = triggerProcessingBlock;  // note: gets released in the continuation task
			if (scheduleTasks)
				mMergeProcessedTriggerInteractions.setContinuation(continuation);

			TriggerInteraction** triggerPairsToDeactivateWriteBack = reinterpret_cast<TriggerInteraction**>(triggerProcessingBlock);
			TriggerContactTask* triggerContactTaskBuffer = reinterpret_cast<TriggerContactTask*>(reinterpret_cast<PxU8*>(triggerProcessingBlock) + pairPtrSize);

			PxU32 remainder = pairCount;
			while(remainder)
			{
				const PxU32 nb = remainder > TriggerContactTask::sTriggerPairsPerTask ? TriggerContactTask::sTriggerPairsPerTask : remainder;
				remainder -= nb;

				TriggerContactTask* task = triggerContactTaskBuffer;
				task = PX_PLACEMENT_NEW(task, TriggerContactTask(	triggerInteractions, nb, mTriggerWriteBackLock,
																	triggerPairsToDeactivateWriteBack, mTriggerPairsToDeactivateCount, scene));
				if (scheduleTasks)
				{
					task->setContinuation(&mMergeProcessedTriggerInteractions);
					task->removeReference();
				}
				else
					task->runInternal();

				triggerContactTaskBuffer++;
				triggerInteractions += nb;
			}

			if (scheduleTasks)
				mMergeProcessedTriggerInteractions.removeReference();
			else
				mMergeProcessedTriggerInteractions.runInternal();
		}
		else
		{
			Ps::getFoundation().getErrorCallback().reportError(PxErrorCode::eOUT_OF_MEMORY, "Temporary memory for trigger pair processing could not be allocated. Trigger overlap tests will not take place.", __FILE__, __LINE__);
		}
	}
}

void Sc::NPhaseCore::mergeProcessedTriggerInteractions(PxBaseTask*)
{
	if (mTmpTriggerProcessingBlock)
	{
		// deactivate pairs that do not need trigger checks any longer (until woken up again)
		TriggerInteraction** triggerPairsToDeactivate = reinterpret_cast<TriggerInteraction**>(mTmpTriggerProcessingBlock);
		for(PxI32 i=0; i < mTriggerPairsToDeactivateCount; i++)
		{
			mOwnerScene.notifyInteractionDeactivated(triggerPairsToDeactivate[i]);
		}
		mTriggerPairsToDeactivateCount = 0;

		mOwnerScene.getLowLevelContext()->getScratchAllocator().free(mTmpTriggerProcessingBlock);
		mTmpTriggerProcessingBlock = NULL;
	}
}

void Sc::NPhaseCore::visualize(Cm::RenderOutput& renderOut, PxsContactManagerOutputIterator& outputs)
{
	if(mOwnerScene.getVisualizationScale() == 0.0f)
		return;

	Interaction** interactions = mOwnerScene.getActiveInteractions(InteractionType::eOVERLAP);
	PxU32 nbActiveInteractions = mOwnerScene.getNbActiveInteractions(InteractionType::eOVERLAP);
	while(nbActiveInteractions--)
		static_cast<ShapeInteraction*>(*interactions++)->visualize(renderOut, outputs);
}

void Sc::NPhaseCore::processPersistentContactEvents(PxsContactManagerOutputIterator& outputs)
{
	//TODO: put back this optimization -- now we have to do this stuff if at least one client has a callback registered.
	// if (ownerScene.getSimulatonEventCallbackFast() != NULL)
	{
		// Go through ShapeInteractions which requested persistent contact event reports. This is necessary since there are no low level events for persistent contact.
		ShapeInteraction*const* persistentEventPairs = getCurrentPersistentContactEventPairs();
		PxU32 size = getCurrentPersistentContactEventPairCount();
		while(size--)
		{
			ShapeInteraction* pair = *persistentEventPairs++;
			if(size)
			{
				ShapeInteraction* nextPair = *persistentEventPairs;
				Ps::prefetchLine(nextPair);
			}

			ActorPair* aPair = pair->getActorPair();
			Ps::prefetchLine(aPair);

			PX_ASSERT(pair->hasTouch());
			PX_ASSERT(pair->isReportPair());

			const PxU32 pairFlags = pair->getPairFlags();
			if ((pairFlags & PxU32(PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eDETECT_DISCRETE_CONTACT)) == PxU32(PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eDETECT_DISCRETE_CONTACT))
			{
				// do not process the pair if only eDETECT_CCD_CONTACT is enabled because at this point CCD did not run yet. Plus the current CCD implementation can not reliably provide eNOTIFY_TOUCH_PERSISTS events
				// for performance reasons.
				//KS - filter based on edge activity!

				const BodySim* bodySim0 = pair->getShape0().getBodySim();
				const BodySim* bodySim1 = pair->getShape1().getBodySim();
				PX_ASSERT(bodySim0);  // the first shape always belongs to a dynamic body

				if (bodySim0->isActive() || 
					(bodySim1 && bodySim1->isActive()))
				{
					pair->processUserNotification(PxPairFlag::eNOTIFY_TOUCH_PERSISTS, 0, false, 0, false, outputs);
				}
			}
		}
	}
}

void Sc::NPhaseCore::fireCustomFilteringCallbacks(PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	PX_PROFILE_ZONE("Sim.fireCustomFilteringCallbacks", mOwnerScene.getContextId());

	PxSimulationFilterCallback* callback = mOwnerScene.getFilterCallbackFast();

	if (callback)
	{
		// Ask user for pair filter status changes
		PxU32 pairID;
		PxFilterFlags filterFlags;
		PxPairFlags pairFlags;
		while(callback->statusChange(pairID, pairFlags, filterFlags))
		{
			const FilterPair& fp = (*mFilterPairManager)[pairID];

			PX_ASSERT(fp.getType() != FilterPair::INVALID);
			// Check if the user tries to update a pair even though he deleted it earlier in the same frame

			filterFlags = checkFilterFlags(filterFlags);

			if (fp.getType() == FilterPair::ELEMENT_ELEMENT)
			{
				ElementSimInteraction* ei = fp.getPtr<ElementSimInteraction>();
				PX_ASSERT(ei->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR));

				PxFilterInfo finfo;
				finfo.filterFlags = filterFlags;
				finfo.pairFlags = pairFlags;
				finfo.filterPairIndex = pairID;

				ElementSimInteraction* refInt = refilterInteraction(ei, &finfo, true, outputs, useAdaptiveForce);

				// this gets called at the end of the simulation -> there should be no dirty interactions around
				PX_ASSERT(!refInt->readInteractionFlag(InteractionFlag::eIN_DIRTY_LIST));
				PX_ASSERT(!refInt->getDirtyFlags());

				if ((refInt == ei) && (refInt->getType() == InteractionType::eOVERLAP))  // No interaction conversion happened, the pairFlags were just updated
				{
					static_cast<ShapeInteraction*>(refInt)->updateState(InteractionDirtyFlag::eFILTER_STATE);
				}
			}
			else
			{
				PX_ASSERT(fp.getType() == FilterPair::ELEMENT_ACTOR);
				ActorElementPair* aep = fp.getPtr<ActorElementPair>();

				PxFilterInfo finfo;

				if ((filterFlags & PxFilterFlag::eNOTIFY) != PxFilterFlag::eNOTIFY)
				{
					mFilterPairManager->releaseIndex(pairID);
					aep->markAsFilterPair(false);
				}
				else
					finfo.filterPairIndex = pairID;

				finfo.filterFlags = filterFlags;
				finfo.pairFlags = pairFlags;

				aep->setPairFlags(pairFlags);
				if (filterFlags & PxFilterFlag::eKILL)
					aep->markAsKilled(true);
				else if (filterFlags & PxFilterFlag::eSUPPRESS)
					aep->markAsSuppressed(true);

#if PX_USE_PARTICLE_SYSTEM_API
				ActorSim* actor = &aep->getActor();
				ElementSim* element = &aep->getElement();

				ElementSim::ElementInteractionReverseIterator iter = element->getElemInteractionsReverse();
				ElementSimInteraction* interaction = iter.getNext();
				while(interaction)
				{
					if(interaction->getType() == InteractionType::ePARTICLE_BODY)
					{
						ParticleElementRbElementInteraction* pri = static_cast<ParticleElementRbElementInteraction*>(interaction);

						PX_ASSERT(pri->getElement0().getElementType() == ElementType::ePARTICLE_PACKET);
						if ((&pri->getElement1() == element) && (&pri->getActor0() == actor))
						{
							PX_ASSERT(pri->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR));
							refilterInteraction(pri, &finfo, true, outputs, useAdaptiveForce);

							// this gets called at the end of the simulation -> there should be no dirty interactions around
							PX_ASSERT(!pri->readInteractionFlag(InteractionFlag::eIN_DIRTY_LIST));
							PX_ASSERT(!pri->getDirtyFlags());
						}
					}

					interaction = iter.getNext();
				}
#endif // PX_USE_PARTICLE_SYSTEM_API
			}
		}
	}
}

void Sc::NPhaseCore::addToDirtyInteractionList(Interaction* pair)
{
	mDirtyInteractions.insert(pair);
}

void Sc::NPhaseCore::removeFromDirtyInteractionList(Interaction* pair)
{
	PX_ASSERT(mDirtyInteractions.contains(pair));

	mDirtyInteractions.erase(pair);
}

void Sc::NPhaseCore::updateDirtyInteractions(PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	// The sleeping SIs will be updated on activation
	// clow: Sleeping SIs are not awaken for visualization updates
	const bool dirtyDominance = mOwnerScene.readFlag(SceneInternalFlag::eSCENE_SIP_STATES_DIRTY_DOMINANCE);
	const bool dirtyVisualization = mOwnerScene.readFlag(SceneInternalFlag::eSCENE_SIP_STATES_DIRTY_VISUALIZATION);
	if (dirtyDominance || dirtyVisualization)
	{
		// Update all interactions.

		const PxU8 mask = Ps::to8((dirtyDominance ? InteractionDirtyFlag::eDOMINANCE : 0) | (dirtyVisualization ? InteractionDirtyFlag::eVISUALIZATION : 0));

		Interaction** it = mOwnerScene.getInteractions(Sc::InteractionType::eOVERLAP);
		PxU32 size = mOwnerScene.getNbInteractions(Sc::InteractionType::eOVERLAP);
		while(size--)
		{
			Interaction* pair = *it++;

			PX_ASSERT(pair->getType() == InteractionType::eOVERLAP);

			if (!pair->readInteractionFlag(InteractionFlag::eIN_DIRTY_LIST))
			{
				PX_ASSERT(!pair->getDirtyFlags());
				static_cast<ShapeInteraction*>(pair)->updateState(mask);
			}
			else
				pair->setDirty(mask);  // the pair will get processed further below anyway, so just mark the flags dirty
		}
	}

	// Update all interactions in the dirty list
	const PxU32 dirtyItcCount = mDirtyInteractions.size();
	Sc::Interaction* const* dirtyInteractions = mDirtyInteractions.getEntries();
	for (PxU32 i = 0; i < dirtyItcCount; i++)
	{
		Interaction* refInt = dirtyInteractions[i];
		Interaction* interaction = refInt;

		if (interaction->isElementInteraction() && interaction->needsRefiltering())
		{
			ElementSimInteraction* pair = static_cast<ElementSimInteraction*>(interaction);

			refInt = refilterInteraction(pair, NULL, false, outputs, useAdaptiveForce);
		}

		if (interaction == refInt)  // Refiltering might convert the pair to another type and kill the old one. In that case we don't want to update the new pair since it has been updated on creation.
		{
			const InteractionType::Enum iType = interaction->getType();
			if (iType == InteractionType::eOVERLAP)
				static_cast<ShapeInteraction*>(interaction)->updateState(0);
			else if (iType == InteractionType::eCONSTRAINTSHADER)
				static_cast<ConstraintInteraction*>(interaction)->updateState();

			interaction->setClean(false);  // false because the dirty interactions list gets cleard further below
		}
	}

	mDirtyInteractions.clear();
}

void Sc::NPhaseCore::releaseElementPair(ElementSimInteraction* pair, PxU32 flags, const PxU32 ccdPass, bool removeFromDirtyList, PxsContactManagerOutputIterator& outputs,
	bool useAdaptiveForce)
{
	pair->setClean(removeFromDirtyList);  // Removes the pair from the dirty interaction list etc.

	if (pair->readInteractionFlag(InteractionFlag::eIS_FILTER_PAIR) && pair->isLastFilterInteraction())
	{
		// Check if this is a filter callback pair
		const PxU32 filterPairIndex = mFilterPairManager->findIndex(pair);
		PX_ASSERT(filterPairIndex!=INVALID_FILTER_PAIR_INDEX);

		// Is this interaction removed because one of the pair object broadphase volumes got deleted by the user?
		const bool userRemovedVolume = ((flags & PairReleaseFlag::eBP_VOLUME_REMOVED) != 0);

		callPairLost(pair->getElement0(), pair->getElement1(), filterPairIndex, userRemovedVolume);

		mFilterPairManager->releaseIndex(filterPairIndex);
	}

	switch (pair->getType())
	{
		case InteractionType::eTRIGGER:
			{
				TriggerInteraction* tri = static_cast<TriggerInteraction*>(pair);
				PxTriggerPair triggerPair;
				TriggerPairExtraData triggerPairExtra;
				if (findTriggerContacts(tri, true, ((flags & PairReleaseFlag::eBP_VOLUME_REMOVED) != 0),
										triggerPair, triggerPairExtra, 
										const_cast<SimStats::TriggerPairCountsNonVolatile&>(mOwnerScene.getStatsInternal().numTriggerPairs)))
										// cast away volatile-ness (this is fine since the method does not run in parallel)
				{
					mOwnerScene.getTriggerBufferAPI().pushBack(triggerPair);
					mOwnerScene.getTriggerBufferExtraData().pushBack(triggerPairExtra);
				}
				mTriggerInteractionPool.destroy(tri);
			}
			break;
		case InteractionType::eMARKER:
			{
				ElementInteractionMarker* interactionMarker = static_cast<ElementInteractionMarker*>(pair);
				mInteractionMarkerPool.destroy(interactionMarker);
			}
			break;
		case InteractionType::eOVERLAP:
			{
				ShapeInteraction* si = static_cast<ShapeInteraction*>(pair);
				releaseShapeInteraction(si, flags, ccdPass, outputs, useAdaptiveForce);
			}
			break;
#if PX_USE_PARTICLE_SYSTEM_API
		case InteractionType::ePARTICLE_BODY:
			{
				ParticleElementRbElementInteraction* pbi = static_cast<ParticleElementRbElementInteraction*>(pair);
				pool_deleteParticleElementRbElementPair(pbi, flags, ccdPass);
			}
			break;
#endif
		case InteractionType::eCONSTRAINTSHADER:
		case InteractionType::eARTICULATION:
		case InteractionType::eTRACKED_IN_SCENE_COUNT:
		case InteractionType::eINVALID:
			PX_ASSERT(0);
			return;
	}
}

void Sc::NPhaseCore::lostTouchReports(ShapeInteraction* si, PxU32 flags, PxU32 ccdPass, PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	if (si->hasTouch())
	{
		if (si->isReportPair())
		{
			si->sendLostTouchReport((flags & PairReleaseFlag::eBP_VOLUME_REMOVED) != 0, ccdPass, outputs);
		}

		si->adjustCountersOnLostTouch(si->getShape0().getBodySim(), si->getShape1().getBodySim(), useAdaptiveForce);
	}

	ActorPair* aPair = si->getActorPair();
	if (aPair && aPair->decRefCount() == 0)
	{
		RigidSim* sim0 = static_cast<RigidSim*>(&si->getActor0());
		RigidSim* sim1 = static_cast<RigidSim*>(&si->getActor1());

		BodyPairKey pair;

		if (sim0->getRigidID() > sim1->getRigidID())
			Ps::swap(sim0, sim1);

		pair.mSim0 = sim0->getRigidID();
		pair.mSim1 = sim1->getRigidID();

		mActorPairMap.erase(pair);

		if (!aPair->isReportPair())
		{
			mActorPairPool.destroy(aPair);
		}
		else
		{
			ActorPairReport& apr = ActorPairReport::cast(*aPair);
			destroyActorPairReport(apr);
		}
	}
	si->clearActorPair();

	if (si->hasTouch() || (!si->hasKnownTouchState()))
	{
		BodySim* b0 = si->getShape0().getBodySim();
		BodySim* b1 = si->getShape1().getBodySim();

		if (flags & PairReleaseFlag::eWAKE_ON_LOST_TOUCH)
		{
			if (!b0 || !b1)
			{
				if (b0)
					b0->internalWakeUp();
				if (b1)
					b1->internalWakeUp();
			}
			else if (!si->readFlag(ShapeInteraction::CONTACTS_RESPONSE_DISABLED))
			{
				mOwnerScene.addToLostTouchList(b0, b1);
			}
		}
	}
}

void Sc::NPhaseCore::releaseShapeInteraction(ShapeInteraction* si, PxU32 flags, const PxU32 ccdPass, PxsContactManagerOutputIterator& outputs, bool useAdaptiveForce)
{
	if (flags & PairReleaseFlag::eSHAPE_BP_VOLUME_REMOVED)
	{
		lostTouchReports(si, flags, ccdPass, outputs, useAdaptiveForce);
	}
	mShapeInteractionPool.destroy(si);
}

void Sc::NPhaseCore::clearContactReportActorPairs(bool shrinkToZero)
{
	for(PxU32 i=0; i < mContactReportActorPairSet.size(); i++)
	{
		//TODO: prefetch?
		ActorPairReport* aPair = mContactReportActorPairSet[i];
		const PxU32 refCount = aPair->getRefCount();
		PX_ASSERT(aPair->isInContactReportActorPairSet());
		PX_ASSERT(refCount > 0);
		aPair->decRefCount();  // Reference held by contact callback
		if (refCount > 1)
		{
			aPair->clearInContactReportActorPairSet();
		}
		else
		{
			BodyPairKey pair;
			PxU32 actorAID = aPair->getActorAID();
			PxU32 actorBID = aPair->getActorBID();
			pair.mSim0 = PxMin(actorAID, actorBID);
			pair.mSim1 = PxMax(actorAID, actorBID);

			mActorPairMap.erase(pair);
			destroyActorPairReport(*aPair);
		}
	}

	if (!shrinkToZero)
		mContactReportActorPairSet.clear();
	else
		mContactReportActorPairSet.reset();
}

#if PX_USE_PARTICLE_SYSTEM_API
Sc::ParticleElementRbElementInteraction* Sc::NPhaseCore::insertParticleElementRbElementPair(ParticlePacketShape& particleShape, ShapeSim& rbShape, ActorElementPair* actorElementPair, const PxU32 ccdPass)
{
	// The ccdPass parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline is running.
	ParticleElementRbElementInteraction* pbi = mParticleBodyPool.construct(particleShape, rbShape, *actorElementPair, ccdPass);
	if (pbi)
		actorElementPair->incRefCount();

	return pbi;
}
#endif

void Sc::NPhaseCore::addToPersistentContactEventPairs(ShapeInteraction* si)
{
	// Pairs which request events which do not get triggered by the sdk and thus need to be tested actively every frame.
	PX_ASSERT(si->getPairFlags() & (PxPairFlag::eNOTIFY_TOUCH_PERSISTS | ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS));
	PX_ASSERT(si->mReportPairIndex == INVALID_REPORT_PAIR_ID);
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST));
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST));
	PX_ASSERT(si->hasTouch()); // only pairs which can from now on lose or keep contact should be in this list

	si->raiseFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST);
	if (mPersistentContactEventPairList.size() == mNextFramePersistentContactEventPairIndex)
	{
		si->mReportPairIndex = mPersistentContactEventPairList.size();
		mPersistentContactEventPairList.pushBack(si);
	}
	else
	{
		//swap with first entry that will be active next frame
		ShapeInteraction* firstDelayedSi = mPersistentContactEventPairList[mNextFramePersistentContactEventPairIndex];
		firstDelayedSi->mReportPairIndex = mPersistentContactEventPairList.size();
		mPersistentContactEventPairList.pushBack(firstDelayedSi);
		si->mReportPairIndex = mNextFramePersistentContactEventPairIndex;
		mPersistentContactEventPairList[mNextFramePersistentContactEventPairIndex] = si;
	}

	mNextFramePersistentContactEventPairIndex++;
}

void Sc::NPhaseCore::addToPersistentContactEventPairsDelayed(ShapeInteraction* si)
{
	// Pairs which request events which do not get triggered by the sdk and thus need to be tested actively every frame.
	PX_ASSERT(si->getPairFlags() & (PxPairFlag::eNOTIFY_TOUCH_PERSISTS | ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS));
	PX_ASSERT(si->mReportPairIndex == INVALID_REPORT_PAIR_ID);
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST));
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST));
	PX_ASSERT(si->hasTouch()); // only pairs which can from now on lose or keep contact should be in this list

	si->raiseFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST);
	si->mReportPairIndex = mPersistentContactEventPairList.size();
	mPersistentContactEventPairList.pushBack(si);
}

void Sc::NPhaseCore::removeFromPersistentContactEventPairs(ShapeInteraction* si)
{
	PX_ASSERT(si->getPairFlags() & (PxPairFlag::eNOTIFY_TOUCH_PERSISTS | ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS));
	PX_ASSERT(si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST));
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST));
	PX_ASSERT(si->hasTouch()); // only pairs which could lose or keep contact should be in this list

	PxU32 index = si->mReportPairIndex;
	PX_ASSERT(index != INVALID_REPORT_PAIR_ID);

	if (index < mNextFramePersistentContactEventPairIndex)
	{
		const PxU32 replaceIdx = mNextFramePersistentContactEventPairIndex - 1;

		if ((mNextFramePersistentContactEventPairIndex < mPersistentContactEventPairList.size()) && (index != replaceIdx))
		{
			// keep next frame persistent pairs at the back of the list
			ShapeInteraction* tmp = mPersistentContactEventPairList[replaceIdx];
			mPersistentContactEventPairList[index] = tmp;
			tmp->mReportPairIndex = index;
			index = replaceIdx;
		}

		mNextFramePersistentContactEventPairIndex--;
	}

	si->clearFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST);
	si->mReportPairIndex = INVALID_REPORT_PAIR_ID;
	mPersistentContactEventPairList.replaceWithLast(index);
	if (index < mPersistentContactEventPairList.size()) // Only adjust the index if the removed SIP was not at the end of the list
		mPersistentContactEventPairList[index]->mReportPairIndex = index;
}

void Sc::NPhaseCore::addToForceThresholdContactEventPairs(ShapeInteraction* si)
{
	PX_ASSERT(si->getPairFlags() & ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS);
	PX_ASSERT(si->mReportPairIndex == INVALID_REPORT_PAIR_ID);
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST));
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST));
	PX_ASSERT(si->hasTouch());

	si->raiseFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST);
	si->mReportPairIndex = mForceThresholdContactEventPairList.size();
	mForceThresholdContactEventPairList.pushBack(si);
}

void Sc::NPhaseCore::removeFromForceThresholdContactEventPairs(ShapeInteraction* si)
{
	PX_ASSERT(si->getPairFlags() & ShapeInteraction::CONTACT_FORCE_THRESHOLD_PAIRS);
	PX_ASSERT(si->readFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST));
	PX_ASSERT(!si->readFlag(ShapeInteraction::IS_IN_PERSISTENT_EVENT_LIST));
	PX_ASSERT(si->hasTouch());

	const PxU32 index = si->mReportPairIndex;
	PX_ASSERT(index != INVALID_REPORT_PAIR_ID);

	si->clearFlag(ShapeInteraction::IS_IN_FORCE_THRESHOLD_EVENT_LIST);
	si->mReportPairIndex = INVALID_REPORT_PAIR_ID;
	mForceThresholdContactEventPairList.replaceWithLast(index);
	if (index < mForceThresholdContactEventPairList.size()) // Only adjust the index if the removed SIP was not at the end of the list
		mForceThresholdContactEventPairList[index]->mReportPairIndex = index;
}

PxU8* Sc::NPhaseCore::reserveContactReportPairData(PxU32 pairCount, PxU32 extraDataSize, PxU32& bufferIndex)
{
	extraDataSize = Sc::ContactStreamManager::computeExtraDataBlockSize(extraDataSize);
	return mContactReportBuffer.allocateNotThreadSafe(extraDataSize + (pairCount * sizeof(Sc::ContactShapePair)), bufferIndex);
}

PxU8* Sc::NPhaseCore::resizeContactReportPairData(PxU32 pairCount, PxU32 extraDataSize, Sc::ContactStreamManager& csm)
{
	PX_ASSERT((pairCount > csm.maxPairCount) || (extraDataSize > csm.getMaxExtraDataSize()));
	PX_ASSERT((csm.currentPairCount == csm.maxPairCount) || (extraDataSize > csm.getMaxExtraDataSize()));
	PX_ASSERT(extraDataSize >= csm.getMaxExtraDataSize()); // we do not support stealing memory from the extra data part when the memory for pair info runs out

	PxU32 bufferIndex;
	Ps::prefetch(mContactReportBuffer.getData(csm.bufferIndex));

	extraDataSize = Sc::ContactStreamManager::computeExtraDataBlockSize(extraDataSize);
	PxU8* stream = mContactReportBuffer.reallocateNotThreadSafe(extraDataSize + (pairCount * sizeof(Sc::ContactShapePair)), bufferIndex, 16, csm.bufferIndex);
	PxU8* oldStream = mContactReportBuffer.getData(csm.bufferIndex);
	if(stream)
	{
		const PxU32 maxExtraDataSize = csm.getMaxExtraDataSize();
		if(csm.bufferIndex != bufferIndex)
		{
			if (extraDataSize <= maxExtraDataSize)
				PxMemCopy(stream, oldStream, maxExtraDataSize + (csm.currentPairCount * sizeof(Sc::ContactShapePair)));
			else
			{
				PxMemCopy(stream, oldStream, csm.extraDataSize);
				PxMemCopy(stream + extraDataSize, oldStream + maxExtraDataSize, csm.currentPairCount * sizeof(Sc::ContactShapePair));
			}
			csm.bufferIndex = bufferIndex;
		}
		else if (extraDataSize > maxExtraDataSize)
			PxMemMove(stream + extraDataSize, oldStream + maxExtraDataSize, csm.currentPairCount * sizeof(Sc::ContactShapePair));

		if (pairCount > csm.maxPairCount)
			csm.maxPairCount = Ps::to16(pairCount);
		if (extraDataSize > maxExtraDataSize)
			csm.setMaxExtraDataSize(extraDataSize);
	}
	return stream;
}

Sc::ActorPairContactReportData* Sc::NPhaseCore::createActorPairContactReportData()
{
	return mActorPairContactReportDataPool.construct();
}

void Sc::NPhaseCore::releaseActorPairContactReportData(ActorPairContactReportData* data)
{
	mActorPairContactReportDataPool.destroy(data);
}

#if PX_USE_PARTICLE_SYSTEM_API
void Sc::NPhaseCore::pool_deleteParticleElementRbElementPair(ParticleElementRbElementInteraction* pair, PxU32 flags, const PxU32 ccdPass)
{
	ActorElementPair* aep = pair->getActorElementPair();

	// The ccdPass parameter is needed to avoid concurrent interaction updates while the gpu particle pipeline is running.
	pair->ParticleElementRbElementInteraction::destroy(((flags & PairReleaseFlag::eSHAPE_BP_VOLUME_REMOVED) == PairReleaseFlag::eSHAPE_BP_VOLUME_REMOVED), ccdPass);
	mParticleBodyPool.destroy(pair);

	if (aep->decRefCount() == 0)
		mActorElementPairPool.destroy(aep);
}
#endif	// PX_USE_PARTICLE_SYSTEM_API
