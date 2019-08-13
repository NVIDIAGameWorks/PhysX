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


#ifndef DV_ARTICULATION_JOINT_CORE_H
#define DV_ARTICULATION_JOINT_CORE_H

#include "DyArticulationCore.h"
#include "PxArticulationJoint.h"
#include "solver/PxSolverDefs.h"
#include "PxArticulationJointReducedCoordinate.h"

namespace physx
{
	namespace Dy
	{
		struct ArticulationJointCoreBase
		{
			//= ATTENTION! =====================================================================================
			// Changing the data layout of this class breaks the binary serialization format.  See comments for 
			// PX_BINARY_SERIAL_VERSION.  If a modification is required, please adjust the getBinaryMetaData 
			// function.  If the modification is made on a custom branch, please change PX_BINARY_SERIAL_VERSION
			// accordingly.
			//==================================================================================================
		public:

			PX_CUDA_CALLABLE bool setJointPose(PxQuat& relativeQuat)
			{
				if (dirtyFlag & ArticulationJointCoreDirtyFlag::ePOSE)
				{
					relativeQuat = (childPose.q * (parentPose.q.getConjugate())).getNormalized();

					//ML: this way work in GPU
					PxU8 flag = PxU8(ArticulationJointCoreDirtyFlag::ePOSE);
					dirtyFlag &= ArticulationJointCoreDirtyFlags(~flag);
				
					return true;
				}

				return false;
			}

			PX_CUDA_CALLABLE PX_FORCE_INLINE void operator=(ArticulationJointCoreBase& other)
			{
				parentPose = other.parentPose;
				childPose = other.childPose;

				dirtyFlag = other.dirtyFlag;

				//KS - temp place to put reduced coordinate limit and drive values
				for(PxU32 i=0; i<PxArticulationAxis::eCOUNT; i++)
				{
					limits[i] = other.limits[i];
					drives[i] = other.drives[i];
					targetP[i] = other.targetP[i];
					targetV[i] = other.targetV[i];

					dofIds[i] = other.dofIds[i];
					motion[i] = other.motion[i];
				}

				frictionCoefficient = other.frictionCoefficient;
				//relativeQuat = other.relativeQuat;
				jointType = other.jointType;
				jointOffset = other.jointOffset; //this is the dof offset for the joint in the cache
			}

			// attachment points, don't change the order, otherwise it will break GPU code
			PxTransform						parentPose;								//28		28
			PxTransform						childPose;								//28		56

			//KS - temp place to put reduced coordinate limit and drive values
			PxArticulationLimit				limits[PxArticulationAxis::eCOUNT];		//48		104
			PxArticulationDrive				drives[PxArticulationAxis::eCOUNT];		//96		200
			PxReal							targetP[PxArticulationAxis::eCOUNT];	//24		224
			PxReal							targetV[PxArticulationAxis::eCOUNT];	//24		248
			
			// initial parent to child rotation. Could be 
			//PxQuat							relativeQuat;							//16		264
			PxReal							frictionCoefficient;					//4			268

			PxU8							dofIds[PxArticulationAxis::eCOUNT];		//6			274
			PxU8							motion[PxArticulationAxis::eCOUNT];		//6			280

			PxReal							maxJointVelocity;						//4			284

			//Currently, jointOffset can't exceed 64*3 so we can use a PxU8 here! This brings mem footprint to exactly a multiple of 16 bytes
			//this is the dof offset for the joint in the cache. 
			PxU8							jointOffset;							//1			285
			ArticulationJointCoreDirtyFlags	dirtyFlag;								//1			286
			PxU8							jointType;								//1			287
			PxU8							pad[1];

			ArticulationJointCoreBase() { maxJointVelocity = 100.f; }
			// PX_SERIALIZATION
			ArticulationJointCoreBase(const PxEMPTY&) : dirtyFlag(PxEmpty) { PX_COMPILE_TIME_ASSERT(sizeof(PxArticulationMotions) == sizeof(PxU8)); }
			//~PX_SERIALIZATION
		};
	}
}

#endif
