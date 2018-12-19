

#ifndef PX_FOUNDATION_VECTOR_H
#define PX_FOUNDATION_VECTOR_H

#include "PlatformConfigHACD.h"

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

namespace hacd
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


		explicit  vector(const PxEmpty& v)
		{
			HACD_UNUSED(v);
			if(mData)
				mCapacity |= HACD_SIGN_BITMASK;
		}

		/*!
		Default array constructor. Initialize an empty array
		*/
		HACD_INLINE explicit vector(void)
			: mData(0), mSize(0), mCapacity(0) 
		{}

		/*!
		Initialize array with given capacity
		*/
		HACD_INLINE explicit vector(uint32_t size, const T& a = T())
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
		HACD_INLINE vector(const vector& other)
			{
			copy(other);
			}

		/*!
		Initialize array with given length
		*/
		HACD_INLINE explicit vector(const T* first, const T* last)
			: mSize(last<first?0:(uint32_t)(last-first)), mCapacity(mSize)
		{
			mData = allocate(mSize);
			copy(mData, mData + mSize, first);
		}

		/*!
		Destructor
		*/
		HACD_INLINE ~vector()
		{
			destroy(mData, mData + mSize);

			if(capacity() && !isInUserMemory())
				deallocate(mData);
		}

		/*!
		Assignment operator. Copy content (deep-copy)
		*/
		HACD_INLINE vector& operator= (const vector<T>& rhs)
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
		HACD_FORCE_INLINE const T& operator[] (uint32_t i) const 
		{
			HACD_ASSERT(i < mSize);
			return mData[i];
		}

		/*!
		vector indexing operator.
		\param i
		The index of the element that will be returned.
		\return
		The element i in the array.
		*/
		HACD_FORCE_INLINE T& operator[] (uint32_t i) 
		{
			HACD_ASSERT(i < mSize);
			return mData[i];
		}

		/*!
		Returns a pointer to the initial element of the array.
		\return
		a pointer to the initial element of the array.
		*/
		HACD_FORCE_INLINE const_iterator begin() const 
		{
			return mData;
		}

		HACD_FORCE_INLINE iterator begin()
		{
			return mData;
		}

		/*!
		Returns an iterator beyond the last element of the array. Do not dereference.
		\return
		a pointer to the element beyond the last element of the array.
		*/

		HACD_FORCE_INLINE const_iterator end() const 
		{
			return mData+mSize;
		}

		HACD_FORCE_INLINE iterator end()
		{
			return mData+mSize;
		}

		/*!
		Returns a reference to the first element of the array. Undefined if the array is empty.
		\return a reference to the first element of the array
		*/

		HACD_FORCE_INLINE const T& front() const 
		{
			HACD_ASSERT(mSize);
			return mData[0];
		}

		HACD_FORCE_INLINE T& front()
		{
			HACD_ASSERT(mSize);
			return mData[0];
		}

		/*!
		Returns a reference to the last element of the array. Undefined if the array is empty
		\return a reference to the last element of the array
		*/

		HACD_FORCE_INLINE const T& back() const 
		{
			HACD_ASSERT(mSize);
			return mData[mSize-1];
		}

		HACD_FORCE_INLINE T& back()
		{
			HACD_ASSERT(mSize);
			return mData[mSize-1];
		}


		/*!
		Returns the number of entries in the array. This can, and probably will,
		differ from the array capacity.
		\return
		The number of of entries in the array.
		*/
		HACD_FORCE_INLINE uint32_t size() const 
		{
			return mSize;
		}

		/*!
		Clears the array.
		*/
		HACD_INLINE void clear() 
		{
			destroy(mData, mData + mSize);
			mSize = 0;
		}

		/*!
		Returns whether the array is empty (i.e. whether its size is 0).
		\return
		true if the array is empty
		*/
		HACD_FORCE_INLINE bool empty() const
		{
			return mSize==0;
		}

		/*!
		Finds the first occurrence of an element in the array.
		\param a
		The element to find.
		*/


		HACD_INLINE iterator find(const T& a)
		{
			uint32_t index;
			for(index=0;index<mSize && mData[index]!=a;index++)
				;
			return mData+index;
		}

		HACD_INLINE const_iterator find(const T& a) const
		{
			uint32_t index;
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

		HACD_FORCE_INLINE T& push_back(const T& a)
		{
			if(capacity()<=mSize) 
				grow(capacityIncrement());

			HACD_PLACEMENT_NEW((void*)(mData + mSize),T)(a);

			return mData[mSize++];
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Returns the element at the end of the array. Only legal if the array is non-empty.
		*/
		/////////////////////////////////////////////////////////////////////////
		HACD_INLINE T pop_back() 
		{
			HACD_ASSERT(mSize);
			T t = mData[mSize-1];
			mData[--mSize].~T();
			return t;
		}

		/////////////////////////////////////////////////////////////////////////
		/*!
		Construct one element at the end of the array. Operation is O(1).
		*/
		/////////////////////////////////////////////////////////////////////////
		HACD_INLINE T& insert()
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
		HACD_INLINE void replaceWithLast(uint32_t i)
		{
			HACD_ASSERT(i<mSize);
			mData[i] = mData[--mSize];
			mData[mSize].~T();
		}

		HACD_INLINE void replaceWithLast(iterator i) 
		{
			replaceWithLast(static_cast<uint32_t>(i-mData));
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

		HACD_INLINE bool findAndReplaceWithLast(const T& a)
		{
			uint32_t index = 0;
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
		HACD_INLINE void remove(uint32_t i)
		{
			HACD_ASSERT(i<mSize);
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
		HACD_INLINE void removeRange(uint32_t begin,uint32_t count)
		{
			HACD_ASSERT(begin<mSize);
			HACD_ASSERT( (begin+count) <= mSize );
			for (uint32_t i=0; i<count; i++)
			{
				mData[begin+i].~T(); // call the destructor on the ones being removed first.
			}
			T *dest = &mData[begin]; // location we are copying the tail end objects to
			T *src  = &mData[begin+count]; // start of tail objects
			uint32_t move_count = mSize - (begin+count); // compute remainder that needs to be copied down
			for (uint32_t i=0; i<move_count; i++)
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
		HACD_NOINLINE void resize(const uint32_t size, const T& a = T());

		//////////////////////////////////////////////////////////////////////////
		/*!
		Resize array such that only as much memory is allocated to hold the 
		existing elements
		*/
		//////////////////////////////////////////////////////////////////////////
		HACD_INLINE void shrink()
		{
			recreate(mSize);
		}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Deletes all array elements and frees memory.
		*/
		//////////////////////////////////////////////////////////////////////////
		HACD_INLINE void reset()
		{
			resize(0);
			shrink();
		}


		//////////////////////////////////////////////////////////////////////////
		/*!
		Ensure that the array has at least size capacity.
		*/
		//////////////////////////////////////////////////////////////////////////
		HACD_INLINE void reserve(const uint32_t capacity)
		{
			if(capacity > this->capacity())
				grow(capacity);
		}

		//////////////////////////////////////////////////////////////////////////
		/*!
		Query the capacity(allocated mem) for the array.
		*/
		//////////////////////////////////////////////////////////////////////////
		HACD_FORCE_INLINE uint32_t capacity()	const
		{
			return mCapacity & ~HACD_SIGN_BITMASK;
		}

	protected:

		HACD_NOINLINE void copy(const vector<T>& other)
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

		HACD_INLINE T* allocate(uint32_t size)
		{
			return size ? (T*)HACD_ALLOC(sizeof(T) * size) : 0;
		}

		HACD_INLINE void deallocate(void* mem)
		{
			HACD_FREE(mem);
		}

		static HACD_INLINE void create(T* first, T* last, const T& a)
		{
			for(; first<last; ++first)
				::new(first)T(a);
		}

		static HACD_INLINE void copy(T* first, T* last, const T* src)
		{
			for(; first<last; ++first, ++src)
				::new (first)T(*src);
		}

		static HACD_INLINE void destroy(T* first, T* last)
		{
			for(; first<last; ++first)
				first->~T();
		}

		/*!
		Resizes the available memory for the array.

		\param capacity
		The number of entries that the set should be able to hold.
		*/	
		HACD_INLINE void grow(uint32_t capacity) 
		{
			HACD_ASSERT(this->capacity() < capacity);
			recreate(capacity);
		}

		/*!
		Creates a new memory block, copies all entries to the new block and destroys old entries.

		\param capacity
		The number of entries that the set should be able to hold.
		*/
		HACD_NOINLINE void recreate(uint32_t capacity);

		// The idea here is to prevent accidental brain-damage with push_back or insert. Unfortunately
		// it interacts badly with InlineArrays with smaller inline allocations.
		// TODO(dsequeira): policy template arg, this is exactly what they're for.
		HACD_INLINE uint32_t capacityIncrement()	const
		{
			const uint32_t capacity = this->capacity();
			return capacity == 0 ? 1 : capacity * 2;
		}

		// We need one bit to mark arrays that have been deserialized from a user-provided memory block.
		// For alignment & memory saving purpose we store that bit in the rarely used capacity member.
		HACD_FORCE_INLINE	uint32_t		isInUserMemory()		const
		{
			return mCapacity & HACD_SIGN_BITMASK;
		}

	public: // need to be public for serialization

		T*					mData;
		uint32_t				mSize;

	protected:

		uint32_t				mCapacity;
	};

	template<class T>
	HACD_NOINLINE void vector<T>::resize(const uint32_t size, const T& a)
	{
		reserve(size);
		create(mData + mSize, mData + size, a);
		destroy(mData + size, mData + mSize);
		mSize = size;
	}

	template<class T>
	HACD_NOINLINE void vector<T>::recreate(uint32_t capacity)
	{
		T* newData = allocate(capacity);
		HACD_ASSERT(!capacity || newData && newData != mData);

		copy(newData, newData + mSize, mData);
		destroy(mData, mData + mSize);
		if(!isInUserMemory())
			deallocate(mData);

		mData = newData;
		mCapacity = capacity;
	}

} // namespace hacd

#endif
