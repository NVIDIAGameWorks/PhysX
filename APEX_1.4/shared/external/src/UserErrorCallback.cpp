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



#include "ApexDefs.h"
#include "UserErrorCallback.h"
#include "PxAssert.h"
#include <algorithm>

#if PX_WINDOWS_FAMILY
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <ctime>
#endif

#if PX_X360
#include <stdarg.h>
#include <cstdio>
#endif

UserErrorCallback* UserErrorCallback::s_instance /* = NULL */;

UserErrorCallback::UserErrorCallback(const char* filename, const char* mode, bool header, bool reportErrors)
	: mNumErrors(0)
	, mOutFile(NULL)
	, mOutFileName(filename == NULL ? "" : filename)
	, mOutFileMode(mode)
	, mOutFileHeader(header)
	, mReportErrors(reportErrors)
{
	mFirstErrorBuffer[0] = '\0';
	mFirstErrorBufferUpdated = false;
	if (!s_instance)
	{
		s_instance =  this;
	}
	else if (s_instance->mOutFileName.empty())
	{
		// replace stub error handler
		delete s_instance;
		s_instance = this;
	}

	// initialize the filtered messages.
	//mFilteredMessages.insert("");
	addFilteredMessage("CUDA not available", true);
	addFilteredMessage("CUDA is not available", true);

	// filter particle debug visualization warnings from PhysX when APEX is using GPU particles in device exclusive mode
	// the message was changed in 3.3, so adding both
	addFilteredMessage("Operation not allowed in device exclusive mode", true);
	addFilteredMessage("Receiving particles through host interface not supported device exclusive mode", true);

	
	// Filter out a harmless particle warning from PhysX 3.2.4
	addFilteredMessage("Adding particles before the first simulation step is not supported.", true);
}

const char* sanitizeFileName(const char* fullPath)
{
	return std::max(::strrchr(fullPath, '\\'), ::strrchr(fullPath, '/')) + 1;
}

bool UserErrorCallback::messageFiltered(const char * code, const char * msg)
{
	if (0 == strcmp(code, "info"))
	{
		return true;
	}

	std::map<std::string, bool*>::iterator found = mFilteredMessages.find(msg);
	if (found != mFilteredMessages.end())
	{
		if (found->second != NULL)
		{
			// set the trigger
			*(found->second) = true;
		}
		return true;
	}

	for (size_t i = 0; i < mFilteredParts.size(); i++)
	{
		const char* fmsg = mFilteredParts[i].first.c_str();
		if (strstr(msg, fmsg) != NULL)
		{
			if (mFilteredParts[i].second != NULL)
			{
				// set the trigger
				*(mFilteredParts[i].second) = true;
			}
			return true;
		}
	}

	return false;
}

#if PX_WINDOWS_FAMILY

void logf(FILE* file, const char* format, ...)
{
	// size = 2047 from SampleApexApplication::printMessageUser()
	// '\n' appended by UserErrorCallback::printError()
	// '\0' appended by vsnprintf()
	enum { BUFFER_SIZE = 2049 };
	char buffer[BUFFER_SIZE];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	// Output to file
	fputs(buffer, file);

	// Output to debug stream
	::OutputDebugString(buffer);
}

#else

#define logf(file, format, ...) fprintf(file, format, __VA_ARGS__)

#endif /* PX_WINDOWS_FAMILY */

void UserErrorCallback::printError(const char* message, const char* errorCode, const char* file, int line)
{
	if (mOutFile == NULL)
	{
		openFile();
	}

	// do not count eDEBUG_INFO "info" messages as errors.
	// errors trigger benchmarks to be exited.
	if (!messageFiltered(errorCode, message))
	{
		mNumErrors++;
	}

	// if this is the first error while running a benchmark
	if (mNumErrors == 1 && !mFirstErrorBufferUpdated)
	{
		if (errorCode)
		{
			strcpy(mFirstErrorBuffer, errorCode);
			strcat(mFirstErrorBuffer, ": ");
		}
		if (file)
		{
			char lineNumBuf[20];
			strcat(mFirstErrorBuffer, sanitizeFileName(file));
			strcat(mFirstErrorBuffer, ":");
			sprintf(lineNumBuf, "%d: ", line);
			strcat(mFirstErrorBuffer, lineNumBuf);
		}
		if (message)
		{
			strcat(mFirstErrorBuffer, message);
			strcat(mFirstErrorBuffer, "\n");
		}
		mFirstErrorBufferUpdated = true;
	}

	if (mOutFile == NULL)
	{
		return;
	}

	if (errorCode != NULL)
	{
		logf(mOutFile, "\n%s: ", errorCode);
	}

	if (file != NULL)
	{
		logf(mOutFile, "%s:%d:\n", sanitizeFileName(file), line);
	}

	if (message != NULL)
	{
		logf(mOutFile, "%s\n", message);
	}

	fflush(mOutFile);
}

int UserErrorCallback::getNumErrors(void)
{
	return((int)mNumErrors);
}

void UserErrorCallback::clearErrorCounter(void)
{
	mNumErrors = 0;
	mFirstErrorBuffer[0] = '\0';
}

const char* UserErrorCallback::getFirstEror(void)
{
	return(mFirstErrorBuffer);
}

void UserErrorCallback::addFilteredMessage(const char* msg, bool fullMatch, bool* trigger)
{
	if (fullMatch)
	{
		mFilteredMessages.insert(std::pair<std::string, bool*>(msg, trigger));
	}
	else
	{
		mFilteredParts.push_back(std::pair<std::string, bool*>(msg, trigger));
	}
}



void UserErrorCallback::reportErrors(bool enabled)
{
	PX_UNUSED(enabled);
	mReportErrors = false;
}



void UserErrorCallback::reportError(physx::PxErrorCode::Enum e, const char* message, const char* file, int line)
{
	const char* errorCode = NULL;

	switch (e)
	{
	case physx::PxErrorCode::eINVALID_PARAMETER:
		errorCode = "invalid parameter";
		break;
	case physx::PxErrorCode::eINVALID_OPERATION:
		errorCode = "invalid operation";
		break;
	case physx::PxErrorCode::eOUT_OF_MEMORY:
		errorCode = "out of memory";
		break;
	case physx::PxErrorCode::eDEBUG_INFO:
		errorCode = "info";
		break;
	case physx::PxErrorCode::eDEBUG_WARNING:
		errorCode = "warning";
		break;
	default:
		errorCode = "unknown error";
		break;
	}

	PX_ASSERT(errorCode != NULL);
	if (errorCode != NULL)
	{
		printError(message, errorCode, file, line);
	}
}

void UserErrorCallback::printError(physx::PxErrorCode::Enum code, const char* file, int line, const char* fmt, ...)
{
	char buff[2048];
	va_list arg;
	va_start(arg, fmt);
	physx::shdfnd::vsnprintf(buff, sizeof(buff), fmt, arg);
	va_end(arg);
	reportError(code, buff, file, line);
}


void UserErrorCallback::openFile()
{
	if (mOutFile != NULL)
	{
		return;
	}

	if (mOutFileName.empty())
	{
		return;
	}

	PX_ASSERT(mNumErrors == 0);

	if (mOutFileMode == NULL)
	{
		mOutFileMode = "w";
	}

	mOutFile = fopen(mOutFileName.c_str(), mOutFileMode);

	// if that failed, try the temp location on windows
#if PX_WINDOWS_FAMILY
	if (!mOutFile)
	{
		DWORD pathLen = ::GetTempPathA(0, NULL);
		if (pathLen)
		{
			char *pathStr = (char*)malloc(pathLen);
			GetTempPathA(pathLen, pathStr);
			std::string tmpPath(pathStr);
			tmpPath.append(mOutFileName);
			mOutFileName = tmpPath;
			free(pathStr);
			
			::fopen_s(&mOutFile, mOutFileName.c_str(), mOutFileMode);
		}
	}
#endif

	if (mOutFile && mOutFileHeader)
	{
		fprintf(mOutFile,
		        "\n\n"
		        "-------------------------------------\n"
		        "-- new error stream\n");
#if PX_WINDOWS_FAMILY
		char timeBuf[30];
		time_t rawTime;
		time(&rawTime);
		ctime_s(timeBuf, sizeof(timeBuf), &rawTime);
		fprintf(mOutFile,
		        "--\n"
		        "-- %s", timeBuf);
#endif
		fprintf(mOutFile,
		        "-------------------------------------\n\n");

		fflush(mOutFile);
	}
}

UserErrorCallback::~UserErrorCallback()
{
#if PX_WINDOWS_FAMILY
	if (mNumErrors > 0 && mReportErrors)
	{
		std::string errorString;
		char buf[64];
		physx::shdfnd::snprintf(buf, sizeof(buf), "The error callback captured %d errors", mNumErrors);

		errorString.append(buf);
		if (mOutFile != stdout && mOutFile != 0)
		{
			errorString.append(" in ");
			errorString.append(mOutFileName);
		}

		::MessageBoxA(NULL, errorString.c_str(), "UserErrorCallback", MB_OK);
	}
#else
	PX_ASSERT(mNumErrors == 0);
#endif

	if (mOutFile != stdout && mOutFile != 0)
	{
		fclose(mOutFile);
		mOutFile = 0;
	}
}
