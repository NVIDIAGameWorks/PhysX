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
// Copyright (c) 2008-2018 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  

#include "foundation/PxAssert.h"
#include "RendererConfig.h"
#include "SampleCommandLine.h"
#include "RendererMemoryMacros.h"
#include "SampleAllocator.h"
#include "PhysXSampleApplication.h"
#include "PxTkFile.h"

using namespace SampleFramework;

#if defined(RENDERER_ANDROID)
#include "SamplePlatform.h"

struct android_app;
static android_app* gState;
#endif

#if defined(RENDERER_IOS) || defined(RENDERER_PS3) || defined(RENDERER_MACOSX)
// IOS does not easily support instrumenting the event
// loop like other OSs. Therefore for IOS the (input) event loop
// and the game loop stay for it in the same thread.
//
// PS3 reports a warning when a thread is starved.  This happens
// when gApp tries to start but then needs to wait until the main loop
// yields.  One consequence of this is that the inputs are polled from
// the main loop before they have been initialized.
//
// MACOSX non-deterministically crashed with one thread ran
// SampleRenderer::createGLView (renderer initialization)
// and the other SampleRenderer::emitEvents (processing ESC key event).
// Apparently connecting the glView to the window interfers with
// OSX event processing for some reason.
#define SEPARATE_EVENT_LOOP 0
#else
#define SEPARATE_EVENT_LOOP 1
#endif

static PhysXSampleApplication* gApp = NULL;
static SampleSetup gSettings;
static SampleCommandLine* gSampleCommandLine = NULL;

void mainInitialize()
{
	PX_ASSERT(gSampleCommandLine);
	const SampleCommandLine& cmdline = *gSampleCommandLine;
	initSampleAllocator();
	gApp = SAMPLE_NEW(PhysXSampleApplication)(cmdline);
	
	gApp->customizeSample(gSettings);
#if defined(RENDERER_ANDROID)
	/* We need to register event handling callbacks and process events, while window is not yet shown. */
	if(!SamplePlatform::platform()->preOpenWindow(gState))
	{
		LOG_INFO("SampleMain", "CMD/Input handlers registration failed. Exiting.");
		return;
	}
	while(!SamplePlatform::platform()->isOpen()) 
	{ 
		SamplePlatform::platform()->update();
	}
#endif

	if (gApp->isOpen())
		gApp->close();

	gApp->open(gSettings.mWidth, gSettings.mHeight, gSettings.mName, gSettings.mFullscreen);
#if SEPARATE_EVENT_LOOP
	gApp->start(Ps::Thread::getDefaultStackSize());
#else
	if(gApp->isOpen()) gApp->onOpen();
#endif
}

void mainTerminate()
{
	DELETESINGLE(gApp);
	DELETESINGLE(gSampleCommandLine);
	releaseSampleAllocator();
}

bool mainContinue()
{
	if (gApp->isOpen() && !gApp->isCloseRequested())
	{
		if(gApp->getInputMutex().trylock())
		{
			gApp->handleMouseVisualization();
			gApp->doInput();
			gApp->update();
#if !SEPARATE_EVENT_LOOP
			gApp->updateEngine();
#endif
			gApp->getInputMutex().unlock();
		}
		Ps::Thread::yield();
#if defined(RENDERER_ANDROID)
		if (SamplePlatform::platform()->isOpen())
			return true;
#else
		return true;
#endif
	}

#if SEPARATE_EVENT_LOOP
	gApp->signalQuit();
	gApp->waitForQuit();
#else
	if (gApp->isOpen() || gApp->isCloseRequested())
		gApp->close();
#endif
		
	return false;
}

void mainLoop()
{
	while(mainContinue());
}


#if defined(RENDERER_ANDROID)

	extern "C" void android_main(struct android_app* state)
	{
		gState = state;
		const char* argv[] = { "dummy", 0 };
		gSampleCommandLine = new SampleCommandLine(1, argv);
		mainInitialize();
		
		const char* argFilePath = getSampleMediaFilename("user/androidCmdLine.cfg"); 
		File* argFp = NULL;
		physx::shdfnd::fopen_s(&argFp, argFilePath, "r");
		if (argFp)
		{
			fclose(argFp);
			SampleCommandLine pvdCmdline(1, argv, argFilePath);
			gApp->setPvdParams(pvdCmdline);
		}
	
		mainLoop();
		mainTerminate();
		exit(0);
		/* Will not return return code, because NDK's native_app_glue declares this function to return nothing. Too bad. */
	}

#elif defined(RENDERER_IOS)

	#include "ios/IosSamplePlatform.h"

	int main(int argc, const char *const* argv)
	{
		gSampleCommandLine = new SampleCommandLine((unsigned int)argc, argv);
		SampleFramework::createApplication(mainInitialize, mainContinue, mainTerminate);
		return 0;
	}

#elif PX_XBOXONE

namespace SampleFramework
{
	void createApplication(void (*initializeFunc)(), bool (*updateFunc)(), void (*terminateFunc)(), SampleCommandLine*& commandLine);
}

int main(Platform::Array<Platform::String^>^ args)
{
	SampleFramework::createApplication(mainInitialize, mainContinue, mainTerminate, gSampleCommandLine);
	return 0;
}
#elif defined(RENDERER_WINDOWS)

	int main()
	{
		gSampleCommandLine = new SampleCommandLine(GetCommandLineA());
		mainInitialize();
		mainLoop();
		mainTerminate();
		return 0;
	}

#elif defined(RENDERER_XBOX360)

	int main()
	{
		const char *argv[32];
	    int argc = 0;
	    volatile LPSTR commandLineString;
        commandLineString = GetCommandLine(); // xbox call to get command line argument string

        //skip directly modify the process's share command line string ,in case that it be used in elsewhere
        size_t len = strlen(commandLineString);
        LPSTR cmdString = (LPSTR)alloca(len+1);
        memcpy(cmdString, commandLineString,len);
        cmdString[len] = '\0';

	    /* first pull out the application name argv[0] */
	    argv[argc] = strtok(cmdString, " ");

	    /* pull out the other args */
	    while (argv[argc] != NULL)
	    {
		    argc++;
		    argv[argc] = strtok(NULL, " ");
	    }
		gSampleCommandLine = new SampleCommandLine(argc, argv);
		mainInitialize();
		mainLoop();
		mainTerminate();
		return 0;
	}
	
#elif defined(RENDERER_PS3) || defined(RENDERER_PS4)

	int main(int argc, char** argv)
	{
		gSampleCommandLine = new SampleCommandLine((unsigned int)argc, argv);
		mainInitialize();
		mainLoop();
		mainTerminate();
		return 0;
	}

#elif defined(RENDERER_LINUX)

	int main(int argc, const char *const* argv)
	{
		char* commandString = NULL;
		PxU32 commandLen = 0;
		const char* specialCommand = "--noXterm";
		const char* xtermCommand = "xterm -e ";
		bool foundSpecial = false;
		
		for(PxU32 i = 0; i < (PxU32)argc; i++)
		{
			foundSpecial = foundSpecial || (::strncmp(argv[i], specialCommand, ::strlen(specialCommand)) == 0);
			commandLen += ::strlen(argv[i]);
		}
		
		// extend command line if not chosen differently
		// and start again with terminal as parent
		if(!foundSpecial)
		{
			// increase size by new commands, spaces between commands and string terminator
			commandLen += ::strlen(xtermCommand) + ::strlen(specialCommand) + argc + 3;
			commandString = (char*)::malloc(commandLen * sizeof(char));
			
			::strcpy(commandString, xtermCommand);
			for(PxU32 i = 0; i < (PxU32)argc; i++)
			{
				::strcat(commandString, argv[i]);
				::strcat(commandString, " ");
			}
			::strcat(commandString, specialCommand);

			int ret = ::system(commandString);
			::free(commandString);
			
			if(ret < 0)
				shdfnd::printFormatted("Failed to run %s! If xterm is missing, try running with this parameter: %s\n", argv[0], specialCommand);
		}
		else
		{
			gSampleCommandLine = new SampleCommandLine((unsigned int)(argc), argv);
			mainInitialize();
			mainLoop();
			mainTerminate();
		}

		return 0;
	}

#else

	int main(int argc, const char *const* argv)
	{
		gSampleCommandLine = new SampleCommandLine((unsigned int)argc, argv);
		mainInitialize();
		mainLoop();
		mainTerminate();
		return 0;
	}

#endif
