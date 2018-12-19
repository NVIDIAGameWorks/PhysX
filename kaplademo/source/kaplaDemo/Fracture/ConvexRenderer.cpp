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

#include "Shader.h"
#include "ConvexRenderer.h"
#include "Compound.h"
#include <foundation/PxMat44.h>

//--------------------------------------------------------
ConvexRenderer::ConvexRenderer()
{
	init();
}

//--------------------------------------------------------
ConvexRenderer::~ConvexRenderer()
{
}

//--------------------------------------------------------
void ConvexRenderer::init()
{
	mShader = NULL;
	mShaderMat.init();

	mBumpTextureUVScale = 0.1f;
	mExtraNoiseScale = 2.0f;
	mRoughnessScale = 0.2f;

	mDiffuseTexArray = 0;
	mBumpTexArray = 0;
	mSpecularTexArray = 0;
	mEmissiveReflectSpecPowerTexArray = 0;

	mActive = true;
}

//--------------------------------------------------------
void ConvexRenderer::add(const base::Convex* convex, Shader* shader)
{
	if (!mActive)
		return; 

	ConvexGroup *g;

	// int gnr = 0;					// to search all groups 
	int gnr = 0;// !mGroups.empty() ? mGroups.size() - 1 : 0;		// to search only last
	int numNewVerts = convex->getVisVertices().size();

	while (gnr < (int)mGroups.size() && (mGroups[gnr]->numVertices + numNewVerts >= maxVertsPerGroup || mGroups[gnr]->mShader != shader))
		gnr++;

	if (gnr == (int)mGroups.size()) {	// create new group
		g = new ConvexGroup();
		g->init();
		g->mShader = shader;
		gnr = mGroups.size();
		mGroups.push_back(g);
	}
	g = mGroups[gnr];
	convex->setConvexRendererInfo(gnr, g->convexes.size());
	g->convexes.push_back((Convex*)convex);
	g->numIndices += convex->getVisTriIndices().size();
	g->numVertices += convex->getVisVertices().size();
	g->dirty = true;

}

//--------------------------------------------------------
void ConvexRenderer::remove(const base::Convex* convex)
{
	if (!mActive)
		return; 

	int gnr = convex->getConvexRendererGroupNr();
	int pos = convex->getConvexRendererGroupPos();
	if (gnr < 0 || gnr >= (int)mGroups.size())
		return;

	ConvexGroup *g = mGroups[gnr];
	if (pos < 0 || pos > (int)g->convexes.size())
		return;

	if (g->convexes[pos] != convex)
		return;

	g->numIndices -= convex->getVisTriIndices().size();
	g->numVertices -= convex->getVisVertices().size();

	g->convexes[pos] = g->convexes[g->convexes.size()-1];
	g->convexes[pos]->setConvexRendererInfo(gnr, pos);
	g->convexes.pop_back();
	g->dirty = true;
}

//--------------------------------------------------------
void ConvexRenderer::updateRenderBuffers()
{
	/*
	static int maxNumV = 0;
	static int maxNumI = 0;
	static int maxNumC = 0;
	static int maxNumG = 0;
	if (mGroups.size() > maxNumG) maxNumG = mGroups.size();
	*/

	for (int i = 0; i < (int)mGroups.size(); i++) {
	
		
		ConvexGroup *g = mGroups[i];
		if (!g->dirty)
			continue;

		if (g->numIndices == 0 || g->numVertices == 0)
			continue;

		if (!g->VBO) {
			glGenBuffersARB(1, &g->VBO);
		}
		if (!g->IBO) {
			glGenBuffersARB(1, &g->IBO);
		}
		if (!g->matTex) {
			glGenTextures(1, &g->matTex);
			glBindTexture(GL_TEXTURE_2D, g->matTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			glBindTexture(GL_TEXTURE_2D, 0);

		}
		if (g->numVertices == 0 || g->numIndices == 0)
			return;

		// recalculate in case they have changed
		g->numVertices = 0;
		g->numIndices = 0;
		for (int i = 0; i < (int)g->convexes.size(); i++) {
			g->numVertices += g->convexes[i]->getVisVertices().size();
			g->numIndices += g->convexes[i]->getVisTriIndices().size();
		}

		g->vertices.resize(g->numVertices*12);
		g->indices.resize(g->numIndices);
		float* vp = &g->vertices[0];
		unsigned int* ip = &g->indices[0];
		
		int sumV = 0;
		// Make cpu copy of VBO and IBO

		int convexNr = 0;
		for (int j = 0; j < (int)g->convexes.size(); j++, convexNr++) {
			const Convex* c = g->convexes[j];
	//			PxVec3 matOff = c->getMaterialOffset();

			int nv = c->getVisVertices().size();
			int ni = c->getVisTriIndices().size();

			if (nv > 0) {
				float* cvp = (float*)&c->getVisVertices()[0];		// float3
				float* cnp = (float*)&c->getVisNormals()[0];		// float3
	//				float* c3dtp = (float*)&c->getVisVertices()[0];		// float3
				float* c2dtp = (float*)&c->getVisTexCoords()[0];	// float2
				float* ctanp = (float*)&c->getVisTangents()[0];		// float3

				int* cip = (int*)&c->getVisTriIndices()[0];
				for (int k = 0; k < nv; k++) {
					*(vp++) = *(cvp++);
					*(vp++) = *(cvp++);
					*(vp++) = *(cvp++);
				
					*(vp++) = *(cnp++);
					*(vp++) = *(cnp++);
					*(vp++) = *(cnp++);				

					*(vp++) = *(ctanp++);
					*(vp++) = *(ctanp++);
					*(vp++) = *(ctanp++);
					*(vp++) = (float)convexNr;

					*(vp++) = *(c2dtp++);
					*(vp++) = *(c2dtp++);

				}
				for (int k = 0; k < ni; k++) {
					*(ip++) = *(cip++) + sumV;
				}
			}
			//memcpy(ip, cip, sizeof(int)*ni);
			//ip += 3*ni;

			//std::vector<PxVec3> mTriVertices;
			//std::vector<PxVec3> mTriNormals;
			//std::vector<int> mTriIndices;
			//std::vector<float> mTriTexCoords;	// 3d + obj nr
			sumV += nv;
		}
/*
		if (g->vertices.size() > maxNumV) maxNumV = g->vertices.size();
		if (g->indices.size() > maxNumI) maxNumI = g->indices.size();
		if (g->convexes.size() > maxNumC) maxNumC = g->convexes.size();
*/
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, g->VBO);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, g->vertices.size()*sizeof(float), &g->vertices[0], GL_DYNAMIC_DRAW);

		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, g->IBO);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, g->indices.size()*sizeof(unsigned int), &g->indices[0], GL_DYNAMIC_DRAW);

		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

		int oldTexSize = g->texSize;
		if (g->texSize == 0) {
			// First time
			oldTexSize = 1;
			g->texSize = 32;
		}

		while (1) {
			int convexesPerRow = g->texSize / 4;
			if (convexesPerRow * g->texSize >= (int)g->convexes.size()) {
				break;
			} else {
				g->texSize *= 2;
			}
		}
		if (g->texSize != oldTexSize) {
			g->texCoords.resize(g->texSize*g->texSize*4);
			// Let's allocate texture
			glBindTexture(GL_TEXTURE_2D, g->matTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, g->texSize, g->texSize, 0, GL_RGBA, GL_FLOAT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		g->dirty = false;
	}

//	printf("maxV = %d, maxI = %d, maxC = %d, maxG = %d\n", maxNumV, maxNumI, maxNumC, maxNumG);

}

//--------------------------------------------------------
void ConvexRenderer::updateTransformations() 
{
	for (int i = 0; i < (int)mGroups.size(); i++) {
		ConvexGroup *g = mGroups[i];
		if (g->texCoords.empty())
			continue;

		float* tt = &g->texCoords[0];

		for (int j = 0; j < (int)g->convexes.size(); j++) {
			const Convex* c = g->convexes[j];

			PxMat44 pose(c->getGlobalPose());
			float* mp = (float*)pose.front();

			float* ta = tt;
			for (int k = 0; k < 16; k++) {
				*(tt++) = *(mp++);
	 		}
			PxVec3 matOff = c->getMaterialOffset();
			ta[3] = matOff.x;
			ta[7] = matOff.y;
			ta[11] = matOff.z;

			int idFor2DTex = c->getSurfaceMaterialId();
			int idFor3DTex = c->getMaterialId();
			const int MAX_3D_TEX = 8;
			ta[15] = (float)(idFor2DTex*MAX_3D_TEX + idFor3DTex);


		}

		glBindTexture(GL_TEXTURE_2D, g->matTex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g->texSize, g->texSize, 
			GL_RGBA, GL_FLOAT, &g->texCoords[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

//--------------------------------------------------------
void ConvexRenderer::render()
{
	if (!mActive)
		return; 

	updateRenderBuffers();

	updateTransformations();

	for (int i = 0; i < (int)mGroups.size(); i++) {
	
		ConvexGroup *g = mGroups[i];

		Shader* shader = mShader;
		if (g->mShader != NULL)
			shader = g->mShader;

		shader->activate(mShaderMat);
		// Assume convex all use the same shader

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_3D, volTex);				

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, g->matTex);				

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mDiffuseTexArray);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mBumpTexArray);
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mSpecularTexArray);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, mEmissiveReflectSpecPowerTexArray);

		glActiveTexture(GL_TEXTURE0);
		
		float itt = 1.0f/g->texSize;
		
		shader->setUniform("diffuseTexArray", 10);
		shader->setUniform("bumpTexArray", 11);
		shader->setUniform("specularTexArray", 12);
		shader->setUniform("emissiveReflectSpecPowerTexArray", 13);

		shader->setUniform("ttt3D", 7);
		shader->setUniform("transTex", 8);
		shader->setUniform("transTexSize", g->texSize);
		shader->setUniform("iTransTexSize", itt);
		shader->setUniform("bumpTextureUVScale", mBumpTextureUVScale);
		shader->setUniform("extraNoiseScale", mExtraNoiseScale);
		shader->setUniform("roughnessScale", mRoughnessScale);

		if (mShaderMat.color[3] < 1.0f) {
			glEnable(GL_BLEND); 
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_DEPTH_TEST);
			glColor4f(mShaderMat.color[0], mShaderMat.color[1], mShaderMat.color[2], mShaderMat.color[3]);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glBindBufferARB(GL_ARRAY_BUFFER_ARB, g->VBO);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, g->IBO);

		int stride = 12*sizeof(float);
		glVertexPointer(3, GL_FLOAT, stride, 0);
		
		glNormalPointer(GL_FLOAT, stride, (void*)(3*sizeof(float)));
		
		glTexCoordPointer(4, GL_FLOAT, stride, (void*)(6*sizeof(float)));

		glClientActiveTexture(GL_TEXTURE1);
		glTexCoordPointer(2, GL_FLOAT, stride, (void*)(10*sizeof(float)));
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glDrawElements(GL_TRIANGLES, g->numIndices, GL_UNSIGNED_INT, 0);	
		
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

		if (mShaderMat.color[3] < 1.0f) {
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
		}
		
		shader->deactivate();
	}
}