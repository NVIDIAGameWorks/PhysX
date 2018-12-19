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



#ifndef FILTER_BITS_H

#define FILTER_BITS_H

// This class lets the user define an arbitrary set of 'types' or 'groups' up to 64 different kinds.
// Each 'type' is assigned a bit position up to 64 bits total.
//
// The application can decide whether a null string should be treated as all bits are off, or all bits are on.
//
// This class organizes and builds everything as 64 bits, but has the option of returning only a 32 bit version
// if that better matches the application requirements.
//
// Both the 64 bit and 32 bit representations are const pointers which are guaranteed to be persistent for the lifetime of the
// parent object.  That way you can cache them in your own internal code.
//
// The purpose of this is so that artists/designers can express a set of 'groups/types' which interact with matching group
// types without depending on a hard coded predefined set of options.
//
// Here are some examples:
//
// "player=terrain,building"
//
// In this example the user would be specifying that this is an object of type 'player' which interacts with types 'terrain and building'
//
// The pointer to the filterData bits are persistent and can be cached by the application.
//
// It's important to note that: "a,b,c=d,e,f"  will return the same result as "c,b,a=f,e,d"
//
// The following two strings are reserved: "none" and "all" where 'none' means all bits set to zero, and "all" means all bits set to one.

#include "PxSimpleTypes.h"
#include "ApexUsingNamespace.h"

class FilterData
{
public:
	FilterData(void)
	{
		w64.typeBits64 = 0;
		w64.matchBits64 = 0;
		typeString = 0;
		for (uint32_t i=0; i<64; i++)
		{
			weights[i] = 1.0f;
		}
	}
	union
	{
		struct 
		{
			uint64_t	typeBits64;		// The set of bits which describe what 'type' or 'group' this corresponds with
			uint64_t	matchBits64;	// The set of bits describing which types or groups this object interacts with.
		} w64;
		struct 
		{
			uint32_t	word0;
			uint32_t	word1;
			uint32_t	word2;
			uint32_t	word3;
		} w32;
	};
	float		weights[64];	// filter data weights.
	const char *typeString;	// the source string which produced this set of bits.
};

// The 128 bit encoding of the FilterData class
class EncodedFilterData
{
public:
	union
	{
		struct 
		{
			uint64_t	typeBits64;		// The set of bits which describe what 'type' or 'group' this corresponds with
			uint64_t	matchBits64;	// The set of bits describing which types or groups this object interacts with.
		} ew64;
		struct 
		{
			uint32_t	word0;
			uint32_t	word1;
			uint32_t	word2;
			uint32_t	word3;
		} ew32;
	};
};

class FilterBits
{
public:
	// Returns the ASCII string equivalent of this set of FilterData flags
	virtual const char *getFilterDataString(const FilterData &fd) const = 0;

  	// Returns the combination of string of types based on this bit sequence.
	virtual const char *getFilterString(uint64_t bits) const = 0;

	// Returns the string for a single bit type 0-63
	virtual const char *getTypeString(uint8_t type) const = 0; // single type 0-63

	// Return the bit flag for a single group/type, the parameter 'index' will be assigned with it's array index.  If the type is 'all', then it will be set to 0xFFFFFFFF
	virtual uint64_t getTypeBit(const char *str,uint32_t &index,bool &applyDefault) = 0;

	// Returns how many types were defined.
	virtual uint8_t getTypeCount(void) const = 0;

	// Converts this ASCII string to it's corresponding binary bit representation with up to 64 bits worth of types.
	virtual const FilterData *getFilterData(const char *str) = 0;

	virtual const EncodedFilterData *getEncodedFilterData(const char *str) = 0;

	// Encode it into 128 bits
	virtual const EncodedFilterData &getEncodedFilterData(const FilterData &fd) = 0;

	// If this is properly encoded filter data, then return the original filter-data pointer
	virtual FilterData * getFilterData(const EncodedFilterData &d) const = 0;

	virtual bool isEncodedFilterData(const EncodedFilterData &d) const = 0;

	// see if these two objects interact and, if so, return the weighting value to apply
	virtual bool getWeightedFilter(const EncodedFilterData &o1,const EncodedFilterData &o2,float &weight) = 0;

	virtual void release(FilterData &fb) = 0;

	virtual void release(void) = 0;

protected:
	virtual ~FilterBits(void)
	{
	}
};

FilterBits *createFilterBits(void);

extern FilterBits *gFilterBits;


#endif
