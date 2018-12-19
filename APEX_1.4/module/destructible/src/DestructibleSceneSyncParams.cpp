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



#include "DestructibleScene.h"

namespace nvidia
{
namespace destructible
{

/*** SyncParams ***/
namespace destructibleDLL
{
	const char * debugMessage = NULL;

    bool error(const char * message)
    {
        APEX_INTERNAL_ERROR(message);
		if(NULL == debugMessage)
		{
			debugMessage = message;
		}
        return true;
    }
}; //namespace destructibleDLL

// only if, for safety, non-logical use of 'if'
// typically to replace assert-if combos
// 'else' after an 'oif' cannot (shouldn't) be used, by design

//  if(A) {B} => A is sufficient for B => if(X) {B} is possible
// oif(A) {B} => A is necessary  for B => B is only possible if at least A
// iff(A) {B} => A is both necessary and sufficient for B => bi-directional (A <=> B)
#if 1
#define oif(condition, message) if((condition) ? 0 : 1) {                                   \
                                    PX_ASSERT((condition) && message);                      \
                                    destructibleDLL::error(message);   \
                                } else
#else
#define oif(empty0, empty1)
#endif // developer debug mode

#define STEP_BY(start, stepSize) static_cast<void*>(reinterpret_cast<char*>(static_cast<void*>(start)) + (stepSize))
#define STEP_BY_K(start, stepSize) static_cast<const void*>(reinterpret_cast<const char*>(static_cast<const void*>(start)) + (stepSize))

typedef DestructibleScene::SyncParams SyncParams;

const char * SyncParams::errorMissingUserActorID				= "actor is not marked for sync";
const char * SyncParams::errorMissingFlagReadDamageEvents		= "actor is not marked for reading damage events";
const char * SyncParams::errorMissingFlagReadFractureEvents		= "actor is not marked for reading fracture events";
const char * SyncParams::errorMissingFlagReadChunkMotion		= "actor is not marked for reading chunk motion";
const char * SyncParams::errorOutOfBoundsActorChunkIndex		= "chunk index exceed chunk count for actor";
const char * SyncParams::errorOutOfBoundsStructureChunkIndex	= "chunk index exceed chunk count for structure";
const char * SyncParams::errorOverrunBuffer						= "user buffer is overrun - may contain invalid data and/or invalid size";

SyncParams::SyncParams(const ModuleDestructibleImpl::SyncParams & moduleParams_)
:ditchStaleBuffers(false)
,lockSyncParams(false)
,moduleParams(moduleParams_)
,damageEventWriteSource(NULL)
,fractureEventWriteSource(NULL)
{
	PX_ASSERT(NULL != &moduleParams);
	PX_ASSERT(segmentSizeChecker.empty());
	PX_ASSERT(damageEventReadSource.empty());
	PX_ASSERT(fractureEventReadSource.empty());
	PX_ASSERT(chunksWithUserControlledChunk.empty());
	::memset(static_cast<void*>(&damageEventUserInstance), 0x00, sizeof(damageEventUserInstance));
	::memset(static_cast<void*>(&damageEventImplInstance), 0x00, sizeof(damageEventImplInstance));
	::memset(static_cast<void*>(&fractureEventUserInstance), 0x00, sizeof(fractureEventUserInstance));
	::memset(static_cast<void*>(&fractureEventImplInstance), 0x00, sizeof(fractureEventImplInstance));
}

SyncParams::~SyncParams()
{
    PX_ASSERT(chunksWithUserControlledChunk.empty());
	PX_ASSERT(fractureEventReadSource.empty());
	PX_ASSERT(damageEventReadSource.empty());
	PX_ASSERT(segmentSizeChecker.empty());
	PX_ASSERT(NULL == fractureEventWriteSource);
	PX_ASSERT(NULL == damageEventWriteSource);
}

bool SyncParams::setSyncActor(uint32_t entryIndex, DestructibleActorImpl * entry, DestructibleActorImpl *& erasedEntry)
{
	bool validEntry = false;
	erasedEntry = NULL;
	const bool reservedIndex = (0 == entryIndex);
	if(!reservedIndex)
	{
		//set up syncActorRecord if necessary
		if(syncActorRecord.getActorContainer().size() <= entryIndex)
		{
			syncActorRecord.onRebuild(entryIndex + 1);
		}

		//update syncActorRecord if appropriate
		const bool validIndex = (!reservedIndex && (syncActorRecord.getActorContainer().size() > entryIndex));
		if(validIndex)
		{
			//determine type of operation
			const bool addEntry			= (NULL != entry && NULL == syncActorRecord.getActorContainer()[entryIndex]);
			const bool removeEntry		= (NULL == entry && NULL != syncActorRecord.getActorContainer()[entryIndex]);
			const bool replaceEntry		= (NULL != entry && NULL != syncActorRecord.getActorContainer()[entryIndex] && entry != syncActorRecord.getActorContainer()[entryIndex]);
			const bool sameEntry		= (NULL != entry && NULL != syncActorRecord.getActorContainer()[entryIndex] && entry == syncActorRecord.getActorContainer()[entryIndex]);
			const bool emptyEntry		= (NULL == entry && NULL == syncActorRecord.getActorContainer()[entryIndex]);
			PX_ASSERT(addEntry ? (!removeEntry && !replaceEntry && !sameEntry && !emptyEntry) : removeEntry ? (!replaceEntry && !sameEntry && !emptyEntry) : replaceEntry ? (!sameEntry && !emptyEntry) : sameEntry? (!emptyEntry) : emptyEntry);

			//update output arguments before update to syncActorRecord's containers
			if(addEntry || removeEntry || replaceEntry)
			{
				validEntry = true;
				if(removeEntry || replaceEntry)
				{
					erasedEntry = syncActorRecord.getActorContainer()[entryIndex];
				}
			}
			else
			{
				PX_ASSERT(sameEntry || emptyEntry);
				PX_UNUSED(sameEntry);
				PX_UNUSED(emptyEntry);
				PX_ASSERT(!"invalid use of function!");
			}

			//update syncActorRecord's containers
			if(validEntry)
			{
				//update index container
				if(addEntry)
				{
					PX_ASSERT(syncActorRecord.getActorContainer().size() > entryIndex);
					syncActorRecord.indexContainer.pushBack(entryIndex);
				}
				if(removeEntry)
				{
					bool found = false;
					for(uint32_t index = 0; index < syncActorRecord.getIndexContainer().size(); ++index)
					{
						if(entryIndex == syncActorRecord.getIndexContainer()[index])
						{
							if(syncActorRecord.getIndexContainer().size() == (index + 1))
								syncActorRecord.indexContainer.popBack();
							else
								syncActorRecord.indexContainer[index] = syncActorRecord.indexContainer.popBack();
							found = true;
							break;
						}
					}
					PX_VERIFY(found);
				}

				//update actor container
				if(addEntry || removeEntry || replaceEntry)
				{
					PX_ASSERT(syncActorRecord.getActorContainer().size() > entryIndex);
					syncActorRecord.actorContainer[entryIndex] = entry;
				}
			}
		}
	}
	PX_ASSERT(syncActorRecord.getActorContainer().empty() ? true : NULL == syncActorRecord.getActorContainer()[0]);
	PX_ASSERT(NULL != erasedEntry ? validEntry : true);
	return validEntry;
}

DestructibleActorImpl *	SyncParams::getSyncActor(uint32_t entryIndex) const
{
	DestructibleActorImpl * found = NULL;
	if((0 != entryIndex) && (syncActorRecord.getActorContainer().size() > entryIndex))
	{
		found = syncActorRecord.getActorContainer()[entryIndex];
		PX_ASSERT(NULL != found);
	}
	return found;
}

uint32_t SyncParams::getUserActorID(uint32_t destructibleID) const
{
	uint32_t userActorID = 0;
	const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
	const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
	const uint32_t indexContainerSize = indexContainer.size();
	for(uint32_t index = 0; index < indexContainerSize; ++index)
	{
		const DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
		PX_ASSERT(NULL != &currentActor);
		if(currentActor.getID() == destructibleID)
		{
			const DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParams();
			userActorID = actorParams.getUserActorID();
			PX_ASSERT(0 != userActorID);
			break;
		}
	}
	return userActorID;
}

void SyncParams::onPreProcessReadData(UserDamageEventHandler & callback, const physx::Array<UserDamageEvent> *& userSource)
{
	PX_ASSERT(NULL != &callback);
	userSource = NULL;
	const DamageEventHeader * bufferStart = processReadBuffer<UserDamageEventHandler, DamageEventHeader, DamageEventUnit>(callback);
	if(NULL != bufferStart)
	{
		loadDataForRead(bufferStart, userSource);
	}
}

void SyncParams::onPreProcessReadData(UserFractureEventHandler & callback, const physx::Array<UserFractureEvent> *& userSource)
{
	PX_ASSERT(NULL != &callback);
	userSource = NULL;
	const FractureEventHeader * bufferStart = processReadBuffer<UserFractureEventHandler, FractureEventHeader, FractureEventUnit>(callback);
	if(NULL != bufferStart)
	{
		loadDataForRead(bufferStart, userSource);
	}
}

void SyncParams::onPreProcessReadData(UserChunkMotionHandler & callback)
{
	PX_ASSERT(NULL != &callback);
	const ChunkTransformHeader * bufferStart = processReadBuffer<UserChunkMotionHandler, ChunkTransformHeader, ChunkTransformUnit>(callback);
	if(NULL != bufferStart)
	{
		loadDataForRead(bufferStart);
	}
}

DamageEvent & SyncParams::interpret(const UserDamageEvent & userDamageEvent_)
{
	PX_ASSERT(NULL != &userDamageEvent_);
	const DestructibleActorImpl & actorAlias = *(userDamageEvent_.actorAlias);
	PX_ASSERT(NULL != &actorAlias);
	const DamageEventUnit & userDamageEvent = *(userDamageEvent_.get());
	PX_ASSERT(NULL != &userDamageEvent);
	const bool isRadiusDamage = (ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(userDamageEvent.chunkIndex)) ? 0 != (DamageEvent::UseRadius & userDamageEvent.damageEventFlags) : false;
	bool validUserArguments = false;
	oif(((userDamageEvent.chunkIndex < actorAlias.getDestructibleAsset()->getChunkCount()) || isRadiusDamage), SyncParams::errorOutOfBoundsActorChunkIndex)
	{
		oif(((userDamageEvent.chunkIndex + actorAlias.getFirstChunkIndex()) < actorAlias.getStructure()->chunks.size()) || isRadiusDamage, SyncParams::errorOutOfBoundsStructureChunkIndex)
		{
			validUserArguments = true;
		}
	}
	damageEventImplInstance.resetFracturesInternal();
	damageEventImplInstance.destructibleID		= actorAlias.getID();
	damageEventImplInstance.damage				= userDamageEvent.damage;
	damageEventImplInstance.momentum			= userDamageEvent.momentum;
	damageEventImplInstance.radius				= userDamageEvent.radius;
	damageEventImplInstance.position			= userDamageEvent.position;
	damageEventImplInstance.direction			= userDamageEvent.direction;
	damageEventImplInstance.chunkIndexInAsset	= (int32_t)userDamageEvent.chunkIndex;
	damageEventImplInstance.flags				= userDamageEvent.damageEventFlags;
	if(!validUserArguments)
	{
		damageEventImplInstance.flags			|= DamageEvent::Invalid;
	}
	return damageEventImplInstance;
}

FractureEvent & SyncParams::interpret(const UserFractureEvent & userFractureEvent_)
{
	PX_ASSERT(NULL != &userFractureEvent_);
	const DestructibleActorImpl & actorAlias = *(userFractureEvent_.actorAlias);
	PX_ASSERT(NULL != &actorAlias);
	const FractureEventUnit & userFractureEvent = *(userFractureEvent_.get());
	PX_ASSERT(NULL != &userFractureEvent);
	bool validUserArguments = false;
	oif(userFractureEvent.chunkIndex < actorAlias.getDestructibleAsset()->getChunkCount(), SyncParams::errorOutOfBoundsActorChunkIndex)
	{
		oif((userFractureEvent.chunkIndex + actorAlias.getFirstChunkIndex()) < actorAlias.getStructure()->chunks.size(), SyncParams::errorOutOfBoundsStructureChunkIndex)
		{
			validUserArguments = true;
		}
	}
	fractureEventImplInstance.position			= userFractureEvent.position;
	fractureEventImplInstance.chunkIndexInAsset = userFractureEvent.chunkIndex;
	fractureEventImplInstance.impulse			= userFractureEvent.impulse;
	fractureEventImplInstance.destructibleID	= actorAlias.getID();
	fractureEventImplInstance.flags				= userFractureEvent.fractureEventFlags;
	fractureEventImplInstance.hitDirection		= userFractureEvent.direction;
	if(!validUserArguments)
	{
		fractureEventImplInstance.flags			|= FractureEvent::Invalid;
	}
	return fractureEventImplInstance;
}

void SyncParams::interceptEdit(DamageEvent & damageEvent, const DestructibleActorImpl::SyncParams & actorParams) const
{
	PX_ASSERT(NULL != &damageEvent);
	PX_ASSERT(NULL != &actorParams);
	typedef bool DestructibleActorSyncStateLocal;
	const DestructibleActorSyncStateLocal * actorSyncState = NULL;
	PX_UNUSED(actorParams);
	if(NULL != actorSyncState)
	{
		PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadDamageEvents));
		//if(actorSyncState->useChunkDeletionMode)
		{
			PX_ASSERT(0 == (DamageEvent::DeleteChunkModeUnused & damageEvent.flags));
			damageEvent.flags |= DamageEvent::DeleteChunkModeUnused;
		}
	}
}

void SyncParams::interceptEdit(FractureEvent & fractureEvent, const DestructibleActorImpl::SyncParams & actorParams) const
{
	PX_ASSERT(NULL != &fractureEvent);
	PX_ASSERT(NULL != &actorParams);
	typedef bool DestructibleActorSyncStateLocal;
	const DestructibleActorSyncStateLocal * actorSyncState = NULL;
	PX_UNUSED(actorParams);
	if(NULL != actorSyncState)
	{
		PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadFractureEvents));
		//if(actorSyncState->useChunkDeletionMode)
		{
			PX_ASSERT(0 == (FractureEvent::DeleteChunk & fractureEvent.flags));
			fractureEvent.flags |= FractureEvent::DeleteChunk;
		}
	}
}

void SyncParams::onPostProcessReadData(UserDamageEventHandler & callback)
{
	postProcessReadData(callback);
}

void SyncParams::onPostProcessReadData(UserFractureEventHandler & callback)
{
	postProcessReadData(callback);
}

void SyncParams::onPostProcessReadData(UserChunkMotionHandler & callback)
{
	postProcessReadData(callback);
}

void SyncParams::onProcessWriteData(UserDamageEventHandler & callback, const RingBuffer<DamageEvent> & localSource)
{
	PX_ASSERT(NULL != &callback);
	PX_ASSERT(NULL != &localSource);
	const uint32_t unitCount = loadDataForWrite(localSource);
	if(0 != unitCount)
	{
		const uint32_t bufferSize = processWriteBuffer<UserDamageEventHandler, DamageEventHeader, DamageEventUnit>(callback);
		PX_ASSERT(0 != bufferSize);
		if(0 != bufferSize)
		{
			unloadDataForWrite(callback);
		}
	}
}

void SyncParams::onProcessWriteData(UserFractureEventHandler & callback, const RingBuffer<FractureEvent> & localSource)
{
	PX_ASSERT(NULL != &callback);
	PX_ASSERT(NULL != &localSource);
	const uint32_t unitCount = loadDataForWrite(localSource);
	if(0 != unitCount)
	{
		const uint32_t bufferSize = processWriteBuffer<UserFractureEventHandler, FractureEventHeader, FractureEventUnit>(callback);
		PX_ASSERT(0 != bufferSize);
		if(0 != bufferSize)
		{
			unloadDataForWrite(callback);
		}
	}
}

void SyncParams::onProcessWriteData(UserChunkMotionHandler & callback)
{
	PX_ASSERT(NULL != &callback);
	loadDataForWrite(callback);
	const uint32_t bufferSize = processWriteBuffer<UserChunkMotionHandler, ChunkTransformHeader, ChunkTransformUnit>(callback);
	if(0 != bufferSize)
	{
		unloadDataForWrite(callback);
	}
}

void SyncParams::loadDataForRead(const DamageEventHeader * bufferStart, const physx::Array<UserDamageEvent> *& userSource)
{
	PX_ASSERT(damageEventReadSource.empty());
	userSource = NULL;
	for(const DamageEventHeader * currentHeader = bufferStart; NULL != currentHeader; currentHeader = currentHeader->next)
	{
		const uint32_t uniformBufferCount = currentHeader->damageEventCount;
		const DamageEventUnit * uniformBufferStart = currentHeader->damageEventBufferStart;
		PX_ASSERT(!((0 == uniformBufferCount) ^ (NULL == uniformBufferStart)));
		if(0 != uniformBufferCount && NULL != uniformBufferStart)
		{
			const DestructibleActorImpl * currentActor = NULL;
			currentActor = getSyncActor(currentHeader->userActorID);
			oif(NULL != currentActor, SyncParams::errorMissingUserActorID)
			{
				const DestructibleActorImpl::SyncParams & actorParams = currentActor->getSyncParams();
				PX_UNUSED(actorParams);
				oif(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadDamageEvents), SyncParams::errorMissingFlagReadDamageEvents)
				{
					for(uint32_t index = 0; index < uniformBufferCount; ++index)
					{
						damageEventReadSource.pushBack(UserDamageEvent(currentActor, uniformBufferStart + index));
					}
				}
			}
		}
	}
	userSource = &damageEventReadSource;
}

void SyncParams::loadDataForRead(const FractureEventHeader * bufferStart, const physx::Array<UserFractureEvent> *& userSource)
{
	PX_ASSERT(fractureEventReadSource.empty());
	userSource = NULL;
	for(const FractureEventHeader * currentHeader = bufferStart; NULL != currentHeader; currentHeader = currentHeader->next)
	{
		const uint32_t uniformBufferCount = currentHeader->fractureEventCount;
		const FractureEventUnit * uniformBufferStart = currentHeader->fractureEventBufferStart;
		PX_ASSERT(!((0 == uniformBufferCount) ^ (NULL == uniformBufferStart)));
		if(0 != uniformBufferCount && NULL != uniformBufferStart)
		{
			const DestructibleActorImpl * currentActor = NULL;
			currentActor = getSyncActor(currentHeader->userActorID);
			oif(NULL != currentActor, SyncParams::errorMissingUserActorID)
			{
				const DestructibleActorImpl::SyncParams & actorParams = currentActor->getSyncParams();
				PX_UNUSED(actorParams);
				oif(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadFractureEvents), SyncParams::errorMissingFlagReadFractureEvents)
				{
					for(uint32_t index = 0; index < uniformBufferCount; ++index)
					{
						fractureEventReadSource.pushBack(UserFractureEvent(currentActor, uniformBufferStart + index));
					}
				}
			}
		}
	}
	userSource = &fractureEventReadSource;
}

void SyncParams::loadDataForRead(const ChunkTransformHeader * bufferStart)
{
	for(const ChunkTransformHeader * currentHeader = bufferStart; NULL != currentHeader; currentHeader = currentHeader->next)
	{
		const uint32_t uniformBufferCount = currentHeader->chunkTransformCount;
		const ChunkTransformUnit * uniformBufferStart = currentHeader->chunkTransformBufferStart;
		PX_ASSERT(!((0 == uniformBufferCount) ^ (NULL == uniformBufferStart)));
		if(0 != uniformBufferCount && NULL != uniformBufferStart)
		{
			const DestructibleActorImpl * currentActor = NULL;
			currentActor = getSyncActor(currentHeader->userActorID);
			oif(NULL != currentActor, SyncParams::errorMissingUserActorID)
			{
				const DestructibleActorImpl::SyncParams & actorParams = currentActor->getSyncParams();
				PX_UNUSED(actorParams);
				oif(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::ReadChunkTransform), SyncParams::errorMissingFlagReadChunkMotion)
				{
					for(uint32_t index = 0; index < uniformBufferCount; ++index)
					{
						const ChunkTransformUnit * currentControlledChunk = uniformBufferStart + index;
						PX_ASSERT(NULL != currentControlledChunk);
						const uint32_t currentChunkIndex = currentControlledChunk->chunkIndex;
						oif(currentChunkIndex < currentActor->getDestructibleAsset()->getChunkCount(), SyncParams::errorOutOfBoundsActorChunkIndex)
						{
							oif((currentChunkIndex + currentActor->getFirstChunkIndex()) < currentActor->getStructure()->chunks.size(), SyncParams::errorOutOfBoundsStructureChunkIndex)
							{
								DestructibleStructure::Chunk & currentLocalChunk = const_cast<DestructibleStructure*>(currentActor->getStructure())->chunks[currentChunkIndex + currentActor->getFirstChunkIndex()];
								if((0 != (currentLocalChunk.state & ChunkVisible)) && (0 != (currentLocalChunk.state & ChunkDynamic)))
								{
									//PX_ASSERT(NULL == currentLocalChunk.controlledChunk); // could be overwritten since the same index could be in the buffer (due to lag or whatever) in the same cycle. so overwriting with the last is ok. hooray!
									currentLocalChunk.controlledChunk = static_cast<const ControlledChunk*>(currentControlledChunk);
									chunksWithUserControlledChunk.pushBack(&currentLocalChunk);
								}
								else
								{
									//PX_ASSERT(!"we need a way to let the user know that these chunks are abandoned!");
								}
							}
						}
					}
				}
			}
		}
	}
}

bool SyncParams::unloadDataForRead(const UserDamageEventHandler &)
{
	const bool dataExist = !damageEventReadSource.empty();
	if(dataExist)
	{
		damageEventReadSource.clear();
	}
	return dataExist;
}

bool SyncParams::unloadDataForRead(const UserFractureEventHandler &)
{
	const bool dataExist = !fractureEventReadSource.empty();
	if(dataExist)
	{
		fractureEventReadSource.clear();
	}
	return dataExist;
}

bool SyncParams::unloadDataForRead(const UserChunkMotionHandler &)
{
	//unloadDataForRead() is unloaded in actor->setRenderTMs()
	const bool dataExist = !chunksWithUserControlledChunk.empty();
	PX_ASSERT(assertControlledChunkContainerOk());
	chunksWithUserControlledChunk.clear();
	return dataExist;
}

template<typename Callback, typename Header, typename Unit> const Header * SyncParams::processReadBuffer(Callback & callback) const
{
	const Header * bufferStartWritten = NULL;
	PX_ASSERT(NULL != &callback);
	Header * bufferStartRetrieved = NULL;
	uint32_t bufferSizeRetrieved = 0;
	bool continuePointerSwizzling = false;
	int32_t headerCount = 0;
	do 
	{
		callback.onPreProcessReadBegin(bufferStartRetrieved, bufferSizeRetrieved, continuePointerSwizzling);
		if(NULL != bufferStartRetrieved && 0 != bufferSizeRetrieved)
		{
			const int32_t currentHeaderCount = writeUserBufferPointers<Header, Unit>(bufferStartRetrieved, bufferSizeRetrieved);
			PX_ASSERT(currentHeaderCount > 0);
			headerCount += currentHeaderCount;
		}
	}
	while(continuePointerSwizzling);
	callback.onPreProcessReadDone((uint32_t)headerCount);
	bufferStartRetrieved = NULL;
	bufferSizeRetrieved = 0;
	callback.onReadBegin(bufferStartWritten);
	return bufferStartWritten;
}

template<typename Header, typename Unit> int32_t SyncParams::writeUserBufferPointers(Header * bufferStart, const uint32_t bufferSize) const
{
	int32_t headerCount = 0;
	if(NULL != bufferStart && 0 != bufferSize)
	{
		uint32_t sizeProcessed = 0;
		for(Header * currentHeader = bufferStart; NULL != currentHeader; currentHeader = currentHeader->next)
		{
			PX_ASSERT(NULL != currentHeader);
			const uint32_t uniformBufferCount = getUniformBufferCount<Unit>(*currentHeader);
			const uint32_t segmentSize = moduleParams.getSize<Header>() + (uniformBufferCount * moduleParams.getSize<Unit>());
			sizeProcessed += segmentSize;
			PX_ASSERT(sizeProcessed <= bufferSize);
			oif(sizeProcessed <= bufferSize, SyncParams::errorOverrunBuffer)
			{
				getUniformBufferStartMutable<Unit>(*currentHeader) = (0 == uniformBufferCount) ? NULL : static_cast<Unit*>(STEP_BY(currentHeader, moduleParams.getSize<Header>()));
				currentHeader->next = (sizeProcessed == bufferSize) ? NULL : static_cast<Header*>(STEP_BY(currentHeader, segmentSize));
				++headerCount;
			}
		}
		PX_ASSERT(sizeProcessed == bufferSize);
		headerCount = (sizeProcessed == bufferSize) ? headerCount : -1;
	}
	return headerCount;
}

template<typename Callback> void SyncParams::postProcessReadData(Callback & callback)
{
	PX_ASSERT(NULL != &callback);
	const bool dataExist = unloadDataForRead(callback);
	if(dataExist || NULL != destructibleDLL::debugMessage)
	{
		callback.onReadDone(destructibleDLL::debugMessage);
	}
	destructibleDLL::debugMessage = NULL;
}

uint32_t SyncParams::loadDataForWrite(const RingBuffer<DamageEvent> & localSource)
{
	PX_ASSERT(NULL != &localSource);
	PX_ASSERT(NULL == damageEventWriteSource);
	uint32_t unitCount = 0;
	DestructibleActorImpl * currentActor = NULL;
	for(uint32_t index = 0; index < localSource.size(); ++index)
	{
		const DamageEvent & currentDamageEvent = localSource[index];
		currentActor = getSyncActor(getUserActorID(currentDamageEvent.destructibleID));
		if(NULL != currentActor)
		{
			DestructibleActorImpl::SyncParams & actorParams = currentActor->getSyncParamsMutable();
			if(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents))
			{
				if(0 == (DamageEvent::Invalid & currentDamageEvent.flags) && !currentDamageEvent.isFromImpact())
				{
					if(!interceptLoad(currentDamageEvent, *currentActor))
					{
						actorParams.pushDamageBufferIndex(index);
						++unitCount;
					}
				}
			}
		}
	}
	damageEventWriteSource = (0 == unitCount) ? NULL : &localSource;
	return unitCount;
}

uint32_t SyncParams::loadDataForWrite(const RingBuffer<FractureEvent> & localSource)
{
	PX_ASSERT(NULL != &localSource);
	PX_ASSERT(NULL == fractureEventWriteSource);
	uint32_t unitCount = 0;
	DestructibleActorImpl * currentActor = NULL;
	for(uint32_t index = 0; index < localSource.size(); ++index)
	{
		const FractureEvent & currentFractureEvent = localSource[index];
		currentActor = getSyncActor(getUserActorID(currentFractureEvent.destructibleID));
		if(NULL != currentActor)
		{
			DestructibleActorImpl::SyncParams & actorParams = currentActor->getSyncParamsMutable();
			if(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents))
			{
				if(0 == (FractureEvent::Invalid & currentFractureEvent.flags) && (0 == (FractureEvent::DamageFromImpact & currentFractureEvent.flags)))
				{
					if(!interceptLoad(currentFractureEvent, *currentActor))
					{
						actorParams.pushFractureBufferIndex(index);
						++unitCount;
					}
				}
			}
		}
	}
	fractureEventWriteSource = (0 == unitCount) ? NULL : &localSource;
	return unitCount;
}

uint32_t SyncParams::loadDataForWrite(const UserChunkMotionHandler &)
{
	//loadDataForWrite() is loaded in actor->setRenderTMs()
	return 0;
}

void SyncParams::unloadDataForWrite(const UserDamageEventHandler &)
{
	PX_ASSERT(NULL != damageEventWriteSource);
	damageEventWriteSource = NULL;
	const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
	const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
	const uint32_t indexContainerSize = indexContainer.size();
	for(uint32_t index = 0; index < indexContainerSize; ++index)
	{
		DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
		PX_ASSERT(NULL != &currentActor);
		DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParamsMutable();
		actorParams.clear<DamageEventUnit>();
	}
}

void SyncParams::unloadDataForWrite(const UserFractureEventHandler &)
{
	PX_ASSERT(NULL != fractureEventWriteSource);
	fractureEventWriteSource = NULL;
	const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
	const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
	const uint32_t indexContainerSize = indexContainer.size();
	for(uint32_t index = 0; index < indexContainerSize; ++index)
	{
		DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
		PX_ASSERT(NULL != &currentActor);
		DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParamsMutable();
		actorParams.clear<FractureEventUnit>();
	}
}

void SyncParams::unloadDataForWrite(const UserChunkMotionHandler &)
{
	const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
	const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
	const uint32_t indexContainerSize = indexContainer.size();
	for(uint32_t index = 0; index < indexContainerSize; ++index)
	{
		DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
		PX_ASSERT(NULL != &currentActor);
		DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParamsMutable();
		actorParams.clear<ChunkTransformUnit>();
	}
}

template<typename Callback, typename Header, typename Unit> uint32_t SyncParams::processWriteBuffer(Callback & callback)
{
	uint32_t bufferSizeWritten = 0;
	PX_ASSERT(NULL != &callback);
	Header * bufferStartRequested = NULL;
	const uint32_t bufferSizeRequested = getBufferSizeRequired<Header, Unit>();
	if(0 != bufferSizeRequested)
	{
		uint32_t headerCount = 0;
		callback.onWriteBegin(bufferStartRequested, bufferSizeRequested);
		if(NULL != bufferStartRequested)
		{
			bufferSizeWritten = writeUserBuffer<Header, Unit>(bufferStartRequested, headerCount);
			PX_ASSERT(bufferSizeWritten == bufferSizeRequested);
			callback.onWriteDone(headerCount);
		}
	}
	return bufferSizeWritten;
}

template<typename Header, typename Unit> uint32_t SyncParams::getBufferSizeRequired() const
{
	uint32_t bufferSizeRequired = 0;
	PX_ASSERT(segmentSizeChecker.empty());
	const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
	const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
	const uint32_t indexContainerSize = indexContainer.size();
	for(uint32_t index = 0; index < indexContainerSize; ++index)
	{
		const DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
		PX_ASSERT(NULL != &currentActor);
		const DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParams();
		if(0 != actorParams.getCount<Unit>())
		{
			const uint32_t segmentSize = getSegmentSizeRequired<Header, Unit>(actorParams);
			bufferSizeRequired += segmentSize;
			segmentSizeChecker.pushBack(segmentSize);
		}
	}
	return bufferSizeRequired;
}

template<typename Header, typename Unit> uint32_t SyncParams::getSegmentSizeRequired(const DestructibleActorImpl::SyncParams & actorParams) const
{
	uint32_t sizeRequired = 0;
	sizeRequired += actorParams.getCount<Unit>() * moduleParams.getSize<Unit>();
	if(0 != sizeRequired)
	{
		sizeRequired += moduleParams.getSize<Header>();
	}
	return sizeRequired;
}

template<typename Header, typename Unit> uint32_t SyncParams::writeUserBuffer(Header * bufferStart, uint32_t & headerCount)
{
	PX_ASSERT(NULL != bufferStart);
	headerCount = 0;
	uint32_t bufferSizeWritten = 0;
	if(NULL != bufferStart)
	{
		uint32_t segmentSizeCheckerIndex = 0;
		Header * currentSegmentHeader = bufferStart;
		const physx::Array<uint32_t> & indexContainer = syncActorRecord.getIndexContainer();
		const physx::Array<DestructibleActorImpl*> & actorContainer = syncActorRecord.getActorContainer();
		const uint32_t indexContainerSize = indexContainer.size();
		for(uint32_t index = 0; index < indexContainerSize; ++index)
		{
			const DestructibleActorImpl & currentActor = *(actorContainer[indexContainer[index]]);
			PX_ASSERT(NULL != &currentActor);
			const DestructibleActorImpl::SyncParams & actorParams = currentActor.getSyncParams();
			if(0 != actorParams.getCount<Unit>())
			{
				PX_ASSERT(segmentSizeChecker.size() > segmentSizeCheckerIndex);
				const uint32_t segmentSizeWritten = writeUserSegment<Header, Unit>(currentSegmentHeader, (segmentSizeChecker.size() - 1) == segmentSizeCheckerIndex, actorParams);
				PX_ASSERT(0 != segmentSizeWritten);
				bufferSizeWritten += segmentSizeWritten;
				currentSegmentHeader = static_cast<Header*>(STEP_BY(currentSegmentHeader, segmentSizeWritten));
				PX_ASSERT(segmentSizeWritten == segmentSizeChecker[segmentSizeCheckerIndex]);
				++segmentSizeCheckerIndex;
			}
		}
		headerCount = segmentSizeChecker.size();
		segmentSizeChecker.clear();
	}
	return bufferSizeWritten;
}

template<typename Header, typename Unit> uint32_t SyncParams::writeUserSegment(Header * header, bool isLast, const DestructibleActorImpl::SyncParams & actorParams)
{
    //write header
    PX_ASSERT(NULL != header);
	const uint32_t headerSize = moduleParams.getSize<Header>();
    header->userActorID = actorParams.getUserActorID();
    getUniformBufferCountMutable<Unit>(*header) = actorParams.getCount<Unit>();
	const uint32_t uniformBufferCount = getUniformBufferCount<Unit>(*header);
    getUniformBufferStartMutable<Unit>(*header) = (0 == uniformBufferCount) ? NULL : static_cast<Unit*>(STEP_BY(header, headerSize));
    const uint32_t uniformBufferSize = uniformBufferCount * moduleParams.getSize<Unit>();
	header->next = isLast ? NULL : static_cast<Header*>(STEP_BY(header, headerSize + uniformBufferSize));

    //write uniform buffers
    uint32_t sizeWritten = 0;
	Unit * uniformBufferStart = getUniformBufferStart<Unit>(*header);
    sizeWritten += (NULL == uniformBufferStart) ? 0 : writeUserUniformBuffer<Unit>(uniformBufferStart, actorParams);
    PX_ASSERT(0 != sizeWritten);
    sizeWritten += headerSize;
    PX_ASSERT(headerSize != sizeWritten);
    return sizeWritten;
}

template<> uint32_t SyncParams::writeUserUniformBuffer<DamageEventUnit>(DamageEventUnit * bufferStart, const DestructibleActorImpl::SyncParams & actorParams)
{
	PX_ASSERT(NULL != bufferStart);
	PX_ASSERT(NULL != damageEventWriteSource);
	uint32_t sizeWritten = 0;
	const uint32_t count = actorParams.getCount<DamageEventUnit>();
	const physx::Array<uint32_t> & damageBufferIndices = actorParams.getDamageBufferIndices();
	for(uint32_t index = 0 ; index < count; ++index)
	{
		PX_ASSERT(damageEventWriteSource->size() > damageBufferIndices[index]);
		DamageEventUnit * const source = interpret((*damageEventWriteSource)[damageBufferIndices[index]]);
		const bool usingEditFeature = false;
		if(usingEditFeature)
		{
			interceptEdit(*source, actorParams);
		}
		const uint32_t stride = moduleParams.getSize<DamageEventUnit>();
		::memcpy(STEP_BY(bufferStart, stride * index), static_cast<const void*>(source), stride);
		sizeWritten += stride;
	}
	return sizeWritten;
}

template<> uint32_t SyncParams::writeUserUniformBuffer<FractureEventUnit>(FractureEventUnit * bufferStart, const DestructibleActorImpl::SyncParams & actorParams)
{
    PX_ASSERT(NULL != bufferStart);
	PX_ASSERT(NULL != fractureEventWriteSource);
    uint32_t sizeWritten = 0;
	const uint32_t count = actorParams.getCount<FractureEventUnit>();
	const physx::Array<uint32_t> & fractureBufferIndices = actorParams.getFractureBufferIndices();
    for(uint32_t index = 0 ; index < count; ++index)
    {
        PX_ASSERT(fractureEventWriteSource->size() > fractureBufferIndices[index]);
        FractureEventUnit * const source = interpret((*fractureEventWriteSource)[fractureBufferIndices[index]]);
		const bool usingEditFeature = false;
		if(usingEditFeature)
		{
			interceptEdit(*source, actorParams);
		}
        const uint32_t stride = moduleParams.getSize<FractureEventUnit>();
        ::memcpy(STEP_BY(bufferStart, stride * index), static_cast<const void*>(source), stride);
        sizeWritten += stride;
    }
    return sizeWritten;
}

template<> uint32_t SyncParams::writeUserUniformBuffer<ChunkTransformUnit>(ChunkTransformUnit * bufferStart, const DestructibleActorImpl::SyncParams & actorParams)
{
	PX_ASSERT(NULL != bufferStart);
	uint32_t sizeWritten = 0;
	const uint32_t count = actorParams.getCount<ChunkTransformUnit>();
	if(0 != count)
	{
		PX_ASSERT(assertCachedChunkContainerOk(actorParams));
		sizeWritten = count * moduleParams.getSize<ChunkTransformUnit>();
		::memcpy(static_cast<void*>(bufferStart), static_cast<const void*>(&(actorParams.getCachedChunkTransforms()[0])), sizeWritten);
	}
	return sizeWritten;
}

bool SyncParams::interceptLoad(const DamageEvent & damageEvent, const DestructibleActorImpl & syncActor) const
{
	PX_ASSERT(NULL != &damageEvent);
	PX_ASSERT(NULL != &syncActor);
	const DestructibleActorImpl::SyncParams & actorParams = syncActor.getSyncParams();
	PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents));
	const int32_t chunkArrayCount = (int32_t)syncActor.getDestructibleAsset()->getChunkCount();
	PX_UNUSED(chunkArrayCount);
	PX_ASSERT(chunkArrayCount > 0);
	PX_ASSERT(ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(damageEvent.chunkIndexInAsset) ? true : (0 <= damageEvent.chunkIndexInAsset) && (chunkArrayCount > damageEvent.chunkIndexInAsset));
	PX_ASSERT(ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(damageEvent.chunkIndexInAsset) ? 0 != (DamageEvent::UseRadius & damageEvent.flags) : true);
	const uint16_t chunkDepth = uint16_t((ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(damageEvent.chunkIndexInAsset)) ? 0 : static_cast<const DestructibleAssetParameters*>(syncActor.getDestructibleAsset()->getAssetNvParameterized())->chunks.buf[damageEvent.chunkIndexInAsset].depth);
	return
		NULL == actorParams.getActorSyncState() ? false :
		actorParams.getActorSyncState()->damageEventFilterDepth < chunkDepth ? true :
		false;
}

bool SyncParams::interceptLoad(const FractureEvent & fractureEvent, const DestructibleActorImpl & syncActor) const
{
	PX_ASSERT(NULL != &fractureEvent);
	PX_ASSERT(NULL != &syncActor);
	const DestructibleActorImpl::SyncParams & actorParams = syncActor.getSyncParams();
	PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents));
	const uint32_t chunkArrayCount = syncActor.getDestructibleAsset()->getChunkCount();
	PX_UNUSED(chunkArrayCount);
	PX_ASSERT(chunkArrayCount > 0);
	const uint16_t chunkDepth = static_cast<const DestructibleAssetParameters*>(syncActor.getDestructibleAsset()->getAssetNvParameterized())->chunks.buf[fractureEvent.chunkIndexInAsset].depth;
	return
		NULL == actorParams.getActorSyncState() ? false :
		actorParams.getActorSyncState()->fractureEventFilterDepth < chunkDepth ? true :
		false;
}

DamageEventUnit * SyncParams::interpret(const DamageEvent & damageEvent)
{
	PX_ASSERT(NULL != &damageEvent);
	PX_ASSERT(ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(damageEvent.chunkIndexInAsset) ? 0 != (DamageEvent::UseRadius & damageEvent.flags) : true);
	PX_ASSERT(0 == (DamageEvent::Invalid & damageEvent.flags));
	damageEventUserInstance.chunkIndex			= static_cast<uint32_t>(damageEvent.chunkIndexInAsset);
	damageEventUserInstance.damageEventFlags	= damageEvent.flags;
	damageEventUserInstance.damage				= damageEvent.damage;
	damageEventUserInstance.momentum			= damageEvent.momentum;
	damageEventUserInstance.radius				= damageEvent.radius;
	damageEventUserInstance.position			= damageEvent.position;
	damageEventUserInstance.direction			= damageEvent.direction;
	return &damageEventUserInstance;
}

FractureEventUnit * SyncParams::interpret(const FractureEvent & fractureEvent)
{
	PX_ASSERT(NULL != &fractureEvent);
	PX_ASSERT(0 == (FractureEvent::Invalid & fractureEvent.flags));
    fractureEventUserInstance.chunkIndex			= fractureEvent.chunkIndexInAsset;
    fractureEventUserInstance.fractureEventFlags	= fractureEvent.flags;
	fractureEventUserInstance.position				= fractureEvent.position;
    fractureEventUserInstance.direction				= fractureEvent.hitDirection;
	fractureEventUserInstance.impulse				= fractureEvent.impulse;
    return &fractureEventUserInstance;
}

void SyncParams::interceptEdit(DamageEventUnit & nxApexDamageEventUnit, const DestructibleActorImpl::SyncParams & actorParams) const
{
	PX_ASSERT(NULL != &nxApexDamageEventUnit);
	PX_ASSERT(NULL != &actorParams);
	typedef bool DestructibleActorSyncStateSource;
	const DestructibleActorSyncStateSource * actorSyncState = NULL;
	PX_UNUSED(actorParams);
	if(NULL != actorSyncState)
	{
		PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyDamageEvents));
		//if(actorSyncState->useChunkDeletionMode)
		{
			PX_ASSERT(0 == (DamageEvent::DeleteChunkModeUnused & nxApexDamageEventUnit.damageEventFlags));
			nxApexDamageEventUnit.damageEventFlags |= DamageEvent::DeleteChunkModeUnused;
		}
	}
}

void SyncParams::interceptEdit(FractureEventUnit & nxApexFractureEventUnit, const DestructibleActorImpl::SyncParams & actorParams) const
{
	PX_ASSERT(NULL != &nxApexFractureEventUnit);
	PX_ASSERT(NULL != &actorParams);
	typedef bool DestructibleActorSyncStateSource;
	const DestructibleActorSyncStateSource * actorSyncState = NULL;
	PX_UNUSED(actorParams);
	if(NULL != actorSyncState)
	{
		PX_ASSERT(actorParams.isSyncFlagSet(DestructibleActorSyncFlags::CopyFractureEvents));
		//if(actorSyncState->useChunkDeletionMode)
		{
			PX_ASSERT(0 == (FractureEvent::DeleteChunk & nxApexFractureEventUnit.fractureEventFlags));
			nxApexFractureEventUnit.fractureEventFlags |= FractureEvent::DeleteChunk;
		}
	}
}

template<> uint32_t SyncParams::getUniformBufferCount<DamageEventUnit>(const DamageEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.damageEventCount;
}

template<> uint32_t SyncParams::getUniformBufferCount<FractureEventUnit>(const FractureEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.fractureEventCount;
}

template<> uint32_t SyncParams::getUniformBufferCount<ChunkTransformUnit>(const ChunkTransformHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.chunkTransformCount;
}

template<> uint32_t & SyncParams::getUniformBufferCountMutable<DamageEventUnit>(DamageEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.damageEventCount;
}

template<> uint32_t & SyncParams::getUniformBufferCountMutable<FractureEventUnit>(FractureEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.fractureEventCount;
}

template<> uint32_t & SyncParams::getUniformBufferCountMutable<ChunkTransformUnit>(ChunkTransformHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.chunkTransformCount;
}

template<> DamageEventUnit * SyncParams::getUniformBufferStart<DamageEventUnit>(const DamageEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.damageEventBufferStart;
}

template<> FractureEventUnit * SyncParams::getUniformBufferStart<FractureEventUnit>(const FractureEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.fractureEventBufferStart;
}

template<> ChunkTransformUnit * SyncParams::getUniformBufferStart<ChunkTransformUnit>(const ChunkTransformHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.chunkTransformBufferStart;
}

template<> DamageEventUnit *& SyncParams::getUniformBufferStartMutable<DamageEventUnit>(DamageEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.damageEventBufferStart;
}

template<> FractureEventUnit *& SyncParams::getUniformBufferStartMutable<FractureEventUnit>(FractureEventHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.fractureEventBufferStart;
}

template<> ChunkTransformUnit *& SyncParams::getUniformBufferStartMutable<ChunkTransformUnit>(ChunkTransformHeader & header) const
{
	PX_ASSERT(NULL != &header);
	return header.chunkTransformBufferStart;
}

SyncParams::SyncActorRecord::SyncActorRecord()
{
	PX_ASSERT(indexContainer.empty());
	PX_ASSERT(actorContainer.empty());
}

SyncParams::SyncActorRecord::~SyncActorRecord()
{
	PX_ASSERT(assertActorContainerOk());
	actorContainer.clear();
	PX_ASSERT(actorContainer.empty());
	PX_ASSERT(indexContainer.empty());
}

const physx::Array<uint32_t> & SyncParams::SyncActorRecord::getIndexContainer() const
{
	return indexContainer;
}

const physx::Array<DestructibleActorImpl*> & SyncParams::SyncActorRecord::getActorContainer() const
{
	return actorContainer;
}

void SyncParams::SyncActorRecord::onRebuild(const uint32_t & newCount)
{
	PX_ASSERT((1 != newCount) && (actorContainer.size() != newCount));
	//expand
	if(newCount > actorContainer.size())
	{
		if(actorContainer.empty())
		{
			PX_ASSERT(indexContainer.empty());
			actorContainer.resize(newCount, NULL);
		}
		else
		{
			if(indexContainer.empty())
			{
				while(!actorContainer.empty())
				{
					PX_ASSERT(NULL == actorContainer.back());
					actorContainer.popBack();
				}
				actorContainer.resize(newCount, NULL);
			}
			else
			{
				const bool duh = true;
				if(duh)
				{
					while(actorContainer.size() < newCount)
					{
						actorContainer.pushBack(NULL);
					}
				}
				else
				{
					//save
					physx::Array<DestructibleActorImpl*> temp;
					for(physx::Array<uint32_t>::ConstIterator kIter = indexContainer.begin(); kIter != indexContainer.end(); ++kIter)
					{
						PX_ASSERT(actorContainer.size() > *kIter);
						temp.pushBack(actorContainer[*kIter]);
						PX_ASSERT(NULL != temp.back());
					}

					//reset
					actorContainer.clear();
					actorContainer.resize(newCount, NULL);

					//reload
					PX_ASSERT(temp.size() == indexContainer.size());
					physx::Array<uint32_t>::Iterator iter = indexContainer.end();
					while(indexContainer.begin() != iter--)
					{
						PX_ASSERT(actorContainer.size() > *iter);
						actorContainer[*iter] = temp.popBack();
					}
					iter = NULL;
					PX_ASSERT(temp.empty());
				}
			}
		}
		PX_ASSERT(actorContainer.size() > 1);
	}
	//contract
	else
	{
		PX_ASSERT(!"contracting actorContainer is not allowed by design!");
		/*
		contracting is not allowed because of the need to delete entries internally should the removed indexes contain valid entries, without informing the user.
		moreover, this does not improve search performance, since we use indexing to find our entries.
		in fact, we incur a cost for performing the contraction, and removing the entries
		*/
#if 0
		PX_ASSERT(actorContainer.size() > 1);
		//search for affected (non-NULL) entries in the actorContainer OR search for affected indices in the indexContainer
		//reset the entry's (actor's) sync params. inform the user if possible
		//copy the actorContainer's entries that would still be in range, resize the actorContainer to newCount, then copy these entries back in
		//search the indexContainer for these entries, and remove them
#endif
	}
}

bool SyncParams::SyncActorRecord::assertActorContainerOk() const
{
	for(physx::Array<DestructibleActorImpl*>::ConstIterator kIter = actorContainer.begin(); kIter != actorContainer.end(); ++kIter)
	{
		PX_ASSERT(NULL == (*kIter));
	}
	return true;
}

SyncParams::ActorEventPair::ActorEventPair(const DestructibleActorImpl * actorAlias_, const DamageEventUnit * userDamageEvent_)
:actorAlias(actorAlias_)
,userDamageEvent(userDamageEvent_)
{
	PX_ASSERT(NULL != actorAlias);
	PX_ASSERT(NULL != userDamageEvent);
}

SyncParams::ActorEventPair::ActorEventPair(const DestructibleActorImpl * actorAlias_, const FractureEventUnit * userFractureEvent_)
:actorAlias(actorAlias_)
,userFractureEvent(userFractureEvent_)
{
	PX_ASSERT(NULL != actorAlias);
	PX_ASSERT(NULL != userFractureEvent);
}

SyncParams::ActorEventPair::~ActorEventPair()
{
	userFractureEvent = NULL;
	PX_ASSERT(NULL == userDamageEvent);
	actorAlias = NULL;
}

SyncParams::UserDamageEvent::UserDamageEvent(const DestructibleActorImpl * actorAlias_, const DamageEventUnit * userDamageEvent_)
:ActorEventPair(actorAlias_, userDamageEvent_)
{
}

const DamageEventUnit * SyncParams::UserDamageEvent::get() const
{
	return userDamageEvent;
}

SyncParams::UserDamageEvent::~UserDamageEvent()
{
}

SyncParams::UserFractureEvent::UserFractureEvent(const DestructibleActorImpl * actorAlias_, const FractureEventUnit * userFractureEvent_)
:ActorEventPair(actorAlias_, userFractureEvent_)
{
}

const FractureEventUnit * SyncParams::UserFractureEvent::get() const
{
	return userFractureEvent;
}

SyncParams::UserFractureEvent::~UserFractureEvent()
{
}

bool SyncParams::assertUserDamageEventOk(const DamageEvent & damageEvent, const DestructibleScene & scene) const
{
	PX_ASSERT(NULL != &damageEvent);
	PX_ASSERT(NULL != &scene);
	PX_ASSERT(damageEvent.destructibleID < scene.mDestructibles.usedCount());
	const DestructibleActorImpl * actor = NULL;
	actor = scene.mDestructibles.direct(damageEvent.destructibleID);
	PX_ASSERT(NULL != actor);
	const DestructibleStructure * structure = NULL;
	structure = actor->getStructure();
	PX_VERIFY(NULL != structure);
	const bool isRadiusDamage = (ModuleDestructibleConst::INVALID_CHUNK_INDEX == static_cast<int32_t>(damageEvent.chunkIndexInAsset)) && (0 != (DamageEvent::UseRadius & damageEvent.flags));
	PX_UNUSED(isRadiusDamage);
	PX_ASSERT((damageEvent.chunkIndexInAsset >= 0) || isRadiusDamage);
	PX_ASSERT((static_cast<uint32_t>(damageEvent.chunkIndexInAsset) < structure->chunks.size()) || isRadiusDamage);
	return true;
}

bool SyncParams::assertUserFractureEventOk(const FractureEvent & fractureEvent, const DestructibleScene & scene) const
{
	PX_ASSERT(NULL != &fractureEvent);
	PX_ASSERT(NULL != &scene);
	PX_ASSERT(fractureEvent.destructibleID < scene.mDestructibles.usedCount());
	const DestructibleActorImpl * actor = NULL;
	actor = scene.mDestructibles.direct(fractureEvent.destructibleID);
	PX_ASSERT(NULL != actor);
	const DestructibleStructure * structure = NULL;
	structure = actor->getStructure();
	PX_VERIFY(NULL != structure);
	PX_ASSERT(fractureEvent.chunkIndexInAsset < structure->chunks.size());
	return true;
}

bool SyncParams::assertCachedChunkContainerOk(const DestructibleActorImpl::SyncParams & actorParams) const
{
	const uint32_t count = actorParams.getCount<ChunkTransformUnit>();
	const physx::Array<CachedChunk> & cachedChunkTransforms = actorParams.getCachedChunkTransforms();
	for(uint32_t index = 0 ; index < count; ++index)
	{
		PX_ASSERT(moduleParams.getSize<ChunkTransformUnit>() == sizeof(cachedChunkTransforms[index]));
		PX_ASSERT(getSyncActor(actorParams.getUserActorID())->getStructure()->chunks.size() > cachedChunkTransforms[index].chunkIndex);
		PX_UNUSED(cachedChunkTransforms);
	}
	return true;
}

bool SyncParams::assertControlledChunkContainerOk() const
{
	for(physx::Array<DestructibleStructure::Chunk*>::ConstIterator kIter = chunksWithUserControlledChunk.begin(); kIter != chunksWithUserControlledChunk.end(); ++kIter)
	{
		PX_ASSERT(NULL == (*kIter)->controlledChunk);
	}
	return true;
}

const DestructibleScene::SyncParams & DestructibleScene::getSyncParams() const
{
	return mSyncParams;
}

DestructibleScene::SyncParams & DestructibleScene::getSyncParamsMutable()
{
	return const_cast<DestructibleScene::SyncParams&>(getSyncParams());
}

#undef STEP_BY_K
#undef STEP_BY
#undef oif

} // namespace destructible
} // namespace nvidia

#if 0
    PX_ASSERT(physx::string::stricmp(targetShapes[0]->getActor().getClassName(), "PxRigidDynamic") == 0);
    physx::PxRigidDynamic * aliasRigidActor = NULL;
    aliasRigidActor = (static_cast<physx::PxRigidDynamic*>(&targetShapes[0]->getActor()));
    aliasRigidActor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
    aliasRigidActor->moveKinematic(sourceShapeActorGlobalPose);
#endif //lioneltest
