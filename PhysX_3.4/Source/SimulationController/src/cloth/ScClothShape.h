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


#ifndef PX_PHYSICS_CLOTHSHAPE
#define PX_PHYSICS_CLOTHSHAPE

#include "CmPhysXCommon.h"
#include "PxPhysXConfig.h"
#if PX_USE_CLOTH_API

#include "ScElementSim.h"
#include "ScClothCore.h"

namespace physx
{
namespace Sc
{
	/**
	A collision detection primitive for cloth.
	*/
	class ClothShape : public ElementSim
	{
		ClothShape &operator=(const ClothShape &);
		public:
												ClothShape(ClothSim& cloth);
												~ClothShape();

		// ElementSim implementation
		virtual		void						getFilterInfo(PxFilterObjectAttributes& filterAttr, PxFilterData& filterData) const;
		// ~ElementSim

		public:
        PX_INLINE	PxBounds3					getWorldBounds() const { return mClothCore.getWorldBounds(); }
		PX_INLINE	ClothSim&	                getClothSim() const { return *mClothCore.getSim(); }

                    void                        updateBoundsInAABBMgr();

					void						createLowLevelVolume();
					void						destroyLowLevelVolume();
		private:
                    ClothCore&                  mClothCore;
					bool						mHasCollision;
	};

} // namespace Sc

}

#endif	// PX_USE_CLOTH_API

#endif
