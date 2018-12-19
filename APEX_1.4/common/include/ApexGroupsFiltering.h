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



#ifndef __APEX_GROUPS_FILTERING_H__
#define __APEX_GROUPS_FILTERING_H__

#include "ApexDefs.h"

namespace nvidia
{
namespace apex
{

template <typename G>
class ApexGroupsFiltering
{
	typedef void (*FilterOp)(const G& mask0, const G& mask1, G& result);

	static void filterOp_AND(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 & mask1);
	}
	static void filterOp_OR(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 | mask1);
	}
	static void filterOp_XOR(const G& mask0, const G& mask1, G& result)
	{
		result = (mask0 ^ mask1);
	}
	static void filterOp_NAND(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 & mask1);
	}
	static void filterOp_NOR(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 | mask1);
	}
	static void filterOp_NXOR(const G& mask0, const G& mask1, G& result)
	{
		result = ~(mask0 ^ mask1);
	}
	static void filterOp_SWAP_AND(const G& mask0, const G& mask1, G& result)
	{
		result = SWAP_AND(mask0, mask1);
	}

	GroupsFilterOp::Enum	mFilterOp0, mFilterOp1, mFilterOp2;
	bool					mFilterBool;
	G						mFilterConstant0;
	G						mFilterConstant1;

public:
	ApexGroupsFiltering()
	{
		mFilterOp0 = mFilterOp1 = mFilterOp2 = GroupsFilterOp::AND;
		mFilterBool = false;
		setZero(mFilterConstant0);
		setZero(mFilterConstant1);
	}

	bool	setFilterOps(GroupsFilterOp::Enum op0, GroupsFilterOp::Enum op1, GroupsFilterOp::Enum op2)
	{
		if (mFilterOp0 != op0 || mFilterOp1 != op1 || mFilterOp2 != op2)
		{
			mFilterOp0 = op0;
			mFilterOp1 = op1;
			mFilterOp2 = op2;
			return true;
		}
		return false;
	}
	void	getFilterOps(GroupsFilterOp::Enum& op0, GroupsFilterOp::Enum& op1, GroupsFilterOp::Enum& op2) const
	{
		op0 = mFilterOp0;
		op1 = mFilterOp1;
		op2 = mFilterOp2;
	}

	bool	setFilterBool(bool flag)
	{
		if (mFilterBool != flag)
		{
			mFilterBool = flag;
			return true;
		}
		return false;
	}
	bool	getFilterBool() const
	{
		return mFilterBool;
	}

	bool	setFilterConstant0(const G& mask)
	{
		if (mFilterConstant0 != mask)
		{
			mFilterConstant0 = mask;
			return true;
		}
		return false;
	}
	G		getFilterConstant0() const
	{
		return mFilterConstant0;
	}
	bool	setFilterConstant1(const G& mask)
	{
		if (mFilterConstant1 != mask)
		{
			mFilterConstant1 = mask;
			return true;
		}
		return false;
	}
	G		getFilterConstant1() const
	{
		return mFilterConstant1;
	}

	bool	operator()(const G& mask0, const G& mask1) const
	{
		static const FilterOp sFilterOpList[] =
		{
			&filterOp_AND,
			&filterOp_OR,
			&filterOp_XOR,
			&filterOp_NAND,
			&filterOp_NOR,
			&filterOp_NXOR,
			&filterOp_SWAP_AND,
		};

		if (hasBits(mask0) & hasBits(mask1))
		{
			G result0, result1, result;
			sFilterOpList[mFilterOp0](mask0, mFilterConstant0, result0);
			sFilterOpList[mFilterOp1](mask1, mFilterConstant1, result1);
			sFilterOpList[mFilterOp2](result0, result1, result);
			return (hasBits(result) == mFilterBool);
		}
		return true;
	}
};


}
} // end namespace nvidia::apex

#endif
