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



// This file is used to define a list of AgPerfMon events.
//
// This file should only contain event definitions, using the
// DEFINE_EVENT macro.  E.g.:
//
//     DEFINE_EVENT(sample_name_1)
//     DEFINE_EVENT(sample_name_2)
//     DEFINE_EVENT(sample_name_3)

// Framework only event definitions

DEFINE_EVENT(ApexScene_simulate)
DEFINE_EVENT(ApexScene_fetchResults)
DEFINE_EVENT(ApexScene_checkResults)
DEFINE_EVENT(ApexSceneManualSubstep)
DEFINE_EVENT(ModuleSceneManualSubstep)
DEFINE_EVENT(ApexSceneBeforeStep)
DEFINE_EVENT(ApexSceneDuringStep)
DEFINE_EVENT(ApexSceneAfterStep)
DEFINE_EVENT(ApexScenePostFetchResults)
DEFINE_EVENT(ApexSceneLODUsedResource)
DEFINE_EVENT(ApexSceneLODSumBenefit)
DEFINE_EVENT(ApexRenderMeshUpdateRenderResources)
DEFINE_EVENT(ApexRenderMeshCreateRenderResources)
DEFINE_EVENT(ApexRenderMeshDispatchRenderResources)
DEFINE_EVENT(ApexRenderMeshUpdateInstances)
DEFINE_EVENT(ApexRenderMeshUpdateInstancesWritePoses)
DEFINE_EVENT(ApexRenderMeshUpdateInstancesWriteScales)