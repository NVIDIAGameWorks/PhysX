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

#ifndef SAMPLE_CHARACTER_CLOTH_FLAG_H
#define SAMPLE_CHARACTER_CLOTH_FLAG_H

#include "cloth/PxCloth.h"
#include "PhysXSample.h"

class SampleCharacterCloth;
class RenderClothActor;

class SampleCharacterClothFlag
{
public:
    SampleCharacterClothFlag(SampleCharacterCloth& sample, const PxTransform &pose, PxU32 resX, PxU32 resY, PxReal sizeX, PxReal sizeY, PxReal height);

	void     setWind(const PxVec3 &dir, PxReal strength, const PxVec3& range);
    void     update(PxReal dtime);
	void     release();
	PxCloth* getCloth() { return mCloth; }
    
protected:
	RenderClothActor*		mRenderActor;
	SampleCharacterCloth&	mSample;
    PxCloth*                mCloth;
    SampleArray<PxVec2>     mUVs;
    PxVec3                  mWindDir;
    PxVec3                  mWindRange;
	PxReal                  mWindStrength;
    PxReal                  mTime;
	PxRigidDynamic*			mCapsuleActor;
private:
	SampleCharacterClothFlag& operator=(const SampleCharacterClothFlag&);
};

#endif
