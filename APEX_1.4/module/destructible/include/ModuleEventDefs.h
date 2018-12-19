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
// This file is included exclusively by AgPerfMonEventSrcAPI.h
// and by AgPerfMonEventSrcAPI.cpp, for the purpose of building
// an enumeration (enum xx) and an array of strings ()
// that contain the list of events.
//
// This file should only contain event definitions, using the
// DEFINE_EVENT macro.  E.g.:
//
//     DEFINE_EVENT(sample_name_1)
//     DEFINE_EVENT(sample_name_2)
//     DEFINE_EVENT(sample_name_3)

DEFINE_EVENT(DestructibleFetchResults)
DEFINE_EVENT(DestructibleUpdateRenderResources)
DEFINE_EVENT(DestructibleDispatchRenderResources)
DEFINE_EVENT(DestructibleSeparateUnsupportedIslands)
DEFINE_EVENT(DestructibleCreateDynamicIsland)
DEFINE_EVENT(DestructibleCookChunkCollisionMeshes)
DEFINE_EVENT(DestructibleAddActors)
DEFINE_EVENT(DestructibleCreateRoot)
DEFINE_EVENT(DestructibleAppendShape)
DEFINE_EVENT(DestructibleStructureTick)
DEFINE_EVENT(DestructiblePhysBasedStressSolver)
DEFINE_EVENT(DestructiblePhysBasedStressSolverInitial)
DEFINE_EVENT(DestructiblePhysBasedStressSolverCalculateForce)
DEFINE_EVENT(DestructiblePhysBasedStressSolverRemoveChunk)
DEFINE_EVENT(DestructibleStructureBoundsCalculateOverlaps)
DEFINE_EVENT(DestructibleStructureDetailedOverlapTest)
DEFINE_EVENT(DestructibleCreateActor)
DEFINE_EVENT(DestructibleCacheChunkCookedCollisionMeshes)
DEFINE_EVENT(DestructibleFractureChunk)
DEFINE_EVENT(DestructibleChunkReport)
DEFINE_EVENT(DestructibleRayCastFindVisibleChunk)
DEFINE_EVENT(DestructibleRayCastFindDeepestChunk)
DEFINE_EVENT(DestructibleRayCastProcessChunk)
DEFINE_EVENT(DestructibleRemoveChunksForBudget)
DEFINE_EVENT(DestructibleProcessFractureBuffer)
DEFINE_EVENT(DestructibleKillStructures)
DEFINE_EVENT(DestructibleOnContactConstraint)
DEFINE_EVENT(DestructibleOnContactNotify)
DEFINE_EVENT(DestructibleBeforeTickLockRenderables)
DEFINE_EVENT(DestructibleUpdateRenderMeshBonePoses)
DEFINE_EVENT(DestructibleStructureBuildSupportGraph)
DEFINE_EVENT(DestructibleSimulate)
DEFINE_EVENT(DestructibleCalcStaticChunkBenefit)
DEFINE_EVENT(DestructibleGatherNonessentialChunks)
DEFINE_EVENT(DestructibleCalculateBenefit)
DEFINE_EVENT(DestructibleGetChunkGlobalPose)
