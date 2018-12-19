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



#include "FilterBits.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#pragma warning(disable:4100 4996)

#define MAX_FILTER_DATA_SPECS 512 // maximum number of unique filter data specs ever expected, 512 is a pretty huge number; bump it if necessary.
#define MAX_TYPES 64	// maximum number of unique bit 'types'
#define MAX_TEMP_STRING 1024 // maximum size of the temporary string to return intermediate ASCII data

FilterBits *gFilterBits=NULL;

namespace FILTER_BITS
{

#define MAGIC_ID1 0x57454947
#define MAGIC_ID2 0x48544544

class FilterDataSpec : public FilterData, public EncodedFilterData
{
public:
	FilterDataSpec(void)
	{
		mSpec = NULL;
	}
	~FilterDataSpec(void)
	{
		free(mSpec);
	}

	void buildEncode(void)
	{
		ew32.word0 = MAGIC_ID1;
		ew32.word1 = MAGIC_ID2;
#if PX_X64
		ew64.matchBits64 = (uint64_t)this;
#else
		ew32.word2 = (uint32_t)this;
		ew32.word3 = 0;
#endif
	}

	float computeWeight(uint64_t mask,float weightIn) const
	{
		float w = weightIn;
		uint32_t index=0;
		while ( mask )
		{
			if ( mask &1 )
			{
				w*=weights[index];
			}
			mask = mask>>1;
			index++;
		}
		return w;
	}

	bool allWeightOne(void) const // return true if the weights of all mask bits are one..
	{
		bool ret = true;

		uint32_t index=0;
		uint64_t mask = ew64.matchBits64;
		while ( mask )
		{
			if ( mask &1 )
			{
				if ( weights[index] != 1 )
				{
					ret = false;
					break;
				}
			}
			mask = mask>>1;
			index++;
		}
		return ret;
	}

	char		*mSpec;
};

class FilterBitsImpl : public FilterBits
{
public:

	FilterBitsImpl(void)
	{
		gFilterBits = this;
		mFilterDataSpecCount = 0;
		mTypesCount = 0;
	}

	virtual ~FilterBitsImpl(void)
	{
		gFilterBits = NULL;
		for (uint32_t i=0; i<mFilterDataSpecCount; i++)
		{
			FilterDataSpec *spec = mFilterDataTypes[i];
			delete spec;
		}
		for (uint32_t i=0; i<mTypesCount; i++)
		{
			char *str = mTypes[i];
			free(str);
		}
	}

	void addTemp(const char *str) const
	{
		size_t slen = strlen(mTemp) + strlen(str)+1;
		if ( slen < MAX_TEMP_STRING )
		{
			strcat(mTemp,str);
		}
		else
		{
			assert(0); // exceeded maximum length of the temporary string; probably need to make it bigger.
		}

	}

	// Returns the ASCII string equivalent of this set of FilterData flags
	virtual const char *getFilterDataString(const FilterData &fd) const
	{
		mTemp[0] = 0;

		if ( fd.w64.typeBits64 )
		{
			uint64_t bits = 1;
			bool previous = false;
			for (uint32_t i=0; i<mTypesCount; i++)
			{
				if ( bits & fd.w64.typeBits64 )
				{
					if ( previous )
					{
						addTemp(",");
					}
					addTemp(mTypes[i]);
					previous = true;
				}
				bits = bits<<1;
			}
		}

		const FilterDataSpec *fds = static_cast< const FilterDataSpec *>(&fd);

		if ( fd.w64.matchBits64 )
		{
			addTemp("=");
			if ( fd.w64.matchBits64 == 0xffffffffffffffff && fds->allWeightOne() )
			{
				addTemp("all");
			}
			else
			{
				uint64_t bits = 1;
				bool previous = false;

				for (uint32_t i=0; i<mTypesCount; i++)
				{
					if ( bits & fd.w64.matchBits64 )
					{
						if ( previous )
						{
							addTemp(",");
						}
						addTemp(mTypes[i]);

						if ( fds->weights[i] != 1 )
						{
							addTemp("(");
							char number[512];
							sprintf(number,"%f", fds->weights[i] );
							addTemp(number);
							addTemp(")");
						}

						previous = true;
					}
					bits = bits<<1;
				}
			}
		}
		else
		{
			addTemp("=none");
		}

		return mTemp;
	}


	// Returns the combination of string of types based on this bit sequence.
	virtual const char *getFilterString(uint64_t bits) const
	{
		mTemp[0] = 0;

		if ( bits == 0 )
		{
			addTemp("none");
		}
		else if ( bits == 0xFFFFFFFFFFFFFFFF )
		{
			addTemp("all");
		}
		else
		{
			uint64_t bits = 1;
			bool previous = false;
			for (uint32_t i=0; i<mTypesCount; i++)
			{
				if ( bits & bits )
				{
					if ( previous )
					{
						addTemp(",");
					}
					addTemp(mTypes[i]);
					previous = true;
				}
				bits = bits<<1;
			}
		}
		return mTemp;
	}

	// Returns the string for a single bit type 0-63
	virtual const char *getTypeString(uint8_t type) const
	{
		const char *ret = NULL;

		if ( type < mTypesCount )
		{
			ret = mTypes[type];
		}

		return ret;
	}

	// Returns how many types were defined.
	virtual uint8_t getTypeCount(void) const
	{
		return (uint8_t) mTypesCount;
	}

	char *getAllocString(const char *str) const
	{
		if ( str == NULL ) str = "";
		size_t slen = strlen(str);
		char *ret = (char *)malloc(slen+1);
		assert(ret);
		if ( ret )
		{
			memcpy(ret,str,slen+1);
		}
		return ret;
	}

	// Return the bit flag for a single group/type
	virtual uint64_t getTypeBit(const char *str,uint32_t &index,bool &applyDefault) 
	{
		uint64_t ret = 0;

		if ( strcmp(str,"all") == 0 )
		{
			ret = 0xFFFFFFFFFFFFFFFF; 
			index = 0xFFFFFFFF;
		}
		else if ( strcmp(str,"none") == 0 )
		{
			index = 0;
		}
		else if ( strcmp(str,"default") == 0 )
		{
			index = 0;
			applyDefault = true;
		}
		else
		{
			uint64_t bit = 1;
			for (size_t i=0; i<mTypesCount; i++)
			{
				const char *t = mTypes[i];
				if ( strcmp(t,str) == 0 )
				{
					ret = bit;
					index = (uint32_t)i;
					break;
				}
				bit = bit<<1; // advance the bit field.
			}

			if ( ret == 0  )  // if the type was not already found
			{
				if ( mTypesCount < 64 )
				{
					index = mTypesCount;
					mTypes[mTypesCount] = getAllocString(str);
					mTypesCount++;
					ret = bit;
				}
				else
				{
					assert(0); // encountered more than 64 uinque bit field 'types' which exceeded the maximum size available.
				}
			}
			
		}
		return ret;
	}

	// Return true if this makes the end of the numeric (either zero byte, a percent sign, or a right parenthesis
	inline bool eon(char c)
	{
		return ( c == 0 || c == '%' || c == ')' || c == ',' );
	}
	inline bool eon1(char c)
	{
		return ( c == 0 || c == ')' || c == ',' );
	}

	// Return true if this marks the end of the keyword/string which is either a zero byte, a comma, or an open parenthesis
	inline bool eos(char c)
	{
		return ( c == 0 || c == ',' || c == '(' );
	}

	inline const char* skipLeadingSpaces(const char* str)
	{
		while ( ::isspace(*str) )
		{
			++str;
		}
		return str;
	}
	inline char* skipTrailingSpaces(char* str, char* str0)
	{
		while ( str > str0 && ::isspace(*(str-1)) )
		{
			--str;
		}
		return str;
	}

	// Converts this ASCII string to it's corresponding binary bit representation.
	virtual const FilterData *getFilterData(const char *str)
	{
		FilterData fd;

		const char *sourceString = str;

		while ( *str && *str != '=' )
		{
			char temp[256];
			char *dest = temp;

			str = skipLeadingSpaces(str);
			while ( *str && *str != ',' && *str != '=' ) 
			{
				*dest++ = *str++;
				if ( dest > &temp[254] )
				{
					break;
				}
			}
			dest = skipTrailingSpaces(dest, temp);
			*dest = 0;
			if ( *temp )
			{
				uint32_t index;
				bool adefault = false;
				fd.w64.typeBits64 |= getTypeBit(temp,index,adefault);
			}
			if ( *str == ',' )
			{
				str++;
			}
		}
		if ( *str == '=' )
		{
			bool applyDefault = false;
			float defaultWeight = 1.0f;
			str++;
			while ( *str )
			{
				char temp[256];
				char *dest = temp;

				str = skipLeadingSpaces(str);
				while ( !eos(*str) ) 
				{
					*dest++ = *str++;
					if ( dest > &temp[254] )
					{
						break;
					}
				}
				dest = skipTrailingSpaces(dest, temp);
				*dest = 0;
				uint64_t bits = 0;
				uint32_t index = 0;
				bool adefault = false;
				if (*temp)
				{
					fd.w64.matchBits64 |= (bits = getTypeBit(temp,index,adefault));
				}
				if ( *str == '(' )
				{
					str++;
					//don't need to skip leading spaces here - atof does it!
					float v = (float)atof(str);
					// scan past the weight value and up to the closing parenthesis
					while ( !eon(*str) ) str++;
					if ( *str == '%' )
					{
						v*=1.0f/100.0f;  // convert from a percentage notation to a straight up multiplier value
						str++;
						// scan up to the closing parenthesis
						while ( !eon1(*str) ) str++;
					}
					if ( adefault )
					{
						applyDefault = true;
						defaultWeight*=v;
					}
					if ( bits )
					{
						if ( index == 0xFFFFFFFF )
						{
							for (uint32_t i=0; i<64; i++)
							{
								fd.weights[i] = v;
							}
						}
						else
						{
							fd.weights[index] = v;
						}
					}
					// If we ended at the close paren, then advance past it and pick up at the next comma separated value
					if ( *str == ')' ) str++;
				}
				else
				{
					if ( adefault )
					{
						applyDefault = true;
					}
				}
				if ( *str == ',' )
				{
					str++;
				}
			}
			if ( applyDefault )
			{
				// ok any bit not currently set to true, we set to true and assign the default weight...
				uint64_t bit = 1;
				for (uint32_t i=0; i<64; i++)
				{
					if ( fd.w64.matchBits64 & bit ) // if it's already set, leave it alone..
					{

					}
					else
					{
						fd.w64.matchBits64|=bit;
						fd.weights[i] = defaultWeight;
					}
					bit = bit << 1;
				}
			}
		}



		FilterData *ret = NULL;

		for (uint32_t i=0; i<mFilterDataSpecCount; i++)
		{
			FilterDataSpec *spec = mFilterDataTypes[i];
			if ( spec )
			{
				if ( spec->w64.typeBits64 == fd.w64.typeBits64 && spec->w64.matchBits64 == fd.w64.matchBits64 )
				{
					bool areWeightsEqual = true;
					for (uint32_t i = 0; i < 64; ++i)
					{
						if (spec->weights[i] != fd.weights[i])
						{
							areWeightsEqual = false;
							break;
						}
					}
					if (areWeightsEqual)
					{
						ret = static_cast< FilterData *>(spec);
						break;
					}
				}
			}
		}
		if ( ret == NULL )
		{
			FilterDataSpec *spec = new FilterDataSpec;
			FilterData *root = static_cast< FilterData *>(spec);
			*root = fd;
			spec->mSpec = getAllocString(sourceString);
			spec->typeString = spec->mSpec;
			bool added = false;
			for (uint32_t i=0; i<mFilterDataSpecCount; i++)
			{
				if ( mFilterDataTypes[i] == NULL )
				{
					mFilterDataTypes[i] = spec;
					added = true;
					break;
				}
			}
			if ( !added )
			{
				if ( mFilterDataSpecCount < MAX_FILTER_DATA_SPECS )
				{
					mFilterDataTypes[mFilterDataSpecCount] = spec;
					mFilterDataSpecCount++;
				}
				else
				{
					assert(0); // if you hit this, it means that we encountered more than MAX_FILTER_DATA_SPECS unique filter strings; bump the maximum up higher to match the needs of your application.
				}
			}
			ret = static_cast< FilterData *>(spec);
		}

		if ( ret )
		{
			FilterDataSpec *fds = static_cast< FilterDataSpec *>(ret);
			fds->buildEncode();
		}

		return ret;
	}

	// Encode it into 128 bits
	virtual const EncodedFilterData &getEncodedFilterData(const FilterData &fd) 
	{
		const FilterDataSpec *fds = static_cast< const FilterDataSpec *>(&fd);
		const EncodedFilterData *efd = static_cast< const EncodedFilterData *>(fds);
		return *efd;
	}

	virtual bool isEncodedFilterData(const EncodedFilterData &d) const
	{
		return getFilterData(d) ? true : false;
	}

	// If this is properly encoded filter data, then return the original filter-data pointer
	virtual FilterData * getFilterData(const EncodedFilterData &d) const
	{
		FilterData *ret = NULL;

		if ( d.ew32.word0 == MAGIC_ID1 && d.ew32.word1 == MAGIC_ID2 )
		{
#if PX_X64
			ret = (FilterData *)d.ew64.matchBits64;
#else
			ret = (FilterData *)d.ew32.word2;
#endif
		}
		return ret;
	}

	// see if these two objects interact and, if so, return the weighting value to apply
	virtual bool getWeightedFilter(const EncodedFilterData &o1,const EncodedFilterData &o2,float &weight) 
	{
		bool ret = false;

		weight = 0;

		const FilterData *fd1 = getFilterData(o1);
		const FilterData *fd2 = getFilterData(o2);

		EncodedFilterData d1 = o1;
		EncodedFilterData d2 = o2;
		if ( fd1 )
		{
			d1.ew64.typeBits64 = fd1->w64.typeBits64;
			d1.ew64.matchBits64 = fd1->w64.matchBits64;
		}
		if ( fd2 )
		{
			d2.ew64.typeBits64  = fd2->w64.typeBits64;
			d2.ew64.matchBits64 = fd2->w64.matchBits64;
		}
		uint64_t mask1 = d1.ew64.typeBits64 & d2.ew64.matchBits64;
		uint64_t mask2 = d2.ew64.typeBits64 & d1.ew64.matchBits64;
		// See if the second object match bits matches the first object's type bits.
		if ( mask1 || mask2 )
		{
			ret = true;
			// Default weighting multiplier value
			weight = 1;
			const FilterDataSpec *fds1 = static_cast< const FilterDataSpec *>(fd1);
			const FilterDataSpec *fds2 = static_cast< const FilterDataSpec *>(fd2);
			// If we have a fully weighted spec, then return the weighted value of matching bits.
			if ( mask1 )
			{
				if ( fds2 )
				{
					weight = fds2->computeWeight(mask1,weight);
					mask2&=~mask1; // remove bits already processed...
				}
			}
			if ( mask2 && fds1 )
			{
				weight = fds1->computeWeight(mask2,weight);
			}
		}
		return ret;
	}

	virtual const EncodedFilterData *getEncodedFilterData(const char *str) 
	{
		const EncodedFilterData *ret = NULL;
		const FilterData *fd = getFilterData(str);
		if ( fd )
		{
			const FilterDataSpec *fds = static_cast< const FilterDataSpec *>(fd);
			ret = static_cast< const EncodedFilterData *>(fds);
		}
		return ret;
	}


	virtual void release(void)
	{
		delete this;
	}

	virtual void release(FilterData &fb) 
	{
		FilterDataSpec *fds = static_cast< FilterDataSpec *>(&fb);
		bool found = false;
		for (uint32_t i=0; i<mFilterDataSpecCount; i++)
		{
			if ( mFilterDataTypes[i] == fds )
			{
				found = true;
				mFilterDataTypes[i] = NULL;
				if ( (i+1) == mFilterDataSpecCount ) // if released the last one in the array, then decrement the array count size.
				{
					mFilterDataSpecCount--;
				}
				break;
			}
		}
		assert(found); // it should always find the address of the item being released!
	}

	mutable char					mTemp[MAX_TEMP_STRING];	// temporary string used to return requests; it's considered 'mutable' i.e. const methods are ok to modify it.
	uint32_t				mTypesCount;
	char					*mTypes[MAX_TYPES];
	uint32_t				mFilterDataSpecCount;
	FilterDataSpec			*mFilterDataTypes[MAX_FILTER_DATA_SPECS];
};

}; // end of namespace FILTER_BITS


FilterBits *createFilterBits(void)
{
	FILTER_BITS::FilterBitsImpl *f = new FILTER_BITS::FilterBitsImpl;
	return static_cast< FilterBits *>(f);
}
