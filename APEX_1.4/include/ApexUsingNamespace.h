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



#ifndef APEX_USING_NAMESPACE_H
#define APEX_USING_NAMESPACE_H

#include "foundation/Px.h"
#include "ApexDefs.h"

#define FORWARD_DECLARATION_AND_USING(T, X) \
	namespace physx \
	{ \
		T X ; \
	}; \

FORWARD_DECLARATION_AND_USING(class,	PxActor);
FORWARD_DECLARATION_AND_USING(class,	PxBase);
FORWARD_DECLARATION_AND_USING(class,	PxBoxGeometry);
FORWARD_DECLARATION_AND_USING(class,	PxCapsuleGeometry);
FORWARD_DECLARATION_AND_USING(class,	PxCloth);
FORWARD_DECLARATION_AND_USING(class,	PxCooking);
FORWARD_DECLARATION_AND_USING(struct,	PxConvexFlag);
FORWARD_DECLARATION_AND_USING(class,	PxConvexMesh);
FORWARD_DECLARATION_AND_USING(class,	PxConvexMeshGeometry);
FORWARD_DECLARATION_AND_USING(struct,	PxDebugPoint);
FORWARD_DECLARATION_AND_USING(struct,	PxDebugLine);
FORWARD_DECLARATION_AND_USING(struct,	PxDebugTriangle);
FORWARD_DECLARATION_AND_USING(struct,	PxDebugText);
FORWARD_DECLARATION_AND_USING(struct,	PxFilterData);
FORWARD_DECLARATION_AND_USING(struct,	PxFilterFlag);
FORWARD_DECLARATION_AND_USING(class,	PxGeometry);
FORWARD_DECLARATION_AND_USING(struct,	PxGeometryType);
FORWARD_DECLARATION_AND_USING(class,	PxJoint);
FORWARD_DECLARATION_AND_USING(class,	PxMaterial);
FORWARD_DECLARATION_AND_USING(struct,	PxPairFlag);
FORWARD_DECLARATION_AND_USING(class,	PxParticleBase);
FORWARD_DECLARATION_AND_USING(class,	PxParticleFluid);
FORWARD_DECLARATION_AND_USING(class,	PxParticleSystem);
FORWARD_DECLARATION_AND_USING(class,	PxPhysics);
FORWARD_DECLARATION_AND_USING(struct,	PxQueryFilterData);
FORWARD_DECLARATION_AND_USING(struct,	PxQueryFlag);
FORWARD_DECLARATION_AND_USING(struct,	PxQueryHit);
FORWARD_DECLARATION_AND_USING(struct,	PxQueryHitType);
FORWARD_DECLARATION_AND_USING(class,	PxRenderBuffer);
FORWARD_DECLARATION_AND_USING(class,	PxRigidActor);
FORWARD_DECLARATION_AND_USING(class,	PxRigidBody);
FORWARD_DECLARATION_AND_USING(struct,	PxRigidBodyFlag);
FORWARD_DECLARATION_AND_USING(class,	PxRigidDynamic);
FORWARD_DECLARATION_AND_USING(class,	PxSimulationEventCallback);
FORWARD_DECLARATION_AND_USING(class,	PxContactModifyCallback);
FORWARD_DECLARATION_AND_USING(class,	PxScene);
FORWARD_DECLARATION_AND_USING(class,	PxShape);
FORWARD_DECLARATION_AND_USING(struct,	PxShapeFlag);
FORWARD_DECLARATION_AND_USING(class,	PxSphereGeometry);
FORWARD_DECLARATION_AND_USING(class,	PxTriangleMesh);
FORWARD_DECLARATION_AND_USING(class,	PxTriangleMeshGeometry);

namespace physx
{
	namespace shdfnd {}
	using namespace shdfnd;

	namespace general_PxIOStream2
	{
		class PxFileBuf;
	}
}

namespace nvidia
{
	namespace apex {}
	using namespace apex;

	using namespace physx;
	using namespace physx::shdfnd;
	using namespace physx::general_PxIOStream2;
};



#endif // APEX_USING_NAMESPACE_H
