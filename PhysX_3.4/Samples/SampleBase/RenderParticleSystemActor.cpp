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

#include "PxPhysXConfig.h"

#if PX_USE_PARTICLE_SYSTEM_API

#include "RenderParticleSystemActor.h"
#include "RendererParticleSystemShape.h"
#include "RendererSimpleParticleSystemShape.h"
#include "ParticleSystem.h"
#include "RendererMemoryMacros.h"

using namespace SampleRenderer;

RenderParticleSystemActor::RenderParticleSystemActor(SampleRenderer::Renderer& renderer, 
													ParticleSystem* ps,
													bool _mesh_instancing,
													bool _fading,
													PxReal fadingPeriod,
													PxReal debriScaleFactor) : mRenderer(renderer), 
																			mPS(ps),
																			mUseMeshInstancing(_mesh_instancing),
																			mFading(_fading)
{
	if(mRenderer.isSpriteRenderingSupported())
	{
		RendererShape* rs = new SampleRenderer::RendererParticleSystemShape(mRenderer, 
											mPS->getPxParticleBase()->getMaxParticles(), 
											mUseMeshInstancing,
											mFading,
											fadingPeriod,
											debriScaleFactor,
											mPS->getCudaContextManager());
		setRenderShape(rs);
		mUseSpritesMesh = true;
	}
	else
	{
		RendererShape* rs = new SampleRenderer::RendererSimpleParticleSystemShape(mRenderer, 
											mPS->getPxParticleBase()->getMaxParticles());
		setRenderShape(rs);
		mUseSpritesMesh = false;
	}
}

RenderParticleSystemActor::~RenderParticleSystemActor() 
{
	DELETESINGLE(mPS);
}

void RenderParticleSystemActor::update(float deltaTime) 
{
	setTransform(PxTransform(PxIdentity));

#if defined(RENDERER_ENABLE_CUDA_INTEROP)

	SampleRenderer::RendererParticleSystemShape* shape = 
		static_cast<SampleRenderer::RendererParticleSystemShape*>(getRenderShape());

	if (shape->isInteropEnabled() && (mPS->getPxParticleSystem().getParticleBaseFlags()&PxParticleBaseFlag::eGPU))
	{
		PxParticleReadData* data = mPS->getPxParticleSystem().lockParticleReadData(PxDataAccessFlag::eREADABLE | PxDataAccessFlag::eDEVICE);

		if(data)
		{
			if(mUseMeshInstancing) 
			{
				shape->updateInstanced(mPS->getValidParticleRange(),
					reinterpret_cast<CUdeviceptr>(&data->positionBuffer[0]),			
					mPS->getValiditiesDevice(),
					mPS->getOrientationsDevice(),
					data->nbValidParticles);
			} 
			else 
			{
				CUdeviceptr lifetimes = 0;
				if(mFading && mPS->useLifetime()) 
					lifetimes = mPS->getLifetimesDevice();

				shape->updateBillboard(mPS->getValidParticleRange(),
					reinterpret_cast<CUdeviceptr>(&data->positionBuffer[0]),			
					mPS->getValiditiesDevice(),
					lifetimes,
					data->nbValidParticles);
			}

			data->unlock();
		}
	}
	else

#endif

	{
		PxParticleReadData* data = mPS->getPxParticleSystem().lockParticleReadData(PxDataAccessFlag::eREADABLE);

		if(data)
		{
			if(mUseMeshInstancing) 
			{
				SampleRenderer::RendererParticleSystemShape* shape = 
					static_cast<SampleRenderer::RendererParticleSystemShape*>(getRenderShape());
				shape->updateInstanced(mPS->getValidParticleRange(),
								&(mPS->getPositions()[0]), 
								mPS->getValidity(),
								&(mPS->getOrientations()[0]));

			} 
			else 
			{
				if(mUseSpritesMesh)
				{
					SampleRenderer::RendererParticleSystemShape* shape = 
						static_cast<SampleRenderer::RendererParticleSystemShape*>(getRenderShape());
					const PxReal* lifetimes = NULL;
					if(mFading && mPS->useLifetime()) 
					{
						lifetimes = &(mPS->getLifetimes()[0]);
					}
					shape->updateBillboard(mPS->getValidParticleRange(),
									&(mPS->getPositions()[0]), 
									mPS->getValidity(),
									lifetimes);
				}
				else
				{
					SampleRenderer::RendererSimpleParticleSystemShape* shape = 
						static_cast<SampleRenderer::RendererSimpleParticleSystemShape*>(getRenderShape());
					const PxReal* lifetimes = NULL;
					if(mFading && mPS->useLifetime()) 
					{
						lifetimes = &(mPS->getLifetimes()[0]);
					}
					shape->updateBillboard(mPS->getValidParticleRange(),
						&(mPS->getPositions()[0]), 
						mPS->getValidity(),
						lifetimes);
				}
			}

			data->unlock();
		}
	}
}

void RenderParticleSystemActor::updateSubstep(float deltaTime)
{
	mPS->update(deltaTime);
}

#endif // PX_USE_PARTICLE_SYSTEM_API
