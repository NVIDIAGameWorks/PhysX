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

// suppress LNK4221
#include "foundation/PxPreprocessor.h"
PX_DUMMY_SYMBOL

#if PX_SUPPORT_PVD

#include "foundation/PxSimpleTypes.h"
#include "foundation/Px.h"

#include "PxMetaDataObjects.h"
#include "PxPvdDataStream.h"
#include "PxScene.h"
#include "ScBodyCore.h"
#include "PvdMetaDataExtensions.h"
#include "PvdMetaDataPropertyVisitor.h"
#include "PvdMetaDataDefineProperties.h"
#include "PvdMetaDataBindingData.h"
#include "PxRigidDynamic.h"
#include "PxArticulation.h"
#include "PxArticulationLink.h"
#include "NpScene.h"
#include "NpPhysics.h"

#include "gpu/PxParticleGpu.h"
#include "PvdTypeNames.h"
#include "PvdMetaDataPvdBinding.h"

using namespace physx;
using namespace Sc;
using namespace Vd;
using namespace Sq;

namespace physx
{
namespace Vd
{

struct NameValuePair
{
	const char* mName;
	PxU32 mValue;
};

static const NameValuePair g_physx_Sq_SceneQueryID__EnumConversion[] = {
	{ "QUERY_RAYCAST_ANY_OBJECT", PxU32(QueryID::QUERY_RAYCAST_ANY_OBJECT) },
	{ "QUERY_RAYCAST_CLOSEST_OBJECT", PxU32(QueryID::QUERY_RAYCAST_CLOSEST_OBJECT) },
	{ "QUERY_RAYCAST_ALL_OBJECTS", PxU32(QueryID::QUERY_RAYCAST_ALL_OBJECTS) },
	{ "QUERY_OVERLAP_SPHERE_ALL_OBJECTS", PxU32(QueryID::QUERY_OVERLAP_SPHERE_ALL_OBJECTS) },
	{ "QUERY_OVERLAP_AABB_ALL_OBJECTS", PxU32(QueryID::QUERY_OVERLAP_AABB_ALL_OBJECTS) },
	{ "QUERY_OVERLAP_OBB_ALL_OBJECTS", PxU32(QueryID::QUERY_OVERLAP_OBB_ALL_OBJECTS) },
	{ "QUERY_OVERLAP_CAPSULE_ALL_OBJECTS", PxU32(QueryID::QUERY_OVERLAP_CAPSULE_ALL_OBJECTS) },
	{ "QUERY_OVERLAP_CONVEX_ALL_OBJECTS", PxU32(QueryID::QUERY_OVERLAP_CONVEX_ALL_OBJECTS) },
	{ "QUERY_LINEAR_OBB_SWEEP_CLOSEST_OBJECT", PxU32(QueryID::QUERY_LINEAR_OBB_SWEEP_CLOSEST_OBJECT) },
	{ "QUERY_LINEAR_CAPSULE_SWEEP_CLOSEST_OBJECT", PxU32(QueryID::QUERY_LINEAR_CAPSULE_SWEEP_CLOSEST_OBJECT) },
	{ "QUERY_LINEAR_CONVEX_SWEEP_CLOSEST_OBJECT", PxU32(QueryID::QUERY_LINEAR_CONVEX_SWEEP_CLOSEST_OBJECT) },
	{ NULL, 0 }
};

struct SceneQueryIDConvertor
{
	const NameValuePair* NameConversion;
	SceneQueryIDConvertor() : NameConversion(g_physx_Sq_SceneQueryID__EnumConversion)
	{
	}
};

PvdMetaDataBinding::PvdMetaDataBinding() : mBindingData(PX_NEW(PvdMetaDataBindingData)())
{
}

PvdMetaDataBinding::~PvdMetaDataBinding()
{
	for(OwnerActorsMap::Iterator iter = mBindingData->mOwnerActorsMap.getIterator(); !iter.done(); iter++)
	{
		iter->second->~OwnerActorsValueType();
		PX_FREE(iter->second);
	}

	PX_DELETE(mBindingData);
	mBindingData = NULL;
}

template <typename TDataType, typename TValueType, typename TClassType>
static inline void definePropertyStruct(PvdDataStream& inStream, const char* pushName = NULL)
{
	PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
	PvdClassInfoValueStructDefine definitionObj(helper);
	bool doPush = pushName && *pushName;
	if(doPush)
		definitionObj.pushName(pushName);
	visitAllPvdProperties<TDataType>(definitionObj);
	if(doPush)
		definitionObj.popName();
	helper.addPropertyMessage(getPvdNamespacedNameForType<TClassType>(), getPvdNamespacedNameForType<TValueType>(), sizeof(TValueType));
}

template <typename TDataType>
static inline void createClassAndDefineProperties(PvdDataStream& inStream)
{
	inStream.createClass<TDataType>();
	PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
	PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<TDataType>());
	visitAllPvdProperties<TDataType>(definitionObj);
}

template <typename TDataType, typename TParentType>
static inline void createClassDeriveAndDefineProperties(PvdDataStream& inStream)
{
	inStream.createClass<TDataType>();
	inStream.deriveClass<TParentType, TDataType>();
	PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
	PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<TDataType>());
	visitInstancePvdProperties<TDataType>(definitionObj);
}

template <typename TDataType, typename TConvertSrc, typename TConvertData>
static inline void defineProperty(PvdDataStream& inStream, const char* inPropertyName, const char* semantic)
{
	PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
	// PxEnumTraits< TValueType > filterFlagsEnum;
	TConvertSrc filterFlagsEnum;
	const TConvertData* convertor = filterFlagsEnum.NameConversion;

	for(; convertor->mName != NULL; ++convertor)
		helper.addNamedValue(convertor->mName, convertor->mValue);

	inStream.createProperty<TDataType, PxU32>(inPropertyName, semantic, PropertyType::Scalar, helper.getNamedValues());
	helper.clearNamedValues();
}

template <typename TDataType, typename TConvertSrc, typename TConvertData>
static inline void definePropertyFlags(PvdDataStream& inStream, const char* inPropertyName)
{
	defineProperty<TDataType, TConvertSrc, TConvertData>(inStream, inPropertyName, "Bitflag");
}
template <typename TDataType, typename TConvertSrc, typename TConvertData>
static inline void definePropertyEnums(PvdDataStream& inStream, const char* inPropertyName)
{
	defineProperty<TDataType, TConvertSrc, TConvertData>(inStream, inPropertyName, "Enumeration Value");
}

static PX_FORCE_INLINE void registerPvdRaycast(PvdDataStream& inStream)
{
	inStream.createClass<PvdRaycast>();
	definePropertyEnums<PvdRaycast, SceneQueryIDConvertor, NameValuePair>(inStream, "type");
	inStream.createProperty<PvdRaycast, PxFilterData>("filterData");
	definePropertyFlags<PvdRaycast, PxEnumTraits<physx::PxQueryFlag::Enum>, PxU32ToName>(inStream, "filterFlags");
	inStream.createProperty<PvdRaycast, PxVec3>("origin");
	inStream.createProperty<PvdRaycast, PxVec3>("unitDir");
	inStream.createProperty<PvdRaycast, PxF32>("distance");
	inStream.createProperty<PvdRaycast, String>("hits_arrayName");
	inStream.createProperty<PvdRaycast, PxU32>("hits_baseIndex");
	inStream.createProperty<PvdRaycast, PxU32>("hits_count");
}

static PX_FORCE_INLINE void registerPvdSweep(PvdDataStream& inStream)
{
	inStream.createClass<PvdSweep>();
	definePropertyEnums<PvdSweep, SceneQueryIDConvertor, NameValuePair>(inStream, "type");
	definePropertyFlags<PvdSweep, PxEnumTraits<physx::PxQueryFlag::Enum>, PxU32ToName>(inStream, "filterFlags");
	inStream.createProperty<PvdSweep, PxVec3>("unitDir");
	inStream.createProperty<PvdSweep, PxF32>("distance");
	inStream.createProperty<PvdSweep, String>("geom_arrayName");
	inStream.createProperty<PvdSweep, PxU32>("geom_baseIndex");
	inStream.createProperty<PvdSweep, PxU32>("geom_count");
	inStream.createProperty<PvdSweep, String>("pose_arrayName");
	inStream.createProperty<PvdSweep, PxU32>("pose_baseIndex");
	inStream.createProperty<PvdSweep, PxU32>("pose_count");
	inStream.createProperty<PvdSweep, String>("filterData_arrayName");
	inStream.createProperty<PvdSweep, PxU32>("filterData_baseIndex");
	inStream.createProperty<PvdSweep, PxU32>("filterData_count");
	inStream.createProperty<PvdSweep, String>("hits_arrayName");
	inStream.createProperty<PvdSweep, PxU32>("hits_baseIndex");
	inStream.createProperty<PvdSweep, PxU32>("hits_count");
}

static PX_FORCE_INLINE void registerPvdOverlap(PvdDataStream& inStream)
{
	inStream.createClass<PvdOverlap>();
	definePropertyEnums<PvdOverlap, SceneQueryIDConvertor, NameValuePair>(inStream, "type");
	inStream.createProperty<PvdOverlap, PxFilterData>("filterData");
	definePropertyFlags<PvdOverlap, PxEnumTraits<physx::PxQueryFlag::Enum>, PxU32ToName>(inStream, "filterFlags");
	inStream.createProperty<PvdOverlap, PxTransform>("pose");
	inStream.createProperty<PvdOverlap, String>("geom_arrayName");
	inStream.createProperty<PvdOverlap, PxU32>("geom_baseIndex");
	inStream.createProperty<PvdOverlap, PxU32>("geom_count");
	inStream.createProperty<PvdOverlap, String>("hits_arrayName");
	inStream.createProperty<PvdOverlap, PxU32>("hits_baseIndex");
	inStream.createProperty<PvdOverlap, PxU32>("hits_count");
}

static PX_FORCE_INLINE void registerPvdSqHit(PvdDataStream& inStream)
{
	inStream.createClass<PvdSqHit>();
	inStream.createProperty<PvdSqHit, ObjectRef>("Shape");
	inStream.createProperty<PvdSqHit, ObjectRef>("Actor");
	inStream.createProperty<PvdSqHit, PxU32>("FaceIndex");
	definePropertyFlags<PvdSqHit, PxEnumTraits<physx::PxHitFlag::Enum>, PxU32ToName>(inStream, "Flags");
	inStream.createProperty<PvdSqHit, PxVec3>("Impact");
	inStream.createProperty<PvdSqHit, PxVec3>("Normal");
	inStream.createProperty<PvdSqHit, PxF32>("Distance");
	inStream.createProperty<PvdSqHit, PxF32>("U");
	inStream.createProperty<PvdSqHit, PxF32>("V");
}

void PvdMetaDataBinding::registerSDKProperties(PvdDataStream& inStream)
{
	if (inStream.isClassExist<PxPhysics>())
		return;
	// PxPhysics
	{
		inStream.createClass<PxPhysics>();
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<PxPhysics>());
		helper.pushName("TolerancesScale");
		visitAllPvdProperties<PxTolerancesScale>(definitionObj);
		helper.popName();
		inStream.createProperty<PxPhysics, ObjectRef>("Scenes", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("SharedShapes", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("Materials", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("HeightFields", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("ConvexMeshes", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("TriangleMeshes", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, ObjectRef>("ClothFabrics", "children", PropertyType::Array);
		inStream.createProperty<PxPhysics, PxU32>("Version.Major");
		inStream.createProperty<PxPhysics, PxU32>("Version.Minor");
		inStream.createProperty<PxPhysics, PxU32>("Version.Bugfix");
		inStream.createProperty<PxPhysics, String>("Version.Build");
		definePropertyStruct<PxTolerancesScale, PxTolerancesScaleGeneratedValues, PxPhysics>(inStream, "TolerancesScale");
	}
	{ // PxGeometry
		inStream.createClass<PxGeometry>();
		inStream.createProperty<PxGeometry, ObjectRef>("Shape", "parents", PropertyType::Scalar);
	}
	{ // PxBoxGeometry
		createClassDeriveAndDefineProperties<PxBoxGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxBoxGeometry, PxBoxGeometryGeneratedValues, PxBoxGeometry>(inStream);
	}
	{ // PxSphereGeometry
		createClassDeriveAndDefineProperties<PxSphereGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxSphereGeometry, PxSphereGeometryGeneratedValues, PxSphereGeometry>(inStream);
	}
	{ // PxCapsuleGeometry
		createClassDeriveAndDefineProperties<PxCapsuleGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxCapsuleGeometry, PxCapsuleGeometryGeneratedValues, PxCapsuleGeometry>(inStream);
	}
	{ // PxPlaneGeometry
		createClassDeriveAndDefineProperties<PxPlaneGeometry, PxGeometry>(inStream);
	}
	{ // PxConvexMeshGeometry
		createClassDeriveAndDefineProperties<PxConvexMeshGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxConvexMeshGeometry, PxConvexMeshGeometryGeneratedValues, PxConvexMeshGeometry>(inStream);
	}
	{ // PxTriangleMeshGeometry
		createClassDeriveAndDefineProperties<PxTriangleMeshGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxTriangleMeshGeometry, PxTriangleMeshGeometryGeneratedValues, PxTriangleMeshGeometry>(inStream);
	}
	{ // PxHeightFieldGeometry
		createClassDeriveAndDefineProperties<PxHeightFieldGeometry, PxGeometry>(inStream);
		definePropertyStruct<PxHeightFieldGeometry, PxHeightFieldGeometryGeneratedValues, PxHeightFieldGeometry>(inStream);
	}

	// PxScene
	{
		// PT: TODO: why inline this for PvdContact but do PvdRaycast/etc in separate functions?
		{ // contact information
			inStream.createClass<PvdContact>();
			inStream.createProperty<PvdContact, PxVec3>("Point");
			inStream.createProperty<PvdContact, PxVec3>("Axis");
			inStream.createProperty<PvdContact, ObjectRef>("Shapes[0]");
			inStream.createProperty<PvdContact, ObjectRef>("Shapes[1]");
			inStream.createProperty<PvdContact, PxF32>("Separation");
			inStream.createProperty<PvdContact, PxF32>("NormalForce");
			inStream.createProperty<PvdContact, PxU32>("InternalFaceIndex[0]");
			inStream.createProperty<PvdContact, PxU32>("InternalFaceIndex[1]");
			inStream.createProperty<PvdContact, bool>("NormalForceValid");
		}

		registerPvdSqHit(inStream);
		registerPvdRaycast(inStream);
		registerPvdSweep(inStream);
		registerPvdOverlap(inStream);

		inStream.createClass<PxScene>();
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<PxScene>());
		visitAllPvdProperties<PxSceneDesc>(definitionObj);
		helper.pushName("SimulationStatistics");
		visitAllPvdProperties<PxSimulationStatistics>(definitionObj);
		helper.popName();
		inStream.createProperty<PxScene, ObjectRef>("Physics", "parents", PropertyType::Scalar);
		inStream.createProperty<PxScene, PxU32>("Timestamp");
		inStream.createProperty<PxScene, PxReal>("SimulateElapsedTime");
		definePropertyStruct<PxSceneDesc, PxSceneDescGeneratedValues, PxScene>(inStream);
		definePropertyStruct<PxSimulationStatistics, PxSimulationStatisticsGeneratedValues, PxScene>(inStream, "SimulationStatistics");
		inStream.createProperty<PxScene, PvdContact>("Contacts", "", PropertyType::Array);

		inStream.createProperty<PxScene, PvdOverlap>("SceneQueries.Overlaps", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdSweep>("SceneQueries.Sweeps", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdSqHit>("SceneQueries.Hits", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdRaycast>("SceneQueries.Raycasts", "", PropertyType::Array);
		inStream.createProperty<PxScene, PxTransform>("SceneQueries.PoseList", "", PropertyType::Array);
		inStream.createProperty<PxScene, PxFilterData>("SceneQueries.FilterDataList", "", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("SceneQueries.GeometryList", "", PropertyType::Array);

		inStream.createProperty<PxScene, PvdOverlap>("BatchedQueries.Overlaps", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdSweep>("BatchedQueries.Sweeps", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdSqHit>("BatchedQueries.Hits", "", PropertyType::Array);
		inStream.createProperty<PxScene, PvdRaycast>("BatchedQueries.Raycasts", "", PropertyType::Array);
		inStream.createProperty<PxScene, PxTransform>("BatchedQueries.PoseList", "", PropertyType::Array);
		inStream.createProperty<PxScene, PxFilterData>("BatchedQueries.FilterDataList", "", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("BatchedQueries.GeometryList", "", PropertyType::Array);

		inStream.createProperty<PxScene, ObjectRef>("RigidStatics", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("RigidDynamics", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("Articulations", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("ParticleSystems", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("ParticleFluids", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("Cloths", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("Joints", "children", PropertyType::Array);
		inStream.createProperty<PxScene, ObjectRef>("Aggregates", "children", PropertyType::Array);
	}
	// PxMaterial
	{
		createClassAndDefineProperties<PxMaterial>(inStream);
		definePropertyStruct<PxMaterial, PxMaterialGeneratedValues, PxMaterial>(inStream);
		inStream.createProperty<PxMaterial, ObjectRef>("Physics", "parents", PropertyType::Scalar);
	}
	// PxHeightField
	{
		{
			inStream.createClass<PxHeightFieldSample>();
			inStream.createProperty<PxHeightFieldSample, PxU16>("Height");
			inStream.createProperty<PxHeightFieldSample, PxU8>("MaterialIndex[0]");
			inStream.createProperty<PxHeightFieldSample, PxU8>("MaterialIndex[1]");
		}

		inStream.createClass<PxHeightField>();
		// It is important the PVD fields match the RepX fields, so this has
		// to be hand coded.
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<PxHeightField>());
		visitAllPvdProperties<PxHeightFieldDesc>(definitionObj);
		inStream.createProperty<PxHeightField, PxHeightFieldSample>("Samples", "", PropertyType::Array);
		inStream.createProperty<PxHeightField, ObjectRef>("Physics", "parents", PropertyType::Scalar);
		definePropertyStruct<PxHeightFieldDesc, PxHeightFieldDescGeneratedValues, PxHeightField>(inStream);
	}
	// PxConvexMesh
	{
		{ // hull polygon data.
			inStream.createClass<PvdHullPolygonData>();
			inStream.createProperty<PvdHullPolygonData, PxU16>("NumVertices");
			inStream.createProperty<PvdHullPolygonData, PxU16>("IndexBase");
		}
		inStream.createClass<PxConvexMesh>();
		inStream.createProperty<PxConvexMesh, PxF32>("Mass");
		inStream.createProperty<PxConvexMesh, PxMat33>("LocalInertia");
		inStream.createProperty<PxConvexMesh, PxVec3>("LocalCenterOfMass");
		inStream.createProperty<PxConvexMesh, PxVec3>("Points", "", PropertyType::Array);
		inStream.createProperty<PxConvexMesh, PvdHullPolygonData>("HullPolygons", "", PropertyType::Array);
		inStream.createProperty<PxConvexMesh, PxU8>("PolygonIndexes", "", PropertyType::Array);
		inStream.createProperty<PxConvexMesh, ObjectRef>("Physics", "parents", PropertyType::Scalar);
	}
	// PxTriangleMesh
	{
		inStream.createClass<PxTriangleMesh>();
		inStream.createProperty<PxTriangleMesh, PxVec3>("Points", "", PropertyType::Array);
		inStream.createProperty<PxTriangleMesh, PxU32>("NbTriangles", "", PropertyType::Scalar);
		inStream.createProperty<PxTriangleMesh, PxU32>("Triangles", "", PropertyType::Array);
		inStream.createProperty<PxTriangleMesh, PxU16>("MaterialIndices", "", PropertyType::Array);
		inStream.createProperty<PxTriangleMesh, ObjectRef>("Physics", "parents", PropertyType::Scalar);
	}
	{ // PxShape
		createClassAndDefineProperties<PxShape>(inStream);
		definePropertyStruct<PxShape, PxShapeGeneratedValues, PxShape>(inStream);
		inStream.createProperty<PxShape, ObjectRef>("Geometry", "children");
		inStream.createProperty<PxShape, ObjectRef>("Materials", "children", PropertyType::Array);
		inStream.createProperty<PxShape, ObjectRef>("Actor", "parents");
	}
	// PxActor
	{
		createClassAndDefineProperties<PxActor>(inStream);
		inStream.createProperty<PxActor, ObjectRef>("Scene", "parents");
	}
	// PxRigidActor
	{
		createClassDeriveAndDefineProperties<PxRigidActor, PxActor>(inStream);
		inStream.createProperty<PxRigidActor, ObjectRef>("Shapes", "children", PropertyType::Array);
		inStream.createProperty<PxRigidActor, ObjectRef>("Joints", "children", PropertyType::Array);
	}
	// PxRigidStatic
	{
		createClassDeriveAndDefineProperties<PxRigidStatic, PxRigidActor>(inStream);
		definePropertyStruct<PxRigidStatic, PxRigidStaticGeneratedValues, PxRigidStatic>(inStream);
	}
	{ // PxRigidBody
		createClassDeriveAndDefineProperties<PxRigidBody, PxRigidActor>(inStream);
	}
	// PxRigidDynamic
	{
		createClassDeriveAndDefineProperties<PxRigidDynamic, PxRigidBody>(inStream);
		// If anyone adds a 'getKinematicTarget' to PxRigidDynamic you can remove the line
		// below (after the code generator has run).
		inStream.createProperty<PxRigidDynamic, PxTransform>("KinematicTarget");
		definePropertyStruct<PxRigidDynamic, PxRigidDynamicGeneratedValues, PxRigidDynamic>(inStream);
		// Manually define the update struct.
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		/*struct PxRigidDynamicUpdateBlock
		{
		    Transform	GlobalPose;
		    Float3		LinearVelocity;
		    Float3		AngularVelocity;
		    PxU8		IsSleeping;
		    PxU8		padding[3];
		};
		*/
		helper.pushName("GlobalPose");
		helper.addPropertyMessageArg<PxTransform>(PX_OFFSET_OF_RT(PxRigidDynamicUpdateBlock, GlobalPose));
		helper.popName();
		helper.pushName("LinearVelocity");
		helper.addPropertyMessageArg<PxVec3>(PX_OFFSET_OF_RT(PxRigidDynamicUpdateBlock, LinearVelocity));
		helper.popName();
		helper.pushName("AngularVelocity");
		helper.addPropertyMessageArg<PxVec3>(PX_OFFSET_OF_RT(PxRigidDynamicUpdateBlock, AngularVelocity));
		helper.popName();
		helper.pushName("IsSleeping");
		helper.addPropertyMessageArg<bool>(PX_OFFSET_OF_RT(PxRigidDynamicUpdateBlock, IsSleeping));
		helper.popName();
		helper.addPropertyMessage<PxRigidDynamic, PxRigidDynamicUpdateBlock>();
	}
	{ // PxArticulation
		createClassAndDefineProperties<PxArticulation>(inStream);
		inStream.createProperty<PxArticulation, ObjectRef>("Scene", "parents");
		inStream.createProperty<PxArticulation, ObjectRef>("Links", "children", PropertyType::Array);
		definePropertyStruct<PxArticulation, PxArticulationGeneratedValues, PxArticulation>(inStream);
	}
	{ // PxArticulationLink
		createClassDeriveAndDefineProperties<PxArticulationLink, PxRigidBody>(inStream);
		inStream.createProperty<PxArticulationLink, ObjectRef>("Parent", "parents");
		inStream.createProperty<PxArticulationLink, ObjectRef>("Links", "children", PropertyType::Array);
		inStream.createProperty<PxArticulationLink, ObjectRef>("InboundJoint", "children");
		definePropertyStruct<PxArticulationLink, PxArticulationLinkGeneratedValues, PxArticulationLink>(inStream);

		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		/*struct PxArticulationLinkUpdateBlock
		{
		    Transform	GlobalPose;
		    Float3		LinearVelocity;
		    Float3		AngularVelocity;
		};
		*/
		helper.pushName("GlobalPose");
		helper.addPropertyMessageArg<PxTransform>(PX_OFFSET_OF(PxArticulationLinkUpdateBlock, GlobalPose));
		helper.popName();
		helper.pushName("LinearVelocity");
		helper.addPropertyMessageArg<PxVec3>(PX_OFFSET_OF(PxArticulationLinkUpdateBlock, LinearVelocity));
		helper.popName();
		helper.pushName("AngularVelocity");
		helper.addPropertyMessageArg<PxVec3>(PX_OFFSET_OF(PxArticulationLinkUpdateBlock, AngularVelocity));
		helper.popName();
		helper.addPropertyMessage<PxArticulationLink, PxArticulationLinkUpdateBlock>();
	}
	{ // PxArticulationJoint
		createClassAndDefineProperties<PxArticulationJoint>(inStream);
		inStream.createProperty<PxArticulationJoint, ObjectRef>("Link", "parents");
		definePropertyStruct<PxArticulationJoint, PxArticulationJointGeneratedValues, PxArticulationJoint>(inStream);
	}
	{ // PxConstraint
		createClassAndDefineProperties<PxConstraint>(inStream);
		definePropertyStruct<PxConstraint, PxConstraintGeneratedValues, PxConstraint>(inStream);
	}
#if PX_USE_PARTICLE_SYSTEM_API
	{ // PxParticleBase
		createClassDeriveAndDefineProperties<PxParticleBase, PxActor>(inStream);
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<PxParticleBase>());
		visitParticleSystemBufferProperties(makePvdPropertyFilter(definitionObj));
	}
	{ // PxParticleSystem
		createClassDeriveAndDefineProperties<PxParticleSystem, PxParticleBase>(inStream);
		inStream.createProperty<PxParticleSystem, PxU32>("NbParticles");
		inStream.createProperty<PxParticleSystem, PxU32>("ValidParticleRange");
		inStream.createProperty<PxParticleSystem, PxU32>("ValidParticleBitmap", "", PropertyType::Array);
		definePropertyStruct<PxParticleSystem, PxParticleSystemGeneratedValues, PxParticleSystem>(inStream);
	}
	{ // PxParticleFluid
		createClassDeriveAndDefineProperties<PxParticleFluid, PxParticleBase>(inStream);
		inStream.createProperty<PxParticleFluid, PxU32>("NbParticles");
		inStream.createProperty<PxParticleFluid, PxU32>("ValidParticleRange");
		inStream.createProperty<PxParticleFluid, PxU32>("ValidParticleBitmap", "", PropertyType::Array);
		PvdPropertyDefinitionHelper& helper(inStream.getPropertyDefinitionHelper());
		PvdClassInfoDefine definitionObj(helper, getPvdNamespacedNameForType<PxParticleFluid>());
		visitParticleFluidBufferProperties(makePvdPropertyFilter(definitionObj));
		definePropertyStruct<PxParticleFluid, PxParticleFluidGeneratedValues, PxParticleFluid>(inStream);
	}
#endif
#if PX_USE_CLOTH_API
	{ // PxClothFabricPhase
		createClassAndDefineProperties<PxClothFabricPhase>(inStream);
	}
	{ // PxClothFabric
		createClassAndDefineProperties<PxClothFabric>(inStream);
		definePropertyStruct<PxClothFabric, PxClothFabricGeneratedValues, PxClothFabric>(inStream);
		inStream.createProperty<PxClothFabric, ObjectRef>("Physics", "parents");
		inStream.createProperty<PxClothFabric, ObjectRef>("Cloths", "children", PropertyType::Array);
		inStream.createProperty<PxClothFabric, PxClothFabricPhase>("Phases", "", PropertyType::Array);
	}
	{ // PxCloth
		{ // PxClothParticle
			createClassAndDefineProperties<PxClothParticle>(inStream);
		}
		{ // PxClothStretchConfig
			createClassAndDefineProperties<PxClothStretchConfig>(inStream);
		}
		{ // PxClothTetherConstraintConfig
			createClassAndDefineProperties<PxClothTetherConfig>(inStream);
		}
		{ // PvdPositionAndRadius
			inStream.createClass<PvdPositionAndRadius>();
			inStream.createProperty<PvdPositionAndRadius, PxVec3>("Position");
			inStream.createProperty<PvdPositionAndRadius, PxF32>("Radius");
		}
		createClassDeriveAndDefineProperties<PxCloth, PxActor>(inStream);
		definePropertyStruct<PxCloth, PxClothGeneratedValues, PxCloth>(inStream);
		inStream.createProperty<PxCloth, ObjectRef>("Fabric");
		inStream.createProperty<PxCloth, PxClothParticle>("ParticleBuffer", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxClothParticle>("ParticleAccelerations", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PvdPositionAndRadius>("MotionConstraints", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PvdPositionAndRadius>("CollisionSpheres", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PvdPositionAndRadius>("SeparationConstraints", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxU32>("CollisionSpherePairs", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PvdPositionAndRadius>("CollisionPlanes", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxU32>("CollisionConvexMasks", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxVec3>("CollisionTriangles", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxU32>("VirtualParticles", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxVec3>("VirtualParticleWeights", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxU32>("SelfCollisionIndices", "", PropertyType::Array);
		inStream.createProperty<PxCloth, PxVec4>("RestPositions", "", PropertyType::Array);
	}
#endif
	{
		// PxAggregate
		createClassAndDefineProperties<PxAggregate>(inStream);
		inStream.createProperty<PxAggregate, ObjectRef>("Scene", "parents");
		definePropertyStruct<PxAggregate, PxAggregateGeneratedValues, PxAggregate>(inStream);
		inStream.createProperty<PxAggregate, ObjectRef>("Actors", "children", PropertyType::Array);
		inStream.createProperty<PxAggregate, ObjectRef>("Articulations", "children", PropertyType::Array);
	}
}

template <typename TClassType, typename TValueType, typename TDataType>
static void doSendAllProperties(PvdDataStream& inStream, const TDataType* inDatatype, const void* instanceId)
{
	TValueType theValues(inDatatype);
	inStream.setPropertyMessage(instanceId, theValues);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxPhysics& inPhysics)
{
	PxTolerancesScale theScale(inPhysics.getTolerancesScale());
	doSendAllProperties<PxPhysics, PxTolerancesScaleGeneratedValues>(inStream, &theScale, &inPhysics);
	inStream.setPropertyValue(&inPhysics, "Version.Major", PxU32(PX_PHYSICS_VERSION_MAJOR));
	inStream.setPropertyValue(&inPhysics, "Version.Minor", PxU32(PX_PHYSICS_VERSION_MINOR));
	inStream.setPropertyValue(&inPhysics, "Version.Bugfix", PxU32(PX_PHYSICS_VERSION_BUGFIX));

#if PX_CHECKED
#if defined(NDEBUG)
	// This is a checked build
	String buildType = "Checked";
#elif defined(_DEBUG)
	// This is a debug build
	String buildType = "Debug";
#endif
#elif PX_PROFILE
	String buildType = "Profile";
#elif defined(NDEBUG)
	// This is a release build
	String buildType = "Release";
#endif
	inStream.setPropertyValue(&inPhysics, "Version.Build", buildType);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxScene& inScene)
{
	PxPhysics& physics(const_cast<PxScene&>(inScene).getPhysics());
	PxTolerancesScale theScale;
	PxSceneDesc theDesc(theScale);

	{
		theDesc.gravity = inScene.getGravity();

		theDesc.simulationEventCallback = inScene.getSimulationEventCallback(PX_DEFAULT_CLIENT);
		theDesc.contactModifyCallback = inScene.getContactModifyCallback();
		theDesc.ccdContactModifyCallback = inScene.getCCDContactModifyCallback();

		theDesc.filterShaderData = inScene.getFilterShaderData();
		theDesc.filterShaderDataSize = inScene.getFilterShaderDataSize();
		theDesc.filterShader = inScene.getFilterShader();
		theDesc.filterCallback = inScene.getFilterCallback();

		theDesc.broadPhaseType = inScene.getBroadPhaseType();
		theDesc.broadPhaseCallback = inScene.getBroadPhaseCallback();

		theDesc.limits = inScene.getLimits();

		theDesc.frictionType = inScene.getFrictionType();

		theDesc.bounceThresholdVelocity = inScene.getBounceThresholdVelocity();

		theDesc.frictionOffsetThreshold = inScene.getFrictionOffsetThreshold();

		theDesc.flags = inScene.getFlags();

		theDesc.cpuDispatcher = inScene.getCpuDispatcher();
		theDesc.gpuDispatcher = inScene.getGpuDispatcher();
		
		theDesc.staticStructure = inScene.getStaticStructure();
		theDesc.dynamicStructure = inScene.getDynamicStructure();
		theDesc.dynamicTreeRebuildRateHint = inScene.getDynamicTreeRebuildRateHint();

		theDesc.userData = inScene.userData;

		theDesc.solverBatchSize = inScene.getSolverBatchSize();

		// theDesc.nbContactDataBlocks			= inScene.getNbContactDataBlocksUsed();
		// theDesc.maxNbContactDataBlocks		= inScene.getMaxNbContactDataBlocksUsed();

		theDesc.contactReportStreamBufferSize = inScene.getContactReportStreamBufferSize();

		theDesc.ccdMaxPasses = inScene.getCCDMaxPasses();

		// theDesc.simulationOrder				= inScene.getSimulationOrder();

		theDesc.wakeCounterResetValue = inScene.getWakeCounterResetValue();
	}

	PxSceneDescGeneratedValues theValues(&theDesc);
	inStream.setPropertyMessage(&inScene, theValues);
	// Create parent/child relationship.
	inStream.setPropertyValue(&inScene, "Physics", reinterpret_cast<const void*>(&physics));
	inStream.pushBackObjectRef(&physics, "Scenes", &inScene);
}

void PvdMetaDataBinding::sendBeginFrame(PvdDataStream& inStream, const PxScene* inScene, PxReal simulateElapsedTime)
{
	inStream.beginSection(inScene, "frame");
	inStream.setPropertyValue(inScene, "Timestamp", inScene->getTimestamp());
	inStream.setPropertyValue(inScene, "SimulateElapsedTime", simulateElapsedTime);
}

template <typename TDataType>
struct NullConverter
{
	void operator()(TDataType& data, const TDataType& src)
	{
		data = src;
	}
};

template <typename TTargetType, PxU32 T_NUM_ITEMS, typename TSourceType = TTargetType,
          typename Converter = NullConverter<TTargetType> >
class ScopedPropertyValueSender
{
	TTargetType			mStack[T_NUM_ITEMS];
	TTargetType*		mCur;
	const TTargetType*	mEnd;
	PvdDataStream&		mStream;

  public:
	ScopedPropertyValueSender(PvdDataStream& inStream, const void* inObj, String name)
	: mCur(mStack), mEnd(&mStack[T_NUM_ITEMS]), mStream(inStream)
	{
		mStream.beginSetPropertyValue(inObj, name, getPvdNamespacedNameForType<TTargetType>());
	}

	~ScopedPropertyValueSender()
	{
		if(mStack != mCur)
		{
			PxU32 size = sizeof(TTargetType) * PxU32(mCur - mStack);
			mStream.appendPropertyValueData(DataRef<const PxU8>(reinterpret_cast<PxU8*>(mStack), size));
		}
		mStream.endSetPropertyValue();
	}

	void append(const TSourceType& data)
	{
		Converter()(*mCur, data);
		if(mCur < mEnd - 1)
			++mCur;
		else
		{
			mStream.appendPropertyValueData(DataRef<const PxU8>(reinterpret_cast<PxU8*>(mStack), sizeof mStack));
			mCur = mStack;
		}
	}

  private:
	ScopedPropertyValueSender& operator=(const ScopedPropertyValueSender&);
};

void PvdMetaDataBinding::sendContacts(PvdDataStream& inStream, const PxScene& inScene)
{
	inStream.setPropertyValue(&inScene, "Contacts", DataRef<const PxU8>(), getPvdNamespacedNameForType<PvdContact>());
}

struct PvdContactConverter
{
	void operator()(PvdContact& data, const Sc::Contact& src)
	{
		data.point = src.point;
		data.axis = src.normal;
		data.shape0 = src.shape0;
		data.shape1 = src.shape1;
		data.separation = src.separation;
		data.normalForce = src.normalForce;
		data.internalFaceIndex0 = src.faceIndex0;
		data.internalFaceIndex1 = src.faceIndex1;
		data.normalForceAvailable = src.normalForceAvailable;
	}
};

void PvdMetaDataBinding::sendContacts(PvdDataStream& inStream, const PxScene& inScene, Array<Contact>& inContacts)
{
	ScopedPropertyValueSender<PvdContact, 32, Sc::Contact, PvdContactConverter> sender(inStream, &inScene, "Contacts");

	for(PxU32 i = 0; i < inContacts.size(); i++)
	{
		sender.append(inContacts[i]);
	}
}

void PvdMetaDataBinding::sendStats(PvdDataStream& inStream, const PxScene* inScene, void* triMeshCacheStats)
{
	PxSimulationStatistics theStats;
	inScene->getSimulationStatistics(theStats);

	PX_UNUSED(triMeshCacheStats);
#if PX_SUPPORT_GPU_PHYSX
	if(triMeshCacheStats)
	{
		// gpu triangle mesh cache stats
		PxTriangleMeshCacheStatistics* tmp = static_cast<PxTriangleMeshCacheStatistics*>(triMeshCacheStats);
		theStats.particlesGpuMeshCacheSize = tmp->bytesTotal;
		theStats.particlesGpuMeshCacheUsed = tmp->bytesUsed;
		theStats.particlesGpuMeshCacheHitrate = tmp->percentageHitrate;
	}
#endif

	PxSimulationStatisticsGeneratedValues values(&theStats);
	inStream.setPropertyMessage(inScene, values);
}

void PvdMetaDataBinding::sendEndFrame(PvdDataStream& inStream, const PxScene* inScene)
{
	//flush other client
	inStream.endSection(inScene, "frame");
}

template <typename TDataType>
static void addPhysicsGroupProperty(PvdDataStream& inStream, const char* groupName, const TDataType& inData, const PxPhysics& ownerPhysics)
{
	inStream.setPropertyValue(&inData, "Physics", reinterpret_cast<const void*>(&ownerPhysics));
	inStream.pushBackObjectRef(&ownerPhysics, groupName, &inData);
	// Buffer type objects *have* to be flushed directly out once created else scene creation doesn't work.
}

template <typename TDataType>
static void removePhysicsGroupProperty(PvdDataStream& inStream, const char* groupName, const TDataType& inData, const PxPhysics& ownerPhysics)
{
	inStream.removeObjectRef(&ownerPhysics, groupName, &inData);
	inStream.destroyInstance(&inData);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxMaterial& inMaterial, const PxPhysics& ownerPhysics)
{
	inStream.createInstance(&inMaterial);
	sendAllProperties(inStream, inMaterial);
	addPhysicsGroupProperty(inStream, "Materials", inMaterial, ownerPhysics);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxMaterial& inMaterial)
{
	PxMaterialGeneratedValues values(&inMaterial);
	inStream.setPropertyMessage(&inMaterial, values);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxMaterial& inMaterial, const PxPhysics& ownerPhysics)
{
	removePhysicsGroupProperty(inStream, "Materials", inMaterial, ownerPhysics);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxHeightField& inData)
{
	PxHeightFieldDesc theDesc;
	// Save the height field to desc.
	theDesc.nbRows = inData.getNbRows();
	theDesc.nbColumns = inData.getNbColumns();
	theDesc.format = inData.getFormat();
	theDesc.samples.stride = inData.getSampleStride();
	theDesc.samples.data = NULL;
	theDesc.thickness = inData.getThickness();
	theDesc.convexEdgeThreshold = inData.getConvexEdgeThreshold();
	theDesc.flags = inData.getFlags();

	PxU32 theCellCount = inData.getNbRows() * inData.getNbColumns();
	PxU32 theSampleStride = sizeof(PxHeightFieldSample);
	PxU32 theSampleBufSize = theCellCount * theSampleStride;
	mBindingData->mTempU8Array.resize(theSampleBufSize);
	PxHeightFieldSample* theSamples = reinterpret_cast<PxHeightFieldSample*>(mBindingData->mTempU8Array.begin());
	inData.saveCells(theSamples, theSampleBufSize);
	theDesc.samples.data = theSamples;
	PxHeightFieldDescGeneratedValues values(&theDesc);
	inStream.setPropertyMessage(&inData, values);
	PxHeightFieldSample* theSampleData = reinterpret_cast<PxHeightFieldSample*>(mBindingData->mTempU8Array.begin());
	inStream.setPropertyValue(&inData, "Samples", theSampleData, theCellCount);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxHeightField& inData, const PxPhysics& ownerPhysics)
{
	inStream.createInstance(&inData);
	sendAllProperties(inStream, inData);
	addPhysicsGroupProperty(inStream, "HeightFields", inData, ownerPhysics);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxHeightField& inData, const PxPhysics& ownerPhysics)
{
	removePhysicsGroupProperty(inStream, "HeightFields", inData, ownerPhysics);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxConvexMesh& inData, const PxPhysics& ownerPhysics)
{
	inStream.createInstance(&inData);
	PxReal mass;
	PxMat33 localInertia;
	PxVec3 localCom;
	inData.getMassInformation(mass, reinterpret_cast<PxMat33&>(localInertia), localCom);
	inStream.setPropertyValue(&inData, "Mass", mass);
	inStream.setPropertyValue(&inData, "LocalInertia", localInertia);
	inStream.setPropertyValue(&inData, "LocalCenterOfMass", localCom);

	// update arrays:
	// vertex Array:
	{
		const PxVec3* vertexPtr = inData.getVertices();
		const PxU32 numVertices = inData.getNbVertices();
		inStream.setPropertyValue(&inData, "Points", vertexPtr, numVertices);
	}

	// HullPolyArray:
	PxU16 maxIndices = 0;
	{

		PxU32 numPolygons = inData.getNbPolygons();
		PvdHullPolygonData* tempData = mBindingData->allocateTemp<PvdHullPolygonData>(numPolygons);
		// Get the polygon data stripping the plane equations
		for(PxU32 index = 0; index < numPolygons; index++)
		{
			PxHullPolygon curOut;
			inData.getPolygonData(index, curOut);
			maxIndices = PxMax(maxIndices, PxU16(curOut.mIndexBase + curOut.mNbVerts));
			tempData[index].mIndexBase = curOut.mIndexBase;
			tempData[index].mNumVertices = curOut.mNbVerts;
		}
		inStream.setPropertyValue(&inData, "HullPolygons", tempData, numPolygons);
	}

	// poly index Array:
	{
		const PxU8* indices = inData.getIndexBuffer();
		inStream.setPropertyValue(&inData, "PolygonIndexes", indices, maxIndices);
	}
	addPhysicsGroupProperty(inStream, "ConvexMeshes", inData, ownerPhysics);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxConvexMesh& inData, const PxPhysics& ownerPhysics)
{
	removePhysicsGroupProperty(inStream, "ConvexMeshes", inData, ownerPhysics);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxTriangleMesh& inData, const PxPhysics& ownerPhysics)
{
	inStream.createInstance(&inData);
	bool hasMatIndex = inData.getTriangleMaterialIndex(0) != 0xffff;
	// update arrays:
	// vertex Array:
	{
		const PxVec3* vertexPtr = inData.getVertices();
		const PxU32 numVertices = inData.getNbVertices();
		inStream.setPropertyValue(&inData, "Points", vertexPtr, numVertices);
	}

	// index Array:
	{
		const bool has16BitIndices = inData.getTriangleMeshFlags() & PxTriangleMeshFlag::e16_BIT_INDICES ? true : false;
		const PxU32 numTriangles = inData.getNbTriangles();

		inStream.setPropertyValue(&inData, "NbTriangles", numTriangles);

		const PxU32 numIndexes = numTriangles * 3;
		const PxU8* trianglePtr = reinterpret_cast<const PxU8*>(inData.getTriangles());
		// We declared this type as a 32 bit integer above.
		// PVD will automatically unsigned-extend data that is smaller than the target type.
		if(has16BitIndices)
			inStream.setPropertyValue(&inData, "Triangles", reinterpret_cast<const PxU16*>(trianglePtr), numIndexes);
		else
			inStream.setPropertyValue(&inData, "Triangles", reinterpret_cast<const PxU32*>(trianglePtr), numIndexes);
	}

	// material Array:
	if(hasMatIndex)
	{
		PxU32 numMaterials = inData.getNbTriangles();
		PxU16* matIndexData = mBindingData->allocateTemp<PxU16>(numMaterials);
		for(PxU32 m = 0; m < numMaterials; m++)
			matIndexData[m] = inData.getTriangleMaterialIndex(m);
		inStream.setPropertyValue(&inData, "MaterialIndices", matIndexData, numMaterials);
	}
	addPhysicsGroupProperty(inStream, "TriangleMeshes", inData, ownerPhysics);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxTriangleMesh& inData, const PxPhysics& ownerPhysics)
{
	removePhysicsGroupProperty(inStream, "TriangleMeshes", inData, ownerPhysics);
}

template <typename TDataType>
void PvdMetaDataBinding::registrarPhysicsObject(PvdDataStream&, const TDataType&, PsPvd*)
{
}

template <>
void PvdMetaDataBinding::registrarPhysicsObject<PxConvexMeshGeometry>(PvdDataStream& inStream, const PxConvexMeshGeometry& geom, PsPvd* pvd)
{
	if(pvd->registerObject(geom.convexMesh))
		createInstance(inStream, *geom.convexMesh, PxGetPhysics());
}

template <>
void PvdMetaDataBinding::registrarPhysicsObject<PxTriangleMeshGeometry>(PvdDataStream& inStream, const PxTriangleMeshGeometry& geom, PsPvd* pvd)
{
	if(pvd->registerObject(geom.triangleMesh))
		createInstance(inStream, *geom.triangleMesh, PxGetPhysics());
}

template <>
void PvdMetaDataBinding::registrarPhysicsObject<PxHeightFieldGeometry>(PvdDataStream& inStream, const PxHeightFieldGeometry& geom, PsPvd* pvd)
{
	if(pvd->registerObject(geom.heightField))
		createInstance(inStream, *geom.heightField, PxGetPhysics());
}

template <typename TGeneratedValuesType, typename TGeomType>
static void sendGeometry(PvdMetaDataBinding& metaBind, PvdDataStream& inStream, const PxShape& inShape, const TGeomType& geom, PsPvd* pvd)
{
	const void* geomInst = (reinterpret_cast<const PxU8*>(&inShape)) + 4;
	inStream.createInstance(getPvdNamespacedNameForType<TGeomType>(), geomInst);
	metaBind.registrarPhysicsObject<TGeomType>(inStream, geom, pvd);
	TGeneratedValuesType values(&geom);
	inStream.setPropertyMessage(geomInst, values);
	inStream.setPropertyValue(&inShape, "Geometry", geomInst);
	inStream.setPropertyValue(geomInst, "Shape", reinterpret_cast<const void*>(&inShape));
}

static void setGeometry(PvdMetaDataBinding& metaBind, PvdDataStream& inStream, const PxShape& inObj, PsPvd* pvd)
{
	switch(inObj.getGeometryType())
	{
#define SEND_PVD_GEOM_TYPE(enumType, geomType, valueType)				\
	case PxGeometryType::enumType:                                      \
	{                                                                   \
		Px##geomType geom;                                              \
		inObj.get##geomType(geom);                                      \
		sendGeometry<valueType>(metaBind, inStream, inObj, geom, pvd);  \
	}                                                                   \
	break;
		SEND_PVD_GEOM_TYPE(eSPHERE, SphereGeometry, PxSphereGeometryGeneratedValues);
	// Plane geometries don't have any properties, so this avoids using a property
	// struct for them.
	case PxGeometryType::ePLANE:
	{
		PxPlaneGeometry geom;
		inObj.getPlaneGeometry(geom);
		const void* geomInst = (reinterpret_cast<const PxU8*>(&inObj)) + 4;
		inStream.createInstance(getPvdNamespacedNameForType<PxPlaneGeometry>(), geomInst);
		inStream.setPropertyValue(&inObj, "Geometry", geomInst);
		inStream.setPropertyValue(geomInst, "Shape", reinterpret_cast<const void*>(&inObj));
	}
	break;
		SEND_PVD_GEOM_TYPE(eCAPSULE, CapsuleGeometry, PxCapsuleGeometryGeneratedValues);
		SEND_PVD_GEOM_TYPE(eBOX, BoxGeometry, PxBoxGeometryGeneratedValues);
		SEND_PVD_GEOM_TYPE(eCONVEXMESH, ConvexMeshGeometry, PxConvexMeshGeometryGeneratedValues);
		SEND_PVD_GEOM_TYPE(eTRIANGLEMESH, TriangleMeshGeometry, PxTriangleMeshGeometryGeneratedValues);
		SEND_PVD_GEOM_TYPE(eHEIGHTFIELD, HeightFieldGeometry, PxHeightFieldGeometryGeneratedValues);
#undef SEND_PVD_GEOM_TYPE
	case PxGeometryType::eGEOMETRY_COUNT:
	case PxGeometryType::eINVALID:
		PX_ASSERT(false);
		break;
	}
}

static void setMaterials(PvdMetaDataBinding& metaBing, PvdDataStream& inStream, const PxShape& inObj, PsPvd* pvd, PvdMetaDataBindingData* mBindingData)
{
	PxU32 numMaterials = inObj.getNbMaterials();
	PxMaterial** materialPtr = mBindingData->allocateTemp<PxMaterial*>(numMaterials);
	inObj.getMaterials(materialPtr, numMaterials);
	for(PxU32 idx = 0; idx < numMaterials; ++idx)
	{
		if(pvd->registerObject(materialPtr[idx]))
			metaBing.createInstance(inStream, *materialPtr[idx], PxGetPhysics());
		inStream.pushBackObjectRef(&inObj, "Materials", materialPtr[idx]);
	}
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxShape& inObj, const PxRigidActor& owner, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	if(!inStream.isInstanceValid(&owner))
		return;

	const OwnerActorsMap::Entry* entry = mBindingData->mOwnerActorsMap.find(&inObj);
	if(entry != NULL)
	{
		if(!mBindingData->mOwnerActorsMap[&inObj]->contains(&owner))
			mBindingData->mOwnerActorsMap[&inObj]->insert(&owner);
	}
	else
	{
		OwnerActorsValueType* data = reinterpret_cast<OwnerActorsValueType*>(
		    PX_ALLOC(sizeof(OwnerActorsValueType), "mOwnerActorsMapValue")); //( 1 );
		OwnerActorsValueType* actors = PX_PLACEMENT_NEW(data, OwnerActorsValueType);
		actors->insert(&owner);

		mBindingData->mOwnerActorsMap.insert(&inObj, actors);
	}

	if(inStream.isInstanceValid(&inObj))
	{
		inStream.pushBackObjectRef(&owner, "Shapes", &inObj);
		return;
	}

	inStream.createInstance(&inObj);
	inStream.pushBackObjectRef(&owner, "Shapes", &inObj);
	inStream.setPropertyValue(&inObj, "Actor", reinterpret_cast<const void*>(&owner));
	sendAllProperties(inStream, inObj);
	setGeometry(*this, inStream, inObj, pvd);
	setMaterials(*this, inStream, inObj, pvd, mBindingData);
	if(!inObj.isExclusive())
		inStream.pushBackObjectRef(&ownerPhysics, "SharedShapes", &inObj);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxShape& inObj)
{
	PxShapeGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::releaseAndRecreateGeometry(PvdDataStream& inStream, const PxShape& inObj, PxPhysics& /*ownerPhysics*/, PsPvd* pvd)
{
	const void* geomInst = (reinterpret_cast<const PxU8*>(&inObj)) + 4;
	inStream.destroyInstance(geomInst);
	// Quick fix for HF modify, PxConvexMesh and PxTriangleMesh need recook, they should always be new if modified
	if(inObj.getGeometryType() == PxGeometryType::eHEIGHTFIELD)
	{
		PxHeightFieldGeometry hfGeom;
		inObj.getHeightFieldGeometry(hfGeom);
		if(inStream.isInstanceValid(hfGeom.heightField))
			sendAllProperties(inStream, *hfGeom.heightField);
	}

	setGeometry(*this, inStream, inObj, pvd);

	// Need update actor cause PVD takes actor-shape as a pair.
	{
		PxRigidActor* actor = inObj.getActor();
		if(actor != NULL)
		{

			if(const PxRigidStatic* rgS = actor->is<PxRigidStatic>())
				sendAllProperties(inStream, *rgS);
			else if(const PxRigidDynamic* rgD = actor->is<PxRigidDynamic>())
				sendAllProperties(inStream, *rgD);
		}
	}
}

void PvdMetaDataBinding::updateMaterials(PvdDataStream& inStream, const PxShape& inObj, PsPvd* pvd)
{
	// Clear the shape's materials array.
	inStream.setPropertyValue(&inObj, "Materials", DataRef<const PxU8>(), getPvdNamespacedNameForType<ObjectRef>());
	setMaterials(*this, inStream, inObj, pvd, mBindingData);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxShape& inObj, const PxRigidActor& owner)
{
	if(inStream.isInstanceValid(&inObj))
	{
		inStream.removeObjectRef(&owner, "Shapes", &inObj);

		bool bDestroy = true;
		const OwnerActorsMap::Entry* entry0 = mBindingData->mOwnerActorsMap.find(&inObj);
		if(entry0 != NULL)
		{
			entry0->second->erase(&owner);
			if(entry0->second->size() > 0)
				bDestroy = false;
			else
			{
				mBindingData->mOwnerActorsMap[&inObj]->~OwnerActorsValueType();
				PX_FREE(mBindingData->mOwnerActorsMap[&inObj]);
				mBindingData->mOwnerActorsMap.erase(&inObj);
			}
		}

		if(bDestroy)
		{			
		    if(!inObj.isExclusive())
	            inStream.removeObjectRef(&PxGetPhysics(), "SharedShapes", &inObj);

			const void* geomInst = (reinterpret_cast<const PxU8*>(&inObj)) + 4;
			inStream.destroyInstance(geomInst);
			inStream.destroyInstance(&inObj);

			const OwnerActorsMap::Entry* entry = mBindingData->mOwnerActorsMap.find(&inObj);
			if(entry != NULL)
			{
				entry->second->~OwnerActorsValueType();
				PX_FREE(entry->second);
				mBindingData->mOwnerActorsMap.erase(&inObj);
			}			
		}
	}
}

template <typename TDataType>
static void addSceneGroupProperty(PvdDataStream& inStream, const char* groupName, const TDataType& inObj, const PxScene& inScene)
{
	inStream.createInstance(&inObj);
	inStream.pushBackObjectRef(&inScene, groupName, &inObj);
	inStream.setPropertyValue(&inObj, "Scene", reinterpret_cast<const void*>(&inScene));
}

template <typename TDataType>
static void removeSceneGroupProperty(PvdDataStream& inStream, const char* groupName, const TDataType& inObj, const PxScene& inScene)
{
	inStream.removeObjectRef(&inScene, groupName, &inObj);
	inStream.destroyInstance(&inObj);
}

static void sendShapes(PvdMetaDataBinding& binding, PvdDataStream& inStream, const PxRigidActor& inObj, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	InlineArray<PxShape*, 5> shapeData;
	PxU32 nbShapes = inObj.getNbShapes();
	shapeData.resize(nbShapes);
	inObj.getShapes(shapeData.begin(), nbShapes);
	for(PxU32 idx = 0; idx < nbShapes; ++idx)
		binding.createInstance(inStream, *shapeData[idx], inObj, ownerPhysics, pvd);
}

static void releaseShapes(PvdMetaDataBinding& binding, PvdDataStream& inStream, const PxRigidActor& inObj)
{
	InlineArray<PxShape*, 5> shapeData;
	PxU32 nbShapes = inObj.getNbShapes();
	shapeData.resize(nbShapes);
	inObj.getShapes(shapeData.begin(), nbShapes);
	for(PxU32 idx = 0; idx < nbShapes; ++idx)
		binding.destroyInstance(inStream, *shapeData[idx], inObj);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxRigidStatic& inObj, const PxScene& ownerScene, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	addSceneGroupProperty(inStream, "RigidStatics", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
	sendShapes(*this, inStream, inObj, ownerPhysics, pvd);
}
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxRigidStatic& inObj)
{
	PxRigidStaticGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}
void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxRigidStatic& inObj, const PxScene& ownerScene)
{
	releaseShapes(*this, inStream, inObj);
	removeSceneGroupProperty(inStream, "RigidStatics", inObj, ownerScene);
}
void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxRigidDynamic& inObj, const PxScene& ownerScene, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	addSceneGroupProperty(inStream, "RigidDynamics", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
	sendShapes(*this, inStream, inObj, ownerPhysics, pvd);
}
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxRigidDynamic& inObj)
{
	PxRigidDynamicGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}
void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxRigidDynamic& inObj, const PxScene& ownerScene)
{
	releaseShapes(*this, inStream, inObj);
	removeSceneGroupProperty(inStream, "RigidDynamics", inObj, ownerScene);
}

static void addChild(PvdDataStream& inStream, const void* inParent, const PxArticulationLink& inChild)
{
	inStream.pushBackObjectRef(inParent, "Links", &inChild);
	inStream.setPropertyValue(&inChild, "Parent", inParent);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxArticulation& inObj, const PxScene& ownerScene, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	addSceneGroupProperty(inStream, "Articulations", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
	PxU32 numLinks = inObj.getNbLinks();
	mBindingData->mArticulationLinks.resize(numLinks);
	inObj.getLinks(mBindingData->mArticulationLinks.begin(), numLinks);
	// From Dilip Sequiera:
	/*
	    No, there can only be one root, and in all the code I wrote (which is not 100% of the HL code for
	   articulations),  the index of a child is always > the index of the parent.
	*/

	// Create all the links
	for(PxU32 idx = 0; idx < numLinks; ++idx)
	{
		if(!inStream.isInstanceValid(mBindingData->mArticulationLinks[idx]))
			createInstance(inStream, *mBindingData->mArticulationLinks[idx], ownerPhysics, pvd);
	}

	// Setup the link graph
	for(PxU32 idx = 0; idx < numLinks; ++idx)
	{
		PxArticulationLink* link = mBindingData->mArticulationLinks[idx];
		if(idx == 0)
			addChild(inStream, &inObj, *link);

		PxU32 numChildren = link->getNbChildren();
		PxArticulationLink** children = mBindingData->allocateTemp<PxArticulationLink*>(numChildren);
		link->getChildren(children, numChildren);
		for(PxU32 i = 0; i < numChildren; ++i)
			addChild(inStream, link, *children[i]);
	}
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxArticulation& inObj)
{
	PxArticulationGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxArticulation& inObj, const PxScene& ownerScene)
{
	removeSceneGroupProperty(inStream, "Articulations", inObj, ownerScene);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxArticulationLink& inObj, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	inStream.createInstance(&inObj);
	PxArticulationJoint* joint(inObj.getInboundJoint());
	if(joint)
	{
		inStream.createInstance(joint);
		inStream.setPropertyValue(&inObj, "InboundJoint", reinterpret_cast<const void*>(joint));
		inStream.setPropertyValue(joint, "Link", reinterpret_cast<const void*>(&inObj));
		sendAllProperties(inStream, *joint);
	}
	sendAllProperties(inStream, inObj);
	sendShapes(*this, inStream, inObj, ownerPhysics, pvd);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxArticulationLink& inObj)
{
	PxArticulationLinkGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxArticulationLink& inObj)
{
	PxArticulationJoint* joint(inObj.getInboundJoint());
	if(joint)
		inStream.destroyInstance(joint);
	releaseShapes(*this, inStream, inObj);
	inStream.destroyInstance(&inObj);
}
// These are created as part of the articulation link's creation process, so outside entities don't need to
// create them.
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxArticulationJoint& inObj)
{
	PxArticulationJointGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::originShift(PvdDataStream& inStream, const PxScene* inScene, PxVec3 shift)
{
	inStream.originShift(inScene, shift);
}

template <typename TReadDataType>
struct ParticleFluidUpdater
{
	TReadDataType& mData;
	Array<PxU8>& mTempU8Array;
	PvdDataStream& mStream;
	const void* mInstanceId;
	PxU32 mRdFlags;
	ParticleFluidUpdater(TReadDataType& d, PvdDataStream& s, const void* id, PxU32 flags, Array<PxU8>& tempArray)
	: mData(d), mTempU8Array(tempArray), mStream(s), mInstanceId(id), mRdFlags(flags)
	{
	}

	template <PxU32 TKey, typename TObjectType, typename TPropertyType, PxU32 TEnableFlag>
	void handleBuffer(const PxBufferPropertyInfo<TKey, TObjectType, PxStrideIterator<const TPropertyType>, TEnableFlag>& inProp, NamespacedName datatype)
	{
		PxU32 nbValidParticles = mData.nbValidParticles;
		PxU32 validParticleRange = mData.validParticleRange;
		PxStrideIterator<const TPropertyType> iterator(inProp.get(&mData));
		const PxU32* validParticleBitmap = mData.validParticleBitmap;

		if(nbValidParticles == 0 || iterator.ptr() == NULL || inProp.isEnabled(mRdFlags) == false)
			return;

		// setup the pvd array
		DataRef<const PxU8> propData;
		mTempU8Array.resize(nbValidParticles * sizeof(TPropertyType));
		TPropertyType* tmpArray = reinterpret_cast<TPropertyType*>(mTempU8Array.begin());
		propData = DataRef<const PxU8>(mTempU8Array.begin(), mTempU8Array.size());
		if(nbValidParticles == validParticleRange)
		{
			for(PxU32 idx = 0; idx < nbValidParticles; ++idx)
				tmpArray[idx] = iterator[idx];
		}
		else
		{
			PxU32 tIdx = 0;
			// iterate over bitmap and send all valid particles
			for(PxU32 w = 0; w <= (validParticleRange - 1) >> 5; w++)
			{
				for(PxU32 b = validParticleBitmap[w]; b; b &= b - 1)
				{
					tmpArray[tIdx++] = iterator[w << 5 | Ps::lowestSetBit(b)];
				}
			}
			PX_ASSERT(tIdx == nbValidParticles);
		}
		mStream.setPropertyValue(mInstanceId, inProp.mName, propData, datatype);
	}
	template <PxU32 TKey, typename TObjectType, typename TPropertyType, PxU32 TEnableFlag>
	void handleBuffer(const PxBufferPropertyInfo<TKey, TObjectType, PxStrideIterator<const TPropertyType>, TEnableFlag>& inProp)
	{
		handleBuffer(inProp, getPvdNamespacedNameForType<TPropertyType>());
	}

	template <PxU32 TKey, typename TObjectType, typename TEnumType, typename TStorageType, PxU32 TEnableFlag>
	void handleFlagsBuffer(const PxBufferPropertyInfo<TKey, TObjectType, PxStrideIterator<const PxFlags<TEnumType, TStorageType> >, TEnableFlag>& inProp, const PxU32ToName*)
	{
		handleBuffer(inProp, getPvdNamespacedNameForType<TStorageType>());
	}

  private:
	ParticleFluidUpdater& operator=(const ParticleFluidUpdater&);
};

#if PX_USE_PARTICLE_SYSTEM_API
void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxParticleSystem& inObj, const PxScene& ownerScene)
{
	addSceneGroupProperty(inStream, "ParticleSystems", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
	PxParticleReadData* readData(const_cast<PxParticleSystem&>(inObj).lockParticleReadData());
	if(readData)
	{
		PxU32 readFlags = inObj.getParticleReadDataFlags();
		sendArrays(inStream, inObj, *readData, readFlags);
		readData->unlock();
	}
}
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxParticleSystem& inObj)
{
	PxParticleSystemGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::sendArrays(PvdDataStream& inStream, const PxParticleSystem& inObj, PxParticleReadData& inData, PxU32 inFlags)
{
	inStream.setPropertyValue(&inObj, "NbParticles", inData.nbValidParticles);
	inStream.setPropertyValue(&inObj, "ValidParticleRange", inData.validParticleRange);
	if(inData.validParticleRange > 0)
		inStream.setPropertyValue(&inObj, "ValidParticleBitmap", inData.validParticleBitmap, (inData.validParticleRange >> 5) + 1);

	ParticleFluidUpdater<PxParticleReadData> theUpdater(inData, inStream, static_cast<const PxActor*>(&inObj), inFlags, mBindingData->mTempU8Array);
	visitParticleSystemBufferProperties(makePvdPropertyFilter(theUpdater));
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxParticleSystem& inObj, const PxScene& ownerScene)
{
	removeSceneGroupProperty(inStream, "ParticleSystems", inObj, ownerScene);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxParticleFluid& inObj, const PxScene& ownerScene)
{
	addSceneGroupProperty(inStream, "ParticleFluids", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
	PxParticleFluidReadData* readData(const_cast<PxParticleFluid&>(inObj).lockParticleFluidReadData());
	if(readData)
	{
		PxU32 readFlags = inObj.getParticleReadDataFlags();
		sendArrays(inStream, inObj, *readData, readFlags);
		readData->unlock();
	}
}
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxParticleFluid& inObj)
{
	PxParticleFluidGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::sendArrays(PvdDataStream& inStream, const PxParticleFluid& inObj, PxParticleFluidReadData& inData, PxU32 inFlags)
{
	inStream.setPropertyValue(&inObj, "NbParticles", inData.nbValidParticles);
	inStream.setPropertyValue(&inObj, "ValidParticleRange", inData.validParticleRange);
	if(inData.validParticleRange > 0)
		inStream.setPropertyValue(&inObj, "ValidParticleBitmap", inData.validParticleBitmap, (inData.validParticleRange >> 5) + 1);

	ParticleFluidUpdater<PxParticleFluidReadData> theUpdater(inData, inStream, static_cast<const PxActor*>(&inObj), inFlags, mBindingData->mTempU8Array);
	visitParticleSystemBufferProperties(makePvdPropertyFilter(theUpdater));
	visitParticleFluidBufferProperties(makePvdPropertyFilter(theUpdater));
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxParticleFluid& inObj, const PxScene& ownerScene)
{
	removeSceneGroupProperty(inStream, "ParticleFluids", inObj, ownerScene);
}
#endif // PX_USE_PARTICLE_SYSTEM_API

template <typename TBlockType, typename TActorType, typename TOperator>
static void updateActor(PvdDataStream& inStream, TActorType** actorGroup, PxU32 numActors, TOperator sleepingOp, PvdMetaDataBindingData& bindingData)
{
	TBlockType theBlock;
	if(numActors == 0)
		return;
	for(PxU32 idx = 0; idx < numActors; ++idx)
	{
		TActorType* theActor(actorGroup[idx]);
		bool sleeping = sleepingOp(theActor, theBlock);
		bool wasSleeping = bindingData.mSleepingActors.contains(theActor);

		if(sleeping == false || sleeping != wasSleeping)
		{
			theBlock.GlobalPose = theActor->getGlobalPose();
			theBlock.AngularVelocity = theActor->getAngularVelocity();
			theBlock.LinearVelocity = theActor->getLinearVelocity();
			inStream.sendPropertyMessageFromGroup(theActor, theBlock);
			if(sleeping != wasSleeping)
			{
				if(sleeping)
					bindingData.mSleepingActors.insert(theActor);
				else
					bindingData.mSleepingActors.erase(theActor);
			}
		}
	}
}

struct RigidDynamicUpdateOp
{
	bool operator()(PxRigidDynamic* actor, PxRigidDynamicUpdateBlock& block)
	{
		bool sleeping = actor->isSleeping();
		block.IsSleeping = sleeping;
		return sleeping;
	}
};

struct ArticulationLinkUpdateOp
{
	bool sleeping;
	ArticulationLinkUpdateOp(bool s) : sleeping(s)
	{
	}
	bool operator()(PxArticulationLink*, PxArticulationLinkUpdateBlock&)
	{
		return sleeping;
	}
};

void PvdMetaDataBinding::updateDynamicActorsAndArticulations(PvdDataStream& inStream, const PxScene* inScene, PvdVisualizer* linkJointViz)
{
	PX_COMPILE_TIME_ASSERT(sizeof(PxRigidDynamicUpdateBlock) == 14 * 4);
	{
		PxU32 actorCount = inScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
		if(actorCount)
		{
			inStream.beginPropertyMessageGroup<PxRigidDynamicUpdateBlock>();
			mBindingData->mActors.resize(actorCount);
			PxActor** theActors = mBindingData->mActors.begin();
			inScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, theActors, actorCount);
			updateActor<PxRigidDynamicUpdateBlock>(inStream, reinterpret_cast<PxRigidDynamic**>(theActors), actorCount, RigidDynamicUpdateOp(), *mBindingData);
			inStream.endPropertyMessageGroup();
		}
	}
	{
		PxU32 articulationCount = inScene->getNbArticulations();
		if(articulationCount)
		{
			mBindingData->mArticulations.resize(articulationCount);
			PxArticulation** firstArticulation = mBindingData->mArticulations.begin();
			PxArticulation** lastArticulation = firstArticulation + articulationCount;
			inScene->getArticulations(firstArticulation, articulationCount);
			inStream.beginPropertyMessageGroup<PxArticulationLinkUpdateBlock>();
			for(; firstArticulation < lastArticulation; ++firstArticulation)
			{
				PxU32 linkCount = (*firstArticulation)->getNbLinks();
				bool sleeping = (*firstArticulation)->isSleeping();
				if(linkCount)
				{
					mBindingData->mArticulationLinks.resize(linkCount);
					PxArticulationLink** theLink = mBindingData->mArticulationLinks.begin();
					(*firstArticulation)->getLinks(theLink, linkCount);
					updateActor<PxArticulationLinkUpdateBlock>(inStream, theLink, linkCount, ArticulationLinkUpdateOp(sleeping), *mBindingData);
					if(linkJointViz)
					{
						for(PxU32 idx = 0; idx < linkCount; ++idx)
							linkJointViz->visualize(*theLink[idx]);
					}
				}
			}
			inStream.endPropertyMessageGroup();
			firstArticulation = mBindingData->mArticulations.begin();
			for(; firstArticulation < lastArticulation; ++firstArticulation)
				inStream.setPropertyValue(*firstArticulation, "IsSleeping", (*firstArticulation)->isSleeping());
		}
	}
}

template <typename TObjType>
struct CollectionOperator
{
	Array<PxU8>& mTempArray;
	const TObjType& mObject;
	PvdDataStream& mStream;

	CollectionOperator(Array<PxU8>& ary, const TObjType& obj, PvdDataStream& stream)
	: mTempArray(ary), mObject(obj), mStream(stream)
	{
	}
	void pushName(const char*)
	{
	}
	void popName()
	{
	}
	template <typename TAccessor>
	void simpleProperty(PxU32 /*key*/, const TAccessor&)
	{
	}
	template <typename TAccessor>
	void flagsProperty(PxU32 /*key*/, const TAccessor&, const PxU32ToName*)
	{
	}

	template <typename TColType, typename TDataType, typename TCollectionProp>
	void handleCollection(const TCollectionProp& prop, NamespacedName dtype, PxU32 countMultiplier = 1)
	{
		PxU32 count = prop.size(&mObject);
		mTempArray.resize(count * sizeof(TDataType));
		TColType* start = reinterpret_cast<TColType*>(mTempArray.begin());
		prop.get(&mObject, start, count * countMultiplier);
		mStream.setPropertyValue(&mObject, prop.mName, DataRef<const PxU8>(mTempArray.begin(), mTempArray.size()), dtype);
	}
	template <PxU32 TKey, typename TObject, typename TColType>
	void handleCollection(const PxReadOnlyCollectionPropertyInfo<TKey, TObject, TColType>& prop)
	{
		handleCollection<TColType, TColType>(prop, getPvdNamespacedNameForType<TColType>());
	}
	// Enumerations or bitflags.
	template <PxU32 TKey, typename TObject, typename TColType>
	void handleCollection(const PxReadOnlyCollectionPropertyInfo<TKey, TObject, TColType>& prop, const PxU32ToName*)
	{
		PX_COMPILE_TIME_ASSERT(sizeof(TColType) == sizeof(PxU32));
		handleCollection<TColType, PxU32>(prop, getPvdNamespacedNameForType<PxU32>());
	}

  private:
	CollectionOperator& operator=(const CollectionOperator&);
};

#if PX_USE_CLOTH_API
struct PxClothFabricCollectionOperator : CollectionOperator<PxClothFabric>
{
	PxClothFabricCollectionOperator(Array<PxU8>& ary, const PxClothFabric& obj, PvdDataStream& stream)
	: CollectionOperator<PxClothFabric>(ary, obj, stream)
	{
	}

	template <PxU32 TKey, typename TObject, typename TColType>
	void handleCollection(const PxReadOnlyCollectionPropertyInfo<TKey, TObject, TColType>& prop)
	{
		// CollectionOperator<PxClothFabric>::handleCollection<TColType,TColType>( prop,
		// getPvdNamespacedNameForType<TColType>(), sizeof( TColType ) );

		// have to duplicate here because the cloth api expects buffer sizes
		// in the number of elements, not the number of bytes
		PxU32 count = prop.size(&mObject);
		mTempArray.resize(count * sizeof(TColType));
		TColType* start = reinterpret_cast<TColType*>(mTempArray.begin());
		prop.get(&mObject, start, count);
		mStream.setPropertyValue(&mObject, prop.mName, DataRef<const PxU8>(mTempArray.begin(), mTempArray.size()), getPvdNamespacedNameForType<TColType>());
	}

	// Enumerations or bitflags.
	template <PxU32 TKey, typename TObject, typename TColType>
	void handleCollection(const PxReadOnlyCollectionPropertyInfo<TKey, TObject, TColType>& prop, const PxU32ToName*)
	{
		PX_COMPILE_TIME_ASSERT(sizeof(TColType) == sizeof(PxU32));
		CollectionOperator<PxClothFabric>::handleCollection<TColType, PxU32>(prop, getPvdNamespacedNameForType<PxU32>());
	}

  private:
	PxClothFabricCollectionOperator& operator=(const PxClothFabricCollectionOperator&);
};

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxClothFabric& fabric, const PxPhysics& ownerPhysics)
{
	inStream.createInstance(&fabric);
	addPhysicsGroupProperty(inStream, "ClothFabrics", fabric, ownerPhysics);
	sendAllProperties(inStream, fabric);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxClothFabric& fabric)
{
	PxClothFabricCollectionOperator op(mBindingData->mTempU8Array, fabric, inStream);
	visitInstancePvdProperties<PxClothFabric>(op);

	PxClothFabricGeneratedValues values(&fabric);
	inStream.setPropertyMessage(&fabric, values);

	PxU32 count = fabric.getNbPhases();
	PxClothFabricPhase* phases = mBindingData->allocateTemp<PxClothFabricPhase>(count);
	if(count)
		fabric.getPhases(phases, count);
	inStream.setPropertyValue(&fabric, "Phases", DataRef<const PxU8>(reinterpret_cast<PxU8*>(phases), sizeof(PxClothFabricPhase) * count), getPvdNamespacedNameForType<PxClothFabricPhase>());
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxClothFabric& fabric, const PxPhysics& ownerPhysics)
{
	removePhysicsGroupProperty(inStream, "ClothFabrics", fabric, ownerPhysics);
}

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxCloth& cloth, const PxScene& ownerScene, const PxPhysics& ownerPhysics, PsPvd* pvd)
{
	// PT: this is only needed to please some bonkers template code that expects all "createInstance" to have the same signature.
	PX_UNUSED(ownerPhysics);

	addSceneGroupProperty(inStream, "Cloths", cloth, ownerScene);
	PxClothFabric* fabric = cloth.getFabric();
	if(fabric != NULL)
	{
		if(pvd->registerObject(fabric))
			createInstance(inStream, *fabric, PxGetPhysics());
		inStream.setPropertyValue(&cloth, "Fabric", reinterpret_cast<const void*>(fabric));
		inStream.pushBackObjectRef(fabric, "Cloths", &cloth);
	}
	sendAllProperties(inStream, cloth);
}
void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxCloth& cloth)
{
	sendSimpleProperties(inStream, cloth);
	sendParticleAccelerations(inStream, cloth);
	sendMotionConstraints(inStream, cloth);
	// PT: TODO: are we sending the collision spheres twice here? The "sendPairs" param is never "false" in the entire SDK.....
	sendCollisionSpheres(inStream, cloth);
	sendCollisionSpheres(inStream, cloth, true);
	sendCollisionTriangles(inStream, cloth);
	sendVirtualParticles(inStream, cloth);
	sendSeparationConstraints(inStream, cloth);
	sendSelfCollisionIndices(inStream, cloth);
	sendRestPositions(inStream, cloth);
}
void PvdMetaDataBinding::sendSimpleProperties(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxClothGeneratedValues values(&cloth);
	inStream.setPropertyMessage(&cloth, values);
}
void PvdMetaDataBinding::sendMotionConstraints(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 count = cloth.getNbMotionConstraints();
	PxClothParticleMotionConstraint* constraints = mBindingData->allocateTemp<PxClothParticleMotionConstraint>(count);
	if(count)
		cloth.getMotionConstraints(constraints);
	inStream.setPropertyValue(&cloth, "MotionConstraints", mBindingData->tempToRef(), getPvdNamespacedNameForType<PvdPositionAndRadius>());
}

void PvdMetaDataBinding::sendRestPositions(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 count = cloth.getNbRestPositions();
	PxVec4* positions = mBindingData->allocateTemp<PxVec4>(count);
	if(count)
		cloth.getRestPositions(positions);
	inStream.setPropertyValue(&cloth, "RestPositions", mBindingData->tempToRef(), getPvdNamespacedNameForType<PxVec4>());
}

void PvdMetaDataBinding::sendParticleAccelerations(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 count = cloth.getNbParticleAccelerations();
	PxVec4* accelerations = mBindingData->allocateTemp<PxVec4>(count);
	if(count)
		cloth.getParticleAccelerations(accelerations);
	inStream.setPropertyValue(&cloth, "ParticleAccelerations", mBindingData->tempToRef(), getPvdNamespacedNameForType<PxVec4>());
}

void PvdMetaDataBinding::sendSelfCollisionIndices(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 count = cloth.getNbSelfCollisionIndices();
	PxU32* selfCollisionIndices = mBindingData->allocateTemp<PxU32>(count);
	if(count)
		cloth.getSelfCollisionIndices(selfCollisionIndices);
	inStream.setPropertyValue(&cloth, "SelfCollisionIndices", mBindingData->tempToRef(), getPvdNamespacedNameForType<PxU32>());
}

void PvdMetaDataBinding::sendCollisionSpheres(PvdDataStream& inStream, const PxCloth& cloth, bool sendPairs)
{
	PxU32 numSpheres = cloth.getNbCollisionSpheres();
	PxU32 numIndices = 2 * cloth.getNbCollisionCapsules();
	PxU32 numPlanes = cloth.getNbCollisionPlanes();
	PxU32 numConvexes = cloth.getNbCollisionConvexes();
	PxU32 numTriangles = cloth.getNbCollisionTriangles();

	PxU32 sphereBytes = numSpheres * sizeof(PxClothCollisionSphere);
	PxU32 pairBytes = numIndices * sizeof(PxU32);
	PxU32 planesBytes = numPlanes * sizeof(PxClothCollisionPlane);
	PxU32 convexBytes = numConvexes * sizeof(PxU32);
	PxU32 triangleBytes = numTriangles * sizeof(PxClothCollisionTriangle);

	mBindingData->mTempU8Array.resize(sphereBytes + pairBytes + planesBytes + convexBytes + triangleBytes);
	PxU8* bufferStart = mBindingData->mTempU8Array.begin();
	PxClothCollisionSphere* spheresBuffer = reinterpret_cast<PxClothCollisionSphere*>(mBindingData->mTempU8Array.begin());
	PxU32* indexBuffer = reinterpret_cast<PxU32*>(spheresBuffer + numSpheres);
	PxClothCollisionPlane* planeBuffer = reinterpret_cast<PxClothCollisionPlane*>(indexBuffer + numIndices);
	PxU32* convexBuffer = reinterpret_cast<PxU32*>(planeBuffer + numPlanes);
	PxClothCollisionTriangle* trianglesBuffer = reinterpret_cast<PxClothCollisionTriangle*>(convexBuffer + numConvexes);

	cloth.getCollisionData(spheresBuffer, indexBuffer, planeBuffer, convexBuffer, trianglesBuffer);
	inStream.setPropertyValue(&cloth, "CollisionSpheres", DataRef<const PxU8>(bufferStart, sphereBytes), getPvdNamespacedNameForType<PvdPositionAndRadius>());
	if(sendPairs)
		inStream.setPropertyValue(&cloth, "CollisionSpherePairs", DataRef<const PxU8>(bufferStart + sphereBytes, pairBytes), getPvdNamespacedNameForType<PxU32>());
}

// begin code to generate triangle mesh from cloth convex planes

namespace
{
PxReal det(PxVec4 v0, PxVec4 v1, PxVec4 v2, PxVec4 v3)
{
	const PxVec3& d0 = reinterpret_cast<const PxVec3&>(v0);
	const PxVec3& d1 = reinterpret_cast<const PxVec3&>(v1);
	const PxVec3& d2 = reinterpret_cast<const PxVec3&>(v2);
	const PxVec3& d3 = reinterpret_cast<const PxVec3&>(v3);

	return v0.w * d1.cross(d2).dot(d3) - v1.w * d0.cross(d2).dot(d3) + v2.w * d0.cross(d1).dot(d3) -
	       v3.w * d0.cross(d1).dot(d2);
}

PxVec3 intersect(PxVec4 p0, PxVec4 p1, PxVec4 p2)
{
	const PxVec3& d0 = reinterpret_cast<const PxVec3&>(p0);
	const PxVec3& d1 = reinterpret_cast<const PxVec3&>(p1);
	const PxVec3& d2 = reinterpret_cast<const PxVec3&>(p2);

	return (p0.w * d1.cross(d2) + p1.w * d2.cross(d0) + p2.w * d0.cross(d1)) / d0.dot(d2.cross(d1));
}

const PxU16 sInvalid = PxU16(-1);

// restriction: only supports a single patch per vertex.
struct HalfedgeMesh
{
	struct Halfedge
	{
		Halfedge(PxU16 vertex = sInvalid, PxU16 face = sInvalid, PxU16 next = sInvalid, PxU16 prev = sInvalid)
		: mVertex(vertex), mFace(face), mNext(next), mPrev(prev)
		{
		}

		PxU16 mVertex; // to
		PxU16 mFace;   // left
		PxU16 mNext;   // ccw
		PxU16 mPrev;   // cw
	};

	PxU16 findHalfedge(PxU16 v0, PxU16 v1)
	{
		PxU16 h = mVertices[v0], start = h;
		while(h != sInvalid && mHalfedges[h].mVertex != v1)
		{
			h = mHalfedges[PxU32(h ^ 1)].mNext;
			if(h == start)
				return sInvalid;
		}
		return h;
	}

	void connect(PxU16 h0, PxU16 h1)
	{
		mHalfedges[h0].mNext = h1;
		mHalfedges[h1].mPrev = h0;
	}

	void addTriangle(PxU16 v0, PxU16 v1, PxU16 v2)
	{
		// add new vertices
		PxU16 n = PxU16(PxMax(v0, PxMax(v1, v2)) + 1);
		if(mVertices.size() < n)
			mVertices.resize(n, sInvalid);

		// collect halfedges, prev and next of triangle
		PxU16 verts[] = { v0, v1, v2 };
		PxU16 handles[3], prev[3], next[3];
		for(PxU16 i = 0; i < 3; ++i)
		{
			PxU16 j = PxU16((i + 1) % 3);
			PxU16 h = findHalfedge(verts[i], verts[j]);
			if(h == sInvalid)
			{
				// add new edge
				h = Ps::to16(mHalfedges.size());
				mHalfedges.pushBack(Halfedge(verts[j]));
				mHalfedges.pushBack(Halfedge(verts[i]));
			}
			handles[i] = h;
			prev[i] = mHalfedges[h].mPrev;
			next[i] = mHalfedges[h].mNext;
		}

		// patch connectivity
		for(PxU16 i = 0; i < 3; ++i)
		{
			PxU16 j = PxU16((i + 1) % 3);

			mHalfedges[handles[i]].mFace = Ps::to16(mFaces.size());

			// connect prev and next
			connect(handles[i], handles[j]);

			if(next[j] == sInvalid) // new next edge, connect opposite
				connect(PxU16(handles[j] ^ 1), next[i] != sInvalid ? next[i] : PxU16(handles[i] ^ 1));

			if(prev[i] == sInvalid) // new prev edge, connect opposite
				connect(prev[j] != sInvalid ? prev[j] : PxU16(handles[j] ^ 1), PxU16(handles[i] ^ 1));

			// prev is boundary, update middle vertex
			if(mHalfedges[PxU32(handles[i] ^ 1)].mFace == sInvalid)
				mVertices[verts[j]] = PxU16(handles[i] ^ 1);
		}

		mFaces.pushBack(handles[2]);
	}

	PxU16 removeTriangle(PxU16 f)
	{
		PxU16 result = sInvalid;

		for(PxU16 i = 0, h = mFaces[f]; i < 3; ++i)
		{
			PxU16 v0 = mHalfedges[PxU32(h ^ 1)].mVertex;
			PxU16 v1 = mHalfedges[PxU32(h)].mVertex;

			mHalfedges[h].mFace = sInvalid;

			if(mHalfedges[PxU32(h ^ 1)].mFace == sInvalid) // was boundary edge, remove
			{
				// update halfedge connectivity
				connect(mHalfedges[h].mPrev, mHalfedges[PxU32(h ^ 1)].mNext);
				connect(mHalfedges[PxU32(h ^ 1)].mPrev, mHalfedges[h].mNext);

				// update vertex boundary or delete
				mVertices[v0] = mVertices[v0] == h ? sInvalid : mHalfedges[PxU32(h ^ 1)].mNext;
				mVertices[v1] = mVertices[v1] == (h ^ 1) ? sInvalid : mHalfedges[h].mNext;
			}
			else
			{
				mVertices[v0] = h; // update vertex boundary
				result = v1;
			}

			h = mHalfedges[h].mNext;
		}

		mFaces[f] = sInvalid;
		return result;
	}

	// true if vertex v is in front of face f
	bool visible(PxU16 v, PxU16 f)
	{
		PxU16 h = mFaces[f];
		if(h == sInvalid)
			return false;

		PxU16 v0 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;
		PxU16 v1 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;
		PxU16 v2 = mHalfedges[h].mVertex;
		h = mHalfedges[h].mNext;

		return det(mPoints[v], mPoints[v0], mPoints[v1], mPoints[v2]) < 0.0f;
	}

	shdfnd::Array<Halfedge> mHalfedges;
	shdfnd::Array<PxU16> mVertices; // vertex -> (boundary) halfedge
	shdfnd::Array<PxU16> mFaces;    // face -> halfedge
	shdfnd::Array<PxVec4> mPoints;
};
}

struct ConvexMeshBuilder
{
	ConvexMeshBuilder(const PxVec4* planes) : mPlanes(planes)
	{
	}

	void operator()(PxU32 mask, float scale = 1.0f);

	const PxVec4* mPlanes;
	shdfnd::Array<PxVec3> mVertices;
	shdfnd::Array<PxU16> mIndices;
};

void ConvexMeshBuilder::operator()(PxU32 planeMask, float scale)
{
	PxU16 numPlanes = Ps::to16(shdfnd::bitCount(planeMask));

	if(numPlanes == 1)
	{
		PxTransform t = PxTransformFromPlaneEquation(reinterpret_cast<const PxPlane&>(mPlanes[lowestSetBit(planeMask)]));

		if(!t.isValid())
			return;

		const PxU16 indices[] = { 0, 1, 2, 0, 2, 3 };
		const PxVec3 vertices[] = { PxVec3(0.0f, scale, scale),   PxVec3(0.0f, -scale, scale),
			                        PxVec3(0.0f, -scale, -scale), PxVec3(0.0f, scale, -scale) };

		PxU16 baseIndex = Ps::to16(mVertices.size());

		for(PxU32 i = 0; i < 4; ++i)
			mVertices.pushBack(t.transform(vertices[i]));

		for(PxU32 i = 0; i < 6; ++i)
			mIndices.pushBack(PxU16(indices[i] + baseIndex));

		return;
	}

	if(numPlanes < 4)
		return; // todo: handle degenerate cases

	HalfedgeMesh mesh;

	// gather points (planes, that is)
	mesh.mPoints.reserve(numPlanes);
	for(; planeMask; planeMask &= planeMask - 1)
		mesh.mPoints.pushBack(mPlanes[shdfnd::lowestSetBit(planeMask)]);

	// initialize to tetrahedron
	mesh.addTriangle(0, 1, 2);
	mesh.addTriangle(0, 3, 1);
	mesh.addTriangle(1, 3, 2);
	mesh.addTriangle(2, 3, 0);

	// flip if inside-out
	if(mesh.visible(3, 0))
		shdfnd::swap(mesh.mPoints[0], mesh.mPoints[1]);

	// iterate through remaining points
	for(PxU16 i = 4; i < mesh.mPoints.size(); ++i)
	{
		// remove any visible triangle
		PxU16 v0 = sInvalid;
		for(PxU16 j = 0; j < mesh.mFaces.size(); ++j)
		{
			if(mesh.visible(i, j))
				v0 = PxMin(v0, mesh.removeTriangle(j));
		}

		if(v0 == sInvalid)
			continue;

		// tesselate hole
		PxU16 start = v0;
		do
		{
			PxU16 h = mesh.mVertices[v0];
			PxU16 v1 = mesh.mHalfedges[h].mVertex;
			mesh.addTriangle(v0, v1, i);
			v0 = v1;
		} while(v0 != start);
	}

	// convert triangles to vertices (intersection of 3 planes)
	shdfnd::Array<PxU32> face2Vertex(mesh.mFaces.size());
	for(PxU32 i = 0; i < mesh.mFaces.size(); ++i)
	{
		face2Vertex[i] = mVertices.size();

		PxU16 h = mesh.mFaces[i];
		if(h == sInvalid)
			continue;

		PxU16 v0 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		PxU16 v1 = mesh.mHalfedges[h].mVertex;
		h = mesh.mHalfedges[h].mNext;
		PxU16 v2 = mesh.mHalfedges[h].mVertex;

		mVertices.pushBack(intersect(mesh.mPoints[v0], mesh.mPoints[v1], mesh.mPoints[v2]));
	}

	// convert vertices to polygons (face one-ring)
	for(PxU32 i = 0; i < mesh.mVertices.size(); ++i)
	{
		PxU16 h = mesh.mVertices[i];
		if(h == sInvalid)
			continue;

		PxU16 v0 = Ps::to16(face2Vertex[mesh.mHalfedges[h].mFace]);
		h = PxU16(mesh.mHalfedges[h].mPrev ^ 1);
		PxU16 v1 = Ps::to16(face2Vertex[mesh.mHalfedges[h].mFace]);

		while(true)
		{
			h = PxU16(mesh.mHalfedges[h].mPrev ^ 1);
			PxU16 v2 = Ps::to16(face2Vertex[mesh.mHalfedges[h].mFace]);

			if(v0 == v2)
				break;

			mIndices.pushBack(v0);
			mIndices.pushBack(v2);
			mIndices.pushBack(v1);

			v1 = v2;
		}
	}
}

void PvdMetaDataBinding::sendCollisionTriangles(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 numSpheres = cloth.getNbCollisionSpheres();
	PxU32 numIndices = 2 * cloth.getNbCollisionCapsules();
	PxU32 numPlanes = cloth.getNbCollisionPlanes();
	PxU32 numConvexes = cloth.getNbCollisionConvexes();
	PxU32 numTriangles = cloth.getNbCollisionTriangles();

	PxU32 sphereBytes = numSpheres * sizeof(PxClothCollisionSphere);
	PxU32 pairBytes = numIndices * sizeof(PxU32);
	PxU32 planesBytes = numPlanes * sizeof(PxClothCollisionPlane);
	PxU32 convexBytes = numConvexes * sizeof(PxU32);
	PxU32 triangleBytes = numTriangles * sizeof(PxClothCollisionTriangle);

	mBindingData->mTempU8Array.resize(sphereBytes + pairBytes + planesBytes + convexBytes + triangleBytes);
	PxU8* bufferStart = mBindingData->mTempU8Array.begin();
	PxClothCollisionSphere* spheresBuffer = reinterpret_cast<PxClothCollisionSphere*>(mBindingData->mTempU8Array.begin());
	PxU32* indexBuffer = reinterpret_cast<PxU32*>(spheresBuffer + numSpheres);
	PxClothCollisionPlane* planeBuffer = reinterpret_cast<PxClothCollisionPlane*>(indexBuffer + numIndices);
	PxU32* convexBuffer = reinterpret_cast<PxU32*>(planeBuffer + numPlanes);
	PxClothCollisionTriangle* trianglesBuffer = reinterpret_cast<PxClothCollisionTriangle*>(convexBuffer + numConvexes);

	cloth.getCollisionData(spheresBuffer, indexBuffer, planeBuffer, convexBuffer, trianglesBuffer);

	inStream.setPropertyValue(&cloth, "CollisionPlanes", DataRef<const PxU8>(bufferStart + sphereBytes + pairBytes, planesBytes), getPvdNamespacedNameForType<PvdPositionAndRadius>());
	inStream.setPropertyValue(&cloth, "CollisionConvexMasks", DataRef<const PxU8>(bufferStart + sphereBytes + pairBytes + planesBytes, convexBytes), getPvdNamespacedNameForType<PxU32>());
	inStream.setPropertyValue(&cloth, "CollisionTriangles", DataRef<const PxU8>(bufferStart + sphereBytes + pairBytes + planesBytes + convexBytes, triangleBytes), getPvdNamespacedNameForType<PxVec3>());
}

void PvdMetaDataBinding::sendVirtualParticles(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 numParticles = cloth.getNbVirtualParticles();
	PxU32 numWeights = cloth.getNbVirtualParticleWeights();
	PxU32 numIndexes = numParticles * 4;
	PxU32 numIndexBytes = numIndexes * sizeof(PxU32);
	PxU32 numWeightBytes = numWeights * sizeof(PxVec3);

	mBindingData->mTempU8Array.resize(PxMax(numIndexBytes, numWeightBytes));
	PxU8* dataStart = mBindingData->mTempU8Array.begin();

	PxU32* indexStart = reinterpret_cast<PxU32*>(dataStart);
	if(numIndexes)
		cloth.getVirtualParticles(indexStart);
	inStream.setPropertyValue(&cloth, "VirtualParticles", DataRef<const PxU8>(dataStart, numIndexBytes), getPvdNamespacedNameForType<PxU32>());

	PxVec3* weightStart = reinterpret_cast<PxVec3*>(dataStart);
	if(numWeights)
		cloth.getVirtualParticleWeights(weightStart);
	inStream.setPropertyValue(&cloth, "VirtualParticleWeights", DataRef<const PxU8>(dataStart, numWeightBytes), getPvdNamespacedNameForType<PxVec3>());
}
void PvdMetaDataBinding::sendSeparationConstraints(PvdDataStream& inStream, const PxCloth& cloth)
{
	PxU32 count = cloth.getNbSeparationConstraints();
	PxU32 byteSize = count * sizeof(PxClothParticleSeparationConstraint);
	mBindingData->mTempU8Array.resize(byteSize);
	if(count)
		cloth.getSeparationConstraints(reinterpret_cast<PxClothParticleSeparationConstraint*>(mBindingData->mTempU8Array.begin()));
	inStream.setPropertyValue(&cloth, "SeparationConstraints", mBindingData->tempToRef(), getPvdNamespacedNameForType<PvdPositionAndRadius>());
}
#endif // PX_USE_CLOTH_API

// per frame update

#if PX_USE_CLOTH_API

void PvdMetaDataBinding::updateCloths(PvdDataStream& inStream, const PxScene& inScene)
{
	PxU32 actorCount = inScene.getNbActors(PxActorTypeFlag::eCLOTH);
	if(actorCount == 0)
		return;
	mBindingData->mActors.resize(actorCount);
	PxActor** theActors = mBindingData->mActors.begin();
	inScene.getActors(PxActorTypeFlag::eCLOTH, theActors, actorCount);
	PX_COMPILE_TIME_ASSERT(sizeof(PxClothParticle) == sizeof(PxVec3) + sizeof(PxF32));
	for(PxU32 idx = 0; idx < actorCount; ++idx)
	{
		PxCloth* theCloth = static_cast<PxCloth*>(theActors[idx]);
		bool isSleeping = theCloth->isSleeping();
		bool wasSleeping = mBindingData->mSleepingActors.contains(theCloth);
		if(isSleeping == false || isSleeping != wasSleeping)
		{
			PxClothParticleData* theData = theCloth->lockParticleData();
			if(theData != NULL)
			{
				PxU32 numBytes = sizeof(PxClothParticle) * theCloth->getNbParticles();
				inStream.setPropertyValue(theCloth, "ParticleBuffer", DataRef<const PxU8>(reinterpret_cast<const PxU8*>(theData->particles), numBytes), getPvdNamespacedNameForType<PxClothParticle>());
				theData->unlock();
			}
		}
		if(isSleeping != wasSleeping)
		{
			inStream.setPropertyValue(theCloth, "IsSleeping", isSleeping);
			if(isSleeping)
				mBindingData->mSleepingActors.insert(theActors[idx]);
			else
				mBindingData->mSleepingActors.erase(theActors[idx]);
		}
	}
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxCloth& cloth, const PxScene& ownerScene)
{
	PxClothFabric* fabric = cloth.getFabric();
	if(fabric)
		inStream.removeObjectRef(fabric, "Cloths", &cloth);
	removeSceneGroupProperty(inStream, "Cloths", cloth, ownerScene);
}

#endif // PX_USE_CLOTH_API

#define ENABLE_AGGREGATE_PVD_SUPPORT 1
#ifdef ENABLE_AGGREGATE_PVD_SUPPORT

void PvdMetaDataBinding::createInstance(PvdDataStream& inStream, const PxAggregate& inObj, const PxScene& ownerScene)
{
	addSceneGroupProperty(inStream, "Aggregates", inObj, ownerScene);
	sendAllProperties(inStream, inObj);
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream& inStream, const PxAggregate& inObj)
{
	PxAggregateGeneratedValues values(&inObj);
	inStream.setPropertyMessage(&inObj, values);
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream& inStream, const PxAggregate& inObj, const PxScene& ownerScene)
{
	removeSceneGroupProperty(inStream, "Aggregates", inObj, ownerScene);
}

class ChangeOjectRefCmd : public PvdDataStream::PvdCommand
{
	ChangeOjectRefCmd& operator=(const ChangeOjectRefCmd&)
	{
		PX_ASSERT(0);
		return *this;
	} // PX_NOCOPY doesn't work for local classes
  public:
	const void*	mInstance;
	String		mPropName;
	const void*	mPropObj;
	const bool	mPushBack;

	ChangeOjectRefCmd(const void* inInst, String inName, const void* inObj, bool pushBack)
	: mInstance(inInst), mPropName(inName), mPropObj(inObj), mPushBack(pushBack)
	{
	}

	// Assigned is needed for copying
	ChangeOjectRefCmd(const ChangeOjectRefCmd& other)
	: PvdDataStream::PvdCommand(other), mInstance(other.mInstance), mPropName(other.mPropName), mPropObj(other.mPropObj), mPushBack(other.mPushBack)
	{
	}

	virtual bool canRun(PvdInstanceDataStream& inStream)
	{
		PX_ASSERT(inStream.isInstanceValid(mInstance));
		return inStream.isInstanceValid(mPropObj);
	}
	virtual void run(PvdInstanceDataStream& inStream)
	{
		if(!inStream.isInstanceValid(mInstance))
			return;

		if(mPushBack)
		{
			if(inStream.isInstanceValid(mPropObj))
				inStream.pushBackObjectRef(mInstance, mPropName, mPropObj);
		}
		else
		{
			// the called function will assert if propobj is already removed
			inStream.removeObjectRef(mInstance, mPropName, mPropObj);
		}
	}
};

static void changeAggregateSubActors(PvdDataStream& inStream, const PxAggregate& inObj, const PxActor& inActor, bool pushBack)
{
	const PxArticulationLink* link = inActor.is<PxArticulationLink>();
	String propName = NULL;
	const void* object = NULL;
	if(link == NULL)
	{
		propName = "Actors";
		object = &inActor;
	}
	else if(link->getInboundJoint() == NULL)
	{
		propName = "Articulations";
		object = &link->getArticulation();
	}
	else
		return;

	ChangeOjectRefCmd* cmd = PX_PLACEMENT_NEW(inStream.allocateMemForCmd(sizeof(ChangeOjectRefCmd)), ChangeOjectRefCmd)(&inObj, propName, object, pushBack);

	if(cmd->canRun(inStream))
		cmd->run(inStream);
	else
		inStream.pushPvdCommand(*cmd);
}
void PvdMetaDataBinding::detachAggregateActor(PvdDataStream& inStream, const PxAggregate& inObj, const PxActor& inActor)
{
	changeAggregateSubActors(inStream, inObj, inActor, false);
}

void PvdMetaDataBinding::attachAggregateActor(PvdDataStream& inStream, const PxAggregate& inObj, const PxActor& inActor)
{
	changeAggregateSubActors(inStream, inObj, inActor, true);
}
#else
void PvdMetaDataBinding::createInstance(PvdDataStream&, const PxAggregate&, const PxScene&, ObjectRegistrar&)
{
}

void PvdMetaDataBinding::sendAllProperties(PvdDataStream&, const PxAggregate&)
{
}

void PvdMetaDataBinding::destroyInstance(PvdDataStream&, const PxAggregate&, const PxScene&)
{
}

void PvdMetaDataBinding::detachAggregateActor(PvdDataStream&, const PxAggregate&, const PxActor&)
{
}

void PvdMetaDataBinding::attachAggregateActor(PvdDataStream&, const PxAggregate&, const PxActor&)
{
}
#endif

template <typename TDataType>
static void sendSceneArray(PvdDataStream& inStream, const PxScene& inScene, const Ps::Array<TDataType>& inArray, const char* propName)
{
	if(0 == inArray.size())
		inStream.setPropertyValue(&inScene, propName, DataRef<const PxU8>(), getPvdNamespacedNameForType<TDataType>());
	else
	{
		ScopedPropertyValueSender<TDataType, 32> sender(inStream, &inScene, propName);
		for(PxU32 i = 0; i < inArray.size(); ++i)
			sender.append(inArray[i]);
	}
}

static void sendSceneArray(PvdDataStream& inStream, const PxScene& inScene, const Ps::Array<PvdSqHit>& inArray, const char* propName)
{
	if(0 == inArray.size())
		inStream.setPropertyValue(&inScene, propName, DataRef<const PxU8>(), getPvdNamespacedNameForType<PvdSqHit>());
	else
	{
		ScopedPropertyValueSender<PvdSqHit, 32> sender(inStream, &inScene, propName);
		for(PxU32 i = 0; i < inArray.size(); ++i)
		{
			if(!inStream.isInstanceValid(inArray[i].mShape) || !inStream.isInstanceValid(inArray[i].mActor))
			{
				PvdSqHit hit = inArray[i];
				hit.mShape = NULL;
				hit.mActor = NULL;
				sender.append(hit);
			}
			else
				sender.append(inArray[i]);
		}
	}
}

void PvdMetaDataBinding::sendSceneQueries(PvdDataStream& inStream, const PxScene& inScene, PsPvd* pvd)
{
	if(!inStream.isConnected())
		return;

	const physx::NpScene& scene = static_cast<const NpScene&>(inScene);
	
	for(PxU32 i = 0; i < 2; i++)
	{
		PvdSceneQueryCollector& collector = ((i == 0) ? scene.getSingleSqCollector() : scene.getBatchedSqCollector());
		Ps::Mutex::ScopedLock lock(collector.getLock());

		String propName = collector.getArrayName(collector.mPvdSqHits);
		sendSceneArray(inStream, inScene, collector.mPvdSqHits, propName);

		propName = collector.getArrayName(collector.mPoses);
		sendSceneArray(inStream, inScene, collector.mPoses, propName);

		propName = collector.getArrayName(collector.mFilterData);
		sendSceneArray(inStream, inScene, collector.mFilterData, propName);

		const NamedArray<PxGeometryHolder>& geometriesToDestroy = collector.getPrevFrameGeometries();
		propName = collector.getArrayName(geometriesToDestroy);
		for(PxU32 k = 0; k < geometriesToDestroy.size(); ++k)
		{
			const PxGeometryHolder& inObj = geometriesToDestroy[k];
			inStream.removeObjectRef(&inScene, propName, &inObj);
			inStream.destroyInstance(&inObj);
		}
		const Ps::Array<PxGeometryHolder>& geometriesToCreate = collector.getCurrentFrameGeometries();
		for(PxU32 k = 0; k < geometriesToCreate.size(); ++k)
		{
			const PxGeometry& geometry = geometriesToCreate[k].any();
			switch(geometry.getType())
			{
#define SEND_PVD_GEOM_TYPE(enumType, TGeomType, TValueType)                         \
	case enumType:                                                                  \
	{                                                                               \
		const TGeomType& inObj = static_cast<const TGeomType&>(geometry);           \
		inStream.createInstance(getPvdNamespacedNameForType<TGeomType>(), &inObj);  \
		registrarPhysicsObject<TGeomType>(inStream, inObj, pvd);					\
		TValueType values(&inObj);                                                  \
		inStream.setPropertyMessage(&inObj, values);                                \
		inStream.pushBackObjectRef(&inScene, propName, &inObj);                     \
	}                                                                               \
	break;
				SEND_PVD_GEOM_TYPE(PxGeometryType::eBOX, PxBoxGeometry, PxBoxGeometryGeneratedValues)
				SEND_PVD_GEOM_TYPE(PxGeometryType::eSPHERE, PxSphereGeometry, PxSphereGeometryGeneratedValues)
				SEND_PVD_GEOM_TYPE(PxGeometryType::eCAPSULE, PxCapsuleGeometry, PxCapsuleGeometryGeneratedValues)
				SEND_PVD_GEOM_TYPE(PxGeometryType::eCONVEXMESH, PxConvexMeshGeometry, PxConvexMeshGeometryGeneratedValues)
#undef SEND_PVD_GEOM_TYPE
			case PxGeometryType::ePLANE:
			case PxGeometryType::eTRIANGLEMESH:
			case PxGeometryType::eHEIGHTFIELD:
			case PxGeometryType::eGEOMETRY_COUNT:
			case PxGeometryType::eINVALID:
				PX_ALWAYS_ASSERT_MESSAGE("unsupported scene query geometry type");
				break;
			}
		}
		collector.prepareNextFrameGeometries();

		propName = collector.getArrayName(collector.mAccumulatedRaycastQueries);
		sendSceneArray(inStream, inScene, collector.mAccumulatedRaycastQueries, propName);

		propName = collector.getArrayName(collector.mAccumulatedOverlapQueries);
		sendSceneArray(inStream, inScene, collector.mAccumulatedOverlapQueries, propName);

		propName = collector.getArrayName(collector.mAccumulatedSweepQueries);
		sendSceneArray(inStream, inScene, collector.mAccumulatedSweepQueries, propName);
	}
}
}
}

#endif
