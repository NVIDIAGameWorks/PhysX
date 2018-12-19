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

#include "SimScene.h"
#include <foundation/PxMat44.h>
#include "PxRigidBodyExt.h"

#include "Actor.h"
#include "Compound.h"
#include "Convex.h"
#include "CompoundCreator.h"
#include "PolygonTriangulator.h"

#include "SampleViewerScene.h"

#include <windows.h>

#include <iostream>

using namespace std;

#include <vector>
#include "TerrainMesh.h"

#define USE_CONVEX_RENDERER 1
#define NUM_NO_SOUND_FRAMES 1

//#define REL_VEL_FRACTURE_THRESHOLD 5.0f
#define REL_VEL_FRACTURE_THRESHOLD 0.1f

//extern std::vector<Compound*> delCompoundList;

SimScene* SimScene::createSimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath)
{
	SimScene* s = PX_NEW(SimScene)(pxPhysics,pxCooking,isGrbScene,minConvexSize,defaultMat,resourcePath);
	s->createSingletons();
	return s;
}

void SimScene::createSingletons()
{
	mCompoundCreator = PX_NEW(CompoundCreator)(this);
	mPolygonTriangulator = PX_NEW(PolygonTriangulator)(this);
	addActor(createActor());
}

base::Actor* SimScene::createActor()
{
	return (base::Actor*)PX_NEW(Actor)(this);
}

base::Convex* SimScene::createConvex()
{
	return (base::Convex*)PX_NEW(Convex)(this);
}

base::Compound* SimScene::createCompound(PxReal contactOffset, PxReal restOffset)
{
	return (base::Compound*)PX_NEW(Compound)(this, contactOffset, restOffset);
}

SimScene::SimScene(PxPhysics *pxPhysics, PxCooking *pxCooking, bool isGrbScene, float minConvexSize, PxMaterial* defaultMat, const char *resourcePath):
		base::SimScene(pxPhysics,pxCooking,isGrbScene,minConvexSize,defaultMat,resourcePath) 
{
	//mParticles = NULL;

	diffuseTexArray = 0;
	bumpTexArray = 0;
	specularTexArray = 0;
	emissiveReflectSpecPowerTexArray = 0;

	create3dTexture();
}

// --------------------------------------------------------------------------------------------
SimScene::~SimScene()
{
	mShader = NULL;
	mShaderMat.init();

	if (mRenderBuffers.volTex) glDeleteTextures(1, &mRenderBuffers.volTex);	
	if (mRenderBuffers.VBO) glDeleteBuffersARB(1, &mRenderBuffers.VBO);
	if (mRenderBuffers.IBO) glDeleteBuffersARB(1, &mRenderBuffers.IBO);
	if (mRenderBuffers.matTex) glDeleteTextures(1, &mRenderBuffers.matTex);
}

// --------------------------------------------------------------------------------------------
void SimScene::clear()
{
	base::SimScene::clear();
}

// --------------------------------------------------------------------------------------------
void SimScene::postSim(float dt, RegularCell3D* fluidSim)
{
	for (int i = 0; i < mActors.size(); i++)
	{
		mActors[i]->postSim(dt);
	}

	mFrameNr++;
	mNoFractureFrames--;
	mNoSoundFrames--;
}

void SimScene::profileBegin(const char* name)
{
}

void SimScene::profileEnd(const char* name)
{
}

// ------------------------- Renderering --------------------------------

GLuint SimScene::loadTextureArray(std::vector<std::string>& names) {

	GLuint texid;
	glGenTextures(1, &texid);
	glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, texid);

	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

	class BmpLoaderBuffer : public ::BmpLoaderBuffer, public ::physx::shdfnd::UserAllocated {};
	BmpLoaderBuffer *bmps;
	bmps = PX_NEW(BmpLoaderBuffer)[names.size()];

	char path[512];

	int w = 0, h = 0, d = 0;
	for (int i = 0; i < (int)names.size(); i++) {

		if (names[i].size() > 0) {
			sprintf_s(path, 512, "%s", names[i].c_str());
			int len = strlen(path);

			int mw = 0, mh = 0;
			if (strcmp(&path[len-4], ".jpg") == 0) {
				printf("Can't load %s, jpg no longer supported\n", path);
			} else {
				bmps[i].loadFile(path);
				mw = bmps[i].mWidth;
				mh = bmps[i].mHeight;
			}

			if ((mw != 0) && (mh != 0)) {
				if ( ((w != 0) && (w != mw)) ||
					((h != 0) && (h != mh)) ) {
					printf("Textures in array need to be same size!\n");
				}
				w = mw;
				h = mh;		
			}
		}
	}
	d = names.size();
	if (w == 0) {
		w=h=128;
		
	}
	unsigned char* tmpBlack = (unsigned char*)PX_ALLOC(sizeof(unsigned char)*w*h*3,PX_DEBUG_EXP("RT_FRACTURE"));
	memset(tmpBlack, 0, w*h*3);

	glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGB8, w, h, d, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < (int)names.size(); i++) {
		glTexSubImage3D( GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, i, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, (bmps[i].mRGB) ? bmps[i].mRGB :tmpBlack		
			);
		
	}

	
	glGenerateMipmapEXT(GL_TEXTURE_2D_ARRAY_EXT);
	
	PX_FREE(tmpBlack);
	PX_DELETE_ARRAY(bmps);
	
/*	names
	// 2D Texture arrays a loaded just like 3D textures
	glTexImage3D(GL_TEXTURE_2D_ARRAY_EXT, 0, GL_RGBA8, images[0].getWidth(), images[0].getHeight(), NIMAGES, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	for (int i = 0; i < NIMAGES; i++) {
		glTexSubImage3D( GL_TEXTURE_2D_ARRAY_EXT, 0, 0, 0, i, images[i].getWidth(), images[i].getHeight(), 1, images[i].getFormat(), images[i].getType(), images[i].getLevel(0));
	}
*/
	return texid;

}

void SimScene::loadAndCreateTextureArrays() {
	diffuseTexArray = loadTextureArray(diffuseTexNames);
	bumpTexArray = loadTextureArray(bumpTexNames);
	specularTexArray = loadTextureArray(specularTexNames);
	emissiveReflectSpecPowerTexArray = loadTextureArray(emissiveReflectSpecPowerTexNames);
	
}

// --------------------------------------------------------------------------------------------
#define WIDTH 128
#define HEIGHT 128
#define DEPTH 128
#define BYTES_PER_TEXEL 3
#define LAYER(r)	(WIDTH * HEIGHT * r * BYTES_PER_TEXEL)
// 2->1 dimension mapping function
#define TEXEL2(s, t)	(BYTES_PER_TEXEL * (s * WIDTH + t))
// 3->1 dimension mapping function
#define TEXEL3(s, t, r)	(TEXEL2(s, t) + LAYER(r))

// --------------------------------------------------------------------------------------------
void SimScene::create3dTexture()
{
	unsigned char *texels = (unsigned char *)malloc(WIDTH * HEIGHT * DEPTH * BYTES_PER_TEXEL);
	memset(texels, 0, WIDTH*HEIGHT*DEPTH);

	// each of the following loops defines one layer of our 3d texture, there are 3 unsigned bytes (red, green, blue) for each texel so each iteration sets 3 bytes
	// the memory pointed to by texels is technically a single dimension (C++ won't allow more than one dimension to be of variable length), the 
	// work around is to use a mapping function like the one above that maps the 3 coordinates onto one dimension
	// layer 0 occupies the first (width * height * bytes per texel) bytes, followed by layer 1, etc...
	// define layer 0 as red

//	Perlin perlin(8,4,1,91114);	
	unsigned char* tt = texels;
	float idx = 1.0f / DEPTH;
	for (int i = 0; i < DEPTH; i++) {
		for (int j = 0; j < HEIGHT; j++) {
			for (int k = 0; k < WIDTH; k++) {
				//float v[3] = {i*idx, j*idx, k*idx};
				//float val = (perlin.noise3(v)*0.5f + 0.5f)*255.0f;//cos( (perlin.Noise3(i*0.01f,j*0.01f,k*0.01f)*255000 + i*0.)*5.0f)*0.5f+0.5f;//perlin.Noise3(k,j,i)*255.0f;
					//(cos(perlin.Noise3(k,j,i) + j*0.01f) + 1.0f)*0.5f*255;
				unsigned char val = rand() % 255;
				tt[0] = val;
				tt[1] = val;
				tt[2] = val;
				tt+=3;
			}
		}
	}

	// request 1 texture name from OpenGL
	glGenTextures(1, &mRenderBuffers.volTex);	
	// tell OpenGL we're going to be setting up the texture name it gave us	
	glBindTexture(GL_TEXTURE_3D, mRenderBuffers.volTex);	
	// when this texture needs to be shrunk to fit on small polygons, use linear interpolation of the texels to determine the color
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// when this texture needs to be magnified to fit on a big polygon, use linear interpolation of the texels to determine the color
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// we want the texture to repeat over the S axis, so if we specify coordinates out of range we still get textured.
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	// same as above for T axis
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// same as above for R axis
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	// this is a 3d texture, level 0 (max detail), GL should store it in RGB8 format, its WIDTHxHEIGHTxDEPTH in size, 
	// it doesnt have a border, we're giving it to GL in RGB format as a series of unsigned bytes, and texels is where the texel data is.
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, WIDTH, HEIGHT, DEPTH, 0, GL_RGB, GL_UNSIGNED_BYTE, texels);

	glBindTexture(GL_TEXTURE_3D, 0);	
}

// --------------------------------------------------------------------------------------------
void SimScene::createRenderBuffers() 
{
	if (!mRenderBuffers.VBO) {
		glGenBuffersARB(1, &mRenderBuffers.VBO);
	}
	if (!mRenderBuffers.IBO) {
		glGenBuffersARB(1, &mRenderBuffers.IBO);
	}
	if (!mRenderBuffers.matTex) {
		glGenTextures(1, &mRenderBuffers.matTex);
		glBindTexture(GL_TEXTURE_2D, mRenderBuffers.matTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glBindTexture(GL_TEXTURE_2D, 0);

	}
	
	// Count number of vertices and indices in convexes
	// Count number of indices in convexes
	mRenderBuffers.numConvexes = 0;
	mRenderBuffers.numVertices = 0;
	mRenderBuffers.numIndices = 0;

	for (int k = 0; k < (int)mActors.size(); k++)
	{
		const shdfnd::Array<base::Compound*>& mCompounds = mActors[k]->getCompounds();
		for (int i = 0; i < (int)mCompounds.size(); i++) {
			if (((Compound*)mCompounds[i])->getShader() != mShader)
				continue;
			const shdfnd::Array<base::Convex*> &convexes = mCompounds[i]->getConvexes();
			mRenderBuffers.numConvexes += convexes.size();
			for (int j = 0; j < (int)convexes.size(); j++) {
				mRenderBuffers.numVertices += convexes[j]->getVisVertices().size();
				mRenderBuffers.numIndices += convexes[j]->getVisTriIndices().size();
			}
		}
	}

	if (mRenderBuffers.numVertices == 0 || mRenderBuffers.numIndices == 0)
		return;

	mRenderBuffers.tmpVertices.resize(mRenderBuffers.numVertices*12);
	mRenderBuffers.tmpIndices.resize(mRenderBuffers.numIndices);
	float* vp = &mRenderBuffers.tmpVertices[0];
	unsigned int* ip = &mRenderBuffers.tmpIndices[0];
	
	int sumV = 0;
	// Make cpu copy of VBO and IBO

	int convexNr = 0;

	for (int k = 0; k < (int)mActors.size(); k++)
	{
		const shdfnd::Array<base::Compound*>& mCompounds = mActors[k]->getCompounds();
		for (int i = 0; i < (int)mCompounds.size(); i++) {
			if (((Compound*)mCompounds[i])->getShader() != mShader)
				continue;

			const shdfnd::Array<base::Convex*> &convexes = mCompounds[i]->getConvexes();
			for (int j = 0; j < (int)convexes.size(); j++, convexNr++) {
				Convex* c = (Convex*)convexes[j];
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
		}
	}

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, mRenderBuffers.VBO);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, mRenderBuffers.tmpVertices.size()*sizeof(float), &mRenderBuffers.tmpVertices[0], GL_STATIC_DRAW);

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mRenderBuffers.IBO);
	glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mRenderBuffers.tmpIndices.size()*sizeof(unsigned int), &mRenderBuffers.tmpIndices[0], GL_STATIC_DRAW);

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	int oldTexSize = mRenderBuffers.texSize;
	if (mRenderBuffers.texSize == 0) {
		// First time
		oldTexSize = 1;
		mRenderBuffers.texSize = 32;
	}

	while (1) {
		int convexesPerRow = mRenderBuffers.texSize / 4;
		if (convexesPerRow * mRenderBuffers.texSize >= mRenderBuffers.numConvexes) {
			break;
		} else {
			mRenderBuffers.texSize *= 2;
		}
	}
	if (mRenderBuffers.texSize != oldTexSize) {
		mRenderBuffers.tmpTexCoords.resize(mRenderBuffers.texSize*mRenderBuffers.texSize*4);
		// Let's allocate texture
		glBindTexture(GL_TEXTURE_2D, mRenderBuffers.matTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, mRenderBuffers.texSize, mRenderBuffers.texSize, 0, GL_RGBA, GL_FLOAT, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	mRenderBufferVersion = mSceneVersion;

	//instanceVBOCPU.resize( newSize * 16);
}

// --------------------------------------------------------------------------------------------
void SimScene::updateConvexesTex() 
{
	float* tt = &mRenderBuffers.tmpTexCoords[0];

	for (int k = 0; k < (int)mActors.size(); k++)
	{
		const shdfnd::Array<base::Compound*>& mCompounds = mActors[k]->getCompounds();
		for (int i = 0; i < (int)mCompounds.size(); i++) {
			if ( ((Compound*)mCompounds[i])->getShader() != mShader)
				continue;

			const shdfnd::Array<base::Convex*> &convexes = mCompounds[i]->getConvexes();
			for (int j = 0; j < (int)convexes.size(); j++) {
				Convex* c = (Convex*)convexes[j];

				PxMat44 pose(convexes[j]->getGlobalPose());
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
				int idFor3DTex = 0;
				const int MAX_3D_TEX = 8;
				ta[15] = (float)(idFor2DTex*MAX_3D_TEX + idFor3DTex);
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, mRenderBuffers.matTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mRenderBuffers.texSize, mRenderBuffers.texSize, 
		GL_RGBA, GL_FLOAT, &mRenderBuffers.tmpTexCoords[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// --------------------------------------------------------------------------------------------
#ifdef USE_OPTIX

// Updater class for Optix renderer.
class FractureModelUpdate : public optix_demo::IModelUpdate
{
public:

	FractureModelUpdate( shdfnd::Array< Compound* > const& compounds, bool complete_update )
		: m_compounds(compounds), m_is_complete_update( complete_update )
	{}

	bool isCompleteUpdate() const {return m_is_complete_update;}
	void to_first();
	void to_next();
	bool at_end() const;

	size_t num_vertices() const {return (*m_iconvex)->getVisVertices().size();}
	size_t num_indices() const  {return (*m_iconvex)->getVisTriIndices().size();}
	float const* vertices() const {return &(*m_iconvex)->getVisVertices()[0].x;}
	float const* normals() const  {return &(*m_iconvex)->getVisNormals()[0].x;}
	int const* indices() const    {return &(*m_iconvex)->getVisTriIndices()[0];}

	void global_transform( float* rmat3x3, float* translation );

private:
	void advance();
	bool is_valid_convex( Convex const* c ) const
	{
		return !c->getVisVertices().empty();
	}

	shdfnd::Array< Compound* > const&		m_compounds;
	bool	m_is_complete_update;
	shdfnd::Array< Compound* >::ConstIterator	m_icompound;
	shdfnd::Array< Convex* >::ConstIterator		m_iconvex;
};

void FractureModelUpdate::to_first() 
{
	m_icompound = m_compounds.begin();
	if (m_icompound == m_compounds.end())  return;

	while ((*m_icompound)->getConvexes().empty()) {
		++m_icompound;
		if (m_icompound == m_compounds.end())  return;
	}
	m_iconvex = (*m_icompound)->getConvexes().begin();
	while (!is_valid_convex(*m_iconvex)) {
		advance();
		if (m_icompound == m_compounds.end())  return;
	}
}

void FractureModelUpdate::to_next()
{
	if (m_icompound != m_compounds.end()) {
		do {
			advance();
		} while (m_icompound != m_compounds.end() && !is_valid_convex(*m_iconvex));
	}
}

void FractureModelUpdate::advance()
{
	if (m_iconvex != (*m_icompound)->getConvexes().end()) {
		++m_iconvex;
	}
	if (m_iconvex == (*m_icompound)->getConvexes().end()) {
		// End of convexes of this compound.
		do {
			++m_icompound;
			if (m_icompound == m_compounds.end())  return;
		} while ((*m_icompound)->getConvexes().empty());
		m_iconvex = (*m_icompound)->getConvexes().begin();
	}
}

bool FractureModelUpdate::at_end() const
{
	return m_icompound == m_compounds.end();
}

void FractureModelUpdate::global_transform( float* rmat3x3, float* translation )
{
	PxTransform pose = (*m_iconvex)->getGlobalPose();
	PxMat33 rot(pose.q);
	rmat3x3[0] = rot.column0.x; rmat3x3[1] = rot.column1.x; rmat3x3[2] = rot.column2.x;
	rmat3x3[3] = rot.column0.y; rmat3x3[4] = rot.column1.y; rmat3x3[5] = rot.column2.y;
	rmat3x3[6] = rot.column0.z; rmat3x3[7] = rot.column1.z; rmat3x3[8] = rot.column2.z;
	translation[0] = pose.p.x; 
	translation[1] = pose.p.y; 
	translation[2] = pose.p.z; 
}

#endif //USE_OPTIX

extern bool gConvexTexOutdate;

// --------------------------------------------------------------------------------------------
void SimScene::draw(bool useShader)
{
	if (mDebugDraw) {
		for (int k = 0; k < (int)mActors.size(); k++)
		{
			const shdfnd::Array<base::Compound*>& mCompounds = mActors[k]->getCompounds();
			for (int i = 0; i < (int)mCompounds.size(); i++)
				mCompounds[i]->draw(false, true);
		}
		return;
	}

	int numCompounds = 0;
	for (int k = 0; k < (int)mActors.size(); k++)
	{
		const shdfnd::Array<base::Compound*>& mCompounds = mActors[k]->getCompounds();
		numCompounds += mCompounds.size();
	}
	if (numCompounds == 0) return;


	if (SampleViewerScene::getRenderType() == SampleViewerScene::rtOPENGL) {
#if USE_CONVEX_RENDERER
		mConvexRenderer.setShaderMaterial(mShader, mShaderMat);
		mConvexRenderer.setTexArrays(diffuseTexArray, bumpTexArray, 
			specularTexArray, emissiveReflectSpecPowerTexArray);
		mConvexRenderer.setVolTex(mRenderBuffers.volTex);
		mConvexRenderer.render();
#else
		mConvexRenderer.setActive(false);

		if (mRenderBufferVersion != mSceneVersion) 
			createRenderBuffers();

		if (mRenderBuffers.numIndices > 0) {

			if (gConvexTexOutdate) {
				updateConvexesTex();
				gConvexTexOutdate = false;
			}
			
			mShader->activate(mShaderMat);
			// Assume convex all use the same shader

			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_3D, mRenderBuffers.volTex);				

			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_2D, mRenderBuffers.matTex);				

	

			glActiveTexture(GL_TEXTURE10);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, diffuseTexArray);
			glActiveTexture(GL_TEXTURE11);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, bumpTexArray);
			glActiveTexture(GL_TEXTURE12);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, specularTexArray);
			glActiveTexture(GL_TEXTURE13);
			glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, emissiveReflectSpecPowerTexArray);

			glActiveTexture(GL_TEXTURE0);

			float itt = 1.0f/mRenderBuffers.texSize;

			mShader->setUniform("diffuseTexArray", 10);
			mShader->setUniform("bumpTexArray", 11);
			mShader->setUniform("specularTexArray", 12);
			mShader->setUniform("emissiveReflectSpecPowerTexArray", 13);

			mShader->setUniform("ttt3D", 7);
			mShader->setUniform("transTex", 8);
			mShader->setUniform("transTexSize", mRenderBuffers.texSize);
			mShader->setUniform("iTransTexSize", itt);
			mShader->setUniform("bumpTextureUVScale", bumpTextureUVScale);
			mShader->setUniform("extraNoiseScale",extraNoiseScale);
			mShader->setUniform("roughnessScale", roughnessScale);
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

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mRenderBuffers.VBO);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mRenderBuffers.IBO);

			int stride = 12*sizeof(float);
			glVertexPointer(3, GL_FLOAT, stride, 0);
			
			glNormalPointer(GL_FLOAT, stride, (void*)(3*sizeof(float)));
			
			glTexCoordPointer(4, GL_FLOAT, stride, (void*)(6*sizeof(float)));

			glClientActiveTexture(GL_TEXTURE1);
			glTexCoordPointer(2, GL_FLOAT, stride, (void*)(10*sizeof(float)));
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			
			glDrawElements(GL_TRIANGLES, mRenderBuffers.numIndices, GL_UNSIGNED_INT, 0);	
			
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
			
			mShader->deactivate();
		}
#endif
	}
#ifdef USE_OPTIX
	else if (SampleViewerScene::getRenderType() == SampleViewerScene::rtOPTIX) {

		FractureModelUpdate update( mCompounds, mOptixBufferVersion != mSceneVersion );

	    if (mOptixBufferVersion != mSceneVersion) {
		    // Complete buffer update.
			mOptixBufferVersion = mSceneVersion;
		}

		Profiler::getInstance()->begin("Optix data update");

		SampleViewerScene::getOptixRenderer()->update( update );

		OptixRenderer::Camera cam;
		const size_t size_of_float3 = sizeof(float) * 3;
		PxVec3 lookat = mCameraPos + mCameraDir;
		memcpy( cam.eye, &mCameraPos.x, size_of_float3);
		memcpy( cam.lookat, &lookat.x, size_of_float3);
		memcpy( cam.up, &mCameraUp.x, size_of_float3);
		cam.fov = mCameraFov;
		Profiler::getInstance()->end("Optix data update");

		Profiler::getInstance()->begin("Optix render");
		SampleViewerScene::getOptixRenderer()->render( cam );
		Profiler::getInstance()->end("Optix render");
	}
#endif // USE_OPTIX

}


// --------------------------------------------------------------------------------------------
void SimScene::dumpSceneGeometry()
{
#ifdef USE_OPTIX
	if( SampleViewerScene::getRenderType() == SampleViewerScene::rtOPTIX && SampleViewerScene::getOptixRenderer()) {
		SampleViewerScene::getOptixRenderer()->dump_geometry();
	}
#endif
}
