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



#ifndef FIELD_SAMPLER_INTL_H
#define FIELD_SAMPLER_INTL_H

#include "InplaceTypes.h"
#include "FieldBoundaryIntl.h"

#ifndef __CUDACC__
#include "ApexSDKIntl.h"
#endif

namespace nvidia
{
namespace apex
{


struct FieldSamplerTypeIntl
{
	enum Enum
	{
		FORCE,
		ACCELERATION,
		VELOCITY_DRAG,
		VELOCITY_DIRECT,
	};
};

struct FieldSamplerGridSupportTypeIntl
{
	enum Enum
	{
		NONE = 0,
		SINGLE_VELOCITY,
		VELOCITY_PER_CELL,
	};
};

#ifndef __CUDACC__

struct FieldSamplerDescIntl
{
	FieldSamplerTypeIntl::Enum			type;
	FieldSamplerGridSupportTypeIntl::Enum	gridSupportType;
	bool								cpuSimulationSupport;
#if PX_PHYSICS_VERSION_MAJOR == 3
	PxFilterData						samplerFilterData;
	PxFilterData						boundaryFilterData;
#endif
	float						boundaryFadePercentage;

	float						dragCoeff; //only used then type is VELOCITY_DRAG

	void*								userData;

	FieldSamplerDescIntl()
	{
		type                     = FieldSamplerTypeIntl::FORCE;
		gridSupportType          = FieldSamplerGridSupportTypeIntl::NONE;
		cpuSimulationSupport = true;
#if PX_PHYSICS_VERSION_MAJOR == 3
		samplerFilterData.word0  = 0xFFFFFFFF;
		samplerFilterData.word1  = 0xFFFFFFFF;
		samplerFilterData.word2  = 0xFFFFFFFF;
		samplerFilterData.word3  = 0xFFFFFFFF;
		boundaryFilterData.word0 = 0xFFFFFFFF;
		boundaryFilterData.word1 = 0xFFFFFFFF;
		boundaryFilterData.word2 = 0xFFFFFFFF;
		boundaryFilterData.word3 = 0xFFFFFFFF;
#endif
		boundaryFadePercentage   = 0.1;
		dragCoeff                = 0;
		userData                 = NULL;
	}
};


class FieldSamplerIntl
{
public:
	//returns true if shape/params was changed
	//required to return true on first call!
	virtual bool updateFieldSampler(FieldShapeDescIntl& shapeDesc, bool& isEnabled) = 0;

	struct ExecuteData
	{
		uint32_t            count;
		uint32_t            positionStride;
		uint32_t            velocityStride;
		uint32_t            massStride;
		uint32_t            indicesMask;
		const float*	    position;
		const float*		velocity;
		const float*		mass;
		const uint32_t*		indices;
		PxVec3*	        resultField;
	};

	virtual void executeFieldSampler(const ExecuteData& data)
	{
		PX_UNUSED(data);
		APEX_INVALID_OPERATION("not implemented");
	}

#if APEX_CUDA_SUPPORT
	struct CudaExecuteInfo
	{
		uint32_t		executeType;
		InplaceHandleBase	executeParamsHandle;
	};

	virtual void getFieldSamplerCudaExecuteInfo(CudaExecuteInfo& info) const
	{
		PX_UNUSED(info);
		APEX_INVALID_OPERATION("not implemented");
	}
#endif

	virtual PxVec3 queryFieldSamplerVelocity() const
	{
		APEX_INVALID_OPERATION("not implemented");
		return PxVec3(0.0f);
	}

protected:
	virtual ~FieldSamplerIntl() {}
};

#endif // __CUDACC__

}
} // end namespace nvidia::apex

#endif // #ifndef FIELD_SAMPLER_INTL_H
