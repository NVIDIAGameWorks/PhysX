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



#ifndef __APEX_INPLACE_STORAGE_H__
#define __APEX_INPLACE_STORAGE_H__

#include "ApexUsingNamespace.h"
#include "PsAllocator.h"
#include "InplaceTypes.h"

namespace nvidia
{
namespace apex
{


class InplaceStorage;

class InplaceStorageGroup
{
	friend class InplaceStorage;

	InplaceStorage*			_storage;
	uint32_t			_lastBlockIndex;

	InplaceStorageGroup*	_groupListPrev;
	InplaceStorageGroup*	_groupListNext;

	PX_INLINE void reset(InplaceStorage* storage)
	{
		PX_UNUSED(storage);
		PX_ASSERT(_storage == storage);
		_storage = 0;
	}

public:
	PX_INLINE InplaceStorageGroup() : _storage(0) {}
	PX_INLINE InplaceStorageGroup(InplaceStorage& storage) : _storage(0)
	{
		init(storage);
	}
	PX_INLINE ~InplaceStorageGroup()
	{
		release();
	}

	PX_INLINE void init(InplaceStorage& storage);
	PX_INLINE void release();

	PX_INLINE void begin();
	PX_INLINE void end();

	PX_INLINE InplaceStorage& getStorage()
	{
		PX_ASSERT(_storage != 0);
		return *_storage;
	}
};

class InplaceStorage
{
	struct ReflectorArg
	{
	};

	class Reflector
	{
		InplaceStorage* _storage;
		uint32_t _blockIndex;
		uint8_t* _blockPtr;

	public:
		Reflector(InplaceStorage* storage, uint32_t blockIndex, uint8_t* blockPtr)
			: _storage(storage), _blockIndex(blockIndex), _blockPtr(blockPtr)
		{
		}

		template <int _inplace_offset_, typename MT>
		PX_INLINE void processType(ReflectorArg, InplaceHandleBase& handle, MT )
		{
			size_t offset = size_t(reinterpret_cast<uint8_t*>(&handle) - _blockPtr);
			_storage->addHandleRef(_blockIndex, offset, MT::AutoFreeValue);
		}
		template <int _inplace_offset_, typename T, typename MT>
		PX_INLINE void processType(ReflectorArg ra, InplaceHandle<T>& handle, MT mt)
		{
			processType<_inplace_offset_>(ra, static_cast<InplaceHandleBase&>(handle), mt);
		}
		template <int _inplace_offset_, typename T, typename MT>
		PX_INLINE void processType(ReflectorArg, T& , MT )
		{
			; //do nothing
		}

		template <int _inplace_offset_, typename T>
		PX_INLINE void processPrimitiveType(ReflectorArg, T& )
		{
			; //do nothing
		}
	};
	friend class Reflector;

	static const uint32_t NULL_INDEX = InplaceHandleBase::NULL_VALUE;

	struct Block
	{
		uint32_t			_size;
		uint32_t			_alignment;
		uint32_t			_offset;
		uint32_t			_prevIndex;
		union
		{
			uint32_t		_nextIndex;
			uint32_t		_nextFreeBlockIndex;
		};
		uint32_t			_firstRefIndex;
		InplaceStorageGroup*	_group;

		void reset()
		{
			_alignment = 0;
			_size = 0;
			_offset = uint32_t(-1);
			_prevIndex = _nextIndex = NULL_INDEX;
			_firstRefIndex = NULL_INDEX;
			_group = NULL;
		}
	};

	struct HandleRef
	{
		enum Flags
		{
			AUTO_FREE = 0x01,
		};
		uint32_t flags;
		uint32_t ownerBlockIndex;
		uint32_t offsetInBlock;
		union
		{
			uint32_t nextIndex;
			uint32_t nextFreeRefIndex;
		};

		void reset()
		{
			flags = 0;
			ownerBlockIndex = NULL_INDEX;
			offsetInBlock = 0;
		}
	};

	void addHandleRef(uint32_t blockIndex, size_t offset, bool autoFree)
	{
		//find free handleRef
		if (_firstFreeRefIndex == NULL_INDEX)
		{
			_firstFreeRefIndex = _handleRefs.size();
			_handleRefs.resize(_firstFreeRefIndex + 1);

			_handleRefs.back().nextFreeRefIndex = NULL_INDEX;
		}
		uint32_t thisRefIndex = _firstFreeRefIndex;
		HandleRef& handleRef = _handleRefs[thisRefIndex];
		_firstFreeRefIndex = handleRef.nextFreeRefIndex;

		Block& block = _blocks[blockIndex];
		handleRef.nextIndex = block._firstRefIndex;
		block._firstRefIndex = thisRefIndex;

		handleRef.ownerBlockIndex = blockIndex;
		handleRef.offsetInBlock = (uint32_t) offset;
		handleRef.flags = 0;
		if (autoFree)
		{
			handleRef.flags |= HandleRef::AUTO_FREE;
		}
	}

	template <typename F>
	void removeHandleRefs(F func, uint32_t blockIndex, uint32_t minOffset = 0)
	{
		Block& block = _blocks[blockIndex];

		uint32_t prevRefIndex = NULL_INDEX;
		uint32_t currRefIndex = block._firstRefIndex;
		while (currRefIndex != NULL_INDEX)
		{
			HandleRef& handleRef = _handleRefs[currRefIndex];
			PX_ASSERT(handleRef.ownerBlockIndex == blockIndex);

			uint32_t nextRefIndex = handleRef.nextIndex;
			if (handleRef.offsetInBlock >= minOffset)
			{
				//remove
				if (handleRef.flags & HandleRef::AUTO_FREE)
				{
					uint32_t blockOffset = block._offset;
					PX_ASSERT(blockOffset != uint32_t(-1));
					InplaceHandleBase handle = *reinterpret_cast<InplaceHandleBase*>(getBufferPtr() + blockOffset + handleRef.offsetInBlock);

					(this->*func)(block, handle);
				}

				if (prevRefIndex != NULL_INDEX)
				{
					_handleRefs[prevRefIndex].nextIndex = nextRefIndex;
				}
				else
				{
					block._firstRefIndex = nextRefIndex;
				}

				handleRef.nextFreeRefIndex = _firstFreeRefIndex;
				_firstFreeRefIndex = currRefIndex;

				handleRef.reset();
			}
			else
			{
				prevRefIndex = currRefIndex;
			}
			currRefIndex = nextRefIndex;
		}
	}


	PX_INLINE void mapHandle(InplaceHandleBase& handle) const
	{
		if (handle._value != NULL_INDEX)
		{
			handle._value = _blocks[handle._value]._offset;
		}
	}
	PX_INLINE uint8_t* getBufferPtr()
	{
		PX_ASSERT(_bufferPtr != 0);
		_isChanged = true;
		return _bufferPtr;
	}
	PX_INLINE const uint8_t* getBufferPtr() const
	{
		PX_ASSERT(_bufferPtr != 0);
		return _bufferPtr;
	}

	template <typename T>
	PX_INLINE const T* resolveType(InplaceHandleBase handle) const
	{
		if (handle._value != NULL_INDEX)
		{
			const Block& block = _blocks[handle._value];
			PX_ASSERT(block._offset != uint32_t(-1));
			return reinterpret_cast<const T*>(getBufferPtr() + block._offset);
		}
		return 0;
	}
	template <typename T>
	PX_INLINE T* resolveType(InplaceHandleBase handle)
	{
		if (handle._value != NULL_INDEX)
		{
			const Block& block = _blocks[handle._value];
			PX_ASSERT(block._offset != uint32_t(-1));
			return reinterpret_cast<T*>(getBufferPtr() + block._offset);
		}
		return 0;
	}

protected:
	//buffer API
	virtual uint8_t* storageResizeBuffer(uint32_t newSize) = 0;

	virtual void storageLock() {}
	virtual void storageUnlock() {}

public:
	InplaceStorage()
	{
		_bufferPtr = 0;
		_isChanged = false;

		_firstFreeBlockIndex = NULL_INDEX;
		_lastAllocatedBlockIndex = NULL_INDEX;
		_allocatedSize = 0;

		_groupListHead = 0;
		_activeGroup = NULL;

		_firstFreeRefIndex = NULL_INDEX;
	}
	virtual ~InplaceStorage()
	{
		release();
	}

	void release()
	{
		releaseGroups();
	}

	bool isChanged() const
	{
		return _isChanged;
	}
	void setUnchanged()
	{
		_isChanged = false;
	}

	template <typename T>
	PX_INLINE bool fetch(InplaceHandleBase handle, T& out, uint32_t index = 0) const
	{
		const T* ptr = resolveType<T>(handle);
		if (ptr != 0)
		{
			out = ptr[index];
			return true;
		}
		return false;
	}
	template <typename T>
	PX_INLINE bool update(InplaceHandleBase handle, const T& in, uint32_t index = 0)
	{
		T* ptr = resolveType<T>(handle);
		if (ptr != 0)
		{
			ptr[index] = in;
			return true;
		}
		return false;
	}
	template <typename T>
	PX_INLINE bool updateRange(InplaceHandleBase handle, const T* in, uint32_t count, uint32_t start = 0)
	{
		T* ptr = resolveType<T>(handle);
		if (ptr != 0)
		{
			::memcpy(ptr + start, in, sizeof(T) * count);
			return true;
		}
		return false;
	}


	template <typename T>
	PX_INLINE bool alloc(InplaceHandleBase& handle, uint32_t count = 1)
	{
		PX_ASSERT(count > 0);
		handle._value = allocBlock(sizeof(T) * count, __alignof(T));
		if (handle._value != NULL_INDEX)
		{
			reflectElems<T>(handle, count);
			return true;
		}
		return false;
	}

	template <typename T>
	PX_INLINE bool alloc(InplaceHandle<T>& handle, uint32_t count = 1)
	{
		return alloc<T>(static_cast<InplaceHandleBase&>(handle), count);
	}

	PX_INLINE void free(InplaceHandleBase& handle)
	{
		if (handle._value != NULL_INDEX)
		{
			freeBlock(handle._value);
			handle._value = NULL_INDEX;
		}
	}

	template <typename T>
	bool realloc(InplaceHandleBase& handle, uint32_t oldCount, uint32_t newCount)
	{
		if (handle._value != NULL_INDEX)
		{
			PX_ASSERT(oldCount > 0);
			if (oldCount != newCount)
			{
				if (newCount > 0)
				{
					if (resizeBlock(handle._value, sizeof(T) * newCount))
					{
						if (newCount > oldCount)
						{
							reflectElems<T>(handle, newCount, oldCount);
						}
						return true;
					}
					return false;
				}
				free(handle);
			}
		}
		else
		{
			PX_ASSERT(oldCount == 0);
			if (newCount > 0)
			{
				return (alloc<T>(handle, newCount) != 0);
			}
		}
		return true;
	}


	template <typename T>
	PX_INLINE InplaceHandle<T> mappedHandle(InplaceHandle<T> handle) const
	{
		mapHandle(handle);
		return handle;
	}

	uint32_t mapTo(uint8_t* destPtr) const
	{
		PX_ASSERT(_lastAllocatedBlockIndex == NULL_INDEX || _blocks[_lastAllocatedBlockIndex]._offset + _blocks[_lastAllocatedBlockIndex]._size == _allocatedSize);

		memcpy(destPtr, getBufferPtr(), _allocatedSize);

		//iterate all blocks
		for (uint32_t blockIndex = _lastAllocatedBlockIndex; blockIndex != NULL_INDEX; blockIndex = _blocks[blockIndex]._prevIndex)
		{
			const Block& block = _blocks[blockIndex];
			//iterate all refs in current block
			for (uint32_t refIndex = block._firstRefIndex; refIndex != NULL_INDEX; refIndex = _handleRefs[refIndex].nextIndex)
			{
				const HandleRef& handleRef = _handleRefs[refIndex];
				PX_ASSERT(handleRef.ownerBlockIndex == blockIndex);

				uint32_t blockOffset = block._offset;
				PX_ASSERT(blockOffset != uint32_t(-1));
				InplaceHandleBase& handle = *reinterpret_cast<InplaceHandleBase*>(destPtr + blockOffset + handleRef.offsetInBlock);

				mapHandle(handle);
			}
		}
		return _allocatedSize;
	}

	uint32_t getAllocatedSize() const
	{
		return _allocatedSize;
	}

private:
	template <typename T>
	T* reflectElems(InplaceHandleBase handle, uint32_t newCount, uint32_t oldCount = 0)
	{
		const Block& block = _blocks[handle._value];

		uint8_t* ptr = (getBufferPtr() + block._offset);
		T* ptrT0 = reinterpret_cast<T*>(ptr);
		T* ptrT = ptrT0 + oldCount;
		Reflector r(this, handle._value, ptr);
		for (uint32_t index = oldCount; index < newCount; ++index)
		{
			::new(ptrT) T;
			InplaceTypeHelper::reflectType<0>(r, ReflectorArg(), *ptrT, InplaceTypeMemberDefaultTraits());
			++ptrT;
		}
		return ptrT0;
	}

	static PX_INLINE uint32_t alignUp(uint32_t size, uint32_t alignment)
	{
		PX_ASSERT(alignment > 0);
		return (size + (alignment - 1)) & ~(alignment - 1);
	}

	PX_INLINE int32_t getMoveDelta(uint32_t blockIndex, uint32_t moveOffset) const
	{
		PX_ASSERT(blockIndex != NULL_INDEX);
		const uint32_t currOffset = _blocks[blockIndex]._offset;

		//calculate max alignment for all subsequent blocks
		uint32_t alignment = 0;
		do
		{
			const Block& block = _blocks[blockIndex];
			alignment = PxMax(alignment, block._alignment);

			blockIndex = block._nextIndex;
		}
		while (blockIndex != NULL_INDEX);

		int32_t moveDelta = (int32_t)moveOffset - (int32_t)currOffset;
		//align moveDelta
		if (moveDelta >= 0)
		{
			moveDelta += (alignment - 1);
			moveDelta &= ~(alignment - 1);
		}
		else
		{
			moveDelta = -moveDelta;
			moveDelta &= ~(alignment - 1);
			moveDelta = -moveDelta;
		}
		PX_ASSERT(currOffset + moveDelta >= moveOffset);
		return moveDelta;
	}

	uint32_t getPrevAllocatedSize(uint32_t prevBlockIndex) const
	{
		uint32_t prevAllocatedSize = 0;
		if (prevBlockIndex != NULL_INDEX)
		{
			const Block& prevBlock = _blocks[prevBlockIndex];
			prevAllocatedSize = prevBlock._offset + prevBlock._size;
		}
		return prevAllocatedSize;
	}

	void moveBlocks(uint32_t moveBlockIndex, int32_t moveDelta)
	{
		if (moveDelta != 0)
		{
			const uint32_t currOffset = _blocks[moveBlockIndex]._offset;
			const uint32_t moveOffset = currOffset + moveDelta;

			uint32_t moveSize = _allocatedSize - currOffset;
			uint8_t* moveFromPtr = getBufferPtr() + currOffset;
			uint8_t* moveToPtr = getBufferPtr() + moveOffset;
			memmove(moveToPtr, moveFromPtr, moveSize);

			_allocatedSize += moveDelta;
			//update moved blocks
			do
			{
				Block& moveBlock = _blocks[moveBlockIndex];
				moveBlock._offset += moveDelta;
				PX_ASSERT((moveBlock._offset & (moveBlock._alignment - 1)) == 0);

				moveBlockIndex = moveBlock._nextIndex;
			}
			while (moveBlockIndex != NULL_INDEX);
		}
	}

	void removeBlocks(uint32_t prevBlockIndex, uint32_t nextBlockIndex, uint32_t lastBlockIndex)
	{
		PX_UNUSED(lastBlockIndex);

		uint32_t prevAllocatedSize = getPrevAllocatedSize(prevBlockIndex);
		if (prevBlockIndex != NULL_INDEX)
		{
			_blocks[prevBlockIndex]._nextIndex = nextBlockIndex;
		}
		if (nextBlockIndex != NULL_INDEX)
		{
			_blocks[nextBlockIndex]._prevIndex = prevBlockIndex;

			const int32_t moveDelta = getMoveDelta(nextBlockIndex, prevAllocatedSize);
			moveBlocks(nextBlockIndex, moveDelta);
		}
		else
		{
			//last block
			PX_ASSERT(lastBlockIndex == _lastAllocatedBlockIndex);
			_lastAllocatedBlockIndex = prevBlockIndex;

			_allocatedSize = prevAllocatedSize;
		}
		shrinkBuffer();
	}

	PX_INLINE bool growBuffer(uint32_t newAllocatedSize)
	{
		PX_ASSERT(newAllocatedSize >= _allocatedSize);

		uint8_t* newBufferPtr = storageResizeBuffer(newAllocatedSize);
		if (newBufferPtr == 0)
		{
			PX_ASSERT(0 && "Out of memory!");
			return false;
		}
		_bufferPtr = newBufferPtr;
		return true;
	}

	PX_INLINE void shrinkBuffer()
	{
		uint8_t* newBufferPtr = storageResizeBuffer(_allocatedSize);
		PX_ASSERT(newBufferPtr != 0);
		_bufferPtr = newBufferPtr;
	}

	uint32_t allocBlock(uint32_t size, uint32_t alignment)
	{
		uint32_t insertBlockIndex;
		uint32_t offset;
		int32_t moveDelta;
		uint32_t newAllocatedSize;

		PX_ASSERT(_activeGroup != NULL);
		if (_activeGroup->_lastBlockIndex == NULL_INDEX || _activeGroup->_lastBlockIndex == _lastAllocatedBlockIndex)
		{
			//push_back new block
			insertBlockIndex = NULL_INDEX;
			offset = alignUp(_allocatedSize, alignment);
			moveDelta = 0;
			newAllocatedSize = offset + size;
		}
		else
		{
			//insert new block
			insertBlockIndex = _blocks[_activeGroup->_lastBlockIndex]._nextIndex;
			PX_ASSERT(insertBlockIndex != NULL_INDEX);

			uint32_t prevAllocatedSize = getPrevAllocatedSize(_blocks[insertBlockIndex]._prevIndex);
			offset = alignUp(prevAllocatedSize, alignment);
			const uint32_t moveOffset = offset + size;
			moveDelta = getMoveDelta(insertBlockIndex, moveOffset);
			newAllocatedSize = _allocatedSize + moveDelta;
		}

		if (growBuffer(newAllocatedSize) == false)
		{
			return NULL_INDEX;
		}

		//find free block
		if (_firstFreeBlockIndex == NULL_INDEX)
		{
			_firstFreeBlockIndex = _blocks.size();
			_blocks.resize(_firstFreeBlockIndex + 1);

			_blocks.back()._nextFreeBlockIndex = NULL_INDEX;
		}
		uint32_t blockIndex = _firstFreeBlockIndex;
		Block& block = _blocks[blockIndex];
		_firstFreeBlockIndex = block._nextFreeBlockIndex;

		//init block
		block._size = size;
		block._alignment = alignment;
		block._offset = offset;
		block._firstRefIndex = NULL_INDEX;
		block._group = _activeGroup;

		PX_ASSERT((block._offset & (block._alignment - 1)) == 0);

		if (insertBlockIndex == NULL_INDEX)
		{
			//add new block after the _lastAllocatedBlockIndex
			block._prevIndex = _lastAllocatedBlockIndex;
			block._nextIndex = NULL_INDEX;

			if (_lastAllocatedBlockIndex != NULL_INDEX)
			{
				PX_ASSERT(_blocks[_lastAllocatedBlockIndex]._nextIndex == NULL_INDEX);
				_blocks[_lastAllocatedBlockIndex]._nextIndex = blockIndex;
			}
			_lastAllocatedBlockIndex = blockIndex;
		}
		else
		{
			PX_ASSERT(_activeGroup->_lastBlockIndex != NULL_INDEX);
			//insert new block before the insertBlockIndex
			block._prevIndex = _activeGroup->_lastBlockIndex;
			_blocks[_activeGroup->_lastBlockIndex]._nextIndex = blockIndex;

			block._nextIndex = insertBlockIndex;
			_blocks[insertBlockIndex]._prevIndex = blockIndex;

			moveBlocks(insertBlockIndex, moveDelta);
			PX_ASSERT(_allocatedSize == newAllocatedSize);
		}
		_allocatedSize = newAllocatedSize;

		//update group
		_activeGroup->_lastBlockIndex = blockIndex;

		return blockIndex;
	}


	PX_INLINE void onRemoveHandle(const Block& block, InplaceHandleBase handle)
	{
		PX_UNUSED(block);
		if (handle._value != InplaceHandleBase::NULL_VALUE)
		{
			PX_ASSERT(handle._value < _blocks.size());
			PX_ASSERT(_blocks[handle._value]._group == block._group);

			freeBlock(handle._value);
		}
	}
	PX_INLINE void onRemoveHandleEmpty(const Block& , InplaceHandleBase )
	{
	}

	void freeBlock(uint32_t blockIndex)
	{
		PX_ASSERT(blockIndex != NULL_INDEX);
		PX_ASSERT(_activeGroup != NULL);
		PX_ASSERT(_blocks[blockIndex]._group == _activeGroup);

		removeHandleRefs(&InplaceStorage::onRemoveHandle, blockIndex);

		Block& block = _blocks[blockIndex];

		removeBlocks(block._prevIndex, block._nextIndex, blockIndex);

		//update group
		if (_activeGroup->_lastBlockIndex == blockIndex)
		{
			_activeGroup->_lastBlockIndex =
			    (block._prevIndex != NULL_INDEX) &&
			    (_blocks[block._prevIndex]._group == _activeGroup) ? block._prevIndex : NULL_INDEX;
		}

		block.reset();
		//add block to free list
		block._nextFreeBlockIndex = _firstFreeBlockIndex;
		_firstFreeBlockIndex = blockIndex;
	}

	bool resizeBlock(uint32_t blockIndex, uint32_t newSize)
	{
		if (newSize < _blocks[blockIndex]._size)
		{
			//remove refs
			removeHandleRefs(&InplaceStorage::onRemoveHandle, blockIndex, newSize);
		}

		Block& block = _blocks[blockIndex];
		const uint32_t nextBlockIndex = block._nextIndex;

		uint32_t newAllocatedSize = block._offset + newSize;
		int32_t moveDelta = 0;
		if (nextBlockIndex != NULL_INDEX)
		{
			moveDelta = getMoveDelta(nextBlockIndex, newAllocatedSize);
			newAllocatedSize = _allocatedSize + moveDelta;
		}

		const bool bGrow = (newAllocatedSize > _allocatedSize);
		if (bGrow)
		{
			if (growBuffer(newAllocatedSize) == false)
			{
				return false;
			}
		}

		block._size = newSize;

		if (nextBlockIndex != NULL_INDEX)
		{
			moveBlocks(nextBlockIndex, moveDelta);
			PX_ASSERT(_allocatedSize == newAllocatedSize);
		}
		_allocatedSize = newAllocatedSize;
		if (!bGrow)
		{
			shrinkBuffer();
		}
		return true;
	}

	void groupInit(InplaceStorageGroup* group)
	{
		storageLock();

		//init new group
		group->_lastBlockIndex = NULL_INDEX;
		group->_groupListPrev = 0;
		group->_groupListNext = _groupListHead;
		if (_groupListHead != NULL)
		{
			_groupListHead->_groupListPrev = group;
		}
		_groupListHead = group;

		storageUnlock();
	}

	void groupFree(InplaceStorageGroup* group)
	{
		storageLock();

		if (group->_lastBlockIndex != NULL_INDEX)
		{
			uint32_t prevBlockIndex = group->_lastBlockIndex;
			uint32_t nextBlockIndex = _blocks[group->_lastBlockIndex]._nextIndex;
			do
			{
				uint32_t freeBlockIndex = prevBlockIndex;
				Block& freeBlock = _blocks[freeBlockIndex];
				prevBlockIndex = freeBlock._prevIndex;

				removeHandleRefs(&InplaceStorage::onRemoveHandleEmpty, freeBlockIndex);

				freeBlock.reset();
				//add block to free list
				freeBlock._nextFreeBlockIndex = _firstFreeBlockIndex;
				_firstFreeBlockIndex = freeBlockIndex;
			}
			while (prevBlockIndex != NULL_INDEX && _blocks[prevBlockIndex]._group == group);

			PX_ASSERT(prevBlockIndex == NULL_INDEX || _blocks[prevBlockIndex]._group != group);
			PX_ASSERT(nextBlockIndex == NULL_INDEX || _blocks[nextBlockIndex]._group != group);

			removeBlocks(prevBlockIndex, nextBlockIndex, group->_lastBlockIndex);
		}

		//remove from GroupList
		if (group->_groupListNext != 0)
		{
			group->_groupListNext->_groupListPrev = group->_groupListPrev;
		}
		if (group->_groupListPrev != 0)
		{
			group->_groupListPrev->_groupListNext = group->_groupListNext;
		}
		else
		{
			PX_ASSERT(_groupListHead == group);
			_groupListHead = group->_groupListNext;
		}

		storageUnlock();
	}

	void groupBegin(InplaceStorageGroup* group)
	{
		storageLock();

		PX_ASSERT(_activeGroup == NULL);
		_activeGroup = group;
	}

	void groupEnd(InplaceStorageGroup* group)
	{
		PX_UNUSED(group);
		PX_ASSERT(group == _activeGroup);
		_activeGroup = NULL;

		storageUnlock();
	}

	void releaseGroups()
	{
		storageLock();

		while (_groupListHead != 0)
		{
			InplaceStorageGroup* group = _groupListHead;
			_groupListHead = _groupListHead->_groupListNext;

			group->reset(this);
		}

		storageUnlock();
	}

	uint8_t*                _bufferPtr;
	bool                        _isChanged;

	uint32_t                _allocatedSize;

	uint32_t                _firstFreeBlockIndex;
	uint32_t                _lastAllocatedBlockIndex;
	physx::Array<Block>         _blocks;

	uint32_t                _firstFreeRefIndex;
	physx::Array<HandleRef>     _handleRefs;

	InplaceStorageGroup*        _groupListHead;
	InplaceStorageGroup*        _activeGroup;

	friend class InplaceStorageGroup;
};

PX_INLINE void InplaceStorageGroup::init(InplaceStorage& storage)
{
	PX_ASSERT(_storage == 0);
	_storage = &storage;
	getStorage().groupInit(this);
}
PX_INLINE void InplaceStorageGroup::release()
{
	if (_storage != 0)
	{
		getStorage().groupFree(this);
		_storage = 0;
	}
}
PX_INLINE void InplaceStorageGroup::begin()
{
	getStorage().groupBegin(this);
}
PX_INLINE void InplaceStorageGroup::end()
{
	getStorage().groupEnd(this);
}

class InplaceStorageGroupScope
{
private:
	InplaceStorageGroupScope& operator=(const InplaceStorageGroupScope&);
	InplaceStorageGroup& _group;

public:
	InplaceStorageGroupScope(InplaceStorageGroup& group) : _group(group)
	{
		_group.begin();
	}
	~InplaceStorageGroupScope()
	{
		_group.end();
	}
};

#define INPLACE_STORAGE_GROUP_SCOPE(group) InplaceStorageGroupScope scopeAccess_##group ( group ); InplaceStorage& _storage_ = group.getStorage();

////
class ApexCpuInplaceStorage : public InplaceStorage
{
public:
	ApexCpuInplaceStorage(uint32_t allocStep = 4096)
		: mAllocStep(allocStep)
	{
		mSize = 0;
		mStoragePtr = 0;
	}
	~ApexCpuInplaceStorage()
	{
		release();
	}

	void release()
	{
		if (mStoragePtr)
		{
			PX_FREE(mStoragePtr);
			mSize = 0;
			mStoragePtr = 0;
		}
	}

protected:
	//interface for InplaceStorage
	uint8_t* storageResizeBuffer(uint32_t newSize)
	{
		if (newSize > mSize)
		{
			newSize = ((newSize + mAllocStep - 1) / mAllocStep) * mAllocStep;
			PX_ASSERT(newSize > mSize && (newSize % mAllocStep) == 0);
			uint8_t* newStoragePtr = (uint8_t*)PX_ALLOC(newSize, PX_DEBUG_EXP("ApexCpuInplaceStorage"));
			if (!newStoragePtr)
			{
				return 0;
			}
			memcpy(newStoragePtr, mStoragePtr, mSize);
			PX_FREE(mStoragePtr);
			mSize = newSize;
			mStoragePtr = newStoragePtr;
		}
		return mStoragePtr;
	}

private:
	uint32_t	mAllocStep;
	uint32_t	mSize;
	uint8_t*	mStoragePtr;
};



}
} // end namespace nvidia::apex

#endif // __APEX_INPLACE_STORAGE_H__
