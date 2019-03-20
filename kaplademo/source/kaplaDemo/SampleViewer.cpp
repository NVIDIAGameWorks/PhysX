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
// Copyright (c) 2011-2018 NVIDIA Corporation. All rights reserved.

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

#ifdef WIN32 
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif


#undef random

#include <stdio.h>
#include <assert.h>

#include "ShaderShadow.h"
#include "ShadowMap.h"
#include "RenderTarget.h"

#include <GL/glut.h>

#include "SampleViewer.h"
#include "MediaPath.h"

#include <task/PxTask.h>
#include "cudamanager/PxCudaContextManager.h"

#include "PxPhysics.h"
#include "PxCooking.h"
#include "PxDefaultAllocator.h"
#include "PxDefaultErrorCallback.h"
#include "PxDefaultCpuDispatcher.h"
#include "PxExtensionsAPI.h"
#include "PxPhysicsVersion.h"

#include "PxScene.h"
#include "PxGpu.h"

#include "foundation/PxProfiler.h"
#include "pvd/PxPvd.h"

//namespace PVD {
//	using namespace physx::debugger;
//	using namespace physx::debugger::comm;
//}
#include "Timing.h"
#include "GLFontRenderer.h"
#include "BmpFile.h"

#include "SceneKaplaTower.h"
#include "SceneKaplaArena.h"
#include "SceneRagdollWashingMachine.h"
#include "SceneVehicle.h"
#include "SceneCrab.h"

#include <string>
#include <algorithm>
//#include "fomHelper.h"
#include <string>
#include "MyShaders.h"
#include "SimScene.h"
#include "Texture.h"
#include "AABox.h"
#include "SSAOHelper.h"
#include "HBAOHelper.h"
#include "FXAAHelper.h"
#include "HDRHelper.h"
#include "SampleViewerGamepad.h"

#include "PhysXMacros.h"

#include "pvd/PxPvdTransport.h"
#include "foundation/PxMat44.h"
#include "PsTime.h"

#define SCREEN_DUMP_PATH "C:\\Capture\\%05i.bmp"

int DEFAULT_SOLVER_ITERATIONS = 8;


#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#define ENABLE_PVD				1
#define FULL_PVD				0

// Globals -------------------------------------------------------------------------------------------

int gPhysicsFrameNumber = 0;					// different from gFrameNr since the latter is for graphics
// ---------------------------------------------------------------------------------------------------
float g_fogStart = 150.0f;
float g_fogEnd = 350.0f;
float shadowClipNear = 0.01f;
float shadowClipFar = 80.0f;

int gNrWorkerThreads = 4;


extern float g_shadowAdd;
const float zNear = 0.1f;
const float zFar = 1000.0f;

SSAOHelper* gSSAOHelper = NULL;
HBAOHelper* gHBAOHelper = NULL;
FXAAHelper* gFXAAHelper = NULL;
HDRHelper* gHDRHelper = NULL;
GLenum shadowMapType = GL_TEXTURE_2D_ARRAY_EXT;
// PhysX
PxPhysics*				gPxPhysics = NULL;
PxFoundation*			gPxFoundation = NULL;
PxCooking*				gPxCooking = NULL;
PxDefaultAllocator		gAllocator;

PxScene*				gPxScene = NULL;
PxDefaultCpuDispatcher*	gPxDispatcher = NULL;

PxCudaContextManager* gCudaContextManager;

bool gUseGrb = true;
bool gCurrentUsingGrb = gUseGrb;

bool gUseVSync = false;
PxU32 gMinSimulatedFrames = 0;

bool gForceSceneRecreate = false;
int gCountdownToRecereate = 2;

bool gShot = false;

PxU32 gMaxSubSteps = 1u;

// Physics param
PxVec3	  gDefaultGravity(0.0f ,-10.0f, 0.0f);
float     gTimeStepSize = 1.0f / 60.0f;
float     gSlowMotionFactor = 1.0f;

// Scene
bool	  gSceneRunning = false;
float	  gTime = 0.0f;
float	  gLastTime = 0.0f;
bool      gPause = false;
bool      gDisableRendering = false;
bool	  gShowSplash = false;

#ifdef USE_OPTIX
int       gNumScenes = 9;
#else
int       gNumScenes = 5;
#endif
int       gSceneNr = 0;

float	  gGroundY = 0.0f; 
GLuint    gGroundTexId = 0;

GLuint	  gSplashScreenTexId = 0;
int       gSplashWidth = 0;
int       gSplashHeight = 0;
char gResourcePath[512] = "../../../externals/resources";

SampleViewerScene *gSampleScene = NULL;

// Display
GLFontRenderer *gFontRenderer = NULL;

int	  gMainHandle;
bool  gHelp = false;
bool  gDisplay = true;
char  gDisplayString[512] = "";
char  gHelpString[2048] = "";
char  gDemoName[512] = "";
float gFontSize = 0.02f;

int   gViewWidth = 0;
int   gViewHeight = 0;
bool  gShadows = false;
bool  gWireframeMode = false;
bool  gAdvancedRendering = true;
bool  gDebugRendering = true;
bool  gDrawGroundPlane = true;
PxVec3 gGroupPlanePose = PxVec3(0.f);
bool gDrawSkyBox = true;

//bool gFXAAOn = true;
//bool gDoSSAO = true;
//bool gDoHDR	 = true;

bool gFXAAOn = false;
bool gDoSSAO = true;
bool gDoHBAO = true;
bool gDoHDR = true;
bool gUseNormalMap = true;

bool gDoDOF	 = false;

// record movie
int gPixelBufferSize = 0;
GLbyte *gPixelBuffer = NULL;
bool  gRecordBMP = false;
int gFrameNr = 0;
int gMaxFrameNr = 0;

// Shading
int gNumShadows = 1;
bool gShowReflections = false;

static const int gNumLights = 1;
PxVec3 gLightDirs[3];
PxVec3 gLightPos[3];
PxVec3 gBackLightDir;

ShadowMap *gShadowMaps[4];
ShaderShadow *gDefaultShader;
RenderTarget *gReflectedSceneTarget = NULL;

int gZval = 15;
//FOMHelper* fom;
Shader* gDisplayTexProg;
Shader* gDisplayTexArrayProg;
Shader* gDisplaySplashProg = 0;
PxBounds3 sceneBounds;
PxBounds3 fluidBounds;
PxVec3 myDustColor;

bool renderDust = true;

#define MSAA_2x 2, 0, "MSAA_2x"
#define CSAA_4x 2, 4, "CSAA_4x"
#define CSAA_8x 4, 8, "CSAA_8x"
#define CSAA_8xQ 8, 8, "CSAA_8xQ"
#define CSAA_16x 4, 16, "CSAA_16x"
#define CSAA_16xQ 8, 16, "CSAA_16xQ"

AABox* g_pAABox = 0;
int g_numAAModes = 5;
//int fullW = 1900;
//int fullH = 1200;

int fullW = WINDOW_WIDTH;
int fullH = WINDOW_HEIGHT;
bool autoShoot = false;
bool gForceReloadShader = false;
bool gForceReloadDebugShader = false;

using namespace std;

struct SAAMode 
{ 
	int ds; 
	int cs; 
	const char * name;
};

SAAMode g_aaModes[] = {
	{MSAA_2x},
	{CSAA_8x},
	{CSAA_8xQ},
	{CSAA_16x},
	{CSAA_16xQ},
	{ CSAA_4x }
};

bool doFXAA = false;
bool doAA = true;
int g_curAA = 0;
int g_curTech = 1;
float g_curSSSize = 1.7f;

bool gShowRayCast = false;
bool fullScreen = false;

// Camera 
struct Camera {
	void init() {
		fov = 40.0f;
		pos = PxVec3(0.0f, 30.0f, 20.0f);
		forward = PxVec3(0.0f,0.0f,-1.0f);
		right = PxVec3(1.0f,0.0f,0.0f);

		PxVec3 up(0, 1, 0);

		PxQuat qy(3.14/4.f, right);
		forward = qy.rotate(forward).getNormalized();
		right = forward.cross(PxVec3(0, 1, 0)).getNormalized();
		

		speed = 0.15f;
		rotate = false;
		translate = false;
		zoom = false;

		trackballMode = false;
		trackballCenter = PxVec3(0.0f, 1.0f, 0.0f);
		trackballRadius = 6.0f;
	}
	float fov;
	PxVec3  pos, forward, right;
	float speed;
	bool rotate, translate, zoom;
	bool trackballMode;
	PxVec3 trackballCenter;
	float trackballRadius;
};

Camera gCamera;

// Mouse
int gMouseX = 0;
int gMouseY = 0;

// keyboard
#define MAX_KEYS 256
bool gKeys[MAX_KEYS];

// fps
float curFPS = 0.0f;
int	   gFrameCounter = 0;
float  gPreviousTime = getCurrentTime();

volatile bool doneOneFrame = false;
volatile bool doneInit = false;
volatile bool doneRender = true;
volatile bool shouldEndThread = false;
volatile bool threadEnded = false;
volatile int simulateDoneS = 0;
bool renderEveryFrame = true;

bool gParticlesTexOutdate = true;
bool gConvexTexOutdate = true;


//std::vector<Compound*> delCompoundList;

SampleViewerGamepad sampleViewerGamepad;

// ------------------------------------------------------------------------------------
// This is needed to properly call cleanup routines when GLUT window is closed, as
// there is no close callback routine, and the default way simply calls exit(0) thus
// calling destructors w/o cleanups, leading to crash
// (see atexit function that sets exit callback)
// ------------------------------------------------------------------------------------
bool gIsSampleCleanedUp = false;
void ReleaseSDKs();
void CleanupSample();
// ------------------------------------------------------------------------------------

physx::PxPvd*                           gPvd = NULL;
physx::PxPvdTransport*                  gTransport = NULL;
physx::PxPvdInstrumentationFlags        gPvdFlags = physx::PxPvdInstrumentationFlag::eDEBUG;

#include "PsString.h"
#include "PsThread.h"

//class SampleViewerPVDHandler : public PxPvd
//{
//public:
//
//	void onPvdSendClassDescriptions(PVD::PvdConnection&) {}
//
//	void onPvdConnected(PVD::PvdConnection& )
//	{
//		//setup joint visualization.  This gets piped to pvd.
//		//gPxPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONSTRAINTS, true);
//		//gPxPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_CONTACTS, true);
//		//gPxPhysics->getVisualDebugger()->setVisualDebuggerFlag(PxVisualDebuggerFlag::eTRANSMIT_SCENEQUERIES, true);
//	}
//
//	void onPvdDisconnected(PVD::PvdConnection& conn)
//	{
//		conn.release();
//	}
//};
//
//SampleViewerPVDHandler gPVDConnectionHandler;

class SampleViewerErrorCallback : public PxErrorCallback
{
public:
	SampleViewerErrorCallback()		{}
	~SampleViewerErrorCallback()	{}

	virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		if (code == PxErrorCode::eDEBUG_INFO)
		{
			char buffer[1024];
			sprintf(buffer, "%s\n", message);
			physx::shdfnd::printString(buffer);
		}
		else
		{
			const char* errorCode = NULL;

			switch (code)
			{
			case PxErrorCode::eINVALID_PARAMETER:
				errorCode = "invalid parameter";
				break;
			case PxErrorCode::eINVALID_OPERATION:
				errorCode = "invalid operation";
				break;
			case PxErrorCode::eOUT_OF_MEMORY:
				errorCode = "out of memory";
				break;
			case PxErrorCode::eDEBUG_INFO:
				errorCode = "info";
				break;
			case PxErrorCode::eDEBUG_WARNING:
				errorCode = "warning";
				break;
			case PxErrorCode::ePERF_WARNING:
				errorCode = "performance warning";
				break;
			case PxErrorCode::eABORT:
				errorCode = "abort";
				break;
			default:
				errorCode = "unknown error";
				break;
			}

			PX_ASSERT(errorCode);
			if (errorCode)
			{
				char buffer[1024];
				sprintf(buffer, "%s (%d) : %s : %s\n", file, line, errorCode, message);

				physx::shdfnd::printString(buffer);

				// in debug builds halt execution for abort codes
				PX_ASSERT(code != PxErrorCode::eABORT);

				// in release builds we also want to halt execution 
				// and make sure that the error message is flushed  
				while (code == PxErrorCode::eABORT)
				{
					physx::shdfnd::printString(buffer);
					physx::shdfnd::Thread::sleep(1000);
				}
			}
		}
	}
};

SampleViewerErrorCallback	gErrorCallback;
// ------------------------------------------------------------------------------------
#include <direct.h>

// ------------------------------------------------------------------------------------
bool LoadTexture(const char *filename, GLuint &texId, bool createMipmaps, GLuint type = GL_TEXTURE_2D, int *width = NULL, int *height = NULL)
{
	char path[512];
	sprintf(path, "%s/%s", gResourcePath, filename);

	// read out image data
	int len = strlen(path);
	unsigned char* ptr = 0, *ptrBegin;
	int w, h;
	BmpLoaderBuffer loader;
	if (strcmp(&path[len-4], ".jpg") == 0) {
		printf("Error loading %s, jpg no longer supported\n", path);
		return false;
	} else {
		// bmp

		if (!loader.loadFile(path)) {
			fprintf(stderr, "Error loading bmp '%s'\n",path);
			return false;  
		}
		ptrBegin = ptr = loader.mRGB;
		h = loader.mHeight;
		w = loader.mWidth;

	}

	unsigned char *pColor = ptrBegin;

	int lineLen = 3*w;
	GLubyte *buffer = new GLubyte[w*h*3];
	GLubyte *writePtr = &buffer[(h-1)*lineLen];

	for (int i = 0; i < h; i++) {
		GLubyte *writePtr = &buffer[i*lineLen];
		GLubyte *readPtr = &pColor[i*lineLen];
		for (int j = 0; j < w; j++) {
			*writePtr++ = *readPtr++;
			*writePtr++ = *readPtr++;
			*writePtr++ = *readPtr++;
		}
	}

	// ------ generate texture --------------------------------

	glGenTextures(1, &texId);

	glBindTexture(type, texId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(type, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(type, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	if (createMipmaps) {
		glTexParameteri(type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		gluBuild2DMipmaps(type, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	}

	delete[] buffer;

	if (width)
		*width = w;
	if (height)
		*height = h;
	return true;
}

void displayTexture(GLuint tex)
{
	gDisplayTexProg->activate();
	gDisplayTexProg->bindTexture("tex", tex, GL_TEXTURE_2D, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
	glEnd();
	gDisplayTexProg->deactivate();
}
void displayTextureArray(GLuint tex, int slice)
{
	gDisplayTexArrayProg->activate();
	gDisplayTexArrayProg->bindTexture("tex", tex, GL_TEXTURE_2D_ARRAY_EXT, 0);
	gDisplayTexArrayProg->setUniform1("slice", slice);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
	glEnd();
	gDisplayTexArrayProg->deactivate();
}
const char *textureRECTPS = STRINGIFY(
uniform sampler2DRect tex;                                             
void main()                                                        
{                                                                  
gl_FragColor = texture2DRect(tex, gl_TexCoord[0].xy);              \n
//gl_FragColor = vec4(1,0,0,1);
}                                                                  
);
const char *texture2DArrayPS = STRINGIFY(
	uniform sampler2DArray tex;
void main()
{
	gl_FragColor = texture2DArray(tex, vec3(gl_TexCoord[0].xy,0.0f));              \n
		//gl_FragColor = vec4(1,0,0,1);
}
);

void displaySplashScreen() 
{
	float as = (float)gSplashWidth/gSplashHeight;

	float xv = 0.0f, yv = 0.0f;
	float bs = gViewWidth / ((float)gViewHeight);
	if (bs > as) {
		// Too wide
		// fit height
		yv = 1.0f;
		xv = (as*gViewHeight) / gViewWidth;
	} else {
		// Too Tall
		xv = 1.0f;
		yv = (gViewWidth/as) / gViewHeight;
	}


	gDisplaySplashProg->activate();
	gDisplaySplashProg->bindTexture("tex", gSplashScreenTexId, GL_TEXTURE_RECTANGLE_ARB, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, gSplashHeight); glVertex2f(-xv, -yv);
	glTexCoord2f(gSplashWidth, gSplashHeight); glVertex2f( xv, -yv);
	glTexCoord2f(gSplashWidth, 0.0f); glVertex2f( xv,  yv);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-xv,  yv);
	glEnd();
	gDisplaySplashProg->deactivate();

}

// ------------------------------------------------------------------------------------
void SaveFrameBuffer()
{
	char filename[256];
	sprintf(filename, SCREEN_DUMP_PATH, gFrameNr);

	int bufferSize = gViewWidth * gViewHeight * 3;
	if (gPixelBufferSize != bufferSize) {
		if (gPixelBuffer != NULL)
			free(gPixelBuffer);
		gPixelBuffer = (GLbyte *)malloc(bufferSize);
		gPixelBufferSize = bufferSize;
	}

	glReadBuffer(GL_BACK);
	glReadPixels(0,0, gViewWidth, gViewHeight, GL_RGB, GL_UNSIGNED_BYTE, gPixelBuffer);
	saveBmpRBG(filename, gViewWidth, gViewHeight, gPixelBuffer);
}

// ------------------------------------------------------------------------------------
void WaitForSim()
{
	if (gSceneRunning)
	{
		if (gSampleScene != NULL)
		{
			gSampleScene->syncAsynchronousWork();
		}

		PxU32 error;
		gPxScene->fetchResults(true, &error);

		assert(error == 0);
		gSceneRunning = false;
	}
}

// ------------------------------------------------------------------------------------
PxFilterFlags contactReportFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
										PxFilterObjectAttributes attributes1, PxFilterData filterData1,
										PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	PX_UNUSED((attributes0, attributes1, filterData0, filterData1, constantBlockSize, constantBlock));

	// all initial and persisting reports for everything, with per-point data
	pairFlags = PxPairFlag::eSOLVE_CONTACT | PxPairFlag::eDETECT_DISCRETE_CONTACT;
	return PxFilterFlag::eDEFAULT;
}

// ------------------------------------------------------------------------------------


void CreateSampleViewerScene();
void InitSampleViewerScene();

void RecreateSDKScenes()
{
	WaitForSim();

	// physX
	if (gPxScene != NULL)
		gPxScene->release();
	
	PxSceneDesc sceneDesc(gPxPhysics->getTolerancesScale());

	sceneDesc.gravity = PxVec3(0.0f, -10.0f * gSlowMotionFactor * gSlowMotionFactor, 0.0f);  // acceleration, factor squared
	if (!gPxDispatcher)
		gPxDispatcher = PxDefaultCpuDispatcherCreate(gNrWorkerThreads);

	if(!gCudaContextManager)
	{
		PxCudaContextManagerDesc cudaContextManagerDesc;
		gCudaContextManager = PxCreateCudaContextManager(*gPxFoundation, cudaContextManagerDesc);
	}



	sceneDesc.cpuDispatcher = gPxDispatcher;
	sceneDesc.cudaContextManager = gCudaContextManager;
	sceneDesc.filterShader = contactReportFilterShader;
	sceneDesc.flags |= PxSceneFlag::eENABLE_PCM;
	sceneDesc.flags |= PxSceneFlag::eENABLE_STABILIZATION;
	if (gUseGrb)
	{
		sceneDesc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		sceneDesc.broadPhaseType = PxBroadPhaseType::eGPU;
	}
	sceneDesc.sceneQueryUpdateMode = PxSceneQueryUpdateMode::eBUILD_ENABLED_COMMIT_DISABLED;
	sceneDesc.gpuDynamicsConfig.constraintBufferCapacity *= 2;
	sceneDesc.gpuDynamicsConfig.contactBufferCapacity *= 2;	//!< Capacity of contact buffer allocated in GPU global memory
	sceneDesc.gpuDynamicsConfig.tempBufferCapacity *= 2;		//!< Capacity of temp buffer allocated in pinned host memory.
	sceneDesc.gpuDynamicsConfig.contactStreamSize *= 2;		//!< Size of contact stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2* contactStreamCapacity * sizeof(PxContact).
	sceneDesc.gpuDynamicsConfig.patchStreamSize *= 2;			//!< Size of the contact patch stream buffer allocated in pinned host memory. This is double-buffered so total allocation size = 2 * patchStreamCapacity * sizeof(PxContactPatch).
	sceneDesc.gpuDynamicsConfig.forceStreamCapacity *= 2;		//!< Capacity of force buffer allocated in pinned host memory.

	gSampleScene->customizeSceneDesc(sceneDesc);

	gPxScene = gPxPhysics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = gPxScene->getScenePvdClient();
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, false);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, false);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, false);
	}

	if(gPxScene == NULL)
	{
		printf("\nError: Unable to create a PhysX scene, exiting the sample.\n\n");
		exit(0);
	}

	gCurrentUsingGrb = gUseGrb;
}

//-----------------------------------------------------------------------------
bool InitPhysX()
{
	// SDK
	gPxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	gTransport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

	gPvd = NULL;

#if ENABLE_PVD
		gPvd = physx::PxCreatePvd(*gPxFoundation);
		//gPvd->connect(*gTransport, physx::PxPvdInstrumentationFlag::eALL);
		gPvd->connect(*gTransport, physx::PxPvdInstrumentationFlag::ePROFILE);
#endif

	gPxPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gPxFoundation, PxTolerancesScale(), false, gPvd);

	

	if(gPxPhysics == NULL) 
	{
		printf("\nPhysXSDK create error.\nUnable to initialize the PhysX SDK, exiting the sample.\n\n");
		return false;
	}
	PxCookingParams params(gPxPhysics->getTolerancesScale());
	params.buildGPUData = true;
	gPxCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gPxFoundation, params);

	if (gPxCooking == NULL) 
	{
		printf("\nError: Unable to initialize the cooking library, exiting the sample.\n\n");
		return false;
	}

	PxInitExtensions(*gPxPhysics, gPvd);

	UpdateTime();

	return true;
}

// ------------------------------------------------------------------------------------
void ReleaseSDKs()
{
	WaitForSim();

	if(gPxPhysics != NULL)
	{
		PxCloseExtensions();

		gPxScene->release();
		gPxDispatcher->release();
		gPxPhysics->release();
		gPxCooking->release();
		if (gPvd)
		{
			gPvd->release();
			gPvd = NULL;
		}

		gCudaContextManager->release();
		
		gPxFoundation->release();
		gPxScene = NULL;
		gPxDispatcher = NULL;
		gPxPhysics = NULL;
		gPxCooking = NULL;
		gPxFoundation = NULL;
		gCudaContextManager = NULL;
	}
}

// ------------------------------------------------------------------------------------
PxReal UpdateTime()
{
    PxReal deltaTime;
    gTime = timeGetTime()*0.001f;  // Get current time in seconds
    deltaTime = gTime - gLastTime;
    gLastTime = gTime;

    return deltaTime;
}

// ------------------------------------------------------------------------------------
static bool   g_simulationRunning = false;
static PxReal g_simulationDT      = 0.0f;
void BeginSimulation(void)
{
	assert(g_simulationRunning == false);
	assert(g_simulationDT == 0.0f);
	if(g_simulationRunning == false)
	{
		//static float prevTime = getCurrentTime();
		static uint64_t prevTime = physx::shdfnd::Time::getCurrentTimeInTensOfNanoSeconds();

		uint64_t currTime = physx::shdfnd::Time::getCurrentTimeInTensOfNanoSeconds();

		uint64_t elapsedTime = currTime - prevTime;

		uint64_t timestepSize = uint64_t(gTimeStepSize * 1e8f);

		PxReal fSubsteps = elapsedTime / timestepSize;
		PxU32 iSubsteps = PxMax(gMinSimulatedFrames, PxU32(fSubsteps));
		PxU32 nbSubsteps = PxMin(iSubsteps, gMaxSubSteps);

		if (elapsedTime > timestepSize)
		{
			prevTime += timestepSize*iSubsteps;
		}

		for (PxU32 a = 0; a < nbSubsteps; ++a)
		{

			if (gForceSceneRecreate)
			{
				if (--gCountdownToRecereate)
				{
					CreateSampleViewerScene();
					RecreateSDKScenes();
					InitSampleViewerScene();

					gForceSceneRecreate = false;
				}
			}
			else if (gUseGrb != gCurrentUsingGrb)
			{
				PxScene* oldScene = gPxScene;
				gPxScene = NULL;

				RecreateSDKScenes(); //Create the PxScene...

				const PxU32 nbRigidStatics = oldScene->getNbActors(PxActorTypeFlag::eRIGID_STATIC);
				const PxU32 nbRigidDynamics = oldScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);

				Ps::Array<PxActor*> actors(nbRigidStatics + nbRigidDynamics);

				oldScene->getActors(PxActorTypeFlag::eRIGID_STATIC, actors.begin(), nbRigidStatics);
				oldScene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC, actors.begin() + nbRigidStatics, nbRigidDynamics);
				

				for (PxU32 a = 0; a < actors.size(); ++a)
				{
					oldScene->removeActor(*actors[a],false);
					//gPxScene->addActor(*actors[a]);
				}

				for (PxU32 a = 0; a < actors.size(); ++a)
				{
					//oldScene->removeActor(*actors[a]);
					gPxScene->addActor(*actors[a]);
				}

				oldScene->release();

				gSampleScene->setScene(gPxScene);

				gCurrentUsingGrb = gUseGrb;
			}

			{
				if (gSampleScene != NULL)
					gSampleScene->preSim(gTimeStepSize);

				gPxScene->simulate(gTimeStepSize);

				if (gSampleScene != NULL)
					gSampleScene->duringSim(gTimeStepSize);

				gSceneRunning = true;

				g_simulationDT = gTimeStepSize;
				g_simulationRunning = true;
			}

			if((a + 1) < nbSubsteps)
			{
				WaitForSim();
				if (gSampleScene != NULL)
				{
					gSampleScene->postSim(gTimeStepSize);
				}
			}
		}

		
	}
}

// ------------------------------------------------------------------------------------
void EndSimulation(void)
{
	if(g_simulationRunning == true)
	{
		PxReal dt = g_simulationDT;

		if (gSceneRunning)
		{
			//fetchResult
			WaitForSim();
			if (gSampleScene != NULL)
			{
				gSampleScene->postSim(gTimeStepSize);
			}
		}

		++gPhysicsFrameNumber;
	
		g_simulationDT = 0.0f;
		g_simulationRunning = false;
		
		gParticlesTexOutdate = true;
		gConvexTexOutdate = true;
	}
}

// ------------------------------------------------------------------------------------
void RunPhysics()
{
	BeginSimulation();
	EndSimulation();
}

// ------------------------------------------------------------------------------------

void DisplayText()
{
	const char* aoString = gDoSSAO ? (gDoHBAO ? "(HBAO+)" : "(SSAO)"): "(NOAO)";
	const char* normalString = gUseNormalMap ? "(NormalMap)" : "(NormalsFromDepth)";
	sprintf(gDisplayString, "");
	if (gDisplay)
		sprintf(gDisplayString, "FPS = %0.2f\n Scene %i: %s %s%s %s %s", curFPS, gSceneNr + 1, gDemoName, gUseGrb ? "(GPU)" : "(CPU)", gHelp ? gHelpString : "", aoString, normalString);


	float y = 0.95f;
	int len = strlen(gDisplayString);
	len = (len < 1024)?len:1023;
	int start = 0;
	char textBuffer[1024];
	for(int i=0;i<len;i++)
	{
		if(gDisplayString[i] == '\n' || i == len-1)
		{
			int offset = i;
			if(i == len-1) offset= i+1;
			memcpy(textBuffer, gDisplayString+start, offset-start);
			textBuffer[offset-start]=0;
			gFontRenderer->print(0.01, y, gFontSize, textBuffer);
			y -= 0.035f;
			start = offset+1;
		}
	}
}

// ------------------------------------------------------------------------------------
void SetHelpString()
{
	//char tempString[256];
	sprintf(gHelpString, "\nGeneral:\n");
	//sprintf(tempString, "    1-%d: choose scene\n", gNumScenes);

	strcat(gHelpString, "    F10: Reset scene\n");
	strcat(gHelpString, "    1-5 or F10: Reset scene\n");
	strcat(gHelpString, "    F5: Toggle between CPU and GPU simulation\n");
	strcat(gHelpString, "    p: Pause\n");
	strcat(gHelpString, "    o: Single step\n");
	if (gSceneNr == 2)
	{
		strcat(gHelpString, "    F7: Detach/reattach camera\n");
		strcat(gHelpString, "    w,a,s,d,q,e: Drive car or move camera if camera is detached\n");
	}

	if (gSceneNr == 3)
	{
		strcat(gHelpString, "    k: Spawn walker at mouse location\n");
	}
	strcat(gHelpString, "    w,a,s,d,q,e: Camera\n");
    strcat(gHelpString, "    f: Toggle full screen mode\n");

	strcat(gHelpString, "    F1: Toggle help\n");
	strcat(gHelpString, "    F3: Uncap simulation rate\n");

	strcat(gHelpString, "    F4: Change weapon: "); 
	if (gSampleScene != NULL)
		strcat(gHelpString, gSampleScene->getWeaponName().c_str());
	strcat(gHelpString, "\n");

	strcat(gHelpString, "    Shift-mouse: Drag objects\n");
	strcat(gHelpString, "    Space: Shoot\n");
	if (gSceneNr == 0)
		strcat(gHelpString, "    b: Attack tower with random shapes\n");
}

// ------------------------------------------------------------------------------------
void ProcessKeys()
{
	// Process keys
	for (int i = 0; i < MAX_KEYS; i++)
	{	
		if (!gKeys[i])  { continue; }

		PxVec3 up;
		up = PxVec3(0,1,0);
		PxVec3 &pos = gCamera.trackballMode ? gCamera.trackballCenter : gCamera.pos;

	
		if (!gSampleScene->isCameraDisable())
		{
			if (gCamera.trackballMode) {
				switch (i)
				{
					// Camera controls
				case 'W':
				case 'w':{
					gCamera.trackballRadius -= gCamera.speed;
					if (gCamera.trackballRadius < 0.1f) gCamera.trackballRadius = 0.1f;
					break;
				}
				case 'S':
				case 's':{ gCamera.trackballRadius += gCamera.speed; break; }
				case 'E':
				case 'e':{ pos -= up * gCamera.speed; break; }
				case 'Q':
				case 'q':{ pos += up * gCamera.speed; break; }
				}
			}
			else {
				switch (i)
				{
					// Camera controls
				case 'W':
				case 'w':{ pos += gCamera.forward * gCamera.speed; break; }
				case 'S':
				case 's':{ pos -= gCamera.forward * gCamera.speed; break; }
				case 'A':
				case 'a':{ pos -= gCamera.right * gCamera.speed; break; }
				case 'D':
				case 'd':{ pos += gCamera.right * gCamera.speed; break; }
				case 'E':
				case 'e':{ pos -= up * gCamera.speed; break; }
				case 'Q':
				case 'q':{ pos += up * gCamera.speed; break; }
				}
			}
		}
		
	}
}
float XXX = 0.0f;
	float modelMatrix[16];
	float projMatrix[16];
// ------------------------------------------------------------------------------------

void SetupCamera(bool mirrored = false)
{
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	float aspectRatio = ((float)glutGet(GLUT_WINDOW_WIDTH)/(float)glutGet(GLUT_WINDOW_HEIGHT));
	gluPerspective(gCamera.fov, aspectRatio, zNear, zFar);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (gCamera.trackballMode) {
		gCamera.trackballCenter.x = 0.0f;
		gCamera.trackballCenter.z = 0.0f;
		gCamera.pos = gCamera.trackballCenter - gCamera.forward * gCamera.trackballRadius;
	}


	gSampleScene->getCamera(gCamera.pos, gCamera.forward);

	PxVec3 pos = gCamera.pos;
	PxVec3 dir = gCamera.forward;
	PxVec3 apos = gCamera.pos + dir;

	
	gluLookAt(pos.x, pos.y, pos.z, pos.x + dir.x, pos.y + dir.y, pos.z + dir.z, 0.0f, 1.0f, 0.0f);

	if (mirrored) {
		PxTransform trans;

		PxVec3 waterN(0.0f, 1.0f, 0.0f);
		PxVec3 waterP(0.0f,gGroundY,0.0f);
		float QQQ = 0.00f;
		waterP.y += QQQ;
		float np2 = 2.0f*waterN.dot(waterP);

		PxVec3 r0 = PxVec3(1.0f, 0.0f, 0.0f) - 2*waterN.x*waterN;
		PxVec3 r1 = PxVec3(0.0f, 1.0f, 0.0f) - 2*waterN.y*waterN;
		PxVec3 r2 = PxVec3(0.0f, 0.0f, 1.0f) - 2*waterN.z*waterN;
		PxVec3 t = np2*waterN;

		float matGL[16] = {
			r0.x, r1.x, r2.x, 0.0f,
			r0.y, r1.y, r2.y, 0.0f,
			r0.z, r1.z, r2.z, 0.0f,
			t.x,  t.y,  t.z,  1.0f};

		glMultMatrixf(matGL);

	}


	glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetFloatv(GL_PROJECTION_MATRIX, projMatrix);

	gDefaultShader->updateCamera(modelMatrix, projMatrix);
	if (gSampleScene != NULL) {
		for (int i = 0; i < (int)gSampleScene->getShaders().size(); i++)
			gSampleScene->getShaders()[i]->updateCamera(modelMatrix, projMatrix);
		gSampleScene->setCamera(gCamera.pos, gCamera.forward, PxVec3(0,1,0), gCamera.fov);
	}

	gCamera.right = gCamera.forward.cross(PxVec3(0,1,0)).getNormalized();
	gCamera.forward.normalize();

}

// ------------------------------------------------------------------------------------
void RenderObjects(bool useShader, bool reflectedOnly)
{
	ShaderMaterial mat;
	mat.init();

	// ground plane
	if (!reflectedOnly && gDrawGroundPlane) 
	{
		if (useShader) {
			mat.init();
			mat.reflectionCoeff = 0.2f;
			if (gGroundTexId > 0) {
				mat.texId = gGroundTexId;
			}
		}
		if (useShader) 
			gDefaultShader->activate(mat);

		const float size = 1000.0f;
		const float y = -0.002f;

		
		PxVec3 center = gGroupPlanePose;
		PxVec3 p0 = center + PxVec3(-size, y, -size);
		PxVec3 p1 = center + PxVec3( size, y, -size);
		PxVec3 p2 = center + PxVec3( size, y,  size);
		PxVec3 p3 = center + PxVec3(-size, y,  size);
		
		float texScale = 300.0f; 
		PxVec3 t0(0.0f, 0.0f, 0.0f);
		PxVec3 t1(texScale, 0.0f, 0.0f);
		PxVec3 t2(texScale, texScale, 0.0f);
		PxVec3 t3(0.0f, texScale, 0.0f);

		glBegin(GL_TRIANGLES);
			glNormal3f(0.0f, 1.0f, 0.0f);
			glTexCoord2f(t0.x, t0.y); glVertex3f(p0.x, p0.y, p0.z);
			glTexCoord2f(t2.x, t2.y); glVertex3f(p2.x, p2.y, p2.z);
			glTexCoord2f(t1.x, t1.y); glVertex3f(p1.x, p1.y, p1.z);

			glTexCoord2f(t0.x, t0.y); glVertex3f(p0.x, p0.y, p0.z);
			glTexCoord2f(t3.x, t3.y); glVertex3f(p3.x, p3.y, p3.z);
			glTexCoord2f(t2.x, t2.y); glVertex3f(p2.x, p2.y, p2.z);
		glEnd();
		if (useShader) 
			gDefaultShader->deactivate();
	}

	if (gSampleScene != NULL)
		gSampleScene->render(useShader);

/*
	glDisable(GL_LIGHTING);

	
	glBindBuffer(GL_ARRAY_BUFFER, gFluidSim.mistPS->glVB[currentBufMultiGPU]);
	glVertexPointer(3, GL_FLOAT, sizeof(CUGLParticleInfo), (void*)0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor3f(1.0f,1.0f,1.0f);
	glDrawArrays(GL_POINTS, 0, gFluidSim.mistPS->numGLPars[currentBufMultiGPU]);

	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	*/
	
//glDisable(GL_LIGHTING);

	if (gShowRayCast) {

		glDisable(GL_LIGHTING);
		SimScene* simScene = ((SceneKapla*)gSampleScene)->getSimScene();
		PxScene* scene = simScene->getScene();

		PxVec3 start(-30.0f,0.0f,-30.0f);
		PxVec3 xdir(1.0f,0.0f,0.0f);
		PxVec3 zdir(0.0f,0.0f,1.0f);
		PxVec3 castDir(0.0f, -1.0f, 0.0f);
		PxVec3 castPos(0.0f, 100.0f, 0.0f);
		float dist = 300.0f;
		float dx = 0.3f;
		int nx = 200;
		int nz = 200;

		
		glPointSize(10.0f);
		glBegin(GL_POINTS);
		
		PxHitFlags ff = PxHitFlag::ePOSITION | PxHitFlag::eNORMAL;
		PxRaycastBuffer buf1;
		PxRaycastHit hit;
		for (int i = 0; i < nz; i++) {
			for (int j = 0; j < nx; j++) {
				PxVec3 startP = start + xdir*dx*j + zdir*dx*i + castPos;
				scene->raycast(startP, castDir, dist, buf1, ff);
				hit = buf1.block;
				if (buf1.hasBlock) {
					PxVec3 pos = startP + hit.distance * castDir;
					glColor3f(1.0f,0.0f,0.0f);
					glVertex3fv(&pos.x);
				} else {
					PxVec3 pos = startP;
					pos.y = 0.0f;
					glColor3f(0.0f,1.0f,0.0f);
					glVertex3fv(&pos.x);

				}

	//			scene->raycastSingle()

			}
		}
		
		glEnd();
		glEnable(GL_LIGHTING);
	}
}

// ------------------------------------------------------------------------------------
void RenderShadowCasters()
{
	RenderObjects(false, true);
}

inline void RenderDepthNormalInline(bool renderNormals){
	if(renderNormals)
		ShaderShadow::renderMode = ShaderShadow::RENDER_DEPTH_NORMAL;
	else
		ShaderShadow::renderMode = ShaderShadow::RENDER_DEPTH;
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (gCamera.trackballMode) {
		gCamera.trackballCenter.x = 0.0f;
		gCamera.trackballCenter.z = 0.0f;
		gCamera.pos = gCamera.trackballCenter - gCamera.forward * gCamera.trackballRadius;
	}

	PxVec3 pos = gCamera.pos;
	PxVec3 dir = gCamera.forward;
	PxVec3 apos = gCamera.pos + dir;
	gluLookAt(pos.x, pos.y, pos.z, pos.x + dir.x, pos.y + dir.y, pos.z + dir.z, 0.0f, 1.0f, 0.0f);
	RenderObjects(true, false);
	
	ShaderShadow::renderMode = ShaderShadow::RENDER_COLOR;

}

void RenderDepthNormal()
{
	RenderDepthNormalInline(true);
}

void RenderDepthOnly()
{
	RenderDepthNormalInline(false);
}

// ------------------------------------------------------------------------------------
void RenderScene()
{
	// shadows
	SetupCamera(false);

	int numShadows = gNumShadows;
	bool showReflections = gShowReflections;
	if (SampleViewerScene::getRenderType() == SampleViewerScene::rtOPTIX) {
		numShadows = 0;
		showReflections = false;
	}

	if (numShadows > 0) {
		ShaderShadow::renderMode = ShaderShadow::RENDER_DEPTH;
		for (int i = 0; i < gNumLights; i++) {
			if (gShadowMaps[i] == NULL)
				//gShadowMaps[i] = new ShadowMap(gViewWidth, gViewHeight, gCamera.fov, i,12000);
				//gShadowMaps[i] = new ShadowMap(gViewWidth, gViewHeight, gCamera.fov, i, 12000);
				gShadowMaps[i] = new ShadowMap(gViewWidth, gViewHeight, gCamera.fov, i, 6144);

			gDefaultShader->setShadowMap(i, gShadowMaps[i]);
			if (gSampleScene != NULL) {
				for (int j = 0; j < (int)gSampleScene->getShaders().size(); j++)
					gSampleScene->getShaders()[j]->setShadowMap(i, gShadowMaps[i]);
			}
			if (i > numShadows)
				continue;

			
			if (gShadowMaps[i] != NULL)
				gShadowMaps[i]->makeShadowMap(gCamera.pos, gCamera.forward, gLightDirs[i], 
					shadowClipNear, shadowClipFar, &RenderShadowCasters);
		}
		ShaderShadow::renderMode = ShaderShadow::RENDER_COLOR;
	}
	
	for (int i = 0; i < gNumLights; i++) 
		gDefaultShader->setSpotLight(i, gLightPos[i], gLightDirs[i]);
	gDefaultShader->setBackLightDir(gBackLightDir);
	gDefaultShader->setNumShadows(numShadows);
	gDefaultShader->setShowReflection(showReflections);

	if (gSampleScene != NULL) {
		for (int i = 0; i < (int)gSampleScene->getShaders().size(); i++) {
			ShaderShadow *s = gSampleScene->getShaders()[i];
			for (int j = 0; j < gNumLights; j++) 
				s->setSpotLight(j, gLightPos[j], gLightDirs[j]);
			s->setBackLightDir(gBackLightDir);
			s->setNumShadows(numShadows);
			s->setShowReflection(showReflections);
		}
	}

	// reflection
	if (showReflections) {

		int reflectWidth = gViewWidth;
		int reflectHeight = gViewHeight;

		if (doAA) {
			reflectWidth = g_pAABox->getBufW();
			reflectHeight = g_pAABox->getBufH();
		}
		if (gReflectedSceneTarget == NULL) {
			
			gReflectedSceneTarget = new RenderTarget(reflectWidth, reflectHeight);
			gReflectedSceneTarget->beginCapture();
			glViewport(0, 0, reflectWidth, reflectHeight);
			gReflectedSceneTarget->endCapture();
			
			gDefaultShader->setReflectionTexId(gReflectedSceneTarget->getColorTexId());
			if (gSampleScene != NULL) {
				for (int i = 0; i < (int)gSampleScene->getShaders().size(); i++)
					gSampleScene->getShaders()[i]->setReflectionTexId(gReflectedSceneTarget->getColorTexId());
			}
		}

		glViewport(0, 0, reflectWidth, reflectHeight);
		gReflectedSceneTarget->beginCapture();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
//		glClear(GL_DEPTH_BUFFER_BIT);	// todo: why is this faster?

		GLdouble clipEq[4];
		clipEq[0] = 0.0f;
		clipEq[1] = -1.0f;
		clipEq[2] = 0.0f;
		clipEq[3] = gGroundY;
		glEnable(GL_CLIP_PLANE0);
		glClipPlane(GL_CLIP_PLANE0, clipEq);

		SetupCamera(true);
		RenderObjects(true, true);

		glDisable(GL_CLIP_PLANE0);
		gReflectedSceneTarget->endCapture();
	}
	glViewport(0, 0, gViewWidth, gViewHeight);

	SetupCamera();
	
	bool useFXAA = doFXAA && gFXAAOn;

	if (gDoHDR) 
		gHDRHelper->beginHDR(true);
	else 
	{
		if (useFXAA)
			gFXAAHelper->StartFXAA();
	}

	if (doAA) {
		if (gDoHDR) {
			g_pAABox->oldFbo = gHDRHelper->mHDRFbo;

		}
		g_pAABox->Activate(0,0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}


	RenderObjects(true, false);

	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	if (gDoSSAO) {
		int oldFbo = (doAA)? (g_pAABox->fbms ? g_pAABox->fbms : g_pAABox->fb) : 0;
		
		if (useFXAA) oldFbo = gFXAAHelper->FBO;
		if (gDoHDR) {
			if (!doAA) {
				oldFbo = gHDRHelper->mHDRFbo;
			}
		}
		
		if (gDoHBAO)
			gHBAOHelper->renderAO(gUseNormalMap ? RenderDepthNormal : RenderDepthOnly, oldFbo, gUseNormalMap);
		else
			gSSAOHelper->DoSSAO(RenderDepthNormal, oldFbo);
	}

	if (doAA) {g_pAABox->Deactivate();}
	if (doAA) g_pAABox->Draw(g_curTech);

	if (gDoHDR) 
	{
		if (useFXAA) gFXAAHelper->StartFXAA();
		if (useFXAA) {
			gHDRHelper->DoHDR(gFXAAHelper->FBO, gDoDOF);
		} else {
			gHDRHelper->DoHDR(0, gDoDOF);
		}
	}

	if (useFXAA)
		gFXAAHelper->EndFXAA(0);
}

// ------------------------------------------------------------------------------------
void RenderCallback()
{
	if (gForceReloadShader) {
		SceneKapla* scene = (SceneKapla*) gSampleScene;		
		scene->mSimSceneShader->loadShaders((string(gResourcePath) + string("\\shaders\\scene_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\scene_fs.cpp")).c_str());
		gForceReloadShader = false;
	}
	if (gForceReloadDebugShader) {
		gDisplayTexArrayProg->loadShaders((string(gResourcePath) + string("\\shaders\\passthrough_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\shadowdebug_fs.cpp")).c_str());
		gDefaultShader->loadShaders((string(gResourcePath) + string("\\shaders\\default_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\default_fs.cpp")).c_str());
		gHDRHelper->mShaderDOF.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\dof_fs.cpp")).c_str());
		gHDRHelper->mShaderBloomH.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\bloomH_fs.cpp")).c_str());
		gHDRHelper->mShaderBloomV.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\bloomV_fs.cpp")).c_str());
		gSSAOHelper->SSAOFilterH.loadShaders((string(gResourcePath) + string("\\shaders\\filterv_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\filterh_fs.cpp")).c_str());
		gSSAOHelper->SSAOFilterV.loadShaders((string(gResourcePath) + string("\\shaders\\filterv_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\filterv_fs.cpp")).c_str());

		SceneKapla* scene = (SceneKapla*)gSampleScene;
		if (!scene->mSimSceneShader->loadShaders((string(gResourcePath) + string("\\shaders\\combine_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\combine_fs.cpp")).c_str())) {
			printf("Can't load shader\n");
		}
		gForceReloadDebugShader = false;
		

	}

	static float first_frame_time = getCurrentTime();

	ProcessKeys();
	if (gDisableRendering)
	{
		if(!gPause)
		{
			BeginSimulation();
			EndSimulation();
		}
	}
	else 
	{	
		if (!gPause) 
			BeginSimulation();

		// Clear buffers
	//	glClearColor(0.52f, 0.60f, 0.71f, 1.0f);  
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (SampleViewerScene::getRenderType() == SampleViewerScene::rtOPTIX) {
			gSampleScene->render(true);
			SetupCamera();
		} else {
			RenderScene();
		}
    	
   		DisplayText();

		if(gSampleScene)
		{
			sampleViewerGamepad.processGamepads(*gSampleScene);
		}


		if (!gPause)
			EndSimulation();

		glutSwapBuffers();
		glutReportErrors();

		

		if (gRecordBMP)
			SaveFrameBuffer();
	}	
	
	gFrameNr++;		  // global frame nr
	gFrameCounter++;  // FPS

	if (gMaxFrameNr > 0) { // benchmark mode
		if (gFrameNr >= gMaxFrameNr) {
			printf("Benchmark done %d frames: %g seconds.\n",
			gMaxFrameNr, getCurrentTime() - first_frame_time );
			SampleViewerScene::cleanupStaticResources();
			exit(0);
		}
	}

	if (gShot) {
	}


}

// ------------------------------------------------------------------------------------
void ReshapeCallback(int width, int height)
{
	gViewWidth = width;
	gViewHeight = height;
    glViewport(0, 0, width, height);

	if (gDoHDR)
	{
		gHDRHelper->Resize(width, height);
	}

	if (doAA) {
		if(g_pAABox) delete g_pAABox;
		g_pAABox = new AABox(gResourcePath);
		g_pAABox->Initialize(width, height, g_curSSSize, g_aaModes[g_curAA].ds, g_aaModes[g_curAA].cs);
		gSSAOHelper->Resize(g_pAABox->bufw, g_pAABox->bufh);
		gHBAOHelper->resize(g_pAABox->bufw, g_pAABox->bufh, width, height);
	} else {
		gSSAOHelper->Resize(width, height);
		gHBAOHelper->resize(width, height, width, height);
	}
	if (doFXAA) {
		gFXAAHelper->Resize(width, height);
	}
	
}

// ------------------------------------------------------------------------------------
void IdleCallback()
{
    glutPostRedisplay();

	float time = getCurrentTime();
	float elapsedTime = time - gPreviousTime;
	
	if (elapsedTime > 1.0f) {
		char title[30];
		curFPS = (float)gFrameCounter / elapsedTime;
		//
		sprintf(title, "Kapla Demo");
		glutSetWindowTitle(title);
		gPreviousTime = time;
		gFrameCounter = 0;
	}
	getElapsedTime();
}


void CreateSampleViewerScene()
{
	if (gSampleScene != NULL)
		delete gSampleScene;
	gSampleScene = NULL;

	if (gMaxFrameNr > 0) { // benchmark mode
		gPause = false;
	}
	else {
		gPause = false;
	}


	float sceneSize = 20.0f;
	sceneBounds.minimum = PxVec3(-sceneSize, 0.0f, -sceneSize);
	sceneBounds.maximum = PxVec3(sceneSize, sceneSize, sceneSize);

	float fluidSize = 5.0f;


	//printf("Scene B is %f %f %f --- %f %f %f\n", sceneBounds.minimum.x, sceneBounds.minimum.y, sceneBounds.minimum.z, 
	//	sceneBounds.maximum.x, sceneBounds.maximum.y, sceneBounds.maximum.z);
	//gForceReloadDebugShader = true;
	PxVec3 target(0.0f, 0.0f, 0.0f);
	g_fogStart = 50.0f;
	g_fogEnd = 150.0f;
	shadowClipNear = 0.2f;
	shadowClipFar = 100.0f;

	DEFAULT_SOLVER_ITERATIONS = 8;

	g_shadowAdd = -0.001f;

	switch (gSceneNr) {
	case 0:
	{
		gSampleScene = new SceneKaplaTower(gPxPhysics, gPxCooking, gUseGrb, gDefaultShader, gResourcePath, gSlowMotionFactor);
		gLightPos[0] = PxVec3(sceneBounds.maximum.x, sceneBounds.maximum.y*3.0f, sceneBounds.maximum.z*2.f)*5.0f;
		//g_shadowAdd = 0.f;// -0.004054f;

		sprintf(gDemoName, "GRB Kapla Tower");
		break;
	}
	case 1:
	{
		gSampleScene = new SceneKaplaArena(gPxPhysics, gPxCooking, gUseGrb, gDefaultShader, gResourcePath, gSlowMotionFactor);
		gLightPos[0] = PxVec3(sceneBounds.maximum.x, sceneBounds.maximum.y*3.0f, sceneBounds.maximum.z)*5.0f;
		//g_shadowAdd = -0.003;
		//g_shadowAdd = 0.f;
		sprintf(gDemoName, "GRB Kapla Arena");
		break;
	}
	case 2:
	{
		gSampleScene = new SceneVehicle(gPxPhysics, gPxCooking, gUseGrb, gDefaultShader, gResourcePath, gSlowMotionFactor);
		gLightPos[0] = PxVec3(sceneBounds.maximum.x, sceneBounds.maximum.y*3.0f, -sceneBounds.maximum.z)*5.0f;
		//g_shadowAdd = -0.003;
		//g_shadowAdd = -0.001891;
		//g_shadowAdd = 0.f;
		sprintf(gDemoName, "GRB Vehicle Demo");
		break;
	}
	case 3:
	{
		//DEFAULT_SOLVER_ITERATIONS = 4;
		gSampleScene = new SceneCrab(gPxPhysics, gPxCooking, gUseGrb, gDefaultShader, gResourcePath, gSlowMotionFactor);
		gLightPos[0] = PxVec3(-sceneBounds.maximum.x, sceneBounds.maximum.y*8.0f, sceneBounds.maximum.z)*8.0f;
		//g_shadowAdd = -0.001891;
		//g_shadowAdd = 0.f;
		g_fogStart = 150.0f;
		g_fogEnd = 350.0f;
		//shadowClipFar = 100.0f;
		sprintf(gDemoName, "GRB Walkers Demo ");

		break;
	}
	default:
	{
		//DEFAULT_SOLVER_ITERATIONS = 4;
		//g_shadowAdd = -0.004054f;
		//g_shadowAdd = 0.f;
		//shadowClipFar = 130.0f;
		gLightPos[0] = PxVec3(0.f, 25.f, 150.f);
		gSampleScene = new SceneRagdollWashingMachine(gPxPhysics, gPxCooking, gUseGrb, gDefaultShader, gResourcePath, gSlowMotionFactor);
		sprintf(gDemoName, "GRB Ragdoll Demo ");
		break;
	}
	}

	SceneKapla* scene = (SceneKapla*)gSampleScene;

	if (!scene->mSimSceneShader->loadShaders((string(gResourcePath) + string("\\shaders\\combine_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\combine_fs.cpp")).c_str()))
	{
		printf("Can't load shader\n");
	}

	gSampleScene->getInitialCamera(gCamera.pos, gCamera.forward);
	float fogColor[4] = { 0, 0, 0, 1 };
	glFogf(GL_FOG_DENSITY, 0.0009f);
	glFogf(GL_FOG_START, g_fogStart);
	glFogf(GL_FOG_END, g_fogEnd);
	glFogfv(GL_FOG_COLOR, fogColor);

	gLightDirs[0] = gLightPos[0] - target;
	gLightDirs[0].normalize();

	if (gSceneNr == 4)
	{
		gLightDirs[0] = PxVec3(0.f, 0.2f, 1.f).getNormalized();
	}

	gLightPos[1] = PxVec3(sceneBounds.minimum.x, sceneBounds.maximum.y, sceneBounds.minimum.z);
	gLightDirs[1] = gLightPos[1] - target;
	gLightDirs[1].normalize();

	gLightPos[2] = PxVec3((sceneBounds.minimum.x + sceneBounds.maximum.x) * 0.5f, sceneBounds.maximum.y, sceneBounds.minimum.z);
	gLightDirs[2] = gLightPos[2] - target;
	gLightDirs[2].normalize();

	gBackLightDir = PxVec3(0.8f, -1.0f, -1.2f);
	gBackLightDir.normalize();

	myDustColor = ((SceneKapla*)gSampleScene)->getSimSceneShader()->getDustColor();
	SetHelpString();

}

// ------------------------------------------------------------------------------------
void InitSampleViewerScene()
{
	gSampleScene->onInit(gPxScene);
}

void InitGlutCallbacks(bool, int, int);

//-----------------------------------------------------------------------------
void ToggleFullscreen()
{
        static int window[4] = {-1, -1, -1, -1};

        if ( window[0] == -1) { // We are in non-fullscreen mode    
                window[0] = glutGet(GLUT_WINDOW_WIDTH);
                window[1] = glutGet(GLUT_WINDOW_HEIGHT);
                window[2] = glutGet(GLUT_WINDOW_X);
                window[3] = glutGet(GLUT_WINDOW_Y);
                glutFullScreen();
        } 
        else 
		{ // We are in fullscreen mode
            glutPositionWindow( window[2], window[3] );
            glutReshapeWindow( window[0], window[1] );
            window[0] = -1;
		}

		

        glutPostRedisplay();
}


// ------------------------------------------------------------------------------------
void KeyboardCallback(unsigned char key, int x, int y)
{
	if (key != 'p' && key != 'w' && key != 'a' && key != 's' && key != 'd' && key != 'q' && key != 'e' && key != 'x' && key != 'v'
		&& key != 'u' && key != 'i' && key != 'k' && key != 'y' && key != 'b' && key != 'c' && key != 'h' && key != 'H' && key != 'n')
		gPause = false;
	
	if (key == ' ') {
		gShot = true;
	}

	gKeys[key] = true;

	if ('1' <= key && key <= '0' + gNumScenes) 
	{
		gSceneNr = key - '0' - 1;

		WaitForSim();

		if (gSampleScene)
			delete gSampleScene;
		gSampleScene = NULL;

		CreateSampleViewerScene();
		RecreateSDKScenes();
		
		InitSampleViewerScene();

	}

	switch (key)
	{
    case 'f' : ToggleFullscreen(); break;

	case 27 : { 
		CleanupSample();
		exit(0);
	} break;
	case 'p': { gPause = !gPause; UpdateTime(); break; }
	case 'o': { if (!gPause) gPause = true; RunPhysics(); glutPostRedisplay(); break; }
	case 'h': { gDoSSAO = !gDoSSAO; break; }
	case 'H': { gDoHBAO = !gDoHBAO; break; }
	//case 'n': { gUseNormalMap = !gUseNormalMap; break; }

	default: ;
	}

	if (gSampleScene != NULL)
		gSampleScene->handleKeyDown(key, x, y);
}

void CleanupSample()
{
	if (gIsSampleCleanedUp)
		return;

	WaitForSim();

	if (gSampleScene != NULL) {
		delete gSampleScene;
		gSampleScene = 0;
	}
	SampleViewerScene::cleanupStaticResources();
	ReleaseSDKs();

	glutDestroyWindow(gMainHandle);

	gIsSampleCleanedUp = true;
}

// ------------------------------------------------------------------------------------

void KeyboardUpCallback(unsigned char key, int x, int y)
{
	gKeys[key] = false;
	if (gSampleScene != NULL)
		gSampleScene->handleKeyUp(key, x, y);
}

// ------------------------------------------------------------------------------------

void SpecialCallback(int key, int x, int y)
{
	switch (key)
	{
		// Reset PhysX
	case GLUT_KEY_F1:
		gHelp = !gHelp;
		break;
	case GLUT_KEY_F2:
		gDisplay = !gDisplay;
		break;

		case GLUT_KEY_F10: 
		{
			WaitForSim();

			if (gSampleScene)
				delete gSampleScene;
			gSampleScene = NULL;

			CreateSampleViewerScene();
			RecreateSDKScenes();

			InitSampleViewerScene();
			return;
		}
		case GLUT_KEY_F5:
		{
			gUseGrb = !gUseGrb;
			break;
		}
		case GLUT_KEY_F3:
		{
			gMinSimulatedFrames = 1 - gMinSimulatedFrames;
			break;
		}
	}
	if (gSampleScene != NULL)
		gSampleScene->handleSpecialKey(key, x, y);

	SetHelpString();
}

// ------------------------------------------------------------------------------------
void MouseCallback(int button, int state, int x, int y)
{
	if (button != GLUT_LEFT_BUTTON || glutGetModifiers() != 0)
		gPause = false;

    gMouseX = x;
	gMouseY = y;

	if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) 
		{
			gCamera.zoom = true;
		}
		else if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		{
			gCamera.rotate = true;
		}
		else if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
		{
			gCamera.translate = true;
		}
	}
	if (state == GLUT_UP) 
	{
		gCamera.rotate = false;
		gCamera.translate = false;
		gCamera.zoom = false;
	}

	if (gSampleScene != NULL)
		gSampleScene->handleMouseButton(button, state, x, y);
}

// ------------------------------------------------------------------------------------

void MotionCallback(int x, int y)
{

    int dx = gMouseX - x;
    int dy = gMouseY - y;

	PxVec3 up;
	up = PxVec3(0,1,0);

    const float translateSpeed = 0.1f;

	if (gCamera.translate)
    {
		PxVec3 &pos = gCamera.trackballMode ? gCamera.trackballCenter : gCamera.pos;
		pos += gCamera.right * dx * translateSpeed;
		pos -= up * dy * translateSpeed;
    }
	else if (gCamera.rotate)
	{
		const float rotationSpeed = 0.01f;
		if (gCamera.trackballMode) {
			gCamera.forward.normalize();
			gCamera.right = gCamera.forward.cross(PxVec3(0,1,0));

			PxQuat qy(dx * rotationSpeed, PxVec3(0.0f, 1.0f, 0.0f));
			gCamera.forward = qy.rotate(gCamera.forward);
			PxQuat qx(dy * rotationSpeed, gCamera.right);
			gCamera.forward = qx.rotate(gCamera.forward);
			gCamera.forward.y = PxClamp(gCamera.forward.y, 0.9f, -0.9f);
			gCamera.forward.normalize();
			gCamera.pos = gCamera.trackballCenter - gCamera.forward * gCamera.trackballRadius;
		}
		else {
			PxQuat qx(dx * rotationSpeed, up);
			gCamera.forward = qx.rotate(gCamera.forward);
			PxQuat qy(dy * rotationSpeed, gCamera.right);
			PxVec3 tempForward = gCamera.forward;
			gCamera.forward = qy.rotate(gCamera.forward);
			gCamera.forward.normalize();

			if (PxAbs(gCamera.forward.y) > 0.98f)
			{
				//Clamp the axis...
				gCamera.forward = tempForward;
				gCamera.forward.normalize();
			}
		}
	}
	else if (gCamera.zoom) {
		if (gCamera.trackballMode)
			gCamera.trackballRadius *= 1.0f + 0.01f * dy;
		else
			gCamera.pos -= gCamera.forward * dy * translateSpeed;
    }

    gMouseX = x;
    gMouseY = y;

	if (gSampleScene != NULL)
		gSampleScene->handleMouseMotion(x, y);
}

// ------------------------------------------------------------------------------------

void InitGlutCallbacks(bool fullscreen, int width, int height)
{
	if (fullScreen) {
		char modeS[500];
		sprintf(modeS, "%dx%d:32@60", width, height);
		glutGameModeString(modeS);
		glutEnterGameMode();
	}
	else {
		gMainHandle = glutCreateWindow("Sample Viewer");
		glutSetWindow(gMainHandle);
	}
	glutDisplayFunc(RenderCallback);
	glutReshapeFunc(ReshapeCallback);
	glutIdleFunc(IdleCallback);
	glutKeyboardFunc(KeyboardCallback);
	glutKeyboardUpFunc(KeyboardUpCallback);
	glutSpecialFunc(SpecialCallback);
	glutMouseFunc(MouseCallback);
	glutMotionFunc(MotionCallback);
	glutPassiveMotionFunc(MotionCallback);
	MotionCallback(0, 0);
}

void SetVSync(bool sync)
{
	typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALPROC)(int);
	PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = 0;

	const char *extensions = (char*)glGetString(GL_EXTENSIONS);

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALPROC)wglGetProcAddress("wglSwapIntervalEXT");

	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(sync);
}

void InitGlut(int argc, char **argv)
{
	//printf("glutInit\n");

    glutInit(&argc, argv);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

	InitGlutCallbacks(fullScreen, WINDOW_WIDTH, WINDOW_HEIGHT);

	
	SetVSync(gUseVSync);

	sampleViewerGamepad.init();

    // Setup default render states
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    //glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
#ifdef WIN32
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
#endif

    // Setup lighting
    glEnable(GL_LIGHTING);
//    float AmbientColor[]    = { 0.0f, 0.1f, 0.2f, 0.0f };         glLightfv(GL_LIGHT0, GL_AMBIENT, AmbientColor);
//    float DiffuseColor[]    = { 0.8f, 0.8f, 0.8f, 0.0f };         glLightfv(GL_LIGHT0, GL_DIFFUSE, DiffuseColor);
//    float SpecularColor[]   = { 0.7f, 0.7f, 0.7f, 0.0f };         glLightfv(GL_LIGHT0, GL_SPECULAR, SpecularColor);

    float AmbientColor[]    = { 0.0f, 0.0f, 0.0f, 0.0f };         glLightfv(GL_LIGHT0, GL_AMBIENT, AmbientColor);
    float DiffuseColor[]    = { 0.6f, 0.6f, 0.6f, 0.0f };         glLightfv(GL_LIGHT0, GL_DIFFUSE, DiffuseColor);
    float SpecularColor[]   = { 0.7f, 0.7f, 0.7f, 0.0f };         glLightfv(GL_LIGHT0, GL_SPECULAR, SpecularColor);

	
	//    float Position[]        = { 100.0f, 100.0f, -400.0f, 1.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);
    //float Position[]        = { 10.0f, 200.0f, 15.0f, 0.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);

	//float Position[]        = { -400,200,300, 0.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);
#if	!TEST_ART_GALLERY_FLOOR
	float Position[]        = { -400,200,300, 0.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);
#else
	float Position[]        = { 674,-300,200, 0.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);
#endif
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);     // so that lighting on scaled objects is correct


	PxVec3 target = PxVec3(0.0f, 8.0f, -25.0f);
	gCamera.forward = target - gCamera.pos;
	gCamera.forward.normalize();

	for (int i = 0; i < gNumLights; i++) 
		gShadowMaps[i] = NULL;

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		printf("glewInit() failed\n");
		exit(0);
	}

	// Setup Fog
	glEnable (GL_FOG);
	float fogColor[4] = {180.0f/255.0f,182.0f/255.0f,202.0f/255.0f,1.0f};
	glFogi (GL_FOG_MODE,  GL_LINEAR);

	glFogf(GL_FOG_DENSITY, 0.0009f);
	glFogf(GL_FOG_START, g_fogStart);
	glFogf(GL_FOG_END, g_fogEnd);
	glFogfv (GL_FOG_COLOR, fogColor);	

	// Load ground texture
	gSplashScreenTexId = 0;

	gDisplayTexArrayProg = new Shader();
	gDisplayTexArrayProg->loadShaderCode(passThruVS, texture2DArrayPS);

	gDisplaySplashProg = new Shader();
	gDisplaySplashProg->loadShaderCode(passThruVS, textureRECTPS);

	if (doAA) {
		g_pAABox = new AABox(gResourcePath);
		g_pAABox->Initialize(WINDOW_WIDTH, WINDOW_HEIGHT, g_curSSSize, g_aaModes[g_curAA].ds, g_aaModes[g_curAA].cs);
	}

	if (gDoHDR)
	{
		ShaderShadow::hdrScale = 2.0f;
	}

	gDefaultShader = new ShaderShadow(ShaderShadow::VS_DEFAULT, ShaderShadow::PS_SHADE);
	gDefaultShader->init();

	/*
	TCHAR pwd[500];
	GetCurrentDirectory(500,pwd);
	MessageBox(NULL,pwd,pwd,0);
*/
	gFXAAHelper = new FXAAHelper(gResourcePath);
	gSSAOHelper = new SSAOHelper(gCamera.fov, 0.1f, zNear, zFar, gResourcePath, doAA ? (1.0f/g_curSSSize) : 1.0f);
	gHBAOHelper = new HBAOHelper(gCamera.fov, zNear, zFar);
	gHDRHelper = new HDRHelper(gCamera.fov, 0.1f, zNear, zFar, gResourcePath, doAA ? (1.0f/g_curSSSize) : 1.0f);

	gDisplayTexArrayProg->loadShaders((string(gResourcePath) + string("\\shaders\\passthrough_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\shadowdebug_fs.cpp")).c_str());
	gDefaultShader->loadShaders((string(gResourcePath) + string("\\shaders\\default_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\default_fs.cpp")).c_str());
	gHDRHelper->mShaderDOF.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\dof_fs.cpp")).c_str());
	gHDRHelper->mShaderBloomH.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\bloomH_fs.cpp")).c_str());
	gHDRHelper->mShaderBloomV.loadShaders((string(gResourcePath) + string("\\shaders\\dof_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\bloomV_fs.cpp")).c_str());
	gSSAOHelper->SSAOFilterH.loadShaders((string(gResourcePath) + string("\\shaders\\filterv_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\filterh_fs.cpp")).c_str());
	gSSAOHelper->SSAOFilterV.loadShaders((string(gResourcePath) + string("\\shaders\\filterv_vs.cpp")).c_str(), (string(gResourcePath) + string("\\shaders\\filterv_fs.cpp")).c_str());
	gForceReloadDebugShader = false;


	if (gSceneNr == 7) {
		gCamera.pos = PxVec3(0.0319193f,3.24621f, 3.47924f);
	}
}

int main(int argc, char** argv)
{
	strcpy(gResourcePath, "../../../externals/resources");
	gCamera.init();

	FILE *f = fopen("resourcePath.txt", "r");
	if (f != NULL) {
		fgets(gResourcePath, 512, f);
		fclose(f);
	}

	const char* usage = "Usage:\n"
		"  -msaa            Do MSAA Antialiasing (Slower, but better quality). On by default. \n"
		"  -fxaa            Do FXAA Antialiasing (Faster, but poorer quality)\n"
		"  -nbThreads n     Use n worker threads in PhysX\n"
		"  -nossao          Disable SSAO\n"
		"  -nhdr            Disable HDR\n"
		"  -grb             Use GRB (default is on)\n"
		"  -nogrb           Use software PhysX\n"
		"  -maxSubSteps n   Enable up to n sub-steps per-frame.\n"
		"  -vsync           Enables vsync (off by default)\n"	
		"\n"
		;
	printf(usage);

	std::string render_opts;
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		std::transform(&arg[0], &arg[0] + arg.length(), &arg[0], tolower);
		if (arg == "-nogrb")  { gUseGrb = false; gCurrentUsingGrb = false; }
		else if (arg == "-grb")  gUseGrb = true;
		else if (arg == "-msaa") {
			doAA = true;
			doFXAA = false;
		}
		else if (arg == "-fxaa") {
			doFXAA = true;
			doAA = false;
		}
		else if (arg == "-maxsubsteps")
		{
			if (argc > (1 + i))
			{
				PxU32 substepCount = atoi(argv[i + 1]);
				gMaxSubSteps = substepCount;
				i++;
			}
		}
		else if (arg == "-nbthreads")
		{
			if (argc > (1 + i))
			{
				PxU32 nbThreads = atoi(argv[i + 1]);
				gNrWorkerThreads = nbThreads;
				i++;
			}
		}
		else if (arg == "-nossao")
		{
			gDoSSAO = false;
		}
		else if (arg == "-nohdr")
		{
			gDoHDR = false;
		}
		else if (arg == "-vsync")
		{
			gUseVSync = true;
		}

	}

	if (!InitPhysX()) {
		ReleaseSDKs();
		return 0;
	}

	InitGlut(argc, argv);

	if (!render_opts.empty()) {
		SampleViewerScene::setRendererOptions(render_opts.c_str());
	}

	// Initialize physics scene and start the application main loop if scene was created

	SampleViewerScene::setRenderType(SampleViewerScene::rtOPENGL);

	gFontRenderer = new GLFontRenderer();

	CreateSampleViewerScene();

	//Hack! When rendering the first frame of the demo, various extremely large graphics buffers are allocated. In GPUs with relatively small amount of memory,
	//these allocations can fail. This seems to relate to CUDA allocations fragmenting the heap. Therefore, we initialize the first frame to use CPU simulation, scheduling a transition to 
	//GPU to ensure that these large allocations succeed. They'll never be reallocated again.
	//Ideally, these allocations would just happen before the GRB scene was initialized but that would require some surgery to the sample framework.
	//bool useGrb = gUseGrb;
	//gUseGrb = false;
	RecreateSDKScenes();
	//gUseGrb = useGrb;

	//gForceSceneRecreate = useGrb;

	InitSampleViewerScene();

	atexit(CleanupSample);

	glutMainLoop();
}
