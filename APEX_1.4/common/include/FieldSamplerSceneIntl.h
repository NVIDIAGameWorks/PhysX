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



#ifndef FIELD_SAMPLER_SCENE_INTL_H
#define FIELD_SAMPLER_SCENE_INTL_H

#include "ApexDefs.h"
#include "PxTask.h"

#include "ModuleIntl.h"
#include "ApexSDKIntl.h"

namespace nvidia
{

namespace fieldsampler
{
	struct FieldSamplerKernelLaunchDataIntl;
}

namespace apex
{

class ApexCudaConstStorage;

struct FieldSamplerSceneDescIntl
{
	bool	isPrimary;

	FieldSamplerSceneDescIntl()
	{
		isPrimary = false;
	}
};

struct FieldSamplerQueryDataIntl;

class FieldSamplerSceneIntl : public ModuleSceneIntl
{
public:
	virtual void getFieldSamplerSceneDesc(FieldSamplerSceneDescIntl& desc) const = 0;

	virtual const PxTask* onSubmitFieldSamplerQuery(const FieldSamplerQueryDataIntl& data, const PxTask* )
	{
		PX_UNUSED(data);
		return 0;
	}

#if APEX_CUDA_SUPPORT
	virtual ApexCudaConstStorage*	getFieldSamplerCudaConstStorage()
	{
		APEX_INVALID_OPERATION("not implemented");
		return 0;
	}

	virtual bool					launchFieldSamplerCudaKernel(const nvidia::fieldsampler::FieldSamplerKernelLaunchDataIntl&)
	{
		APEX_INVALID_OPERATION("not implemented");
		return false;
	}
#endif

	virtual SceneStats* getStats()
	{
		return 0;
	}

};

#define FSST_PHYSX_MONITOR_LOAD		"FieldSamplerScene::PhysXMonitorLoad"
#define FSST_PHYSX_MONITOR_FETCH	"FieldSamplerScene::PhysXMonitorFetch"
#define FSST_PHYSX_MONITOR_UPDATE	"FieldSamplerPhysXMonitor::Update"
}

} // end namespace nvidia::apex

#endif // FIELD_SAMPLER_SCENE_INTL_H
