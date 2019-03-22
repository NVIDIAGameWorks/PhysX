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

#ifndef SAMPLE_CAMERA_H
#define SAMPLE_CAMERA_H

#include "SampleAllocator.h"
#include "RendererProjection.h"
#include "foundation/PxPlane.h"

	class RenderPhysX3Debug;

	struct Viewport : public SampleAllocateable
	{
				Viewport() : mClientWidth(0), mClientHeight(0), mWindowWidth(0), mWindowHeight(0)	{}

		PxU32	mClientWidth;
		PxU32	mClientHeight;
		PxU32	mWindowWidth;
		PxU32	mWindowHeight;

		PX_FORCE_INLINE	PxReal	computeRatio()	const	{ return PxReal(mWindowWidth)/PxReal(mWindowHeight);	}
	};

	enum PlaneAABBCode
	{
		PLANEAABB_EXCLUSION,
		PLANEAABB_INTERSECT,
		PLANEAABB_INCLUSION
	};

	class Camera : public SampleAllocateable
	{
		public:
													Camera();
													~Camera();

		///////////////////////////////////////////////////////////////////////////////

		// Projection part

		PX_FORCE_INLINE	PxReal						getFOV()			const	{ return mFOV;						}
		PX_FORCE_INLINE	PxReal						getNearPlane()		const	{ return mNearPlane;				}
		PX_FORCE_INLINE	PxReal						getFarPlane()		const	{ return mFarPlane;					}
		PX_FORCE_INLINE	PxU32						getScreenWidth()	const	{ return mViewport.mClientWidth;	}
		PX_FORCE_INLINE	PxU32						getScreenHeight()	const	{ return mViewport.mClientHeight;	}

		PX_FORCE_INLINE	void						setFOV(PxReal fov)			{ mFOV			= fov;	mDirtyProj = true;	}
		PX_FORCE_INLINE	void						setNearPlane(PxReal d)		{ mNearPlane	= d;	mDirtyProj = true;	}
		PX_FORCE_INLINE	void						setFarPlane(PxReal d)		{ mFarPlane		= d;	mDirtyProj = true;	}
		PX_FORCE_INLINE	void						setScreenSize(PxU32 clientWidth, PxU32 clientHeight, PxU32 windowWidth, PxU32 windowHeight)
													{
														mViewport.mClientWidth	= clientWidth;
														mViewport.mClientHeight	= clientHeight;
														mViewport.mWindowWidth	= windowWidth;
														mViewport.mWindowHeight	= windowHeight;
														mDirtyProj				= true;
													}

		PX_FORCE_INLINE	const SampleRenderer::RendererProjection&
													getProjMatrix()		const
													{
														if(mDirtyProj)
															const_cast<Camera*>(this)->updateInternals();

														return mProjMatrix;
													}

		///////////////////////////////////////////////////////////////////////////////

		// View part

		PX_FORCE_INLINE	const PxVec3&				getPos()					const	{ return mPos;								}
		PX_FORCE_INLINE	const PxVec3&				getRot()					const	{ return mRot;								}

		PX_FORCE_INLINE	void						setPos(const PxVec3& pos)			{ mPos = pos;			mDirtyView = true;	}
		PX_FORCE_INLINE	void						setRot(const PxVec3& rot)			{ mRot = rot;			mDirtyView = true;	}
		PX_FORCE_INLINE	void						setView(const PxTransform& view)	{ mViewMatrix = view;	mPos = view.p; mDirtyView = false;	}

		PX_FORCE_INLINE	const PxTransform&			getViewMatrix()				const
													{
														if(mDirtyView)
															const_cast<Camera*>(this)->updateInternals();

														return mViewMatrix;
													}

						PxVec3						getViewDir()				const;
						void						lookAt(const PxVec3& position, const PxVec3& target);

		///////////////////////////////////////////////////////////////////////////////

		PX_FORCE_INLINE	const PxVec3*				getFrustumVerts()	const	{ return mFrustum;	}

		///////////////////////////////////////////////////////////////////////////////

		// Culling

						PlaneAABBCode				cull(const PxBounds3& aabb)	const;
						bool						mDrawDebugData;
						bool						mFreezeFrustum;
						bool						mPerformVFC;

		///////////////////////////////////////////////////////////////////////////////

						void						drawDebug(RenderPhysX3Debug*);
		private:
		mutable			SampleRenderer::RendererProjection
													mProjMatrix;
		mutable			PxTransform					mViewMatrix;

						PxVec3						mPos;
						PxVec3						mRot;

						Viewport					mViewport;
						PxReal						mFOV;
						PxReal						mNearPlane;
						PxReal						mFarPlane;

						PxVec3						mFrustum[8];	//!< Frustum's vertices
						PxPlane						mPlanes[6];		//!< Frustum's planes

						bool						mDirtyProj;
						bool						mDirtyView;

						void						updateInternals();
	public:
						void						BuildFrustum();
	};

#endif
