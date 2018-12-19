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


// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.
#include "ExportDynamicSystem.h"
#include "MeshImport.h"
#include "MiFloatMath.h"
#include "MiMemoryBuffer.h"
#include "MiIOStream.h"
#include "MiFileInterface.h"

#pragma warning(disable:4100)

namespace mimp
{

const char * getMotionString(MiF32 v)
{
	const char *ret = "LOCKED";
	if ( v!=0 )
	{
		ret = "LIMITED";
	}
	return ret;
}

const char * getMotionString(MiF32 v1,MiF32 v2)
{
	const char *ret = "LOCKED";
	if ( v1!=0 || v2 != 0)
	{
		ret = "LIMITED";
	}
	return ret;
}

void saveBodyPairCollision(const MeshSystem &ms,FILE_INTERFACE *fph)
{
	fi_fprintf(fph,"<array name=\"BodyPairCollisions\" size=\"%d\" type=\"Struct\">\r\n", ms.mMeshPairCollisionCount);
	for (MiU32 i=0; i<ms.mMeshPairCollisionCount; i++)
	{
		MeshPairCollision &m = ms.mMeshPairCollisions[i];
		fi_fprintf(fph,"<struct>\r\n");
		fi_fprintf(fph,"<value name=\"ObjectA\" type=\"String\">%s</value>\r\n", m.mBody1 );
		fi_fprintf(fph,"<value name=\"ObjectB\" type=\"String\">%s</value>\r\n", m.mBody2 );
		fi_fprintf(fph,"</struct>\r\n");
	}
	fi_fprintf(fph,"</array>\r\n");
}

const char * getJointType(const MeshSimpleJoint &msj)
{
	const char *ret;

	if ( strlen(msj.mName) == 0 )
	{
		ret = "NONE";
	}
	else if ( msj.mSwing1 == 0 && msj.mSwing2 == 0 )
	{
		if ( msj.mTwistLow == 0 && msj.mTwistHigh == 0 )
		{
			ret = "FIXED";
		}
		else
		{
			ret = "HINGE";
		}
	}
	else if ( msj.mTwistLow == 0 && msj.mTwistHigh == 0 )
	{
		ret = "SPHERICAL";
	}
	else
	{
		ret = "BALLSOCKET";
	}
	return ret;
}

void saveJoint(const MeshSimpleJoint &msj,FILE_INTERFACE *fph)
{
	fi_fprintf(fph,"<value name=\"Joint\" type=\"Ref\" included=\"1\" className=\"SimpleJoint\" version=\"0.0\">\r\n");
	fi_fprintf(fph,"<struct name=\"\">\r\n");
	fi_fprintf(fph,"<value name=\"JointType\" type=\"Enum\">%s</value>\r\n", getJointType(msj));
	fi_fprintf(fph,"<value name=\"JointUserString\" type=\"String\"></value>\r\n");
	fi_fprintf(fph,"<value name=\"ConstraintAxis\" type=\"U32\">0</value>\r\n");
	fi_fprintf(fph,"<value name=\"Xaxis\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n", msj.mXaxis[0], msj.mXaxis[1], msj.mXaxis[2]);
	fi_fprintf(fph,"<value name=\"Zaxis\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n", msj.mZaxis[0], msj.mZaxis[1], msj.mZaxis[2]);
	fi_fprintf(fph,"<value name=\"Anchor\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n", msj.mOffset[0], msj.mOffset[1], msj.mOffset[2]);
	fi_fprintf(fph,"<value name=\"TwistLow\" type=\"F32\">%0.9f</value>\r\n", msj.mTwistLow*FM_RAD_TO_DEG);
	fi_fprintf(fph,"<value name=\"TwistHigh\" type=\"F32\">%0.9f</value>\r\n", msj.mTwistHigh*FM_RAD_TO_DEG);
	fi_fprintf(fph,"<value name=\"Swing1\" type=\"F32\">%0.9f</value>\r\n", msj.mSwing1*FM_RAD_TO_DEG);
	fi_fprintf(fph,"<value name=\"Swing2\" type=\"F32\">%0.9f</value>\r\n", msj.mSwing2*FM_RAD_TO_DEG);
	fi_fprintf(fph,"</struct>\r\n");
	fi_fprintf(fph,"</value>\r\n");
}


MiI32 getJointParentIndex(const MeshSystem *ms,MiU32 i)
{
	MiI32 ret = -1;

	if ( ms->mMeshJointCount )
	{
		const char *rootName = ms->mMeshCollisionRepresentations[i]->mName;
		for (MiU32 j=0; j<ms->mMeshJointCount; j++)
		{
			const MeshSimpleJoint &msj = ms->mMeshJoints[j];
			if ( strcmp(msj.mName,rootName) == 0 )
			{
				ret = (MiI32)j;
				break;
			}
		}
	}
	return ret;
}

const char * getParentName(const MeshSystem *ms,MiU32 i)
{
	const char *ret = "NONE";

	MiI32 index = getJointParentIndex(ms,i);
	if ( index != -1 )
	{
		ret = ms->mMeshJoints[index].mParent;
	}

	return ret;
}

MiU32 getActorParentIndex(const MeshSystem *ms,MiU32 i)
{
	MiI32 ret = -1;
	const char *parentName = getParentName(ms,i);
	for (MiU32 i=0; i<ms->mMeshCollisionCount; i++)
	{
		const MeshCollisionRepresentation &mcr = *ms->mMeshCollisionRepresentations[i];
		if ( strcmp(mcr.mName,parentName) == 0 )
		{
			ret = (MiI32)i;
			break;
		}
	}
	return (MiU32)ret;
}


void *serializeDynamicSystemAuthoring(const MeshSystem *ms,MiU32 &len)
{
	void * ret = NULL;
	len = 0;

	if ( ms )
	{
		FILE_INTERFACE *fph = fi_fopen(ms->mAssetName,"wmem",NULL,0);
		if ( fph )
		{
			//



// ** Athoring

			fi_fprintf(fph,"<!DOCTYPE NvParameters>\r\n");
			fi_fprintf(fph,"<NvParameters numObjects=\"1\" version=\"1.0\" >\r\n");
			fi_fprintf(fph,"<value name=\"\" type=\"Ref\" className=\"DynamicSystemAssetAuthoringParams\" version=\"0.0\" >\r\n");
			fi_fprintf(fph,"<struct name=\"\">\r\n");
			fi_fprintf(fph,"<value name=\"Name\" type=\"String\">%s</value>\r\n", ms->mAssetName);
			fi_fprintf(fph,"<value name=\"ModelUserString\" null=\"1\" type=\"String\"></value>\r\n");
			fi_fprintf(fph,"<value name=\"ModelType\" type=\"Ref\" included=\"1\" className=\"ModelCollision\" version=\"0.0\" >\r\n");
			fi_fprintf(fph,"<struct name=\"\">\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"</value>\r\n");

			fi_fprintf(fph,"<value name=\"PositionSolverIterationCount\" type=\"U32\">30</value>\r\n");
			fi_fprintf(fph,"<value name=\"VelocitySolverIterationCount\" type=\"U32\">30</value>\r\n");

			fi_fprintf(fph,"<value name=\"RootBone\" type=\"Transform\">0 0 0 1  0 0 0 </value>\r\n");
			fi_fprintf(fph,"<array name=\"Materials\" size=\"1\" type=\"Struct\">\r\n");
			fi_fprintf(fph,"<struct>\r\n");
			fi_fprintf(fph,"<value name=\"Name\" type=\"String\">dummy_material</value>\r\n");
			fi_fprintf(fph,"<value name=\"IncludeTriangles\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"</array>\r\n");
			fi_fprintf(fph,"<array name=\"Skeleton\" size=\"%d\" type=\"Struct\">\r\n", ms->mMeshCollisionCount);

			for (MiU32 i=0; i<ms->mMeshCollisionCount; i++)
			{
				MeshCollisionRepresentation &mr = *ms->mMeshCollisionRepresentations[i];
				// For each Bone
				fi_fprintf(fph,"<struct>\r\n");
				fi_fprintf(fph,"<value name=\"Name\" type=\"String\">%s</value>\r\n", mr.mName);
				fi_fprintf(fph,"<value name=\"BoneRule\" type=\"Enum\">USER_INCLUDE</value>\r\n");
				fi_fprintf(fph,"<struct name=\"MeshStats\">\r\n");
				fi_fprintf(fph,"<value name=\"VertexCount\" type=\"U32\">0</value>\r\n");
				fi_fprintf(fph,"<value name=\"TriangleCount\" type=\"U32\">0</value>\r\n");
				fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">0</value>\r\n");
				fi_fprintf(fph,"</struct>\r\n");

				fi_fprintf(fph,"<value name=\"Transform\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f </value>\r\n",
					mr.mOrientation[0],mr.mOrientation[1],mr.mOrientation[2],mr.mOrientation[3],
					mr.mPosition[0],mr.mPosition[1],mr.mPosition[2]);
				
				fi_fprintf(fph,"<value name=\"ParentIndex\" type=\"I32\">%d</value>\r\n", getActorParentIndex(ms,i));

				fi_fprintf(fph,"</struct>\r\n");
			}
			fi_fprintf(fph,"</array>\r\n");

			fi_fprintf(fph,"<array name=\"DynamicSystemSkeleton\" size=\"%d\" type=\"Ref\">\r\n", ms->mMeshCollisionCount);
			for (MiU32 i=0; i<ms->mMeshCollisionCount; i++)
			{
				// For each bone
				MeshCollisionRepresentation &mr = *ms->mMeshCollisionRepresentations[i];
				fi_fprintf(fph,"<value type=\"Ref\" included=\"1\" className=\"PrunedBone\" version=\"0.0\" >\r\n");
				fi_fprintf(fph,"<struct name=\"\">\r\n");
				fi_fprintf(fph,"<value name=\"Name\" type=\"String\">%s</value>\r\n", mr.mName);
				fi_fprintf(fph,"<value name=\"ParentName\" type=\"String\">%s</value>\r\n", getParentName(ms,i));
				fi_fprintf(fph,"<value name=\"BoneUserString\" null=\"1\" type=\"String\"></value>\r\n");
				fi_fprintf(fph,"<value name=\"Locked\" type=\"Bool\">false</value>\r\n");

				MeshCollision &mc = *(mr.mCollisionGeometry[0]);

				switch ( mc.getType() )
				{
					case MCT_SPHERE:
						{
							MeshCollisionSphere &mcb = *(static_cast< MeshCollisionSphere *>(&mc));
							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"SphereStatistics\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<value name=\"Radius\" type=\"F32\">%0.9f</value>\r\n", mcb.mRadius);
							fi_fprintf(fph,"<value name=\"Center\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n",
								mcb.mLocalPosition[0],mcb.mLocalPosition[1],mcb.mLocalPosition[2]);
							fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">%0.9f</value>\r\n", 3.141592654f*mcb.mRadius*mcb.mRadius);
							fi_fprintf(fph,"<value name=\"ScaleRadius\" type=\"F32\">1</value>\r\n");
							fi_fprintf(fph,"<struct name=\"SphereOrientation\">\r\n");
							fi_fprintf(fph,"<value name=\"RotateX\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"RotateY\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"RotateZ\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"<struct name=\"SphereTranslation\">\r\n");
							fi_fprintf(fph,"<value name=\"TranslateX\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"TranslateY\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"TranslateZ\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");
						}
						break;
					case MCT_BOX:
						{
							MeshCollisionBox &mcb = *(static_cast< MeshCollisionBox *>(&mc));
							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"BoxStatistics\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<value name=\"Sides\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n",
								mcb.mSides[0],mcb.mSides[1],mcb.mSides[2]);
							fi_fprintf(fph,"<value name=\"Transform\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f</value>\r\n",
								mcb.mLocalOrientation[0],mcb.mLocalOrientation[1],mcb.mLocalOrientation[2],mcb.mLocalOrientation[3],
								mcb.mLocalPosition[0],mcb.mLocalPosition[1],mcb.mLocalPosition[2]);

							fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">0</value>\r\n");

							fi_fprintf(fph,"<struct name=\"BoxScaling\">\r\n");
							fi_fprintf(fph,"<value name=\"UniformScale\" type=\"F32\">1</value>\r\n");
							fi_fprintf(fph,"<value name=\"ScaleX\" type=\"F32\">1</value>\r\n");
							fi_fprintf(fph,"<value name=\"ScaleY\" type=\"F32\">1</value>\r\n");
							fi_fprintf(fph,"<value name=\"ScaleZ\" type=\"F32\">1</value>\r\n");
							fi_fprintf(fph,"</struct>\r\n");

							fi_fprintf(fph,"<struct name=\"BoxOrientation\">\r\n");
							fi_fprintf(fph,"<value name=\"RotateX\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"RotateY\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"RotateZ\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"<struct name=\"BoxTranslation\">\r\n");

							fi_fprintf(fph,"<value name=\"TranslateX\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"TranslateY\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"TranslateZ\" type=\"F32\">0</value>\r\n");

							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");
						}
						break;
					case MCT_CONVEX:
						{
							//MeshCollisionConvex &mcc = *(static_cast< MeshCollisionConvex *>(&mc));
							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"HullStatistics\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<struct name=\"MeshStats\">\r\n");
							fi_fprintf(fph,"<value name=\"VertexCount\" type=\"U32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"TriangleCount\" type=\"U32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"MaxHullVertices\" type=\"U32\">64</value>\r\n");
							fi_fprintf(fph,"<value name=\"ApproximationMethod\" type=\"Enum\">SINGLE_HULL</value>\r\n");
							fi_fprintf(fph,"<value name=\"Concavity\" type=\"F32\">0.200000003</value>\r\n");
							fi_fprintf(fph,"<value name=\"MaxHACDConvexHullCount\" type=\"U32\">64</value>\r\n");
							fi_fprintf(fph,"<value name=\"MaxMergeConvexHullCount\" type=\"U32\">128</value>\r\n");
							fi_fprintf(fph,"<value name=\"SmallClusterThreshold\" type=\"F32\">0</value>\r\n");
							fi_fprintf(fph,"<value name=\"BackFaceDistanceFactor\" type=\"F32\">0.00999999978</value>\r\n");
							fi_fprintf(fph,"<value name=\"DecompositionDepth\" type=\"U32\">5</value>\r\n");
							fi_fprintf(fph,"<value name=\"ClampMin\" type=\"Vec3\">0 0 0</value>\r\n");
							fi_fprintf(fph,"<value name=\"ClampMax\" type=\"Vec3\">1 1 1</value>\r\n");
							fi_fprintf(fph,"<array name=\"ConvexShapes\" size=\"0\" type=\"Ref\"></array>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");
						}
						break;
					default:
						assert(0); // not yet implemented!
						break;
				}

				fi_fprintf(fph,"<value name=\"SimulationFilterData\" type=\"String\">default</value>\r\n");
				fi_fprintf(fph,"<value name=\"QueryFilterData\" type=\"String\">default</value>\r\n");
				fi_fprintf(fph,"<value name=\"Chassis\" type=\"Bool\">false</value>\r\n");
				fi_fprintf(fph,"<value name=\"Wheel\" type=\"Ref\" included=\"1\" className=\"NoWheel\" version=\"0.0\" >\r\n");
				fi_fprintf(fph,"<struct name=\"\">\r\n");
				fi_fprintf(fph,"</struct>\r\n");
				fi_fprintf(fph,"</value>\r\n");

				MiI32 jointIndex = getJointParentIndex(ms,i);
				if ( jointIndex == - 1 )
				{
					fi_fprintf(fph,"<value name=\"Joint\" type=\"Ref\" included=\"1\" className=\"JointNone\" version=\"0.0\">\r\n");
					fi_fprintf(fph,"	<struct name=\"\">\r\n");
					fi_fprintf(fph,"	</struct>\r\n");
					fi_fprintf(fph,"</value>\r\n");
				}
				else
				{
					saveJoint(ms->mMeshJoints[jointIndex],fph);
				}

				fi_fprintf(fph,"<value name=\"CenterOfMass\" type=\"Vec3\">0 0 0</value>\r\n");
				fi_fprintf(fph,"<value name=\"Mass\" type=\"F32\">1</value>\r\n");
				fi_fprintf(fph,"<value name=\"PhysicsMaterialName\" type=\"String\">defaultMaterial</value>\r\n");
				fi_fprintf(fph,"<struct name=\"MeshStats\">\r\n");
				fi_fprintf(fph,"<value name=\"VertexCount\" type=\"U32\">0</value>\r\n");
				fi_fprintf(fph,"<value name=\"TriangleCount\" type=\"U32\">0</value>\r\n");
				fi_fprintf(fph,"<value name=\"Volume\" type=\"F32\">0</value>\r\n");
				fi_fprintf(fph,"</struct>\r\n");
				fi_fprintf(fph,"<value name=\"HIDDEN\" type=\"Bool\">false</value>\r\n");
				fi_fprintf(fph,"</struct>\r\n");
				fi_fprintf(fph,"</value>\r\n");
			}
			fi_fprintf(fph,"</array>\r\n");
			fi_fprintf(fph,"<struct name=\"CollisionSettings\">\r\n");
			fi_fprintf(fph,"<value name=\"OverallUniformPrimitiveScaling\" type=\"F32\">1</value>\r\n");
			fi_fprintf(fph,"<value name=\"NeighborDistance\" type=\"U32\">2</value>\r\n");
			fi_fprintf(fph,"<value name=\"MinimumVertices\" type=\"U32\">9</value>\r\n");
			fi_fprintf(fph,"<value name=\"MinimumPercentageVolume\" type=\"F32\">10</value>\r\n");
			fi_fprintf(fph,"<value name=\"SkinWidth\" type=\"F32\">0.00999999978</value>\r\n");
			fi_fprintf(fph,"<value name=\"BoxPercent\" type=\"F32\">90</value>\r\n");
			fi_fprintf(fph,"<value name=\"SpherePercent\" type=\"F32\">80</value>\r\n");
			fi_fprintf(fph,"<value name=\"CapsulePercent\" type=\"F32\">70</value>\r\n");
			fi_fprintf(fph,"<value name=\"FitBox\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"FitSphere\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"FitCapsule\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"FitConvex\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"<struct name=\"SDK_VISUALIZATION\">\r\n");
			fi_fprintf(fph,"<value name=\"WORLD_AXES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"BODY_AXES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"BODY_MASS_AXES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"BODY_LIN_VELOCITY\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"BODY_ANG_VELOCITY\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"BODY_JOINT_GROUPS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"CONTACT_POINT\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"CONTACT_ERROR\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"CONTACT_FORCE\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ACTOR_AXES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_AABBS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_SHAPES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_AXES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_COMPOUNDS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_FNORMALS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_EDGES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_STATIC\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_DYNAMIC\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"COLLISION_PAIRS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"JOINT_LOCAL_FRAMES\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"JOINT_LIMITS\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"<struct name=\"Visualization\">\r\n");
			fi_fprintf(fph,"<value name=\"RenderScale\" type=\"F32\">1</value>\r\n");
			fi_fprintf(fph,"<value name=\"GameScale\" type=\"F32\">1</value>\r\n");
			fi_fprintf(fph,"<value name=\"ZAxisUp\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"Gravity\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"Collision\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowCenterOfMass\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowWheels\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowWheelLocation\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowApplyTirePoint\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowSuspensionPoint\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowSkeleton\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowSkeletonNames\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowSourceMesh\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowCollisionVolumes\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"SourceMeshWireframe\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"CollisionVolumeWireframe\" type=\"Bool\">true</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowOnlySelectedMesh\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowOnlySelectedCollision\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowConstraintAxis\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"PhysXConstraintScale\" type=\"F32\">0.100000001</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowPhysXContacts\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"ShowVehicleMomentOfInertia\" type=\"Bool\">false</value>\r\n");
			fi_fprintf(fph,"<value name=\"WheelGraphChannel\" type=\"Enum\">NONE</value>\r\n");
			fi_fprintf(fph,"<value name=\"EngineGraphChannel\" type=\"Enum\">NONE</value>\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"<struct name=\"AnimationControls\">\r\n");
			fi_fprintf(fph,"<value name=\"PlayAnimation\" type=\"Enum\">STOP</value>\r\n");
			fi_fprintf(fph,"<value name=\"ScrollAnimation\" type=\"F32\">0</value>\r\n");
			fi_fprintf(fph,"<value name=\"PlaybackSpeed\" type=\"F32\">1</value>\r\n");
			fi_fprintf(fph,"</struct>\r\n");

			saveBodyPairCollision(*ms,fph);

			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"</value>\r\n");
			fi_fprintf(fph,"</NvParameters>\r\n");

			size_t outLen;
			void *outMem = fi_getMemBuffer(fph,&outLen);
			if ( outMem )
			{
				len = (MiU32)outLen;
				ret = MI_ALLOC(len);
				memcpy(ret,outMem,len);
			}
			fi_fclose(fph);
		}
	}
	return ret;
}



void *serializeDynamicSystem(const MeshSystem *ms,MiU32 &len)
{
	void * ret = NULL;
	len = 0;

	if ( ms )
	{
		FILE_INTERFACE *fph = fi_fopen(ms->mAssetName,"wmem",NULL,0);
		if ( fph )
		{
			//


// asset
			fi_fprintf(fph,"<!DOCTYPE NvParameters>\r\n");
			fi_fprintf(fph,"<NvParameters numObjects=\"1\" version=\"1.0\" >\r\n");
			fi_fprintf(fph,"<value name=\"\" type=\"Ref\" className=\"DynamicSystemAssetParam\" version=\"0.0\" >\r\n");
			fi_fprintf(fph,"<struct name=\"\">\r\n");
			fi_fprintf(fph,"<value name=\"Name\" type=\"String\">DynamicSystemAsset.1</value>\r\n");
			fi_fprintf(fph,"<value name=\"ModelUserString\" null=\"1\" type=\"String\"></value>\r\n");
			fi_fprintf(fph,"<value name=\"ModelType\" type=\"Ref\" included=\"1\" className=\"ModelCollision\" version=\"0.0\" >\r\n");
			fi_fprintf(fph,"<struct name=\"\">\r\n");
			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"</value>\r\n");
			fi_fprintf(fph,"<value name=\"PositionSolverIterationCount\" type=\"U32\">30</value>\r\n");
			fi_fprintf(fph,"<value name=\"VelocitySolverIterationCount\" type=\"U32\">30</value>\r\n");
			fi_fprintf(fph,"<value name=\"RootBone\" type=\"Transform\">0 0 0 1  0 0 0 </value>\r\n");
			fi_fprintf(fph,"<array name=\"DynamicSystemSkeleton\" size=\"%d\" type=\"Struct\">\r\n",ms->mMeshCollisionCount);
// For each bone
			for (MiU32 i=0; i<ms->mMeshCollisionCount; i++)
			{
				MeshCollisionRepresentation &mr = *ms->mMeshCollisionRepresentations[i];
				MeshCollision &mc = *(mr.mCollisionGeometry[0]);

				fi_fprintf(fph,"<struct>\r\n");
				fi_fprintf(fph,"<value name=\"Name\" type=\"String\">%s</value>\r\n", mr.mName);
				fi_fprintf(fph,"<value name=\"ParentBone\" type=\"String\">%s</value>\r\n", getParentName(ms,i));
				fi_fprintf(fph,"<value name=\"BoneUserString\" null=\"1\" type=\"String\"></value>\r\n");
				fi_fprintf(fph,"<value name=\"Mass\" type=\"F32\">1</value>\r\n");
				fi_fprintf(fph,"<value name=\"CenterOfMass\" type=\"Vec3\">0 0 0</value>\r\n");
				fi_fprintf(fph,"<value name=\"SimulationFilterData\" type=\"String\">default</value>\r\n");
				fi_fprintf(fph,"<value name=\"QueryFilterData\" type=\"String\">default</value>\r\n");
				fi_fprintf(fph,"<value name=\"Chassis\" type=\"Bool\">false</value>\r\n");
				fi_fprintf(fph,"<value name=\"Wheel\" type=\"Ref\" included=\"1\" className=\"NoWheel\" version=\"0.0\" >\r\n");
				fi_fprintf(fph,"<struct name=\"\">\r\n");
				fi_fprintf(fph,"</struct>\r\n");
				fi_fprintf(fph,"</value>\r\n");
				fi_fprintf(fph,"<value name=\"PhysicsMaterialName\" type=\"String\">defaultMaterial</value>\r\n");

				switch ( mc.getType() )
				{
					case MCT_SPHERE:
						{
							MeshCollisionSphere &mcb = *(static_cast<MeshCollisionSphere *>(&mc));

							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"DynamicSystemSphereShapeParams\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<value name=\"Radius\" type=\"F32\">%0.9f</value>\r\n", mcb.mRadius);

							fi_fprintf(fph,"<value name=\"LocalPose\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f</value>\r\n",
								mc.mLocalOrientation[0],mc.mLocalOrientation[1],mc.mLocalOrientation[2],mc.mLocalOrientation[3],
								mc.mLocalPosition[0],mc.mLocalPosition[1],mc.mLocalPosition[2]);

							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");

						}
						break;
					case MCT_BOX:
						{
							MeshCollisionBox &mcb = *(static_cast<MeshCollisionBox *>(&mc));
							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"DynamicSystemBoxShapeParams\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<value name=\"Sides\" type=\"Vec3\">%0.9f %0.9f %0.9f</value>\r\n",
								mcb.mSides[0],mcb.mSides[1],mcb.mSides[2]);

							fi_fprintf(fph,"<value name=\"LocalPose\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f</value>\r\n",
								mc.mLocalOrientation[0],mc.mLocalOrientation[1],mc.mLocalOrientation[2],mc.mLocalOrientation[3],
								mc.mLocalPosition[0],mc.mLocalPosition[1],mc.mLocalPosition[2]);

							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");
						}
						break;
					case MCT_CONVEX:
						{

							MeshCollisionConvex &mcc = *(static_cast<MeshCollisionConvex *>(&mc));

							fi_fprintf(fph,"<value name=\"CollisionShape\" type=\"Ref\" included=\"1\" className=\"DynamicSystemConvexShapeParams\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<array name=\"ConvexShapes\" size=\"1\" type=\"Ref\">\r\n");
							fi_fprintf(fph,"<value type=\"Ref\" included=\"1\" className=\"ConvexShape\" version=\"0.0\" >\r\n");
							fi_fprintf(fph,"<struct name=\"\">\r\n");
							fi_fprintf(fph,"<array name=\"Vertices\" size=\"%d\" type=\"Vec3\">\r\n", mcc.mVertexCount);

							for (MiU32 i=0; i<mcc.mVertexCount; i++)
							{
								const MiF32 *v = &mcc.mVertices[i*3];
								fi_fprintf(fph,"%0.9f %0.9f %0.9f,\r\n", v[0], v[1], v[2] );
							}

							fi_fprintf(fph,"</array>\r\n");

							if ( mcc.mTriCount )
							{
								fi_fprintf(fph,"<array name=\"Indices\" size=\"%d\" type=\"U32\">\r\n", mcc.mTriCount*3);
								for (MiU32 i=0; i<mcc.mTriCount; i++)
								{
									const MiU32 *tri = &mcc.mIndices[i*3];
									fi_fprintf(fph,"%d %d %d\r\n", tri[0],tri[1],tri[2]);
								}
								fi_fprintf(fph,"</array>\r\n");
							}

							fi_fprintf(fph,"<value name=\"LocalPose\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f</value>\r\n",
								mc.mLocalOrientation[0],mc.mLocalOrientation[1],mc.mLocalOrientation[2],mc.mLocalOrientation[3],
								mc.mLocalPosition[0],mc.mLocalPosition[1],mc.mLocalPosition[2]);


							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");
							fi_fprintf(fph,"</array>\r\n");
							fi_fprintf(fph,"</struct>\r\n");
							fi_fprintf(fph,"</value>\r\n");


						}
						break;
					default:
						assert(0); // not yet implemented
						break;
				}


				MiI32 jointIndex = getJointParentIndex(ms,i);
				if ( jointIndex == -1 )
				{
					fi_fprintf(fph,"<value name=\"Joint\" type=\"Ref\" included=\"1\" className=\"JointNone\" version=\"0.0\">\r\n");
					fi_fprintf(fph,"	<struct name=\"\">\r\n");
					fi_fprintf(fph,"	</struct>\r\n");
					fi_fprintf(fph,"</value>\r\n");

				}
				else
				{
					saveJoint(ms->mMeshJoints[jointIndex],fph);
				}

				fi_fprintf(fph,"<value name=\"Transform\" type=\"Transform\">%0.9f %0.9f %0.9f %0.9f  %0.9f %0.9f %0.9f</value>\r\n",
					mr.mOrientation[0],mr.mOrientation[1],mr.mOrientation[2],mr.mOrientation[3],
					mr.mPosition[0],mr.mPosition[1],mr.mPosition[2]);

				fi_fprintf(fph,"</struct>\r\n");
			}
			fi_fprintf(fph,"</array>\r\n");

			saveBodyPairCollision(*ms,fph);

			fi_fprintf(fph,"</struct>\r\n");
			fi_fprintf(fph,"</value>\r\n");
			fi_fprintf(fph,"</NvParameters>\r\n");

			size_t outLen;
			void *outMem = fi_getMemBuffer(fph,&outLen);
			if ( outMem )
			{
				len = (MiU32)outLen;
				ret = MI_ALLOC(len);
				memcpy(ret,outMem,len);
			}
			fi_fclose(fph);
		}
	}
	return ret;
}


};// end of namespace




