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



#ifndef __APEX_CONSTRAINED_DISTRIBUTOR_H__
#define __APEX_CONSTRAINED_DISTRIBUTOR_H__

#include "Apex.h"
#include "PsUserAllocated.h"

namespace nvidia
{
namespace apex
{

template <typename T = uint32_t>
class ApexConstrainedDistributor
{
public:
	ApexConstrainedDistributor()
	{
	}

	PX_INLINE void resize(uint32_t size)
	{
		mConstraintDataArray.resize(size);
	}
	PX_INLINE void setBenefit(uint32_t index, float benefit)
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		mConstraintDataArray[index].benefit = benefit;
	}
	PX_INLINE void setTargetValue(uint32_t index, T targetValue)
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		mConstraintDataArray[index].targetValue = targetValue;
	}
	PX_INLINE T getResultValue(uint32_t index) const
	{
		PX_ASSERT(index < mConstraintDataArray.size());
		return mConstraintDataArray[index].resultValue;
	}

	void solve(T totalValueLimit)
	{
		uint32_t size = mConstraintDataArray.size();
		if (size == 0)
		{
			return;
		}
		if (size == 1)
		{
			ConstraintData& data = mConstraintDataArray.front();
			data.resultValue = PxMin(data.targetValue, totalValueLimit);
			return;
		}

		float totalBenefit = 0;
		T totalValue = 0;
		for (uint32_t i = 0; i < size; i++)
		{
			ConstraintData& data = mConstraintDataArray[i];

			totalBenefit += data.benefit;
			totalValue += data.targetValue;

			data.resultValue = data.targetValue;
		}
		if (totalValue <= totalValueLimit)
		{
			//resultValue was setted in prev. for-scope
			return;
		}

		mConstraintSortPairArray.resize(size);
		for (uint32_t i = 0; i < size; i++)
		{
			ConstraintData& data = mConstraintDataArray[i];

			data.weight = (totalValueLimit * data.benefit / totalBenefit);
			if (data.weight > 0)
			{
				mConstraintSortPairArray[i].key = (data.targetValue / data.weight);
			}
			else
			{
				mConstraintSortPairArray[i].key = FLT_MAX;
				data.resultValue = 0; //reset resultValue
			}
			mConstraintSortPairArray[i].index = i;
		}

		nvidia::sort(mConstraintSortPairArray.begin(), size, ConstraintSortPredicate());

		for (uint32_t k = 0; k < size; k++)
		{
			float firstKey = mConstraintSortPairArray[k].key;
			if (firstKey == FLT_MAX)
			{
				break;
			}
			ConstraintData& firstData = mConstraintDataArray[mConstraintSortPairArray[k].index];

			//special case when k == i
			float sumWeight = firstData.weight;
			T sum = firstData.targetValue;
			for (uint32_t i = k + 1; i < size; i++)
			{
				const ConstraintData& data = mConstraintDataArray[mConstraintSortPairArray[i].index];

				sumWeight += data.weight;
				const T value = static_cast<T>(firstKey * data.weight);
				PX_ASSERT(value <= data.targetValue);
				sum += value;
			}

			if (sum > totalValueLimit)
			{
				for (uint32_t i = k; i < size; i++)
				{
					ConstraintData& data = mConstraintDataArray[mConstraintSortPairArray[i].index];

					const T value = static_cast<T>(totalValueLimit * data.weight / sumWeight);
					PX_ASSERT(value <= data.targetValue);
					data.resultValue = value;
				}
				break;
			}
			//allready here: firstData.resultData = firstData.targetValue
			totalValueLimit -= firstData.targetValue;
		}
	}

private:
	struct ConstraintData
	{
		float	benefit;     //input benefit
		T		targetValue; //input constraint on value
		float	weight;      //temp
		T		resultValue; //output
	};
	struct ConstraintSortPair
	{
		float key;
		uint32_t index;
	};
	class ConstraintSortPredicate
	{
	public:
		PX_INLINE bool operator()(const ConstraintSortPair& a, const ConstraintSortPair& b) const
		{
			return a.key < b.key;
		}
	};

	physx::Array<ConstraintData>		mConstraintDataArray;
	physx::Array<ConstraintSortPair>	mConstraintSortPairArray;
};

}
} // end namespace nvidia::apex

#endif
