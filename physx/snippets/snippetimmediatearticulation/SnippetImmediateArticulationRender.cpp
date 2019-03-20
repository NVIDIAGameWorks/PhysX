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

#ifdef RENDER_SNIPPET

#include <vector>

#include "PxPhysicsAPI.h"
#include "PxImmediateMode.h"

#include "../snippetrender/SnippetRender.h"
#include "../snippetrender/SnippetCamera.h"

using namespace physx;

extern void initPhysics(bool interactive);
extern void stepPhysics(bool interactive);	
extern void cleanupPhysics(bool interactive);
extern void keyPress(unsigned char key, const PxTransform& camera);
extern PxU32 getNbGeoms();
extern const PxGeometryHolder* getGeoms();
extern const PxTransform* getGeomPoses();
extern PxU32 getNbContacts();
extern const Gu::ContactPoint* getContacts();
extern PxU32 getNbArticulations();
extern Dy::ArticulationV** getArticulations();
extern PxU32 getNbBounds();
extern const PxBounds3* getBounds();

namespace
{
Snippets::Camera*	sCamera;

void motionCallback(int x, int y)
{
	sCamera->handleMotion(x, y);
}

void keyboardCallback(unsigned char key, int x, int y)
{
	if(key==27)
		exit(0);

	if(!sCamera->handleKey(key, x, y))
		keyPress(key, sCamera->getTransform());
}

void keyboardCallback2(int key, int /*x*/, int /*y*/)
{
	keyPress(key, sCamera->getTransform());
}

void mouseCallback(int button, int state, int x, int y)
{
	sCamera->handleMouse(button, state, x, y);
}

void idleCallback()
{
	glutPostRedisplay();
}

static void DrawLine(const PxVec3& p0, const PxVec3& p1, const PxVec3& color)
{
	glDisable(GL_LIGHTING);
	glColor4f(color.x, color.y, color.z, 1.0f);
	const PxVec3 Pts[] = { p0, p1 };
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(PxVec3), &Pts[0].x);
	glDrawArrays(GL_LINES, 0, 2);
	glDisableClientState(GL_VERTEX_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_LIGHTING);
}

static void DrawFrame(const PxVec3& pt, float scale=1.0f)
{
	DrawLine(pt, pt + PxVec3(scale, 0.0f, 0.0f), PxVec3(1.0f, 0.0f, 0.0f));
	DrawLine(pt, pt + PxVec3(0.0f, scale, 0.0f), PxVec3(0.0f, 1.0f, 0.0f));
	DrawLine(pt, pt + PxVec3(0.0f, 0.0f, scale), PxVec3(0.0f, 0.0f, 1.0f));
}

static void DrawBounds(const PxBounds3& box)
{
	DrawLine(PxVec3(box.minimum.x, box.minimum.y, box.minimum.z), PxVec3(box.maximum.x, box.minimum.y, box.minimum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.minimum.y, box.minimum.z), PxVec3(box.maximum.x, box.maximum.y, box.minimum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.maximum.y, box.minimum.z), PxVec3(box.minimum.x, box.maximum.y, box.minimum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.maximum.y, box.minimum.z), PxVec3(box.minimum.x, box.minimum.y, box.minimum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.minimum.y, box.minimum.z), PxVec3(box.minimum.x, box.minimum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.minimum.y, box.maximum.z), PxVec3(box.maximum.x, box.minimum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.minimum.y, box.maximum.z), PxVec3(box.maximum.x, box.maximum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.maximum.y, box.maximum.z), PxVec3(box.minimum.x, box.maximum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.maximum.y, box.maximum.z), PxVec3(box.minimum.x, box.minimum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.minimum.y, box.maximum.z), PxVec3(box.minimum.x, box.minimum.y, box.minimum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.minimum.y, box.minimum.z), PxVec3(box.maximum.x, box.minimum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.maximum.x, box.maximum.y, box.minimum.z), PxVec3(box.maximum.x, box.maximum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
	DrawLine(PxVec3(box.minimum.x, box.maximum.y, box.minimum.z), PxVec3(box.minimum.x, box.maximum.y, box.maximum.z), PxVec3(1.0f, 1.0f, 0.0f));
}

static void InitLighting()
{
	glEnable(GL_COLOR_MATERIAL);

	const float zero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, zero);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zero);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);

	glEnable(GL_LIGHTING);
	PxVec3 Dir(-1.0f, 1.0f, 0.5f);
//	PxVec3 Dir(0.0f, 1.0f, 0.0f);
	Dir.normalize();

	const float AmbientValue = 0.3f;
	const float ambientColor0[]		= { AmbientValue, AmbientValue, AmbientValue, 0.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientColor0);

	const float specularColor0[]	= { 0.0f, 0.0f, 0.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularColor0);

	const float diffuseColor0[]	= { 0.7f, 0.7f, 0.7f, 0.0f };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseColor0);

	const float position0[]		= { Dir.x, Dir.y, Dir.z, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, position0);

	glEnable(GL_LIGHT0);

//	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//	glColor4f(0.0f, 0.0f, 0.0f, 0.0f);

	if(0)
	{
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE,GL_LINEAR); 
		//glFogi(GL_FOG_MODE,GL_EXP); 
		//glFogi(GL_FOG_MODE,GL_EXP2); 
		glFogf(GL_FOG_START, 0.0f);
		glFogf(GL_FOG_END, 100.0f);
		glFogf(GL_FOG_DENSITY, 0.005f);
//		glClearColor(0.2f, 0.2f, 0.2f, 1.0);
//		const PxVec3 FogColor(0.2f, 0.2f, 0.2f);
		const PxVec3 FogColor(1.0f);
		glFogfv(GL_FOG_COLOR, &FogColor.x);
	}
}

void renderCallback()
{
	stepPhysics(true);

/*	if(0)
	{
		PxVec3 camPos = sCamera->getEye();
		PxVec3 camDir = sCamera->getDir();
		printf("camPos: (%ff, %ff, %ff)\n", camPos.x, camPos.y, camPos.z);
		printf("camDir: (%ff, %ff, %ff)\n", camDir.x, camDir.y, camDir.z);
	}*/

	Snippets::startRender(sCamera->getEye(), sCamera->getDir());
	InitLighting();

	const PxVec3 color(0.6f, 0.8f, 1.0f);
//	const PxVec3 color(0.75f, 0.75f, 1.0f);
//	const PxVec3 color(1.0f);

	Snippets::renderGeoms(getNbGeoms(), getGeoms(), getGeomPoses(), true, color);

/*	PxU32 getNbGeoms();
	const PxGeometry* getGeoms();
	const PxTransform* getGeomPoses();
	Snippets::renderGeoms(getNbGeoms(), getGeoms(), getGeomPoses(), true, PxVec3(1.0f));*/

/*	PxBoxGeometry boxGeoms[10];
	for(PxU32 i=0;i<10;i++)
		boxGeoms[i].halfExtents = PxVec3(1.0f);

	PxTransform poses[10];
	for(PxU32 i=0;i<10;i++)
	{
		poses[i] = PxTransform(PxIdentity);
		poses[i].p.y += 1.5f;
		poses[i].p.x = float(i)*2.5f;
	}

	Snippets::renderGeoms(10, boxGeoms, poses, true, PxVec3(1.0f));*/

	if(1)
	{				
		const PxU32 nbContacts = getNbContacts();
		const Gu::ContactPoint* contacts = getContacts();
		for(PxU32 j=0;j<nbContacts;j++)
		{
			DrawFrame(contacts[j].point, 1.0f);
		}
	}

	if(0)
	{
		const PxU32 nbArticulations = getNbArticulations();
		Dy::ArticulationV** articulations = getArticulations();
		for(PxU32 j=0;j<nbArticulations;j++)
		{
			immediate::PxLinkData data[64];
			const PxU32 nbLinks = immediate::PxGetAllLinkData(articulations[j], data);
			for(PxU32 i=0;i<nbLinks;i++)
			{
				DrawFrame(data[i].pose.p, 1.0f);
			}
		}
	}

	const PxBounds3* bounds = getBounds();
	const PxU32 nbBounds = getNbBounds();
	for(PxU32 i=0;i<nbBounds;i++)
	{
		DrawBounds(bounds[i]);
	}

	Snippets::finishRender();
}

void exitCallback(void)
{
	delete sCamera;
	cleanupPhysics(true);
}
}

void renderLoop()
{
	sCamera = new Snippets::Camera(	PxVec3(8.526230f, 5.546278f, 5.448466f),
									PxVec3(-0.784231f, -0.210605f, -0.583632f));

	Snippets::setupDefaultWindow("PhysX Snippet Immediate Articulation");
	Snippets::setupDefaultRenderState();

	glutIdleFunc(idleCallback);
	glutDisplayFunc(renderCallback);
	glutKeyboardFunc(keyboardCallback);
	glutSpecialFunc(keyboardCallback2);
	glutMouseFunc(mouseCallback);
	glutMotionFunc(motionCallback);
	motionCallback(0,0);

	atexit(exitCallback);

	initPhysics(true);
	glutMainLoop();
}

#endif
