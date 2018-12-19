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


#ifndef PX_PHYSICS_SCP_CLOTH_SIM
#define PX_PHYSICS_SCP_CLOTH_SIM

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "CmMatrix34.h"
#include "ScClothShape.h"

namespace physx
{

#if PX_SUPPORT_GPU_PHYSX
	struct PxClothCollisionSphere;
#endif

namespace Sc
{
	class ClothCore;
	class ShapeSim;

	class ClothSim : public ActorSim
	{
	public:

		ClothSim(Scene&, ClothCore&);
		~ClothSim();

		//---------------------------------------------------------------------------------
		// Actor implementation
		//---------------------------------------------------------------------------------
	public:
		ClothCore& getCore() const;

		void updateBounds();
		void startStep();
		void reinsert();

		bool addCollisionShape(const ShapeSim* shape);
		void removeCollisionShape(const ShapeSim* shape);

		bool addCollisionSphere(const ShapeSim* shape);
		void removeCollisionSphere(const ShapeSim* shape);

		bool addCollisionCapsule(const ShapeSim* shape);
		void removeCollisionCapsule(const ShapeSim* shape);

		bool addCollisionPlane(const ShapeSim* shape);
		void removeCollisionPlane(const ShapeSim* shape);

		bool addCollisionBox(const ShapeSim* shape);
		void removeCollisionBox(const ShapeSim* shape);

		bool addCollisionConvex(const ShapeSim* shape);
		void removeCollisionConvex(const ShapeSim* shape);

		bool addCollisionMesh(const ShapeSim* shape);
		void removeCollisionMesh(const ShapeSim* shape);

		bool addCollisionHeightfield(const ShapeSim* shape);
		void removeCollisionHeightfield(const ShapeSim* shape);

		void updateRigidBodyPositions();
		void clearCollisionShapes();

	private:
		void insertShapeSim(PxU32, const ShapeSim*);
		ClothSim &operator=(const ClothSim &);

	private:
        ClothShape mClothShape;

		PxU32 mNumSpheres;
		PxU32 mNumCapsules;
		PxU32 mNumPlanes;
		PxU32 mNumBoxes;
		PxU32 mNumConvexes;
		PxU32 mNumMeshes;
		PxU32 mNumHeightfields;
		PxU32 mNumConvexPlanes;

		shdfnd::Array<const ShapeSim*> mShapeSims;
		shdfnd::Array<Cm::Matrix34> mStartShapeTrafos;

	};

} // namespace Sc
}

#endif	// PX_PHYSICS_SCP_CLOTH_SIM

#endif
