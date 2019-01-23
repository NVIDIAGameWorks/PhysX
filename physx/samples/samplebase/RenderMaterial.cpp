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

#include "SamplePreprocessor.h"
#include "RenderMaterial.h"
#include "RenderTexture.h"
#include "RendererMaterialDesc.h"
#include "Renderer.h"
#include "RendererMaterial.h"
#include "RendererMaterialInstance.h"
#include "RendererMemoryMacros.h"

using namespace physx;
using namespace SampleRenderer;

const char* defaultMaterialLitVertexShader = "vertex/staticmesh.cg";
const char* defaultMaterialTexturedUnlitFragmentShader = "fragment/sample_diffuse_and_texture.cg";
const char* defaultMaterialTexturedLitFragmentShader = "fragment/sample_diffuse_and_texture.cg";
const char* defaultMaterialFragmentShader = "fragment/sample_diffuse_no_texture.cg";

RenderMaterial::RenderMaterial(Renderer& renderer, const PxVec3& diffuseColor, PxReal opacity, bool doubleSided, PxU32 id, RenderTexture* texture, bool lit, bool flat, bool instanced) :
	mRenderMaterial			(NULL),
	mRenderMaterialInstance	(NULL),
	mID						(id),
	mDoubleSided			(doubleSided),
	mOwnsRendererMaterial	(true)
{
	RendererMaterialDesc matDesc;
	if(lit)
		matDesc.type			= RendererMaterial::TYPE_LIT;
	else 
		matDesc.type			= RendererMaterial::TYPE_UNLIT;
	matDesc.alphaTestFunc		= RendererMaterial::ALPHA_TEST_ALWAYS;
	matDesc.alphaTestRef		= 0.0f;
	if(opacity==1.0f)
	{
		matDesc.blending		= false;
		matDesc.srcBlendFunc	= RendererMaterial::BLEND_ONE;
		matDesc.dstBlendFunc	= RendererMaterial::BLEND_ONE;
	}
	else
	{
		matDesc.type			= RendererMaterial::TYPE_UNLIT;
		matDesc.blending		= true;
//		matDesc.srcBlendFunc	= RendererMaterial::BLEND_ONE;
//		matDesc.dstBlendFunc	= RendererMaterial::BLEND_ONE;
		matDesc.srcBlendFunc	= RendererMaterial::BLEND_SRC_ALPHA;
		matDesc.dstBlendFunc	= RendererMaterial::BLEND_ONE_MINUS_SRC_ALPHA;
	}

	if(instanced)
	{
		matDesc.instanced = true;
	}
	else
	{
		matDesc.instanced = false;
	}

	matDesc.geometryShaderPath	= NULL;
	
	matDesc.vertexShaderPath	= defaultMaterialLitVertexShader;
		
	if(texture)
	{
		if(lit)
		{
			matDesc.fragmentShaderPath	= defaultMaterialTexturedLitFragmentShader;
		}
		else
		{
			matDesc.fragmentShaderPath	= defaultMaterialTexturedUnlitFragmentShader;
		}
	}
	else
	{
		matDesc.fragmentShaderPath	= defaultMaterialFragmentShader;
	}
	PX_ASSERT(matDesc.isValid());

	mRenderMaterial = renderer.createMaterial(matDesc);
	mRenderMaterialInstance = new RendererMaterialInstance(*mRenderMaterial);

	setDiffuseColor(PxVec4(diffuseColor.x, diffuseColor.y, diffuseColor.z, opacity));
	setShadeMode(flat);

	update(renderer);

	if(texture)
	{
		const RendererMaterial::Variable* var = mRenderMaterialInstance->findVariable("diffuseTexture", RendererMaterial::VARIABLE_SAMPLER2D);
		//PX_ASSERT(var);
		if(var)
			mRenderMaterialInstance->writeData(*var, &texture->mTexture);
	}
}

RenderMaterial::RenderMaterial(Renderer& renderer, RendererMaterial* mat, RendererMaterialInstance* matInstance, PxU32 id) :
	mRenderMaterial			(mat),
	mRenderMaterialInstance	(matInstance),
	mID						(id),
	mDoubleSided			(false),
	mOwnsRendererMaterial	(false)
{
	update(renderer);
}

void RenderMaterial::update(SampleRenderer::Renderer& renderer)
{
	const RendererMaterial::Variable* var = mRenderMaterialInstance->findVariable("windowWidth", RendererMaterial::VARIABLE_FLOAT);
	if(var)
	{
		PxU32 tmpWindowWidth, tmpWindowHeight;
		renderer.getWindowSize(tmpWindowWidth, tmpWindowHeight);

		const PxReal windowWidth = PxReal(tmpWindowWidth);
		mRenderMaterialInstance->writeData(*var, &windowWidth);
	}

}

void RenderMaterial::setParticleSize(const PxReal particleSize)
{
	const RendererMaterial::Variable* var = mRenderMaterialInstance->findVariable("particleSize", RendererMaterial::VARIABLE_FLOAT);
	if(var)
	{
		mRenderMaterialInstance->writeData(*var, &particleSize);
	}	
}

void RenderMaterial::setDiffuseColor(const PxVec4& color)
{
	const RendererMaterial::Variable* var = mRenderMaterialInstance->findVariable("diffuseColor", RendererMaterial::VARIABLE_FLOAT4);
	if(var)
	{
		const PxReal data[] = { color.x, color.y, color.z, color.w };
		mRenderMaterialInstance->writeData(*var, data);
	}
}

void RenderMaterial::setShadeMode(bool flat)
{
	const RendererMaterial::Variable* var = mRenderMaterialInstance->findVariable("shadeMode", RendererMaterial::VARIABLE_FLOAT);
	if(var)
	{
		float shadeMode = flat?1.0f:0.0f;
		mRenderMaterialInstance->writeData(*var, &shadeMode);
	}
}

RenderMaterial::~RenderMaterial()
{
	if(mOwnsRendererMaterial)
	{
		DELETESINGLE(mRenderMaterialInstance);
		SAFE_RELEASE(mRenderMaterial);
	}
}
