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

#ifndef CONVEX_RENDERER
#define CONVEX_RENDERER

#include "Convex.h"
#include <vector>

using namespace physx;

class Shader;

// ----------------------------------------------------------------------------
class ConvexRenderer
{
public:
	ConvexRenderer();
	~ConvexRenderer();

	void init();

	const static int maxVertsPerGroup = 100000;

	void setActive(bool active) { mActive = active; };

	void add(const base::Convex* convex, Shader* shader);
	void remove(const base::Convex* convex);

	void render();
	void setShaderMaterial(Shader* shader, const ShaderMaterial& mat) {this->mShader = shader; this->mShaderMat = mat;}
	void setTexArrays(unsigned int diffuse, unsigned int bump, unsigned int specular, unsigned int specPower) {
		mDiffuseTexArray = diffuse; mBumpTexArray = bump; 
		mSpecularTexArray = specular; mEmissiveReflectSpecPowerTexArray = specPower; }
	void setVolTex(unsigned int volTexi) { volTex = volTexi;}
private:
	void updateRenderBuffers();
	void updateTransformations();

	Shader* mShader;
	ShaderMaterial mShaderMat;

	struct ConvexGroup {
		void init() {
			numVertices = 0; numIndices = 0;
			VBO = 0; IBO = 0; matTex = 0;
			texSize = 0;
		}
		bool dirty;
		std::vector<const Convex*> convexes;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;
		std::vector<float> texCoords;

		int numVertices, numIndices;
		unsigned int VBO;
		unsigned int IBO;
		unsigned int matTex;
		int texSize;

		Shader* mShader;
	};

	std::vector<ConvexGroup*> mGroups;

	bool mActive;

	float mBumpTextureUVScale;
	float mExtraNoiseScale;
	float mRoughnessScale;

	unsigned int mDiffuseTexArray;
	unsigned int mBumpTexArray;
	unsigned int mSpecularTexArray;
	unsigned int mEmissiveReflectSpecPowerTexArray;
	unsigned int volTex;
};

#endif
