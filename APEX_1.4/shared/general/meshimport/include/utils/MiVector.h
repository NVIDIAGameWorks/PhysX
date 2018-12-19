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



#ifndef MI_FOUNDATION_VECTOR_H
#define MI_FOUNDATION_VECTOR_H

#include "MiPlatformConfig.h"

namespace mimp
{

	/*!
	An array is a sequential container.

	Implementation note
	* entries between 0 and size are valid objects
	* we use inheritance to build this because the array is included inline in a lot
	  of objects and we want the allocator to take no space if it's not stateful, which
	  aggregation doesn't allow. Also, we want the metadata at the front for the inline
	  case where the allocator contains some inline storage space
	*/
	template<class T>
	class vector
	{

	public:

		typedef T*			iterator;
		typedef const T*	const_iterator;


		explicit  vector(const MiEmpty& /*v*/)
		{
			if(mData)
				mCapacity |= MI_SIGN_BITMASK;
		}

		/*!
		Default array constructor. Initialize an empty array
		*/
		MI_INLINE explicit vector(void)
			: mData(0), mSize(0), mCapacity(0) 
		{}

		/*!
		Initialize array with given capacity
		*/
		MI_INLINE explicit vector(MiU32 size, const T& a = T())
		: mData(0), mSize(0), mCapacity(0) 
		{
			resize(size, a);
		}

		// This is necessary else the basic default copy constructor is used in the case of both arrays being of the same template instance
		// The C++ standard clearly states that a template constructor is never a copy constructor [2]. In other words, 
		// the presence of a template constructor does not suppress the implicit declaration of the copy constructor.
		// Also never make a copy constructor explicit, or copy-initialization* will no longer work. This is because
		// 'binding an rvalue to a const reference requires an accessible copy constructor' (http://gcc.gnu.org/bugs/)
		// *http://stackoverflow.com/questions/1051379/is-there-a-difference-in-c-between-copy-initialization-and-assignment-initializ
		MI_INLINE vector(const vector& other)
			{
			copy(other);
			}

		/*!
		Initialize array with given length
		*/
		MI_INLINE explicit vector(const T* first, const T* last)
			: mSize(last<first?0:(MiU32)(last-first)), mCapacity(mSize)
		{
			mData = allocate(mSize);
			copy(mData, mData + mSize, first);
		}

		/*!
		Destructor
		*/
		MI_INLINE ~vector()
		{
			destroy(mData, mData + mSize);

			if(capacity() && !isInUserMemory())
				deallocate(mData);
		}

		/*!
		Assignment operator. Copy content (deep-copy)
		*/
		MI_INLINE vector& operator= (const vector<T>& rhs)
		{
			if(&rhs == this)
				return *this;

			clear();
			reserve(rhs.mSize);
			copy(mData, mData + rhs.mSize, rhs.mData);

			mSize = rhs.mSize;
			return *this;
		}

		/*!
		vector indexing operator.
		\param i
		The index of the element that will be returned.
		\return
		The element i in the array.
		*/
		MI_FORCE_INLINE const T& operator[] (MiU32 i) const 
		{
			MI_ASSERT(i < mSize);
			return mData[i];
		}

		/*!
		vector indexing operator.
		\param i
		The index of the element that will be returned.
		\return
		The element i in the array.
		*/
		MI_FORCE_INLINE T& operator[] (MiU32 i) 
		{
			MI_ASSERT(i < mSize);
			return mData[i];
		}

		/*!
		Returns a pointer to the initial element of the array.
		\return
		a pointer to the initial element of the array.
		*/
		MI_FORCE_INLINE const_iterator begin() const 
		{
			return mData;
		}

		MI_FORCE_INLINE iterator begin()
		{
			return mData;
		}

		/*!
		Returns an iterator beyond the last element of the array. Do not dereference.
		\return
		a pointer to the element beyond the last element of the array.
		*/

		MI_FORCE_INLINE const_iterator end() const 
		{
			return mData+mSize;
		}

		MI_FORCE_INLINE iterator end()
		{
			return mData+mSize;
		}

		/*!
		Returns a reference to the first element of the array. Undefined if the array is empty.
		\return a reference to the first element of the array
		*/

		MI_FORCE_INLINE const T& front() const 
		{
			MI_ASSERT(mSize);
			return mData[0];
		}

		MI_FORCE_INLINE T& front()
		{
			MI_ASSERT(mSize);
			return mData[0];
		}

		/*!
		Returns a reference to the last element of the array. Undefined if the array is empty
		\return a reference to the last element of the array
		*/

		MI_FORCE_INLINE const T& back() const 
		{
			MI_ASSERT(mSize);
			return mData[mSize-1];
		}

		MI_FORCE_INLINE T& back()
		{
			MI_ASSERT(mSize);
			return mData[mSize-1];
		}


		/*!
		Returns the number of entries in the array. This can, and probably will,
		differ from the array capacity.
		\return
		The number of of entries in the array.
		*/
		MI_FORCE_INLINE MiU32 size() const 
		{
			return mSize;
		}

		/*!
		Clears the array.
		*/
		MI_INLINE void clear() 
		{
			destroy(mData, mData + mSize);
			mSize = 0;
		}

		/*!
		Returns whether the array is empty (i.e. whether its size is 0).
		\return
		true if the array is empty
		*/
		MI_FORCE_INLINE bool empty() const
		{
			return mSize==0;
		}

		/*!
		Finds the first occurrence of an element in the array.
		\param a
		The element to find.
		*/


		MI_INLINE iterator find(const T& a)
		{
			MiU32 index;
			for(index=0;index<mSize && mData[index]!=a;index++)
				;
			return mData+index;
		}

		MI_INLINE const_iterator find(const T& a) const
		{
			MiU32 index;
			for(index=0;index<mSize && mData[index]!=a;index++)
				;
			return mData+index;
		}


		/////////////////////////////////////////////////////////////////////////
		/*!
		Adds one element to the end of the array. Operation is O(1).
		\param a
		The element that will be added to this array.
		*/
		/////////////////////////////////////////////////////////////////////////

		MI_FORCE_INLINE T& push_back(const T& a)
		{
			if(capacity()<=mSize) 
				grow(capacityIncrement());

			MI_PLACEMENT_NEW((void*)(mData + mSize),T)(a);

			return mData[mSize++];
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Returns the element at the end of the array. Only legal if the array is non-empty.
		*/
		/////////////////////////////////////////////////////////////////////////
		MI_INLINE T pop_back() 
		{
			MI_ASSERT(mSize);
			T t = mData[mSize-1];
			mData[--mSize].~T();
			return t;
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Construct one element at the end of the array. Operation is O(1).
		*/
		/////////////////////////////////////////////////////////////////////////
		MI_INLINE T& insert()
		{
			if(capacity()<=mSize) 
				grow(capacityIncrement());

			return *(new (mData+mSize++)T);
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Subtracts the element on position i from the array and replace it with
		the last element.
		Operation is O(1)
		\param i
		The position of the element that will be subtracted from this array.
		\return
		The element that was removed.
		*/
		/////////////////////////////////////////////////////////////////////////
		MI_INLINE void replaceWithLast(MiU32 i)
		{
			MI_ASSERT(i<mSize);
			mData[i] = mData[--mSize];
			mData[mSize].~T();
		}

		MI_INLINE void replaceWithLast(iterator i) 
		{
			replaceWithLast(static_cast<MiU32>(i-mData));
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Replaces the first occurrence of the element a with the last element
		Operation is O(n)
		\param i
		The position of the element that will be subtracted from this array.
		\return Returns true if the element has been removed.
		*/
		/////////////////////////////////////////////////////////////////////////

		MI_INLINE bool findAndReplaceWithLast(const T& a)
		{
			MiU32 index = 0;
			while(index<mSize && mData[index]!=a)
				++index;
			if(index == mSize)
				return false;
			replaceWithLast(index);
			return true;
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Subtracts the element on position i from the array. Shift the entire
		array one step.
		Operation is O(n)
		\param i
		The position of the element that will be subtracted from this array.
		*/
		/////////////////////////////////////////////////////////////////////////
		MI_INLINE void remove(MiU32 i)
		{
			MI_ASSERT(i<mSize);
			for(T* it=mData+i; it->~T(), ++i<mSize; ++it)
				new(it)T(mData[i]);

			--mSize;
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Removes a range from the array.  Shifts the array so order is maintained.
		Operation is O(n)
		\param begin
		The starting position of the element that will be subtracted from this array.
		\param end
		The ending position of the elment that will be subtracted from this array.
		*/
		/////////////////////////////////////////////////////////////////////////
		MI_INLINE void removeRange(MiU32 begin,MiU32 count)
		{
			MI_ASSERT(begin<mSize);
			MI_ASSERT( (begin+count) <= mSize );
			for (MiU32 i=0; i<count; i++)
			{
				mData[begin+i].~T(); // call the destructor on the ones being removed first.
			}
			T *dest = &mData[begin]; // location we are copying the tail end objects to
			T *src  = &mData[begin+count]; // start of tail objects
			MiU32 move_count = mSize - (begin+count); // compute remainder that needs to be copied down
			for (MiU32 i=0; i<move_count; i++)
			{
				new ( dest ) T(*src); // copy the old one to the new location
			    src->~T(); // call the destructor on the old location
				dest++;
				src++;
			}
			mSize-=count;
		}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Resize array
		*/
		//////////////////////////////////////////////////////////////////////////
		MI_NOINLINE void resize(const MiU32 size, const T& a = T());

		//////////////////////////////////////////////////////////////////////////
		/*!
		Resize array such that only as much memory is allocated to hold the 
		existing elements
		*/
		//////////////////////////////////////////////////////////////////////////
		MI_INLINE void shrink()
		{
			recreate(mSize);
		}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Deletes all array elements and frees memory.
		*/
		//////////////////////////////////////////////////////////////////////////
		MI_INLINE void reset()
		{
			resize(0);
			shrink();
		}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Ensure that the array has at least size capacity.
		*/
		//////////////////////////////////////////////////////////////////////////
		MI_INLINE void reserve(const MiU32 capacity)
		{
			if(capacity > this->capacity())
				grow(capacity);
		}

		//////////////////////////////////////////////////////////////////////////
		/*!
		Query the capacity(allocated mem) for the array.
		*/
		//////////////////////////////////////////////////////////////////////////
		MI_FORCE_INLINE MiU32 capacity()	const
		{
			return mCapacity & ~MI_SIGN_BITMASK;
		}

	protected:

		MI_NOINLINE void copy(const vector<T>& other)
		{
			if(!other.empty())
			{
				mData = allocate(mSize = mCapacity = other.size());
				copy(mData, mData + mSize, other.begin());
			}
			else
			{
				mData = NULL;
				mSize = 0;
				mCapacity = 0;
			}

			//mData = allocate(other.mSize);
			//mSize = other.mSize;
			//mCapacity = other.mSize;
			//copy(mData, mData + mSize, other.mData);

		}

		MI_INLINE T* allocate(MiU32 size)
		{
			return size ? (T*)MI_ALLOC(sizeof(T) * size) : 0;
		}

		MI_INLINE void deallocate(void* mem)
		{
			MI_FREE(mem);
		}

		static MI_INLINE void create(T* first, T* last, const T& a)
		{
			for(; first<last; ++first)
				::new(first)T(a);
		}

		static MI_INLINE void copy(T* first, T* last, const T* src)
		{
			for(; first<last; ++first, ++src)
				::new (first)T(*src);
		}

		static MI_INLINE void destroy(T* first, T* last)
		{
			for(; first<last; ++first)
				first->~T();
		}

		/*!
		Resizes the available memory for the array.

		\param capacity
		The number of entries that the set should be able to hold.
		*/	
		MI_INLINE void grow(MiU32 capacity) 
		{
			MI_ASSERT(this->capacity() < capacity);
			recreate(capacity);
		}

		/*!
		Creates a new memory block, copies all entries to the new block and destroys old entries.

		\param capacity
		The number of entries that the set should be able to hold.
		*/
		MI_NOINLINE void recreate(MiU32 capacity);

		// The idea here is to prevent accidental brain-damage with push_back or insert. Unfortunately
		// it interacts badly with InlineArrays with smaller inline allocations.
		// TODO(dsequeira): policy template arg, this is exactly what they're for.
		MI_INLINE MiU32 capacityIncrement()	const
		{
			const MiU32 capacity = this->capacity();
			return capacity == 0 ? 1 : capacity * 2;
		}

		// We need one bit to mark arrays that have been deserialized from a user-provided memory block.
		// For alignment & memory saving purpose we store that bit in the rarely used capacity member.
		MI_FORCE_INLINE	MiU32		isInUserMemory()		const
		{
			return mCapacity & MI_SIGN_BITMASK;
		}

	public: // need to be public for serialization

		T*					mData;
		MiU32				mSize;

	protected:

		MiU32				mCapacity;
	};

	template<class T>
	MI_NOINLINE void vector<T>::resize(const MiU32 size, const T& a)
	{
		reserve(size);
		create(mData + mSize, mData + size, a);
		destroy(mData + size, mData + mSize);
		mSize = size;
	}

	template<class T>
	MI_NOINLINE void vector<T>::recreate(MiU32 capacity)
	{
		T* newData = allocate(capacity);
		MI_ASSERT(!capacity || newData && newData != mData);

		copy(newData, newData + mSize, mData);
		destroy(mData, mData + mSize);
		if(!isInUserMemory())
			deallocate(mData);

		mData = newData;
		mCapacity = capacity;
	}

} // namespace mimp

#endif
