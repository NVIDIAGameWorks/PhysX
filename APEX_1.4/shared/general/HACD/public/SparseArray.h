
#ifndef SPARSE_ARRAY_H

#define SPARSE_ARRAY_H

#include "PlatformConfigHACD.h"

using namespace hacd;

/*!
**
** Copyright (c) 2015 by John W. Ratcliff mailto:jratcliffscarab@gmail.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


**
** If you find this code snippet useful; you can tip me at this bitcoin address:
**
** BITCOIN TIP JAR: "1BT66EoaGySkbY9J6MugvQRhMMXDwPxPya"
**


*/

// This class implements a sparse array.
// You must know the maximum number of actual elements you will ever have in your array at creation time.
// Meaning it does not support 'growing' the number of active elements.
// The size of the array can be any array indexes up to 32 bits.
//


template < class Value,	size_t hashTableSize = 512 >		// *MUST* be a power of 2!
class SparseArray : public UANS::UserAllocated
{
public:
	SparseArray(HaSizeT maxEntries)
	{
		mFreeList = NULL;
		mHashTableCount = 0;
		memset(mHashTable,0,sizeof(void *)*hashTableSize);
		mMaxEntries = maxEntries;
		mEntries = HACD_NEW(HashEntry)[maxEntries];
	}
	~SparseArray(void)
	{
		delete []mEntries;
	}

	Value& operator[] (HaSizeT key)
	{
		Value *v = find(key);
		if ( v == NULL )
		{
			Value dummy=0;
			insert(key,dummy);
			v = find(key);
			HACD_ASSERT(v);
		}
		return *v;
	}

	const Value& operator[](HaSizeT key) const
	{
		Value *v = find(key);
		if ( v == NULL )
		{
			Value dummy=0;
			insert(key,dummy);
			v = find(key);
			HACD_ASSERT(v);
		}
		return *v;
	}


	Value* find(HaSizeT key)  const
	{
		Value* ret = NULL;
		HaSizeT hash = getHash(key);
		HashEntry* h = mHashTable[hash];
		while (h)
		{
			if (h->mKey == key)
			{
				ret = &h->mValue;
				break;
			}
			h = h->mNext;
		}
		return ret;
	}

	void erase(HaSizeT key)
	{
		HaSizeT hash = getHash(key);
		HashEntry* h = mHashTable[hash];
		HashEntry *prev = NULL;
		while (h)
		{
			if (h->mKey == key)
			{
				if ( prev )
				{
					prev->mNext = h->mNext; // if there was a previous, then it's next is our old next
				}
				else
				{
					mHashTable[hash] = h->mNext; // if there was no previous than the new head of the list is our next.
				}
				// add this hash entry to the free list.
				HashEntry *oldFreeList = mFreeList;
				mFreeList = h;
				h->mNext = oldFreeList;
				break;
			}
			prev = h;
			h = h->mNext;
		}
	}

	void insert(HaSizeT key, const Value& value)
	{
		if (mHashTableCount < mMaxEntries )
		{
			HashEntry* h;
			if ( mFreeList ) // if there are previously freed hash entry items
			{
				h = mFreeList;
				mFreeList = h->mNext;
				h->mNext = NULL;
			}
			else
			{
				h = &mEntries[mHashTableCount];
				mHashTableCount++;
				HACD_ASSERT( mHashTableCount < mMaxEntries );
			}

			h->mKey = key;
			h->mValue = value;
			HaSizeT hash = getHash(key);
			if (mHashTable[hash])
			{
				HashEntry* next = mHashTable[hash];
				mHashTable[hash] = h;
				h->mNext = next;
			}
			else
			{
				mHashTable[hash] = h;
			}
		}
	}
private:

	// Thomas Wang's 32 bit mix
	// http://www.cris.com/~Ttwang/tech/inthash.htm
	HACD_INLINE uint32_t hash(const uint32_t key) const
	{
		uint32_t k = key;
		k += ~(k << 15);
		k ^= (k >> 10);
		k += (k << 3);
		k ^= (k >> 6);
		k += ~(k << 11);
		k ^= (k >> 16);
		return (uint32_t)k;
	}

	HACD_INLINE uint32_t getHash(uint32_t key) const
	{
		uint32_t ret = hash(key);
		return ret & (hashTableSize - 1);
	}

	HACD_INLINE uint64_t getHash(uint64_t key) const
	{
		union {
			uint32_t parts[2];
			uint64_t whole;
		} temp;
		temp.whole = key;
		temp.parts[0] = hash(temp.parts[0]);
		temp.parts[1] = hash(temp.parts[1]);
		return temp.whole & (hashTableSize - 1);
	}

	class HashEntry : public UANS::UserAllocated
	{
	public:
		HashEntry(void)
		{
			mNext = NULL;
		}
		HashEntry*	mNext;
		HaSizeT		mKey;
		Value		mValue;
	};

	HashEntry*		mFreeList;
	HashEntry*		mHashTable[hashTableSize];
	unsigned int	mHashTableCount;
	HaSizeT			mMaxEntries;
	HashEntry		*mEntries;

};


template < class Value,	size_t hashTableSize = 512, size_t maxEntries = 2048 >		// *MUST* be a power of 2!
class SparseArrayFixed : public UANS::UserAllocated
{
public:
	SparseArrayFixed(void)
	{
		mFreeList = NULL;
		mHashTableCount = 0;
		memset(mHashTable,0,sizeof(void *)*hashTableSize);
	}
	~SparseArrayFixed(void)
	{
	}

	Value& operator[] (HaSizeT key)
	{
		Value *v = find(key);
		if ( v == NULL )
		{
			Value dummy=0;
			insert(key,dummy);
			v = find(key);
			HACD_ASSERT(v);
		}
		return *v;
	}

	const Value& operator[](HaSizeT key) const
	{
		Value *v = find(key);
		if ( v == NULL )
		{
			Value dummy=0;
			insert(key,dummy);
			v = find(key);
			HACD_ASSERT(v);
		}
		return *v;
	}


	Value* find(HaSizeT key)  const
	{
		Value* ret = NULL;
		HaSizeT hash = getHash(key);
		HashEntry* h = mHashTable[hash];
		while (h)
		{
			if (h->mKey == key)
			{
				ret = &h->mValue;
				break;
			}
			h = h->mNext;
		}
		return ret;
	}

	void erase(HaSizeT key)
	{
		HaSizeT hash = getHash(key);
		HashEntry* h = mHashTable[hash];
		HashEntry *prev = NULL;
		while (h)
		{
			if (h->mKey == key)
			{
				if ( prev )
				{
					prev->mNext = h->mNext; // if there was a previous, then it's next is our old next
				}
				else
				{
					mHashTable[hash] = h->mNext; // if there was no previous than the new head of the list is our next.
				}
				// add this hash entry to the free list.
				HashEntry *oldFreeList = mFreeList;
				mFreeList = h;
				h->mNext = oldFreeList;
				break;
			}
			prev = h;
			h = h->mNext;
		}
	}

	void insert(HaSizeT key, const Value& value)
	{
		if (mHashTableCount < maxEntries )
		{
			HashEntry* h;
			if ( mFreeList ) // if there are previously freed hash entry items
			{
				h = mFreeList;
				mFreeList = h->mNext;
				h->mNext = NULL;
			}
			else
			{
				h = &mEntries[mHashTableCount];
				mHashTableCount++;
				HACD_ASSERT( mHashTableCount < maxEntries );
			}

			h->mKey = key;
			h->mValue = value;
			HaSizeT hash = getHash(key);
			if (mHashTable[hash])
			{
				HashEntry* next = mHashTable[hash];
				mHashTable[hash] = h;
				h->mNext = next;
			}
			else
			{
				mHashTable[hash] = h;
			}
		}
	}
private:

	// Thomas Wang's 32 bit mix
	// http://www.cris.com/~Ttwang/tech/inthash.htm
	HACD_INLINE uint32_t hash(const uint32_t key) const
	{
		uint32_t k = key;
		k += ~(k << 15);
		k ^= (k >> 10);
		k += (k << 3);
		k ^= (k >> 6);
		k += ~(k << 11);
		k ^= (k >> 16);
		return (uint32_t)k;
	}

	HACD_INLINE uint32_t getHash(uint32_t key) const
	{
		uint32_t ret = hash(key);
		return ret & (hashTableSize - 1);
	}

	HACD_INLINE uint64_t getHash(uint64_t key) const
	{
		union {
			uint32_t parts[2];
			uint64_t whole;
		} temp;
		temp.whole = key;
		temp.parts[0] = hash(temp.parts[0]);
		temp.parts[1] = hash(temp.parts[1]);
		return temp.whole & (hashTableSize - 1);
	}

	class HashEntry : public UANS::UserAllocated
	{
	public:
		HashEntry(void)
		{
			mNext = NULL;
		}
		HashEntry*	mNext;
		HaSizeT		mKey;
		Value		mValue;
	};

	HashEntry*		mFreeList;
	HashEntry*		mHashTable[hashTableSize];
	unsigned int	mHashTableCount;
	HashEntry		mEntries[maxEntries];

};


#endif
