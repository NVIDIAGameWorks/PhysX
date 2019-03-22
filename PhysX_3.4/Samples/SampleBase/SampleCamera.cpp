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

#include "SampleCamera.h"
#include "SampleUtils.h"
#include "RenderPhysX3Debug.h"
#include "RendererColor.h"

using namespace SampleRenderer;

// PT: the base camera code should be the same for all cameras, regardless of how
// the camera is controlled. For example this should deal with VFC, etc.

Camera::Camera() :
	mProjMatrix		(degtorad(45.0f), 1.0f, 1.0f, 100.0f),
	mFOV			(0.0f),
	mNearPlane		(0.0f),
	mFarPlane		(0.0f),
	mDirtyProj		(true),
	mDirtyView		(true)
{
	mViewMatrix		= PxTransform(PxIdentity);
	mPos	= PxVec3(0);
	mRot	= PxVec3(0);

	mDrawDebugData	= false;
	mFreezeFrustum	= false;
	mPerformVFC		= true;
}

Camera::~Camera()
{
}

// PT: TODO: copied from SampleApplication. Refactor.
static PxMat33 EulerToMat33(const PxVec3 &e)
{
	float c1 = cosf(e.z);
	float s1 = sinf(e.z);
	float c2 = cosf(e.y);
	float s2 = sinf(e.y);
	float c3 = cosf(e.x);
	float s3 = sinf(e.x);
	PxMat33 m(PxVec3(c1*c2,              -s1*c2,             s2),
			  PxVec3((s1*c3)+(c1*s2*s3), (c1*c3)-(s1*s2*s3),-c2*s3),
			  PxVec3((s1*s3)-(c1*s2*c3), (c1*s3)+(s1*s2*c3), c2*c3));

	return m;
}

void Camera::updateInternals()
{
	if(mDirtyProj)
	{
		mDirtyProj	= false;
		mProjMatrix = RendererProjection(degtorad(mFOV), mViewport.computeRatio(), mNearPlane, mFarPlane);
	}
	if(mDirtyView)
	{
		mDirtyView	= false;

		mViewMatrix.q	= PxQuat(EulerToMat33(mRot));
		mViewMatrix.p	= mPos;
	}
}

PxVec3 Camera::getViewDir() const
{
	const PxTransform& camPose = getViewMatrix();
	PxVec3 forward = PxMat33(camPose.q)[2];
	return -forward;
}

void Camera::lookAt(const PxVec3& position, const PxVec3& target)
{
	PxVec3 dir, right, up;
	Ps::computeBasis(position, target, dir, right, up);

	PxTransform view;
	view.p	= position;
	view.q	= PxQuat(PxMat33(-right, up, -dir));
	setView(view);
}


	enum FrustumPlaneIndex
	{
		FRUSTUM_PLANE_LEFT			= 0,	//!< Left clipping plane
		FRUSTUM_PLANE_RIGHT			= 1,	//!< Right clipping plane
		FRUSTUM_PLANE_TOP			= 2,	//!< Top clipping plane
		FRUSTUM_PLANE_BOTTOM		= 3,	//!< Bottom clipping plane
		FRUSTUM_PLANE_NEAR			= 4,	//!< Near clipping plane
		FRUSTUM_PLANE_FAR			= 5,	//!< Far clipping plane (must be last for infinite far clip)

		FRUSTUM_PLANE_FORCE_DWORD	= 0x7fffffff
	};

static PxMat44 convertViewMatrix(const PxTransform& eye)
{
	PxTransform viewMatrix = eye.getInverse();
	PxMat44 mat44 = PxMat44(viewMatrix).getTranspose();

	float m[16];
	memcpy(m, mat44.front(), sizeof m);

	PxMat44 view44;
	view44.column0.x = m[0];
	view44.column0.y = m[1];
	view44.column0.z = m[2];
	view44.column0.w = m[3];
	view44.column1.x = m[4];
	view44.column1.y = m[5];
	view44.column1.z = m[6];
	view44.column1.w = m[7];
	view44.column2.x = m[8];
	view44.column2.y = m[9];
	view44.column2.z = m[10];
	view44.column2.w = m[11];
	view44.column3.x = m[12];
	view44.column3.y = m[13];
	view44.column3.z = m[14];
	view44.column3.w = m[15];

PxMat44 tmpmat = view44.getTranspose(); view44 = tmpmat;

	return view44;
}

static PxMat44 convertProjMatrix(const RendererProjection& proj)
{
	float renderProjMatrix[16];
	proj.getColumnMajor44(renderProjMatrix);

	PxMat44 proj44;
	proj44.column0.x = renderProjMatrix[0];
	proj44.column0.y = renderProjMatrix[1];
	proj44.column0.z = renderProjMatrix[2];
	proj44.column0.w = renderProjMatrix[3];
	proj44.column1.x = renderProjMatrix[4];
	proj44.column1.y = renderProjMatrix[5];
	proj44.column1.z = renderProjMatrix[6];
	proj44.column1.w = renderProjMatrix[7];
	proj44.column2.x = renderProjMatrix[8];
	proj44.column2.y = renderProjMatrix[9];
	proj44.column2.z = renderProjMatrix[10];
	proj44.column2.w = renderProjMatrix[11];
	proj44.column3.x = renderProjMatrix[12];
	proj44.column3.y = renderProjMatrix[13];
	proj44.column3.z = renderProjMatrix[14];
	proj44.column3.w = renderProjMatrix[15];

//PxMat44 tmpmat = proj44.getTranspose(); proj44 = tmpmat;

	return proj44;
}

void Camera::BuildFrustum()
{
	if(mFreezeFrustum)
		return;

	// PT: a better way is to extract the planes from the view-proj matrix but it has some subtle differences with D3D/GL.
	// Building the frustum explicitly is just easier here (although not as efficient)

	const PxReal ratio		= mViewport.computeRatio();

	const PxReal Tan		= tanf(degtorad(0.5f * mFOV)) / ratio;

	const PxReal nearCoeff	= mNearPlane * Tan;
	const PxReal farCoeff	= mFarPlane * Tan;

	const PxReal rightCoeff = ratio;
	const PxReal upCoeff	= 1.0f;

	const PxTransform& view = getViewMatrix();
	PxMat33 mat33(view.q);
	PxVec3 right	= mat33[0];
	PxVec3 up		= mat33[1];
	PxVec3 forward	=-mat33[2];

	mFrustum[0] = mPos + forward*mNearPlane - right*nearCoeff*rightCoeff + up*nearCoeff*upCoeff;
	mFrustum[1] = mPos + forward*mNearPlane - right*nearCoeff*rightCoeff - up*nearCoeff*upCoeff;
	mFrustum[2] = mPos + forward*mNearPlane + right*nearCoeff*rightCoeff - up*nearCoeff*upCoeff;
	mFrustum[3] = mPos + forward*mNearPlane + right*nearCoeff*rightCoeff + up*nearCoeff*upCoeff;

	mFrustum[4] = mPos + forward*mFarPlane - right*farCoeff*rightCoeff + up*farCoeff*upCoeff;
	mFrustum[5] = mPos + forward*mFarPlane - right*farCoeff*rightCoeff - up*farCoeff*upCoeff;
	mFrustum[6] = mPos + forward*mFarPlane + right*farCoeff*rightCoeff - up*farCoeff*upCoeff;
	mFrustum[7] = mPos + forward*mFarPlane + right*farCoeff*rightCoeff + up*farCoeff*upCoeff;

	if(1)
	{
		mPlanes[0] = PxPlane(mFrustum[4], mFrustum[1], mFrustum[5]);
		mPlanes[1] = PxPlane(mFrustum[6], mFrustum[3], mFrustum[7]);
		mPlanes[2] = PxPlane(mFrustum[4], mFrustum[7], mFrustum[3]);
		mPlanes[3] = PxPlane(mFrustum[1], mFrustum[6], mFrustum[5]);
		mPlanes[4] = PxPlane(mFrustum[0], mFrustum[2], mFrustum[1]);
		mPlanes[5] = PxPlane(mFrustum[5], mFrustum[7], mFrustum[4]);

		{
			for(int i=0;i<6;i++)
			{
				mPlanes[i].n = -mPlanes[i].n;
				mPlanes[i].d = -mPlanes[i].d;
			}
		}
	}

	if(0)
	{
		//
		const PxVec3 axisX(1.0f, 0.0f, 0.0f);
		const PxVec3 axisY(0.0f, 1.0f, 0.0f);
		const PxVec3 axisZ(0.0f, 0.0f, 1.0f);

		PxQuat RotX(degtorad(0.5f * mFOV), axisX);
		PxQuat RotY(degtorad(0.5f * mFOV), axisY);
		PxQuat RotZ(degtorad(0.5f * mFOV), axisZ);

		PxVec3 tmp1 = RotY.rotate(-axisX);
		PxVec3 tmp11 = view.q.rotate(tmp1);	// Plane0
		mPlanes[0].n = tmp11;
		mPlanes[0].d = - mPos.dot(mPlanes[0].n);
		//

		RotY = PxQuat(-degtorad(0.5f * mFOV), axisY);

		PxVec3 tmpy = RotY.rotate(axisX);
		PxVec3 tmpyy = view.q.rotate(tmpy);	// Plane1
		mPlanes[1].n = tmpyy;
		mPlanes[1].d = - mPos.dot(mPlanes[1].n);

		//

		RotX = PxQuat(degtorad(0.5f * mFOV)/ratio, axisX);
		PxVec3 tmpx = RotX.rotate(axisY);
		PxVec3 tmpxx = view.q.rotate(tmpx);		// Plane2?
		mPlanes[2].n = tmpxx;
		mPlanes[2].d = - mPos.dot(mPlanes[2].n);

		//

		RotX = PxQuat(-degtorad(0.5f * mFOV)/ratio, axisX);
		tmpx = RotX.rotate(axisY);
		tmpxx = view.q.rotate(tmpx);		// -Plane3?
		mPlanes[3].n = -tmpxx;
		mPlanes[3].d = - mPos.dot(mPlanes[3].n);

		//

		mPlanes[4].n = -forward;
		mPlanes[4].d = - (mPos.dot(mPlanes[4].n) + forward.dot(mPlanes[4].n)*mNearPlane);

		mPlanes[5].n = forward;
		mPlanes[5].d = - (mPos.dot(mPlanes[5].n) + forward.dot(mPlanes[5].n)*mFarPlane);
	}


	if(0)
	{
		PxMat44 proj44 = convertProjMatrix(mProjMatrix);
		PxMat44 view44 = convertViewMatrix(view);
//		PxMat44 combo44 = view44 * proj44;
		PxMat44 combo44 = proj44 * view44;

		PxReal combo[4][4];
		PxReal* dst = &combo[0][0];
		memcpy(dst, &combo44, sizeof(PxReal)*16);

		// D3D:
		// -w' < x' < w'
		// -w' < y' < w'
		// 0 < z' < w'
		//
		// GL:
		// -w' < x' < w'
		// -w' < y' < w'
		// -w' < z' < w'

		// Left clipping plane
		mPlanes[FRUSTUM_PLANE_LEFT].n.x	= -(combo[0][3] + combo[0][0]);
		mPlanes[FRUSTUM_PLANE_LEFT].n.y	= -(combo[1][3] + combo[1][0]);
		mPlanes[FRUSTUM_PLANE_LEFT].n.z	= -(combo[2][3] + combo[2][0]);
		mPlanes[FRUSTUM_PLANE_LEFT].d	= -(combo[3][3] + combo[3][0]);

		// Right clipping plane
		mPlanes[FRUSTUM_PLANE_RIGHT].n.x	= -(combo[0][3] - combo[0][0]);
		mPlanes[FRUSTUM_PLANE_RIGHT].n.y	= -(combo[1][3] - combo[1][0]);
		mPlanes[FRUSTUM_PLANE_RIGHT].n.z	= -(combo[2][3] - combo[2][0]);
		mPlanes[FRUSTUM_PLANE_RIGHT].d		= -(combo[3][3] - combo[3][0]);

		// Top clipping plane
		mPlanes[FRUSTUM_PLANE_TOP].n.x	= -(combo[0][3] - combo[0][1]);
		mPlanes[FRUSTUM_PLANE_TOP].n.y	= -(combo[1][3] - combo[1][1]);
		mPlanes[FRUSTUM_PLANE_TOP].n.z	= -(combo[2][3] - combo[2][1]);
		mPlanes[FRUSTUM_PLANE_TOP].d	= -(combo[3][3] - combo[3][1]);

		// Bottom clipping plane
		mPlanes[FRUSTUM_PLANE_BOTTOM].n.x	= -(combo[0][3] + combo[0][1]);
		mPlanes[FRUSTUM_PLANE_BOTTOM].n.y	= -(combo[1][3] + combo[1][1]);
		mPlanes[FRUSTUM_PLANE_BOTTOM].n.z	= -(combo[2][3] + combo[2][1]);
		mPlanes[FRUSTUM_PLANE_BOTTOM].d		= -(combo[3][3] + combo[3][1]);

		// Near clipping plane
		if(1)
		{
			// OpenGL path
			mPlanes[FRUSTUM_PLANE_NEAR].n.x	= -(combo[0][3] + combo[0][2]);
			mPlanes[FRUSTUM_PLANE_NEAR].n.y	= -(combo[1][3] + combo[1][2]);
			mPlanes[FRUSTUM_PLANE_NEAR].n.z	= -(combo[2][3] + combo[2][2]);
			mPlanes[FRUSTUM_PLANE_NEAR].d	= -(combo[3][3] + combo[3][2]);
		}
		else
		{
			// D3D path
			mPlanes[FRUSTUM_PLANE_NEAR].n.x	= - combo[0][2];
			mPlanes[FRUSTUM_PLANE_NEAR].n.y	= - combo[1][2];
			mPlanes[FRUSTUM_PLANE_NEAR].n.z	= - combo[2][2];
			mPlanes[FRUSTUM_PLANE_NEAR].d	= - combo[3][2];
		}

		// Far clipping plane (must be last for infinite far clip)
		mPlanes[FRUSTUM_PLANE_FAR].n.x	= -(combo[0][3] - combo[0][2]);
		mPlanes[FRUSTUM_PLANE_FAR].n.y	= -(combo[1][3] - combo[1][2]);
		mPlanes[FRUSTUM_PLANE_FAR].n.z	= -(combo[2][3] - combo[2][2]);
		mPlanes[FRUSTUM_PLANE_FAR].d	= -(combo[3][3] - combo[3][2]);

		// Normalize if needed
		for(PxU32 i=0;i<6;i++)
		{
//			mPlanes[i].normalize();
			mPlanes[i].n.normalize();
//			mPlanes[i].normal = -mPlanes[i].normal;
//			mPlanes[i].d = -mPlanes[i].d;
			mPlanes[i].d *= 0.5f;
		}
	}
}

	// Following code from Umbra/dPVS.

	//------------------------------------------------------------------------
	//
	// Function:        DPVS::intersectAABBFrustum()
	//
	// Description:     Determines whether an AABB intersects a frustum
	//
	// Parameters:      a           = reference to AABB (defined by minimum & maximum vectors)
	//                  p           = array of pre-normalized clipping planes
	//                  outClipMask = output clip mask (if function returns 'true')
	//                  inClipMask  = input clip mask (indicates which planes are active)
	//
	// Returns:         true if AABB intersects the frustum, false otherwise
	//
	//                  Intersection of AABB and a frustum. The frustum may 
	//                  contain 0-32 planes (active planes are defined by inClipMask). 
	//                  If AABB intersects the frustum, an output clip mask is returned 
	//                  as well (indicating which planes are crossed by the AABB). This 
	//                  information can be used to optimize testing of child nodes or 
	//                  objects inside the nodes (pass value as 'inClipMask' next time).
	//
	//                  This is a variant of the classic "fast" AABB/frustum 
	//                  intersection tester. AABBs that are not culled away by any single 
	//                  plane are classified as "intersecting" even though the AABB may 
	//                  actually be outside the convex volume formed by the planes.
	//------------------------------------------------------------------------

static PX_FORCE_INLINE bool planesAABBOverlap(const PxBounds3& a, const PxPlane* p, PxU32& out_clip_mask, PxU32 in_clip_mask)
	{
		//------------------------------------------------------------------------
		// Convert the AABB from (minimum,maximum) form into (center,half-diagonal).
		// Note that we could get rid of these six subtractions and three
		// multiplications if the AABB was originally expressed in (center,
		// half-diagonal) form.
		//------------------------------------------------------------------------

		PxVec3 m = a.getCenter();		// get center of AABB ((minimum+maximum)*0.5f)
		PxVec3 d = a.maximum;	d-=m;	// get positive half-diagonal (maximum - center)

		//------------------------------------------------------------------------
		// Evaluate through all active frustum planes. We determine the relation 
		// between the AABB and a plane by using the concept of "near" and "far"
		// vertices originally described by Zhang (and later by Moeller). Our
		// variant here uses 3 fabs ops, 6 muls, 7 adds and two floating point
		// comparisons per plane. The routine early-exits if the AABB is found
		// to be outside any of the planes. The loop also constructs a new output
		// clip mask. Most FPUs have a native single-cycle fabsf() operation.
		//------------------------------------------------------------------------

		PxU32 Mask				= 1;			// current mask index (1,2,4,8,..)
		PxU32 TmpOutClipMask	= 0;			// initialize output clip mask into empty. 

		while(Mask<=in_clip_mask)				// keep looping while we have active planes left...
		{
			if(in_clip_mask & Mask)				// if clip plane is active, process it..
			{               
				const float NP = d.x*PxAbs(p->n.x) + d.y*PxAbs(p->n.y) + d.z*PxAbs(p->n.z);
				const float MP = m.x*p->n.x + m.y*p->n.y + m.z*p->n.z + p->d;

				if(NP < MP)						// near vertex behind the clip plane... 
					return false;				// .. so there is no intersection..
				if((-NP) < MP)					// near and far vertices on different sides of plane..
					TmpOutClipMask |= Mask;		// .. so update the clip mask...
			}
			Mask+=Mask;							// mk = (1<<plane)
			p++;								// advance to next plane
		}

		out_clip_mask = TmpOutClipMask;			// copy output value (temp used to resolve aliasing!)
		return true;							// indicate that AABB intersects frustum
	}

PlaneAABBCode Camera::cull(const PxBounds3& aabb) const
{
	const PxU32 nbFrustumPlanes		= 6;	// PT: can sometimes be 5 with infinite far clip plane
	const PxU32 frustumPlanesMask	= (1<<nbFrustumPlanes)-1;

	PxU32 outClipMask;
	if(!planesAABBOverlap(aabb, mPlanes, outClipMask, frustumPlanesMask))
		return PLANEAABB_EXCLUSION;

	if(outClipMask)
		return PLANEAABB_INTERSECT;

	return PLANEAABB_INCLUSION;
}

void Camera::drawDebug(RenderPhysX3Debug* debug)
{
	if(mDrawDebugData)
	{
/*		for(PxU32 i=0;i<8;i++)
		{
			debug->addLine(mFrustum[i], mFrustum[i]+PxVec3(1,0,0), RendererColor(255,0,0));
			debug->addLine(mFrustum[i], mFrustum[i]+PxVec3(0,1,0), RendererColor(0, 255,0));
			debug->addLine(mFrustum[i], mFrustum[i]+PxVec3(0,0,1), RendererColor(0, 0, 255));
		}*/

		const RendererColor lineColor(255, 255, 0);
		debug->addLine(mFrustum[0], mFrustum[1], lineColor);
		debug->addLine(mFrustum[1], mFrustum[2], lineColor);
		debug->addLine(mFrustum[2], mFrustum[3], lineColor);
		debug->addLine(mFrustum[3], mFrustum[0], lineColor);

		debug->addLine(mFrustum[4], mFrustum[5], lineColor);
		debug->addLine(mFrustum[5], mFrustum[6], lineColor);
		debug->addLine(mFrustum[6], mFrustum[7], lineColor);
		debug->addLine(mFrustum[7], mFrustum[4], lineColor);

		debug->addLine(mFrustum[0], mFrustum[4], lineColor);
		debug->addLine(mFrustum[3], mFrustum[7], lineColor);
		debug->addLine(mFrustum[1], mFrustum[5], lineColor);
		debug->addLine(mFrustum[6], mFrustum[2], lineColor);
	}
}
