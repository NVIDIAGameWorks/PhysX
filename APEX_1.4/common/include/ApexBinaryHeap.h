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



#ifndef APEX_BINARY_HEAP_H
#define APEX_BINARY_HEAP_H

#include "ApexDefs.h"

namespace nvidia
{
namespace apex
{

template <class Comparable>
class ApexBinaryHeap
{
public:
	explicit ApexBinaryHeap(int capacity = 100) : mCurrentSize(0)
	{
		if (capacity > 0)
		{
			mArray.reserve((uint32_t)capacity + 1);
		}

		mArray.insert();
	}



	bool isEmpty() const
	{
		return mCurrentSize == 0;
	}



	const Comparable& peek() const
	{
		PX_ASSERT(mArray.size() > 1);
		return mArray[1];
	}



	void push(const Comparable& x)
	{
		mArray.insert();
		// Percolate up
		mCurrentSize++;
		uint32_t hole = mCurrentSize;
		while (hole > 1)
		{
			uint32_t parent = hole >> 1;
			if (!(x < mArray[parent]))
			{
				break;
			}
			mArray[hole] = mArray[parent];
			hole = parent;
		}
		mArray[hole] = x;
	}



	void pop()
	{
		if (mArray.size() > 1)
		{
			mArray[1] = mArray[mCurrentSize--];
			percolateDown(1);
			mArray.popBack();
		}
	}



	void pop(Comparable& minItem)
	{
		if (mArray.size() > 1)
		{
			minItem = mArray[1];
			mArray[1] = mArray[mCurrentSize--];
			percolateDown(1);
			mArray.popBack();
		}
	}

private:
	uint32_t mCurrentSize;  // Number of elements in heap
	physx::Array<Comparable> mArray;

	void buildHeap()
	{
		for (uint32_t i = mCurrentSize / 2; i > 0; i--)
		{
			percolateDown(i);
		}
	}



	void percolateDown(uint32_t hole)
	{
		Comparable tmp = mArray[hole];

		while (hole * 2 <= mCurrentSize)
		{
			uint32_t child = hole * 2;
			if (child != mCurrentSize && mArray[child + 1] < mArray[child])
			{
				child++;
			}
			if (mArray[child] < tmp)
			{
				mArray[hole] = mArray[child];
			}
			else
			{
				break;
			}

			hole = child;
		}

		mArray[hole] = tmp;
	}
};

}
} // end namespace nvidia::apex

#endif // APEX_BINARY_HEAP_H
