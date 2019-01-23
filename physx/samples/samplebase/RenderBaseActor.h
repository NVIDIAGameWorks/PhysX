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

#ifndef RENDER_BASE_ACTOR_H
#define RENDER_BASE_ACTOR_H

#include "RendererMeshContext.h"
#include "common/PxPhysXCommonConfig.h"
#include "RenderBaseObject.h"
#include "geometry/PxMeshScale.h"
#include "foundation/PxBounds3.h"

namespace SampleRenderer
{
	class RendererShape;
}

	class RenderMaterial;

namespace physx
{
	class PxShape;
	class PxRigidActor;
	class PxRigidDynamic;
	class PxArticulationLink;
	class PxMeshScale;
}

	class RenderPhysX3Debug;

	class RenderBaseActor : public RenderBaseObject
	{
		public:
																RenderBaseActor();
																RenderBaseActor(const RenderBaseActor&);
		virtual													~RenderBaseActor();

		virtual			void									update(float deltaTime);
		virtual			void									render(SampleRenderer::Renderer& renderer, RenderMaterial* material=NULL, bool wireFrame = false);
		virtual			void									drawDebug(RenderPhysX3Debug*);

		PX_FORCE_INLINE	void									setTransform(const PxTransform& tr)		{ mTransform = tr; updateScale(); }
		PX_FORCE_INLINE	void									setMeshScale(const PxMeshScale& scaling){ mScaling = scaling; updateScale(); }
						void									setPhysicsShape(PxShape* shape, PxRigidActor* actor);
						void									setRenderMaterial(RenderMaterial*);

		PX_FORCE_INLINE	const PxTransform&						getTransform()		const	{ return mTransform;		}
		PX_FORCE_INLINE	PxShape*								getPhysicsShape()	const	{ return mPhysicsShape;		}
		PX_FORCE_INLINE	PxRigidActor*							getPhysicsActor()	const	{ return mPhysicsActor;		}
		PX_FORCE_INLINE	SampleRenderer::RendererShape*			getRenderShape()			{ return mRendererShape;	}
		PX_FORCE_INLINE	SampleRenderer::RendererShape*			getRenderShape()	const	{ return mRendererShape;	}
		PX_FORCE_INLINE	RenderMaterial*							getRenderMaterial()			{ return mMaterial;			}
		PX_FORCE_INLINE	RenderMaterial*							getRenderMaterial()	const	{ return mMaterial;			}

						PxBounds3								getWorldBounds()	const;
						void									setWorldBounds(const PxBounds3& bounds);

		PX_FORCE_INLINE	void									setRaycastCCD(bool flag)					{ mEnableCCD = flag;			}
		PX_FORCE_INLINE	void									setCCDWitnessOffset(const PxVec3& offset)	{ mCCDWitnessOffset = offset;	}

		PX_FORCE_INLINE	void									setRendering(bool flag)						{ mEnableRender = flag;			}
		
		PX_FORCE_INLINE	void									setEnableDebugRender(bool flag)				{ mEnableDebugRender = flag;	}
		PX_FORCE_INLINE bool                                    getEnableDebugRender()				const	{ return mEnableDebugRender;	}

		PX_FORCE_INLINE	void									setEnableCameraCull(bool flag)				{ mEnableCameraCull = flag;	}
		PX_FORCE_INLINE bool                                    getEnableCameraCull()				const	{ return mEnableCameraCull;	}

		private:
						SampleRenderer::RendererShape*			mRendererShape;
						PxMeshScale								mScaling;
						PxTransform								mTransform;
						PxBounds3								mWorldBounds;

		protected:

						SampleRenderer::RendererMeshContext		mRendererMeshContext;
						PxMat44									mScaledTransform;
						PxQuat									mPhysicsToGraphicsRot;

						PxShape*								mPhysicsShape;
						PxRigidActor*							mPhysicsActor;
						PxRigidDynamic*							mDynamicActor;
						PxArticulationLink*						mArticulationLink;
						RenderMaterial*							mMaterial;

						PxVec3									mCCDWitness;
						PxVec3									mCCDWitnessOffset;
						bool									mEnableCCD;
						bool									mEnableRender;
						bool                                    mEnableDebugRender;
						bool									mEnableCameraCull;
		protected:
						void									setRenderShape(SampleRenderer::RendererShape*);
						void									deleteRenderShape();
	private:
						void									updateScale();
	};

#endif
