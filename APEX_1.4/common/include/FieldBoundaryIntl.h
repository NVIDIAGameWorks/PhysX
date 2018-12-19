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



#ifndef FIELD_BOUNDARY_INTL_H
#define FIELD_BOUNDARY_INTL_H

#include "InplaceTypes.h"

namespace nvidia
{
namespace apex
{


struct FieldShapeTypeIntl
{
	enum Enum
	{
		NONE = 0,
		SPHERE,
		BOX,
		CAPSULE,

		FORCE_DWORD = 0xFFFFFFFFu
	};
};

//struct FieldShapeDescIntl
//dimensions for
//SPHERE: x = radius
//BOX:    (x,y,z) = 1/2 size
//CAPUSE: x = radius, y = height
#define INPLACE_TYPE_STRUCT_NAME FieldShapeDescIntl
#define INPLACE_TYPE_STRUCT_FIELDS \
	INPLACE_TYPE_FIELD(InplaceEnum<FieldShapeTypeIntl::Enum>,	type) \
	INPLACE_TYPE_FIELD(PxTransform,				worldToShape) \
	INPLACE_TYPE_FIELD(PxVec3,						dimensions) \
	INPLACE_TYPE_FIELD(float,						weight)
#include INPLACE_TYPE_BUILD()


#ifndef __CUDACC__

struct FieldBoundaryDescIntl
{
#if PX_PHYSICS_VERSION_MAJOR == 3
	PxFilterData	boundaryFilterData;
#endif
};

class FieldBoundaryIntl
{
public:
	virtual bool updateFieldBoundary(physx::Array<FieldShapeDescIntl>& shapes) = 0;

protected:
	virtual ~FieldBoundaryIntl() {}
};
#endif

}
} // end namespace nvidia::apex

#endif // #ifndef FIELD_BOUNDARY_INTL_H
