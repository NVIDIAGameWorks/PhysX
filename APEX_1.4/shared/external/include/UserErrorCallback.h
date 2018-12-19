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



#ifndef USER_ERROR_CALLBACK_H
#define USER_ERROR_CALLBACK_H

#include "PxErrorCallback.h"
#include "PxErrors.h"
#include <PsString.h>
#include <ApexUsingNamespace.h>

#include <map>
#include <string>
#include <vector>


class UserErrorCallback : public physx::PxErrorCallback
{
public:
	UserErrorCallback(const char* filename, const char* mode, bool header, bool reportErrors);
	~UserErrorCallback();

	void		printError(const char* message, const char* errorCode = NULL, const char* file = NULL, int line = 0);
	void		reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line);
	void		printError(physx::PxErrorCode::Enum code, const char* file, int line, const char* fmt, ...);
	int			getNumErrors();
	void		clearErrorCounter();
	const char*	getFirstEror();
	void		addFilteredMessage(const char* msg, bool fullMatch, bool* trigger = NULL);

	void		reportErrors(bool enabled);

	static UserErrorCallback* instance()
	{
		if (!s_instance)
		{
			// Allocate a stub (bitbucket) error handler
			s_instance = ::new UserErrorCallback(NULL, NULL, false, false);
		}
		return s_instance;
	}

private:
	bool	messageFiltered(const char * code, const char * msg);
	void	openFile();

	uint32_t				mNumErrors;
	FILE*						mOutFile;
	std::string					mOutFileName;
	const char*					mOutFileMode;
	bool						mOutFileHeader;
	bool						mReportErrors;
	char						mFirstErrorBuffer[2048];
	bool						mFirstErrorBufferUpdated;

	std::map<std::string, bool*> mFilteredMessages;
	std::vector<std::pair<std::string, bool*> >	mFilteredParts;

	static UserErrorCallback* s_instance;
};

// gcc uses names ...s
#define ERRORSTREAM_INVALID_PARAMETER(_A, ...) \
	UserErrorCallback::instance()->printError(physx::PxErrorCode::eINVALID_PARAMETER, __FILE__, __LINE__, _A, ##__VA_ARGS__)
#define ERRORSTREAM_INVALID_OPERATION(_A, ...) \
	UserErrorCallback::instance()->printError(physx::PxErrorCode::eINVALID_OPERATION, __FILE__, __LINE__, _A, ##__VA_ARGS__)
#define ERRORSTREAM_DEBUG_ERROR(_A, ...) \
	UserErrorCallback::instance()->printError(physx::PxErrorCode::eINTERNAL_ERROR   , __FILE__, __LINE__, _A, ##__VA_ARGS__)
#define ERRORSTREAM_DEBUG_INFO(_A, ...) \
	UserErrorCallback::instance()->printError(physx::PxErrorCode::eDEBUG_INFO       , __FILE__, __LINE__, _A, ##__VA_ARGS__)
#define ERRORSTREAM_DEBUG_WARNING(_A, ...) \
	UserErrorCallback::instance()->printError(physx::PxErrorCode::eDEBUG_WARNING    , __FILE__, __LINE__, _A, ##__VA_ARGS__)

#endif
