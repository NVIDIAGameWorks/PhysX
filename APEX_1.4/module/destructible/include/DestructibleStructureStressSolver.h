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



#ifndef DESTRUCTIBLE_STRUCTURE_STRESS_SOLVER_H
#define DESTRUCTIBLE_STRUCTURE_STRESS_SOLVER_H

#include "PsUserAllocated.h"
#include "ApexUsingNamespace.h"
#include "PsArray.h"


#include "DestructibleScene.h"
#include "Scene.h"
#include "ApexDefs.h"
#include <PxPhysics.h>
#include "cooking/PxCooking.h"

#include <PxScene.h>
#include "extensions/PxExtensionsAPI.h"
#include "extensions/PxDefaultCpuDispatcher.h"
#include "extensions/PxDefaultSimulationFilterShader.h"
#include <PxFiltering.h>
#include <PxMaterial.h>
#include <PxFixedJoint.h> 
#include <PxConstraint.h>

namespace nvidia
{
namespace destructible
{

class DestructibleStructure;
class DestructibleActorImpl;
struct ActorChunkPair;
struct FractureEvent;

// a helper class for managing states using an array
template<class T, typename F, unsigned int N>
class ArraySet
{
public:
	explicit ArraySet(T v)
	{
		PX_COMPILE_TIME_ASSERT(N > 0);
#if defined(PX_PS4)
		typedef char PxCompileTimeAssert_Dummy2[(N * sizeof(T) == sizeof(*this)) ? 1 : -1] __attribute__((unused));
#else
		typedef char PxCompileTimeAssert_Dummy2[(N * sizeof(T) == sizeof(*this)) ? 1 : -1];
#endif
		::memset(static_cast<void*>(&t[0]), v, N * sizeof(T));
	}
	~ArraySet()
	{
	}
	void	set(unsigned int i, F f)
	{
		PX_ASSERT(i < N);
		PX_ASSERT(!isSet(i, f));
		t[i] |= f;
	}
	void	unset(unsigned int i, F f)
	{
		PX_ASSERT(i < N);
		PX_ASSERT(isSet(i, f));
		t[i] &= ~f;
	}
	bool	isSet(unsigned int i, F f) const
	{
		PX_ASSERT(i < N);
		return (0 != (f & t[i]));
	}
	void	reset(T v)
	{
		::memset(static_cast<void*>(&t[0]), v, N * sizeof(T));
	}
	const T		(&get() const) [N]
	{
		return t;
	}
	const T *	getPtr() const
	{
		return &t[0];
	}
private:
	T	t[N];
}; // class ArraySet

// Int-To-Type idiom
template<unsigned int I>
struct Int2Type
{
	enum
	{
		Value = I,
	};
}; // struct Int2Type

// a type container used to identify the different ways of evaluating stresses on the structure
struct StressEvaluationType
{
	enum Enum
	{
		EvaluateByCount = 0,
		EvaluateByMoment,
	};
}; // struct StressEvaluationType

// a helper data structure used for recording bit values
struct StructureScratchBuffer
{
public:
	StructureScratchBuffer(uint8_t * const bufferStart, uint32_t bufferCount);
	~StructureScratchBuffer();

	bool isOccupied(uint32_t index) const;
	void setOccupied(uint32_t index);

private:
	StructureScratchBuffer();
	StructureScratchBuffer& operator=(const StructureScratchBuffer&);

	enum ByteState
	{
		ByteEmpty	= 0x00,
		ByteExist	= 0xFF,
	};

	uint8_t * const	bufferStart;
	const uint32_t	bufferCount;
}; // struct StructureScratchBuffer

// a data structure used to collect and hold potential new island data
struct PeninsulaData
{
public:
	PeninsulaData();
	PeninsulaData(uint8_t peninsulaState, uint32_t chunkCount);
	PeninsulaData(uint8_t peninsulaState, uint32_t chunkCount, float aggregateMass, PxVec3 geometricCenter);
	~PeninsulaData();
	enum PeninsulaFlag
	{
		PeninsulaEncounteredExternalSupport	= (1 << 0),
		PeninsulaEncounteredAnotherWeakLink	= (1 << 1),
		PeninsulaInvalidMaxFlag				= (1 << 2),
	};

	bool						isValid() const;
	void						resetData();
	void						setFlag(PeninsulaData::PeninsulaFlag peninsulaFlag);
	void						setLinkedChunkData(const ActorChunkPair & actorChunkPair, Int2Type<StressEvaluationType::EvaluateByCount>);
	void						setLinkedChunkData(const ActorChunkPair & actorChunkPair, Int2Type<StressEvaluationType::EvaluateByMoment>);
	void						setRootLinkedChunkIndex(uint32_t linkedChunkIndex);
	void						assimilate(const PeninsulaData & that, Int2Type<StressEvaluationType::EvaluateByCount>);
	void						assimilate(const PeninsulaData & that, Int2Type<StressEvaluationType::EvaluateByMoment>);
	bool						hasFlag(PeninsulaData::PeninsulaFlag peninsulaFlag) const;
	uint32_t				getDataChunkCount() const;
	float				getDataAggregateMass() const;
	const PxVec3	&		getDataGeometricCenter() const;
	uint32_t				getRootLinkedChunkIndex() const;
private:
	const PeninsulaData			operator+	(const PeninsulaData &);
	PeninsulaData &				operator+=	(const PeninsulaData &);

	uint32_t				rootLinkedChunkIndex;
	uint8_t					peninsulaState;
	uint32_t				chunkCount;
	float				aggregateMass;
	PxVec3				geometricCenter;
}; // struct PeninsulaData

// a data structure used to hold minimally required fracture data
struct FractureEventProxy : public UserAllocated
{
public:
	uint32_t				rootLinkedChunkIndex;
	uint32_t				chunkIndexInAsset;
	uint32_t				destructibleId;
	uint32_t				fractureEventProxyFlags;
	PxVec3				rootLinkedChunkPosition;
	PxVec3				impulse;
}; // struct FractureEventProxy

// a data structure with a 1-N convertible relationship with fracture event(s)
struct SnapEvent : public UserAllocated
{
public:
	static SnapEvent*			instantiate(FractureEventProxy * fractureEventProxyBufferStart, uint32_t fractureEventProxyBufferCount, float tickSecondsToSnap);

	void						onDestroy();
	bool						isRipeAfterUpdatingTime(float deltaTime);
	bool						isExpired() const;
	uint32_t				getFractureEventProxyCount() const;
	const FractureEventProxy&	getFractureEventProxy(uint32_t index) const;

private:
	SnapEvent(FractureEventProxy* fractureEventProxyBufferStart, uint32_t fractureEventProxyBufferCount, float tickSecondsToSnap);
	SnapEvent& operator=(const SnapEvent&);

	~SnapEvent();
	SnapEvent();

	FractureEventProxy*			fractureEventProxyBufferStart;
	const uint32_t			fractureEventProxyBufferCount;
	float				tickSecondsRemainingToSnap;
}; // struct SnapEvent

// a solver with a 1-1 or 0-1 relationship with a destructible structure
class DestructibleStructureStressSolver : public UserAllocated
{
public:
	explicit DestructibleStructureStressSolver(const DestructibleStructure & bindedStructureAlias, float strength);

	~DestructibleStructureStressSolver();

	void							onTick(float deltaTime);
	void							onUpdate(uint32_t linkedChunkIndex);
	void							onResolve();
private:
	DestructibleStructureStressSolver();
	DestructibleStructureStressSolver(const DestructibleStructureStressSolver &);
	DestructibleStructureStressSolver & operator= (const DestructibleStructureStressSolver &);
	enum StructureLinkFlag
	{
		LinkedChunkExist			= (1 << 0),
		LinkedChunkBroken			= (1 << 1),
		LinkedChunkStrained			= (1 << 2),
		LinkedChunkInvalidMaxFlag	= (1 << 3),
	};
	enum LinkTestParameters
	{
		LowerLimitActivationCount	= 2,
		UpperLimitActivationCount	= 3,
	};
	enum LinkTraverseParameters
	{
		PathTraversalCount			= 2,
	};
	static const StressEvaluationType::Enum StressEvaluationEnum = StressEvaluationType::EvaluateByMoment;

	void							processLinkedChunkIndicesForEvaluation(physx::Array<uint32_t> & linkedChunkIndicesForEvaluation);
	void							evaluateForPotentialIslands(const physx::Array<uint32_t> & linkedChunkIndicesForEvaluation);
	bool							passLinkCountTest(uint32_t linkedChunkIndex, physx::Array<uint32_t> & unbrokenAdjacentLinkedChunkIndices) const;
	bool							passLinkAdjacencyTest(const physx::Array<uint32_t> & unbrokenAdjacentLinkedChunkIndices, uint32_t (&linkedChunkIndicesForTraversal)[2]) const;
	void							evaluateForPotentialPeninsulas(uint32_t rootLinkedChunkIndex, const uint32_t (&linkedChunkIndicesForTraversal)[2]);
	PeninsulaData					traverseLink(uint32_t linkedChunkIndex, StructureScratchBuffer & traverseRecord) const;
	void							evaluatePeninsulas(const PeninsulaData (&peninsulasForEvaluation)[2]);
	SnapEvent *						interpret(const PeninsulaData & peninsulaData, Int2Type<StressEvaluationType::EvaluateByCount>) const;
	SnapEvent *						interpret(const PeninsulaData & peninsulaData, Int2Type<StressEvaluationType::EvaluateByMoment>) const;
	const FractureEvent &			interpret(const FractureEventProxy & fractureEventProxy) const;

	bool							isLinkedChunkBroken(uint32_t linkedChunkIndex) const;
	void							setLinkedChunkBroken(uint32_t linkedChunkIndex);
	bool							isLinkedChunkStrained(uint32_t linkedChunkIndex) const;
	void							setLinkedChunkStrained(uint32_t linkedChunkIndex);
	void							getUnbrokenAdjacentLinkedChunkIndices(uint32_t linkedChunkIndex, physx::Array<uint32_t> & unbrokenAdjacentLinkedChunkIndices) const;
	bool							areUnbrokenLinkedChunksAdjacent(uint32_t linkedChunkIndex, uint32_t linkedChunkIndexSubject) const;
	ActorChunkPair					getActorChunkPair(uint32_t chunkIndexInStructure) const;
	StructureScratchBuffer			acquireScratchBuffer() const;
	void							relinquishScratchBuffer() const;
	DestructibleStructure &			getBindedStructureMutable();
	bool							assertLinkedChunkIndexOk(uint32_t linkedChunkIndex) const;
	bool							assertLinkedChunkIndicesForEvaluationOk(const physx::Array<uint32_t> & linkedChunkIndicesForEvaluation) const;

	const DestructibleStructure &	bindedStructureAlias;
	uint32_t					structureLinkCount;
	uint8_t *					structureLinkCondition;
	uint8_t *					scratchBuffer;
	mutable bool					scratchBufferLocked;
	float					userTimeDelay;
	float					userMassThreshold;
	physx::Array<uint32_t>		recentlyBrokenLinkedChunkIndices;
	physx::Array<uint32_t>		strainedLinkedChunkIndices;
	physx::Array<SnapEvent*>		snapEventContainer;

	//Shadow scene to simulate a set of linked rigidbodies
	public :
	struct LinkedJoint 
	{
		uint32_t						actor1Index;
		uint32_t						actor2Index;
		physx::PxFixedJoint *			joint;
		bool							isDestroyed;
	};

	struct ShadowActor
	{
		physx::PxRigidDynamic*		actor;
		float						currentForce;
		float						maxInitialForce;
		bool						isDestroyed;
	};
private: 
	//The basic assumption is that the initialize structure is valid, so we should know the maximum force for each link through initializing, and use it to calculate the threshold.
	//So initializingMaxForce is to judge if it is initializing or something breaks the initializing. 
	bool									initializingMaxForce;

	// The isNoChunkWarmingUpdate is the variable that fewer chunks updating during the warming, in this case, just quit warming. 
	bool									isNoChunkWarmingUpdate;

	// The isSimulating symbol indicates if the shadowScene is simulating.
	bool									isSimulating;

	uint32_t							continuousNoUpdationCount;
	physx::Array<ShadowActor>				shadowActors;

	//Hash the global chunk index to the support chunk's index
	physx::Array<uint32_t>						globalHashedtoSupportChunks;

	//initialWarmingKillList caches the chunks breaking during the warming stage, after warming is done, all chunks in this list will be removed from shadow scene.
	physx::Array<uint32_t>						initialWarmingKillList;

	//islandKillList caches the chunks removed by the reason of new island generation, all supported chunks will be removed from the shadow scene at once. 
	physx::Array<uint32_t>						islandKillList;

	//forces[i] are the current force for supported chunk i in the scene.  
	physx::Array<float>					forces; 
	
	//linked joints are constructed as a network for chunk breaking prediction.
	physx::Array<LinkedJoint>				linkedJoint;

	//The SDK is usde to create the shadow scene
	physx::PxPhysics *						newSceneSDK;	

	//The shadow scene is used to construct linked joints which are used for chunk breaking prediction
	physx::PxScene*							shadowScene;

	//Size of the supported chunk in the real scene.
	uint32_t							supportChunkSize;

	//The supportStrength is used for the stress solver.  As this value is increased it takes more force to break apart the chunks under the effect of stress solver.
	float							supportStrength;

	//currentWarmingFrame increases by 1 in the warming stage. 
	uint32_t							currentWarmingFrame;

	//initialWarmFrames is used with currentWarmingFrame. If  currentWarmingFrame is larger than initialWarmFrames, the warming will stop.
	uint32_t							initialWarmFrames;

	//If no fracturing during one activCycle, just turn the mode to sleep.
	uint32_t							activeCycle;		

	//It is used with the activeCycle, 
	uint32_t							currentActiveSimulationFrame;

	//If any chunks break isActiveWakedUp is set to true indicates that the stress solver will wake up.
	bool									isActiveWakedUp;

	bool									isSleep;
public :
	// The isPhysxBasedSim is the variable that uses by the customer if they want to choose the physics based simulation mode. 
	bool									isPhysxBasedSim;

	// The isCustomEnablesSimulating symbol is used by the user if the they would like to stop the shadowScene simulating after creating the shadow scene. 
	bool									isCustomEnablesSimulating;

	//Functions: 
	uint32_t 							predictBreakChunk(float deltaTime, float supportStrength_);
	//create the shadow scene 
	void							createShadowScene(uint32_t initialWarmFrames_,uint32_t actCycle, float supStrength);
	//calculate current maximum force
	void							calculateMaxForce(uint32_t& index, float supportStrength_);  

	//If new island genereates, remove the chunks in the island
	void							removeChunkFromIsland(uint32_t index);
	//Break specific chunk 
	void							breakChunks(uint32_t index);
	//Entry to use physx based stress solver
	void							physcsBasedStressSolverTick();
	void							removeChunkFromShadowScene(bool localIndex, uint32_t index);
	void							resetSupportStrength(float supportStrength_);
	void							resetActiveCycle(uint32_t actCycle);
	void							resetInitialWarmFrames(uint32_t iniWarmFrames);
	void							enablePhyStressSolver(float strength);
	// The stress solver will stop simulating after calling disablePhyStressSolver; 
	void							disablePhyStressSolver();
	void							removePhysStressSolver();
}; // class DestructibleStructureStressSolver

} // namespace destructible
} // namespace nvidia

#endif // DESTRUCTIBLE_STRUCTURE_STRESS_SOLVER_H

#if 0
/*
issues and improvements:
1) performance - traverseLink() may generate a stack that is too deep - explore ways to return out earlier.
		1.1) tried to return out earlier by returning when we hit a strained link, but this will not work for some composite cases
		1.2) basic assumption that a strained link will go on and find an externally supported chunk may be true initially,
			 but this condition needs to be verified again subsequently.
		1.3) tried to evaluate past strained links for evaluation runs that contain recently broken links that used to be strained,
			 this seemed promising initially, but under an uncommon, yet possible scenario, an unsupported peninsula could be instantly
			 generated by breaking links, without the intermediate stage of it being strained. Thus, this method was abandoned.
2) performance - do less traversing by assimilating previously-traversed peninsula data
		2.1) while this will not lessen the stack generated, performance could be improved if somehow we can cache peninsula information
			 for an evaluation run. We can reliably reuse this peninsula information instead traversing past it again. Using the flag
			 PeninsulaEncounteredAnotherWeakLink could be useful for this purpose. Doing this may require significant refactoring.
3) realism - widen criteria for qualifying weak links
		3.1) we can improve passLinkCountTest() and passLinkAdjacencyTest() to qualify more links for peninsula assessment. For example,
			 we can try extending it to assess 4 links as well, and also possibly to assess 3 traversal paths.
4) realism - make better use of physical data to create better break-offs
		4.1) we can try and make a peninsula break off in a better direction, such as about the pivot (the weak link), instead of it
			 dropping down vertically.
*/
#endif // notes
