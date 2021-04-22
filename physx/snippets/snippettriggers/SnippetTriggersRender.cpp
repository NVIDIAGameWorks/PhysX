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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
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
// Copyright (c) 2008-2021 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#ifdef RENDER_SNIPPET

#include <vector>

#include "PxPhysicsAPI.h"
#include "../snippetrender/SnippetRender.h"
#include "../snippetrender/SnippetCamera.h"

using namespace physx;

extern void initPhysics(bool interactive);
extern void stepPhysics(bool interactive);	
extern void cleanupPhysics(bool interactive);
extern void keyPress(unsigned char key, const PxTransform& camera);
bool isTrigger(const PxFilterData& data);
bool isTriggerShape(PxShape* shape);

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

	class MyTriggerRender : public Snippets::TriggerRender
	{
		public:
		virtual	bool	isTrigger(physx::PxShape* shape)	const
		{
			return ::isTriggerShape(shape);
		}
	};

void renderCallback()
{
	stepPhysics(true);

	if(0)
	{
		PxVec3 camPos = sCamera->getEye();
		PxVec3 camDir = sCamera->getDir();
		printf("camPos: (%ff, %ff, %ff)\n", double(camPos.x), double(camPos.y), double(camPos.z));
		printf("camDir: (%ff, %ff, %ff)\n", double(camDir.x), double(camDir.y), double(camDir.z));
	}

	Snippets::startRender(sCamera->getEye(), sCamera->getDir());
	InitLighting();

	PxScene* scene;
	PxGetPhysics().getScenes(&scene,1);
	PxU32 nbActors = scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
	if(nbActors)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);

		MyTriggerRender tr;
		Snippets::renderActors(&actors[0], static_cast<PxU32>(actors.size()), true, PxVec3(0.0f, 0.75f, 0.0f), &tr);
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
	sCamera = new Snippets::Camera(PxVec3(8.757190f, 12.367847f, 23.541956f), PxVec3(-0.407947f, -0.042438f, -0.912019f));

	Snippets::setupDefaultWindow("PhysX Snippet Triggers");
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
