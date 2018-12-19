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



#ifndef FIELD_SAMPLER_QUERY_INTL_H
#define FIELD_SAMPLER_QUERY_INTL_H

#include "ApexDefs.h"
#include "ApexMirroredArray.h"

#include "PxTask.h"
#include "ApexActor.h"
#include "PxMat44.h"

namespace nvidia
{
namespace apex
{


class FieldSamplerSceneIntl;

struct FieldSamplerQueryDescIntl
{
	uint32_t					maxCount;
#if PX_PHYSICS_VERSION_MAJOR == 3
	PxFilterData			samplerFilterData;
#endif
	FieldSamplerSceneIntl*	ownerFieldSamplerScene;


	FieldSamplerQueryDescIntl()
	{
		maxCount                = 0;
#if PX_PHYSICS_VERSION_MAJOR == 3
		samplerFilterData.word0 = 0xFFFFFFFF;
		samplerFilterData.word1 = 0xFFFFFFFF;
		samplerFilterData.word2 = 0xFFFFFFFF;
		samplerFilterData.word3 = 0xFFFFFFFF;
#endif
		ownerFieldSamplerScene  = 0;
	}
};

struct FieldSamplerQueryDataIntl
{
	float						timeStep;
	uint32_t						count;
	bool						isDataOnDevice;

	uint32_t						positionStrideBytes; //Stride for position
	uint32_t						velocityStrideBytes; //Stride for velocity
	float*						pmaInPosition;
	float*						pmaInVelocity;
	PxVec4*						pmaOutField;

	uint32_t						massStrideBytes; //if massStride set to 0 supposed single mass for all objects
	float*						pmaInMass;

	uint32_t*						pmaInIndices;
};


#if APEX_CUDA_SUPPORT

class ApexCudaArray;

struct FieldSamplerQueryGridDataIntl
{
	uint32_t numX, numY, numZ;

	PxMat44			gridToWorld;

	float			mass;

	float			timeStep;

	PxVec3			cellSize;

	ApexCudaArray*	resultVelocity; //x, y, z = velocity vector, w = weight

	CUstream		stream;
};
#endif

class FieldSamplerCallbackIntl
{
public:
	virtual void operator()(void* stream = NULL) = 0;
};

class FieldSamplerQueryIntl : public ApexActor
{
public:
	virtual PxTaskID submitFieldSamplerQuery(const FieldSamplerQueryDataIntl& data, PxTaskID taskID) = 0;

	//! FieldSamplerCallbackIntl will be called before FieldSampler computations
	virtual void setOnStartCallback(FieldSamplerCallbackIntl*) = 0;
	//! FieldSamplerCallbackIntl will be called after FieldSampler computations
	virtual void setOnFinishCallback(FieldSamplerCallbackIntl*) = 0;

#if APEX_CUDA_SUPPORT
	virtual PxVec3 executeFieldSamplerQueryOnGrid(const FieldSamplerQueryGridDataIntl&)
	{
		APEX_INVALID_OPERATION("not implemented");
		return PxVec3(0.0f);
	}
#endif

protected:
	virtual ~FieldSamplerQueryIntl() {}
};

}
} // end namespace nvidia::apex

#endif // #ifndef FIELD_SAMPLER_QUERY_INTL_H
