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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "TerrainRandomSamplePrecompute.h"
#include "SceneVehicle.h"
#include "PxTkStream.h"
#include "MediaPath.h"

using namespace PxToolkit;

#define WRITE_SEQUENCE 0

PX_FORCE_INLINE PxF32 flip(const PxF32* v)
{
	const PxU8* b = (const PxU8*)v;
	PxF32 f;
	PxU8* bf = (PxU8*)&f;
	bf[0] = b[3];
	bf[1] = b[2];
	bf[2] = b[1];
	bf[3] = b[0];
	return f;
}

TerrainRandomSamplePrecompute::TerrainRandomSamplePrecompute(SceneVehicle& app, const char* resourcePath)
	: mPrecomputedRandomSequence(NULL),
	mPrecomputedRandomSequenceCount(0)
{
	mPrecomputedRandomSequence = new PxF32[(sizeof(PxF32)*(PRECOMPUTED_RANDOM_SEQUENCE_SIZE + 1))];

#if WRITE_SEQUENCE
	char buffer[256];
	const char* filename = getSampleOutputDirManager().getFilePath("SampleBaseRandomSequence", buffer, false);
	const PxF32 denom = (1.0f / float(RAND_MAX));
	for (PxU32 i = 0; i<PRECOMPUTED_RANDOM_SEQUENCE_SIZE; i++)
	{
		mPrecomputedRandomSequence[i] = float(rand()) * denom;
	}
	mPrecomputedRandomSequence[PRECOMPUTED_RANDOM_SEQUENCE_SIZE] = 1.0f;
	PxDefaultFileOutputStream stream(filename);
	stream.write(mPrecomputedRandomSequence, sizeof(PxF32)*(PRECOMPUTED_RANDOM_SEQUENCE_SIZE + 1));

#else
	const char* filename = FindMediaFile("SampleBaseRandomSequence", resourcePath);
	PxDefaultFileInputData stream(filename);
	if (!stream.isValid())
		printf("SampleBaseRandomSequence file not found");
	stream.read(mPrecomputedRandomSequence, sizeof(PxF32)*(PRECOMPUTED_RANDOM_SEQUENCE_SIZE + 1));

	const bool mismatch = (1.0f != mPrecomputedRandomSequence[PRECOMPUTED_RANDOM_SEQUENCE_SIZE]);
	if (mismatch)
	{
		for (PxU32 i = 0; i<PRECOMPUTED_RANDOM_SEQUENCE_SIZE; i++)
		{
			mPrecomputedRandomSequence[i] = flip(&mPrecomputedRandomSequence[i]);
		}
	}

#endif
}

TerrainRandomSamplePrecompute::~TerrainRandomSamplePrecompute()
{
	delete[] mPrecomputedRandomSequence;
	mPrecomputedRandomSequenceCount = 0;
}
