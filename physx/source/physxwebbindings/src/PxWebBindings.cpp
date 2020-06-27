#include <emscripten.h>
#include <emscripten/bind.h>
#include "PxPhysicsAPI.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <PsSocket.h>
#include <unistd.h>
#include <chrono>
#include <ctime>

using namespace physx;
using namespace emscripten;

struct PxRaycastCallbackWrapper : public wrapper<PxRaycastCallback> {
  EMSCRIPTEN_WRAPPER(PxRaycastCallbackWrapper)
  PxAgain processTouches(const PxRaycastHit *buffer, PxU32 nbHits) {
    for (PxU32 i = 0; i < nbHits; i++) {
      bool again = call<PxAgain>("processTouches", buffer[i]);
      if (!again) {
        return false;
      }
    }
    return true;
  }
};

PxRaycastHit* allocateRaycastHitBuffers(PxU32 nb) {
  PxRaycastHit *myArray = new PxRaycastHit[nb];
  return myArray;
}

struct PxSimulationEventCallbackWrapper : public wrapper<PxSimulationEventCallback> {
  EMSCRIPTEN_WRAPPER(PxSimulationEventCallbackWrapper)
  void onConstraintBreak(PxConstraintInfo *, PxU32) {}
  void onWake(PxActor **, PxU32) {}
  void onSleep(PxActor **, PxU32) {}
  void onContact(const PxContactPairHeader &, const PxContactPair *pairs, PxU32 nbPairs) {
    for(PxU32 i=0; i < nbPairs; i++)
    {
      const PxContactPair& cp = pairs[i];

      if (cp.flags & (PxContactPairFlag::eREMOVED_SHAPE_0 | PxContactPairFlag::eREMOVED_SHAPE_1))
          continue;

      if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
        call<void>("onContactBegin", cp.shapes[0], cp.shapes[1]);
      } else if(cp.events & PxPairFlag::eNOTIFY_TOUCH_LOST) {
        call<void>("onContactEnd", cp.shapes[0], cp.shapes[1]);
      } else if(cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
        call<void>("onContactPersist", cp.shapes[0], cp.shapes[1]);
      }
    }
  }
  void onTrigger(PxTriggerPair *pairs, PxU32 count) {
    for(PxU32 i=0; i < count; i++)
    {
      const PxTriggerPair& tp = pairs[i];
      if (tp.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
          continue;

      if(tp.status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
        call<void>("onTriggerBegin", tp.triggerShape, tp.otherShape);
      } else if(tp.status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
        call<void>("onTriggerEnd", tp.triggerShape, tp.otherShape);
      }
    }
  }
  void onAdvance(const PxRigidBody *const *, const PxTransform *, const PxU32) {}
};

PxFilterFlags DefaultFilterShader(  
  PxFilterObjectAttributes attributes0, PxFilterData , 
  PxFilterObjectAttributes attributes1, PxFilterData ,
  PxPairFlags& pairFlags, const void* , PxU32 )
{
  if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
  {
    pairFlags = PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eDETECT_CCD_CONTACT;
    return PxFilterFlag::eDEFAULT;
  }
  pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_TOUCH_PERSISTS |PxPairFlag::eDETECT_CCD_CONTACT;
  return PxFilterFlag::eDEFAULT;
}

// TODO: Getting the  global PxDefaultSimulationFilterShader into javascript
// is problematic, so let's provide this custom factory function for now

PxSceneDesc *getDefaultSceneDesc(PxTolerancesScale &scale, int numThreads, PxSimulationEventCallback* callback)
{
  PxSceneDesc *sceneDesc = new PxSceneDesc(scale);
  sceneDesc->gravity = PxVec3(0.0f, -9.81f, 0.0f);
  sceneDesc->cpuDispatcher = PxDefaultCpuDispatcherCreate(numThreads);
  sceneDesc->filterShader = DefaultFilterShader;
  sceneDesc->simulationEventCallback = callback;
  sceneDesc->kineKineFilteringMode = PxPairFilteringMode::eKEEP;
  sceneDesc->staticKineFilteringMode = PxPairFilteringMode::eKEEP;
  sceneDesc->flags |= PxSceneFlag::eENABLE_CCD;
  return sceneDesc;
}

PxConvexMesh* createConvexMesh(std::vector<PxVec3>& vertices, PxCooking& cooking, PxPhysics& physics) {
  PxConvexMeshDesc convexDesc;
  convexDesc.points.count = vertices.size();
  convexDesc.points.stride = sizeof(PxVec3);
  convexDesc.points.data = vertices.data();
  convexDesc.flags        = PxConvexFlag::eCOMPUTE_CONVEX;

  PxConvexMesh* convexMesh = cooking.createConvexMesh(convexDesc, physics.getPhysicsInsertionCallback());

  return convexMesh;
}

PxConvexMesh* createConvexMeshFromBuffer(int vertices, PxU32 vertCount, PxCooking& cooking, PxPhysics& physics) {
  PxConvexMeshDesc convexDesc;
  convexDesc.points.count = vertCount;
  convexDesc.points.stride = sizeof(PxVec3);
  convexDesc.points.data = (PxVec3*)vertices;
  convexDesc.flags        = PxConvexFlag::eCOMPUTE_CONVEX;

  PxConvexMesh* convexMesh = cooking.createConvexMesh(convexDesc, physics.getPhysicsInsertionCallback());

  return convexMesh;
}

PxTriangleMesh* createTriMesh(int vertices, PxU32 vertCount, int indices, PxU32 indexCount, bool isU16, PxCooking& cooking, PxPhysics& physics) {
  PxTriangleMeshDesc meshDesc;
  meshDesc.points.count           = vertCount;
  meshDesc.points.stride          = sizeof(PxVec3);
  meshDesc.points.data            = (PxVec3*)vertices;

  meshDesc.triangles.count        = indexCount;
  if (isU16) {
    meshDesc.triangles.stride       = 3*sizeof(PxU16);
    meshDesc.triangles.data         = (PxU16*)indices;
    meshDesc.flags                  = PxMeshFlag::e16_BIT_INDICES;  
  } else {
    meshDesc.triangles.stride       = 3*sizeof(PxU32);
    meshDesc.triangles.data         = (PxU32*)indices;
  }

  PxTriangleMesh* triangleMesh = cooking.createTriangleMesh(meshDesc, physics.getPhysicsInsertionCallback());
  return triangleMesh;
}

EMSCRIPTEN_BINDINGS(physx)
{

  constant("PX_PHYSICS_VERSION", PX_PHYSICS_VERSION);

  // Global functions
  // These are generaly system/scene level initialization
  function("PxCreateFoundation", &PxCreateFoundation, allow_raw_pointers());
  function("PxInitExtensions", &PxInitExtensions, allow_raw_pointers());
  function("PxDefaultCpuDispatcherCreate", &PxDefaultCpuDispatcherCreate, allow_raw_pointers());
  function("PxCreatePvd", &PxCreatePvd, allow_raw_pointers());
  function("PxCreatePhysics", &PxCreateBasePhysics, allow_raw_pointers());
  function("PxCreateCooking", &PxCreateCooking, allow_raw_pointers());
  function("PxCreatePlane", &PxCreatePlane, allow_raw_pointers());
  function("getDefaultSceneDesc", &getDefaultSceneDesc, allow_raw_pointers());


  class_<PxSimulationEventCallback>("PxSimulationEventCallback")
      .allow_subclass<PxSimulationEventCallbackWrapper>("PxSimulationEventCallbackWrapper");

  // Joints
  function("PxFixedJointCreate", &PxFixedJointCreate, allow_raw_pointers());
  function("PxRevoluteJointCreate", &PxRevoluteJointCreate, allow_raw_pointers());
  function("PxSphericalJointCreate", &PxSphericalJointCreate, allow_raw_pointers());
  function("PxDistanceJointCreate", &PxDistanceJointCreate, allow_raw_pointers());
  function("PxPrismaticJointCreate", &PxPrismaticJointCreate, allow_raw_pointers());
  function("PxD6JointCreate", &PxD6JointCreate, allow_raw_pointers());

  class_<PxJoint>("PxJoint")
      .function("release", &PxJoint::release);
  class_<PxSphericalJoint, base<PxJoint>>("PxSphericalJoint");
  class_<PxRevoluteJoint, base<PxJoint>>("PxRevoluteJoint");
  class_<PxFixedJoint, base<PxJoint>>("PxFixedJoint");
  class_<PxDistanceJoint, base<PxJoint>>("PxDistanceJoint");
  class_<PxPrismaticJoint, base<PxJoint>>("PxPrismaticJoint");
  class_<PxD6Joint, base<PxJoint>>("PxD6Joint");

  
  class_<PxAllocatorCallback>("PxAllocatorCallback");
  class_<PxDefaultAllocator, base<PxAllocatorCallback>>("PxDefaultAllocator").constructor<>();
  class_<PxTolerancesScale>("PxTolerancesScale").constructor<>()
    .property("speed", &PxTolerancesScale::speed);

  // Define PxVec3, PxQuat and PxTransform as value objects to allow sumerian Vector3 and Quaternion to be used directly without the need to free the memory
  value_object<PxVec3>("PxVec3")
    .field("x", &PxVec3::x)
    .field("y", &PxVec3::y)
    .field("z", &PxVec3::z)
    ;
  register_vector<PxVec3>("PxVec3Vector");
  value_object<PxQuat>("PxQuat")
    .field("x", &PxQuat::x)
    .field("y", &PxQuat::y)
    .field("z", &PxQuat::z)
    .field("w", &PxQuat::w)
    ;
  value_object<PxTransform>("PxTransform")
    .field("translation", &PxTransform::p)
    .field("rotation", &PxTransform::q)
    ;

  enum_<PxIDENTITY>("PxIDENTITY")
      .value("PxIdentity", PxIDENTITY::PxIdentity);

  enum_<PxPvdInstrumentationFlag::Enum>("PxPvdInstrumentationFlag")
      .value("eALL", PxPvdInstrumentationFlag::Enum::eALL)
      .value("eDEBUG", PxPvdInstrumentationFlag::Enum::eDEBUG)
      .value("ePROFILE", PxPvdInstrumentationFlag::Enum::ePROFILE)
      .value("eMEMORY", PxPvdInstrumentationFlag::Enum::eMEMORY);

  enum_<PxForceMode::Enum>("PxForceMode")
      .value("eFORCE", PxForceMode::Enum::eFORCE)
      .value("eIMPULSE", PxForceMode::Enum::eIMPULSE)
      .value("eVELOCITY_CHANGE", PxForceMode::Enum::eVELOCITY_CHANGE)
      .value("eACCELERATION", PxForceMode::Enum::eACCELERATION);


  class_<PxSceneDesc>("PxSceneDesc").constructor<PxTolerancesScale>()
      .property("gravity", &PxSceneDesc::gravity);

  class_<PxFoundation>("PxFoundation").function("release", &PxFoundation::release);

  class_<PxSceneFlags>("PxSceneFlags");
  enum_<PxSceneFlag::Enum>("PxSceneFlag")
      .value("eENABLE_ACTIVE_ACTORS ", PxSceneFlag::Enum::eENABLE_ACTIVE_ACTORS)
      .value("eENABLE_CCD", PxSceneFlag::Enum::eENABLE_CCD)
      .value("eDISABLE_CCD_RESWEEP", PxSceneFlag::Enum::eDISABLE_CCD_RESWEEP)
      .value("eADAPTIVE_FORCE", PxSceneFlag::Enum::eADAPTIVE_FORCE)
      .value("eENABLE_PCM", PxSceneFlag::Enum::eENABLE_PCM)
      .value("eDISABLE_CONTACT_REPORT_BUFFER_RESIZE", PxSceneFlag::Enum::eDISABLE_CONTACT_REPORT_BUFFER_RESIZE)
      .value("eDISABLE_CONTACT_CACHE", PxSceneFlag::Enum::eDISABLE_CONTACT_CACHE)
      .value("eREQUIRE_RW_LOCK", PxSceneFlag::Enum::eREQUIRE_RW_LOCK)
      .value("eENABLE_STABILIZATION", PxSceneFlag::Enum::eENABLE_STABILIZATION)
      .value("eENABLE_AVERAGE_POINT", PxSceneFlag::Enum::eENABLE_AVERAGE_POINT)
      .value("eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS", PxSceneFlag::Enum::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS)
      .value("eENABLE_ENHANCED_DETERMINISM", PxSceneFlag::Enum::eENABLE_ENHANCED_DETERMINISM)
      .value("eENABLE_FRICTION_EVERY_ITERATION", PxSceneFlag::Enum::eENABLE_FRICTION_EVERY_ITERATION);

  class_<PxScene>("PxScene")
      .function("setGravity", &PxScene::setGravity)
      .function("getGravity", &PxScene::getGravity)
      .function("addActor", &PxScene::addActor, allow_raw_pointers())
      .function("removeActor", &PxScene::removeActor, allow_raw_pointers())
      .function("getScenePvdClient", &PxScene::getScenePvdClient, allow_raw_pointers())
      .function("getActors", &PxScene::getActors, allow_raw_pointers())
      .function("setVisualizationCullingBox", &PxScene::setVisualizationCullingBox)
      .function("simulate", optional_override(
                                [](PxScene &scene, PxReal elapsedTime, bool controlSimulation) {
                                  scene.simulate(elapsedTime, NULL, 0, 0, controlSimulation);
                                  return;
                                }))
      .function("fetchResults", optional_override(
                                    [](PxScene &scene, bool block) {
                                      // fetchResults uses an out pointer
                                      // which embind can't represent 
                                      // so let's override.
                                      bool fetched = scene.fetchResults(block);
                                      return fetched;
                                    }))
      .function("raycast", &PxScene::raycast, allow_raw_pointers());

  class_<PxLocationHit>("PxLocationHit")
      .property("position", &PxLocationHit::position)
      .property("normal", &PxLocationHit::normal)
      .property("distance", &PxLocationHit::distance);
  class_<PxRaycastHit, base<PxLocationHit>>("PxRaycastHit").constructor<>()
      .function("getShape", optional_override(
                                [](PxRaycastHit &block){
                                  return block.shape;
                                }), allow_raw_pointers());
  class_<PxRaycastCallback>("PxRaycastCallback")
      .property("block", &PxRaycastCallback::block)
      .property("hasBlock", &PxRaycastCallback::hasBlock)
      .allow_subclass<PxRaycastCallbackWrapper>("PxRaycastCallbackWrapper", constructor<PxRaycastHit*, PxU32>());
  class_<PxRaycastBuffer, base<PxRaycastCallback>>("PxRaycastBuffer").constructor<>();

  function("allocateRaycastHitBuffers", &allocateRaycastHitBuffers, allow_raw_pointers());

  class_<PxHitFlags>("PxHitFlags").constructor<int>();
  enum_<PxHitFlag::Enum>("PxHitFlag")
      .value("eDEFAULT", PxHitFlag::Enum::eDEFAULT)
      .value("eMESH_BOTH_SIDES", PxHitFlag::Enum::eMESH_BOTH_SIDES)
      .value("eMESH_MULTIPLE", PxHitFlag::Enum::eMESH_MULTIPLE);

  class_<PxQueryFilterData>("PxQueryFilterData").constructor<>()
      .property("flags", &PxQueryFilterData::flags);
  class_<PxQueryFlags>("PxQueryFlags").constructor<int>();
  enum_<PxQueryFlag::Enum>("PxQueryFlag")
      .value("eANY_HIT", PxQueryFlag::Enum::eANY_HIT)
      .value("eDYNAMIC", PxQueryFlag::Enum::eDYNAMIC)
      .value("eSTATIC", PxQueryFlag::Enum::eSTATIC)
      .value("eNO_BLOCK", PxQueryFlag::Enum::eNO_BLOCK);

  class_<PxQueryFilterCallback>("PxQueryFilterCallback");
  class_<PxQueryCache>("PxQueryCache");

  class_<PxMaterial>("PxMaterial")
      .function("release", &PxMaterial::release);
  register_vector<PxMaterial *>("VectorPxMaterial");
  // setMaterials has 'PxMaterial**' as an input, which is not representable with embind
  // This is overrided to use std::vector<PxMaterial*>
  class_<PxShape>("PxShape")
      .function("release", &PxShape::release)
      .function("getFlags", &PxShape::getFlags)
      .function("setFlag", &PxShape::setFlag)
      .function("setLocalPose", &PxShape::setLocalPose)
      .function("setGeometry", &PxShape::setGeometry)
      .function("getBoxGeometry", &PxShape::getBoxGeometry, allow_raw_pointers())
      .function("getSphereGeometry", &PxShape::getSphereGeometry, allow_raw_pointers())
      .function("getPlaneGeometry", &PxShape::getPlaneGeometry, allow_raw_pointers())
      .function("setMaterials", optional_override(
                                    [](PxShape &shape, std::vector<PxMaterial *> materials) {
                                      return shape.setMaterials(materials.data(), materials.size());
                                    }));

  class_<PxPhysics>("PxPhysics")
      .function("release", &PxPhysics::release)
      .function("getTolerancesScale", &PxPhysics::getTolerancesScale)
      .function("createScene", &PxPhysics::createScene, allow_raw_pointers())
      .function("createShape", select_overload<PxShape *(const PxGeometry &, const PxMaterial &, bool, PxShapeFlags)>(&PxPhysics::createShape), allow_raw_pointers())
      .function("createMaterial", &PxPhysics::createMaterial, allow_raw_pointers())
      .function("createRigidDynamic", &PxPhysics::createRigidDynamic, allow_raw_pointers())
      .function("createRigidStatic", &PxPhysics::createRigidStatic, allow_raw_pointers());

  class_<PxPvd>("PxPvd");

  class_<PxShapeFlags>("PxShapeFlags").constructor<int>().function("isSet", &PxShapeFlags::isSet);
  enum_<PxShapeFlag::Enum>("PxShapeFlag")
      .value("eSIMULATION_SHAPE", PxShapeFlag::Enum::eSIMULATION_SHAPE)
      .value("eSCENE_QUERY_SHAPE", PxShapeFlag::Enum::eSCENE_QUERY_SHAPE)
      .value("eTRIGGER_SHAPE", PxShapeFlag::Enum::eTRIGGER_SHAPE)
      .value("eVISUALIZATION", PxShapeFlag::Enum::eVISUALIZATION);

  enum_<PxActorFlag::Enum>("PxActorFlag")
      .value("eDISABLE_GRAVITY", PxActorFlag::Enum::eDISABLE_GRAVITY);

  class_<PxErrorCallback>("PxErrorCallback");
  class_<PxDefaultErrorCallback, base<PxErrorCallback>>("PxDefaultErrorCallback").constructor<>();

  class_<PxCooking>("PxCooking")
      .function("createConvexMesh", optional_override(
                                        [](PxCooking& cooking, std::vector<PxVec3>& vertices, PxPhysics& physics) {
                                          return createConvexMesh(vertices, cooking, physics);
                                        }), allow_raw_pointers())
      .function("createConvexMeshFromBuffer", optional_override(
                                        [](PxCooking& cooking, int vertices, PxU32 vertCount, PxPhysics& physics) {
                                          return createConvexMeshFromBuffer(vertices, vertCount, cooking, physics);
                                        }), allow_raw_pointers())
      .function("createTriMesh", optional_override(
                                        [](PxCooking& cooking, int vertices, PxU32 vertCount, int indices, PxU32 indexCount, bool isU16, PxPhysics& physics) {
                                          return createTriMesh(vertices, vertCount, indices, indexCount, isU16, cooking, physics);
                                        }), allow_raw_pointers());
  class_<PxCookingParams>("PxCookingParams").constructor<PxTolerancesScale>();
  class_<PxCpuDispatcher>("PxCpuDispatcher");
  class_<PxBVHStructure>("PxBVHStructure");
  class_<PxBaseTask>("PxBaseTask");
  class_<PxDefaultCpuDispatcher, base<PxCpuDispatcher>>("PxDefaultCpuDispatcher");

  class_<PxFilterData>("PxFilterData");
  class_<PxPairFlags>("PxPairFlags");
  class_<PxFilterFlags>("PxFilterFlags");

  enum_<PxPairFlag::Enum>("PxPairFlag");
  enum_<PxFilterFlag::Enum>("PxFilterFlag");

  class_<PxActor>("PxActor")
      .function("setActorFlag", &PxActor::setActorFlag)
      .function("release", &PxActor::release);

  class_<PxRigidActor, base<PxActor>>("PxRigidActor")
      .function("attachShape", &PxRigidActor::attachShape)
      .function("detachShape", &PxRigidActor::detachShape)
      .function("getGlobalPose", &PxRigidActor::getGlobalPose, allow_raw_pointers())
      .function("setGlobalPose", &PxRigidActor::setGlobalPose, allow_raw_pointers());

  class_<PxRigidBody, base<PxRigidActor>>("PxRigidBody")
      .function("setAngularDamping", &PxRigidBody::setAngularDamping)
      .function("getAngularDamping", &PxRigidBody::getAngularDamping)
      .function("setLinearDamping", &PxRigidBody::setLinearDamping)
      .function("getLinearDamping", &PxRigidBody::getLinearDamping)
      .function("setAngularVelocity", &PxRigidBody::setAngularVelocity)
      .function("getAngularVelocity", &PxRigidBody::getAngularVelocity)
      .function("setMass", &PxRigidBody::setMass)
      .function("setLinearVelocity", &PxRigidBody::setLinearVelocity)
      .function("getLinearVelocity", &PxRigidBody::getLinearVelocity)
      .function("addForceAtPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &force, const PxVec3 &pos) {
                                  PxRigidBodyExt::addForceAtPos(body, force, pos, PxForceMode::eFORCE, true);
                                }))
      .function("addForceAtLocalPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &force, const PxVec3 &pos) {
                                  PxRigidBodyExt::addForceAtLocalPos(body, force, pos, PxForceMode::eFORCE, true);
                                }))
      .function("addLocalForceAtLocalPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &force, const PxVec3 &pos) {
                                  PxRigidBodyExt::addLocalForceAtLocalPos(body, force, pos, PxForceMode::eFORCE, true);
                                }))
      .function("addImpulseAtPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &impulse, const PxVec3 &pos) {
                                  PxRigidBodyExt::addForceAtPos(body, impulse, pos, PxForceMode::eIMPULSE, true);
                                }))
      .function("addImpulseAtLocalPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &impulse, const PxVec3 &pos) {
                                  PxRigidBodyExt::addForceAtLocalPos(body, impulse, pos, PxForceMode::eIMPULSE, true);
                                }))
      .function("addLocalImpulseAtLocalPos", optional_override(
                                [](PxRigidBody &body, const PxVec3 &impulse, const PxVec3 &pos) {
                                  PxRigidBodyExt::addLocalForceAtLocalPos(body, impulse, pos, PxForceMode::eIMPULSE, true);
                                }))
      .function("addTorque", optional_override(
                                [](PxRigidBody &body, const PxVec3 &torque) {
                                  body.addTorque(torque, PxForceMode::eFORCE, true);
                                }))
      .function("setRigidBodyFlag", &PxRigidBody::setRigidBodyFlag)
      .function("getRigidBodyFlags", optional_override(
                                [](PxRigidBody &body) {
                                  return (bool)(body.getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC);
                                }))
      .function("setMassAndUpdateInertia", optional_override(
                                [](PxRigidBody &body, PxReal mass) {
                                  return PxRigidBodyExt::setMassAndUpdateInertia(body, mass, NULL, false);
                                }))
      .function("setMassSpaceInertiaTensor", &PxRigidBody::setMassSpaceInertiaTensor);

  class_<PxRigidBodyFlags>("PxRigidBodyFlags");
  enum_<PxRigidBodyFlag::Enum>("PxRigidBodyFlag")
      .value("eKINEMATIC", PxRigidBodyFlag::Enum::eKINEMATIC)
      .value("eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES", PxRigidBodyFlag::Enum::eUSE_KINEMATIC_TARGET_FOR_SCENE_QUERIES)
      .value("eENABLE_CCD", PxRigidBodyFlag::Enum::eENABLE_CCD)
      .value("eENABLE_CCD_FRICTION", PxRigidBodyFlag::Enum::eENABLE_CCD_FRICTION)
      .value("eENABLE_POSE_INTEGRATION_PREVIEW", PxRigidBodyFlag::Enum::eENABLE_POSE_INTEGRATION_PREVIEW)
      .value("eENABLE_SPECULATIVE_CCD", PxRigidBodyFlag::Enum::eENABLE_SPECULATIVE_CCD)
      .value("eENABLE_CCD_MAX_CONTACT_IMPULSE", PxRigidBodyFlag::Enum::eENABLE_CCD_MAX_CONTACT_IMPULSE)
      .value("eRETAIN_ACCELERATIONS", PxRigidBodyFlag::Enum::eRETAIN_ACCELERATIONS);

  class_<PxRigidStatic, base<PxRigidActor>>("PxRigidStatic");
  class_<PxRigidDynamic, base<PxRigidBody>>("PxRigidDynamic")
      .function("wakeUp", &PxRigidDynamic::wakeUp)
      .function("setWakeCounter", &PxRigidDynamic::setWakeCounter)
      .function("isSleeping", &PxRigidDynamic::isSleeping)
      .function("getWakeCounter", &PxRigidDynamic::getWakeCounter)
      .function("setSleepThreshold", &PxRigidDynamic::setSleepThreshold)
      .function("getSleepThreshold", &PxRigidDynamic::getSleepThreshold)
      .function("setKinematicTarget", &PxRigidDynamic::setKinematicTarget)
      .function("setRigidDynamicLockFlags", &PxRigidDynamic::setRigidDynamicLockFlags);
  class_<PxRigidDynamicLockFlags>("PxRigidDynamicLockFlags").constructor<int>();
  enum_<PxRigidDynamicLockFlag::Enum>("PxRigidDynamicLockFlag")
      .value("eLOCK_LINEAR_X", PxRigidDynamicLockFlag::Enum::eLOCK_LINEAR_X)
      .value("eLOCK_LINEAR_Y", PxRigidDynamicLockFlag::Enum::eLOCK_LINEAR_Y)
      .value("eLOCK_LINEAR_Z", PxRigidDynamicLockFlag::Enum::eLOCK_LINEAR_Z)
      .value("eLOCK_ANGULAR_X", PxRigidDynamicLockFlag::Enum::eLOCK_ANGULAR_X)
      .value("eLOCK_ANGULAR_Y", PxRigidDynamicLockFlag::Enum::eLOCK_ANGULAR_Y)
      .value("eLOCK_ANGULAR_Z", PxRigidDynamicLockFlag::Enum::eLOCK_ANGULAR_Z);

  /** Geometry **/
  class_<PxGeometry>("PxGeometry");
  class_<PxBoxGeometry, base<PxGeometry>>("PxBoxGeometry").constructor<>().constructor<float, float, float>().function("isValid", &PxBoxGeometry::isValid).property("halfExtents", &PxBoxGeometry::halfExtents);
  class_<PxSphereGeometry, base<PxGeometry>>("PxSphereGeometry").constructor<>().constructor<float>().function("isValid", &PxSphereGeometry::isValid);

  class_<PxCapsuleGeometry, base<PxGeometry>>("PxCapsuleGeometry").constructor<float, float>();

  class_<PxTriangleMesh>("PxTriangleMesh")
        .function("release", &PxTriangleMesh::release);
  class_<PxTriangleMeshGeometry, base<PxGeometry>>("PxTriangleMeshGeometry").constructor<PxTriangleMesh*, const PxMeshScale&, PxMeshGeometryFlags>();

  class_<PxMeshGeometryFlags>("PxMeshGeometryFlags").constructor<int>();
  enum_<PxMeshGeometryFlag::Enum>("PxMeshGeometryFlag")
  .value("eDOUBLE_SIDED", PxMeshGeometryFlag::Enum::eDOUBLE_SIDED);

  class_<PxPlaneGeometry, base<PxGeometry>>("PxPlaneGeometry").constructor<>();

  class_<PxConvexMesh>("PxConvexMesh")
      .function("release", &PxConvexMesh::release);
  class_<PxConvexMeshGeometry, base<PxGeometry>>("PxConvexMeshGeometry").constructor<PxConvexMesh*, const PxMeshScale&, PxConvexMeshGeometryFlags>();

  class_<PxMeshScale>("PxMeshScale").constructor<const PxVec3&, const PxQuat&>();

  class_<PxConvexMeshGeometryFlags>("PxConvexMeshGeometryFlags").constructor<int>();
  enum_<PxConvexMeshGeometryFlag::Enum>("PxConvexMeshGeometryFlag")
  .value("eTIGHT_BOUNDS", PxConvexMeshGeometryFlag::Enum::eTIGHT_BOUNDS);

  /** End Geometry **/

  class_<PxPlane>("PxPlane").constructor<float, float, float, float>();
}


namespace emscripten
{
namespace internal
{
// Physx uses private destructors all over the place for its own reference counting
// embind doesn't deal with this well, so we have to override the destructors to keep them private 
// in the bindings
// See: https://github.com/emscripten-core/emscripten/issues/5587
template <>
void raw_destructor<PxFoundation>(PxFoundation *)
{ /* do nothing */
}
template <>
void raw_destructor<PxPvd>(PxPvd *)
{ /* do nothing */
}
template <>
void raw_destructor<PxPvdTransport>(PxPvdTransport *)
{ /* do nothing */
}
template <>
void raw_destructor<PxMaterial>(PxMaterial *)
{ /* do nothing */
}
template <>
void raw_destructor<PxScene>(PxScene *)
{ /* do nothing */
}
template <>
void raw_destructor<PxRigidDynamic>(PxRigidDynamic *)
{ /* do nothing */
}
template <>
void raw_destructor<PxRigidBody>(PxRigidBody *)
{ /* do nothing */
}
template <>
void raw_destructor<PxRigidActor>(PxRigidActor *)
{ /* do nothing */
}
template <>
void raw_destructor<PxActor>(PxActor *)
{ /* do nothing */
}
template <>
void raw_destructor<PxShape>(PxShape *)
{ /* do nothing */
}
template <>
void raw_destructor<PxBVHStructure>(PxBVHStructure *)
{ /* do nothing */
}
template <>
void raw_destructor<PxRigidStatic>(PxRigidStatic *)
{ /* do nothing */
}
template <>
void raw_destructor<PxJoint>(PxJoint *)
{ /* do nothing */
}
template <>
void raw_destructor<PxPvdSceneClient>(PxPvdSceneClient *)
{ /* do nothing */
}
template<> 
void raw_destructor<PxCooking>(PxCooking *) 
{ /* do nothing */ 
}
template<> 
void raw_destructor<PxConvexMesh>(PxConvexMesh *) 
{ /* do nothing */ 
}
template<> 
void raw_destructor<PxTriangleMesh>(PxTriangleMesh *) 
{ /* do nothing */ 
}
} // namespace internal
} // namespace emscripten
