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
// Copyright (c) 2008-2019 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifndef ACCLAIM_LOADER
#define ACCLAIM_LOADER

#include "foundation/PxFlags.h"
#include "foundation/PxTransform.h"
#include "SampleAllocator.h"
#include "SampleArray.h"

namespace Acclaim
{

#define MAX_BONE_NAME_CHARACTER_LENGTH 100
#define MAX_BONE_NUMBER 32

///////////////////////////////////////////////////////////////////////////////
struct BoneDOFFlag
{
	enum Enum
	{
		eRX	= (1<<0), 
		eRY	= (1<<1), 
		eRZ	= (1<<2),
		eLENGTH	= (1<<3)
	};
};

typedef PxFlags<BoneDOFFlag::Enum,PxU16> BoneDOFFlags;
PX_FLAGS_OPERATORS(BoneDOFFlag::Enum, PxU16)

///////////////////////////////////////////////////////////////////////////////
struct Bone
{
	int              mID;
	char             mName[MAX_BONE_NAME_CHARACTER_LENGTH];
	PxVec3           mDirection;
	PxReal           mLength;
	PxVec3           mAxis;
	BoneDOFFlags     mDOF; 
	Bone*	         mParent;

public:
	Bone() :
		mID(-1),
		mDirection(0.0f),
		mLength(0.0f),
		mAxis(0.0f, 0.0f, 1.0f),
		mDOF(0),
		mParent(NULL)
		{
		}	
};

///////////////////////////////////////////////////////////////////////////////
struct ASFData
{
	struct Header
	{
		PxReal           mMass;
		PxReal           mLengthUnit;
		bool             mAngleInDegree;
	};

	struct Root
	{
		PxVec3           mPosition;
		PxVec3           mOrientation;
	};

	Header               mHeader;
	Root	             mRoot;
	Bone*                mBones;
	PxU32                mNbBones;

public:
	void release() { if (mBones) free(mBones); }
};

///////////////////////////////////////////////////////////////////////////////
struct FrameData
{
	PxVec3   mRootPosition;
	PxVec3   mRootOrientation;

	PxVec3   mBoneFrameData[MAX_BONE_NUMBER];
	PxU32    mNbBones;
};

///////////////////////////////////////////////////////////////////////////////
struct AMCData
{
	FrameData*    mFrameData;
	PxU32         mNbFrames;

public:
	void release() { if (mFrameData) free(mFrameData); }
};

///////////////////////////////////////////////////////////////////////////////
bool readASFData(const char* filename, ASFData& data);
bool readAMCData(const char* filename, ASFData& asfData, AMCData& amcData);
}

#endif // ACCLAIM_LOADER
