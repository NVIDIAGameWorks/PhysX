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

#ifndef GU_VEC_SHRUNK_BOX_H
#define GU_VEC_SHRUNK_BOX_H

/** \addtogroup geomutils
@{
*/
#include "PxPhysXCommonConfig.h"
#include "GuVecConvex.h"
#include "PsVecTransform.h"
#include "GuConvexSupportTable.h"
#include "GuVecBox.h"



namespace physx
{

namespace Gu
{

	/**
	\brief Represents an oriented bounding box. 

	As a center point, extents(radii) and a rotation. i.e. the center of the box is at the center point, 
	the box is rotated around this point with the rotation and it is 2*extents in width, height and depth.
	*/

	/**
	Box geometry

	The rot member describes the world space orientation of the box.
	The center member gives the world space position of the box.
	The extents give the local space coordinates of the box corner in the positive octant.
	Dimensions of the box are: 2*extent.
	Transformation to world space is: worldPoint = rot * localPoint + center
	Transformation to local space is: localPoint = T(rot) * (worldPoint - center)
	Where T(M) denotes the transpose of M.
	*/

	class ShrunkBoxV : public BoxV
	{
	public:

		/**
		\brief Constructor
		*/
		PX_INLINE ShrunkBoxV() : BoxV()
		{
		}

		//this is used by CCD system
		PX_INLINE ShrunkBoxV(const PxGeometry& geom) : BoxV(geom)
		{
			margin = minExtent * BOX_MARGIN_CCD_RATIO;
			initialiseMarginDif();
		}

		/**
		\brief Constructor

		\param origin Center of the OBB
		\param extent Extents/radii of the obb.
		*/

		PX_FORCE_INLINE ShrunkBoxV(const Ps::aos::Vec3VArg origin, const Ps::aos::Vec3VArg extent) 
			: BoxV(origin, extent)
		{
			//calculate margin
			margin = minExtent * BOX_MARGIN_RATIO;
			initialiseMarginDif();
		}
		
		/**
		\brief Destructor
		*/
		PX_INLINE ~ShrunkBoxV()
		{
		}

		PX_FORCE_INLINE void resetMargin(const PxReal toleranceLength)
		{
			BoxV::resetMargin(toleranceLength);
			margin = PxMin(toleranceLength * BOX_MARGIN_RATIO, margin);
			initialiseMarginDif();
		}

		//! Assignment operator
		PX_INLINE const ShrunkBoxV& operator=(const ShrunkBoxV& other)
		{
			center	= other.center;
			extents	= other.extents;
			margin =  other.margin;
			minMargin = other.minMargin;
			return *this;
		}

		PX_FORCE_INLINE Ps::aos::Vec3V supportPoint(const PxI32 index, Ps::aos::FloatV* marginDif_)const
		{
			using namespace Ps::aos;
			const Vec3V extents_ = V3Sub(extents,  V3Splat(getMargin()));
			const BoolV con = boxVertexTable[index];
			(*marginDif_) = marginDif;
			return V3Sel(con, extents_, V3Neg(extents_));
		}  

		//local space point
		PX_FORCE_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir)const
		{
			using namespace Ps::aos;
		
			const Vec3V zero = V3Zero();
			const Vec3V extents_ = V3Sub(extents,  V3Splat(getMargin()));
			const BoolV comp = V3IsGrtr(dir, zero);
			return V3Sel(comp, extents_, V3Neg(extents_));
		}

		//local space point
		PX_FORCE_INLINE Ps::aos::Vec3V supportLocal(const Ps::aos::Vec3VArg dir,  PxI32& index, Ps::aos::FloatV* marginDif_)const
		{
			using namespace Ps::aos;
		
			const Vec3V zero = V3Zero();
			const Vec3V extents_ = V3Sub(extents,  V3Splat(getMargin()));
			const BoolV comp = V3IsGrtr(dir, zero);
			getIndex(comp, index);
			(*marginDif_) = marginDif; 
			return V3Sel(comp, extents_, V3Neg(extents_));
		}

		PX_FORCE_INLINE Ps::aos::Vec3V supportRelative(const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob, const Ps::aos::PsMatTransformV& aTobT) const
		{
			using namespace Ps::aos;
		
			//transfer dir into the local space of the box
//			const Vec3V dir_ =aTob.rotateInv(dir);//relTra.rotateInv(dir);
			const Vec3V dir_ = aTobT.rotate(dir);//relTra.rotateInv(dir);
			const Vec3V p = supportLocal(dir_);
			return aTob.transform(p);//relTra.transform(p);//V3Add(center, M33MulV3(rot, p));
		}

		PX_FORCE_INLINE Ps::aos::Vec3V supportRelative(	const Ps::aos::Vec3VArg dir, const Ps::aos::PsMatTransformV& aTob,
														const Ps::aos::PsMatTransformV& aTobT, PxI32& index,  Ps::aos::FloatV* marginDif_)const
		{
			using namespace Ps::aos;
	
			//transfer dir into the local space of the box
//			const Vec3V dir_ =aTob.rotateInv(dir);//relTra.rotateInv(dir);
			const Vec3V dir_ = aTobT.rotate(dir);//relTra.rotateInv(dir);
			const Vec3V p = supportLocal(dir_, index, marginDif_);
			return aTob.transform(p);//relTra.transform(p);//V3Add(center, M33MulV3(rot, p));
		}

	private:

		PX_FORCE_INLINE void initialiseMarginDif()
		{
			using namespace Ps::aos;
			//calculate the marginDif
			const PxReal sqMargin = margin * margin;
			const PxReal tempMarginDif = sqrtf(sqMargin * 3.f);
			const PxReal marginDif_ = tempMarginDif - margin;
			marginDif = FLoad(marginDif_);
		}
	};
}	

}

/** @} */
#endif
