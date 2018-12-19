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

#ifndef COMPOUND
#define COMPOUND

#include <foundation/PxVec3.h>
#include <foundation/PxPlane.h>
#include <foundation/PxBounds3.h>
#include <foundation/PxTransform.h>
#include <PsArray.h>
#include <PxRigidDynamic.h>

#include "Shader.h"

#include "CompoundBase.h"

using namespace physx;

class Convex;
class Particles;
class Mesh;
class SimScene;
class CompoundGeometry;
class XMLParser;

class ShaderShadow;

using namespace physx::fracture;

class Compound : public base::Compound
{
	friend class SimScene;
protected:
	Compound(SimScene* scene, PxReal contactOffset = 0.005f, PxReal restOffset = -0.001f):
		physx::fracture::base::Compound((base::SimScene*)scene,contactOffset,restOffset) {}
public:

	virtual void convexAdded(base::Convex* c, Shader* shader);
	virtual void convexRemoved(base::Convex* c);

	bool createFromXml(XMLParser *p, float scale, const PxTransform &trans, bool ignoreVisualMesh = false);

	void setShader(Shader* shader, const ShaderMaterial &mat) { mShader = shader; mShaderMat = mat; }
	Shader* getShader() const { return mShader; }
	const ShaderMaterial& getShaderMat() { return mShaderMat; }

	virtual void draw(bool useShader, bool debug = false);

	virtual void clear();

	virtual void copyShaders(base::Compound*);

protected:

	virtual float getSleepingThresholdRB();

	Shader *mShader;
	ShaderMaterial mShaderMat;
};

#endif
