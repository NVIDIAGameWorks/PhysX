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


#include "Mesh.h"
#include "CompoundGeometry.h"
#include "CompoundCreator.h"
#include "PxConvexMeshGeometry.h"
#include "PxRigidBodyExt.h"
#include "foundation/PxMat44.h"
#include "PxScene.h"
#include "PxShape.h"

#include "SimScene.h"
#include "Compound.h"
#include "Convex.h"
#include "foundation/PxMathUtils.h"

#include <vector>

#include <stdio.h>

#include "MathUtils.h"

using namespace std;

#define CREATE_DEBRIS 1

float Compound::getSleepingThresholdRB()
{
	return 0.1f;
}

// --------------------------------------------------------------------------------------------
void Compound::draw(bool useShader, bool debug)
{
	if (useShader && mShader)
		mShader->activate(mShaderMat);
	else {
		glColor3f(mShaderMat.color[0], mShaderMat.color[1], mShaderMat.color[2]);
	}

	for (int i = 0; i < (int)mConvexes.size(); i++)
		mConvexes[i]->draw(debug);

	if (useShader && mShader)
		mShader->deactivate();
}

// --------------------------------------------------------------------------------------------
void Compound::clear()
{
	base::Compound::clear();

	mShader = NULL;
	mShaderMat.init();
}

void Compound::copyShaders(base::Compound* m)
{
	((Compound*)m)->mShader = mShader;
	((Compound*)m)->mShaderMat = mShaderMat;	
}

void Compound::convexAdded(base::Convex* c, Shader* shader)
{
	((SimScene*)mScene)->getConvexRenderer().add(c, shader);
}

void Compound::convexRemoved(base::Convex* c)
{
	((SimScene*)mScene)->getConvexRenderer().remove(c);
}