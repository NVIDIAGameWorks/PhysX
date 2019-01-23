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

#ifndef PLANET_H
#define PLANET_H

#include "SampleAllocator.h"
#include "foundation/PxVec3.h"

	class PlanetMesh
	{
		public:
											PlanetMesh();
											~PlanetMesh();

							void			generate(const PxVec3& center, PxF32 radius, PxU32 nbX, PxU32 nbY);

			PX_FORCE_INLINE	PxU32			getNbVerts()	const	{ return mNbVerts;	}
			PX_FORCE_INLINE	PxU32			getNbTris()		const	{ return mNbTris;	}
			PX_FORCE_INLINE	const PxVec3*	getVerts()		const	{ return mVerts;	}
			PX_FORCE_INLINE	const PxU32*	getIndices()	const	{ return mIndices;	}

		protected:
							PxU32			mNbVerts;
							PxU32			mNbTris;
							PxVec3*			mVerts;
							PxU32*			mIndices;

							PxU8			checkCulling(const PxVec3& center, PxU32 index0, PxU32 index1, PxU32 index2)	const;
	};

#endif
