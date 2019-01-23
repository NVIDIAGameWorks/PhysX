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
#include "SampleHelloWorld.h"
#include "SampleUtils.h"
#include "SampleConsole.h"
#include "RendererMemoryMacros.h"
#include "RenderMeshActor.h"

#include "PxPhysics.h"
#include "PxScene.h"
#include "PxRigidDynamic.h"
#include "PxShape.h"
#include "PxPhysicsAPI.h"
#include "RenderBoxActor.h"

#include <SampleBaseInputEventIds.h>
#include <SamplePlatform.h>
#include <SampleUserInput.h>
#include <SampleUserInputIds.h>
#include <SampleUserInputDefines.h>

using namespace SampleRenderer;
using namespace SampleFramework;

REGISTER_SAMPLE(SampleHelloWorld, "SampleHelloWorld")

///////////////////////////////////////////////////////////////////////////////

SampleHelloWorld::SampleHelloWorld(PhysXSampleApplication& app) :
	PhysXSample(app)
{
}

SampleHelloWorld::~SampleHelloWorld()
{
}

void SampleHelloWorld::onTickPreRender(float dtime)
{
	PhysXSample::onTickPreRender(dtime);
}

void SampleHelloWorld::onTickPostRender(float dtime)
{
	PhysXSample::onTickPostRender(dtime);
}

void SampleHelloWorld::customizeSceneDesc(PxSceneDesc& sceneDesc)
{
	sceneDesc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
}

void SampleHelloWorld::newMesh(const RAWMesh& data)
{
}

static void gValue(Console* console, const char* text, void* userData)
{
	if(!text)
	{
		console->out("Usage: value <float>");
		return;
	}

	const float val = (float)::atof(text);
	shdfnd::printFormatted("value: %f\n", val);
}

static void gExport(Console* console, const char* text, void* userData)
{
	if(!text)
	{
		console->out("Usage: export <filename>");
		return;
	}
}

static void gImport(Console* console, const char* text, void* userData)
{
	if(!text)
	{
		console->out("Usage: import <filename>");
		return;
	}
}

void SampleHelloWorld::onInit()
{
	if(getConsole())
	{
		getConsole()->addCmd("value", gValue);
		getConsole()->addCmd("export", gExport);
		getConsole()->addCmd("import", gImport);
	}

	PhysXSample::onInit();

	mApplication.setMouseCursorHiding(true);
	mApplication.setMouseCursorRecentering(true);
	mCameraController.init(PxVec3(0.0f, 10.0f, 0.0f), PxVec3(0.0f, 0.0f, 0.0f));
	mCameraController.setMouseSensitivity(0.5f);
}

void SampleHelloWorld::collectInputEvents(std::vector<const SampleFramework::InputEvent*>& inputEvents)
{
	PhysXSample::collectInputEvents(inputEvents);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_INCREASE);
	getApplication().getPlatform()->getSampleUserInput()->unregisterInputEvent(CAMERA_SPEED_DECREASE);
}

void SampleHelloWorld::helpRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	Renderer* renderer = getRenderer();
	const PxU32 yInc = 18;
	const PxReal scale = 0.5f;
	const PxReal shadowOffset = 6.0f;
	const RendererColor textColor(255, 255, 255, textAlpha);
	const bool isMouseSupported = getApplication().getPlatform()->getSampleUserInput()->mouseSupported();
	const bool isPadSupported = getApplication().getPlatform()->getSampleUserInput()->gamepadSupported();
	const char* msg;

	if (isMouseSupported && isPadSupported)
		renderer->print(x, y += yInc, "Use mouse or right stick to rotate", scale, shadowOffset, textColor);
	else if (isMouseSupported)
		renderer->print(x, y += yInc, "Use mouse to rotate", scale, shadowOffset, textColor);
	else if (isPadSupported)
		renderer->print(x, y += yInc, "Use right stick to rotate", scale, shadowOffset, textColor);
	if (isPadSupported)
		renderer->print(x, y += yInc, "Use left stick to move",scale, shadowOffset, textColor);
	msg = mApplication.inputMoveInfoMsg("Press "," to move", CAMERA_MOVE_FORWARD,CAMERA_MOVE_BACKWARD, CAMERA_MOVE_LEFT, CAMERA_MOVE_RIGHT);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to move fast", CAMERA_SHIFT_SPEED, -1);
	if(msg)
		renderer->print(x, y += yInc, msg, scale, shadowOffset, textColor);
	msg = mApplication.inputInfoMsg("Press "," to throw an object", SPAWN_DEBUG_OBJECT, -1);
	if(msg)
		renderer->print(x, y += yInc, msg,scale, shadowOffset, textColor);
}

void SampleHelloWorld::descriptionRender(PxU32 x, PxU32 y, PxU8 textAlpha)
{
	bool print=(textAlpha!=0.0f);

	if(print)
	{
		Renderer* renderer = getRenderer();
		const PxU32 yInc = 18;
		const PxReal scale = 0.5f;
		const PxReal shadowOffset = 6.0f;
		const RendererColor textColor(255, 255, 255, textAlpha);

		char line1[256]="This sample demonstrates how to set up and simulate a PhysX";
		char line2[256]="scene.  Further, it illustrates the creation, simulation and";
		char line3[256]="collision of simple dynamic objects.";
		renderer->print(x, y+=yInc, line1, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line2, scale, shadowOffset, textColor);
		renderer->print(x, y+=yInc, line3, scale, shadowOffset, textColor);
	}
}

PxU32 SampleHelloWorld::getDebugObjectTypes() const
{
	return DEBUG_OBJECT_BOX | DEBUG_OBJECT_SPHERE | DEBUG_OBJECT_CAPSULE | DEBUG_OBJECT_CONVEX;
}
