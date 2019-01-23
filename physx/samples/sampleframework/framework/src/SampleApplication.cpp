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
#include <PsUtilities.h>
#include <SampleApplication.h>
#include <SampleCommandLine.h>
#include <SampleAssetManager.h>
#include <Renderer.h>
#include <RendererMemoryMacros.h>
#include <SamplePlatform.h>
#include "SampleFrameworkInputEventIds.h"

#include "PsString.h"
#include "PsFoundation.h"
#include "PsUtilities.h"

namespace Ps = physx::shdfnd;

#define MEDIA_PATH "samples/sampleframework/media/" 

//#define RENDERER_USE_OPENGL_ON_WINDONWS	1

#if defined(SMOOTH_CAM)
const float g_smoothCamBaseVel  = 6.0f;
const float g_smoothCamFastMul = 4.0f;
const float g_smoothCamPosLerp  = 0.4f;

const float g_smoothCamRotSpeed = 0.005f;
const float g_smoothCamRotLerp  = 0.4f;
#endif

#include "FrameworkFoundation.h"

using namespace SampleFramework;

static PxMat44 EulerToMat33(const PxVec3& e)
{
	const float c1 = cosf(e.z);
	const float s1 = sinf(e.z);
	const float c2 = cosf(e.y);
	const float s2 = sinf(e.y);
	const float c3 = cosf(e.x);
	const float s3 = sinf(e.x);
	PxMat44 m;
	m.column0 = PxVec4(c1*c2,              -s1*c2,             s2,	0.0f);
	m.column1 = PxVec4((s1*c3)+(c1*s2*s3), (c1*c3)-(s1*s2*s3),-c2*s3, 0.0f);
	m.column2 = PxVec4((s1*s3)-(c1*s2*c3), (c1*s3)+(s1*s2*c3), c2*c3, 0.0f);
	m.column3 = PxVec4(0,0,0,1);
	return m;
}

static PxVec3 Mat33ToEuler(const PxMat44& m)
{
	const PxF32 epsilon = 0.99999f;
	PxVec3 e, x, y, z;

	x = PxVec3(m.column0.x, m.column1.x, m.column2.x);
	y = PxVec3(m.column0.y, m.column1.y, m.column2.y);
	z = PxVec3(m.column0.z, m.column1.z, m.column2.z);

	if(x.z > epsilon)
	{
		e.x = PxAtan2(z.y, y.y);
		e.y = PxPi * 0.5f;
		e.z = 0;
	}
	else if(x.z < -epsilon)
	{
		e.x = PxAtan2(z.y, y.y);
		e.y = -PxPi * 0.5f;
		e.z = 0;
	}
	else
	{
		e.x = PxAtan2(-y.z, z.z);
		e.y = PxAsin(x.z);
		e.z = PxAtan2(-x.y, x.x);
	}
	return e;
}

SampleApplication::SampleApplication(const SampleCommandLine &cmdline) :
  m_cmdline(cmdline)
, m_disableRendering(false)
,	m_rotationSpeedScale(200.0f)
,	m_moveSpeedScale(40.0f)
,	m_rightStickRotate(false)
,	m_rewriteBuffers(false)
{
	// Don't set the working directory to the exe's working directory. We want to be able to set the working directory to have the media somewhere else.
  //	m_platform->setCWDToEXE();
	m_renderer = 0;
	m_sceneSize = 1.0f;
	m_assetManager = 0;
	m_timeCounter	= 0;
	m_camMoveButton = -1;
}

SampleApplication::~SampleApplication(void)
{
	RENDERER_ASSERT(!m_renderer, "Renderer was not released prior to window closure.");
	RENDERER_ASSERT(!m_assetManager, "Asset Manager was not released prior to window closure.");

	DELETESINGLE(m_platform);

	clearSearchPaths();
}

void SampleApplication::setEyeTransform(const PxMat44& eyeTransform)
{
	m_worldToView.setInverseTransform(eyeTransform);
	m_eyeRot = Mat33ToEuler(eyeTransform);
#if defined(SMOOTH_CAM)
	m_targetEyePos = m_worldToView.getInverseTransform().getPosition();
	m_targetEyeRot = m_eyeRot;
#endif
}

void SampleApplication::setEyeTransform(const PxVec3& pos, const PxVec3& rot)
{
	PxMat44 eye;
	m_eyeRot = rot;
	eye = EulerToMat33(m_eyeRot);
	eye.setPosition(pos);
#if defined(SMOOTH_CAM)
	m_targetEyePos = pos;
	m_targetEyeRot = m_eyeRot;
#endif

	m_worldToView.setInverseTransform(eye);
}

void SampleApplication::setViewTransform(const PxMat44 &viewTransform)
{
	m_worldToView.setForwardTransform(viewTransform);
	m_eyeRot = Mat33ToEuler( m_worldToView.getInverseTransform() );
#if defined(SMOOTH_CAM)
	m_targetEyePos = m_worldToView.getInverseTransform().getPosition();
	m_targetEyeRot = m_eyeRot;
#endif
}

const PxMat44& SampleApplication::getViewTransform() const
{
	return m_worldToView.getForwardTransform();
}

void SampleApplication::onOpen(void)
{
	m_platform->preRendererSetup();

	m_eyeRot = PxVec3(0,0,0);
	PxMat44 eye = PxMat44(PxIdentity);
	const PxVec3 pos = PxVec3(0.0f, 2.0f, 16.0f);
	eye.setPosition(pos);
	m_worldToView.setInverseTransform(eye);
#if defined(SMOOTH_CAM)
	m_targetEyePos = pos;
	m_targetEyeRot = m_eyeRot;
#endif

	// default renderer drivers for various platforms...
	SampleRenderer::RendererDesc renDesc;
	setupRendererDescription(renDesc);
#if defined RENDERER_USE_OPENGL_ON_WINDONWS
	renDesc.driver = SampleRenderer::Renderer::DRIVER_OPENGL;
#endif

	// check to see if the user wants to override the renderer driver...
	if(m_cmdline.hasSwitch("ogl"))        renDesc.driver = SampleRenderer::Renderer::DRIVER_OPENGL;
	else if(m_cmdline.hasSwitch("d3d9"))  renDesc.driver = SampleRenderer::Renderer::DRIVER_DIRECT3D9;
	else if(m_cmdline.hasSwitch("d3d11")) renDesc.driver = SampleRenderer::Renderer::DRIVER_DIRECT3D11;
	else if(m_cmdline.hasSwitch("null"))   renDesc.driver = SampleRenderer::Renderer::DRIVER_NULL;

  char assetPath[512];
  if (!searchForPath(MEDIA_PATH, assetPath, PX_ARRAY_SIZE(assetPath), true, 20)) {
    RENDERER_ASSERT(false, MEDIA_PATH " could not be found in any of the parent directories!");
    exit(1);
  }
  addSearchPath(assetPath);

	m_renderer = SampleRenderer::Renderer::createRenderer(renDesc, assetPath);
	m_platform->postRendererSetup(m_renderer);

	m_timeCounter = m_time.getCurrentCounterValue();

	m_assetManager = new SampleAssetManager(*m_renderer);
	
	onInit();

	// make sure the resize method is called once
	if (m_renderer)
	{
		PxU32 width,height;
		m_renderer->getWindowSize(width, height);
		onResize(width, height);
	}
}

bool SampleApplication::onClose(void)
{
	onShutdown();
	DELETESINGLE(m_assetManager);

	SAFE_RELEASE(m_renderer);
	m_platform->postRendererRelease();

	return true;
}

inline float SmoothStepPolynomial( float s )
{
	if( s <= 0 ) return 0;
	if( s >= 1 ) return 1;
	return s*s*(3-2*s);
}

template <typename T>
T SmoothStep( const T& start, const T& end, float s )
{
	float ss = SmoothStepPolynomial( s );
	return ss * (end - start) + start;
}

void SampleApplication::onDraw(void)
{
	
	if (!getRenderer())
	{
		return;
	}

	PX_PROFILE_ZONE("OnDraw", 0);

	physx::PxU64 qpc = m_time.getCurrentCounterValue();
	static float sToSeconds = float(m_time.getBootCounterFrequency().mNumerator) / float(m_time.getBootCounterFrequency().mDenominator * m_time.sNumTensOfNanoSecondsInASecond);
	float dtime = float(qpc - m_timeCounter) * sToSeconds;
	m_lastDTime = dtime;
	m_timeCounter = qpc;
	PX_ASSERT(dtime > 0);
	if(dtime > 0)
	{
		dtime = tweakElapsedTime(dtime);

		{
			PX_PROFILE_ZONE("PreRender", 0);
			onTickPreRender(dtime);
		}
		if(m_renderer)
		{
			PX_PROFILE_ZONE("onRender", 0);
			if(!m_disableRendering)
			{
				onRender();
			}
		}
		{
			PX_PROFILE_ZONE("onPostRender", 0);
			onTickPostRender(dtime);
		}

		if(m_renderer && !m_disableRendering)
		{
			m_rewriteBuffers = m_renderer->swapBuffers();
		}

		// update scene...

		PxMat44 tmp = m_worldToView.getInverseTransform();
		PxMat44 eye = EulerToMat33(m_eyeRot);
		eye.column3 = tmp.column3;

		PxVec3* targetParam;
#if defined(SMOOTH_CAM)
		const float eyeSpeed = m_sceneSize * g_smoothCamBaseVel * dtime * (getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_SHIFT_SPEED) ? g_smoothCamFastMul : 1.0f);
		targetParam = &m_targetEyePos;
#else
		const float eyeSpeed = m_sceneSize * 4.0f * dtime * (getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_SHIFT_SPEED) ? 4.0f : 1.0f);
		targetParam = &eye.t;
#endif

		const PxVec3 column2 = eye.getBasis(2);
		const PxVec3 column0 = eye.getBasis(0);
		const PxVec3 column1 = eye.getBasis(1);

		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_FORWARD))
			*targetParam -= column2 * eyeSpeed;
		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_LEFT))
			*targetParam -= column0 * eyeSpeed;
		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_BACKWARD))
			*targetParam += column2 * eyeSpeed;
		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_RIGHT))
			*targetParam += column0 * eyeSpeed;
		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_UP))
			*targetParam += column1 * eyeSpeed;
		if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_DOWN))
			*targetParam -= column1 * eyeSpeed;

		// move forward from gamepad
		*targetParam -= column2 * eyeSpeed *  getPlatform()->getSampleUserInput()->getAnalogInputEventState(CAMERA_GAMEPAD_MOVE_FORWARD_BACK) * m_moveSpeedScale * dtime;
		// strafe from gamepad
		*targetParam += column0 * eyeSpeed * getPlatform()->getSampleUserInput()->getAnalogInputEventState(CAMERA_GAMEPAD_MOVE_LEFT_RIGHT) * m_moveSpeedScale* dtime;

#if defined(SMOOTH_CAM)
		PxVec3 eye_t = eye.getPosition();
		eye_t = eye_t + (m_targetEyePos - eye_t) * g_smoothCamPosLerp;
		eye.setPosition(eye_t);
#endif

		// rotate from gamepad
		{
			const PxF32 rotationSpeed = m_rotationSpeedScale * dtime;
			PxF32 dx = getPlatform()->getSampleUserInput()->getAnalogInputEventState(CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT) * rotationSpeed;
			PxF32 dy = getPlatform()->getSampleUserInput()->getAnalogInputEventState(CAMERA_GAMEPAD_ROTATE_UP_DOWN) * rotationSpeed;
			rotateCamera(dx, dy);
		}

		m_worldToView.setInverseTransform(eye);
	}
}

// When holding down the mouse button and dragging across the edge of the window,
//   the input provider will spit out nonsensical delta values... so threshold appropriately
void thresholdCameraRotate(physx::PxReal& dx, physx::PxReal& dy) 
{
	static const physx::PxReal invalidDelta = 1000.f;
	if ( physx::PxAbs(dx) > invalidDelta )
		dx = physx::PxSign(dx);
	if ( physx::PxAbs(dy) > invalidDelta )
		dy = physx::PxSign(dy);
}

void SampleApplication::onPointerInputEvent(const InputEvent& ie, physx::PxU32 x, physx::PxU32 y, physx::PxReal dx, physx::PxReal dy, bool val)
{
	switch (ie.m_Id)
	{
	case CAMERA_MOUSE_LOOK:
		{	
			if(getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_MOVE_BUTTON))
			{
				thresholdCameraRotate(dx, dy);
				rotateCamera(dx, dy);
			}
		}
		break;
	}
}

void SampleApplication::onDigitalInputEvent(const InputEvent& , bool val)
{
}

void SampleApplication::moveCamera(PxF32 dx, PxF32 dy)
{
	PxMat44 tmp = m_worldToView.getInverseTransform();
	PxMat44 eye = EulerToMat33(m_eyeRot);
	eye.column3 = tmp.column3;

	PxVec3* targetParam;
#if defined(SMOOTH_CAM)
	const float eyeSpeed = m_sceneSize * g_smoothCamBaseVel * m_lastDTime * (getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_SHIFT_SPEED) ? g_smoothCamFastMul : 1.0f);
	targetParam = &m_targetEyePos;
#else
	const float eyeSpeed = m_sceneSize * 4.0f * m_lastDTime * (getPlatform()->getSampleUserInput()->getDigitalInputEventState(CAMERA_SHIFT_SPEED) ? 4.0f : 1.0f);
	targetParam = &eye.t;
#endif

	const PxVec3 column2 = eye.getBasis(2);
	const PxVec3 column0 = eye.getBasis(0);

	// strafe from gamepad
	*targetParam += column0 * eyeSpeed * dx * m_moveSpeedScale* m_lastDTime;
	// move forward from gamepad
	*targetParam -= column2 * eyeSpeed *  dy * m_moveSpeedScale * m_lastDTime;

#if defined(SMOOTH_CAM)
	PxVec3 eye_t = eye.getPosition();
	eye_t = eye_t + (m_targetEyePos - eye_t) * g_smoothCamPosLerp;
	eye.setPosition(eye_t);
#endif

	m_worldToView.setInverseTransform(eye);	
}

void SampleApplication::onAnalogInputEvent(const InputEvent& ie, float val)
{
	switch (ie.m_Id)
	{
	case CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT:
		{	
			rotateCamera((PxF32)val, (PxF32)0.0);
		}
		break;
	case CAMERA_GAMEPAD_ROTATE_UP_DOWN:
		{	
			rotateCamera((PxF32)0.0, (PxF32)val);
		}
		break;
	case CAMERA_GAMEPAD_MOVE_LEFT_RIGHT:
		{	
			moveCamera((PxF32)val, (PxF32)0.0);
		}
		break;
	case CAMERA_GAMEPAD_MOVE_FORWARD_BACK:
		{	
			moveCamera((PxF32)0.0, (PxF32)val);
		}
		break;
	}
}

void SampleApplication::rotateCamera(PxF32 dx, PxF32 dy)
{
	const float eyeCap      = 1.5f;
#if defined(SMOOTH_CAM)
	m_targetEyeRot.x -= dy * g_smoothCamRotSpeed;
	m_targetEyeRot.y += dx * g_smoothCamRotSpeed;
	if(m_targetEyeRot.x >  eyeCap) m_targetEyeRot.x =  eyeCap;
	if(m_targetEyeRot.x < -eyeCap) m_targetEyeRot.x = -eyeCap;

	m_eyeRot= m_eyeRot + (m_targetEyeRot - m_eyeRot) * g_smoothCamRotLerp;
#else
	const float eyeRotSpeed = 0.005f;
	m_eyeRot.x -= dy * eyeRotSpeed;
	m_eyeRot.y += dx * eyeRotSpeed;
	if(m_eyeRot.x >  eyeCap) m_eyeRot.x =  eyeCap;
	if(m_eyeRot.x < -eyeCap) m_eyeRot.x = -eyeCap;
#endif
}

void SampleApplication::fatalError(const char * msg)
{
shdfnd::printFormatted("Fatal Error in SampleApplication: %s\n", msg);
close();
exit(1);
}

void SampleApplication::doInput()
{
	m_platform->doInput();
}

void SampleApplication::setupRendererDescription(SampleRenderer::RendererDesc& renDesc)
{
	m_platform->setupRendererDescription(renDesc);
}

