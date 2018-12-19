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

#ifndef TEST_CLOTH_HELPERS_H
#define TEST_CLOTH_HELPERS_H

#include "PxShape.h"
#include "PsArray.h"
#include "cloth/PxCloth.h"
#include "cloth/PxClothTypes.h"
#include "cloth/PxClothCollisionData.h"
#include "extensions/PxClothMeshDesc.h"
#include "cooking/PxCooking.h"
#include "foundation/PxVec2.h"
#include "foundation/PxVec4.h"
#include "Test.h"


template<typename T> class SampleArray;

namespace Test
{
	/// simple utility functions for PxCloth
	class ClothHelpers
	{
	public:

		// border flags
		enum BorderFlags
		{
			NONE = 0,
			BORDER_LEFT		= (1 << 0),
			BORDER_RIGHT	= (1 << 1),
			BORDER_BOTTOM	= (1 << 2),
			BORDER_TOP		= (1 << 3),
			BORDER_NEAR		= (1 << 4),
			BORDER_FAR		= (1 << 5)
		};
        typedef PxFlags<BorderFlags, PxU16> PxBorderFlags;
		// attach cloth border 
		static bool attachBorder(PxClothParticle* particles, PxU32 numParticles, PxBorderFlags borderFlag);
		static bool attachBorder(PxCloth& cloth, PxBorderFlags borderFlag);

		// constrain cloth particles that overlap the given shape
		static bool attachClothOverlapToShape(PxCloth& cloth, PxShape& shape, PxReal radius = 0.1f);

		// create cloth mesh descriptor for a grid mesh defined along two (u,v) axis.
		static PxClothMeshDesc createMeshGrid(PxVec3 dirU, PxVec3 dirV, PxU32 numU, PxU32 numV,
			SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords);

		// create cloth mesh from obj file
		static PxClothMeshDesc createMeshFromObj(const char* filename, PxReal scale, PxQuat rot, PxVec3 offset, 
			SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords);

		// create capsule data in local space of pose
		static bool createCollisionCapsule(const PxTransform &pose, const PxVec3 &center0, PxReal r0, const PxVec3 &center1, PxReal r1, 
			SampleArray<PxClothCollisionSphere> &spheres, SampleArray<PxU32> &indexPairs);

		// create virtual particle samples
		static bool createVirtualParticles(PxCloth& cloth, PxClothMeshDesc& meshDesc, int numSamples);

		// get world bounds containing all the colliders and the cloth
		static PxBounds3 getAllWorldBounds(PxCloth& cloth);

		// get particle location from the cloth
		static bool getParticlePositions(PxCloth&cloth, SampleArray<PxVec3> &positions);

		// remove duplicate vertices 
		static PxClothMeshDesc removeDuplicateVertices(PxClothMeshDesc &inMesh,  SampleArray<PxVec2> &inTexcoords,
			SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices, SampleArray<PxVec2>& texcoords);

		// merge two mesh descriptor and uvs into a single one
		static PxClothMeshDesc mergeMeshDesc(PxClothMeshDesc &desc1, PxClothMeshDesc &desc2,  
			SampleArray<PxVec4>& vertices, SampleArray<PxU32>& indices,
			SampleArray<PxVec2>& texcoords1, SampleArray<PxVec2>& texcoords2, SampleArray<PxVec2>& texcoords);  

		// set motion constraint radius
		static bool setMotionConstraints(PxCloth &cloth, PxReal radius);

		// set particle location from the cloth
		static bool setParticlePositions(PxCloth&cloth, const SampleArray<PxVec3> &positions, bool useConstrainedOnly = true, bool useCurrentOnly = true);

		// set stiffness for all the phases
		static bool setStiffness(PxCloth& cloth, PxReal stiffness);
	};
}


#endif
