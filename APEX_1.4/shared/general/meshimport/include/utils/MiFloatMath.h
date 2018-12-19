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



#ifndef FLOAT_MATH_LIB_H

#define FLOAT_MATH_LIB_H

// a set of routines that let you do common 3d math
// operations without any vector, matrix, or quaternion
// classes or templates.
//
// a vector (or point) is a 'MiF32 *' to 3 floating point numbers.
// a matrix is a 'MiF32 *' to an array of 16 floating point numbers representing a 4x4 transformation matrix compatible with D3D or OGL
// a quaternion is a 'MiF32 *' to 4 floats representing a quaternion x,y,z,w
//
//


#include <float.h>
#include "MiPlatformConfig.h"

namespace mimp
{

enum FM_ClipState
{
  FMCS_XMIN       = (1<<0),
  FMCS_XMAX       = (1<<1),
  FMCS_YMIN       = (1<<2),
  FMCS_YMAX       = (1<<3),
  FMCS_ZMIN       = (1<<4),
  FMCS_ZMAX       = (1<<5),
};

enum FM_Axis
{
  FM_XAXIS   = (1<<0),
  FM_YAXIS   = (1<<1),
  FM_ZAXIS   = (1<<2)
};

enum LineSegmentType
{
  LS_START,
  LS_MIDDLE,
  LS_END
};


const MiF32 FM_PI = 3.1415926535897932384626433832795028841971693993751f;
const MiF32 FM_DEG_TO_RAD = ((2.0f * FM_PI) / 360.0f);
const MiF32 FM_RAD_TO_DEG = (360.0f / (2.0f * FM_PI));

//***************** Float versions
//***
//*** vectors are assumed to be 3 floats or 3 doubles representing X, Y, Z
//*** quaternions are assumed to be 4 floats or 4 doubles representing X,Y,Z,W
//*** matrices are assumed to be 16 floats or 16 doubles representing a standard D3D or OpenGL style 4x4 matrix
//*** bounding volumes are expressed as two sets of 3 floats/MiF64 representing bmin(x,y,z) and bmax(x,y,z)
//*** Plane equations are assumed to be 4 floats or 4 doubles representing Ax,By,Cz,D

FM_Axis fm_getDominantAxis(const MiF32 normal[3]);
FM_Axis fm_getDominantAxis(const MiF64 normal[3]);

void fm_decomposeTransform(const MiF32 local_transform[16],MiF32 trans[3],MiF32 rot[4],MiF32 scale[3]);
void fm_decomposeTransform(const MiF64 local_transform[16],MiF64 trans[3],MiF64 rot[4],MiF64 scale[3]);

void  fm_multiplyTransform(const MiF32 *pA,const MiF32 *pB,MiF32 *pM);
void  fm_multiplyTransform(const MiF64 *pA,const MiF64 *pB,MiF64 *pM);

void  fm_inverseTransform(const MiF32 matrix[16],MiF32 inverse_matrix[16]);
void  fm_inverseTransform(const MiF64 matrix[16],MiF64 inverse_matrix[16]);

void  fm_identity(MiF32 matrix[16]); // set 4x4 matrix to identity.
void  fm_identity(MiF64 matrix[16]); // set 4x4 matrix to identity.

void  fm_inverseRT(const MiF32 matrix[16], const MiF32 pos[3], MiF32 t[3]); // inverse rotate translate the point.
void  fm_inverseRT(const MiF64 matrix[16],const MiF64 pos[3],MiF64 t[3]); // inverse rotate translate the point.

void  fm_transform(const MiF32 matrix[16], const MiF32 pos[3], MiF32 t[3]); // rotate and translate this point.
void  fm_transform(const MiF64 matrix[16],const MiF64 pos[3],MiF64 t[3]); // rotate and translate this point.

MiF32  fm_getDeterminant(const MiF32 matrix[16]);
MiF64 fm_getDeterminant(const MiF64 matrix[16]);

void fm_getSubMatrix(MiI32 ki,MiI32 kj,MiF32 pDst[16],const MiF32 matrix[16]);
void fm_getSubMatrix(MiI32 ki,MiI32 kj,MiF64 pDst[16],const MiF32 matrix[16]);

void  fm_rotate(const MiF32 matrix[16],const MiF32 pos[3],MiF32 t[3]); // only rotate the point by a 4x4 matrix, don't translate.
void  fm_rotate(const MiF64 matri[16],const MiF64 pos[3],MiF64 t[3]); // only rotate the point by a 4x4 matrix, don't translate.

void  fm_eulerToMatrix(MiF32 ax,MiF32 ay,MiF32 az,MiF32 matrix[16]); // convert euler (in radians) to a dest 4x4 matrix (translation set to zero)
void  fm_eulerToMatrix(MiF64 ax,MiF64 ay,MiF64 az,MiF64 matrix[16]); // convert euler (in radians) to a dest 4x4 matrix (translation set to zero)

void  fm_getAABB(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 bmin[3],MiF32 bmax[3]);
void  fm_getAABB(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 bmin[3],MiF64 bmax[3]);

void  fm_getAABBCenter(const MiF32 bmin[3],const MiF32 bmax[3],MiF32 center[3]);
void  fm_getAABBCenter(const MiF64 bmin[3],const MiF64 bmax[3],MiF64 center[3]);

void fm_transformAABB(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 matrix[16],MiF32 tbmin[3],MiF32 tbmax[3]);
void fm_transformAABB(const MiF64 bmin[3],const MiF64 bmax[3],const MiF64 matrix[16],MiF64 tbmin[3],MiF64 tbmax[3]);

void  fm_eulerToQuat(MiF32 x,MiF32 y,MiF32 z,MiF32 quat[4]); // convert euler angles to quaternion.
void  fm_eulerToQuat(MiF64 x,MiF64 y,MiF64 z,MiF64 quat[4]); // convert euler angles to quaternion.

void  fm_quatToEuler(const MiF32 quat[4],MiF32 &ax,MiF32 &ay,MiF32 &az);
void  fm_quatToEuler(const MiF64 quat[4],MiF64 &ax,MiF64 &ay,MiF64 &az);

void  fm_eulerToQuat(const MiF32 euler[3],MiF32 quat[4]); // convert euler angles to quaternion. Angles must be radians not degrees!
void  fm_eulerToQuat(const MiF64 euler[3],MiF64 quat[4]); // convert euler angles to quaternion.

void  fm_scale(MiF32 x,MiF32 y,MiF32 z,MiF32 matrix[16]); // apply scale to the matrix.
void  fm_scale(MiF64 x,MiF64 y,MiF64 z,MiF64 matrix[16]); // apply scale to the matrix.

void  fm_eulerToQuatDX(MiF32 x,MiF32 y,MiF32 z,MiF32 quat[4]); // convert euler angles to quaternion using the fucked up DirectX method
void  fm_eulerToQuatDX(MiF64 x,MiF64 y,MiF64 z,MiF64 quat[4]); // convert euler angles to quaternion using the fucked up DirectX method

void  fm_eulerToMatrixDX(MiF32 x,MiF32 y,MiF32 z,MiF32 matrix[16]); // convert euler angles to quaternion using the fucked up DirectX method.
void  fm_eulerToMatrixDX(MiF64 x,MiF64 y,MiF64 z,MiF64 matrix[16]); // convert euler angles to quaternion using the fucked up DirectX method.

void  fm_quatToMatrix(const MiF32 quat[4],MiF32 matrix[16]); // convert quaterinion rotation to matrix, translation set to zero.
void  fm_quatToMatrix(const MiF64 quat[4],MiF64 matrix[16]); // convert quaterinion rotation to matrix, translation set to zero.

void  fm_quatRotate(const MiF32 quat[4],const MiF32 v[3],MiF32 r[3]); // rotate a vector directly by a quaternion.
void  fm_quatRotate(const MiF64 quat[4],const MiF64 v[3],MiF64 r[3]); // rotate a vector directly by a quaternion.

void  fm_getTranslation(const MiF32 matrix[16],MiF32 t[3]);
void  fm_getTranslation(const MiF64 matrix[16],MiF64 t[3]);

void  fm_setTranslation(const MiF32 *translation,MiF32 matrix[16]);
void  fm_setTranslation(const MiF64 *translation,MiF64 matrix[16]);

void  fm_multiplyQuat(const MiF32 *qa,const MiF32 *qb,MiF32 *quat);
void  fm_multiplyQuat(const MiF64 *qa,const MiF64 *qb,MiF64 *quat);

void  fm_matrixToQuat(const MiF32 matrix[16],MiF32 quat[4]); // convert the 3x3 portion of a 4x4 matrix into a quaterion as x,y,z,w
void  fm_matrixToQuat(const MiF64 matrix[16],MiF64 quat[4]); // convert the 3x3 portion of a 4x4 matrix into a quaterion as x,y,z,w

MiF32 fm_sphereVolume(MiF32 radius); // return's the volume of a sphere of this radius (4/3 PI * R cubed )
MiF64 fm_sphereVolume(MiF64 radius); // return's the volume of a sphere of this radius (4/3 PI * R cubed )

MiF32 fm_cylinderVolume(MiF32 radius,MiF32 h);
MiF64 fm_cylinderVolume(MiF64 radius,MiF64 h);

MiF32 fm_capsuleVolume(MiF32 radius,MiF32 h);
MiF64 fm_capsuleVolume(MiF64 radius,MiF64 h);

MiF32 fm_distance(const MiF32 p1[3],const MiF32 p2[3]);
MiF64 fm_distance(const MiF64 p1[3],const MiF64 p2[3]);

MiF32 fm_distanceSquared(const MiF32 p1[3],const MiF32 p2[3]);
MiF64 fm_distanceSquared(const MiF64 p1[3],const MiF64 p2[3]);

MiF32 fm_distanceSquaredXZ(const MiF32 p1[3],const MiF32 p2[3]);
MiF64 fm_distanceSquaredXZ(const MiF64 p1[3],const MiF64 p2[3]);

MiF32 fm_computePlane(const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3],MiF32 *n); // return D
MiF64 fm_computePlane(const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3],MiF64 *n); // return D

MiF32 fm_distToPlane(const MiF32 plane[4],const MiF32 pos[3]); // computes the distance of this point from the plane.
MiF64 fm_distToPlane(const MiF64 plane[4],const MiF64 pos[3]); // computes the distance of this point from the plane.

MiF32 fm_dot(const MiF32 p1[3],const MiF32 p2[3]);
MiF64 fm_dot(const MiF64 p1[3],const MiF64 p2[3]);

void  fm_cross(MiF32 cross[3],const MiF32 a[3],const MiF32 b[3]);
void  fm_cross(MiF64 cross[3],const MiF64 a[3],const MiF64 b[3]);

void  fm_computeNormalVector(MiF32 n[3],const MiF32 p1[3],const MiF32 p2[3]); // as P2-P1 normalized.
void  fm_computeNormalVector(MiF64 n[3],const MiF64 p1[3],const MiF64 p2[3]); // as P2-P1 normalized.

bool  fm_computeWindingOrder(const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3]); // returns true if the triangle is clockwise.
bool  fm_computeWindingOrder(const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3]); // returns true if the triangle is clockwise.

MiF32  fm_normalize(MiF32 n[3]); // normalize this vector and return the distance
MiF64  fm_normalize(MiF64 n[3]); // normalize this vector and return the distance

void  fm_matrixMultiply(const MiF32 A[16],const MiF32 B[16],MiF32 dest[16]);
void  fm_matrixMultiply(const MiF64 A[16],const MiF64 B[16],MiF64 dest[16]);

void  fm_composeTransform(const MiF32 position[3],const MiF32 quat[4],const MiF32 scale[3],MiF32 matrix[16]);
void  fm_composeTransform(const MiF64 position[3],const MiF64 quat[4],const MiF64 scale[3],MiF64 matrix[16]);

MiF32 fm_computeArea(const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3]);
MiF64 fm_computeArea(const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3]);

void  fm_lerp(const MiF32 p1[3],const MiF32 p2[3],MiF32 dest[3],MiF32 lerpValue);
void  fm_lerp(const MiF64 p1[3],const MiF64 p2[3],MiF64 dest[3],MiF64 lerpValue);

bool  fm_insideTriangleXZ(const MiF32 test[3],const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3]);
bool  fm_insideTriangleXZ(const MiF64 test[3],const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3]);

bool  fm_insideAABB(const MiF32 pos[3],const MiF32 bmin[3],const MiF32 bmax[3]);
bool  fm_insideAABB(const MiF64 pos[3],const MiF64 bmin[3],const MiF64 bmax[3]);

bool  fm_insideAABB(const MiF32 obmin[3],const MiF32 obmax[3],const MiF32 tbmin[3],const MiF32 tbmax[3]); // test if bounding box tbmin/tmbax is fully inside obmin/obmax
bool  fm_insideAABB(const MiF64 obmin[3],const MiF64 obmax[3],const MiF64 tbmin[3],const MiF64 tbmax[3]); // test if bounding box tbmin/tmbax is fully inside obmin/obmax

MiU32 fm_clipTestPoint(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 pos[3]);
MiU32 fm_clipTestPoint(const MiF64 bmin[3],const MiF64 bmax[3],const MiF64 pos[3]);

MiU32 fm_clipTestPointXZ(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 pos[3]); // only tests X and Z, not Y
MiU32 fm_clipTestPointXZ(const MiF64 bmin[3],const MiF64 bmax[3],const MiF64 pos[3]); // only tests X and Z, not Y


MiU32 fm_clipTestAABB(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3],MiU32 &andCode);
MiU32 fm_clipTestAABB(const MiF64 bmin[3],const MiF64 bmax[3],const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3],MiU32 &andCode);


bool     fm_lineTestAABBXZ(const MiF32 p1[3],const MiF32 p2[3],const MiF32 bmin[3],const MiF32 bmax[3],MiF32 &time);
bool     fm_lineTestAABBXZ(const MiF64 p1[3],const MiF64 p2[3],const MiF64 bmin[3],const MiF64 bmax[3],MiF64 &time);

bool     fm_lineTestAABB(const MiF32 p1[3],const MiF32 p2[3],const MiF32 bmin[3],const MiF32 bmax[3],MiF32 &time);
bool     fm_lineTestAABB(const MiF64 p1[3],const MiF64 p2[3],const MiF64 bmin[3],const MiF64 bmax[3],MiF64 &time);


void  fm_initMinMax(const MiF32 p[3],MiF32 bmin[3],MiF32 bmax[3]);
void  fm_initMinMax(const MiF64 p[3],MiF64 bmin[3],MiF64 bmax[3]);

void  fm_initMinMax(MiF32 bmin[3],MiF32 bmax[3]);
void  fm_initMinMax(MiF64 bmin[3],MiF64 bmax[3]);

void  fm_minmax(const MiF32 p[3],MiF32 bmin[3],MiF32 bmax[3]); // accmulate to a min-max value
void  fm_minmax(const MiF64 p[3],MiF64 bmin[3],MiF64 bmax[3]); // accmulate to a min-max value


MiF32 fm_solveX(const MiF32 plane[4],MiF32 y,MiF32 z); // solve for X given this plane equation and the other two components.
MiF64 fm_solveX(const MiF64 plane[4],MiF64 y,MiF64 z); // solve for X given this plane equation and the other two components.

MiF32 fm_solveY(const MiF32 plane[4],MiF32 x,MiF32 z); // solve for Y given this plane equation and the other two components.
MiF64 fm_solveY(const MiF64 plane[4],MiF64 x,MiF64 z); // solve for Y given this plane equation and the other two components.

MiF32 fm_solveZ(const MiF32 plane[4],MiF32 x,MiF32 y); // solve for Z given this plane equation and the other two components.
MiF64 fm_solveZ(const MiF64 plane[4],MiF64 x,MiF64 y); // solve for Z given this plane equation and the other two components.

bool  fm_computeBestFitPlane(MiU32 vcount,     // number of input data points
                     const MiF32 *points,     // starting address of points array.
                     MiU32 vstride,    // stride between input points.
                     const MiF32 *weights,    // *optional point weighting values.
                     MiU32 wstride,    // weight stride for each vertex.
                     MiF32 plane[4]);

bool  fm_computeBestFitPlane(MiU32 vcount,     // number of input data points
                     const MiF64 *points,     // starting address of points array.
                     MiU32 vstride,    // stride between input points.
                     const MiF64 *weights,    // *optional point weighting values.
                     MiU32 wstride,    // weight stride for each vertex.
                     MiF64 plane[4]);

bool  fm_computeCentroid(MiU32 vcount,     // number of input data points
						 const MiF32 *points,     // starting address of points array.
						 MiU32 vstride,    // stride between input points.
						 MiF32 *center);

bool  fm_computeCentroid(MiU32 vcount,     // number of input data points
						 const MiF64 *points,     // starting address of points array.
						 MiU32 vstride,    // stride between input points.
						 MiF64 *center);


MiF32  fm_computeBestFitAABB(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 bmin[3],MiF32 bmax[3]); // returns the diagonal distance
MiF64 fm_computeBestFitAABB(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 bmin[3],MiF64 bmax[3]); // returns the diagonal distance

MiF32  fm_computeBestFitSphere(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 center[3]);
MiF64  fm_computeBestFitSphere(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 center[3]);

bool fm_lineSphereIntersect(const MiF32 center[3],MiF32 radius,const MiF32 p1[3],const MiF32 p2[3],MiF32 intersect[3]);
bool fm_lineSphereIntersect(const MiF64 center[3],MiF64 radius,const MiF64 p1[3],const MiF64 p2[3],MiF64 intersect[3]);

bool fm_intersectRayAABB(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 pos[3],const MiF32 dir[3],MiF32 intersect[3]);
bool fm_intersectLineSegmentAABB(const MiF32 bmin[3],const MiF32 bmax[3],const MiF32 p1[3],const MiF32 p2[3],MiF32 intersect[3]);

bool fm_lineIntersectsTriangle(const MiF32 rayStart[3],const MiF32 rayEnd[3],const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3],MiF32 sect[3]);
bool fm_lineIntersectsTriangle(const MiF64 rayStart[3],const MiF64 rayEnd[3],const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3],MiF64 sect[3]);

bool fm_rayIntersectsTriangle(const MiF32 origin[3],const MiF32 dir[3],const MiF32 v0[3],const MiF32 v1[3],const MiF32 v2[3],MiF32 &t);
bool fm_rayIntersectsTriangle(const MiF64 origin[3],const MiF64 dir[3],const MiF64 v0[3],const MiF64 v1[3],const MiF64 v2[3],MiF64 &t);

bool fm_raySphereIntersect(const MiF32 center[3],MiF32 radius,const MiF32 pos[3],const MiF32 dir[3],MiF32 distance,MiF32 intersect[3]);
bool fm_raySphereIntersect(const MiF64 center[3],MiF64 radius,const MiF64 pos[3],const MiF64 dir[3],MiF64 distance,MiF64 intersect[3]);

void fm_catmullRom(MiF32 out_vector[3],const MiF32 p1[3],const MiF32 p2[3],const MiF32 p3[3],const MiF32 *p4, const MiF32 s);
void fm_catmullRom(MiF64 out_vector[3],const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3],const MiF64 *p4, const MiF64 s);

bool fm_intersectAABB(const MiF32 bmin1[3],const MiF32 bmax1[3],const MiF32 bmin2[3],const MiF32 bmax2[3]);
bool fm_intersectAABB(const MiF64 bmin1[3],const MiF64 bmax1[3],const MiF64 bmin2[3],const MiF64 bmax2[3]);


// computes the rotation quaternion to go from unit-vector v0 to unit-vector v1
void fm_rotationArc(const MiF32 v0[3],const MiF32 v1[3],MiF32 quat[4]);
void fm_rotationArc(const MiF64 v0[3],const MiF64 v1[3],MiF64 quat[4]);

MiF32  fm_distancePointLineSegment(const MiF32 Point[3],const MiF32 LineStart[3],const MiF32 LineEnd[3],MiF32 intersection[3],LineSegmentType &type,MiF32 epsilon);
MiF64 fm_distancePointLineSegment(const MiF64 Point[3],const MiF64 LineStart[3],const MiF64 LineEnd[3],MiF64 intersection[3],LineSegmentType &type,MiF64 epsilon);


bool fm_colinear(const MiF64 p1[3],const MiF64 p2[3],const MiF64 p3[3],MiF64 epsilon=0.999);               // true if these three points in a row are co-linear
bool fm_colinear(const MiF32  p1[3],const MiF32  p2[3],const MiF32 p3[3],MiF32 epsilon=0.999f);

bool fm_colinear(const MiF32 a1[3],const MiF32 a2[3],const MiF32 b1[3],const MiF32 b2[3],MiF32 epsilon=0.999f);  // true if these two line segments are co-linear.
bool fm_colinear(const MiF64 a1[3],const MiF64 a2[3],const MiF64 b1[3],const MiF64 b2[3],MiF64 epsilon=0.999);  // true if these two line segments are co-linear.

enum IntersectResult
{
  IR_DONT_INTERSECT,
  IR_DO_INTERSECT,
  IR_COINCIDENT,
  IR_PARALLEL,
};

IntersectResult fm_intersectLineSegments2d(const MiF32 a1[3], const MiF32 a2[3], const MiF32 b1[3], const MiF32 b2[3], MiF32 intersectionPoint[3]);
IntersectResult fm_intersectLineSegments2d(const MiF64 a1[3],const MiF64 a2[3],const MiF64 b1[3],const MiF64 b2[3],MiF64 intersectionPoint[3]);

IntersectResult fm_intersectLineSegments2dTime(const MiF32 a1[3], const MiF32 a2[3], const MiF32 b1[3], const MiF32 b2[3],MiF32 &t1,MiF32 &t2);
IntersectResult fm_intersectLineSegments2dTime(const MiF64 a1[3],const MiF64 a2[3],const MiF64 b1[3],const MiF64 b2[3],MiF64 &t1,MiF64 &t2);

// Plane-Triangle splitting

enum PlaneTriResult
{
  PTR_ON_PLANE,
  PTR_FRONT,
  PTR_BACK,
  PTR_SPLIT,
};

PlaneTriResult fm_planeTriIntersection(const MiF32 plane[4],    // the plane equation in Ax+By+Cz+D format
                                    const MiF32 *triangle, // the source triangle.
                                    MiU32 tstride,  // stride in bytes of the input and output *vertices*
                                    MiF32        epsilon,  // the co-planer epsilon value.
                                    MiF32       *front,    // the triangle in front of the
                                    MiU32 &fcount,  // number of vertices in the 'front' triangle
                                    MiF32       *back,     // the triangle in back of the plane
                                    MiU32 &bcount); // the number of vertices in the 'back' triangle.


PlaneTriResult fm_planeTriIntersection(const MiF64 plane[4],    // the plane equation in Ax+By+Cz+D format
                                    const MiF64 *triangle, // the source triangle.
                                    MiU32 tstride,  // stride in bytes of the input and output *vertices*
                                    MiF64        epsilon,  // the co-planer epsilon value.
                                    MiF64       *front,    // the triangle in front of the
                                    MiU32 &fcount,  // number of vertices in the 'front' triangle
                                    MiF64       *back,     // the triangle in back of the plane
                                    MiU32 &bcount); // the number of vertices in the 'back' triangle.


void fm_intersectPointPlane(const MiF32 p1[3],const MiF32 p2[3],MiF32 *split,const MiF32 plane[4]);
void fm_intersectPointPlane(const MiF64 p1[3],const MiF64 p2[3],MiF64 *split,const MiF64 plane[4]);

PlaneTriResult fm_getSidePlane(const MiF32 p[3],const MiF32 plane[4],MiF32 epsilon);
PlaneTriResult fm_getSidePlane(const MiF64 p[3],const MiF64 plane[4],MiF64 epsilon);


void fm_computeBestFitOBB(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 *sides,MiF32 matrix[16],bool bruteForce=true);
void fm_computeBestFitOBB(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 *sides,MiF64 matrix[16],bool bruteForce=true);

void fm_computeBestFitOBB(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 *sides,MiF32 pos[3],MiF32 quat[4],bool bruteForce=true);
void fm_computeBestFitOBB(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 *sides,MiF64 pos[3],MiF64 quat[4],bool bruteForce=true);

void fm_computeBestFitABB(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 *sides,MiF32 pos[3]);
void fm_computeBestFitABB(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF64 *sides,MiF64 pos[3]);


//** Note, if the returned capsule height is less than zero, then you must represent it is a sphere of size radius.
void fm_computeBestFitCapsule(MiU32 vcount,const MiF32 *points,MiU32 pstride,MiF32 &radius,MiF32 &height,MiF32 matrix[16],bool bruteForce=true);
void fm_computeBestFitCapsule(MiU32 vcount,const MiF64 *points,MiU32 pstride,MiF32 &radius,MiF32 &height,MiF64 matrix[16],bool bruteForce=true);


void fm_planeToMatrix(const MiF32 plane[4],MiF32 matrix[16]); // convert a plane equation to a 4x4 rotation matrix.  Reference vector is 0,1,0
void fm_planeToQuat(const MiF32 plane[4],MiF32 quat[4],MiF32 pos[3]); // convert a plane equation to a quaternion and translation

void fm_planeToMatrix(const MiF64 plane[4],MiF64 matrix[16]); // convert a plane equation to a 4x4 rotation matrix
void fm_planeToQuat(const MiF64 plane[4],MiF64 quat[4],MiF64 pos[3]); // convert a plane equation to a quaternion and translation

inline void fm_doubleToFloat3(const MiF64 p[3],MiF32 t[3]) { t[0] = (MiF32) p[0]; t[1] = (MiF32)p[1]; t[2] = (MiF32)p[2]; };
inline void fm_floatToDouble3(const MiF32 p[3],MiF64 t[3]) { t[0] = (MiF64)p[0]; t[1] = (MiF64)p[1]; t[2] = (MiF64)p[2]; };


void  fm_eulerMatrix(MiF32 ax,MiF32 ay,MiF32 az,MiF32 matrix[16]); // convert euler (in radians) to a dest 4x4 matrix (translation set to zero)
void  fm_eulerMatrix(MiF64 ax,MiF64 ay,MiF64 az,MiF64 matrix[16]); // convert euler (in radians) to a dest 4x4 matrix (translation set to zero)


MiF32  fm_computeMeshVolume(const MiF32 *vertices,MiU32 tcount,const MiU32 *indices);
MiF64 fm_computeMeshVolume(const MiF64 *vertices,MiU32 tcount,const MiU32 *indices);


#define FM_DEFAULT_GRANULARITY 0.001f  // 1 millimeter is the default granularity

class fm_VertexIndex
{
public:
  virtual MiU32          getIndex(const MiF32 pos[3],bool &newPos) = 0;  // get welded index for this MiF32 vector[3]
  virtual MiU32          getIndex(const MiF64 pos[3],bool &newPos) = 0;  // get welded index for this MiF64 vector[3]
  virtual const MiF32 *   getVerticesFloat(void) const = 0;
  virtual const MiF64 *  getVerticesDouble(void) const = 0;
  virtual const MiF32 *   getVertexFloat(MiU32 index) const = 0;
  virtual const MiF64 *  getVertexDouble(MiU32 index) const = 0;
  virtual MiU32          getVcount(void) const = 0;
  virtual bool            isDouble(void) const = 0;
  virtual bool            saveAsObj(const char *fname,MiU32 tcount,MiU32 *indices) = 0;
};

fm_VertexIndex * fm_createVertexIndex(MiF64 granularity,bool snapToGrid); // create an indexed vertex system for doubles
fm_VertexIndex * fm_createVertexIndex(MiF32 granularity,bool snapToGrid);  // create an indexed vertext system for floats
void             fm_releaseVertexIndex(fm_VertexIndex *vindex);



#if 0 // currently disabled

class fm_LineSegment
{
public:
  fm_LineSegment(void)
  {
    mE1 = mE2 = 0;
  }

  fm_LineSegment(MiU32 e1,MiU32 e2)
  {
    mE1 = e1;
    mE2 = e2;
  }

  MiU32 mE1;
  MiU32 mE2;
};


// LineSweep *only* supports doublees.  As a geometric operation it needs as much precision as possible.
class fm_LineSweep
{
public:

 virtual fm_LineSegment * performLineSweep(const fm_LineSegment *segments,
                                   MiU32 icount,
                                   const MiF64 *planeEquation,
                                   fm_VertexIndex *pool,
                                   MiU32 &scount) = 0;


};

fm_LineSweep * fm_createLineSweep(void);
void           fm_releaseLineSweep(fm_LineSweep *sweep);

#endif

class fm_Triangulate
{
public:
  virtual const MiF64 *       triangulate3d(MiU32 pcount,
                                             const MiF64 *points,
                                             MiU32 vstride,
                                             MiU32 &tcount,
                                             bool consolidate,
                                             MiF64 epsilon) = 0;

  virtual const MiF32  *       triangulate3d(MiU32 pcount,
                                             const MiF32  *points,
                                             MiU32 vstride,
                                             MiU32 &tcount,
                                             bool consolidate,
                                             MiF32 epsilon) = 0;
};

fm_Triangulate * fm_createTriangulate(void);
void             fm_releaseTriangulate(fm_Triangulate *t);


const MiF32 * fm_getPoint(const MiF32 *points,MiU32 pstride,MiU32 index);
const MiF64 * fm_getPoint(const MiF64 *points,MiU32 pstride,MiU32 index);

bool   fm_insideTriangle(MiF32 Ax, MiF32 Ay,MiF32 Bx, MiF32 By,MiF32 Cx, MiF32 Cy,MiF32 Mi, MiF32 Py);
bool   fm_insideTriangle(MiF64 Ax, MiF64 Ay,MiF64 Bx, MiF64 By,MiF64 Cx, MiF64 Cy,MiF64 Mi, MiF64 Py);
MiF32  fm_areaPolygon2d(MiU32 pcount,const MiF32 *points,MiU32 pstride);
MiF64 fm_areaPolygon2d(MiU32 pcount,const MiF64 *points,MiU32 pstride);

bool  fm_pointInsidePolygon2d(MiU32 pcount,const MiF32 *points,MiU32 pstride,const MiF32 *point,MiU32 xindex=0,MiU32 yindex=1);
bool  fm_pointInsidePolygon2d(MiU32 pcount,const MiF64 *points,MiU32 pstride,const MiF64 *point,MiU32 xindex=0,MiU32 yindex=1);

MiU32 fm_consolidatePolygon(MiU32 pcount,const MiF32 *points,MiU32 pstride,MiF32 *dest,MiF32 epsilon=0.999999f); // collapses co-linear edges.
MiU32 fm_consolidatePolygon(MiU32 pcount,const MiF64 *points,MiU32 pstride,MiF64 *dest,MiF64 epsilon=0.999999); // collapses co-linear edges.


bool fm_computeSplitPlane(MiU32 vcount,const MiF64 *vertices,MiU32 tcount,const MiU32 *indices,MiF64 *plane);
bool fm_computeSplitPlane(MiU32 vcount,const MiF32 *vertices,MiU32 tcount,const MiU32 *indices,MiF32 *plane);

void fm_nearestPointInTriangle(const MiF32 *pos,const MiF32 *p1,const MiF32 *p2,const MiF32 *p3,MiF32 *nearest);
void fm_nearestPointInTriangle(const MiF64 *pos,const MiF64 *p1,const MiF64 *p2,const MiF64 *p3,MiF64 *nearest);

MiF32  fm_areaTriangle(const MiF32 *p1,const MiF32 *p2,const MiF32 *p3);
MiF64 fm_areaTriangle(const MiF64 *p1,const MiF64 *p2,const MiF64 *p3);

void fm_subtract(const MiF32 *A,const MiF32 *B,MiF32 *diff); // compute A-B and store the result in 'diff'
void fm_subtract(const MiF64 *A,const MiF64 *B,MiF64 *diff); // compute A-B and store the result in 'diff'

void fm_multiply(MiF32 *A,MiF32 scaler);
void fm_multiply(MiF64 *A,MiF64 scaler);

void fm_add(const MiF32 *A,const MiF32 *B,MiF32 *sum);
void fm_add(const MiF64 *A,const MiF64 *B,MiF64 *sum);

void fm_copy3(const MiF32 *source,MiF32 *dest);
void fm_copy3(const MiF64 *source,MiF64 *dest);

// re-indexes an indexed triangle mesh but drops unused vertices.  The output_indices can be the same pointer as the input indices.
// the output_vertices can point to the input vertices if you desire.  The output_vertices buffer should be at least the same size
// is the input buffer.  The routine returns the new vertex count after re-indexing.
MiU32  fm_copyUniqueVertices(MiU32 vcount,const MiF32 *input_vertices,MiF32 *output_vertices,MiU32 tcount,const MiU32 *input_indices,MiU32 *output_indices);
MiU32  fm_copyUniqueVertices(MiU32 vcount,const MiF64 *input_vertices,MiF64 *output_vertices,MiU32 tcount,const MiU32 *input_indices,MiU32 *output_indices);

bool    fm_isMeshCoplanar(MiU32 tcount,const MiU32 *indices,const MiF32 *vertices,bool doubleSided); // returns true if this collection of indexed triangles are co-planar!
bool    fm_isMeshCoplanar(MiU32 tcount,const MiU32 *indices,const MiF64 *vertices,bool doubleSided); // returns true if this collection of indexed triangles are co-planar!

bool    fm_samePlane(const MiF32 p1[4],const MiF32 p2[4],MiF32 normalEpsilon=0.01f,MiF32 dEpsilon=0.001f,bool doubleSided=false); // returns true if these two plane equations are identical within an epsilon
bool    fm_samePlane(const MiF64 p1[4],const MiF64 p2[4],MiF64 normalEpsilon=0.01,MiF64 dEpsilon=0.001,bool doubleSided=false);

void    fm_OBBtoAABB(const MiF32 obmin[3],const MiF32 obmax[3],const MiF32 matrix[16],MiF32 abmin[3],MiF32 abmax[3]);

// a utility class that will tesseleate a mesh.
class fm_Tesselate
{
public:
  virtual const MiU32 * tesselate(fm_VertexIndex *vindex,MiU32 tcount,const MiU32 *indices,MiF32 longEdge,MiU32 maxDepth,MiU32 &outcount) = 0;
};

fm_Tesselate * fm_createTesselate(void);
void           fm_releaseTesselate(fm_Tesselate *t);

void fm_computeMeanNormals(MiU32 vcount,       // the number of vertices
                           const MiF32 *vertices,     // the base address of the vertex position data.
                           MiU32 vstride,      // the stride between position data.
                           MiF32 *normals,            // the base address  of the destination for mean vector normals
                           MiU32 nstride,      // the stride between normals
                           MiU32 tcount,       // the number of triangles
                           const MiU32 *indices);     // the triangle indices

void fm_computeMeanNormals(MiU32 vcount,       // the number of vertices
                           const MiF64 *vertices,     // the base address of the vertex position data.
                           MiU32 vstride,      // the stride between position data.
                           MiF64 *normals,            // the base address  of the destination for mean vector normals
                           MiU32 nstride,      // the stride between normals
                           MiU32 tcount,       // the number of triangles
                           const MiU32 *indices);     // the triangle indices


bool fm_isValidTriangle(const MiF32 *p1,const MiF32 *p2,const MiF32 *p3,MiF32 epsilon=0.00001f);
bool fm_isValidTriangle(const MiF64 *p1,const MiF64 *p2,const MiF64 *p3,MiF64 epsilon=0.00001f);

};

#endif
