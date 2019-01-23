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

#ifndef MESHBUILDER_H
#define MESHBUILDER_H

#include "PhysXSample.h"

class SampleLargeWorld;
class MeshBuilder : public SampleAllocateable
{
public:
	MeshBuilder(SampleLargeWorld& sample, const char* filename);
	~MeshBuilder() {}
	
	void			addRepXToPxCollection(
												const PxTransform&		pose, 
												const PxVec3&			scale, 
												PxCollection&			collection,
												const char*				name = NULL);

	static void		addObjMeshToPxCollection(
												PxPhysics&				physics,
												PxCooking&				cooking,
												PxMaterial&				material,
												const char*				filename,
												const char*				filenameCooked,
												const PxTransform&		pose, 
												const PxVec3&			scale, 
												PxCollection&			collection,
												const char*				name = NULL);

private:
	MeshBuilder& operator=(const MeshBuilder&);
	SampleLargeWorld*	mSampleLargeWorld;

	PxPhysics&			mPhysics;
	PxScene&			mScene;
	PxCooking&			mCooking;
	PxMaterial&			mMaterial;
	PxRigidStatic*		mStaticActor;
	PxStringTable*		mStringTable;

	std::vector<PxConvexMesh*>		mConvexMeshes;
	std::vector<PxTriangleMesh*>	mTriMeshes;
};


#endif
