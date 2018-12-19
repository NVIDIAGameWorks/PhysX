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



#ifndef TABLE_LOOKUP_H
#define TABLE_LOOKUP_H

namespace nvidia
{
namespace apex
{

// Stored Tables
const float custom[] =	{ 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f };
const float linear[] =	{ 1.00f, 0.90f, 0.80f, 0.70f, 0.60f, 0.50f, 0.40f, 0.30f, 0.20f, 0.10f, 0.00f };
const float steep[] =	{ 1.00f, 0.99f, 0.96f, 0.91f, 0.84f, 0.75f, 0.64f, 0.51f, 0.36f, 0.19f, 0.00f };
const float scurve[] =	{ 1.00f, 0.99f, 0.96f, 0.91f, 0.80f, 0.50f, 0.20f, 0.09f, 0.04f, 0.01f, 0.00f };

// Table sizes must be same for all stored tables
#define NUM_ELEMENTS(X) (sizeof(X)/sizeof(*(X)))
#define TABLE_SIZE NUM_ELEMENTS(custom)

// Stored Table enums
struct TableName
{
	enum Enum
	{
		CUSTOM = 0,
		LINEAR,
		STEEP,
		SCURVE,
	};
};

struct TableLookup
{
	float xVals[TABLE_SIZE];
	float yVals[TABLE_SIZE];
	float x1;
	float x2;
	float multiplier;

#ifdef __CUDACC__
	__device__ TableLookup() {}
#else
	TableLookup():
		x1(0),
		x2(0),
		multiplier(0)
	{
		zeroTable();
	}
#endif

	PX_CUDA_CALLABLE void zeroTable()
	{
		for (size_t i = 0; i < TABLE_SIZE; ++i)
		{
			xVals[i] = 0.0f;
			yVals[i] = 0.0f;
		}
	}

	PX_CUDA_CALLABLE void applyStoredTable(TableName::Enum tableName)
	{
		// build y values
		for (size_t i = 0; i < TABLE_SIZE; ++i)
		{
			if (tableName == TableName::LINEAR)
			{
				yVals[i] = linear[i];
			}
			else if (tableName == TableName::STEEP)
			{
				yVals[i] = steep[i];
			}
			else if (tableName == TableName::SCURVE)
			{
				yVals[i] = scurve[i];
			}
			else if (tableName == TableName::CUSTOM)
			{
				yVals[i] = custom[i];
			}
		}
	}

	PX_CUDA_CALLABLE void buildTable()
	{
		// build x values
		float interval = (x2 - x1) / (TABLE_SIZE - 1);
		for (size_t i = 0; i < TABLE_SIZE; ++i)
		{
			xVals[i] = x1 + i * interval;
		}

		// apply multipler to y values
		if (multiplier >= -1.0f && multiplier <= 1.0f)
		{
			for (size_t i = 0; i < TABLE_SIZE; ++i)
			{
				yVals[i] = yVals[i] * multiplier;
			}

			// offset = max y value in array
			float max = yVals[0];
			for (size_t i = 1; i < TABLE_SIZE; ++i)
			{
				if (yVals[i] > max)
				{
					max = yVals[i];
				}
			}

			// apply offset
			for (size_t i = 0; i < TABLE_SIZE; ++i)
			{
				yVals[i] = yVals[i] + (1.0f - max);
			}
		}
	}
	
	PX_CUDA_CALLABLE float lookupTableValue(float x) const
	{
		if (x <= xVals[0])
		{
			return yVals[0];
		}
		else if (x >= xVals[TABLE_SIZE-1])
		{
			return yVals[TABLE_SIZE-1];
		}
		else
		{
			// linear interpolation between x values
			float interval = (xVals[TABLE_SIZE-1] - xVals[0]) / (TABLE_SIZE - 1);
			uint32_t lerpPos = (uint32_t)((x - xVals[0]) / interval);
			float yDiff = yVals[lerpPos+1] - yVals[lerpPos];
			return yVals[lerpPos] + (x - xVals[lerpPos]) / interval * yDiff;
		}
	}
};

}
} // namespace nvidia::apex

#endif
