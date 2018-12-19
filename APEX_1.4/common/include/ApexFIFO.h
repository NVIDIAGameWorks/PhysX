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



#ifndef __APEX_FIFO_H__
#define __APEX_FIFO_H__

#include "Apex.h"
#include "PsUserAllocated.h"

namespace nvidia
{
namespace apex
{

template <typename T>
struct FIFOEntry
{
	T data;
	uint32_t next;
	bool isValidEntry;
};

template<typename T>
class ApexFIFO : public UserAllocated
{
public:
	ApexFIFO() : first((uint32_t) - 1), last((uint32_t) - 1), count(0) {}

	bool popFront(T& frontElement)
	{
		if (first == (uint32_t)-1)
		{
			return false;
		}

		PX_ASSERT(first < list.size());
		frontElement = list[first].data;

		if (first == last)
		{
			list.clear();
			first = (uint32_t) - 1;
			last = (uint32_t) - 1;
		}
		else
		{
			list[first].isValidEntry = false;

			if (list[last].next == (uint32_t)-1)
			{
				list[last].next = first;
			}
			first = list[first].next;
		}

		count--;
		return true;
	}


	void pushBack(const T& newElement)
	{
		if (list.size() == 0 || list[last].next == (uint32_t)-1)
		{
			FIFOEntry<T> newEntry;
			newEntry.data = newElement;
			newEntry.next = (uint32_t) - 1;
			newEntry.isValidEntry = true;
			list.pushBack(newEntry);

			if (first == (uint32_t) - 1)
			{
				PX_ASSERT(last == (uint32_t) - 1);
				first = list.size() - 1;
			}
			else
			{
				PX_ASSERT(last != (uint32_t) - 1);
				list[last].next = list.size() - 1;
			}

			last = list.size() - 1;
		}
		else
		{
			uint32_t freeIndex = list[last].next;
			PX_ASSERT(freeIndex < list.size());

			FIFOEntry<T>& freeEntry = list[freeIndex];
			freeEntry.data = newElement;
			freeEntry.isValidEntry = true;

			if (freeEntry.next == first)
			{
				freeEntry.next = (uint32_t) - 1;
			}

			last = freeIndex;
		}
		count++;
	}

	uint32_t size()
	{
		return count;
	}

	PX_INLINE void reserve(const uint32_t capacity)
	{
		list.reserve(capacity);
	}

	PX_INLINE uint32_t capacity() const
	{
		return list.capacity();
	}

private:
	uint32_t first;
	uint32_t last;
	uint32_t count;
	physx::Array<FIFOEntry<T> > list;
};

}
} // end namespace nvidia::apex

#endif