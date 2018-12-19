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

#include "foundation/PxIO.h"
#include "PxPhysicsSerialization.h"
#include "NpShape.h"
#include "NpShapeManager.h"
#include "NpConstraint.h"
#include "NpRigidStatic.h"
#include "NpRigidDynamic.h"
#include "NpArticulation.h"
#include "NpArticulationLink.h"
#include "NpArticulationJoint.h"
#include "NpClothFabric.h"
#include "NpCloth.h"
#include "NpAggregate.h"
#include "NpParticleFluid.h"
#include "GuConvexMesh.h"
#include "GuTriangleMesh.h"
#include "GuTriangleMeshBV4.h"
#include "GuTriangleMeshRTree.h"
#include "GuHeightField.h"

using namespace physx;
using namespace Cm;

///////////////////////////////////////////////////////////////////////////////

// PT: the offsets can be different for different templated classes so I need macros here.

#define DefineMetaData_PxActor(x) \
	PX_DEF_BIN_METADATA_ITEM(stream,	x, void,			userData,		PxMetaDataFlag::ePTR)

#define DefineMetaData_NpRigidActorTemplate(x) \
	PX_DEF_BIN_METADATA_ITEM(stream,	x, NpShapeManager,	mShapeManager,	0) \
	PX_DEF_BIN_METADATA_ITEM(stream,	x, PxU32,			mIndex,			0) \

#define DefineMetaData_NpRigidBodyTemplate(x) \
	PX_DEF_BIN_METADATA_ITEM(stream,	x, Scb::Body,		mBody,			0)

///////////////////////////////////////////////////////////////////////////////

static void getBinaryMetaData_PxVec3(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxVec3)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec3,		PxReal,	x,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec3,		PxReal,	y,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec3,		PxReal,	z,	0)
}

static void getBinaryMetaData_PxVec4(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxVec4)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec4,		PxReal,	x,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec4,		PxReal,	y,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec4,		PxReal,	z,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxVec4,		PxReal,	w,	0)
} 

static void getBinaryMetaData_PxQuat(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxQuat)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxQuat,		PxReal,	x,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxQuat,		PxReal,	y,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxQuat,		PxReal,	z,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxQuat,		PxReal,	w,	0)
}

static void getBinaryMetaData_PxBounds3(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxBounds3)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxBounds3,	PxVec3,	minimum,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxBounds3,	PxVec3,	maximum,	0)
}

static void getBinaryMetaData_PxTransform(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxTransform)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxTransform,	PxQuat,	q,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxTransform,	PxVec3,	p,	0)
}

static void getBinaryMetaData_PxMat33(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxMat33)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxMat33,	PxVec3,	column0,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxMat33,	PxVec3,	column1,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxMat33,	PxVec3,	column2,	0)
}

namespace
{
	class ShadowBitMap : public BitMap
	{
	public:
		static void getBinaryMetaData(PxOutputStream& stream_)
		{
			PX_DEF_BIN_METADATA_CLASS(stream_,		ShadowBitMap)
			PX_DEF_BIN_METADATA_ITEM(stream_,		ShadowBitMap, PxU32,		mMap,		PxMetaDataFlag::ePTR)
			PX_DEF_BIN_METADATA_ITEM(stream_,		ShadowBitMap, PxU32,		mWordCount,	0)
			PX_DEF_BIN_METADATA_ITEM(stream_,		ShadowBitMap, Allocator,	mAllocator,	0)
			PX_DEF_BIN_METADATA_ITEMS_AUTO(stream_,	ShadowBitMap, PxU8,			mPadding,	PxMetaDataFlag::ePADDING)

			//------ Extra-data ------

			// mMap
			PX_DEF_BIN_METADATA_EXTRA_ARRAY(stream_,	ShadowBitMap, PxU32, mWordCount, PX_SERIAL_ALIGN, PxMetaDataFlag::eCOUNT_MASK_MSB)
		}
	};
}

static void getBinaryMetaData_BitMap(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_TYPEDEF(stream, Allocator, PxU8)
	ShadowBitMap::getBinaryMetaData(stream);
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	BitMap, ShadowBitMap)
}

static void getBinaryMetaData_PxPlane(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxPlane)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxPlane,	PxVec3,	n,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxPlane,	PxReal,	d,	0)	
}

static void getBinaryMetaData_PxConstraintInvMassScale(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxConstraintInvMassScale)

	PX_DEF_BIN_METADATA_ITEM(stream,	PxConstraintInvMassScale,	PxReal,		linear0,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxConstraintInvMassScale,	PxReal,		angular0,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxConstraintInvMassScale,	PxReal,		linear1,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxConstraintInvMassScale,	PxReal,		angular1,	0)
}

///////////////////////////////////////////////////////////////////////////////

void NpActor::getBinaryMetaData(PxOutputStream& stream)
{
	// 12 bytes
	PX_DEF_BIN_METADATA_CLASS(stream,		NpActor)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpActor, char,				mName,				PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpActor, NpConnectorArray,	mConnectorArray,	PxMetaDataFlag::ePTR)

}

///////////////////////////////////////////////////////////////////////////////

void NpMaterial::getBinaryMetaData(PxOutputStream& stream)
{
// 76 => 72 => 64 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpMaterial)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpMaterial, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpMaterial, RefCountable)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpMaterial, void,				userData,	PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpMaterial, MaterialCore,		mMaterial,	0)
}

///////////////////////////////////////////////////////////////////////////////

void NpConstraint::getBinaryMetaData(PxOutputStream& stream)
{
// 136 => 140 => 144 => 128 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpConstraint)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpConstraint, PxBase)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpConstraint, PxRigidActor,		mActor0,			PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConstraint, PxRigidActor,		mActor1,			PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConstraint, Scb::Constraint,	mConstraint,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConstraint, bool,				mIsDirty,			0)
	PX_DEF_BIN_METADATA_ITEMS_AUTO(stream,	NpConstraint, bool,				mPaddingFromBool,	PxMetaDataFlag::ePADDING)
}

///////////////////////////////////////////////////////////////////////////////

void NpShapeManager::getBinaryMetaData(PxOutputStream& stream)
{
// 8 bytes
	PX_DEF_BIN_METADATA_CLASS(stream,	NpShapeManager)
	PX_DEF_BIN_METADATA_ITEM(stream,	NpShapeManager, PtrTable,	mShapes,			0)
	PX_DEF_BIN_METADATA_ITEM(stream,	NpShapeManager, PtrTable,	mSceneQueryData,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	NpShapeManager, Sq::PruningStructure, mPruningStructure, PxMetaDataFlag::ePTR)
}

///////////////////////////////////////////////////////////////////////////////

void NpShape::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	NpInternalShapeFlags, PxU8)

// 208 => 224 => 208 => 192 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpShape)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpShape, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpShape, RefCountable)

	// PxShape
	PX_DEF_BIN_METADATA_ITEM(stream,		NpShape, void,					userData,			PxMetaDataFlag::ePTR)

	// NpShape
	PX_DEF_BIN_METADATA_ITEM(stream,		NpShape, PxRigidActor,			mActor,				PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpShape, Scb::Shape,			mShape,				0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpShape, char,					mName,				PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpShape, PxI32,					mExclusiveAndActorCount,	0)


	PX_DEF_BIN_METADATA_EXTRA_NAME(stream,	NpShape, mName, 0)
}

///////////////////////////////////////////////////////////////////////////////

void NpRigidStatic::getBinaryMetaData(PxOutputStream& stream)
{
// 124 => 128 => 124 => 108 => 96 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpRigidStatic)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpRigidStatic, PxBase)
//	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpRigidStatic, NpRigidStaticT)	// ### ???
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpRigidStatic, NpActor)

	DefineMetaData_PxActor(NpRigidStatic)
	DefineMetaData_NpRigidActorTemplate(NpRigidStatic)

	// NpRigidStatic
	PX_DEF_BIN_METADATA_ITEM(stream,		NpRigidStatic, Scb::RigidStatic,	mRigidStatic,	0)

	//------ Extra-data ------

	PX_DEF_BIN_METADATA_EXTRA_ITEM(stream,	NpRigidStatic, NpConnectorArray,	mConnectorArray, PX_SERIAL_ALIGN)
	PX_DEF_BIN_METADATA_EXTRA_NAME(stream,	NpRigidStatic, mName, 0)
}

///////////////////////////////////////////////////////////////////////////////

void NpConnector::getBinaryMetaData(PxOutputStream& stream)
{
// 8 bytes
	PX_DEF_BIN_METADATA_CLASS(stream,		NpConnector)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnector, PxU8,		mType,		0)
	PX_DEF_BIN_METADATA_ITEMS_AUTO(stream,	NpConnector, PxU8,		mPadding,	PxMetaDataFlag::ePADDING)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnector, PxBase,	mObject,	PxMetaDataFlag::ePTR)
}

void NpConnectorArray::getBinaryMetaData(PxOutputStream& stream)
{
// 48 bytes
	PX_DEF_BIN_METADATA_CLASS(stream,		NpConnectorArray)
	PX_DEF_BIN_METADATA_ITEMS_AUTO(stream,	NpConnectorArray, NpConnector,	mBuffer,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnectorArray, bool,			mBufferUsed,	0)
	// PT: OMG this is so painful... I can't put the padding explicitly in the template
	{ PxMetaDataEntry tmp = {"char", "mPadding", 1 + PxU32(PX_OFFSET_OF_RT(NpConnectorArray, mBufferUsed)), 3, 3, 0, PxMetaDataFlag::ePADDING, 0}; PX_STORE_METADATA(stream, tmp); }
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnectorArray, NpConnector,	mData,			PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnectorArray, PxU32,		mSize,			0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpConnectorArray, PxU32,		mCapacity,		0)

	//------ Extra-data ------

	PX_DEF_BIN_METADATA_EXTRA_ITEMS(stream,	NpConnectorArray, NpConnector, mBufferUsed, mCapacity, PxMetaDataFlag::eCONTROL_FLIP|PxMetaDataFlag::eCOUNT_MASK_MSB, 0)
}

///////////////////////////////////////////////////////////////////////////////

void NpRigidDynamic::getBinaryMetaData(PxOutputStream& stream)
{
// 368 => 352 => 304 => 288 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpRigidDynamic)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpRigidDynamic, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpRigidDynamic, NpActor)

	DefineMetaData_PxActor(NpRigidDynamic)
	DefineMetaData_NpRigidActorTemplate(NpRigidDynamic)
	DefineMetaData_NpRigidBodyTemplate(NpRigidDynamic)

	// NpRigidDynamic

	//------ Extra-data ------

// Extra data:
// - inline array from shape manager
// - optional constraint array

//	PX_DEF_BIN_METADATA_ITEM(stream,NpRigidDynamic, NpShapeManager,	mShapeManager.mShapes,	0)

/*
		virtual	void				exportExtraData(PxOutputStream& stream)
				{
					mShapeManager.exportExtraData(stream);
					ActorTemplateClass::exportExtraData(stream);
				}
void NpActorTemplate<APIClass, LeafClass>::exportExtraData(PxOutputStream& stream)					
{
	if(mConnectorArray)
	{
		stream.storeBuffer(mConnectorArray, sizeof(NpConnectorArray)
		mConnectorArray->exportExtraData(stream);
	}
}
*/
	PX_DEF_BIN_METADATA_EXTRA_ITEM(stream,	NpRigidDynamic, NpConnectorArray, mConnectorArray, PX_SERIAL_ALIGN)
//### missing inline array data here... only works for "buffered" arrays so far
/*
	Big issue: we can't output the "offset of" the inline array within the class, since the inline array itself is extra-data. But we need to read
	the array itself to know if it's inline or not (the "is buffered" bool). So we need to read from the extra data!
*/

/*
[17:41:39] Gordon Yeoman nvidia: PxsBodyCore need to be 16-byte aligned for spu.  If it is 128-byte aligned then that is a mistake.  Feel free to change it to 16.
*/
	PX_DEF_BIN_METADATA_EXTRA_NAME(stream,	NpRigidDynamic, mName, 0)
}

///////////////////////////////////////////////////////////////////////////////

void NpArticulationLinkArray::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	NpArticulationLinkArray)
	PX_DEF_BIN_METADATA_ITEMS(stream,	NpArticulationLinkArray,	NpArticulationLink,	mBuffer,		PxMetaDataFlag::ePTR, 4)
	PX_DEF_BIN_METADATA_ITEM(stream,	NpArticulationLinkArray,	bool,				mBufferUsed,	0)
	// PT: OMG this is so painful... I can't put the padding explicitely in the template
	{ PxMetaDataEntry tmp = {"char", "mPadding", 1 + PxU32(PX_OFFSET_OF_RT(NpArticulationLinkArray, mBufferUsed)), 3, 3, 0, PxMetaDataFlag::ePADDING, 0}; PX_STORE_METADATA(stream, tmp); }
	PX_DEF_BIN_METADATA_ITEM(stream,	NpArticulationLinkArray,	NpArticulationLink,	mData,		PxMetaDataFlag::ePTR)	// ###
	PX_DEF_BIN_METADATA_ITEM(stream,	NpArticulationLinkArray,	PxU32,				mSize,			0)
	PX_DEF_BIN_METADATA_ITEM(stream,	NpArticulationLinkArray,	PxU32,				mCapacity,		0)

	//------ Extra-data ------

	PX_DEF_BIN_METADATA_EXTRA_ITEMS(stream,	NpArticulationLinkArray, NpArticulationLink, mBufferUsed, mCapacity, PxMetaDataFlag::eCONTROL_FLIP|PxMetaDataFlag::eCOUNT_MASK_MSB|PxMetaDataFlag::ePTR, 0)
}

void NpArticulation::getBinaryMetaData(PxOutputStream& stream)
{
// 92 => 108 => 104 => 116 => 120 => 104 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpArticulation)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpArticulation, PxBase)

	// PxArticulation
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulation, void,						userData,			PxMetaDataFlag::ePTR)

	// NpArticulation
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulation, Scb::Articulation,			mArticulation,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulation, NpArticulationLinkArray,	mArticulationLinks,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulation, NpAggregate,				mAggregate,			PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulation, char,					    mName,	    		PxMetaDataFlag::ePTR)

	PX_DEF_BIN_METADATA_EXTRA_NAME(stream,	NpArticulation, mName, 0)
}

void NpArticulationLink::getBinaryMetaData(PxOutputStream& stream)
{
// 400 (!) => 352 => 336 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpArticulationLink)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpArticulationLink, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpArticulationLink, NpActor)

	DefineMetaData_PxActor(NpArticulationLink)
	DefineMetaData_NpRigidActorTemplate(NpArticulationLink)
	DefineMetaData_NpRigidBodyTemplate(NpArticulationLink)

	// NpArticulationLink
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationLink, NpArticulation,				mRoot,			PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationLink, NpArticulationJoint,		mInboundJoint,	PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationLink, NpArticulationLink,			mParent,		PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationLink, NpArticulationLinkArray,	mChildLinks,	0)
#ifdef EXPLICIT_PADDING_METADATA
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationLink, PxU32,						mPadding,		PxMetaDataFlag::ePADDING)
#endif

	//------ Extra-data ------

	PX_DEF_BIN_METADATA_EXTRA_ITEM(stream,	NpArticulationLink, NpConnectorArray,			mConnectorArray, PX_SERIAL_ALIGN)
	PX_DEF_BIN_METADATA_EXTRA_NAME(stream,	NpArticulationLink, mName, 0)
}

void NpArticulationJoint::getBinaryMetaData(PxOutputStream& stream)
{
// 184 => 200 => 192 => 224 => 208 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpArticulationJoint)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpArticulationJoint, PxBase)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationJoint, Scb::ArticulationJoint,	mJoint,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationJoint, NpArticulationLink,		mParent,	PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpArticulationJoint, NpArticulationLink,		mChild,		PxMetaDataFlag::ePTR)

}

///////////////////////////////////////////////////////////////////////////////

void NpAggregate::getBinaryMetaData(PxOutputStream& stream)
{
// 36 => 56 => 40 bytes
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpAggregate)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpAggregate, PxBase)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpAggregate, Scb::Aggregate,	mAggregate,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpAggregate, PxU32,				mNbActors,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpAggregate, PxActor,			mActors,		PxMetaDataFlag::ePTR)

	//------ Extra-data ------

	// mActors
	PX_DEF_BIN_METADATA_EXTRA_ITEMS(stream,	NpAggregate, PxActor,			mActors,		mNbActors, PxMetaDataFlag::ePTR, PX_SERIAL_ALIGN)
}

///////////////////////////////////////////////////////////////////////////////
#if PX_USE_CLOTH_API
void NpClothFabric::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpClothFabric)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpClothFabric, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpClothFabric, RefCountable)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpClothFabric, Sc::ClothFabricCore, mFabric, 0)		
}

///////////////////////////////////////////////////////////////////////////////

void NpCloth::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpCloth)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpCloth, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpCloth, NpActor)

	DefineMetaData_PxActor(NpCloth)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpCloth, Scb::Cloth,	mCloth, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,		NpCloth, NpClothFabric, mClothFabric,	PxMetaDataFlag::ePTR)
	PX_DEF_BIN_METADATA_ITEMS(stream,		NpCloth, PxU8,			mParticleData,	PxMetaDataFlag::ePADDING, sizeof(NpClothParticleData))
}

#endif // PX_USE_CLOTH_API

///////////////////////////////////////////

#if PX_USE_PARTICLE_SYSTEM_API

void NpParticleSystem::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpParticleSystem)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpParticleSystem, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpParticleSystem, NpActor)

	DefineMetaData_PxActor(NpParticleSystem)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpParticleFluid, Scb::ParticleSystem, mParticleSystem, 0)
}

///////////////////////////////////////////////////////////////////////////////

void NpParticleFluid::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_VCLASS(stream,		NpParticleFluid)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpParticleFluid, PxBase)
	PX_DEF_BIN_METADATA_BASE_CLASS(stream,	NpParticleFluid, NpActor)

	DefineMetaData_PxActor(NpParticleFluid)

	PX_DEF_BIN_METADATA_ITEM(stream,		NpParticleFluid, Scb::ParticleSystem, mParticleSystem, 0)	
}

#endif // PX_USE_PARTICLE_SYSTEM_API

///////////////////////////////////////////////////////////////////////////////

static void getBinaryMetaData_PxMeshScale(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_CLASS(stream,	PxMeshScale)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxMeshScale,		PxVec3,	scale,		0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxMeshScale,		PxQuat,	rotation,	0)
}

#if PX_USE_PARTICLE_SYSTEM_API

static void getBinaryMetaData_ParticleSystemParameter(PxOutputStream& stream)
{	
	PX_DEF_BIN_METADATA_CLASS(stream,	Pt::ParticleSystemParameter)

	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, restParticleDistance, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, kernelRadiusMultiplier, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, viscosity, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, surfaceTension, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, fadeInTime, 0)	
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxU32,	flags, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxU32,	packetSizeMultiplierLog2, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, restitution, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, dynamicFriction, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, staticFriction, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, restDensity, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, damping, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, stiffness, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, maxMotionDistance, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, restOffset, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxReal, contactOffset, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxPlane,projectionPlane, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxU16,	particleReadDataFlags, 0)
	PX_DEF_BIN_METADATA_ITEM(stream,	Pt::ParticleSystemParameter, PxU32,	noiseCounter, 0)	
}

#endif // PX_USE_PARTICLE_SYSTEM_API

///////////////////////////////////////////////////////////////////////////////
namespace physx
{
void getBinaryMetaData_PxBase(PxOutputStream& stream)
{
	// 8 bytes
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxBaseFlags,	PxU16)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxType,			PxU16)
	PX_DEF_BIN_METADATA_VCLASS(stream,	PxBase)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxBase,			PxType,			mConcreteType,	0)
	PX_DEF_BIN_METADATA_ITEM(stream,	PxBase,			PxBaseFlags,	mBaseFlags,	0)
}
}
void RefCountable::getBinaryMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_VCLASS(stream,	RefCountable)
	PX_DEF_BIN_METADATA_ITEM(stream,	RefCountable,	PxI32,			mRefCount,		0)
}

static void getFoundationMetaData(PxOutputStream& stream)
{
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxU8,	char)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxI8,	char)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxU16,	short)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxI16,	short)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxU32,	int)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxI32,	int)
	PX_DEF_BIN_METADATA_TYPEDEF(stream,	PxReal,	float)

	getBinaryMetaData_PxVec3(stream);
	getBinaryMetaData_PxVec4(stream);
	getBinaryMetaData_PxQuat(stream);
	getBinaryMetaData_PxBounds3(stream);
	getBinaryMetaData_PxTransform(stream);
	getBinaryMetaData_PxMat33(stream);
	getBinaryMetaData_BitMap(stream);
	Cm::PtrTable::getBinaryMetaData(stream);
	getBinaryMetaData_PxPlane(stream);
	getBinaryMetaData_PxConstraintInvMassScale(stream);

	getBinaryMetaData_PxBase(stream);
	RefCountable::getBinaryMetaData(stream);
}

///////////////////////////////////////////////////////////////////////////////

void PxGetPhysicsBinaryMetaData(PxOutputStream& stream)
{
	getFoundationMetaData(stream);
	
	getBinaryMetaData_PxMeshScale(stream);
#if PX_USE_PARTICLE_SYSTEM_API
	getBinaryMetaData_ParticleSystemParameter(stream);
#endif

	MaterialIndicesStruct::getBinaryMetaData(stream);
	Gu::GeometryUnion::getBinaryMetaData(stream);
	Gu::ConvexMesh::getBinaryMetaData(stream);
	Gu::TriangleMesh::getBinaryMetaData(stream);
	Gu::RTreeTriangleMesh::getBinaryMetaData(stream);
	Gu::BV4TriangleMesh::getBinaryMetaData(stream);
	Gu::HeightField::getBinaryMetaData(stream);

	Sc::ActorCore::getBinaryMetaData(stream);
	Sc::RigidCore::getBinaryMetaData(stream);
	Sc::StaticCore::getBinaryMetaData(stream);
	Sc::BodyCore::getBinaryMetaData(stream);
	Sc::MaterialCore::getBinaryMetaData(stream);
	Sc::ShapeCore::getBinaryMetaData(stream);
	Sc::ConstraintCore::getBinaryMetaData(stream);
	Sc::ArticulationCore::getBinaryMetaData(stream);
	Sc::ArticulationJointCore::getBinaryMetaData(stream);
#if PX_USE_CLOTH_API
	Sc::ClothFabricCore::getBinaryMetaData(stream);
	Sc::ClothCore::getBinaryMetaData(stream);
#endif
#if PX_USE_PARTICLE_SYSTEM_API
	Sc::ParticleSystemCore::getBinaryMetaData(stream);
#endif

	Scb::Base::getBinaryMetaData(stream);
	Scb::Actor::getBinaryMetaData(stream);
	Scb::RigidObject::getBinaryMetaData(stream);
	Scb::RigidStatic::getBinaryMetaData(stream);
	Scb::Body::getBinaryMetaData(stream);
	Scb::Shape::getBinaryMetaData(stream);
	Scb::Constraint::getBinaryMetaData(stream);
	Scb::Articulation::getBinaryMetaData(stream);
	Scb::ArticulationJoint::getBinaryMetaData(stream);
	Scb::Aggregate::getBinaryMetaData(stream);
#if PX_USE_CLOTH_API
	Scb::Cloth::getBinaryMetaData(stream);
#endif
#if PX_USE_PARTICLE_SYSTEM_API
	Scb::ParticleSystem::getBinaryMetaData(stream);
#endif

	NpConnector::getBinaryMetaData(stream);
	NpConnectorArray::getBinaryMetaData(stream);
	NpActor::getBinaryMetaData(stream);
	NpMaterial::getBinaryMetaData(stream);			// NP_MATERIAL
	NpRigidDynamic::getBinaryMetaData(stream);		// NP_RIGID_DYNAMIC
	NpRigidStatic::getBinaryMetaData(stream);			// NP_RIGID_STATIC
	NpShape::getBinaryMetaData(stream);				// NP_SHAPE
	NpConstraint::getBinaryMetaData(stream);			// NP_CONSTRAINT
	NpArticulation::getBinaryMetaData(stream);		// NP_ARTICULATION
	NpArticulationLink::getBinaryMetaData(stream);	// NP_ARTICULATION_LINK
	NpArticulationJoint::getBinaryMetaData(stream);	// NP_ARTICULATION_JOINT
	NpArticulationLinkArray::getBinaryMetaData(stream);
	NpShapeManager::getBinaryMetaData(stream);
	NpAggregate::getBinaryMetaData(stream);			// NP_AGGREGATE	
#if PX_USE_CLOTH_API
	NpClothFabric::getBinaryMetaData(stream);
	NpCloth::getBinaryMetaData(stream);
#endif

#if PX_USE_PARTICLE_SYSTEM_API
	NpParticleSystem::getBinaryMetaData(stream);
	NpParticleFluid::getBinaryMetaData(stream);
#endif
}
